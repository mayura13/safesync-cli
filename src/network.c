// src/network.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../include/network.h"

// ----------------------------------------------------
// [수신측/Server 엔진] 클라이언트의 연결을 기다리는 함수
// ----------------------------------------------------
int setup_server(int port) {
    int server_sock;
    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("[Network] 소켓 생성 실패");
        exit(1);
    }

    // 포트 점유(Bind failed) 에러 방지용 옵션 (강제 종료 후 바로 재시작 가능하게 함)
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[Network] 바인딩 실패");
        exit(1);
    }

    if (listen(server_sock, 5) < 0) {
        perror("[Network] 리슨 실패");
        exit(1);
    }

    return server_sock;
}

// ----------------------------------------------------
// [송신측/Client 엔진] 서버(수신측)로 전화를 거는 함수
// ----------------------------------------------------
int connect_to_server(const char* ip, int port) {
    int sock;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        close(sock);
        return -1;
    }

    // 서버에 연결 시도
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock; // 개통된 소켓(전화기) 반환
}