#include "../Common.h"
#include <ctype.h>

#define SERVERPORT 9000
#define BUFSIZE    512
#define window_size 4
#define full_size 6

void create_ack(int number, char *result){
	sprintf(result, "ACK %d", number);
	strcat(result, "\0");
}

int extract_number(char *arr){
	int number = 0;
	for(int i=0; arr[i]!='\0'; i++){
		if(isdigit(arr[i])){
			number = number*10 + (arr[i]-'0');
		}else continue;;
	}
	return number;
}

int main(int argc, char *argv[])
{
	int retval;
	int recv_seq;
	int first_seq = 0;
	int sr_buf[window_size];
	char ACK_num[BUFSIZE+1];

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
	char buf[BUFSIZE + 1];

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
		//printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));

		// 클라이언트와 데이터 통신
		while (1) {
			retval = recv(client_sock, buf, BUFSIZE, 0);
			if(retval == 0){
				continue;
			}
			buf[retval] = '\0';
			recv_seq = extract_number(buf);
			if(first_seq == recv_seq){
				if(sr_buf[1] == recv_seq+1 || sr_buf[2] == recv_seq+2 || sr_buf[3] == recv_seq+3){
					first_seq += 4;
					create_ack(recv_seq ,ACK_num);
					sleep(1);
					send(client_sock, ACK_num, (int)strlen(ACK_num), 0);
					printf("\"%s\" is received. %s, packet %d, packet %d, and packet %d are delivered. \"%s\" is transmitted \n", buf, buf, sr_buf[1], sr_buf[2], sr_buf[3], ACK_num);
				}else{
					create_ack(recv_seq ,ACK_num);
					sleep(1);
					send(client_sock, ACK_num, (int)strlen(ACK_num), 0);
					printf("\"%s\" is received. \"%s\" is transmitted.\n", buf, ACK_num);
					first_seq ++;
				}
			}else {
				sr_buf[recv_seq-first_seq] = recv_seq;
				create_ack(recv_seq, ACK_num);
				sleep(1);
				send(client_sock, ACK_num, (int)strlen(ACK_num), 0);
				printf("\"%s\" is received and buffered. \"%s\" is retransmitted.\n", buf, ACK_num);
			}
			if(recv_seq == full_size){
				break;
			}

		}

		// 소켓 닫기
		close(client_sock);
		//printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",addr, ntohs(clientaddr.sin_port));
	}

	// 소켓 닫기
	close(listen_sock);
	return 0;
}
