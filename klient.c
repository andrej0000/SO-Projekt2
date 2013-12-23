/*
	Andrzej Sulecki
	as320426
	2gie zadanie zaliczeniowe
	Systemy Operacyjne 2013/14Z
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include "err.h"
#include "msg.h"
#ifdef DEBUG
	const int debug = 1;
#else
	const int debug = 0;
#endif

/*
	Should be run with 3 parameters - ./klient k n s
	where
		k - type of resource
		n - quantity of resource
		s - time of work with resource
*/
int query_msg_qid;
int free_msg_qid;
int acqu_msg_qid;

int main(int argc, char ** argv){
	if (argc != 4){
		printf("Wrong arguments, terminating\n");
		exit(0);
	}
	const int K = atoi(argv[1]);
	const int N = atoi(argv[2]);
	const int S = atoi(argv[3]);

	if ((query_msg_qid = msgget(QUER_MSGQID, 0)) == -1)
		syserr(0,"Klient: msgget failed in client\n");
	if ((free_msg_qid = msgget(FREE_MSGQID, 0)) == -1)
		syserr(0,"Klient: msgget failed in client\n");
	if ((acqu_msg_qid = msgget(ACQD_MSGQID, 0)) == -1)
		syserr(0,"Klient: msgget failed in client\n");

	t_msg msg;
	msg.mtype = MAXPID + 1;
	msg.my_pid = getpid();
	msg.partner_pid = 0;
	msg.res_number = K;
	msg.res_quantity = N;

	if (msgsnd(query_msg_qid, &msg, MSGSIZE, 0) == -1)
		syserr(0, "Klient: msgsnd failed\n");
	if (debug)
		printf("Klient: msgsnd %i\n", getpid());

	msgrcv(acqu_msg_qid, &msg, MSGSIZE, getpid(), 0);

	printf("%i %i %li %li\n", K, N, msg.my_pid, msg.partner_pid);

	sleep(S);

	msgsnd(free_msg_qid, &msg, MSGSIZE, 0);

	printf("%i KONIEC\n", getpid());
	return 0;
}
