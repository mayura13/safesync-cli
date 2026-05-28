// src/watcher.c
#include "../include/watcher.h"
#include "../include/queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // 시간 변환 함수 라이브러리 추가
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

// 모니터링중인 파일들의 데이터베이스
FileInfo file_db[MAX_FILES];
int file_count = 0;

// Unix 타임스탬프를 "YYYY-MM-DD HH:MM:SS" 형식으로 변환하는 함수
void format_time(time_t t, char *buf, size_t maxsize) {
    struct tm *tm_info = localtime(&t);
    // %Y: 연도, %m: 월, %d: 일, %H: 시, %M: 분, %S: 초
    strftime(buf, maxsize, "%Y-%m-%d %H:%M:%S", tm_info);
}

// 특정 파일이 이미 DB에 존재하는지 확인하고 인덱스를 반환하는 함수
int find_file_index(const char *path) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(file_db[i].filepath, path) == 0) {
            return i;
        }
    }
    return -1; // 존재하지 않음
}

// 디렉토리를 1회 스캔하며 파일의 신규, 수정, 삭제 이벤트를 탐지하는 함수
void scan_directory(const char *dir_path, int is_initial) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("[Watcher Error] 디렉토리를 열 수 없습니다");
        return;
    }

    // 모든 파일의 체크 플래그 초기화
    for (int i = 0; i < file_count; i++) {
        file_db[i].is_checked = 0;
    }

    struct dirent *entry;
    struct stat file_stat;
    char full_path[PATH_LEN];

    // 디렉토리 내의 항목들을 순회하며 검사
    while ((entry = readdir(dir)) != NULL) {
        // 상위 디렉토리 및 현재 디렉토리 링크는 감시 제외
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 전체 파일 경로 생성
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        // 파일 상세 정보(stat) 획득
        if (stat(full_path, &file_stat) == -1) {
            continue;
        }

        // 디렉토리가 아닌 일반 파일(Regular File)만 감시 대상으로 설정
        if (S_ISREG(file_stat.st_mode)) {
            int idx = find_file_index(full_path);

            if (idx == -1) {
                // 새로운 파일이 생성된 경우 DB에 새로 등록
                if (file_count < MAX_FILES) {
                    strcpy(file_db[file_count].filepath, full_path);
                    file_db[file_count].last_mtime = file_stat.st_mtime;
                    file_db[file_count].is_checked = 1;
                    file_count++;
                    
                    // 초기 프로그램 실행 시점의 전송 대량 발생 방지
                    if (!is_initial) {
                        char time_str[32];
                        format_time(file_stat.st_mtime, time_str, sizeof(time_str));
                        printf("[Watcher] 신규 파일 감지: %s (생성시간: %s)\n", entry->d_name, time_str);
                        push_queue(full_path); // 전송 대기열 큐에 푸시
                    }
                }
            } else {
                // 이미 추적 중인 파일인 경우 생존 플래그 활성화
                file_db[idx].is_checked = 1;

                // 마지막 저장 시간과 비교하여 수정되었는지 검사
                if (file_stat.st_mtime > file_db[idx].last_mtime) {
                    char old_time[32];
                    char new_time[32];
                    format_time(file_db[idx].last_mtime, old_time, sizeof(old_time));
                    format_time(file_stat.st_mtime, new_time, sizeof(new_time));

                    printf("[Watcher] 파일 수정 감지: %s (수정시간: %s -> %s)\n", 
                           entry->d_name, old_time, new_time);
                    
                    // 데이터베이스 시간 정보 갱신
                    file_db[idx].last_mtime = file_stat.st_mtime;
                    push_queue(full_path); // 전송 대기열 큐에 푸시
                }
            }
        }
    }
    closedir(dir);

    // 스캔 종료 후 DB에는 있으나 실제 디렉토리에는 없는 파일 탐지 (삭제 처리)
    for (int i = 0; i < file_count; i++) {
        if (file_db[i].is_checked == 0) {
            printf("[Watcher] 파일 삭제 감지: %s\n", file_db[i].filepath);
            
            // 데이터베이스에서 해당 데이터 제거 (가장 뒤에 있는 데이터로 대체)
            if (i != file_count - 1) {
                file_db[i] = file_db[file_count - 1];
                i--; // 인덱스를 뒤로 돌려 교체된 데이터 재검증
            }
            file_count--;
        }
    }
}

// 지속적으로 디렉토리를 주기적 폴링(2초)하며 감시하는 스레드 함수 기반 구조
void start_monitoring(const char *target_dir) {
    printf("[Watcher] '%s' 디렉토리 실시간 감시를 시작합니다.\n", target_dir);
    printf("[Watcher] 종료하려면 Ctrl+C를 누르세요.\n");
    
    // 초기 스캔을 진행하여 감시 시작 전 파일 목록 초기 세팅 (is_initial = 1)
    scan_directory(target_dir, 1);
    printf("[Watcher] 초기 탐색 완료. 파일 %d개 추적 중.\n", file_count);
    printf("-----------------------------------------\n");

    while (1) {
        sleep(2); // 2초 주기 설정
        scan_directory(target_dir, 0); // 모니터링 모드로 주기적 스캔 진행
    }
}
