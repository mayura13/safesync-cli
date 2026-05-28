// src/watcher.c
#include "../include/watcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

// 모니터링중인 파일들의 데이터베이스
FileInfo file_db[MAX_FILES];
int file_count = 0;

// 특정 파일이 이미 DB에 존재하는지 인덱스를 반환하는 함수
int find_file_index(const char *path) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(file_db[i].filepath, path) == 0) {
            return i;
        }
    }
    return -1; // 존재하지 않음
}

// 디렉토리를 1회 스캔하면서 파일 변경/생성/삭제를 감지하는 함수
void scan_directory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("[Watcher Error] 디렉토리를 열 수 없습니다");
        return;
    }

    // 1. 모든 파일의 체크 플래그를 초기화 (삭제 감지용)
    for (int i = 0; i < file_count; i++) {
        file_db[i].is_checked = 0;
    }

    struct dirent *entry;
    struct stat file_stat;
    char full_path[PATH_LEN];

    // 2. 디렉토리 내의 모든 항목을 순회
    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 은 스캔에서 제외
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 전체 경로 파일명 생성
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        // 파일 정보(stat) 읽기
        if (stat(full_path, &file_stat) == -1) {
            continue;
        }

        // 일반 파일만 감시 (디렉토리나 특수 파일 제외)
        if (S_ISREG(file_stat.st_mode)) {
            int idx = find_file_index(full_path);

            if (idx == -1) {
                // [신규 파일 발견] DB에 등록
                if (file_count < MAX_FILES) {
                    strcpy(file_db[file_count].filepath, full_path);
                    file_db[file_count].last_mtime = file_stat.st_mtime;
                    file_db[file_count].is_checked = 1;
                    file_count++;
                    printf("[Watcher] 신규 파일 감지: %s\n", entry->d_name);
                    
                    // TODO: 팀원 B의 전송 대기열(Queue)에 추가하는 코드가 들어갈 자리입니다.
                }
            } else {
                // 이미 존재하는 파일인 경우
                file_db[idx].is_checked = 1; // 살아있음 표시

                // 수정 시간 비교
                if (file_stat.st_mtime > file_db[idx].last_mtime) {
                    printf("[Watcher] 파일 수정 감지: %s (시간: %ld -> %ld)\n", 
                           entry->d_name, file_db[idx].last_mtime, file_stat.st_mtime);
                    
                    // 수정 시간 업데이트
                    file_db[idx].last_mtime = file_stat.st_mtime;

                    // TODO: 팀원 B의 전송 대기열(Queue)에 추가하는 코드가 들어갈 자리입니다.
                }
            }
        }
    }
    closedir(dir);

    // 3. 스캔이 끝났는데 checked 플래그가 0인 파일은 삭제된 파일임
    for (int i = 0; i < file_count; i++) {
        if (file_db[i].is_checked == 0) {
            printf("[Watcher] 파일 삭제 감지: %s\n", file_db[i].filepath);
            
            // 데이터베이스에서 해당 항목 제거 (마지막 항목을 덮어쓰기)
            if (i != file_count - 1) {
                file_db[i] = file_db[file_count - 1];
                i--; // 현재 인덱스 재검사
            }
            file_count--;
        }
    }
}

// 2초마다 무한히 돌며 감시를 수행하는 루프 함수
void start_monitoring(const char *target_dir) {
    printf("[Watcher] '%s' 디렉토리 실시간 감시를 시작합니다.\n", target_dir);
    printf("[Watcher] 종료하려면 Ctrl+C를 누르세요.\n");
    
    // 초기 1회 스캔으로 기존 파일 목록 등록 (처음 켰을 때 전부 백업하는 현상 방지)
    scan_directory(target_dir);
    printf("[Watcher] 초기 탐색 완료. 파일 %d개 추적 중.\n", file_count);
    printf("-----------------------------------------\n");

    while (1) {
        sleep(2); // 2초 간격으로 폴링
        scan_directory(target_dir);
    }
}
