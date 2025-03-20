#include "../Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512


int Leafyear(int year){ //윤년 파악
	if( ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0) ) return 1; // 윤년이면 1 반환
	else return 0; // 평년이면 0 반환
}

int get_monthinday(int year, int month){
	if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12)
		return 31; // 1,3,5,7,8,10,12월은 31일까지
	if (month == 4 || month == 6 || month == 9 || month == 11)
		return 30; //나머지는 30일까지
	if(month == 2){
		if (Leafyear(year) == 1)
			return 29; // 윤년이면 29일지
		else
			return 28; //평년이면 28일까지
	} return 0;
}


int get_totalday(int year, int month){
	int total = 0;
	for (int i = 2000; i < year; i++){ //2000년 부터 해당 년도의 이전년도 까지의 모든 날의 합
		if(Leafyear(i) == 1) total += 366; // 윤년이라면 366일을 더하기
		else total += 365; //아니라면 365일을 더하기
	}
	for (int i = 1; i < month; i++) total = total + get_monthinday(year, i); // 해당 월까지의 날의 합
	return total;
}

int create_firstday(int year, int month){ //2000년 이후의 해당 년도, 월의 첫날의 요일 반환
	int startday_2000 = 6; //2000년 1월 1일의 요일은 토요일
	int totalday = get_totalday(year, month); // 2000년 이후에 해당 년도, 월까지 날의 합
	return (startday_2000 + totalday) % 7; // 요일 반환을 위해 %7 사용
}

void get_title(char *buf){
	strcpy(buf, "\n-----------------------------\n"); 
	strcat(buf, " SUN MON TUE WED THU FRI SAT\n"); 
}

void get_body(char *buf, int year, int month){
	int firstday = create_firstday(year, month); // 해당 년,월에 처음 시작되는 요일
	int totalday = get_monthinday(year, month); // 해당월에 몇일까지 있는지
	char day[5]; //공백 담을 배열
	
	for(int i = 0; i < firstday; i++) strcat(buf, "    "); //해달 월 요일 시작 전까지 공백 채우기
	for (int i = 1; i <= totalday; i++){
		sprintf(day, "%4d", i); //일별로 총 4자리 되게 배열 맞추기
		strcat(buf, day); //그 날짜 넣기
		if( (i + firstday) % 7 == 0) strcat(buf, "\n"); // 요일의 한줄을 다 채우면 한 줄 내리기
	}
}

void create_calendar(char *buf, int year, int month){
	buf[0] = '\0';
	get_title(buf); //buf에 title 추가
	get_body(buf, year, month); //buf에 body 추가
}


int main(int argc, char *argv[]){
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
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",addr, ntohs(clientaddr.sin_port));

		// 클라이언트와 데이터 통신
		while (1) {
			// 데이터 받기
			retval = recv(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;

			// 받은 데이터 출력
			buf[retval] = '\0';
			printf("[TCP/%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf);
			
			//받은 데이터 달력으로 바꾸기
			if(buf[4] == '.'){
				int year = atoi(buf);
				char month_char[3];
				month_char[0] = buf[6];
				month_char[1] = buf[7];
				int month = atoi(month_char);
				create_calendar(buf, year, month);
				printf("%s\n", buf);
			} else {
				strcat(buf, " <- 년도. 월. (ex. 0000. 00.) 단위가 아닙니다. 다시 입력해 주세요");	
			}

			// 데이터 보내기
			retval = send(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
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
