#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h> // pthread 사용을 위해 추가

#define PORT 3600 // 서버가 수신 대기할 포트 번호

// 클라이언트와 서버 간 데이터를 전달하기 위한 구조체 정의
struct cal_data {
    int left_num;
    int right_num;
    char op;
    int result;
    int max;
    int min;
    short int error;       // 에러 : 0->정상 , 1-> 오류
    char word[50];
    char ip_address[50];
    struct tm calc_time;
};

// 전역 변수로 최소값과 최대값을 관리 -> client에서 입력하여서 바뀌어야할때를 위함
int global_min = INT_MAX; // 초기 최소값은 최대 정수값
int global_max = INT_MIN; // 초기 최대값은 최소 정수값
pthread_mutex_t lock; // 전역 변수 동기화를 위한 뮤텍스

// 클라이언트 요청을 처리하는 함수 -> 과제 10에서 변경점 : 멀티쓰레드 버전
void *handle_client(void *arg) {
    int cli_sock = *((int *)arg); // 전달된 소켓 파일 디스크립터 추출
    free(arg); // 동적 메모리 해제
    struct cal_data data; // 클라이언트 데이터 저장 구조체
    struct sockaddr_in cli_addr; // 클라이언트 주소 구조체
    socklen_t cli_addr_len = sizeof(cli_addr);

    // 클라이언트의 IP 주소를 가져옴
    getpeername(cli_sock, (struct sockaddr *)&cli_addr, &cli_addr_len);

    while (1) {
        // 클라이언트로부터 데이터를 수신
        if (read(cli_sock, &data, sizeof(data)) <= 0) {
            break; // 수신 실패 시 루프 종료
        }

        // 연산 수행
        data.error = 0; // 에러 플래그 초기화
        switch (data.op) {
            case '+':
                data.result = data.left_num + data.right_num;
                break;
            case '-':
                data.result = data.left_num - data.right_num;
                break;
            case '*':
                data.result = data.left_num * data.right_num;
                break;
            case '/':
                if (data.right_num != 0)
                    data.result = data.left_num / data.right_num;
                else
                    data.error = 1; // 0으로 나누기 에러
                break;
            default:
                data.error = 1; // 알 수 없는 연산자
        }

        // 전역 최소값과 최대값 업데이트 (뮤텍스 사용)
        pthread_mutex_lock(&lock); // 동기화 시작
        if (!data.error) { // 에러가 없을 경우에만 업데이트
            if (data.result < global_min) {
                global_min = data.result;
            }
            if (data.result > global_max) {
                global_max = data.result;
            }
        }
        pthread_mutex_unlock(&lock); // 동기화 종료

        data.min = global_min; // 업데이트된 최소값
        data.max = global_max; // 업데이트된 최대값

        // 32220062 강우주가 쓴 코드
        // 현재 시간 기록
        time_t t = time(NULL); // 현재 시간을 가져옴
        data.calc_time = *localtime(&t); // 로컬 시간 구조체로 변환

        // 클라이언트의 IP 주소를 문자열로 저장
        strncpy(data.ip_address, inet_ntoa(cli_addr.sin_addr), sizeof(data.ip_address) - 1);

        // 서버 로그 출력
        char time_buffer[100];
        strftime(time_buffer, sizeof(time_buffer), "%a %b %d %H:%M:%S %Y", &data.calc_time); // 시간 포맷
        printf("%d%c%d=%d %s min=%d max=%d %s from %s\n",
               data.left_num, data.op, data.right_num, data.result,
               data.word, data.min, data.max, time_buffer, data.ip_address);

        // 10초 대기 후 클라이언트로 결과 전송
        sleep(10);
        write(cli_sock, &data, sizeof(data)); // 결과 전송

        // quit 입력시 종료
        if (strcmp(data.word, "quit") == 0) {
            break;
        }
    }

    close(cli_sock); // 클라이언트 소켓 닫기
    pthread_exit(NULL); // 쓰레드 종료
}

int main() {
    int serv_sock; // 서버 소켓 파일 디스크립터
    struct sockaddr_in serv_addr, cli_addr; // 서버와 클라이언트 주소 구조체
    socklen_t cli_addr_len = sizeof(cli_addr);

    pthread_mutex_init(&lock, NULL); // 뮤텍스 초기화

    // 서버 소켓 생성
    if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // 서버 주소 구조체 초기화
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; // IPv4
    serv_addr.sin_port = htons(PORT); // 포트 번호 설정
    serv_addr.sin_addr.s_addr = INADDR_ANY; // 모든 인터페이스에서 수신 대기

    // 소켓에 주소 바인딩
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(serv_sock);
        return 1;
    }

    // 클라이언트 연결 대기 시작
    if (listen(serv_sock, 5) < 0) {
        perror("Listen failed");
        close(serv_sock);
        return 1;
    }

    printf("Server is running...\n");

    while (1) {
        int *cli_sock = malloc(sizeof(int)); // 클라이언트 소켓을 위한 동적 메모리 할당
        if ((*cli_sock = accept(serv_sock, (struct sockaddr *)&cli_addr, &cli_addr_len)) < 0) {
            perror("Accept failed");
            free(cli_sock); // 실패 시 메모리 해제
            continue;
        }

        // 새로운 쓰레드 생성 -> 과제 10의 요구사항
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, cli_sock) != 0) {
            perror("Thread creation failed");
            free(cli_sock); // 실패 시 메모리 해제
            continue;
        }
        pthread_detach(tid); // 쓰레드 종료 후에 자동으로 리소스 정리하는 함수
    }

    close(serv_sock); // 서버 소켓 닫기
    pthread_mutex_destroy(&lock); // 뮤텍스 제거
    return 0;
}

