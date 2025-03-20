#include "../Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512

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
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char buf[BUFSIZE + 1];

	// 클라이언트와 데이터 통신
	while (1) {
		// 데이터 받기
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(struct sockaddr *)&clientaddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			break;
		}

		// 받은 데이터 출력
		buf[retval] = '\0';
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("[UDP/%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf);

		if (strncmp(buf, "request \"", 9) == 0 && buf[retval - 1] == '"') {
			printf("The server received a request from a client\n");
			fflush(stdout);
            // 요청된 파일 이름 추출
            char filename[BUFSIZE];
            strncpy(filename, buf + 9, retval - 10);
            filename[retval - 10] = '\0';

			FILE *file = fopen(filename, "rb");
        	if (file == NULL) {
            	perror("fopen");
            	continue;
			}
			// 파일 내용을 클라이언트에게 전송
        	while ((retval = fread(buf, 1, BUFSIZE, file)) > 0) {
            	retval = sendto(sock, buf, retval, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
            	if (retval == SOCKET_ERROR) {
                	err_display("sendto()");
                	break;
            	}
			}
			// 파일 전송이 완료되면 파일 닫기
        	fclose(file);
			printf("The server sent “novel.txt” to the client\n");
			fflush(stdout);
        } else{// 데이터 보내기
			retval = sendto(sock, buf, retval, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
			if (retval == SOCKET_ERROR) {
				err_display("sendto()");
				break;
			}
		}
	}

	// 소켓 닫기
	close(sock);
	return 0;
}
