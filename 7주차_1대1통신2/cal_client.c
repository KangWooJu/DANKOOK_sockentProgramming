#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>  // 여기에 추가

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
    int sockfd;
    struct sockaddr_in serv_addr;
    struct cal_data data;

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <left_num> <right_num> <operator> <string>\n", argv[0]);
        return 1;
    }

    data.left_num = htonl(atoi(argv[1]));
    data.right_num = htonl(atoi(argv[2]));
    data.op = argv[3][0];
    strncpy(data.word, argv[4], sizeof(data.word) - 1);
    data.word[sizeof(data.word) - 1] = '\0'; // 널 종료

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    write(sockfd, (void *)&data, sizeof(data));

    // 서버로부터 응답 받기
    read(sockfd, (void *)&data, sizeof(data));

    printf("Result: %d %c %d = %d, String: %s\n",
           ntohl(data.left_num), data.op, ntohl(data.right_num),
           ntohl(data.result), data.word);

    close(sockfd);
    return 0;
}
