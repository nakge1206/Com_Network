#include "../Common.h"
#include <ctype.h>

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512
#define window_size 4
#define full_size 6
#define time_out_interver 5

void create_number(int number, char *result){
	sprintf(result, "Packet %d", number);
	strcat(result, "\0");
}
void create_timeout(int number, char *result){
	sprintf(result, "Packet %d is timeout", number);
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
	int recv_seq; // 받은 sequnce number
	int time_unit = 0;
	int first_seq_number = 0;
	int ignore = 0;
	int first = 0;
	char packet_num[BUFSIZE+1];
	char time_out_char[BUFSIZE+1];


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
	char buf[BUFSIZE + 1];

	// 서버와 데이터 통신
	while (first_seq_number + window_size <= full_size) {
		for(int i=0; i<window_size; i++){ //맨처음에 window_size만큼 packet을 보냄.
			create_number(first_seq_number + i, packet_num);
			int a = (int)strlen(packet_num);
			sleep(1);
			if(first == 0 && first_seq_number+i == 2){
				first ++;
			}else{
				send(sock, packet_num, a, 0);
			}
			printf("\"%s\" is transmitted.\n", packet_num);
			time_unit ++;
		}
		while(first_seq_number + window_size <= full_size){
			if(first_seq_number + 4 < time_unit){
				create_timeout(first_seq_number, time_out_char);
				printf("%s \n", time_out_char);
				time_unit = 0;
				break; // 만약 타임아웃이면 종료
			}
			if(first_seq_number + window_size < 14){
				retval = recv(sock, buf, 5, 0);
			} else{
				retval = recv(sock, buf, 6, 0);
			}
			buf[retval] = '\0';
			recv_seq = extract_number(buf); // ack뒤에 있는 숫자만 분리
			if(recv_seq == first_seq_number){ // 만약 받은게 순차적이면 다음껄 보냄
				create_number(first_seq_number+window_size, packet_num);
				sleep(1);
				send(sock, packet_num, (int)strlen(packet_num), 0);
				first_seq_number++;
				printf("\"%s\"  is received. \"%s\" is transmitted. \n", buf, packet_num);
				time_unit ++;
			}else if(ignore == 0){ // 그게 아니라면 ignore를 올리고 다음부터 무시함.
				sleep(1);
				printf("\"%s\" is received and ignored.\n", buf);
				ignore ++;
				time_unit ++;
			}else {
				time_unit++;
				continue;
			}
		}
	}

	// 소켓 닫기
	close(sock);
	return 0;
}
