# Com_Network
컴퓨터네트워크 과목. TCP/UDP에 관한 다양한 이론을 ubuntu C로 구현

HW1.TCP
  1) TCP Server-Client.
     Server : 소켓활성화, 클라이언트 접속 시 IP address 및 port 정보 표시, 클라이언트 측에서 받은 메세지 출력 후 다시 메세지 전송
     Client : 소켓입장, 메세지 전송 시 몇byte를 보냈고 받았는지 정보 출력, 받은 메세지 출력
  2) TCP Calender.
     Server : 소켓활성화, 클라이언트 접속 시 IP address 및 port 정보 표시, 클라이언트 측에서 {년도. 월.}을 받으면 그에 맞는 달력 전송
     Client : 소켓입장, {년도. 월.}을 전송 후 받은 달력 출력, 이때의 byte도 출력

HW2. UDP
  1) HW1.1과 같은 기능을 UDP로 구현.
  2) UDP로 파일 전송 구현.
     Client가 "request ~.txt" 보낼 시 서버에 있는 txt파일 내 글자를 모두 전송받아 표시됨.

HW3. Server-Client 채팅 프로그램 구현.
    server-client가 서로 메세지를 주고 받는 기능을 TCP와 UDP로 각각 구현

HW4. Retransmisson알고리즘 구현.
    Go-back-n 알고리즘, Selective repeat 알고리즘 구현

HW5. TCP fast retransmisson알고리즘 구현

HW6. 버퍼 overflow에 의한 Packet loss 상황에서의 통신 구현.

HW7. Congestion Control을 고려한 TCP통신 구현.
