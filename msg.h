/*
	Andrzej Sulecki
	as320426
	2gie zadanie zaliczeniowe
	Systemy Operacyjne 2013/14Z
*/

#ifndef MSG_H
#define MSG_H

#define MAXPID 32768
#define MSGSIZE sizeof(long) * 4
const long QUER_MSGQID = 1234L;
const long ACQD_MSGQID = 1235L;
const long FREE_MSGQID = 1236L;

typedef struct msg {
/*
	MAXPID + 1 for sending to server only first query
	Klient PID for sending to klient or thread
*/
	long mtype;

/*
	pid of klient from/to which is the msg
*/

	long my_pid;
	/*
	 * partner_pid - only when server sends msg - pid of another
	 * process from pair to which we gave resources
	 */
	long partner_pid;
	/*
	 * res_number - only when clients send msg - type of resource
	 * needed
	 */
	long res_number;
	/*
	 * res_quantity - only when clients send msg - quantity of
	 * resource needed
	 */
	long res_quantity;
} t_msg;


#endif
