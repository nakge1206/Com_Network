#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 50
#define PACKET_SIZE 7
#define PAYLOAD_SIZE 4

typedef struct { //Packet 구조체
    char payload[PAYLOAD_SIZE];
    unsigned short checksum;
    unsigned char packet_number;
} Packet;

unsigned short calculate_checksum(char *data, size_t length) { //체크섬 계산
    unsigned short checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}

Packet buffer[BUFFER_SIZE]; //멀티스레드의 공유자원을 위해 전역변수로 만들었음.
int buffer_start = 0;
int buffer_end = 0;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;

void *receiver_thread(void *arg) { //수신자 스레드
    int client_sock = *(int *)arg;
    Packet packet;

    while (1) {
        ssize_t bytes_received = recv(client_sock, &packet, PACKET_SIZE, 0);

        if (bytes_received <= 0) {
            break; //연결이 끊겼거나 오류발생 시 종료
        }

        pthread_mutex_lock(&buffer_mutex); //임계영역 진입
        if ((buffer_end + 1) % BUFFER_SIZE != buffer_start) { //버퍼가 가득 차있지 않으면 버퍼에 패킷을 저장하고, 문제없다는 신호를 다른스레드에게 보냄
            buffer[buffer_end] = packet;
            buffer_end = (buffer_end + 1) % BUFFER_SIZE;
            pthread_cond_signal(&buffer_not_empty);
        } else {
            printf("Buffer overflow, packet %d is dropped.\n", packet.packet_number); //버퍼가 가득찼다면 overflow발생하며 드랍했다는 문구 출력
        }
        pthread_mutex_unlock(&buffer_mutex); //임계영역 탈출
    }
    return NULL;
}

void *processor_thread(void *arg) { //패킷처리 스레드
    int client_sock = *(int *)arg;
    FILE *output_file = fopen("output.txt", "w");

    while (1) {
        pthread_mutex_lock(&buffer_mutex); //임계영역 진입
        while (buffer_start == buffer_end) { //버퍼가 비어있다면 대기
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
        }

        Packet packet = buffer[buffer_start];
        buffer_start = (buffer_start + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&buffer_mutex); //임계영역 탈출

        if (packet.checksum == calculate_checksum(packet.payload, PAYLOAD_SIZE)) { //체크섬 계산후 받은것과 맞는지 확인
            printf("packet %d is received and there is no error. (%.*s)\n", packet.packet_number, PAYLOAD_SIZE, packet.payload); //맞다면 제대로 받았다는 문구 출력 후, txt파일에 저장
            fprintf(output_file, "%.*s", PAYLOAD_SIZE, packet.payload);
            fflush(output_file); //버퍼를 초기화 함으로써 바로 저장 될 수 있게함.
            int ack_number = (packet.packet_number + 1) * PAYLOAD_SIZE;
            send(client_sock, &ack_number, sizeof(int), 0); 
            printf("(ACK = %d) is transmitted.\n", ack_number); //ack보내고 문구 출력
        } else {
            printf("packet %d is received and there is an error.\n", packet.packet_number); //체크섬이 틀리면 에러문구 출력
        }

        usleep(100000); //전송주기 0.1초
    }
    fclose(output_file);
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    pthread_t recv_thread, proc_thread;

    //소켓 생성
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    //blind
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_sock);
        exit(1);
    }

    //listen
    if (listen(server_sock, 1) == -1) {
        perror("Listen failed");
        close(server_sock);
        exit(1);
    }

    printf("Server is listening on port %d\n", PORT);

    //accept
    if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
        perror("Accept failed");
        close(server_sock);
        exit(1);
    }

    //스레드 생성
    pthread_create(&recv_thread, NULL, receiver_thread, &client_sock);
    pthread_create(&proc_thread, NULL, processor_thread, &client_sock);

    //스레드 반환
    pthread_join(recv_thread, NULL);
    pthread_join(proc_thread, NULL);

    //소켓닫음
    close(client_sock);
    close(server_sock);
    return 0;
}
