#include "../include/signals.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// SIGINT (Ctrl+C) 핸들러 - Graceful Shutdown 구현 예정
void handle_sigint(int sig) {
    printf("\n[Signal] SIGINT 감지! 안전하게 프로세스를 종료합니다.\n");
    // TODO: 소켓 정리 및 대기열 비우기 코드 추가 예정
    exit(0);
}

// SIGUSR1 핸들러 - 상태 리포팅 구현 예정
void handle_sigusr1(int sig) {
    printf("\n[Signal] 현재 동기화 상태: 대기열이 비어있거나 전송 완료 상태입니다.\n");
}

void setup_signal_handlers() {
    signal(SIGINT, handle_sigint);
    signal(SIGUSR1, handle_sigusr1);
    printf("[Signal] SIGINT 및 SIGUSR1 핸들러 등록 완료.\n");
}
