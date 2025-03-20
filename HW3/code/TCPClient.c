#include "../Common.h"

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512

void *thread_receive(void *arg){
	SOCKET client_sock = *((SOCKET *)arg);
	int retval;
	char *receive_buf = (char *)malloc(BUFSIZE + 1);
	retval = recv(client_sock, receive_buf, BUFSIZE, 0);
	if (retval == SOCKET_ERROR) {
		err_display("recv()");
		free(receive_buf);
		pthread_exit(NULL);
	}
	receive_buf[retval] = '\0';
	pthread_exit(receive_buf);
}

void *thread_send(void *arg){
	SOCKET sock = *((SOCKET *)arg);
	int retval = 0;
	char buf[BUFSIZE + 1];
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
		retval = send(sock, buf, (int)strlen(buf), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
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
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	// 데이터 통신에 사용할 변수
	char *buf = NULL;

	// 서버와 데이터 통신
	while (1) {
		pthread_t tid1, tid2;
		// send
			if(pthread_create(&tid1, NULL, thread_send, (void *)&sock) != 0){
				printf("thread_send create error");
				pthread_join(tid1, NULL);
				break;
			}else{
				pthread_join(tid1, NULL);
			}

		// 데이터 받기
		if(pthread_create(&tid2, NULL, thread_receive, (void *)&sock) != 0){
				printf("thread_receive create error");
				free(buf);
				break;
			}else{
				pthread_join(tid2, (void**)&buf);
			}
			if(strlen(buf) == 0){
				break;
			}
			// 받은 데이터 출력
			printf("Server: %s\n", buf);

			free(buf);
	}

	// 소켓 닫기
	close(sock);
	return 0;
}
