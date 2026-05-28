// src/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include "../include/network.h"

#define BUFFER_SIZE 4096
// 서버가 받은 파일을 저장할 전용 폴더 이름
#define SYNC_DIR "./server_sync_folder" 

void handle_client(int client_sock) {
    FileHeader header;
    char filename[256];
    char filepath[512];
    char buffer[BUFFER_SIZE];
    ssize_t read_bytes, written_bytes;
    off_t local_size = 0;
    struct stat file_stat;

    // 1. 클라이언트가 보낸 헤더(이름 길이, 파일 전체 크기) 수신
    if (read(client_sock, &header, sizeof(FileHeader)) <= 0) {
        close(client_sock);
        return;
    }

    // 2. 파일명 수신
    memset(filename, 0, sizeof(filename));
    read(client_sock, filename, header.name_len);

    // 파일 경로에서 순수 파일명만 쏙 빼내는 작업 (예: /home/test.txt -> test.txt)
    char *pure_filename = strrchr(filename, '/');
    pure_filename = (pure_filename != NULL) ? pure_filename + 1 : filename;

    // 저장할 최종 목적지 경로 만들기 (예: ./server_sync_folder/test.txt)
    snprintf(filepath, sizeof(filepath), "%s/%s", SYNC_DIR, pure_filename);

    // 3. 내 컴퓨터에 이미 똑같은 파일이 있는지 확인 (이어받기를 위한 크기 계산)
    int fd = open(filepath, O_RDWR | O_CREAT, 0644);
    if (fd >= 0) {
        if (fstat(fd, &file_stat) == 0) {
            local_size = file_stat.st_size;
        }
    } else {
        perror("[서버 오류] 파일 생성 실패");
        close(client_sock);
        return;
    }

    // 4. 클라이언트에게 "나 이만큼 가지고 있어!" 하고 응답 (Handshake)
    write(client_sock, &local_size, sizeof(off_t));

    // 5. 이미 파일 크기가 똑같거나 크면 받을 필요가 없음
    if (local_size >= header.file_size) {
        printf("[서버] '%s'는 이미 최신 상태입니다. (동기화 스킵)\n", pure_filename);
        close(fd);
        close(client_sock);
        return;
    }

    // 6. 진짜 이어받기 시작 (lseek)
    lseek(fd, local_size, SEEK_SET);
    printf("[서버] '%s' 수신 시작! (이어받기: %lld 바이트부터)\n", pure_filename, (long long)local_size);

    // 7. 파일 데이터 수신 및 저장 (쓰기)
    while ((read_bytes = read(client_sock, buffer, BUFFER_SIZE)) > 0) {
        written_bytes = write(fd, buffer, read_bytes);
        if (written_bytes <= 0) break;
    }

    printf("[서버] '%s' 동기화 저장 완벽 종료!\n", pure_filename);
    close(fd);
    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 수신 전용 폴더가 없으면 미리 만들어둠
    mkdir(SYNC_DIR, 0777);

    printf("=========================================\n");
    printf("[SafeSync 수신 서버] 가동 준비 중...\n");
    
    // 네가 깎아둔 파트 B 네트워크 엔진 호출! (8080 포트 오픈)
    server_sock = setup_server(8080);
    
    printf("[SafeSync 수신 서버] 클라이언트 접속을 무한 대기합니다...\n");
    printf("=========================================\n");

    while (1) {
        // accept(): 전화가 올 때까지 여기서 귀를 쫑긋 세우고 멈춰있음
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) continue;

        printf("\n[서버] 클라이언트 연결 됨! 동기화 처리 시작...\n");
        handle_client(client_sock);
    }

    close(server_sock);
    return 0;
}