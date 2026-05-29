// include/threads.h
#ifndef THREADS_H
#define THREADS_H

#include <pthread.h>
#include <sys/types.h>

// 현재 전송 중인 워커 스레드의 상태를 기록하는 구조체
typedef struct {
    pthread_t thread_id;
    int is_active;
    char current_file[256];
    long long total_bytes;
    long long sent_bytes;
    int progress_percent;
} WorkerStatus;

// 최대 워커 스레드 수 (동시 전송 한도)
#define MAX_WORKERS 4

extern WorkerStatus g_worker_statuses[MAX_WORKERS];
extern pthread_mutex_t g_status_mutex;
extern int g_shutdown_requested;

void init_mutex(void);
void destroy_mutex(void);
int get_empty_worker_slot(pthread_t tid);
void update_worker_status(int worker_idx, const char* filename, long long total, long long sent, int percent);
void clear_worker_status(int worker_idx);

#endif
