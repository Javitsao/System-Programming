#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "status.h"
#include<fcntl.h>
#include<sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
int main (int argc, char *argv[])
{
	char battle[30][5] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N"};
	char left_child[30][5] = {"B", "D", "F", "G", "I", "K", "0", "2", "4", "6", "M", "N", "8", "11"};
	char right_child[30][5] = {"C", "E", "14", "H", "J", "L", "1", "3", "5", "7", "10", "13", "9", "12"};
	char parent[30][5] = {"0", "A", "A", "B", "B", "C", "D", "D", "E", "E", "F", "F", "K", "L"};
	int pid_l , pid_r, fd[2][2][2], battle_id = argv[1][0] - 'A';
	//printf("%d\n", battle_id);
	Status left_stat, right_stat;
	pid_t pid = getpid();
	//fprintf(stderr, "%c: %d\n", argv[1][0], pid);
	char s_pid[100] = {};
	sprintf(s_pid, "%d", pid);
	pipe(fd[0][0]);
	pipe(fd[0][1]);
	if((pid_l = fork()) > 0){
		close(fd[0][0][0]);
		close(fd[0][1][1]);
	}
	else{
		dup2(fd[0][0][0], STDIN_FILENO);
		dup2(fd[0][1][1], STDOUT_FILENO);
		close(fd[0][0][0]);
		close(fd[0][0][1]);
		close(fd[0][1][0]);
		close(fd[0][1][1]);
		if(strcmp(left_child[battle_id], "A") >= 0 && strcmp(left_child[battle_id], "N") <= 0){
			execl("./battle", "./battle", left_child[battle_id], s_pid, NULL); //pid string?
		}
		else{
			int play_id = atoi(left_child[battle_id]);
			execl("./player", "./player", left_child[battle_id], s_pid, NULL);
		}
	}
	pipe(fd[1][0]);
	pipe(fd[1][1]);
	if((pid_r = fork()) > 0){
		close(fd[1][0][0]);
		close(fd[1][1][1]);
	}
	else{
		dup2(fd[1][0][0], STDIN_FILENO);
		dup2(fd[1][1][1], STDOUT_FILENO);
		close(fd[1][0][0]);
		close(fd[1][0][1]);
		close(fd[1][1][0]);
		close(fd[1][1][1]);
		close(fd[0][0][1]);
		close(fd[0][1][0]);
		if(strcmp(right_child[battle_id], "A") >= 0 && strcmp(right_child[battle_id], "N") <= 0){
			execl("./battle", "./battle", right_child[battle_id], s_pid, NULL);
		}
		else{
			int play_id = atoi(right_child[battle_id]);
			execl("./player", "./player", right_child[battle_id], s_pid, NULL);
		}
	}
	char filename[100] = {};
	sprintf(filename, "log_battle%c.txt", argv[1][0]);
	int log_fd = open(filename, O_CREAT | O_RDWR, 0777);
	int l_play_id = atoi(left_child[battle_id]), r_play_id = atoi(right_child[battle_id]), alive = 0;
	Attribute battle_attr[14] = {FIRE, GRASS, WATER, WATER, FIRE, FIRE, FIRE, GRASS, WATER, GRASS, GRASS, GRASS, FIRE, WATER};

	while(read(fd[0][1][0], &left_stat, sizeof(Status)) && read(fd[1][1][0], &right_stat, sizeof(Status))){
		int left_times = (left_stat.attr == battle_attr[battle_id])? 2: 1, right_times = (right_stat.attr == battle_attr[battle_id])? 2: 1;
		char from_r[100] = {}, from_l[100] = {};
		sprintf(from_l, "%c,%d pipe from %s,%d %d,%d,%c,%d\n", argv[1][0], pid, left_child[battle_id], pid_l, left_stat.real_player_id, left_stat.HP, left_stat.current_battle_id, left_stat.battle_ended_flag);
		sprintf(from_r, "%c,%d pipe from %s,%d %d,%d,%c,%d\n", argv[1][0], pid, right_child[battle_id], pid_r, right_stat.real_player_id, right_stat.HP, right_stat.current_battle_id, right_stat.battle_ended_flag);
		write(log_fd, from_l, strlen(from_l));
		write(log_fd, from_r, strlen(from_r));

		left_stat.current_battle_id = right_stat.current_battle_id = 'A' + battle_id;
		if(left_stat.HP < right_stat.HP){
			right_stat.HP -= left_stat.ATK * left_times;
			if(right_stat.HP <= 0){
				left_stat.battle_ended_flag = right_stat.battle_ended_flag = 1;  // finish
				write(fd[0][0][1], &left_stat, sizeof(Status));
				write(fd[1][0][1], &right_stat, sizeof(Status));
				alive = 0;
				break;
			}
			left_stat.HP -= right_stat.ATK * right_times;
			if(left_stat.HP <= 0){
				left_stat.battle_ended_flag = right_stat.battle_ended_flag = 1;  // finish
				write(fd[0][0][1], &left_stat, sizeof(Status));
				write(fd[1][0][1], &right_stat, sizeof(Status));
				alive = 1;
				break;
			}
		}
		else if(left_stat.HP > right_stat.HP){
			left_stat.HP -= right_stat.ATK * right_times;
			if(left_stat.HP <= 0){
				left_stat.battle_ended_flag = right_stat.battle_ended_flag = 1;  // finish
				write(fd[0][0][1], &left_stat, sizeof(Status));
				write(fd[1][0][1], &right_stat, sizeof(Status));
				alive = 1;
				break;
			}
			right_stat.HP -= left_stat.ATK * left_times;
			if(right_stat.HP <= 0){
				left_stat.battle_ended_flag = right_stat.battle_ended_flag = 1;  // finish
				write(fd[0][0][1], &left_stat, sizeof(Status));
				write(fd[1][0][1], &right_stat, sizeof(Status));
				alive = 0;
				break;
			}
		}
		else{
			if(left_stat.real_player_id < right_stat.real_player_id){
				right_stat.HP -= left_stat.ATK * left_times;
				if(right_stat.HP <= 0){
					left_stat.battle_ended_flag = right_stat.battle_ended_flag = 1;  // finish
					write(fd[0][0][1], &left_stat, sizeof(Status));
					write(fd[1][0][1], &right_stat, sizeof(Status));
					alive = 0;
					break;
				}
				left_stat.HP -= right_stat.ATK * right_times;
				if(left_stat.HP <= 0){
					left_stat.battle_ended_flag = right_stat.battle_ended_flag = 1;  // finish
					write(fd[0][0][1], &left_stat, sizeof(Status));
					write(fd[1][0][1], &right_stat, sizeof(Status));
					alive = 1;
					break;
				}
			}
			else{
				left_stat.HP -= right_stat.ATK * right_times;
				if(left_stat.HP <= 0){
					left_stat.battle_ended_flag = right_stat.battle_ended_flag = 1;  // finish
					write(fd[0][0][1], &left_stat, sizeof(Status));
					write(fd[1][0][1], &right_stat, sizeof(Status));
					alive = 1;
					break;
				}
				right_stat.HP -= left_stat.ATK * left_times;
				if(right_stat.HP <= 0){
					left_stat.battle_ended_flag = right_stat.battle_ended_flag = 1;  // finish
					write(fd[0][0][1], &left_stat, sizeof(Status));
					write(fd[1][0][1], &right_stat, sizeof(Status));
					alive = 0;
					break;
				}
			}
		}
		//write to log
		char to_r[100] = {}, to_l[100] = {};
		sprintf(to_l, "%c,%d pipe to %s,%d %d,%d,%c,%d\n", argv[1][0], pid, left_child[battle_id], pid_l, left_stat.real_player_id, left_stat.HP, left_stat.current_battle_id, left_stat.battle_ended_flag);
		sprintf(to_r, "%c,%d pipe to %s,%d %d,%d,%c,%d\n", argv[1][0], pid, right_child[battle_id], pid_r, right_stat.real_player_id, right_stat.HP, right_stat.current_battle_id, right_stat.battle_ended_flag);
		write(log_fd, to_l, strlen(to_l));
		write(log_fd, to_r, strlen(to_r));
		write(fd[0][0][1], &left_stat, sizeof(Status));
		write(fd[1][0][1], &right_stat, sizeof(Status));
	}
	//write to log
	char to_r[100] = {}, to_l[100] = {};
	sprintf(to_l, "%c,%d pipe to %s,%d %d,%d,%c,%d\n", argv[1][0], pid, left_child[battle_id], pid_l, left_stat.real_player_id, left_stat.HP, left_stat.current_battle_id, left_stat.battle_ended_flag);
	sprintf(to_r, "%c,%d pipe to %s,%d %d,%d,%c,%d\n", argv[1][0], pid, right_child[battle_id], pid_r, right_stat.real_player_id, right_stat.HP, right_stat.current_battle_id, right_stat.battle_ended_flag);
	write(log_fd, to_l, strlen(to_l));
	write(log_fd, to_r, strlen(to_r));
	//fprintf(stderr, "waiting pid: %d\n", (alive == 1)? pid_l: pid_r);
	waitpid((alive == 1)? pid_l: pid_r, NULL, 0);

	if(battle_id == 0){  // if A then print champion
		printf("Champion is P%d\n", (alive == 0)? left_stat.real_player_id: right_stat.real_player_id);
		//fprintf(stderr, "champion waiting pid: %d\n", (alive == 0)? pid_l: pid_r);
		waitpid((alive == 0)? pid_l: pid_r, NULL, 0);
		return 0;
	}
	//passing mode
	Status pass;
	//fprintf(stderr, "%c in passing mode\n", 'A' + battle_id);
	while(1){
		read(fd[alive][1][0], &pass, sizeof(Status));
		char from_alive[100] = {};
		sprintf(from_alive, "%c,%d pipe from %s,%d %d,%d,%c,%d\n", argv[1][0], pid, (alive == 0)? left_child[battle_id]: right_child[battle_id], (alive == 0)? pid_l: pid_r, pass.real_player_id, pass.HP, pass.current_battle_id, pass.battle_ended_flag);
		write(log_fd, from_alive, strlen(from_alive));

		write(STDOUT_FILENO, &pass, sizeof(Status));
		char to_b[100] = {};
		sprintf(to_b, "%c,%d pipe to %s,%s %d,%d,%c,%d\n", argv[1][0], pid, parent[battle_id], argv[2], pass.real_player_id, pass.HP, pass.current_battle_id, pass.battle_ended_flag);
		write(log_fd, to_b, strlen(to_b));
		
		read(STDIN_FILENO, &pass, sizeof(Status));
		char from_b[100] = {};
		sprintf(from_b, "%c,%d pipe from %s,%s %d,%d,%c,%d\n", argv[1][0], pid, parent[battle_id], argv[2], pass.real_player_id, pass.HP, pass.current_battle_id, pass.battle_ended_flag);
		write(log_fd, from_b, strlen(from_b));

		write(fd[alive][0][1], &pass, sizeof(Status));
		char to_alive[100] = {};
		sprintf(to_alive, "%c,%d pipe to %s,%d %d,%d,%c,%d\n", argv[1][0], pid, (alive == 0)? left_child[battle_id]: right_child[battle_id], (alive == 0)? pid_l: pid_r, pass.real_player_id, pass.HP, pass.current_battle_id, pass.battle_ended_flag);
		write(log_fd, to_alive, strlen(to_alive));

		if(pass.HP <= 0 || (pass.battle_ended_flag == 1 && pass.current_battle_id == 'A')){
			waitpid((alive == 0)? pid_l: pid_r, NULL, 0);
			break;
		}
	}
	return 0;

}