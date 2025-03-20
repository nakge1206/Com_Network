#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 8080
#define PACKET_SIZE 14
#define PAYLOAD_SIZE 10
#define BUFFER_SIZE 70
#define TIMEOUT 5

typedef struct {
    char payload[PAYLOAD_SIZE];
    unsigned short checksum;
    unsigned short packet_num;
} Packet;

unsigned short calculate_checksum(char *data, int size) {
    unsigned short checksum = 0;
    for (int i = 0; i < size; i++) {
        checksum += data[i];
    }
    return checksum;
}

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    unsigned short ack;
    FILE *input_file = fopen("input1.txt", "r");
    if (!input_file) {
        perror("Failed to open input file");
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    int packet_num = 0;
    char payload[PAYLOAD_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(payload, sizeof(char), PAYLOAD_SIZE, input_file)) > 0) {
        Packet packet;
        memset(packet.payload, 0, PAYLOAD_SIZE); // 패딩을 위해 초기화
        memcpy(packet.payload, payload, bytes_read);
        packet.checksum = calculate_checksum(packet.payload, PAYLOAD_SIZE);
        packet.packet_num = packet_num;

        send(sock, &packet, PACKET_SIZE, 0);
        printf("packet %d is transmitted. (%.*s)\n", packet_num, (int)bytes_read, payload);
        packet_num++;

        // ACK 수신
        valread = read(sock, &ack, sizeof(ack));
        if (valread > 0) {
            printf("(ACK = %d bytes) is received.\n", ack);
        } else {
            printf("Failed to receive ACK\n");
        }

        usleep(500000); // 0.5초 대기
    }

    // 파일 끝 알림
    Packet end_packet;
    memset(&end_packet, 0, sizeof(end_packet));
    send(sock, &end_packet, PACKET_SIZE, 0);

    fclose(input_file);
    close(sock);
    return 0;
}
