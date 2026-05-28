// include/queue.h
#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

#define QUEUE_SIZE 100
#define PATH_LEN 512

// 동기화 대상 파일을 담을 스레드 안전한 큐 구조체 정의
typedef struct {
    char filepaths[QUEUE_SIZE][PATH_LEN];
    int front;
    int rear;
    int count;
    pthread_mutex_t lock;      // 공유 자원 보호를 위한 뮤텍스
    pthread_cond_t cond;       // 큐가 비어있을 때 전송 스레드를 대기시키기 위한 조건 변수
} SyncQueue;

// 전역 큐 선언 (main.c에서 초기화 예정)
extern SyncQueue sync_queue;

// 큐 관련 함수 프로토타입
void init_queue();
void push_queue(const char *path);
void pop_queue(char *path);

#endif
