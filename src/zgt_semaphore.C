/* all semaphore operations are implemented in this file */

#include <stdio.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "zgt_def.h"
//#include "zgt_tx.h"
#include "zgt_tm.h"
#include "zgt_extern.h"
union semun {
	int val;
	struct semid_ds *buf;
	ushort *array;
} ZGT_arg;

struct sembuf ZGT_sopbuf, *ZGT_sops = &ZGT_sopbuf;

int zgt_init_sema(int create) {
	int semid;
	if (create != IPC_CREAT)
		create = 0;

	if ((semid = semget(ZGT_Key_sem, ZGT_Nsema, create | 0666)) < 0) {
		/* error handling */
		Zgt_errno = ZGT_SEMA_ALLOC_FAILS;
		return (-1);
	}
	return (semid);

}

// 0 (zero) semaphore is used for the Tx manager data structure; hence it is 
// initialized to 1 to permit the first operation to proceed

void zgt_init_sema_0(int semid) {
	ZGT_arg.val = 1;
	semctl(semid, 0, SETVAL, ZGT_arg);
}

// The rest of the semaphores are used by the transactions to wait on other Txs
// if a lock is NOT obtained. Hence they are initialized to 0. ZGT_Nsema is 
// initialized to MAX_TRANSATIONS+1. A transaction can be waiting only for 
// one other tx. Mutiple Txs can be waiting for a Tx

void zgt_init_sema_rest(int semid) {
	int k;
	for (k = 1; k < ZGT_Nsema; k++) {
		ZGT_arg.val = 0;
		semctl(semid, k, SETVAL, ZGT_arg);
	}

}

// executes the p operation on the semaphore indicated.
// wait()/lock() on sem

int zgt_p(int sem) {
	int errno;
	ZGT_sops->sem_num = sem;
	ZGT_sops->sem_op = -1;
	ZGT_sops->sem_flg = 0;
	if (semop(ZGT_Semid, ZGT_sops, 1) < 0) {
		Zgt_errno = ZGT_SEMA_P_OP_FAILS;
		return (-1);
	}

	return (0);
}

// executes the v operation on the semaphore indicated
// signal()/unlock() on sem
int zgt_v(int sem) {
	ZGT_sops->sem_num = sem;
	ZGT_sops->sem_op = 1;
	ZGT_sops->sem_flg = 0;
	if (semop(ZGT_Semid, ZGT_sops, 1) < 0) {
		Zgt_errno = ZGT_SEMA_V_OP_FAILS;

		return (-1);
	}

	return (0);
}

// Returns the # of Txs waiting on a given semaphore; This is needed to release
// all Txs that are waiting on a tx when it commits or aborts.

int zgt_nwait(int sem) {
	return (semctl(ZGT_Semid, sem, GETNCNT, ZGT_arg));
}
