#ifndef _LOG_MSG_H
#define _LOG_MSG_H


#define LOGKEYPATH	"/root/1701PM.bin"
#define LOGKEYOBG	'c'

#define TEXT_LENG	500
#define MESSAGE_SIZE 	1000

#define ccprintf		

typedef struct{
	long mtype;
	char level;
	char dat_time[30];
	char msg_text[TEXT_LENG];
}msgdata_t;

typedef struct log_list{
	msgdata_t logmg;
	struct log_list *next;	
	struct log_list *head;	
}log_list_t;

void Log_Message_task(void);
void Log_Message(char level, char *msg);
void trival_log_llist(void);

#endif
