// src/queue.c
#include "../include/queue.h"
#include <stdio.h>
#include <string.h>

SyncQueue sync_queue;

// 큐 초기화 함수
void init_queue() {
    sync_queue.front = 0;
    sync_queue.rear = 0;
    sync_queue.count = 0;
    pthread_mutex_init(&sync_queue.lock, NULL);
    pthread_cond_init(&sync_queue.cond, NULL);
    printf("[Queue] 전송 대기열(Mutex & Condition Variable) 초기화 완료.\n");
}

// 감시 엔진(Watcher)이 호출하여 변경된 파일을 대기열에 넣는 함수 (Producer)
void push_queue(const char *path) {
    pthread_mutex_lock(&sync_queue.lock);

    // 큐가 가득 찬 경우 예외 처리
    if (sync_queue.count >= QUEUE_SIZE) {
        printf("[Queue Warning] 대기열이 가득 찼습니다. %s 전송 대기 누락\n", path);
        pthread_mutex_unlock(&sync_queue.lock);
        return;
    }

    // 큐에 파일 경로 복사
    strncpy(sync_queue.filepaths[sync_queue.rear], path, PATH_LEN - 1);
    sync_queue.filepaths[sync_queue.rear][PATH_LEN - 1] = '\0';
    
    // 원형 큐 인덱스 조정
    sync_queue.rear = (sync_queue.rear + 1) % QUEUE_SIZE;
    sync_queue.count++;

    printf("[Queue] 대기열 추가 완료 (대기건수: %d): %s\n", sync_queue.count, path);

    // 대기 중인 네트워크 전송 스레드가 있다면 깨움
    pthread_cond_signal(&sync_queue.cond);

    pthread_mutex_unlock(&sync_queue.lock);
}

// 팀원 B가 구현할 네트워크 전송 스레드가 호출하여 전송할 파일을 꺼내는 함수 (Consumer)
void pop_queue(char *path) {
    pthread_mutex_lock(&sync_queue.lock);

    // 큐가 비어있으면 데이터가 들어올 때까지 대기 (CPU 자원을 소모하지 않고 휴식 상태로 대기)
    while (sync_queue.count == 0) {
        pthread_cond_wait(&sync_queue.cond, &sync_queue.lock);
    }

    // 큐에서 파일 경로 추출
    strncpy(path, sync_queue.filepaths[sync_queue.front], PATH_LEN - 1);
    
    // 원형 큐 인덱스 조정
    sync_queue.front = (sync_queue.front + 1) % QUEUE_SIZE;
    sync_queue.count--;

    printf("[Queue] 대기열에서 전송 대상 추출 (남은건수: %d): %s\n", sync_queue.count, path);

    pthread_mutex_unlock(&sync_queue.lock);
}
