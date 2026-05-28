#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0}; // 손님 말을 받아적을 메모장(버퍼)

    // 1 & 2단계: 가게 터 잡고 간판 달기 (앞서 짠 코드와 동일)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    // 3단계: listen - 영업 시작! (대기열 3명까지 허용)
    if (listen(server_fd, 3) < 0) {
        perror("영업 시작(listen) 실패");
        exit(EXIT_FAILURE);
    }
    printf("🟢 [서버] %d번 포트에서 24시간 영업을 시작합니다...\n", PORT);

    // 4단계: 무한 루프 (24시간 손님 받기)
    while(1) {
        // 여기서 손님이 올 때까지 서버가 멈춰서 기다립니다! (Blocking)
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            perror("손님 입장(accept) 실패");
            continue;
        }
        
        // 손님이 오면 새로운 번호표(client_socket, 아마 4번)가 발급됩니다.
        printf("\n👋 [서버] 새로운 손님이 입장했습니다! (손님 번호표: %d)\n", client_socket);

        // 손님이 보내는 메시지 읽기 (read)
        memset(buffer, 0, 1024); // 메모장 깨끗하게 지우기
        read(client_socket, buffer, 1024);
        printf("📩 [손님] %s\n", buffer);

        // 메아리 쳐주기 (write 대신 소켓 전용 함수인 send 사용)
        send(client_socket, buffer, strlen(buffer), 0);
        printf("📤 [서버] 메시지를 그대로 반사했습니다.\n");

        // 손님 퇴장 (close)
        close(client_socket);
        printf("🚪 [서버] 손님(%d번)이 퇴장했습니다.\n", client_socket);
    }

    // (무한 루프라 사실상 이 코드는 실행되지 않음)
    close(server_fd);
    return 0;
}