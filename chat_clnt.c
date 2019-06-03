#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 100



void error_handler(char* message);
void * recv_thread_main(void* arg);

char nickname[30] = { '\0', };

int main(int argc, char* argv[]){
	int serv_sd;
	struct sockaddr_in serv_addr;
	pthread_t thread_id;
	char buf[50] = {'\0', };
	
	int str_len;	

	if(argc != 4){
		printf("Usage : %s <IP> <PORT> <NICKNAME> \n", argv[0]);
		exit(1);
	}

	serv_sd = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sd == -1)
		error_handler("socket() error");
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));
	strcpy(buf, argv[3]);

	if((connect(serv_sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))==-1){
		error_handler("connect() error");
	}else{
		printf("connected\n");
		// 받는 역할만 하는 쓰레드 생성 
		if((pthread_create(&thread_id, NULL, recv_thread_main, (void*)&serv_sd)) != 0){
			error_handler("pthread_create() error");
		}
		
	}
	write(serv_sd, buf, sizeof(buf));
	pthread_detach(thread_id);
	strcpy(nickname, buf);
	strcpy(nickname, strcat(nickname, ">>\0"));
	// 여기까지 소켓 커넥트 


	// 메인쓰레드는 계속 보내는 역할만 수행한다.
	while(1){
		fputs(nickname, stdout);
		fgets(buf, BUF_SIZE, stdin);
		if(!strcmp(buf, "q\n") || !strcmp(buf, "Q\n"))
			break;
		str_len = write(serv_sd, buf, strlen(buf));

	}
	close(serv_sd);
	return 0;
}



// 받기만 하는 쓰레드
// 이 쓰레드는 받기만 한다.
// 화면에 뿌려주기도한다.
void* recv_thread_main(void* arg){
	int serv_sd = *(int*)arg;
	int recv_len;
	int str_len;
	char buf[BUF_SIZE];

	while(1){

		recv_len = read(serv_sd, buf, BUF_SIZE);
		
		

		write(1, "\033[0G", 4);//커서위치 변경

		buf[recv_len] = '\0';
		fputs(buf, stdout);

		
		fprintf(stderr, "%s>", nickname); // 아이디 

		memset(buf, '\0', sizeof(buf));
	}

}






void error_handler(char * message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

		

	


