/*
 * ==============================================================================
 * 프로젝트    : 확장 가능한 C 서버 (Scalable C Server)
 * 파일명      : server.c
 * 설명        : Phase 2 - 멀티 스레드 기반 TCP 에코 서버
 * 아키텍처    : 연결당 스레드 모델 (메인 스레드는 연결 수락, 워커 스레드는 통신 전담)
 * ==============================================================================
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <pthread.h> 
 
 #define PORT 8080
 #define BUFFER_SIZE 1024
 #define MAX_QUEUE 3
 
 /*
  * ------------------------------------------------------------------------------
  * 함수명      : handle_client
  * 설명        : 1:1 클라이언트 통신을 전담하여 처리하는 워커 스레드 루틴.
  * 매개변수    : arg - 동적 할당된 클라이언트 소켓 파일 디스크립터(FD)의 포인터
  * 반환값      : NULL (스레드 업무 종료 시 자동 반환)
  * ------------------------------------------------------------------------------
  */
 void *handle_client(void *arg) {
     // 1. 소켓 FD 값을 복사하고, 메모리 누수를 방지하기 위해 전달받은 힙 메모리 해제
     int client_socket = *((int *)arg);
     free(arg); 
 
     char buffer[BUFFER_SIZE];
     memset(buffer, 0, BUFFER_SIZE);
 
     printf("[정보] 스레드 시작. 할당된 클라이언트 소켓: %d\n", client_socket);
 
     // 2. 클라이언트의 메시지가 도착할 때까지 대기 (read - 블로킹)
     if (read(client_socket, buffer, BUFFER_SIZE) > 0) {
         printf("[수신 - 소켓 %d] %s\n", client_socket, buffer);
 
         // 3. 수신한 메시지를 클라이언트에게 그대로 반사 (send)
         send(client_socket, buffer, strlen(buffer), 0);
         printf("[송신 - 소켓 %d] 메시지 에코 완료.\n", client_socket);
     }
 
     // 4. 통신이 완료되면 소켓을 닫고 연결 종료
     close(client_socket);
     printf("[정보] 스레드 종료. 클라이언트 소켓 %d 닫힘.\n", client_socket);
 
     return NULL;
 }
 
 /*
  * ------------------------------------------------------------------------------
  * 함수명      : main
  * 설명        : 서버 프로그램의 진입점. 수신용 대기 소켓을 설정하고,
  * 무한 루프를 돌며 들어오는 클라이언트의 연결을 수락합니다.
  * ------------------------------------------------------------------------------
  */
 int main() {
     int server_fd, client_socket;
     struct sockaddr_in address;
     int opt = 1;
     int addrlen = sizeof(address);
 
     // [1단계] 서버 소켓 생성 (IPv4, TCP)
     server_fd = socket(AF_INET, SOCK_STREAM, 0);
     if (server_fd == 0) {
         perror("[오류] 소켓 생성 실패");
         exit(EXIT_FAILURE);
     }
 
     // [2단계] 소켓 옵션 설정 (서버 재시작 시 "Address already in use" 포트 잠김 에러 방지)
     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
         perror("[오류] setsockopt 설정 실패");
         exit(EXIT_FAILURE);
     }
 
     // [3단계] 서버 주소 구조체 초기화 및 소켓에 바인딩(결합)
     memset(&address, 0, sizeof(address));
     address.sin_family = AF_INET;
     address.sin_addr.s_addr = INADDR_ANY;
     address.sin_port = htons(PORT);
 
     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
         perror("[오류] 바인드 실패");
         exit(EXIT_FAILURE);
     }
 
     // [4단계] 클라이언트 연결 수신 대기열(Listen Queue) 생성
     if (listen(server_fd, MAX_QUEUE) < 0) {
         perror("[오류] 수신 대기(listen) 실패");
         exit(EXIT_FAILURE);
     }
     printf("[시스템] 멀티 스레드 서버가 %d번 포트에서 가동 중입니다...\n", PORT);
 
     // [5단계] 클라이언트 연결 수락 무한 루프 (메인 스레드 전담 업무)
     while (1) {
         // 새로운 클라이언트가 접속할 때까지 여기서 실행을 멈추고 대기 (블로킹)
         client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
         if (client_socket < 0) {
             perror("[경고] 연결 수락 실패. 다음 연결을 기다립니다...");
             continue;
         }
 
         printf("\n[시스템] 새로운 연결 수락됨. 소켓 FD: %d\n", client_socket);
 
         // 데이터 경합(Data Race)을 방지하기 위해 새로운 힙 메모리에 소켓 번호를 기록
         int *new_sock = malloc(sizeof(int));
         if (new_sock == NULL) {
             perror("[오류] 메모리 할당 실패");
             close(client_socket);
             continue;
         }
         *new_sock = client_socket;
 
         // 클라이언트를 응대할 새로운 워커 스레드(알바생) 고용
         pthread_t thread_id;
         if (pthread_create(&thread_id, NULL, handle_client, (void *)new_sock) != 0) {
             perror("[오류] 스레드 생성 실패");
             free(new_sock);
             close(client_socket);
             continue;
         }
 
         // 스레드를 분리(Detach)하여, 업무가 끝나면 메인 스레드의 확인(join) 없이 즉시 메모리를 반환하도록 설정
         pthread_detach(thread_id);
     }
 
     close(server_fd);
     return 0;
 }