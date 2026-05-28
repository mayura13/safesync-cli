#ifndef WATCHER_H
#define WATCHER_H

#include <time.h>

#define MAX_FILES 256
#define PATH_LEN 512

// 파일의 상태를 추적하기 위한 구조체 정의
typedef struct {
    char filepath[256];
    time_t last_mtime; // 마지막 수정 시간 (stat.st_mtime)
    int is_checked;
} FileInfo;

// 디렉토리 내 변경 사항을 감시하는 함수 프로토타입
void start_monitoring(const char *target_dir);

#endif
