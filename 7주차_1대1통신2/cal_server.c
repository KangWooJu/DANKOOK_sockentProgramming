#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 3600

struct cal_data {
    int left_num;
    int right_num;
    char op;
    int result;
    int max;
    int min;
    short int error;
    char word[50]; // 문자열을 저장할 배열
};

int main(int argc, char **argv) {
    struct sockaddr_in client_addr, sock_addr;
    int listen_sockfd, client_sockfd;
    int addr_len;
    struct cal_data rdata;
    int left_num, right_num, cal_result;
    short int cal_error;

    // min과 max 초기화
    int current_min = INT_MAX;
    int current_max = INT_MIN;

    if ((listen_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error ");
        return 1;
    }

    memset((void *)&sock_addr, 0x00, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(PORT);

    if (bind(listen_sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1) {
        perror("Error ");
        return 1;
    }

    if (listen(listen_sockfd, 5) == -1) {
        perror("Error ");
        return 1;
    }

    for (;;) {
        addr_len = sizeof(client_addr);
        client_sockfd = accept(listen_sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sockfd == -1) {
            perror("Error ");
            return 1;
        }
        printf("New Client Connect : %s\n", inet_ntoa(client_addr.sin_addr));

        read(client_sockfd, (void *)&rdata, sizeof(rdata));

        cal_result = 0;
        cal_error = 0;

        left_num = ntohl(rdata.left_num);
        right_num = ntohl(rdata.right_num);

        switch (rdata.op) {
            case '+':
                cal_result = left_num + right_num;
                break;
            case '-':
                cal_result = left_num - right_num;
                break;
            case 'x':
                cal_result = left_num * right_num;
                break;
            case '/':
                if (right_num == 0) {
                    cal_error = 2;
                    break;
                }
                cal_result = left_num / right_num;
                break;
            default:
                cal_error = 1;
        }

        // 최소값 및 최대값 업데이트
        if (cal_error == 0) {
            current_min = (cal_result < current_min) ? cal_result : current_min;
            current_max = (cal_result > current_max) ? cal_result : current_max;
        }

        rdata.result = htonl(cal_result);
        rdata.error = htons(cal_error);
        rdata.min = htonl(current_min);
        rdata.max = htonl(current_max);

        // 클라이언트로부터 받은 문자열을 그대로 사용합니다.
        rdata.word[sizeof(rdata.word) - 1] = '\0'; // 널 종료

        printf("%d %c %d = %d (Min: %d, Max: %d, String: %s)\n",
               left_num, rdata.op, right_num, cal_result,
               ntohl(rdata.min), ntohl(rdata.max), rdata.word);

        write(client_sockfd, (void *)&rdata, sizeof(rdata));
        close(client_sockfd);
    }

    close(listen_sockfd);
    return 0;
}
