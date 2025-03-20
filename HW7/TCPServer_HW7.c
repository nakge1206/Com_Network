#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

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

typedef struct {
    Packet packets[BUFFER_SIZE / PACKET_SIZE];
    int head;
    int tail;
    int count;
    int socket;
    pthread_mutex_t lock;
} PacketBuffer;

unsigned short calculate_checksum(char *data, int size) {
    unsigned short checksum = 0;
    for (int i = 0; i < size; i++) {
        checksum += data[i];
    }
    return checksum;
}

void *receiver_thread(void *arg) {
    PacketBuffer *buffer = (PacketBuffer *)arg;
    FILE *output_file = fopen("output1.txt", "w");
    if (!output_file) {
        perror("Failed to open output file");
        return NULL;
    }

    while (1) {
        usleep(1000000); // 1초 대기
        pthread_mutex_lock(&buffer->lock);
        if (buffer->count > 0) {
            Packet packet = buffer->packets[buffer->head];
            buffer->head = (buffer->head + 1) % (BUFFER_SIZE / PACKET_SIZE);
            buffer->count--;
            pthread_mutex_unlock(&buffer->lock);

            // 파일 끝 패킷 확인
            if (packet.payload[0] == '\0') {
                printf("End of file packet received.\n");
                break;
            }

            // 체크섬 확인
            if (calculate_checksum(packet.payload, PAYLOAD_SIZE) == packet.checksum) {
                int bytes_received = 0;
                for (int i = 0; i < PAYLOAD_SIZE; i++) {
                    if (packet.payload[i] != '\0') {
                        bytes_received++;
                    }
                }
                fwrite(packet.payload, sizeof(char), bytes_received, output_file);
                printf("packet %d is received and there is no error. (%.*s) (ACK = %d) is transmitted.\n", packet.packet_num, bytes_received, packet.payload, bytes_received);
                send(buffer->socket, &bytes_received, sizeof(bytes_received), 0);
            } else {
                printf("Checksum error for packet %d\n", packet.packet_num);
                unsigned short error_ack = 0;
                send(buffer->socket, &error_ack, sizeof(error_ack), 0);
            }
        } else {
            pthread_mutex_unlock(&buffer->lock);
        }
    }

    fclose(output_file);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    PacketBuffer buffer;
    buffer.head = 0;
    buffer.tail = 0;
    buffer.count = 0;
    pthread_mutex_init(&buffer.lock, NULL);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    buffer.socket = new_socket;

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, receiver_thread, (void *)&buffer);

    while (1) {
        Packet packet;
        int valread = read(new_socket, &packet, PACKET_SIZE);
        if (valread > 0) {
            pthread_mutex_lock(&buffer.lock);
            if (buffer.count < BUFFER_SIZE / PACKET_SIZE) {
                buffer.packets[buffer.tail] = packet;
                buffer.tail = (buffer.tail + 1) % (BUFFER_SIZE / PACKET_SIZE);
                buffer.count++;
                printf("Stored packet %d in buffer\n", packet.packet_num);

                // 파일 끝 패킷 확인
                if (packet.payload[0] == '\0') {
                    break;
                }
            } else {
                printf("Buffer overflow, dropping packet %d\n", packet.packet_num);
            }
            pthread_mutex_unlock(&buffer.lock);
        }
    }

    close(new_socket);
    close(server_fd);
    return 0;
}

