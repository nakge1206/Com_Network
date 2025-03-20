#include "../Common.h"
#include <ctype.h>

#define SERVERPORT 9000
#define BUFSIZE    512
#define window_size 4
#define full_size 6

typedef struct 
{
	int packetnum;
	int seqnum;
	char message[BUFSIZE];

}Packet;

int char_bit(const char *message);
int all_ones(unsigned int value);
unsigned char cal_checksum(const char* str);
void create_ack(int number, char *result);

int main(int argc, char *argv[])
{
	int retval;
	int first = 0; //처음 들어온건지 아닌지 확인

	Packet packet;

	int ack = 0; //맨처음 도착하는 seq_num은 0이므로, 맨처음 패킷을 정상적으로 받기 위함.
	unsigned char checksum[5] = {
        cal_checksum("I am a boy."),
        cal_checksum("You are a girl."),
        cal_checksum("There are many animals in the zoo."),
        cal_checksum("철수와 영희는 서로 좋아합니다!"),
        cal_checksum("나는 점심을 맛있게 먹었습니다.")
    };
	
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
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));

		// 클라이언트와 데이터 통신
		while (1) {
            // 데이터 수신
            retval = recv(client_sock, &packet, sizeof(packet), 0);
            if (retval == SOCKET_ERROR) {
                err_display("recv()");
                break;
            } else if (retval == 0) {
                // 클라이언트가 연결을 종료
                printf("클라이언트가 연결을 종료했습니다.\n");
                break;
            }

			unsigned char left = cal_checksum(packet.message);
			unsigned char right = checksum[packet.packetnum];

			if(all_ones(~left + right)){ // checksum 계산 후, 알고있던 checksum과 같다면
				if(first == 0 && packet.packetnum == 1){ //처음으로 들어온 packet1은 바로 넘기기 -> packet loss 구현
					first++;
					continue;
				}
				if(packet.seqnum == ack){ //받은 패킷의 seqnum이 현재 저장했던 ack와 같다면 -> 정상적으로 패킷을 받았을때
					ack += char_bit(packet.message); //ack에 메세지의 바이트 더하고,
					char ack_c[BUFSIZE];
					create_ack(ack, ack_c);
					send(client_sock, ack_c, sizeof(ack_c), 0);
					printf("packet %d is received and there is no error. (%s) (ACK=%d)is transmitted.\n",packet.packetnum, packet.message, ack); // 수신된 데이터 출력 및 다음 seqnum을 ack로 요청
				}else{
					char ack_c[BUFSIZE];
					create_ack(ack, ack_c);
					send(client_sock, ack_c, sizeof(ack_c), 0);
					printf("packet %d is received and there is no error. (%s) (ACK=%d)is transmitted.\n", packet.packetnum, packet.message, ack); //아직 못받았던 ack 요청
				}
			} else{ //checksum이 이상하면 error있다고 표시.
				printf("packet %d is received and there is error.\n", packet.packetnum);
			}
		}

		// 소켓 닫기
		close(client_sock);
		printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",addr, ntohs(clientaddr.sin_port));
		break;
	}

	// 소켓 닫기
	close(listen_sock);
	return 0;
}

int all_ones(unsigned int value) {
    // 바이너리 값을 비트 단위로 확인하여 모든 비트가 1인지를 검사
    while (value) {
        if ((value & 1) == 0) {
            return 0; // 하나 이상의 비트가 0이면 모든 비트가 1이 아님
        }
        value >>= 1; // 다음 비트로 이동
    }
    return 1; // 모든 비트가 1임
}


// checksum을 계산하는 함수
unsigned char cal_checksum(const char* str) {
    size_t len = strlen(str);
    unsigned int sum = 0; // 바이너리들의 합을 저장할 변수

    // 각 문자의 ASCII 코드를 바이너리로 변환하여 더함
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        for (int j = 7; j >= 0; j--) {
            sum += ((c >> j) & 1); // 각 비트를 더함
        }
    }

    // 모든 바이너리 표현의 합을 반환
    return sum;
}

//발송에 필요한 ack 문자열 만드는 함수
void create_ack(int number, char *result){
	sprintf(result, "(ack= %d)", number);
	strcat(result, "\0");
}

//문자열의 bit를 구하는 함수
int char_bit(const char *message1) {
    char message[BUFSIZE];
    strcpy(message, message1); // 문자열 복사
    
    int i = 0;
    int bit = 0;
    while (message[i] != '\0') { // 문자열의 끝까지 반복
        if (message[i] & 0x80) { // 한글 문자인 경우
            i += 3;
            bit += 2;
        } else { // 영어 또는 기타 문자인 경우
            i++;
            bit += 2;
        }
        if (bit > BUFSIZE) { // 문자열 길이가 60을 넘으면 종료
            break;
        }
    }
    return bit;
}