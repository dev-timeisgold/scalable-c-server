/*
 * ==============================================================================
 * 프로젝트    : 확장 가능한 C 서버 (Scalable C Server)
 * 파일명      : server.c
 * 설명        : Phase 3 - epoll 기반 I/O 멀티플렉싱 서버
 * 아키텍처    : 이벤트 기반 단일 스레드 구조 (Event-Driven Single Thread)
 * ==============================================================================
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <sys/epoll.h> // epoll 전광판 기능을 위한 헤더 파일
 
 #define PORT 8080
 #define BUFFER_SIZE 1024
 #define MAX_EVENTS 10  // 전광판에 한 번에 띄울 수 있는 최대 알림 개수
 
 int main() {
     int server_fd, client_socket, epoll_fd, event_count;
     struct sockaddr_in address;
     int opt = 1;
     int addrlen = sizeof(address);
     char buffer[BUFFER_SIZE];
 
     // epoll 전용 구조체 (호출 벨과 전광판 알림을 담는 그릇)
     struct epoll_event event;
     struct epoll_event events[MAX_EVENTS];
 
     // [1단계] 서버 정문(Listen Socket) 개설 (Phase 1, 2와 동일)
     server_fd = socket(AF_INET, SOCK_STREAM, 0);
     setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
     
     memset(&address, 0, sizeof(address));
     address.sin_family = AF_INET;
     address.sin_addr.s_addr = INADDR_ANY;
     address.sin_port = htons(PORT);
     
     bind(server_fd, (struct sockaddr *)&address, sizeof(address));
     listen(server_fd, 3);
     printf("[시스템] epoll 기반 싱글 스레드 서버가 %d번 포트에서 가동 중입니다...\n", PORT);
 
     // ==========================================================================
     // [2단계] epoll 마법의 시작 (전광판 구매 및 정문 등록)
     // ==========================================================================
 
     // 1. epoll_create1: 사장님이 볼 거대한 알림 전광판 생성
     epoll_fd = epoll_create1(0);
     if (epoll_fd == -1) {
         perror("[오류] epoll 전광판 생성 실패");
         exit(EXIT_FAILURE);
     }
 
     // 2. 정문(server_fd)에 호출 벨을 달아서 전광판에 등록 (epoll_ctl)
     // "누군가 정문에 오면(EPOLLIN), 전광판에 server_fd 번호로 알림을 띄워라!"
     event.events = EPOLLIN; 
     event.data.fd = server_fd;
     epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);
 
     // ==========================================================================
     // [3단계] 24시간 이벤트 처리 루프 (단 1명의 사장님이 모든 것을 통제)
     // ==========================================================================
     while (1) {
         // 3. epoll_wait: 전광판만 쳐다보며 대기 (아무도 안 부르면 여기서 휴식)
         // 알림이 울리면 울린 개수만큼 event_count에 숫자가 담김
         event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
 
         // 울린 알림(이벤트)들을 하나씩 확인하며 처리
         for (int i = 0; i < event_count; i++) {
             int active_fd = events[i].data.fd; // 알림이 울린 소켓 번호
 
             // 케이스 A: 정문(server_fd)에서 알림이 울린 경우 (새로운 손님 도착!)
             if (active_fd == server_fd) {
                 client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                 printf("\n[시스템] 새로운 연결 수락됨. 소켓 FD: %d\n", client_socket);
 
                 // 새로 온 손님 테이블(client_socket)에도 호출 벨을 달아서 전광판에 등록
                 event.events = EPOLLIN;
                 event.data.fd = client_socket;
                 epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
             }
             // 케이스 B: 기존 손님 테이블(client_socket)에서 알림이 울린 경우 (메시지 도착!)
             else {
                 memset(buffer, 0, BUFFER_SIZE);
                 int read_bytes = read(active_fd, buffer, BUFFER_SIZE);
 
                 if (read_bytes <= 0) {
                     // 손님이 나갔거나 에러 발생 시: 전광판에서 벨을 떼고 테이블 정리
                     epoll_ctl(epoll_fd, EPOLL_CTL_DEL, active_fd, NULL);
                     close(active_fd);
                     printf("[시스템] 클라이언트 소켓 %d 연결 종료 및 전광판에서 제거됨.\n", active_fd);
                 } else {
                     // 손님이 정상적으로 메시지를 보낸 경우: 메아리(Echo) 전송
                     printf("[수신 - 소켓 %d] %s\n", active_fd, buffer);
                     send(active_fd, buffer, read_bytes, 0);
                     printf("[송신 - 소켓 %d] 메시지 에코 완료.\n", active_fd);
                 }
             }
         }
     }
 
     close(server_fd);
     close(epoll_fd);
     return 0;
 }