#include <stdio.h>      // 표준 입출력 함수
#include <stdlib.h>     // atoi 등 유틸리티 함수
#include <string.h>     // 문자열 처리 함수
#include <sys/socket.h> // 소켓 관련 함수
#include <netinet/in.h> // sockaddr_in 등 네트워크 관련 구조체 및 함수
#include <arpa/inet.h>  // inet_pton 함수
#include <unistd.h>     // close 함수
#include <time.h>       // struct tm 관련 함수

#define PORT 3600

struct cal_data {
    int left_num;
    int right_num;
    char op;
    int result;
    int max;
    int min;
    short int error;
    char word[50];
    char ip_address[50];
    struct tm calc_time; // 시간 정보 저장
};

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <left_num> <right_num> <op> <string>\n", argv[0]);
        return 1;
    }

    int sock;
    struct sockaddr_in serv_addr;
    struct cal_data data;

    // 명령줄 인자를 구조체에 저장
    data.left_num = atoi(argv[1]);
    data.right_num = atoi(argv[2]);
    data.op = argv[3][0];
    strncpy(data.word, argv[4], sizeof(data.word) - 1);
    data.word[sizeof(data.word) - 1] = '\0'; // Null-terminate to avoid overflow

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "10.20.0.170", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    // 서버로 데이터 전송
    write(sock, &data, sizeof(data));

    // 서버로부터 계산 결과 수신
    read(sock, &data, sizeof(data));

    // 클라이언트 출력
    char time_buffer[100];
    strftime(time_buffer, sizeof(time_buffer), "%a %b %d %H:%M:%S %Y", &data.calc_time);
    printf("%d%c%d=%d %s min=%d max=%d %s from %s\n",
           data.left_num, data.op, data.right_num, data.result,
           data.word, data.min, data.max, time_buffer, data.ip_address);

    close(sock);
    return 0;
}
