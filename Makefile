CC = gcc
CFLAGS = -Wall -Iinclude -pthread

CLIENT_TARGET = safesync
SERVER_TARGET = server

CLIENT_SRCS = src/main.c src/watcher.c src/signals.c src/queue.c src/threads.c src/network.c
SERVER_SRCS = src/server.c src/network.c

all: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_SRCS)
	$(CC) -o $(CLIENT_TARGET) $(CLIENT_SRCS) $(CFLAGS)
	@echo "[Build] 클라이언트 컴파일 완료"

$(SERVER_TARGET): $(SERVER_SRCS)
	$(CC) -o $(SERVER_TARGET) $(SERVER_SRCS) $(CFLAGS)
	@echo "[Build] 서버 컴파일 완료"

clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET)
	@echo "[Build] 빌드 청소 완료"

.PHONY: all clean
