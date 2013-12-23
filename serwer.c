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

	pthread_cond_t res_q;
	int c_res_q;
	pthread_cond_t res_p;
	int c_res_p;

	int free;
	int pair_pid;
	int pair_weight;

	pthread_cond_t *free_pair;
	int *partner_finished;
} t_resource;

int query_msg_qid;
int free_msg_qid;
int acqu_msg_qid;

int K;
int N;
t_resource * resources;
//pthread_t * client_thread[MAXPID];
pthread_attr_t client_thread_attr;

/*
	Initialization of mutex and conditional variables
*/
void res_init(t_resource * r, int N){
	int err = 0;

	if ((err = pthread_mutex_init(&(r->mutex), 0)) != 0)
		syserr(err, "Server: mutex init failed");

	if ((err = pthread_cond_init(&(r->res_q), 0)) != 0)
		syserr(err, "Server: cond init failed");
	r->c_res_q = 0;
	
	if ((err = pthread_cond_init(&(r->res_p), 0)) != 0)
		syserr(err, "Server: cond init failed");
	r->c_res_p = 0;

	r->free = N;
}

/*
	Deletion of mutex and conditional variables
*/
void res_delete(t_resource * r){
	int err = 0;
	
	if ((err = pthread_attr_destroy(&(client_thread_attr))) != 0)
		syserr(err, "Server: pthread attr destroy failed");

	if ((err = pthread_mutex_destroy(&(r->mutex))) != 0)
		syserr(err, "Server: mutex destroy failed\n");

	if ((err = pthread_cond_destroy(&(r->res_q))) != 0)
		syserr(err, "Server: cond destroy failed\n");
	
	if ((err = pthread_cond_destroy(&(r->res_p))) != 0)
		syserr(err, "Server: cond destroy failed\n");

}

void exit_server(int sig){
	//Deleting message queue
	if (debug)
		printf("Server: exit\n");
	if (msgctl(query_msg_qid, IPC_RMID, 0) == -1)
		syserr(0,"Server: msgctl failed RMID\n");
	if (msgctl(acqu_msg_qid, IPC_RMID, 0) == -1)
		syserr(0,"Server: msgctl failed RMID\n");
	if (msgctl(free_msg_qid, IPC_RMID, 0) == -1)
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

void * client_cleanup(void * arg){
	int err;

	if ((err = pthread_mutex_unlock((pthread_mutex_t *) arg)) != 0)
		syserr(err, "Thread cleanup failed\n");
	return 0;
}

void * client(void * m){
	t_msg * msg = m;
	int err;
	int state;
//	pthread_cond_t * pair_finish;
	int * pair_finished;
		
	if ((err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &state)) != 0)
		syserr(err, "Thread setcancelstate fail\n");

	if ((err = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &state)) != 0)
		syserr(err, "Thread setcanceltype fail\n");
	
	if (debug)
		printf("Thread with msg %li %li %li %li %li\n", 
					msg->mtype,
					msg->my_pid,
					msg->res_number,
					msg->res_quantity, 
					msg->partner_pid);

	t_resource * res = &(resources[msg->res_number-1]);

	pthread_cleanup_push(client_cleanup, (void *)&(res->mutex));
	if ((err = pthread_mutex_lock(&(res->mutex))) != 0)
		syserr(err, "Thread mutex lock fail\n");
	
	if (res->c_res_q != 0 || res->c_res_p == 2){
		res->c_res_q++;
		if (debug)
			printf("Czekamy na miejsce w parze %li\n", msg->my_pid);
		pthread_cond_wait(&(res->res_q), &(res->mutex));
		res->c_res_q--;
	}

	if (res->c_res_p == 0){
		
		//res->free_pair = malloc(sizeof(pthread_cond_t));
		//if ((err = pthread_cond_init(res->free_pair, 0)) != 0)
		//	syserr(err, "Free pair cond init fail\n");
		res->partner_finished = calloc(sizeof(int), 1);


		res->c_res_p++;
		res->pair_weight = msg->res_quantity;
		res->pair_pid = msg->my_pid;

		if (res->c_res_q != 0)
			pthread_cond_signal(&(res->res_q));

		if (debug)
			printf("Czekamy na drugiego w parze %li\n", msg->my_pid);
		pthread_cond_wait(&(res->res_p), &(res->mutex));
		msg->partner_pid = res->pair_pid;
	} else {
		res->c_res_p++;
		res->pair_weight += msg->res_quantity;
		msg->partner_pid = res->pair_pid;
		res->pair_pid = msg->my_pid;
		if (res->pair_weight > res->free){
			if (debug)
				printf("Czekamy na wolne zasoby %li\n", msg->my_pid);
			pthread_cond_wait(&(res->res_p), &(res->mutex));
		}
		res->free -= res->pair_weight;
		printf("Watek %ld przyznal %li+%li zasobow klientom %li %li, pozostalo %i zasobow\n",
			pthread_self(),
			msg->res_quantity,
			res->pair_weight - msg->res_quantity,
			msg->my_pid,
			msg->partner_pid,
			res->free);
	}
	if (res->c_res_p == 2){
		res->c_res_p--;
		pthread_cond_signal(&(res->res_p));
	} else {
		res->c_res_p--;
		pthread_cond_signal(&(res->res_q));
	}
	
	msg->mtype = msg->my_pid;
	//pair_finish = res->free_pair;
	pair_finished = res->partner_finished;
	
	if ((err = pthread_mutex_unlock(&(res->mutex))) != 0)
		syserr(err, "Thread mutex unlock fail\n");
	pthread_cleanup_pop(0);
	if (msgsnd(acqu_msg_qid, msg, MSGSIZE, 0) == -1)
		syserr(0, "Thread msgsnd fail\n");
	if (debug)
		printf("Message sent to klient %li, starting work\n", msg->mtype);
	
	//czekamy na wiadomosc

	t_msg msgend;
	if (debug)
		printf("Waiting for klient %li end\n", msg->my_pid);
	msgrcv(free_msg_qid, &msgend, MSGSIZE, msg->my_pid, 0);
	
	
	pthread_cleanup_push(client_cleanup, (void *)&(res->mutex));
	if ((err = pthread_mutex_lock(&(res->mutex))) != 0)
		syserr(err, "Thread mutex lock fail\n");

	if ((*pair_finished) == 0){
		*pair_finished = msg->res_quantity;
		//pthread_cond_wait(pair_finish, &(res->mutex));
	} else {
		//pthread_cond_signal(pair_finish);
		*pair_finished += msg->res_quantity;
		res->free += *pair_finished;

		if (res->c_res_p == 2 && res->free >= res->pair_weight)
			pthread_cond_signal(&(res->res_p));

		if (debug)
			printf("Zwolniono %i dostepnych %i\n", *pair_finished, res->free);
		//free(pair_finish);
		//free(pair_finished);
	}

	
	if ((err = pthread_mutex_unlock(&(res->mutex))) != 0)
		syserr(err, "Thread mutex unlock fail\n");
	pthread_cleanup_pop(0);

	if (debug)
		printf("Koniec watku %li\n", msg->my_pid);


	free(msg);
	pthread_exit(0);
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
	resources = calloc(sizeof(t_resource), K);
	for (i = 0; i < K; i++){
		res_init(&resources[i], N);
	}
	if (debug)
		printf("Server: resources initialized\n");

	//Creating message queue
//	if ((msg_qid = msgget(MSGQID, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
//		syserr(0,"Server: msgget failed\n");
	if ((query_msg_qid = msgget(QUER_MSGQID, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
		syserr(0,"Klient: msgget failed in client\n");
	if ((free_msg_qid = msgget(FREE_MSGQID, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
		syserr(0,"Klient: msgget failed in client\n");
	if ((acqu_msg_qid = msgget(ACQD_MSGQID, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
		syserr(0,"Klient: msgget failed in client\n");



	if (debug)
		printf("Server: msg queue created\n");
	
	//Client thread attributes
	if ((err = pthread_attr_init(&client_thread_attr)) == -1)
		syserr(err, "Server pthread attr init failed\n");
	if ((err = pthread_attr_setdetachstate(
				&client_thread_attr,
				PTHREAD_CREATE_DETACHED)) != 0)
		syserr(err, "Server: setdetachstate failed\n");


	if (debug)
		printf("Server: initialization successful\n");
//	int threads = 0;
	while(1){
		/*threads ++;
		if (threads > 250){
			sleep(10);
			threads = 0;
		}*/
		t_msg * msg = malloc(sizeof(t_msg));
		if (debug)
			printf("Waiting for msg\n");
		msgrcv(query_msg_qid, msg, MSGSIZE, MAXPID+1, 0);
		pthread_t client_thread;// = malloc(sizeof(pthread_t));
		
		if ((err = pthread_create(
					&client_thread,
					&client_thread_attr,
					client, msg))
					== -1)
			syserr(err, "Server: pthread create failed\n");
		if (debug)
			printf("Msg rcv %li %li %li %li\n", 
				msg->mtype, 
				msg->res_number, 
				msg->res_quantity, 
				msg->partner_pid);

	}






}
