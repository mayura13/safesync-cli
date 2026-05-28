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

#define BUFFER_SIZE 4096

pthread_mutex_t screen_lock;

void init_mutex(void) {
    pthread_mutex_init(&screen_lock, NULL);
}

void destroy_mutex(void) {
    pthread_mutex_destroy(&screen_lock);
}

// 🌟 [핵심] 이어받기 및 패킷 통신이 적용된 워커 스레드
void* thread_sync_worker(void* arg) {
    SyncTask* task = (SyncTask*)arg;
    int file_fd;
    struct stat file_stat;
    char buffer[BUFFER_SIZE];
    ssize_t read_bytes, written_bytes;
    off_t resume_offset = 0;

    // 1. 전송할 파일 열기 및 크기 확인 (stat)
    file_fd = open(task->filepath, O_RDONLY);
    if (file_fd < 0 || fstat(file_fd, &file_stat) < 0) {
        pthread_mutex_lock(&screen_lock);
        perror("[스레드 오류] 파일 열기 또는 stat 실패");
        pthread_mutex_unlock(&screen_lock);
        close(task->client_socket);
        free(task);
        pthread_exit(NULL);
    }

    // 2. 패킷 헤더(Header) 생성 및 전송
    FileHeader header;
    header.name_len = strlen(task->filepath);
    header.file_size = file_stat.st_size;

    write(task->client_socket, &header, sizeof(FileHeader));           // 헤더 구조체 전송
    write(task->client_socket, task->filepath, header.name_len);       // 가변 길이 파일명 전송

    // 3. 서버로부터 이어받기 시작점(offset) 응답 받기 (Handshake)
    read(task->client_socket, &resume_offset, sizeof(off_t));

    // 4. Partial Sync (lseek 적용)
    if (resume_offset > 0 && resume_offset < file_stat.st_size) {
        lseek(file_fd, resume_offset, SEEK_SET);
        pthread_mutex_lock(&screen_lock);
        printf("[Partial Sync] '%s' 파일을 %lld 바이트부터 이어서 전송합니다.\n", 
                task->filepath, (long long)resume_offset);
        pthread_mutex_unlock(&screen_lock);
    } else if (resume_offset >= file_stat.st_size) {
        // 이미 서버에 파일이 완벽히 존재할 경우 전송 생략
        close(file_fd);
        close(task->client_socket);
        free(task);
        pthread_exit(NULL);
    }

    // 5. 실제 파일 데이터(Body) 전송 루프
    while ((read_bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        written_bytes = write(task->client_socket, buffer, read_bytes);
        if (written_bytes <= 0) break; // 네트워크 단절 시 루프 탈출
    }

    close(file_fd);
    close(task->client_socket);
    
    pthread_mutex_lock(&screen_lock);
    printf("[스레드 %lu] '%s' 파일 동기화 완료!\n", (unsigned long)pthread_self(), task->filepath);
    pthread_mutex_unlock(&screen_lock);
    
    free(task);
    pthread_exit(NULL);
}

// 큐에서 파일을 빼서 서버로 연결을 시도하는 매니저 스레드
void* sync_dispatcher_thread(void* arg) {
    char target_file[PATH_LEN];

    while(1) {
        pop_queue(target_file);

        // 서버 연결 (실제 구동 시에는 서버 IP를 인자로 받도록 수정 가능)
        int sock = connect_to_server("127.0.0.1", 8080);
        if (sock < 0) {
            printf("[경고] 서버 연결 실패. '%s' 전송 대기.\n", target_file);
            // 실패 시 큐에 다시 넣거나 재시도하는 로직을 추후 추가할 수 있음
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
}