#include "../include/watcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

void start_monitoring(const char *target_dir) {
    printf("[Watcher] %s 디렉토리 실시간 모니터링을 시작합니다...\n", target_dir);
    // TODO: opendir(), readdir(), stat()을 활용한 감시 루프 구현 예정
}
