#ifndef NETWORK_H
#define NETWORK_H

#include <pthread.h>
#include <sys/types.h>

// 스레드에 전달할 인자 구조체 (어떤 파일을, 어디서부터 보낼지)
typedef struct {
    int client_socket;
    char filepath[256];
    off_t resume_offset; // 이어받기를 위한 오프셋 (lseek 용)
} SyncTask;

// 네트워크 관련 함수 선언
int setup_server(int port);
int connect_to_server(const char* ip, int port);

// 스레드 관련 함수 선언
void* thread_sync_worker(void* arg);
void init_mutex(void);
void destroy_mutex(void);

#endif