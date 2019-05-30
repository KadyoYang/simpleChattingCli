#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>




#define BUF_SIZE 50

#define EXIT_ROOM_CODE /exit
#define MAKE_ROOM_CODE /make
#define SHOW_ROOM_CODE /ls
#define MESS_USER_CODE /w
#define JOIN_ROOM_CODE /join
#define BREK_ROOM_CODE /delete


void error_handler(char *message);


int main(int argc, char* argv[]){
	int serv_sd, clnt_sd; //서버 클라이어느 소켓 디스크립터 변수 선언
	struct sockaddr_in serv_addr, clnt_addr;
	
	if(argc != 2){
		printf("Usage : %s <port> \n", argv[0]);
		exit(1);
	}

	
	serv_sd = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sd == -1)
		error_handler("socket() error");
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if((bind(serv_sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handler("bind() error");
	if((listen(serv_sd, 5))==-1)
		error_handler("listen() error");
	























}






void error_handler(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
	
