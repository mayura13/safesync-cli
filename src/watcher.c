// src/watcher.c
#include "../include/watcher.h"
#include "../include/queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

// 모니터링중인 파일들의 데이터베이스
FileInfo file_db[MAX_FILES];
int file_count = 0;

// Unix 타임스탬프를 "YYYY-MM-DD HH:MM:SS" 형식으로 변환하는 함수
void format_time(time_t t, char *buf, size_t maxsize) {
    struct tm *tm_info = localtime(&t);
    strftime(buf, maxsize, "%Y-%m-%d %H:%M:%S", tm_info);
}

// 특정 파일이 이미 DB에 존재하는지 확인하고 인덱스를 반환하는 함수
int find_file_index(const char *path) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(file_db[i].filepath, path) == 0) {
            return i;
        }
    }
    return -1;
}

// 디렉토리를 재귀적으로 스캔하며 파일의 신규, 수정, 삭제 이벤트를 탐지하는 함수
void scan_directory(const char *dir_path, int is_initial) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        fprintf(stderr, "[Watcher Warning] 디렉토리 열기 실패: %s (이유: %s)\n", dir_path, strerror(errno));
        fflush(stderr);
        return;
    }

    struct dirent *entry;
    struct stat file_stat;
    char full_path[PATH_LEN];

    // 디렉토리 내의 모든 항목들을 순회하며 검사
    while ((entry = readdir(dir)) != NULL) {
        // 상위 디렉토리 및 현재 디렉토리 링크는 스캔에서 제외
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 탐색 대상의 전체 파일 경로 생성
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        // 파일 상세 정보(lstat) 획득
        if (lstat(full_path, &file_stat) == -1) {
            continue;
        }

        // 심볼릭 링크 파일인 경우 디렉토리 무한 순환 참조 방지를 위해 무시
        if (S_ISLNK(file_stat.st_mode)) {
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            // 하위 디렉토리일 경우 안으로 파고들어 다시 재귀 스캔 진행
            scan_directory(full_path, is_initial);
        } else if (S_ISREG(file_stat.st_mode)) {
            // 일반 파일인 경우 감시 데이터베이스와 대조 진행
            int idx = find_file_index(full_path);

            if (idx == -1) {
                // 새로운 파일이 생성된 경우 DB에 등록
                if (file_count < MAX_FILES) {
                    strcpy(file_db[file_count].filepath, full_path);
                    file_db[file_count].last_mtime = file_stat.st_mtime;
                    file_db[file_count].is_checked = 1;
                    file_count++;
                    
                    // 프로그램 시작 이후에 생성된 파일만 대기열에 푸시 및 출력
                    if (!is_initial) {
                        char time_str[32];
                        format_time(file_stat.st_mtime, time_str, sizeof(time_str));
                        printf("[Watcher] 신규 파일 감지: %s (위치: %s, 생성시간: %s)\n", entry->d_name, dir_path, time_str);
                        fflush(stdout); // 터미널에 즉시 출력 강제 실행
                        push_queue(full_path);
                    }
                }
            } else {
                // 이미 DB에 존재하는 파일이면 생존 확인 플래그 처리
                file_db[idx].is_checked = 1;

                // 마지막 저장 시간과 비교하여 수정되었는지 검사
                if (file_stat.st_mtime > file_db[idx].last_mtime) {
                    char old_time[32];
                    char new_time[32];
                    format_time(file_db[idx].last_mtime, old_time, sizeof(old_time));
                    format_time(file_stat.st_mtime, new_time, sizeof(new_time));

                    printf("[Watcher] 파일 수정 감지: %s (수정시간: %s -> %s)\n", 
                           entry->d_name, old_time, new_time);
                    fflush(stdout); // 터미널에 즉시 출력 강제 실행
                    
                    file_db[idx].last_mtime = file_stat.st_mtime;
                    push_queue(full_path);
                }
            }
        }
    }
    closedir(dir);
}

// 2초마다 무한히 돌며 전체 파일 시스템을 추적 및 리프레시하는 동기화 메인 루프
void start_monitoring(const char *target_dir) {
    printf("[Watcher] '%s' 디렉토리 실시간 감시를 시작합니다.\n", target_dir);
    printf("[Watcher] 종료하려면 Ctrl+C를 누르세요.\n");
    fflush(stdout);
    
    // 초기 스캔 진행하여 기존 파일 목록 정보 로드 (is_initial = 1)
    scan_directory(target_dir, 1);
    
    // 초기 로딩 시점에 파일들의 DB 생존 플래그를 미리 켜둠
    for (int i = 0; i < file_count; i++) {
        file_db[i].is_checked = 1;
    }
    
    printf("[Watcher] 초기 탐색 완료. 하위 폴더 포함 총 %d개 파일 추적 시작.\n", file_count);
    printf("-----------------------------------------\n");
    fflush(stdout);

    while (1) {
        sleep(2); // 2초 주기 설정
        
        // [Heartbeat] 루프가 살아있는지 주기적으로 확인시켜주는 로그
        //printf("[Watcher] 2초 주기 감시 스캔 완료 (현재 추적 파일: %d개)\n", file_count);
        fflush(stdout);
        
        // 1. 모든 감시 대상의 존재 플래그를 꺼두고 탐색 시작 (삭제 탐지용)
        for (int i = 0; i < file_count; i++) {
            file_db[i].is_checked = 0;
        }

        // 2. 하위 디렉토리 포함 실시간 주기 스캔 (is_initial = 0)
        scan_directory(target_dir, 0);

        // 3. 스캔이 모두 끝났는데 여전히 존재 플래그가 0인 파일은 삭제된 것으로 간주
        for (int i = 0; i < file_count; i++) {
            if (file_db[i].is_checked == 0) {
                printf("[Watcher] 파일 삭제 감지: %s\n", file_db[i].filepath);
                fflush(stdout); // 터미널에 즉시 출력 강제 실행
                
                // 데이터베이스에서 제거 처리
                if (i != file_count - 1) {
                    file_db[i] = file_db[file_count - 1];
                    i--;
                }
                file_count--;
            }
        }
    }
}
