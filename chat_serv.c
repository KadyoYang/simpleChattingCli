#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_ROOM_NUM 10
#define MAX_USER_NUM 40
#define BUF_SIZE 100



// 유저정보담는 구조체 선언
struct sd_nickname_t {
	int sd;  
	char nickname[30]; 
	int room; 
	int isuse; 
};

// 방정보 담는 구조체 선언
struct room_t {
	char title[30];
	int isuse;
};

// 무식한 디버그용 로그 함수 ㅎㅎ
void log(char* message);

// 클라이언트 에서는 먼저 ./chat_clnt 127.0.0.1 9849 id 식으로 접속을 해야함 
// 클라이언트 명령통신용 쓰레드
void* clnt_thread_main(void* arg);

//에러 핸들링
void error_handler(char* message);

// 클라이언트 명령 함수 선언은 여기다가 했고 
// 정의는 밑에다가 했음 
void exit_room(int sd);
void make_room(int sd, char* title);
void show_room(int sd);
void mess_user(int sd, char* nickname, char* message);
void join_room(int sd, int roomnum);
void delt_room(int sd, int roomnum);
void mess_room(int sd, char* message);
void user_list(int sd);
void kick_user(int sd, char* nickname);

// user_info 배열 room_info 배열의 비어있는 공간 찾아서 리턴하는 함수들
// 여기에 세마포어 걸어놓으면 동시에 여러 쓰레드가 접근함으로 인해서 생기는 대부분의 문제가 해결될듯해서 세마포어 걸어놓음
int get_blank_room();
int get_blank_user();

// 문제의 세마포어 변수 선언
sem_t room_sem;
sem_t user_sem;

// 유저정보 방정보 담는 배열 선언 
struct sd_nickname_t user_info[MAX_USER_NUM];
struct room_t room_info[MAX_ROOM_NUM];

//############################################ 메인 함수 ######################################################
// 메인 함수
int main(int argc, char* argv[]) {
	int serv_sd, clnt_sd; // 서버 클라이어느 소켓 디스크립터 변수 선언
	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t clnt_addr_size; 
	struct sd_nickname_t sd_nickname; // 전역 room_info배열에 넣을 정보가 들어갈 구조체변수
	char nickname[BUF_SIZE];
	char buf[BUF_SIZE];
	int str_len = 0;
	pthread_t thread_id;
	int arraytemp;

	// 세마포어 초기화 단일 프로세스에서 여러 쓰레드가 돌면서 공유할것이므로 0 설정 세마포어 키 초기값은 1설정 
	if ((sem_init(&room_sem, 0, 1) != 0 ) || (sem_init(&user_sem, 0, 1) != 0) ) 
		error_handler("sem_init() error!");

	// 전역 배열 초기화
	memset(user_info, 0, sizeof(user_info));
	memset(room_info, 0, sizeof(room_info));
	// 혹시 몰라서 무식하게 초기화
	for (int i = 0; i < MAX_USER_NUM; i++) {
		user_info[i].isuse = 0;
		user_info[i].room = 0;
		user_info[i].sd = 2;
	}
	for (int i = 0; i < MAX_ROOM_NUM; i++) {
		room_info[i].isuse = 0;
	}
	// 대기실 생성
	room_info[0].isuse = 1;
	strcpy(room_info[0].title, "대기실\n");


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

	// 여기까지 보통의 소켓 생성후 바인드 리슨


	// 메인쓰레드는 계속 이것만 반복하면서 클라이언트 받고 유저한계치가 도달하면 접속 거절한다.
	while (1) {
		log("mainthread while");
		clnt_addr_size = sizeof(clnt_addr);
		if ((clnt_sd = accept(serv_sd, (struct sockaddr*) & clnt_addr, &clnt_addr_size)) == -1) {
			close(clnt_sd);
			continue;
		}
		str_len = read(clnt_sd, buf, BUF_SIZE);
		memset(&sd_nickname, 0, sizeof(sd_nickname));
		sd_nickname.sd = clnt_sd;
		strcpy(sd_nickname.nickname, buf);
		sd_nickname.room = 0;
		sd_nickname.isuse = 1;
		arraytemp = get_blank_user();
		if (arraytemp == -1) {
			write(clnt_sd, "SYSTEM : 접속 유저를 더 이상 만들 수 없습니다.\n", strlen("SYSTEM : 접속 유저를 더 이상 만들 수 없습니다.\n"));
			close(clnt_sd);
			continue;
		}
		sem_wait(&user_sem);
		user_info[arraytemp] = sd_nickname;
		printf("sd = %d \n", user_info[0].sd);
		printf("isuse = %d \n", user_info[0].isuse);
		sem_post(&user_sem);
		// 쓰레드 생성 실패시 클라이언트 소켓 클로즈 후 컨티뉴
		if ((pthread_create(&thread_id, NULL, clnt_thread_main, (void*)& arraytemp)) != 0) {
			close(clnt_sd);
			continue;
		}
		pthread_detach(thread_id);
		printf("new client is connected %s %s \n", buf, inet_ntoa(clnt_addr.sin_addr));

	}

	sem_destroy(&user_sem);
	sem_destroy(&room_sem);
	close(serv_sd);
	return 0;
}

// ############################################# 클라이언트 쓰레드 메인함수#############################
// 클라이언트 통신용 쓰레드 
// arg 매개변수로 몇번째 room_user[] 배열 공간 쓰는지 받아온다
void* clnt_thread_main(void* arg) {
	int arraynum = *((int*)arg);
	// 각종 변수 선언 및 초기화 
	int clnt_sd = user_info[arraynum].sd;
	int str_len, roomtemp;
	char buf[BUF_SIZE] = { '\0', };
	char buftemp[BUF_SIZE] = { '\0', };
	char bufnick[BUF_SIZE] = { '\0', };
	char command[10] = { '\0', };
	char numbuf[10] = { '\0', };
	char helpmsg[BUF_SIZE] = { '\0', };


	log("clnt_thread_main init");

	// 클라이언트 통신용 쓰레드는 이것을 계속 반복한다.
	// 주된 일은 문자열을 분석해서 명령 구문인지 아니면 그냥 메시지인지
	// 알아낸 다음 알맞는 클라이언트 명령 함수 호출
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
			printf("strlen(command) = %ld", strlen(command));
			if ((strncmp(command, "/exit", 5)) == 0 && strlen(command) == 6) {

				exit_room(clnt_sd);
			}
			else if ((strncmp(command, "/make", 5)) == 0 && strlen(command) == 6) {
				
				int j = 0;
				int k;
				for (int i = 0; i < str_len; i++) {
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
			else if ((strncmp(command, "/ls", 3)) == 0 && strlen(command) == 4) {
				show_room(clnt_sd);

			}
			else if ((strncmp(command, "/w", 2)) == 0 && strlen(command) == 3) {
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
				log(bufnick);
				log(buftemp);
				mess_user(clnt_sd, bufnick, buftemp);

			}
			else if ((strncmp(command, "/join", 5)) == 0 && strlen(command) == 6) {
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
			else if ((strncmp(command, "/delete", 7)) == 0 && strlen(command) == 8) {
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
			else if ((strncmp(command, "/userlist", 9 )) == 0 && strlen(command) == 10) {
				user_list(clnt_sd);
			}
			else if ((strncmp(command, "/kick", 5)) == 0 && strlen(command) == 6) {

				int j = 0;
				int k;
				for (int i = 0; i < str_len; i++) {
					if (buf[i] == ' ') {
						j = i + 1;
						for (j, k = 0; j < str_len; j++, k++) {
							buftemp[k] = buf[j];
						}
						break;
					}
				}

				kick_user(clnt_sd, buftemp);
			}
			else {
				strcpy(helpmsg,"---명령어---\n");
				write(clnt_sd, helpmsg, strlen(helpmsg));
				strcpy(helpmsg, "/help - 도움말\n");
				write(clnt_sd, helpmsg, strlen(helpmsg));
				strcpy(helpmsg, "/exit - 방에서 나가기\n");
				write(clnt_sd, helpmsg, strlen(helpmsg));
				strcpy(helpmsg, "/ls - 방 목록 보기\n");
				write(clnt_sd, helpmsg, strlen(helpmsg));
				strcpy(helpmsg, "/make <title> - 방 만들기\n");
				write(clnt_sd, helpmsg, strlen(helpmsg));
				strcpy(helpmsg, "/w <nickname> <message> - 귓속말하기\n");
				write(clnt_sd, helpmsg, strlen(helpmsg));
				strcpy(helpmsg, "/join <room number> - 방 들어가기\n");
				write(clnt_sd, helpmsg, strlen(helpmsg));
				strcpy(helpmsg, "/delete <room number> - 방 없애기\n");
				write(clnt_sd, helpmsg, strlen(helpmsg));
				strcpy(helpmsg, "/userlist - 유저 목록 보기\n");
				write(clnt_sd, helpmsg, strlen(helpmsg));
				strcpy(helpmsg, "/kick <nickname> - 해당 닉네임 강퇴하기\n");
				write(clnt_sd, helpmsg, strlen(helpmsg));
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
		// 다 사용한 변수들은 다시 널로 초기화/ char배열이라서 널 넣었는데 괜찮은건가요? ㅎㅎ
	}
	// 클라이언트가 접속종료하거나 꺼버리면 정보 삭제 후 클로즈 
	sem_wait(&user_sem);
	memset(&user_info[arraynum], 0, sizeof(user_info[arraynum]));
	user_info[arraynum].sd = 2;
	sem_post(&user_sem);
	close(clnt_sd);
	// delete elements of user_info[]
	
}




// ############################## 클라이언트 명령 함수 ############################

void exit_room(int sd) {
	log("exit room start");
	int roomnum;
	char msg[BUF_SIZE] = { '\0', };

	strcpy(msg, "SYSTEM : ");
	char* alert = "SYSTEM : 대기실로 나오셨습니다.\n";
	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].sd == sd) {
			roomnum = user_info[i].room;
			user_info[i].room = 0;
			write(sd, alert, strlen(alert));

			strcpy(msg, strcat(msg, user_info[i].nickname));
			strcpy(msg, strcat(msg, "님이 퇴장했습니다.\n"));
			break;
		}
	}
	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].room == roomnum) {
			write(user_info[i].sd, msg, strlen(msg));
		}
	}
	log("exit room end");

}

void make_room(int sd, char* title) {
	log("make room start");

	int room_num = get_blank_room();
	if (room_num == -1) {
		write(sd, "SYSTEM : 방을 더 이상 만들 수 없습니다.\n", strlen("SYSTEM : 방을 더 이상 만들 수 없습니다.\n"));
	}
	else {
		printf("%d is blank room \n", room_num);
		printf("%s is room name \n", title);
		strcpy(room_info[room_num].title, title);
		room_info[room_num].isuse = 1;

		join_room(sd, room_num);
	}
	log("make room end");
}

void show_room(int sd) {
	log("show room start");
	char msg[BUF_SIZE] = {'\0', };
	char num[BUF_SIZE] = {'\0', };

	write(sd, "SYSTEM : 방 목록\n", strlen("SYSTEM : 방 목록\n"));
	for (int i = 0; i < MAX_ROOM_NUM; i++) {
		if (room_info[i].isuse == 1) {
			sprintf(num, "%d", i);
			strcpy(msg, strcat(msg, num));
			strcpy(msg, strcat(msg, "_번방_방제목 : "));
			strcpy(msg, strcat(msg, room_info[i].title));
			write(sd, msg, strlen(msg));
			memset(msg, 0, sizeof(msg));
			memset(num, 0, sizeof(num));
			printf("room name %s \n", room_info[i].title);
		}
	}
	log("show room end");

}

void mess_user(int sd, char* nickname, char* message) {
	log("mess user start");
	char sender[BUF_SIZE] = {'\0', };
	char msg[BUF_SIZE] ={'\0', };
	char to[BUF_SIZE] = {'\0', };
	strcpy(to, nickname);
	
	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].sd == sd) {
			strcpy(sender, user_info[i].nickname);
			break;
		}
	}
	log(nickname);
	printf("%ld \n", strlen(nickname));
	for (int i = 0; i < MAX_USER_NUM; i++) {

		printf("%ld \n", strlen(user_info[i].nickname));
		log(user_info[i].nickname);
		if ((strncmp(user_info[i].nickname, to, strlen(to)-1)) == 0 && (strlen(user_info[i].nickname) == strlen(to)-1)) {
			log("mess id found!!");
			strcpy(msg, strcat(sender, "님 으로부터의 귓속말 : "));
			strcpy(msg, strcat(msg, message));
			log(msg);
			write(user_info[i].sd, msg, strlen(msg));
			break;
		}
	}
	log("mess user end");

}

void join_room(int sd, int roomnum) {
	log("join room start");
	char tempnum[BUF_SIZE] = {'\0', };
	char alert[BUF_SIZE] = {'\0', };
	char bdcast[BUF_SIZE] = { '\0', };
	sprintf(tempnum, "%d", roomnum);

	strcpy(alert, "SYSTEM : 다음 방에 입장하셨습니다. ::");
	strcpy(bdcast, "SYSTEM : ");

	if (room_info[roomnum].isuse == 1) {
		for (int i = 0; i < MAX_USER_NUM; i++) {
			if (user_info[i].sd == sd) {
				strcpy(bdcast, strcat(bdcast, user_info[i].nickname));
				strcpy(bdcast, strcat(bdcast, "님이 방에 입장하셨습니다.\n"));
				strcpy(tempnum, strcat(tempnum, "_번방_방제목 : "));
				strcpy(tempnum, strcat(tempnum, room_info[roomnum].title));
				strcpy(alert, strcat(alert, tempnum));
				user_info[i].room = roomnum;
				write(sd, alert, strlen(alert));
				break;
			}
		}
		for (int i = 0; i < MAX_USER_NUM; i++) {
			if (user_info[i].sd != sd && user_info[i].room == roomnum && user_info[i].isuse == 1) {
				write(user_info[i].sd, bdcast, strlen(bdcast));
			}
		}
	}
	else {
		write(sd, "SYSTEM : 해당 번호의 방이 없습니다.\n", strlen("SYSTEM : 해당 번호의 방이 없습니다.\n"));
	}
	log("join room end");

}


void delt_room(int sd, int roomnum) {
	log("delt room start");
	room_info[roomnum].isuse = 0;
	char alert[BUF_SIZE] = "SYSTEM : 계시던 방이 닫혀서 대기실로 이동합니다.\n";

	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].room == roomnum && user_info[i].isuse == 1) {
			user_info[i].room = 0;
			write(user_info[i].sd, alert, strlen(alert));
			
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

void user_list(int sd) {

	char numbuf[20] = { '\0', };
	char msg[BUF_SIZE] = { '\0', };
	int num = 0;
	
	write(sd, "현재 유저 목록\n", strlen("현재 유저 목록\n"));

	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].isuse == 1) {
			sprintf(numbuf, "%d", i);
			strcpy(msg, numbuf);
			strcpy(msg, strcat(msg, "번 닉네임:"));
			strcpy(msg, strcat(msg, user_info[i].nickname));
			strcpy(msg, strcat(msg, " 방번호:"));
			memset(numbuf, '\0', sizeof(numbuf));
			sprintf(numbuf, "%d", user_info[i].room);
			strcpy(msg, strcat(msg, numbuf));
			strcpy(msg, strcat(msg, " 방제목:"));
			num = user_info[i].room;
			strcpy(msg, strcat(msg, room_info[num].title));
			
			write(sd, msg, strlen(msg));

			memset(numbuf, '\0', sizeof(numbuf));
			memset(msg, '\0', sizeof(msg));
			num = 0;
		}
	}
}

void kick_user(int sd, char* nickname) {
	log("kick user start");
	char sender[BUF_SIZE] = { '\0', };
	char msg[BUF_SIZE] = { '\0', };
	

	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].sd == sd) {
			strcpy(sender, user_info[i].nickname);
			break;
		}
	}
	
	
	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (strncmp(user_info[i].nickname, nickname, strlen(user_info[i].nickname)) == 0) {
			strcpy(msg, strcat(msg, sender));
			strcpy(msg, strcat(msg, "님이 당신을 추방했습니다.\n"));
			user_info[i].room = 0;
			write(user_info[i].sd, msg, strlen(msg));
			break;
		}
	}

	log("kick user end");

}



//  클라이언트 명령함수 끝 



// 각종 함수들 정의 시작 

void error_handler(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int get_blank_room() {
	log("get blank room start");
	sem_wait(&room_sem);
	int un_used_num = -1;
	for (int i = 0; i < MAX_ROOM_NUM; i++) {
		if (room_info[i].isuse == 0) {
			un_used_num = i;
			break;
		}
	}
	sem_post(&room_sem);
	log("get blank room end");
	return un_used_num;
}

int get_blank_user() {
	log("get blank user start");
	sem_wait(&user_sem);
	int un_used_num = -1;
	for (int i = 0; i < MAX_USER_NUM; i++) {
		if (user_info[i].isuse == 0) { // #####################
			un_used_num = i;
			break;
		}
	}
	sem_post(&user_sem);
	log("get blank user end");
	return un_used_num;
}


// 로그 함수 
void log(char* message) {
	fputs(message, stdout);
	fputc('\n', stdout);
}
