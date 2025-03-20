#include "../Common.h"

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
	SOCKET client_sock = *((SOCKET *)arg);
	int retval = 0;
	char buf[BUFSIZE + 1];
	int len;
	while(1){
		// 데이터 입력
		printf("\n[Server-보낼 데이터] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			continue;

		// '\n' 문자 제거
		len = (int)strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			continue;

		// 데이터 보내기
		retval = send(client_sock, buf, (int)strlen(buf), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
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
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char *buf = NULL;


	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));
		
		// 클라이언트와 데이터 통신
		while (1) {
			pthread_t tid1, tid2;
			// 데이터 받기
			if(pthread_create(&tid1, NULL, thread_receive, (void *)&client_sock) != 0){
				printf("thread_receive create error");
				free(buf);
				break;
			}else{
				pthread_join(tid1, (void**)&buf);
			}
			if(strlen(buf) == 0){
				break;
			}
			// 받은 데이터 출력
			printf("Client: %s\n", buf);

			free(buf);

			// 데이터 보내기
			if(pthread_create(&tid2, NULL, thread_send, (void *)&client_sock) != 0){
				printf("thread_send create error");
				pthread_join(tid2, NULL);
				break;
			}else{
				pthread_join(tid2, NULL);
			}
		}

		// 소켓 닫기
		close(client_sock);
		printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));
	}

	// 소켓 닫기
	close(listen_sock);
	return 0;
}
