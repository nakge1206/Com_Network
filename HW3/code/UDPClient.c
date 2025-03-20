#include "../Common.h"

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512

struct ThreadArgs {
    sockaddr_in server_sock;
	SOCKET client_sock;
    char *buf;
};

void *thread_receive(void *arg){
	struct ThreadArgs *args = (struct ThreadArgs *)arg;
    SOCKET sock = args->client_sock;
	sockaddr_in serveraddr = args -> server_sock;
	char *receive_buf = (char *)malloc(BUFSIZE + 1);
	int retval;
	struct sockaddr_in peeraddr;
	socklen_t addrlen;

	addrlen = sizeof(peeraddr);
	retval = recvfrom(sock, receive_buf, BUFSIZE, 0,
			(struct sockaddr *)&peeraddr, &addrlen);
	if (retval == SOCKET_ERROR) {
		err_display("recvfrom()");
		free(receive_buf);
		pthread_exit(NULL);
	}

	// 송신자의 주소 체크
	if (memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))) {
		printf("[오류] 잘못된 데이터입니다!\n");
		free(receive_buf);
		pthread_exit(NULL);
	}	

	receive_buf[retval] = '\0';
	pthread_exit(receive_buf);
}

void *thread_send(void *arg){
	struct ThreadArgs *args = (struct ThreadArgs *)arg;
    SOCKET sock = args->client_sock;
	sockaddr_in serveraddr = args -> server_sock;
	char *buf = args -> buf;
	int retval = 0;
	int len;
	
	while(1){
		// 데이터 입력
		printf("\n[Client-보낼 데이터] ");
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
			printf("Client: %s\n", buf);
			break;
		}
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	int retval;

	// 명령행 인수가 있으면 IP 주소로 사용
	if (argc > 1) SERVERIP = argv[1];

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// 소켓 주소 구조체 초기화
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	// 데이터 통신에 사용할 변수
	struct ThreadArgs args;
	args.server_sock = serveraddr;
	args.client_sock = sock;

	// 서버와 데이터 통신
	while (1) {
		char *buf;
		pthread_t tid1, tid2;
		// 데이터 입력
		if(pthread_create(&tid1, NULL, thread_send, (void *)&args) != 0){
				printf("thread_send create error");
				pthread_join(tid1, NULL);
				break;
			}else{
				pthread_join(tid1, NULL);
			}
		/*
		printf("\n[보낼 데이터] ");
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
		}
		printf("[UDP 클라이언트] %d바이트를 보냈습니다.\n", retval); */

		// 데이터 받기
		if(pthread_create(&tid2, NULL, thread_receive, (void *)&args) != 0){
			printf("thread_receive create error");
			break;
		}else{
			pthread_join(tid2, (void**)&buf);
		}
		if(strlen(buf) == 0){
			free(buf);
			break;
		}
		// 받은 데이터 출력
		printf("Server: %s\n", buf);

		free(buf);


		/*addrlen = sizeof(peeraddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(struct sockaddr *)&peeraddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			break;
		}

		// 송신자의 주소 체크
		if (memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))) {
			printf("[오류] 잘못된 데이터입니다!\n");
			break;
		}

		// 받은 데이터 출력
		buf[retval] = '\0';
		printf("[UDP 클라이언트] %d바이트를 받았습니다.\n", retval);
		printf("[받은 데이터] %s\n", buf);*/
	}

	// 소켓 닫기
	close(sock);
	return 0;
}
