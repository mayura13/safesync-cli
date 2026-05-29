// src/threads.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include "../include/network.h"
#include "../include/queue.h"
#include "../include/threads.h"

#define BUFFER_SIZE 4096

pthread_mutex_t screen_lock;
WorkerStatus g_worker_statuses[MAX_WORKERS];
pthread_mutex_t g_status_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_mutex(void) {
    pthread_mutex_init(&screen_lock, NULL);
    
    pthread_mutex_lock(&g_status_mutex);
    for (int i = 0; i < MAX_WORKERS; i++) {
        g_worker_statuses[i].thread_id = 0;
        g_worker_statuses[i].is_active = 0;
        memset(g_worker_statuses[i].current_file, 0, sizeof(g_worker_statuses[i].current_file));
        g_worker_statuses[i].total_bytes = 0;
        g_worker_statuses[i].sent_bytes = 0;
        g_worker_statuses[i].progress_percent = 0;
    }
    pthread_mutex_unlock(&g_status_mutex);
}

void destroy_mutex(void) {
    pthread_mutex_destroy(&screen_lock);
}

int get_empty_worker_slot(pthread_t tid) {
    int slot = -1;
    pthread_mutex_lock(&g_status_mutex);
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (!g_worker_statuses[i].is_active) {
            g_worker_statuses[i].is_active = 1;
            g_worker_statuses[i].thread_id = tid;
            slot = i;
            break;
        }
    }
    pthread_mutex_unlock(&g_status_mutex);
    return slot;
}

void update_worker_status(int worker_idx, const char* filename, long long total, long long sent, int percent) {
    if (worker_idx < 0 || worker_idx >= MAX_WORKERS) return;
    
    pthread_mutex_lock(&g_status_mutex);
    strncpy(g_worker_statuses[worker_idx].current_file, filename, sizeof(g_worker_statuses[worker_idx].current_file) - 1);
    g_worker_statuses[worker_idx].total_bytes = total;
    g_worker_statuses[worker_idx].sent_bytes = sent;
    g_worker_statuses[worker_idx].progress_percent = percent;
    pthread_mutex_unlock(&g_status_mutex);
}

void clear_worker_status(int worker_idx) {
    if (worker_idx < 0 || worker_idx >= MAX_WORKERS) return;
    
    pthread_mutex_lock(&g_status_mutex);
    g_worker_statuses[worker_idx].is_active = 0;
    g_worker_statuses[worker_idx].thread_id = 0;
    memset(g_worker_statuses[worker_idx].current_file, 0, sizeof(g_worker_statuses[worker_idx].current_file));
    g_worker_statuses[worker_idx].total_bytes = 0;
    g_worker_statuses[worker_idx].sent_bytes = 0;
    g_worker_statuses[worker_idx].progress_percent = 0;
    pthread_mutex_unlock(&g_status_mutex);
}

void* thread_sync_worker(void* arg) {
    SyncTask* task = (SyncTask*)arg;
    int file_fd;
    struct stat file_stat;
    char buffer[BUFFER_SIZE];
    ssize_t read_bytes, written_bytes;
    off_t resume_offset = 0;

    pthread_t tid = pthread_self();
    int slot = get_empty_worker_slot(tid);

    file_fd = open(task->filepath, O_RDONLY);
    if (file_fd < 0 || fstat(file_fd, &file_stat) < 0) {
        pthread_mutex_lock(&screen_lock);
        perror("[스레드 오류] 파일 열기 또는 stat 실패");
        pthread_mutex_unlock(&screen_lock);
        close(task->client_socket);
        if (slot != -1) clear_worker_status(slot);
        free(task);
        pthread_exit(NULL);
    }

    char *pure_filename = strrchr(task->filepath, '/');
    pure_filename = (pure_filename != NULL) ? pure_filename + 1 : task->filepath;

    if (slot != -1) {
        update_worker_status(slot, pure_filename, file_stat.st_size, 0, 0);
    }

    FileHeader header;
    header.name_len = strlen(task->filepath);
    header.file_size = file_stat.st_size;

    write(task->client_socket, &header, sizeof(FileHeader));
    write(task->client_socket, task->filepath, header.name_len);

    read(task->client_socket, &resume_offset, sizeof(off_t));

    if (resume_offset > 0 && resume_offset < file_stat.st_size) {
        lseek(file_fd, resume_offset, SEEK_SET);
        pthread_mutex_lock(&screen_lock);
        printf("\n[Partial Sync] '%s' 파일을 %lld 바이트부터 이어서 전송합니다.\n", 
                pure_filename, (long long)resume_offset);
        fflush(stdout);
        pthread_mutex_unlock(&screen_lock);
    } else if (resume_offset >= file_stat.st_size) {
        pthread_mutex_lock(&screen_lock);
        printf("\n[스킵] '%s'는 이미 최신 상태입니다.\n", pure_filename);
        fflush(stdout);
        pthread_mutex_unlock(&screen_lock);
        close(file_fd);
        close(task->client_socket);
        if (slot != -1) clear_worker_status(slot);
        free(task);
        pthread_exit(NULL);
    }

    off_t total_sent = resume_offset;
    int last_percent = -1;

    while (total_sent < file_stat.st_size && (read_bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        if (total_sent + read_bytes > file_stat.st_size) {
            read_bytes = file_stat.st_size - total_sent;
        }

        usleep(1200);

        written_bytes = write(task->client_socket, buffer, read_bytes);
        if (written_bytes <= 0) break;
        
        total_sent += written_bytes;
        int percent = (int)((total_sent * 100) / file_stat.st_size);
        if (percent > 100) percent = 100;
        
        if (slot != -1) {
            update_worker_status(slot, pure_filename, file_stat.st_size, total_sent, percent);
        }

        if (percent != last_percent) {
            pthread_mutex_lock(&screen_lock);
            printf("\r[전송 중] %s -> %d%% 완료 (%lld / %lld 바이트)", 
                   pure_filename, percent, (long long)total_sent, (long long)file_stat.st_size);
            fflush(stdout);
            pthread_mutex_unlock(&screen_lock);
            last_percent = percent;
        }
    }

    close(file_fd);
    close(task->client_socket);
    
    if (slot != -1) clear_worker_status(slot);
    
    pthread_mutex_lock(&screen_lock);
    printf("\n[스레드 %lu] '%s' 파일 동기화 완료!\n", (unsigned long)tid, pure_filename);
    fflush(stdout);
    pthread_mutex_unlock(&screen_lock);
    
    free(task);
    pthread_exit(NULL);
}

void* sync_dispatcher_thread(void* arg) {
    char target_file[PATH_LEN];

    while(1) {
        pop_queue(target_file);

        int sock = connect_to_server("127.0.0.1", 8080);
        if (sock < 0) {
            printf("[경고] 서버 연결 실패. '%s' 전송 대기.\n", target_file);
            fflush(stdout);
            continue; 
        }

        SyncTask* task = malloc(sizeof(SyncTask));
        task->client_socket = sock;
        strcpy(task->filepath, target_file);

        pthread_t worker_tid;
        pthread_create(&worker_tid, NULL, thread_sync_worker, (void*)task);
        pthread_detach(worker_tid); 
    }
    return NULL;
}

void start_network_engine() {
    init_mutex();
    pthread_t disp_tid;
    pthread_create(&disp_tid, NULL, sync_dispatcher_thread, NULL);
    pthread_detach(disp_tid);
    printf("[Network] Multi-threaded Sync Engine 및 통신 프로토콜 가동 완료.\n");
    fflush(stdout);
}
