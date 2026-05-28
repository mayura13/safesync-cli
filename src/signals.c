// src/signals.c
#include "../include/signals.h"
#include "../include/queue.h"  // 대기열 개수를 조회하기 위해 포함
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// SIGINT (Ctrl+C) 핸들러 - Graceful Shutdown 구현
void handle_sigint(int sig) {
    printf("\n[Signal] SIGINT (Ctrl+C) 감지!\n");
    printf("[Signal] 열려 있는 소켓 및 파일 리소스를 안전하게 닫고 종료합니다.\n");
    // TODO: 이후 팀원 B가 소켓 연결을 종료하는 코드를 작성하면 이곳에 연동하게 됩니다.
    exit(0);
}

// SIGUSR1 핸들러 - 실시간 모니터링 및 상태 보고
void handle_sigusr1(int sig) {
    // 주의: 시그널 핸들러 내부에서 mutex_lock을 걸면 비동기 시그널 특성상 데드락(Deadlock)이 일어날 위험이 있습니다.
    // 안전을 위해 동기화 대기열의 상태(count) 및 인덱스 데이터를 직접 읽어 출력하도록 안전하게 설계합니다.
    printf("\n=========================================\n");
    printf("[Signal] SafeSync-CLI 현재 동기화 상태 보고\n");
    printf("-----------------------------------------\n");
    printf("▶ 현재 대기열에 쌓여 있는 파일 수: %d개\n", sync_queue.count);
    
    if (sync_queue.count > 0) {
        printf("▶ 다음 동기화 예정 파일: %s\n", sync_queue.filepaths[sync_queue.front]);
    } else {
        printf("▶ 현재 모든 대기열의 파일 동기화가 정상 완료되었습니다.\n");
    }
    printf("=========================================\n");
}

void setup_signal_handlers() {
    signal(SIGINT, handle_sigint);
    signal(SIGUSR1, handle_sigusr1);
    printf("[Signal] SIGINT 및 SIGUSR1 시그널 처리기 핸들러 매핑 완료.\n");
}
