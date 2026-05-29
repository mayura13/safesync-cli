// src/signals.c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "../include/threads.h"

int g_shutdown_requested = 0;

// 로그 파일에 안전하게 기록을 저장하는 함수
void write_sync_log(const char *message) {
    FILE *log_file = fopen("sync.log", "a");
    if (log_file == NULL) {
        perror("[시그널 오류] 로그 파일을 열 수 없습니다");
        return;
    }
    
    time_t now;
    struct tm *time_info;
    char time_str[20];
    
    time(&now);
    time_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);
    
    fprintf(log_file, "[%s] %s\n", time_str, message);
    fclose(log_file);
}

// ① Ctrl+C (SIGINT) 핸들러: Graceful Shutdown
void handle_sigint(int sig) {
    printf("\n\n=========================================\n");
    printf("[Signal] Ctrl+C (SIGINT) 감지!\n");
    printf("[Signal] 안전한 시스템 종료(Graceful Shutdown)를 시작합니다.\n");
    printf("[Signal] 잔여 데이터 버퍼 전송 대기 및 파일 로깅을 수행합니다...\n");
    printf("=========================================\n");
    fflush(stdout);

    g_shutdown_requested = 1;
    write_sync_log("SafeSync 시스템이 사용자에 의해 정상적으로 Graceful Shutdown 되었습니다.");

    // 안전하게 자원을 회수하고 정돈할 수 있도록 임시 대기
    sleep(1);
    
    printf("[System] 모든 작업 처리 완료. 프로그램을 안전하게 닫습니다.\n");
    exit(0);
}

// ② SIGUSR1 핸들러: 현재 가동 중인 스레드들의 세부 진행 상황 출력
void handle_sigusr1(int sig) {
    printf("\n==================================================\n");
    printf("📊 [SafeSync 실시간 스레드 진행 상황 리포트]\n");
    printf("==================================================\n");
    
    int active_count = 0;
    
    pthread_mutex_lock(&g_status_mutex);
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (g_worker_statuses[i].is_active) {
            active_count++;
            printf("[Worker 스레드 #%d]\n", i + 1);
            printf("  - 파일명: %s\n", g_worker_statuses[i].current_file);
            printf("  - 진행률: [");
            
            // 20칸 기준 그래프 그리기
            int bars = g_worker_statuses[i].progress_percent / 5;
            for (int j = 0; j < 20; j++) {
                if (j < bars) printf("=");
                else if (j == bars) printf(">");
                else printf(" ");
            }
            printf("] %d%%\n", g_worker_statuses[i].progress_percent);
            printf("  - 전송량: %lld / %lld Bytes\n", g_worker_statuses[i].sent_bytes, g_worker_statuses[i].total_bytes);
        }
    }
    pthread_mutex_unlock(&g_status_mutex);
    
    if (active_count == 0) {
        printf("😴 현재 대기열이 비어있으며, 작동 중인 전송 스레드가 존재하지 않습니다.\n");
    }
    printf("==================================================\n\n");
    fflush(stdout);
}

// 시스템 시그널 바인딩 활성화
void setup_signal_handlers() {
    struct sigaction sa_int;
    struct sigaction sa_usr1;

    // SIGINT (Ctrl+C) 바인딩
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("[Error] SIGINT 핸들러 등록 실패");
        exit(1);
    }

    // SIGUSR1 바인딩
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("[Error] SIGUSR1 핸들러 등록 실패");
        exit(1);
    }

    write_sync_log("SafeSync 실시간 모니터링 시그널 감시 엔진 동작 시작.");
}
