// src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "../include/watcher.h"
#include "../include/signals.h"
#include "../include/queue.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("사용법: %s <감시할_디렉토리_경로>\n", argv[0]);
        fflush(stdout);
        return 1;
    }

    // 1. 시그널 핸들러 세팅
    setup_signal_handlers();

    // 2. 전송 대기열(Queue) 초기화
    init_queue();

    // canonical 절대 경로 변환 실행
    char resolved_path[PATH_MAX];
    if (realpath(argv[1], resolved_path) == NULL) {
        perror("[Error] 입력된 경로의 실제 절대 경로를 해석할 수 없습니다");
        fflush(stderr);
        return 1;
    }

    // 3. 변환된 절대 경로로 모니터링 루프 시작
    start_monitoring(resolved_path);

    return 0;
}
