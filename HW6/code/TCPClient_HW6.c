#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>

#define PORT 8080
#define BUFFER_SIZE 50
#define PACKET_SIZE 7
#define PAYLOAD_SIZE 4
#define WINDOW_SIZE 4
#define TIMEOUT_INTERVAL 500000 // 0.5초

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

int main() {
    int sock;
    struct sockaddr_in server_addr;
    FILE *file;
    char buffer[PAYLOAD_SIZE + 1];
    int base = 0;
    int next_seq_num = 0;
    int ack_number;
    fd_set readfds;
    struct timeval timeout;
    Packet window[WINDOW_SIZE];
    int window_start = 0, window_end = 0;

    // 소켓생성
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // 서버연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(sock);
        exit(1);
    }

    // 파일열기
    file = fopen("text.txt", "r");
    if (file == NULL) {
        perror("Failed to open file");
        close(sock);
        exit(1);
    }

    while (1) {
        while (next_seq_num < base + WINDOW_SIZE && fgets(buffer, PAYLOAD_SIZE + 1, file) != NULL) {
            Packet packet;
            strncpy(packet.payload, buffer, PAYLOAD_SIZE);
            packet.checksum = calculate_checksum(packet.payload, PAYLOAD_SIZE);
            packet.packet_number = next_seq_num;

            // packet전송
            send(sock, &packet, PACKET_SIZE, 0);
            printf("packet %d is transmitted. (%.*s)\n", next_seq_num, PAYLOAD_SIZE, packet.payload);

            // 윈도우에 패킷 저장
            window[window_end] = packet;
            window_end = (window_end + 1) % WINDOW_SIZE;
            next_seq_num++;
        }

        // ack를 기다림
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT_INTERVAL;

        int activity = select(sock + 1, &readfds, NULL, NULL, &timeout); //소켓에 대기중이며, 타임아웃 전에 있는 패킷을 받아옴.
        if (activity > 0 && FD_ISSET(sock, &readfds)) { //타임아웃이 되지 않고, 제대로 받아드려졌다면 ack수신
            recv(sock, &ack_number, sizeof(int), 0);
            printf("(ACK = %d) is received.\n", ack_number);

            if (ack_number >= (base + 1) * PAYLOAD_SIZE) { //ack가 제대로 들어왔으면 윈도우를 업데이트 함.
                base = ack_number / PAYLOAD_SIZE;
                window_start = (window_start + (ack_number / PAYLOAD_SIZE - base)) % WINDOW_SIZE;
            }
        } else {
            // 타임아웃이 됐을때, 윈도우의 모든 패킷을 다시 보냄
            printf("Timeout occurred, resending packets from %d to %d\n", base, next_seq_num - 1);
            for (int i = base; i < next_seq_num; i++) {
                Packet packet = window[(window_start + (i - base)) % WINDOW_SIZE];
                send(sock, &packet, PACKET_SIZE, 0);
                printf("packet %d is retransmitted. (%.*s)\n", packet.packet_number, PAYLOAD_SIZE, packet.payload);
            }
        }

        if (feof(file) && base == next_seq_num) {
            break; // 모든패킷이 전송되고 확인함.
        }

        usleep(50000); // 전송주기 0.5초
    }

    fclose(file);
    close(sock);
    return 0;
}
