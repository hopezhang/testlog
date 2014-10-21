#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>

#include "log_msg.h"

static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

log_list_t *log_lit;
void create_log_llist(void)
{
	int i = 0;
	log_list_t *temp_lit, *head_lit;

	log_lit = malloc(sizeof(log_list_t));
	if(log_lit == NULL){
		printf("creat llist fail\n");
		return;
	}
	log_lit->head = log_lit;
	log_lit->next = log_lit;
	memset(&log_lit->logmg, -1 , sizeof(msgdata_t));

	head_lit = log_lit;
	for(i = 0;i < MESSAGE_SIZE - 1;i++){
		temp_lit = malloc(sizeof(log_list_t));
		temp_lit->next = log_lit;
		temp_lit->head = head_lit;
		head_lit->next = temp_lit;
		head_lit = temp_lit;
		memset(&temp_lit->logmg, -1 , sizeof(msgdata_t));
	}
	log_lit->head = temp_lit;

	return;
}

void fill_log_llist(char level, char* time, char *text)
{
	pthread_mutex_lock(&mut);
	log_lit->logmg.level = level;
	memcpy(log_lit->logmg.dat_time, time, 30);
	memcpy(log_lit->logmg.msg_text, text, TEXT_LENG);
	log_lit = log_lit->next;	
	pthread_mutex_unlock(&mut);
}
void trival_log_llist(void)
{
	log_list_t *cur_lit, *next_lit;
	int count = 1;

	for(cur_lit = log_lit->head; cur_lit != log_lit;cur_lit = next_lit){
		next_lit = cur_lit->head;
		printf("\n%d---->%s\n", cur_lit->logmg.level, cur_lit->logmg.msg_text);
		count++;
	}
	printf("\n%d---->%s\n", log_lit->logmg.level, log_lit->logmg.msg_text);
	printf("msg length -----%d\n", count);
	
	return ;
}


void Log_Message_task(void)
{
	key_t log_key;
	int ret = 0;
	int log_msgid;
	msgdata_t rbuf;

	
	log_key = ftok(LOGKEYPATH, LOGKEYOBG);
	if(log_key < 0){
		printf("log message create key fail.\n");
		return ;
	}

	log_msgid = msgget(log_key, IPC_CREAT|0600);
	if(log_msgid < 0){
		printf("log message create key fail.\n");
		return ;
	}

	do{
		ret = msgrcv(log_msgid, &rbuf, sizeof(rbuf)-sizeof(long), 0, 0);
		if(ret < 0){
			printf("log msgrcv() error\n");
			return ;
		}
		ccprintf("WARNING [%d]---->%s\n", rbuf.level, rbuf.msg_text);
		fill_log_llist(rbuf.level, rbuf.dat_time, rbuf.msg_text);
	}while(1);

	msgctl(log_msgid, IPC_RMID, NULL);	
}


void llist_file(void)
{
	FILE *fp ;
	log_list_t *cur_lit, *next_lit;

	fp = fopen("/root/pbilog", "w+");
	if(fp == NULL){
		printf("create log file error\n");
		return ;
	}

	pthread_mutex_lock(&mut);
	for(cur_lit = log_lit->head; cur_lit != log_lit;cur_lit = next_lit){
		next_lit = cur_lit->head;
		if(cur_lit->logmg.level != -1)
			fprintf(fp, "%s %d: %s\n", cur_lit->logmg.dat_time, cur_lit->logmg.level, cur_lit->logmg.msg_text);
	}
	if(cur_lit->logmg.level != -1)
		fprintf(fp, "%s %d: %s\n", log_lit->logmg.dat_time, log_lit->logmg.level, log_lit->logmg.msg_text);
	pthread_mutex_unlock(&mut);

	fclose(fp);

	return ;
}
void *save_log(void *ptr)
{
	while(1)
	{
		sleep(30);
		llist_file();	
	}
}

void Load_file_llist(void)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char *tik = NULL, *text = NULL;
	int tlevel = 0, count=0;
	log_list_t *load_lit;
	char time_buf[30] = {0};

	fp = fopen("/root/pbilog", "r");
	if(fp == NULL){
		printf("load log file error\n");
		return ;
	}
	load_lit = log_lit->head;

	while ((read = getline(&line, &len, fp)) != -1){
		if(count++ >= MESSAGE_SIZE)
			break;;
		memcpy(time_buf, line, 19);
	//	printf("time %s\n", time_buf);
		tik = strtok(line + 20, ":");
		if(tik == NULL){
			printf("strtok failed\n");
			return ;
		}

		text = strtok(line + 23, "\n");
		if(text == NULL){
			printf("strtok failed\n");
			return ;
		}
	//	printf("[%d]file----%d->%s\n", read, atoi(tik), text);
		pthread_mutex_lock(&mut);
		load_lit->logmg.level = atoi(tik);
		strcpy(load_lit->logmg.msg_text, text);
		strcpy(load_lit->logmg.dat_time, time_buf);
		load_lit= load_lit->head;	
		pthread_mutex_unlock(&mut);
	}

	fclose(fp);
	printf("count %d.\n", count);

	return ;
}

int main(void)
{
	pthread_t tid;

	create_log_llist();
	Load_file_llist();
	pthread_create(&tid, NULL, save_log, NULL);	
	Log_Message_task();

	pthread_join(tid, NULL);
	pthread_mutex_destroy(&mut);

	return 0;
}
