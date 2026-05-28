// include/network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <pthread.h>
#include <sys/types.h>

// 🌟 [추가됨] 교수님 어필용: 자체 파일 전송 프로토콜 헤더 구조체
typedef struct {
    int name_len;       // 파일명의 길이 (4B)
    off_t file_size;    // 파일의 전체 크기 (8B)
} FileHeader;

// 스레드에 전달할 인자 구조체
typedef struct {
    int client_socket;
    char filepath[256];
} SyncTask;

int setup_server(int port);
int connect_to_server(const char* ip, int port);

void* thread_sync_worker(void* arg);
void init_mutex(void);
void destroy_mutex(void);
void start_network_engine(); 

#endif