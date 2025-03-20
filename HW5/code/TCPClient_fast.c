#include "../Common.h"
#include <ctype.h>
#include <stdint.h>

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512
#define window_size 5
#define time_out_interver 15

typedef struct {
	int packetnum;
	int seqnum;
	char message[BUFSIZE];
}Packet;

int char_bit(const char *message); //문자열의 길이를 bit로 변환
unsigned char cal_checksum(const char* str); //바뀐 바이너리를 합하여 checksum 계산
int extract_num(char *ack_c); //받은 ack_c에서 숫자만 추출
int search_packet(Packet packet[], int ack_num); //중복 시 받은 ack를 seq_num으로 가지는 packet_num 찾기

int main(int argc, char *argv[])
{
	//기본 변수 선언
	int retval; //송수신 시 사용하는 변수
	char ack_c[BUFSIZE]; //받을 ack_c
	int temp = 0; //받은 ack_c를 저장할 변수
	int dup = 0; //중복일때 더하기 위해 사용할 변수
	int time_unit = 0; //타이머 체크 변수
	int first_window = 0; //현재 윈도우의 첫번째 번호

	Packet packet[5]; //보낼 패킷 5개
	char *messages[] = { //각 패킷에 대한 메세지
        "I am a boy.",
        "You are a girl.",
        "There are many animals in the zoo.",
        "철수와 영희는 서로 좋아합니다!",
        "나는 점심을 맛있게 먹었습니다."
    };


	int seq_num = 0; //각 패킷에 대해 기본 정보 저장. 
	for(int i=0; i<5; i++){
		packet[i].packetnum = i;
		strcpy(packet[i].message, messages[i]);
		packet[i].seqnum = seq_num;
		seq_num += char_bit(messages[i]);
		unsigned char checksum = cal_checksum(messages[i]);
	}

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

	// 서버와 데이터 통신
	while(1){
		for(int i=0; i<5; i++){ //5개의 패킷 먼저 보냄.
			sleep(1);
			send(sock, &packet[i], sizeof(packet[i]), 0);
			printf("packet %d is transmitted. (%s)\n", i, packet[i].message);
			time_unit++;
		}
		while(extract_num(ack_c) == packet[5].seqnum){ //받은 ack가 현재 가지고 있는 패킷의 마지막seqnum일때 까지 -> 가지고 있는 패킷에 대한 ack를 받을 때 까지
			if(time_unit-first_window >= time_out_interver){ //각 패킷에 대해 타임아웃이 났다면,
				send(sock, &packet[first_window], sizeof(packet[first_window]), 0);
			}
			for(int i=0; i<5; i++){ //5개 패킷 보낸거에 대한 ACK를 받아야함.
				if(dup == 3){ //중복이 3개가 쌓이면 fast retransmission 수행
					int a = search_packet(packet, temp);
					send(sock, &packet[a], sizeof(packet[a]), 0);
					printf("packet %d is retransmitted. (%s)\n", a, packet[a].message);
					break;
				}
				sleep(1);
				retval = recv(sock, &ack_c, sizeof(ack_c), 0); //데이터 받기
				if(temp != extract_num(ack_c)){ //받은 데이터가 전에 받은 데이터가 아니라면 중복갯수 초기화 하고, 임시 저장에 새로운 seq저장
					temp = extract_num(ack_c);
					dup = 0;
					first_window++;
				}else{ //받았던 ack가 들어왔다면, 중복갯수 올림
					dup++;
				}
				printf("%s is received. \n", ack_c);
			}
			time_unit++;
		}
		close(sock);
		return 0;
	};
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

//받은 ack에서 숫자만 추출하는 함수
int extract_num(char *ack_c) { 
    char temp[BUFSIZE] = ""; // 문자열을 저장할 배열로 초기화
    int temp_index = 0; // temp 배열의 인덱스

    // ack_c에서 숫자를 추출하여 temp에 저장
    for (int i = 0; i < strlen(ack_c) && temp_index < BUFSIZE - 1; i++) {
        if (ack_c[i] >= '0' && ack_c[i] <= '9') {
            temp[temp_index++] = ack_c[i];
        }
    }
    temp[temp_index] = '\0'; // 문자열의 끝에 널 종료 문자 추가

    // 문자열을 정수로 변환하여 반환
    return atoi(temp);
}



//중복된 ack를 seq_num으로 가지는 packet_num 찾는 함수.
int search_packet(Packet packet[], int ack_num){
	for(int i=0; i<5; i++){
		if(packet[i].seqnum == ack_num){
			return packet[i].packetnum;
		}
	}
}


//https://cholol.tistory.com/383
