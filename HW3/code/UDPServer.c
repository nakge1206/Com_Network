#include "../Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512

struct ThreadArgs {
    SOCKET server_sock;
	sockaddr_in client_sock;
    char *buf;
};

void *thread_receive(void *arg){
	struct ThreadArgs *args = (struct ThreadArgs *)arg;
    SOCKET sock = args->server_sock;
	sockaddr_in clientaddr;
	char *receive_buf = (char *)malloc(BUFSIZE + 1);
	int retval;
	socklen_t addrlen = sizeof(clientaddr);

	retval = recvfrom(sock, receive_buf, BUFSIZE, 0,
		(struct sockaddr *)&clientaddr, &addrlen);
	if (retval == SOCKET_ERROR) {
		err_display("recvfrom()");
		free(receive_buf);
		pthread_exit(NULL);
	}

	receive_buf[retval] = '\0';
	args->client_sock = clientaddr;
	pthread_exit(receive_buf);
}

void *thread_send(void *arg){
	struct ThreadArgs *args = (struct ThreadArgs *)arg;
    SOCKET sock = args->server_sock;
	sockaddr_in serveraddr = args -> client_sock;
	char *buf = args -> buf;
	int retval = 0;
	int len;

	while(1){
		// 데이터 입력
		printf("\n[Server-보낼 데이터] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		// '\n' 문자 제거
		len = (int)strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			break;

		// 데이터 보내기
		retval = sendto(sock, buf, (int)strlen(buf), 0,
			(struct sockaddr *)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			break;
		}else{
			printf("Server: %s\n", buf);
			break;
		}
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	int retval;

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// 데이터 통신에 사용할 변수
	struct ThreadArgs args;
    args.server_sock = sock;
    args.buf = (char *)malloc(BUFSIZE + 1);

	// 클라이언트와 데이터 통신
	while (1) {
		char *buf;
		pthread_t tid1, tid2;
		// 데이터 받기
		if(pthread_create(&tid1, NULL, thread_receive, (void *)&args) != 0){
			printf("thread_receive create error");
			break;
		}else{
			pthread_join(tid1, (void**)&buf);
		}
		if(strlen(buf) == 0){
			free(buf);
			break;
		}
		/*
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(struct sockaddr *)&clientaddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			break;
		}

		// 받은 데이터 출력
		buf[retval] = '\0';
		*/
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &args.client_sock.sin_addr, addr, sizeof(addr));
		printf("Client: %s\n", buf);

		// 데이터 보내기
		if(pthread_create(&tid1, NULL, thread_send, (void *)&args) != 0){
				printf("thread_send create error");
				pthread_join(tid1, NULL);
				break;
			}else{
				pthread_join(tid1, NULL);
			}
		/*
		retval = sendto(sock, buf, retval, 0,
			(struct sockaddr *)&clientaddr, sizeof(clientaddr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			break;
		}
		*/
	}
	free(args.buf);
	
	// 소켓 닫기
	close(sock);
	return 0;
}
