#include <stdio.h>
#include "../include/watcher.h"
#include "../include/signals.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("사용법: %s <감시할_디렉토리_경로>\n", argv[0]);
        return 1;
    }

    // 1. 시그널 핸들러 세팅
    setup_signal_handlers();

    // 2. 모니터링 루프 시작
    start_monitoring(argv[1]);

    return 0;
}
