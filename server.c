#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080 // 우리가 사용할 포트 번호

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // 1. socket() : 마법의 파이프라인(소켓) 생성
    // AF_INET = IPv4 주소체계 사용, SOCK_STREAM = TCP 통신 사용
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("소켓 생성 실패");
        exit(EXIT_FAILURE);
    }
    printf("[성공] 1단계: 리눅스 커널에 서버 소켓 생성 완료 (FD 번호: %d)\n", server_fd);

    // 가상머신을 재부팅했을 때 포트가 이미 사용 중이라며 굳어버리는 에러를 방지하는 무적의 옵션
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 주소 구조체(sockaddr_in) 설정 : 어디서 들어오는 접속을 허용할 것인가?
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 아무 IP로 들어오는 접속이든 다 받겠다!
    address.sin_port = htons(PORT);       // 포트 번호를 네트워크 표준 정렬 방식으로 변환

    // 2. bind() : 소켓에 IP 주소와 포트 번호(Port)를 박아넣기
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("바인드 실패");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("[성공] 2단계: 소켓에 포트 번호 %d번 바인딩 완료\n", PORT);

    // 아직 3, 4단계(listen, accept) 코드가 없으므로 정상 종료 처리
    printf("기본 기틀 잡기 성공! 소켓을 닫고 종료합니다.\n");
    close(server_fd);
    return 0;
}