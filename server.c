/*
 * ==============================================================================
 * 프로젝트    : 확장 가능한 C 서버
 * 파일명      : server.c
 * 설명        : Phase 3-1 - epoll 기반 다중 접속 채팅(Broadcasting) 서버
 * ==============================================================================
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <sys/epoll.h>
 
 #define PORT 8080
 #define BUFFER_SIZE 1024
 #define MAX_EVENTS 10 
 #define MAX_CLIENTS 100 // 최대 동시 접속자 수 제한
 
 int main() {
     // ---------------------------------------------------------
     // [변수 준비 공간]
     // ---------------------------------------------------------
     int server_fd, client_socket, epoll_fd, event_count;
     struct sockaddr_in address;
     int opt = 1;
     int addrlen = sizeof(address);
     
     // 사장님의 유일한 공용 짐칸 (수신 버퍼)
     char buffer[BUFFER_SIZE];
     
     // 사장님의 방명록 (0으로 가득 찬 100칸짜리 배열, 0은 '빈자리'를 뜻함)
     int clients[MAX_CLIENTS] = {0}; 
 
     // epoll 전광판을 위한 구조체
     struct epoll_event event;
     struct epoll_event events[MAX_EVENTS];
 
 
     // ---------------------------------------------------------
     // [1단계: 가게 오픈 준비 (네트워크 소켓 세팅)]
     // ---------------------------------------------------------
     // 1. 서버 정문 역할을 할 소켓(server_fd) 생성
     server_fd = socket(AF_INET, SOCK_STREAM, 0);
     
     // 2. 서버가 튕겼을 때 8080 포트가 잠기는 에러 방지
     setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
     
     // 3. 서버 주소와 8080 포트 번호 설정
     memset(&address, 0, sizeof(address));
     address.sin_family = AF_INET;
     address.sin_addr.s_addr = INADDR_ANY;
     address.sin_port = htons(PORT);
     
     // 4. 소켓에 주소와 포트 결합 (bind)
     bind(server_fd, (struct sockaddr *)&address, sizeof(address));
     
     // 5. 손님 대기열 생성 (본격적으로 손님을 받을 준비)
     listen(server_fd, 3);
     printf("[시스템] 멀티플렉싱 채팅 서버가 %d번 포트에서 가동 중입니다...\n", PORT);
 
 
     // ---------------------------------------------------------
     // [2단계: 전광판 설치 (epoll 세팅)]
     // ---------------------------------------------------------
     // 1. 알림 전광판 생성
     epoll_fd = epoll_create1(0);
     
     // 2. 정문(server_fd)에 알림 벨(EPOLLIN: 읽기 이벤트) 달기
     event.events = EPOLLIN; 
     event.data.fd = server_fd;
     
     // 3. 정문을 전광판 시스템에 등록
     epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);
 
 
     // ---------------------------------------------------------
     // [3단계: 영업 시작 (무한 대기 및 알림 확인)]
     // ---------------------------------------------------------
     while (1) {
         // 전광판만 보면서 대기하다가, 벨이 울린 개수(event_count)를 받아옴
         event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
 
         // 울린 벨의 개수만큼 반복하며 하나씩 순차적으로 처리
         for (int i = 0; i < event_count; i++) {
             // 이번 턴에 처리할 소켓 번호표(FD) 뽑기
             int active_fd = events[i].data.fd; 
 
 
             // ---------------------------------------------------------
             // [4단계: 정문에서 벨이 울린 경우 = 새로운 손님 도착!]
             // ---------------------------------------------------------
             if (active_fd == server_fd) {
                 // 문을 열어주고 새 손님의 전용 소켓(client_socket) 생성
                 client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                 printf("[시스템] 새로운 손님 입장. (소켓 번호: %d)\n", client_socket);
 
                 // 방명록(clients 배열)의 빈자리(0)를 찾아서 새 손님 번호 적기
                 for (int j = 0; j < MAX_CLIENTS; j++) {
                     if (clients[j] == 0) {
                         clients[j] = client_socket;
                         break; // 자리 찾았으면 반복문 탈출
                     }
                 }
 
                 // 새 손님의 테이블에도 벨을 달아서 전광판에 등록
                 event.events = EPOLLIN;
                 event.data.fd = client_socket;
                 epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
             }
             
 
             // ---------------------------------------------------------
             // [5단계: 기존 손님 테이블에서 벨이 울린 경우 = 메시지 도착!]
             // ---------------------------------------------------------
             else {
                 // 이전 찌꺼기가 남지 않게 공용 짐칸(버퍼) 물청소
                 memset(buffer, 0, BUFFER_SIZE);
                 
                 // 손님이 보낸 데이터 읽어오기
                 int read_bytes = read(active_fd, buffer, BUFFER_SIZE);
 
                 // 상황 1: 손님이 강제로 나가버렸거나 연결이 끊긴 경우 (읽은 크기가 0 이하)
                 if (read_bytes <= 0) {
                     // 방명록에서 나간 손님의 번호를 찾아 0(빈자리)으로 지우기
                     for (int j = 0; j < MAX_CLIENTS; j++) {
                         if (clients[j] == active_fd) {
                             clients[j] = 0;
                             break;
                         }
                     }
                     // 전광판에서 해당 손님의 벨 제거 및 소켓 닫기
                     epoll_ctl(epoll_fd, EPOLL_CTL_DEL, active_fd, NULL);
                     close(active_fd);
                     printf("[시스템] 손님 %d 퇴장.\n", active_fd);
                 } 
                 // 상황 2: 손님이 정상적으로 채팅 메시지를 보낸 경우 (방송 시작)
                 else {
                     // 메시지 앞에 "[손님 번호]: " 말머리 붙여주기
                     char broadcast_msg[BUFFER_SIZE + 50];
                     sprintf(broadcast_msg, "[손님 %d]: %s", active_fd, buffer);
                     
                     // 서버 콘솔 화면에도 출력해서 확인
                     printf("%s", broadcast_msg); 
 
                     // ⭐️ 방명록을 처음부터 끝까지 쫙 훑으면서 모두에게 복사해서 던져줌
                     for (int j = 0; j < MAX_CLIENTS; j++) {
                         // 조건 1: 빈자리가 아닐 것 (0이 아님)
                         // 조건 2: 메시지를 보낸 당사자가 아닐 것 (본인한테 메아리 치지 않기)
                         if (clients[j] != 0 && clients[j] != active_fd) {
                             send(clients[j], broadcast_msg, strlen(broadcast_msg), 0);
                         }
                     }
                 }
             }
         }
     }
 
     // 서버 종료 시 정문과 전광판 자원 반납
     close(server_fd);
     close(epoll_fd);
     return 0;
 }