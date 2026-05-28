#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "network.h"

// 1. 서버 열기 (상대방 연결 대기)
int setup_server(int port) {
    int server_fd;
    struct sockaddr_in server_addr;

    // socket(): 전화기 구입 (IPv4, TCP 방식)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // bind(): 전화기에 내 번호(IP/Port) 부여
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 내 컴퓨터의 모든 IP 허용
    server_addr.sin_port = htons(port);       // 포트 번호 설정

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // listen(): 개통 완료, 전화 오기를 기다림 (최대 대기열 5)
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[네트워크] 서버가 포트 %d에서 대기 중입니다...\n", port);
    return server_fd; // 이 디스크립터로 나중에 accept()를 호출함
}

// 2. 클라이언트 접속 (서버로 전화 걸기)
int connect_to_server(const char* ip, int port) {
    int client_fd;
    struct sockaddr_in server_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("Socket creation failed");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // connect(): 서버로 연결 시도
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(client_fd);
        return -1;
    }

    printf("[네트워크] 서버(%s:%d)에 성공적으로 연결되었습니다.\n", ip, port);
    return client_fd;
}