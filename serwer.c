#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/msg.h>
#include <signal.h>
#include "err.h"
#include "msg.h"

#ifdef DEBUG
	const int debug = 1;
#else
	const int debug = 0;
#endif

typedef struct resource {
	pthread_mutex_t mutex;

	pthread_cond_t frame[2];
	int c_frame[2];
	pthread_cond_t frame_pair;
	int c_frame_pair;

	pthread_cond_t res_q;
	int c_res_q;
	pthread_cond_t res_p;
	int c_res_p;
} t_resource;

int msg_qid;
int K;
int N;
t_resource * resources;
pthread_t * client_thread[MAXPID];
pthread_attr_t client_thread_attr;

/*
	Initialization of mutex and conditional variables
*/
void res_init(t_resource * r){
	int i = 0;
	int err = 0;

	if ((err = pthread_mutex_init(&(r->mutex), 0)) != 0)
		syserr(err, "Server: mutex init failed");

	for (i = 0; i < 2; i++){
		if ((err = pthread_cond_init(&(r->frame[i]), 0)) != 0)
			syserr(err, "Server: cond init failed");
		r->c_frame[i] = 0;
	}

	if ((err = pthread_cond_init(&(r->frame_pair), 0)) != 0)
		syserr(err, "Server: cond init failed");
	r->c_frame_pair = 0;

	if ((err = pthread_cond_init(&(r->res_q), 0)) != 0)
		syserr(err, "Server: cond init failed");
	r->c_res_q = 0;
	
	if ((err = pthread_cond_init(&(r->res_p), 0)) != 0)
		syserr(err, "Server: cond init failed");
	r->c_res_p = 0;
}

/*
	Deletion of mutex and conditional variables
*/
void res_delete(t_resource * r){
	int i = 0;
	int err = 0;

	if ((err = pthread_mutex_destroy(&(r->mutex))) != 0)
		syserr(err, "Server: mutex destroy failed");

	for (i = 0; i < 2; i++){
		if ((err = pthread_cond_destroy(&(r->frame[i]))) != 0)
			syserr(err, "Server: cond destroy failed");
	}

	if ((err = pthread_cond_destroy(&(r->frame_pair))) != 0)
		syserr(err, "Server: cond destroy failed");

	if ((err = pthread_cond_destroy(&(r->res_q))) != 0)
		syserr(err, "Server: cond destroy failed");
	
	if ((err = pthread_cond_destroy(&(r->res_p))) != 0)
		syserr(err, "Server: cond destroy failed");

}

void exit_server(int sig){
	//Deleting message queue
	//TODO wyjebac poza main
	if (debug)
		printf("Server: exit\n");
	if (msgctl(msg_qid, IPC_RMID, 0) == -1)
		syserr(0,"Server: msgctl failed RMID\n");
	if (debug)
		printf("Server: msg queue destroyed\n");
	int i;
	for (i = 0; i < K; i++){
		res_delete(&resources[i]);
	}
	free(resources);
	if (debug)
		printf("Server: resources freed\n");
	if (debug)
		printf("Server: end of exit protocol\n");
	exit(0);
}

void * client(void * msg){
	return 0;
}

int main(int argc, char ** argv){
	int err;
	if (argc != 3){
		printf("Wrong arguments, terminating\n");
		exit(0);
	}
	K = atoi(argv[1]);
	N = atoi(argv[2]);

	if (signal(SIGINT, exit_server) == SIG_ERR)
		syserr(0, "Server: signal failed\n");

	if (signal(SIGTERM, exit_server) == SIG_ERR)
		syserr(0, "Server: signal failed\n");

	if (debug)
		printf("Server: run with K: %i N: %i\n", K, N);

	int i = 0;
	//Initialization of resources
	resources = malloc(sizeof(t_resource) * K);
	for (i = 0; i < K; i++){
		res_init(&resources[i]);
	}
	if (debug)
		printf("Server: resources initialized\n");

	//Creating message queue
	if ((msg_qid = msgget(MSGQID, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
		syserr(0,"Server: msgget failed\n");

	if (debug)
		printf("Server: msg queue created\n");

	if ((err = pthread_attr_init(&client_thread_attr)) == -1)
		syserr(err, "Server pthread attr init failed\n");

	if (debug)
		printf("Server: initialization successful\n");

	while(1){
		t_msg msg;
		printf("Waiting for msg\n");
		msgrcv(msg_qid, &msg, MSGSIZE, 0, 0);
		client_thread[msg.mtype-1] = malloc(sizeof(pthread_t));
		
		if ((err = pthread_create(
					client_thread[msg.mtype-1],
					&client_thread_attr,
					client, &msg))
					== -1)
			syserr(err, "Server: pthread create failed\n");
		
		printf("Msg rcv %li %li %li %li\n", msg.mtype, msg.res_number, msg.res_quantity, msg.partner_pid);
		

	}






}
