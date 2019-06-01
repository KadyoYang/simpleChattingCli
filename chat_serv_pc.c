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


struct sd_nickname_t {
	int sd;
	char nickname[30];
	int room; // 0 is room list i > 0 is specific room
	int isuse; // 0 false 1 true
};

struct room_t {
	char title[30];
	int isuse;
};

void log(char* message);

// 클라이언트 에서는 먼저 ./chat_clnt 127.0.0.1 9849 id 식으로 접속을 해야함 
// 그 정보가 내가 손수만든 구조체에 들어가서 포인터로 날아간다
void* clnt_thread_main(void* arg);
void error_handler(char* message);


void exit_room(int sd);
void make_room(int sd, char* title);
void show_room(int sd);
void mess_user(int sd, char* nickname, char* message);
void join_room(int sd, int roomnum);
void delt_room(int sd, int roomnum);
void mess_room(int sd, char* message);
void alert_msg_to_user(int sd, char* message);

int get_blank_room();
int get_blank_user();

struct sd_nickname_t user_info[MAX_USER_NUM];
struct room_t room_info[MAX_ROOM_NUM];



int main(int argc, char* argv[]) {
	int serv_sd, clnt_sd; //서버 클라이어느 소켓 디스크립터 변수 선언
	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t clnt_addr_size;
	struct sd_nickname_t sd_nickname;
	char nickname[BUF_SIZE];
	char buf[BUF_SIZE];
	int strlen = 0;
	pthread_t thread_id;
	int arraytemp;


	memset(user_info, 0, sizeof(user_info));
	memset(room_info, 0, sizeof(room_info));
	for (int i = 0; i < MAX_USER_NUM; i++) {
		user_info[i].isuse = 0;
		user_info[i].room = 0;
	}
	for (int i = 0; i < MAX_ROOM_NUM; i++) {
		room_info[i].isuse = 0;
	}


	// 그 위에 배열 isuse 0 으로 다 초기화 해야한다 	
	if (argc != 2) {
		printf("Usage : %s <port> \n", argv[0]);
		exit(1);
	}
	serv_sd = socket(PF_INET, SOCK_STREAM, 0);
	if (serv_sd == -1)
		error_handler("socket() error");
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if ((bind(serv_sd, (struct sockaddr*) & serv_addr, sizeof(serv_addr))) == -1)
		error_handler("bind() error");
	if ((listen(serv_sd, 5)) == -1)
		error_handler("listen() error");

	while (1) {
		log("mainthread while");
		clnt_addr_size = sizeof(clnt_addr);
		if ((clnt_sd = accept(serv_sd, (struct sockaddr*) & clnt_addr, &clnt_addr_size)) == -1) {
			close(clnt_sd);
			continue;
		}
		strlen = read(clnt_sd, buf, BUF_SIZE);
		memset(&sd_nickname, 0, sizeof(sd_nickname));
		sd_nickname.sd = clnt_sd;
		strcpy(sd_nickname.nickname, buf);
		sd_nickname.room = 0;
		sd_nickname.isuse = 1;
		arraytemp = get_blank_user();
		user_info[arraytemp] = sd_nickname; // ##########임게영역 
		printf("sd = %d \n", user_info[0].sd);
		printf("isuse = %d \n", user_info[0].isuse);
		if ((pthread_create(&thread_id, NULL, clnt_thread_main, (void*)& arraytemp)) != 0) {
			close(clnt_sd);
			continue;
		}
		pthread_detach(thread_id);
		printf("new client is connected %s %s \n", buf, inet_ntoa(clnt_addr.sin_addr));

	}
	close(serv_sd);
	return 0;
}



void* clnt_thread_main(void* arg) {//arg로 그냥 몇번째 유저인지 번호만 알려주고 그 번호로 전역구조체 참조해서use
	int arraynum = *((int*)arg);
	// 흠.. 
	int clnt_sd = user_info[arraynum].sd; // ############### im gae
	int str_len, roomtemp;
	char buf[BUF_SIZE] = { '\0', };
	char buftemp[BUF_SIZE] = { '\0', };
	char bufnick[BUF_SIZE] = { '\0', };
	char command[10] = { '\0', };
	char numbuf[10] = { '\0', };

	log("clnt_thread_main init");

	while (str_len = read(clnt_sd, buf, sizeof(buf))) {
		buf[str_len] = '\0';
		log("clnt while() 1 ");

		log(buf);

		if (buf[0] == '/') { // if message contains command
			log("clnt 131");
			for (int i = 0; i < str_len + 1; i++) {
				if (buf[i] == ' ' || buf[i] == '\0') {
					for (int j = 0; j < i + 1; j++) {
						command[j] = buf[j];
					}

					log("clnt 138");
					break;
				}

			}
			log("command is ");
			log(command);
			log("clnt 143");
			if ((strncmp(command, "/exit", 5)) == 0) {

				exit_room(clnt_sd);
			}
			else if ((strncmp(command, "/make", 5)) == 0) {
				char* title;
				int j = 0;
				int k;
				for (int i = 0; i, str_len; i++) {
					if (buf[i] == ' ') {
						j = i + 1;
						for (j, k = 0; j < str_len; j++, k++) {
							buftemp[k] = buf[j];
						}
						break;
					}
				}
				make_room(clnt_sd, buftemp);

			}
			else if ((strncmp(command, "/ls", 3)) == 0) {
				show_room(clnt_sd);

			}
			else if ((strncmp(command, "/w", 2)) == 0) {
				int j = 0;
				int n = 0;
				int u;
				int k;
				for (int i = 0; i < str_len; i++) {
					if (buf[i] == ' ') {
						j = i + 1;
						for (j, k = 0; j < str_len; j++, k++) {
							bufnick[k] = buf[j];
							if (buf[j] == ' ') {
								n = j + 1;
								for (n, u = 0; n < str_len; n++, u++) {
									buftemp[u] = buf[n];
								}
								break;
							}
						}
						break;
					}
				}
				mess_user(clnt_sd, bufnick, buftemp);

			}
			else if ((strncmp(command, "/join", 5)) == 0) {
				int j = 0;
				int k;
				for (int i = 0; i < str_len; i++) {
					if (buf[i] == ' ') {
						j = i + 1;
						for (j, k = 0; j < str_len; j++, k++) {
							numbuf[k] = buf[j];
						}
						break;
					}
				}
				join_room(clnt_sd, atoi(numbuf));

			}
			else if ((strncmp(command, "/delete", 7)) == 0) {
				int j = 0;
				int k;
				for (int i = 0; i < str_len; i++) {
					if (buf[i] == ' ') {
						j = i + 1;
						for (j, k = 0; j < str_len; j++, k++) {
							numbuf[k] = buf[j];
						}
						break;
					}
				}
				delt_room(clnt_sd, atoi(numbuf));
			}
			else {
				write(clnt_sd, "/ help", sizeof("/ help"));
			}
		}
		else { // if message not contains command
			log("clnt 219");
			mess_room(clnt_sd, buf);
		}
		memset(buf, '\0', sizeof(buf));
		memset(buftemp, '\0', sizeof(buftemp));
		memset(bufnick, '\0', sizeof(bufnick));
		memset(command, '\0', sizeof(command));
		memset(numbuf, '\0', sizeof(numbuf));

	}

	memset(&user_info[arraynum], 0, sizeof(user_info[arraynum]));
	close(clnt_sd);
	// delete array
	
}

void exit_room(int sd) {
	log("exit room start");
	char* alert = "SYSTEM : 대기실로 나오셨습니다.";
	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].sd == sd) {
			user_info[i].room = 0;
			write(sd, alert, strlen(alert));
			break;
		}
	}
	log("exit room end");

}
void make_room(int sd, char* title) {
	log("make room start");

	int room_num = get_blank_room();
	printf("%d is blank room \n", room_num);
	printf("%s is room name \n", title);
	strcpy(room_info[room_num].title, title);
	room_info[room_num].isuse = 1;

	join_room(sd, room_num);
	log("make room end");
}

void show_room(int sd) {
	log("show room start");
	char* msg = "_";
	write(sd, "SYSTEM : 방 목록", strlen("SYSTEM : 방 목록"));
	for (int i = 0; i < MAX_ROOM_NUM; i++) {
		if (room_info[i].isuse == 1) {
			strcpy(msg, strcat(msg, atoi(i)));
			strcpy(msg, strcat(msg, "_번방_방제목 : "));
			strcpy(msg, strcat(msg, room_info[i].title));
			write(sd, msg, strlen(msg));
			printf("room name %s \n", room_info[i].title);
		}
	}
	log("show room end");

}

void mess_user(int sd, char* nickname, char* message) {
	log("mess user start");
	char sender[50];
	char* msg;

	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].sd == sd) {
			strcpy(sender, user_info[i].nickname);
			break;
		}
	}

	for (int i = 0; i < MAX_USER_NUM; i++) {
		if ((strcmp(user_info[i].nickname, nickname)) == 0) {
			strcpy(msg, strcat(sender, "님 으로부터의 귓속말 : "));
			strcpy(msg, strcat(msg, message));
			write(user_info[i].sd, msg, strlen(msg));
			break;
		}
	}
	log("mess user end");

}

void join_room(int sd, int roomnum) {
	log("join room start");
	char* tempnum = atoi(roomnum);
	char* alert = "SYSTEM : 다음 방에 입장하셨습니다. ::";
	
	if (room_info[roomnum].isuse == 1) {
		for (int i = 0; i < MAX_USER_NUM; i++) {
			if (user_info[i].sd == sd) {
				strcpy(tempnum, strcat(tempnum, "_번방_방제목 : "));
				strcpy(tempnum, strcat(tempnum, room_info[i].title));
				strcpy(alert, strcat(alert, tempnum));
				user_info[i].room = roomnum;
				write(sd, alert, strlen(alert));
				break;
			}
		}
	}
	log("join room end");

}


void delt_room(int sd, int roomnum) {
	log("delt room start");
	room_info[roomnum].isuse = 0;
	char* alert = "SYSTEM : 계시던 방이 닫혀서 대기실로 이동합니다.";

	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].room == roomnum) {
			user_info[i].room = 0;
			write(user_info[i].sd, alert, strlen(alert));
			break;
		}
	}
	log("delt room end");


}


void mess_room(int sd, char* message) {
	log("mess room start");
	//위와 동! 
	char msg[BUF_SIZE] = { '\0', };
	char nickname[BUF_SIZE] = { '\0', };
	int roomnum;
	for (int i = 0; i < MAX_USER_NUM; i++) {
		log("mess room 1");
		if (user_info[i].sd == sd) {
			strcpy(nickname, user_info[i].nickname);
			log("mess room 2");
			strcpy(msg, strcat(nickname, ": "));
			log("mess room 2.5");
			strcpy(msg, strcat(msg, message));
			log("mess room 3.0");
			roomnum = user_info[i].room;
			printf("msg is %s \n", msg);
			break;
		}

	}
	log("mess room 3");
	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].room == roomnum && user_info[i].isuse == 1) {
			if (user_info[i].sd == sd) {
				continue;
			}
			write(user_info[i].sd, msg, sizeof(msg));
		}
	}
	log("mess room end");

}

void alert_msg_to_user(int sd, char* message) {

}

void error_handler(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int get_blank_room() {
	log("get blank room start");
	int un_used_num;
	for (int i = 0; i < MAX_ROOM_NUM; i++) {
		if (room_info[i].isuse == 0) {
			un_used_num = i;
			break;
		}
	}
	log("get blank room end");
	return un_used_num;
}

int get_blank_user() {
	log("get blank user start");
	int un_used_num;
	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].isuse == 0) { // #####################
			un_used_num = i;
			break;
		}
	}
	log("get blank user end");
	return un_used_num;
}

void log(char* message) {
	fputs(message, stdout);
	fputc('\n', stdout);
}
