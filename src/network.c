#ifndef NETWORK_H
#define NETWORK_H

#include <pthread.h>
#include <sys/types.h>

typedef struct {
    int client_socket;
    char filepath[256];
    off_t resume_offset; 
} SyncTask;

int setup_server(int port);
int connect_to_server(const char* ip, int port);

// 스레드 및 엔진 관련 함수 선언
void* thread_sync_worker(void* arg);
void init_mutex(void);
void destroy_mutex(void);

// 메인에서 네트워크 엔진을 켜는 스위치
void start_network_engine(); 

#endif