/* the Tx mgr functions are implemented here */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sys/sem.h>
#include <fstream>
#include "zgt_def.h"
#include "zgt_tm.h"
#include "zgt_extern.h"

#define TEAM_NO      4     //replace 1 with your team number here

// opens the logfile in append mode. prints debug messages if the flag is 
// defined. An error message is printed if ther is a problem opening

void zgt_tm::openlog(string lfile) {
	FILE *outfile;
#ifdef TM_DEGUG
	printf("entering openlog\n"); fflush(stdout);
	fflush(stdout);
#endif
	logfile = (char *) malloc(sizeof(char) * MAX_FILENAME);
	int i = 0;
	while (lfile[i] != '\0') {
		logfile[i] = lfile[i];
		i++;
	}
	logfile[i] = '\0';
	if ((outfile = fopen(logfile, "a")) == NULL) {
		printf("\nCannot open log file for append:%s\n", logfile);
		fflush(stdout);
		exit(1);
	}
	fprintf(outfile, "---------------------------------------------------------------\n");
	fprintf(outfile, "TxId\tOperation\tObId:Obvalue:optime\tLockType\tStatus\tTxStatus\n");
	fflush(outfile);
#ifdef TM_DEBUG
	printf("leaving openlog\n"); fflush(stdout);
	fflush(stdout);
#endif
}

//create a thread and call the constructor of transaction to
//create the object and intialize the other members of zgt_tx in
//begintx(void *thdarg). Pass the thread arguments in a structure.   

int zgt_tm::BeginTx(long tid) {
#ifdef TM_DEBUG
	printf("\nentering BeginTx\n");
	fflush(stdout);
#endif
	struct param *nodeinfo = (struct param*) malloc(sizeof(struct param));
	pthread_t thread1;
	nodeinfo->tid = tid;
	nodeinfo->obno = -1;
	nodeinfo->count = SEQNUM[tid] = 0;
	int status;
	status = pthread_create(&thread1, NULL, begintx, (void*) nodeinfo); // Fork a thr with tid
	if (status) {
		printf("ERROR: return code from pthread_create() is:%d\n", status);
		exit(-1);
	}
#ifdef TM_DEBUG
	printf("\nleaving BeginTx\n");
	fflush(stdout);
#endif
	return (0);

}

// Call the read function in transaction class. Read operation is just printing 
// the value of the item; But to perform the operation, one needs to make sure
// that the strict 2-phase locking is followed.
// now create the thread and call the method readtx(void *)

int zgt_tm::TxRead(long tid, long obno) {
#ifdef TM_DEBUG
	printf("\nentering TxRead\n"); fflush(stdout);
	fflush(stdout);
#endif
	struct param *nodeinfo = (struct param*) malloc(sizeof(struct param));
	pthread_t thread1;

	nodeinfo->tid = tid;
	nodeinfo->obno = obno;
	nodeinfo->count = --SEQNUM[tid];
	int status;
	status = pthread_create(&thread1, NULL, readtx, (void*) nodeinfo);		//create thread and call readtx method
	if (status) {
		printf("ERROR: return code from pthread_create() is:%d\n", status);
		exit(-1);
	}

#ifdef TM_DEBUG
	printf("\nleaving TxRead\n");
	fflush(stdout);
#endif
	return (0);   //successful operation
}

// write operation is to increement the value by 1. Again the protocol
// need to be adheared to

int zgt_tm::TxWrite(long tid, long obno) {
#ifdef TM_DEBUG
	printf("\nentering TxWrite\n"); fflush(stdout);
	fflush(stdout);
#endif
	struct param *nodeinfo = (struct param*) malloc(sizeof(struct param));
	pthread_t thread1;

	nodeinfo->tid = tid;
	nodeinfo->obno = obno;
	nodeinfo->count = --SEQNUM[tid];
	int status;
	status = pthread_create(&thread1, NULL, writetx, (void*) nodeinfo);		//create thread and call writetx method
	if (status) {
		printf("ERROR: return code from pthread_create() is:%d\n", status);
		exit(-1);
	}

#ifdef TM_DEBUG
	printf("\nleaving TxWrite\n");
	fflush(stdout);
#endif
	return (0);   //successful operation
}

// Calls the commitx in a thread and passes the appropriate parameters

int zgt_tm::CommitTx(long tid) {
#ifdef TM_DEBUG
	printf("\nentering TxCommit\n"); fflush(stdout);
	fflush(stdout);
#endif
	pthread_t thread1;

	struct param *nodeinfo = (struct param*) malloc(sizeof(struct param));
	nodeinfo->tid = tid;
	nodeinfo->obno = -1;
	nodeinfo->count = --SEQNUM[tid];
	
	int status;
	status = pthread_create(&thread1, NULL, committx, (void*) nodeinfo);		//create thread and call committx method
	if (status) {
		printf("ERROR: return code from pthread_create() is:%d\n", status);
		exit(-1);
	}

#ifdef TM_DEBUG
	printf("\nleaving TxCommit\n");
	fflush(stdout);
#endif
	return (0);   //successful operation

}

int zgt_tm::AbortTx(long tid) {
#ifdef TM_DEBUG
	printf("\nentering TxAbort\n"); fflush(stdout);
	fflush(stdout);
#endif
	pthread_t thread1;
	
	struct param *nodeinfo = (struct param*) malloc(sizeof(struct param));
	nodeinfo->tid = tid;
	nodeinfo->obno = -1;
	nodeinfo->count = --SEQNUM[tid];
	
	int status;
	status = pthread_create(&thread1, NULL, aborttx, (void*) nodeinfo);		//create thread and call aborttx method
	if (status) {
		printf("ERROR: return code from pthread_create() is:%d\n", status);
		exit(-1);
	}

#ifdef TM_DEBUG
	printf("\nleaving TxAbort\n");
	fflush(stdout);
#endif

	return (0);
}

// NOT used in this project

int zgt_tm::ddlockDet() {
#ifdef TM_DEBUG
	printf("\nentering ddlockDet\n");
	fflush(stdout);
#endif

	pthread_t thread1;
	int status;
	//   status=pthread_create(&thread1, NULL,ddlockdet,(void*)NULL);	// Fork a thread and pass tid
	//if (status){
	//printf("ERROR: return code from pthread_create() is:%d\n", status);
	//exit(-1);
	//}
#ifdef TM_DEBUG
	printf("\nleaving ddlockDet\n");
	fflush(stdout);
#endif
	return (0);  //successful operation
}

// NOT used  in this project

int zgt_tm::chooseVictim() {
#ifdef TM_DEBUG
	printf("\nentering chooseVictim\n");
	fflush(stdout);
#endif

	pthread_t thread1;
	int status;
	//   status=pthread_create(&thread1, NULL,choosevictim,(void*)NULL);	// Fork a thread and pass tid
	//if (status){
	//printf("ERROR: return code from pthread_create() is:%d\n", status);
	//exit(-1);
	//}
#ifdef TM_DEBUG
	printf("\nleaving chooseVictim\n");
	fflush(stdout);
#endif
	return (0);  //successful operation
}

// initializes the TX manager object.

zgt_tm::zgt_tm() {
	int i, init;

	lastr = NULL;
	//initialize objarray; each element points to a different item
	for (i = 0; i < MAX_ITEMS; ++i)
		objarray[i] = new item(0);  //all init'd to zero

	//initialize optime for the thread to sleep;
	//get an int from random function to sleep 

	int seed = time(NULL);
	srand(seed); /*initialize random number generator*/
	int M = 1000; /* Multiplier */
	for (i = 1; i < MAX_TRANSACTIONS + 1; ++i) {
		double r = (double) ((double) rand() / (double) (RAND_MAX + 1.0));
		//RAND_MAX is defined in stdlib
		double x = (r * M);
		int y = (int) x;
		optime[i] = abs(y);
	}

	//initialize condpool and mutexpool
	for (i = 1; i < MAX_TRANSACTIONS + 1; ++i) {
		pthread_mutex_init(&mutexpool[i], NULL);
		pthread_cond_init(&condpool[i], NULL);
		condset[i] = 0;
	}
	ZGT_Nsema = MAX_TRANSACTIONS + 1; //setting the no of semaphore 

	ZGT_Key_sem = TEAM_NO * 100; //setting the key_t data to a unique value based on team no

	//semget() gets a array of semaphore for a particular key.Here
	//	creating a semaphore with  key 1

	if ((sem = zgt_init_sema(IPC_CREAT)) < 0)
		cout << "Error creating semaphore \n";

	ZGT_Semid = sem;

	//intialising the semaphore value of 0 to 1 and the rest to 0
	zgt_init_sema_0(ZGT_Semid);
	zgt_init_sema_rest(ZGT_Semid);

}
;
