#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "network.h"

#define BUFFER_SIZE 4096

// 스레드들이 화면(printf)이나 공용 변수를 동시에 건드리지 못하게 막는 자물쇠
pthread_mutex_t screen_lock;

void init_mutex(void) {
    pthread_mutex_init(&screen_lock, NULL);
}

void destroy_mutex(void) {
    pthread_mutex_destroy(&screen_lock);
}

// pthread_create()로 실행될 스레드 작업 함수 (파일 병렬 전송)
void* thread_sync_worker(void* arg) {
    SyncTask* task = (SyncTask*)arg;
    int file_fd;
    ssize_t read_bytes, written_bytes;
    char buffer[BUFFER_SIZE];

    // 1. 파일 열기 (읽기 전용)
    file_fd = open(task->filepath, O_RDONLY);
    if (file_fd < 0) {
        pthread_mutex_lock(&screen_lock);
        perror("File open failed");
        pthread_mutex_unlock(&screen_lock);
        free(task);
        pthread_exit(NULL);
    }

    // 2. lseek 시스템 콜: 파일 커서 이동 (이어받기 핵심)
    // task->resume_offset 위치로 훅 건너뜀 (처음부터면 0)
    if (task->resume_offset > 0) {
        off_t current_pos = lseek(file_fd, task->resume_offset, SEEK_SET);
        
        pthread_mutex_lock(&screen_lock);
        printf("[스레드 %lu] '%s' 파일을 %lld 바이트부터 이어 보내기 시작합니다.\n", 
                (unsigned long)pthread_self(), task->filepath, (long long)current_pos);
        pthread_mutex_unlock(&screen_lock);
    }

    // 3. 파일 읽어서 소켓으로 쓰기 (read, write)
    while ((read_bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        written_bytes = write(task->client_socket, buffer, read_bytes);
        
        if (written_bytes <= 0) {
            pthread_mutex_lock(&screen_lock);
            printf("[스레드 %lu] 통신 오류 발생. 전송 중단.\n", (unsigned long)pthread_self());
            pthread_mutex_unlock(&screen_lock);
            break; // 네트워크 끊기면 전송 중단
        }
    }

    // 4. 마무리 정리
    close(file_fd);
    
    pthread_mutex_lock(&screen_lock);
    printf("[스레드 %lu] '%s' 파일 전송 완료!\n", (unsigned long)pthread_self(), task->filepath);
    pthread_mutex_unlock(&screen_lock);
    
    free(task); // 동적 할당된 메모리 해제
    pthread_exit(NULL); // 스레드 종료
}