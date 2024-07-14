#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "status.h"
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include<errno.h>
int main (int argc, char *argv[])
{
	char par_battle[30][5] = {"G", "G", "H", "H", "I", "I", "J", "J", "M", "M", "K", "N", "N", "L", "C"};
	pid_t pid = getpid();
	int player_id = atoi(argv[1]), log_fd;
	//fprintf(stderr, "%d: %d\n", player_id, pid);
	if(player_id >= 0 && player_id <= 7){
		char filename[100] = {};
		sprintf(filename, "log_player%s.txt", argv[1]);
		log_fd = open(filename, O_CREAT | O_RDWR, 0777);
	}
	Status cur_p;
	if(player_id >= 0 && player_id <= 7){
		//int play_id = atoi(argv[1]);
		FILE *fp = fopen("./player_status.txt", "r");
		Status player_stat[8];
		char attr[30][10] = {};
		for(int i = 0; i < 8; i++){
			fscanf(fp, "%d %d %s %c %d", &player_stat[i].HP, &player_stat[i].ATK, attr[i], &player_stat[i].current_battle_id, &player_stat[i].battle_ended_flag);
		}
		for(int i = 0; i < 8; i++){
			if(strcmp(attr[i], "FIRE") == 0) player_stat[i].attr = FIRE;
			if(strcmp(attr[i], "GRASS") == 0) player_stat[i].attr = GRASS;
			if(strcmp(attr[i], "WATER") == 0) player_stat[i].attr = WATER;
		}
		cur_p = player_stat[player_id];
		cur_p.real_player_id = player_id;
	}
	else{
		//fprintf(stderr, "%d\n", player_id);
		char fifo_name[100] = {};
		sprintf(fifo_name, "player%s.fifo", argv[1]);
		mkfifo(fifo_name, 0755);
		int fifo_fd = open(fifo_name, O_RDONLY);
		read(fifo_fd, &cur_p, sizeof(Status));
		char filename[100] = {};
		sprintf(filename, "log_player%d.txt", cur_p.real_player_id);
		//fprintf(stderr, "player %d: %s\n",player_id, filename);
		log_fd = open(filename, O_APPEND | O_RDWR);
		char from_fifo[100] = {};
		sprintf(from_fifo, "%d,%d fifo from %d %d,%d\n", player_id, pid, cur_p.real_player_id, cur_p.real_player_id, cur_p.HP);
		write(log_fd, &from_fifo, strlen(from_fifo));
	}
	char to_b[100] = {};
	sprintf(to_b, "%d,%d pipe to %s,%s %d,%d,%c,%d\n", player_id, pid, par_battle[player_id], argv[2], cur_p.real_player_id, cur_p.HP, cur_p.current_battle_id, cur_p.battle_ended_flag);
	write(log_fd, &to_b, strlen(to_b));
	write(STDOUT_FILENO, &cur_p, sizeof(Status));
	int ori_hp = cur_p.HP;
	while(read(STDIN_FILENO, &cur_p, sizeof(Status))){
		char from_b[100] = {};
		sprintf(from_b, "%d,%d pipe from %s,%s %d,%d,%c,%d\n", player_id, pid, par_battle[player_id], argv[2], cur_p.real_player_id, cur_p.HP, cur_p.current_battle_id, cur_p.battle_ended_flag);
		write(log_fd, &from_b, strlen(from_b));

		if(cur_p.battle_ended_flag == 0){
			char to_b[100] = {};
			sprintf(to_b, "%d,%d pipe to %s,%s %d,%d,%c,%d\n", player_id, pid, par_battle[player_id], argv[2], cur_p.real_player_id, cur_p.HP, cur_p.current_battle_id, cur_p.battle_ended_flag);
			write(log_fd, &to_b, strlen(to_b));
			write(STDOUT_FILENO, &cur_p, sizeof(Status));
		}
		else{
			cur_p.battle_ended_flag = 0;
			if(cur_p.HP > 0){  // winner
				if(cur_p.current_battle_id == 'A'){
					exit(0);
				}
				cur_p.HP += (ori_hp - cur_p.HP) / 2;
				char to_b[100] = {};
				sprintf(to_b, "%d,%d pipe to %s,%s %d,%d,%c,%d\n", player_id, pid, par_battle[player_id], argv[2], cur_p.real_player_id, cur_p.HP, cur_p.current_battle_id, cur_p.battle_ended_flag);
				write(log_fd, &to_b, strlen(to_b));
				write(STDOUT_FILENO, &cur_p, sizeof(Status));
			}
			else{  // loser
				int B;
				if(cur_p.current_battle_id == 'G') B = 8;
				else if(cur_p.current_battle_id == 'I') B = 9;
				else if(cur_p.current_battle_id == 'D') B = 10;
				else if(cur_p.current_battle_id == 'H') B = 11;
				else if(cur_p.current_battle_id == 'J') B = 12;
				else if(cur_p.current_battle_id == 'E') B = 13;
				else if(cur_p.current_battle_id == 'B') B = 14;
				else exit(0);
				if(player_id < 8){
					cur_p.HP = ori_hp;
					char fifo_name[100] = {};
					//fprintf(stderr, "d: %d\n", player_id);
					sprintf(fifo_name, "player%d.fifo", B);
					mkfifo(fifo_name, 0755);
					int fifo_fd = open(fifo_name, O_WRONLY);
					write(fifo_fd, &cur_p, sizeof(Status));
					char to_fifo[100] = {};
					sprintf(to_fifo, "%d,%d fifo to %d %d,%d\n", player_id, pid, B, cur_p.real_player_id, cur_p.HP);
					write(log_fd, &to_fifo, strlen(to_fifo));
					exit(0);
				}
			}
		}
	}
	return 0;
}