#ifndef MSG_H
#define MSG_H

#define MAXPID 32768
#define MSGSIZE sizeof(long) * 4
const long MSGQID = 1234L;

typedef struct msg {
	/*
	 * mtype - pid of client process which sends msg
	 * or MAXPID + pid of client process to whom msg is send
	 */
	long mtype;
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
