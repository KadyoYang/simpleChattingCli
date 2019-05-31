#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>


#define MAX_ROOM_NUM 5
#define MAX_USER_NUM 30
#define BUF_SIZE 50

#define EXIT_ROOM_CODE /exit
#define MAKE_ROOM_CODE /make
#define SHOW_ROOM_CODE /ls
#define MESS_USER_CODE /w
#define JOIN_ROOM_CODE /join
#define DELT_ROOM_CODE /delete

struct sd_nickname_t{
	int sd;
	char* nickname;
	int room; // 0 is room list i > 0 is specific room
	int isuse; // 0 false 1 true
};

struct room_t{
	char* title;
	int isuse;
};



// 클라이언트 에서는 먼저 ./chat_clnt 127.0.0.1 9849 id 식으로 접속을 해야함 
// 그 정보가 내가 손수만든 구조체에 들어가서 포인터로 날아간다
void* clnt_thread_main(void* arg);
void error_handler(char *message);
int get_unused_num();

struct sd_nickname_t user_info[MAX_USER_NUM]; 
struct root_t room_info[MAX_ROOM_NUM];



int main(int argc, char* argv[]){
	int serv_sd, clnt_sd; //서버 클라이어느 소켓 디스크립터 변수 선언
	struct sockaddr_in serv_addr, clnt_addr;
	struct socklen_t clnt_addr_size;
	struct sd_nickname_t sd_nickname;	
	char nickname[BUF_SIZE];
	int strlen = 0;
	pthread_t thread_id;
	int arraytemp;

	memset(user_info, 0, sizeof(user_info));
	memset(room_info, 0, sizeof(room_info));

	// 그 위에 배열 isuse 0 으로 다 초기화 해야한다 	
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

	if((bind(serv_sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))==-1)
		error_handler("bind() error");
	if((listen(serv_sd, 5))==-1)
		error_handler("listen() error");
	
	while(1){
		clnt_addr_size = sizeof(clnt_addr);
		if((clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_addr, &clnt_addr_size)) == -1){
			close(clnt_sd);
			continue;
		}
		strlen = read(clnt_sd, buf, BUF_SIZE);
		memset(sd_nickname, 0, sizeof(sd_nickname));
		sd_nickname.sd = clnt_sd;
		sd_nickname.nickname = &buf[0];	
		sd_nickname.room = 0;
		sd_nickname.isuse = 1;
		arraytemp = get_blank_user();
		user_info[arraytemp] = sd_nickname; // ##########임게영역 
		if((pthread_create(&thread_id, NULL, clnt_thread_main, (void*)&arraytemp)) != 0){
			close(clnt_sd);
			continue;
		}
		printf("new client is connected %s %s \n", buf, inet_ntoa(clnt_addr.sin_addr)); 	
	
	}	
	close(serv_sd);
	return 0;
}



void* clnt_thread_main(void* arg){//arg로 그냥 몇번째 유저인지 번호만 알려주고 그 번호로 전역구조체 참조해서use
	int arraynum = *((int*)arg);
	// 흠.. 
	int clnt_sd = user_info[arraynum].sd; // ############### im gae
	int str_len, roomtemp;
	char buf[BUF_SIZE] = {0, };
	char buftemp[BUF_SIZE] = {0, };
	char bufnick[BUF_SIZE] = {0, };
	char command[10] = {0, };
	char numbuf[10] = {0, };
	
	

	while((str_len = read(clnt_sd, buf, sizeof(buf))) != 0){
		if((strcmp(buf[0], '/')) == 0){ // if message contains command

			for(int i = 0 ; i < str_len; i++){
				if((strcmp(buf[i], ' ')) == 0){
					for(int j = 0 ; j < i ; j++){
						command[j] = buf[j];
					}
					command[i] = '\0';
					
					break;
				}
				
			}

			if((strncmp(command, EXIT_ROOM_CODE, sizeof(EXIT_ROOM_CODE)))==0){
				
				exit_room(clnt_sd);	
			}else if((strncmp(command, MAKE_ROOM_CODE, sizeof(MAKE_ROOM_CODE)))==0){
				char* title;
				int j = 0;
				for(int i = 0; i , str_len; i++){
					if(strcmp(buf[i], " ")==0){
						j = i + 1;
						for(j, int k = 0; j < str_len; j++, k++){
							buftemp[k] = buf[j];
						}
						break;
					}
				}
				make_room(clnt_sd, buftemp);

			}else if((strncmp(command, SHOW_ROOM_CODE, sizeof(SHOW_ROOM_CODE)))==0){
				show_room(clnt_sd);

			}else if((strncmp(command, MESS_USER_CODE, sizeof(MESS_USER_CODE)))==0){
				int j = 0;
				int n = 0;
				for(int i = 0; i < str_len; i++){
					if(strcmp(buf[i], " ")==0){
						j = i + 1;
						for(j, int k = 0; j < str_len ; j++, k++){
							bufnick[k] = buf[j];
							if(strcmp(buf[j], " ") == 0){
								n = j + 1;
								for(n, int u = 0; n < str_len; n++, u++){
									buftemp[u] = buf[n];
								}
								break;
							}
						}
						break;
					}
				}
				mess_user(clnt_sd, bufnick, buftemp);

			}else if((strncmp(command, JOIN_ROOM_CODE, sizeof(JOIN_ROOM_CODE)))==0){
				int j = 0;
				for(int i = 0; i < str_len; i++){
					if(strcmp(buf[i], " ")==0){
						j = i + 1;
						for(j, int k = 0; j < str_len ; j++, k++){
							numbuf[k] = buf[j];
						}
						break;
					}
				}
				join_room(clnt_sd, atoi(numbuf));
				
			}else if((strncmp(command, DELT_ROOM_CODE, sizeof(DELT_ROOM_CODE)))==0){
				int j = 0;
				for(int i = 0; i < str_len; i ++){
					if(strcmp(buf[i], " ")==0){
						j = i + 1;
						for(j, int k = 0; j < str_len; j++, k++){
							numbuf[k] = buf[j];
						}
						break;
					}
				}
				delt_room(clnt_sd, atoi(numbuf));	
			}else{
				write(clnt_sd, "/ help", sizeof("/ help"));
			}	
		}else{ // if message not contains command
			int j = 0;
			for(int i = 0; i < strlen; i++){
				if(strcmp(buf[i], " ")==0){
					j = i + 1;
					for(j, int k = 0; j < str_len; j++, k++){
						buftemp[k] = buf[j];
					}
					break;
				}
			}
			mess_room(clnt_sd, buftemp);		
		}

	}
	
	close(clnt_sd);
	// delete array
		
}
       	
void exit_room(int sd){
	
	for(int i = 0 ; i < MAX_USER_NUM ; i ++){
		if(user_info[i].sd == sd){
			user_info[i].room = 0;
			break;
		}
	}

}
void make_room(int sd, char* title){
	int room_num = get_blank_room();
	room_info[room_num].title = title;
	room_info[room_num].isuse = 1;	

}

void show_room(int sd){
	 
	for(int i = 0 ; i < MAX_ROOM_NUM; i++){
		if(room_info[i].isuse == 1){	
			write(sd, room_info[i].title, sizeof(room_info[i].title));
		}
	}	

}

void mess_user(int sd, char* nickname, char* message){
	char sender[50];
	char *msg;
		
	for(int i = 0; i < MAX_USER_NUM; i++){
		if(user_info[i].sd == sd){
			sender = user_info[i].nickname;
			break;
		}
	}

	for(int i = 0; i < MAX_USER_NUM; i++){
		if((strcmp(user_info[i].nickname, nickname))==0){
			strcpy(msg, strcat(sender, "로부터의 귓속말:"));
			strcpy(msg, strcat(msg, message));
		        write(user_info[i].sd, msg, sizeof(msg));
			break;
		}
	}

}

void join_room(int sd, int roomnum){
	
	if(room_info[roomnum].isuse == 1){
		for(int i = 0; i < MAX_USER_NUM; i++){
			if(user_info[i].sd == sd){
				user_info[i].room = roomnum;
				break;
			}
		}
	}

}


void delt_room(int sd, int roomnum){

	room_info[roomnum].isuse = 0;
	for(int i = 0; i < MAX_USER_NUM; i++){
		if(user_info[i].room == roomnum){
			user_info[i].room = 0;
			break;
		}
	}


}


int mess_room(int sd, char* message){
	//위와 동! 
	char* msg;
	int roomnum;
	for(int i = 0; i < MAX_USER_NUM; i++){
		if(user_info[i].sd == sd){
			strcpy(msg, strcat(user_info[i].nickname, ": "));
			strcpy(msg, strcat(msg, message));
			roomnum = user_info[i].room;
			break;
		}
	}
	for(int i = 0; i < MAX_USER_NUM; i++){
		if(user_info[i].room == roomnum){
			write(user_info[i].sd, msg, sizeof(msg));
		}
	}

}

void alert_msg_to_user(int sd, char* message){

}

void error_handler(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int get_blank_room(){
	int un_used_num;
	for(int i = 0; i < MAX_ROOM_NUM; i++){
		if(room_info[i].isuse == 0){
			un_used_num = i;
			break;
		}
	}
	return un_used_num;
}

int get_blank_user(){
	int un_used_num;	
	for(int i = 0; i < MAX_USER_NUM; i ++){
		if(user_info[i].isuse == 0){ // #####################
			un_used_num = i;
			break;
		}		
	}
	return un_used_num;
}

