/***************** Transaction class **********************/
/*** Implements methods that handle Begin, Read, Write, ***/
/*** Abort, Commit operations of transactions. These    ***/
/*** methods are passed as parameters to threads        ***/
/*** spawned by Transaction manager class.              ***/
/**********************************************************/

/* Required header files */

#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include "zgt_def.h"
#include "zgt_tm.h"
#include "zgt_extern.h"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <pthread.h>

extern void *start_operation(long, long); 										//starts operation by doing conditional wait
extern void *finish_operation(long); 											//finishes an operation by removing conditional wait
extern void *open_logfile_for_append(); 										//opens log file for writing
extern void *do_commit_abort(long, char); 										//performs commit or abort (the code is same for us)
extern void *process_read_write(long, long, int, char);							//performs read or write

extern zgt_tm *ZGT_Sh;															//Transaction manager object

FILE *logfile; 																	//declare globally to be used by all

// general instructions for all thread functions:
// In the thread, to avoid simultaneous operation by the same transaction, 
// check the condset to see if the value is zero. Before doing any operation 
// on the shared condset, set the mutex. If obtained, decrement the condset 
// value and release the mutex. Else do a cond_wait till a cond_broadcast is 
// received.

// Lock the transaction manager with semaphore before any operations. Use
// semaphore 0. Print the errors as and when occurs as the thread will not 
// return the errors to the calling program. 

/* Transaction class constructor */
/* Initializes transaction id and status and thread id */
/* Input: Transaction id, status, thread id */

zgt_tx::zgt_tx(long tid, char Txstatus, pthread_t thrid) {
	this->lockmode = (char) ' ';  												//default
	this->sgno = 1;
	this->tid = tid;
	this->obno = -1; 															//set it to a invalid value
	this->status = Txstatus;
	this->pid = thrid;
	this->head = NULL;
	this->nextr = NULL;
	this->semno = -1; 															//init to  an invalid sem value
}

/* Method used to obtain reference to a transaction node      */
/* Inputs the transaction id. Makes a linear scan over the    */
/* linked list of transaction nodes and returns the reference */
/* of the required node if found. Otherwise returns NULL      */

zgt_tx* get_tx(long tid1) {
	zgt_tx *txptr, *lastr1;
	if (ZGT_Sh->lastr != NULL)													//If the list is not empty
	{
		lastr1 = ZGT_Sh->lastr;													//Initialize lastr1 to first node's ptr
		for (txptr = lastr1; (txptr != NULL); txptr = txptr->nextr)
			if (txptr->tid == tid1) 											//if required id is found
				return txptr;

		return (NULL);															//if not found in list return NULL
	}
	return (NULL);																//if list is empty return NULL
}

/* function that handles "BeginTx tid" in zgt_tm.C     */
/* Inputs a pointer to transaction id, obj pair as a struct. Creates a new  */
/* transaction node, initializes its data members and */
/* adds it to transaction list */

void *begintx(void *thdarg) {
	//initialise a transaction object. Make sure it is 
	//done after acquiring the semaphore for the tm and making sure that 
	//the operation can proceed using the condition variable. when creating
	//the tx object, set the tx to TR_ACTIVE and obno to -1; there is no 
	//semno as yet as none is waiting on this tx.

	struct param *node = (struct param*) thdarg;								//get thread arguments
	start_operation(node->tid, node->count);									//continue begin only if cond var permits
	zgt_tx *tx = new zgt_tx(node->tid, TR_ACTIVE, pthread_self());				//Create new transaction object
	open_logfile_for_append();													//Write to log file
	fprintf(logfile, "T%d\tBeginTx\n", node->tid);
	fflush(logfile);
	
	zgt_p(0);																	//Lock tx-mgr semaphore
	tx->nextr = ZGT_Sh->lastr;													//Insert new transaction in linked list
	ZGT_Sh->lastr = tx;
	zgt_v(0);																	//Release semaphore

	finish_operation(node->tid);

	pthread_exit(NULL); 														//thread exit
}

/* Method to handle Readtx action in test file    */
/* Inputs a pointer to structure that contains     */
/* tx id and object no to read. Reads the object  */
/* if the object is not yet present in the hash table */
/* or same tx holds a lock on it. Otherwise waits */
/* until the lock is released */

/* Read operation loops for optime doing some computation */
/* it also prints the value of the item to the log */

void *readtx(void *arg){
	struct param *node = (struct param*)arg;
	process_read_write(node->tid, node->obno, node->count, 'S');				//try to acquire READ lock on the given object
}

void *writetx(void *arg){
	struct param *node = (struct param*)arg;
	process_read_write(node->tid, node->obno, node->count, 'X');				//try to acquire WRITE lock on the given object
}

void *process_read_write(long tid, long obno, int count, char mode){
	start_operation(tid, count);												//proceed only if the cond var permits
	zgt_p(0);																	//lock the tx manager object
	zgt_tx *txptr = get_tx(tid);
	if(txptr == NULL){															//trying to lock non-existing tx 
		fprintf(logfile, "\n***T%d ReadTx/WriteTx (%c) on obno %d failed. Transaction does not exist.***\n",  tid, mode, obno);
		fflush(logfile);
		zgt_v(0);
		finish_operation(tid);
		pthread_exit(NULL);
	}

	switch (txptr->status){														//check status of the transaction and take appropriate action
		case TR_ABORT:															//if tx is aborted, throw error
			fprintf(logfile, "\n***T%d is in abort state while reading/writing (%C) on obno %d.***\n",  tid, mode, obno);
			fflush(logfile);
			do_commit_abort(tid, TR_ABORT);
			zgt_v(0);
			finish_operation(tid);
			pthread_exit(NULL);
			break;
		case TR_WAIT:															//if tx is waiting, throw error
			fprintf(logfile, "\n***T%d is in wait state while reading/writing (%C) on obno %d.***\n",  tid, mode, obno);
			fflush(logfile);
			do_commit_abort(tid, TR_ABORT);
			zgt_v(0);
			finish_operation(tid);
			pthread_exit(NULL);
			break;
		case TR_END:															//if tx is committed, throw error
			fprintf(logfile, "\n***T%d is in commit state while reading/writing (%C) on obno %d.***\n",  tid, mode, obno);
			fflush(logfile);
			do_commit_abort(tid, TR_END);
			zgt_v(0);
			finish_operation(tid);
			pthread_exit(NULL);
			break;
		case TR_ACTIVE:															//if tx is active, try to acquire the lock
			txptr->set_lock(tid, 1, obno, mode);
			finish_operation(tid);
			pthread_exit(NULL);
			break;
		default:																//default, throw error
			fprintf(logfile, "\n***ERROR: T%d\t while reading/writing (%C) on obno %d.***\n", tid, mode, obno);
			fflush(logfile);
			do_commit_abort(tid, TR_ABORT);
			finish_operation(tid);
			zgt_v(0);
			pthread_exit(NULL);
			break;
	}
}


// This function aborts the tx. You need to set the transaction status 
// to TR_ABORT. As the abort can happen when a Tx is blocked (in wait state)
// you need to test the status when you continue after waiting for a resource 
// to make sure you do not continue a transaction that has been aborted 
// while waiting for a resource. Of course, you need to release all resources
// helld by an aborted Tx

void *aborttx(void *arg) {
	struct param *node = (struct param*) arg;									//get tid and objno
	start_operation(node->tid, node->count);									//proceed only if the cond var permits
	zgt_p(0);
	do_commit_abort(node->tid, TR_ABORT);										//Free the locks ans remove transaction from linked list
	zgt_v(0);
	finish_operation(node->tid);
	pthread_exit(NULL);															//thread exit
}

// similar to abort. Releases resources as well

void *committx(void *arg) {
	struct param *node = (struct param*) arg;									//get tid and objno
	start_operation(node->tid, node->count);									//proceed only if the cond var permits
	zgt_p(0);
	do_commit_abort(node->tid, TR_END);											//Free the locks ans remove transaction from linked list
	zgt_v(0);
	finish_operation(node->tid);
	pthread_exit(NULL);															//thread exit
}

// called from commit/abort with appropriate parameter to do the actual
// operation. Make sure you give error messages if you are trying to
// commit/abort a non-existant tx

void *do_commit_abort(long t, char status) {
	open_logfile_for_append();													//write the status in log file
	if(status == TR_ABORT)
		fprintf(logfile, "T%d\tAbortTx\t", t);
	else
		fprintf(logfile, "T%d\tCommitTx\t", t);
	fflush(logfile);

	zgt_tx *txptr = get_tx(t);
	if(txptr == NULL){															//trying to commit/abort non-existing tx
		fprintf(logfile, "\n***Trying to commit/abort transction that does not exit.***\n");
		fflush(logfile);
	}
	else{
		if(txptr->status == TR_WAIT){											//trying to commit/abort waiting tx
			fprintf(logfile, "T%d\t is in wrong state for abort/commit.\n", t);
			fflush(logfile);
		}
		txptr->status = status;													//change the status of the tx
		txptr->free_locks();													//free the locks
		int releasetxs = txptr->semno;
		txptr->remove_tx();														//remove tx from list of txs
		int i, txswaiting;
		if (releasetxs != -1) {													//if some other txs were waiting on this tx, release the semaphores so that they can proceed
			txswaiting = zgt_nwait(releasetxs);
			for(i=1; i<=txswaiting; i++)
				zgt_v(releasetxs);
		}
	}
}

// removes the transaction from the tm. Make sure it is either 
// in abort (TR_ABORT) or end (TR_END) state

int zgt_tx::remove_tx() {
	zgt_tx *txptr = NULL, *lastr1;
	if (ZGT_Sh->lastr != NULL){
		lastr1 = ZGT_Sh->lastr;
		if (lastr1->tid == tid)													//if first node is req node
			ZGT_Sh->lastr = lastr1->nextr;										//update lastr value
		else{
			for (txptr = lastr1; txptr != NULL; txptr = txptr->nextr){			//else scan through list
				if (txptr->tid == tid){											//if req node is found
					lastr1->nextr = txptr->nextr;								//update nextr value
					break;
				}
				else
					lastr1 = txptr;												//else update prev value
			}
		}
	}
	return (0);
}

// this method sets lock on objno1 with lockmode1 for a tx 
// if the operation cannot be done, it waits on a semaphore
int zgt_tx::set_lock(long tid1, long sgno1, long obno1, char lockmode1) {
	//if the thread has to wait, block the thread on a semaphore from the
	//sempool in the transaction manager. Set the appropriate parameters in the
	//transaction list if waiting.
	//if successful  return(0);

	zgt_hlink *hnode, *others, *temp;
	open_logfile_for_append();

	hnode = ZGT_Ht->find(sgno1, obno1);											//find the tx holding lock on the current object
	if(hnode == NULL){															//if no tx is holding the lock, set the lock and add an entry in lock table
		if(ZGT_Ht->add(this, sgno1, obno1, lockmode1) < 0){
			printf("\n***ERROR: Error in creating hash node in ZGT_Ht->add.***\n");
			fflush(stdout);
			zgt_v(0);
			return(0);
		}
		zgt_v(0);
		perform_readWrite(tid1, obno1, lockmode1);								//increment/decrement objvalue
		return(0);
	}
	else{																		//if some tx is holding the lock on the current object
		temp = ZGT_Ht->findt(this->tid, sgno1, obno1);							//find which is that tx
		if(temp == NULL){														//if that tx is other than current tx
			others = this->others_lock(hnode, sgno1, obno1);
			if(others != NULL){
				this->status = TR_WAIT;											//change status of current tx to wait
				this->lockmode = lockmode1;
				this->obno = obno1;
				this->setTx_semno(others->tid, others->tid);					//set the semno of other tx to indicate that the current tx is waiting on it
				zgt_v(0);
				zgt_p(others->tid);												//wait for the other tx to release the object
				this->status = TR_ACTIVE;										//when the object is released, proceed with current tx
				zgt_p(0);
				set_lock(this->tid, this->sgno, this->obno, this->lockmode);	//try to acquire the lock on the object now
				return(0);
			}
		}
		status = TR_ACTIVE;														//if the holding tx is same as current tx
		hnode->lockmode = lockmode1;											//set the lockmode
		zgt_v(0);
		this->perform_readWrite(tid, obno1, lockmode1);							//increment/decrement objvalue
	}
}

// this part frees all locks owned by the transaction
// Need to free the thread in the waiting queue 
// try to obtain the lock for the freed threads
// if the process itself is blocked, clear the wait and semaphores

int zgt_tx::free_locks() {
	zgt_hlink* temp = head;  													//first obj of tx
	open_logfile_for_append();
	for (; temp; temp = temp->nextp){											//For each lock held by tx
																				//Record release of lock
#ifdef TX_DEBUG
		fprintf(logfile, "%d : %d \t", temp->obno, ZGT_Sh->objarray[temp->obno]->value);
		fflush(logfile);
#endif
		if (ZGT_Ht->remove(this, 1, (long) temp->obno) == 1) {
			printf(":::ERROR:node with tid:%d and onjno:%d was not found for deleting", this->tid, temp->obno);		// Release from hash table
			fflush(stdout);
		} else {
#ifdef TX_DEBUG
			printf("\n:::Hash node with Tid:%d, obno:%d lockmode:%c removed\n", temp->tid, temp->obno, temp->lockmode);
			fflush(stdout);
#endif
		}
	}
	fprintf(logfile, "\n");
	fflush(logfile);

	return (0);
}

// CURRENTLY Not USED
// USED to COMMIT 
// remove the transaction and free all associate dobjects. For the time being 
// this can be used for commit of the transaction.

int zgt_tx::end_tx() {
	zgt_tx *linktx, *prevp;

	linktx = prevp = ZGT_Sh->lastr;

	while (linktx) {
		if (linktx->tid == this->tid)
			break;
		prevp = linktx;
		linktx = linktx->nextr;
	}
	if (linktx == NULL) {
		printf("\ncannot remove a Tx node; error\n");
		fflush(stdout);
		return (1);
	}
	if (linktx == ZGT_Sh->lastr)
		ZGT_Sh->lastr = linktx->nextr;
	else {
		prevp = ZGT_Sh->lastr;
		while (prevp->nextr != linktx)
			prevp = prevp->nextr;
		prevp->nextr = linktx->nextr;
	}
}

// currently not used

int zgt_tx::cleanup() {
	return (0);

}

// check which other transaction has the lock on the same obno
// returns the hash node

zgt_hlink *zgt_tx::others_lock(zgt_hlink *hnodep, long sgno1, long obno1) {

	zgt_hlink *ep;
	ep = ZGT_Ht->find(sgno1, obno1);         									//locate the object
	while (ep)																	//while ep is not null
	{
		if ((ep->obno == obno1) && (ep->sgno == sgno1) && (ep->tid != this->tid))
			return (ep);														//return the hashnode that holds the lock
		else
			ep = ep->next;
	}
	return (NULL);																//Return null otherwise 
}

// routine to print the tx list
// TX_DEBUG should be defined in the Makefile to print

void zgt_tx::print_tm() {

	zgt_tx *txptr;

#ifdef TX_DEBUG
	printf("printing the tx list \n");
	printf("Tid \t Thrid \t objno \t lock \t status \t semno\n");
	fflush(stdout);
#endif
	txptr = ZGT_Sh->lastr;
	while (txptr != NULL) {
#ifdef TX_DEBUG
		printf("%d\t%d\t%d\t%c\t%c\t%d\n", txptr->tid, txptr->pid, txptr->obno, txptr->lockmode, txptr->status, txptr->semno);
		fflush(stdout);
#endif
		txptr = txptr->nextr;
	}
	fflush(stdout);
}

//currently not used

void zgt_tx::print_wait() {

	//route for printing for debugging

	printf("\n    SGNO        OBNO        TID        PID         SEMNO   L\n");
	printf("\n");
}
void zgt_tx::print_lock() {
	//routine for printing for debugging

	printf("\n    SGNO        OBNO        TID        PID   L\n");
	printf("\n");

}

//  routine to perform the acutual read/write operation
// based  on the lockmode

void zgt_tx::perform_readWrite(long tid, long obno, char lockmode) {

	int j;
	double result = 0.0;

	open_logfile_for_append();

	int obValue = ZGT_Sh->objarray[obno]->value;
	if (lockmode == WRITE_LOCK) { 												//write op
		ZGT_Sh->objarray[obno]->value = obValue + 1;							//update value of obj
		fprintf(logfile, "T%d\tWriteTx\t\t%d:%d:%d\t\t\tWriteLock\tGranted\t\t%c\n", this->tid, obno, ZGT_Sh->objarray[obno]->value, ZGT_Sh->optime[tid], this->status);
		fflush(logfile);
		for (j = 0; j < ZGT_Sh->optime[tid] * 20; j++)
			result = result + (double) random();
	} else {   																	//read op
		ZGT_Sh->objarray[obno]->value = obValue - 1;							//update value of obj
		fprintf(logfile, "T%d\tReadTx\t\t%d:%d:%d\t\t\tReadLock\tGranted\t\t%c\n", this->tid, obno, ZGT_Sh->objarray[obno]->value, ZGT_Sh->optime[tid], this->status);
		fflush(logfile);
		for (j = 0; j < ZGT_Sh->optime[tid] * 10; j++)
			result = result + (double) random();
	}
}

// routine that sets the semno in the Tx when another tx waits on it.
// the same number is the same as the tx number on which a Tx is waiting 

int zgt_tx::setTx_semno(long tid, int semno) {
	zgt_tx *txptr = get_tx(tid);
	if(txptr == NULL){
		printf("\n***ERROR: Tx holding the lock is NULL.***\n");
		fflush(stdout);
		return(-1);
	}
	if(txptr->semno == -1){
		txptr->semno = semno;
		return(0);
	}
	else if(txptr->semno != semno){
		exit(1);
	}
	return 0;
}

// routine to start an operation by checking the previous operation of the same
// tx has completed; otherwise, it will do a conditional wait until the 
// current thread signals a broadcast

void *start_operation(long tid, long count) {
	pthread_mutex_lock(&ZGT_Sh->mutexpool[tid]);								//Lock mutex[t] to make other threads of same transaction to wait
	while (ZGT_Sh->condset[tid] != count)										//wait if condset[t] is != 0
		pthread_cond_wait(&ZGT_Sh->condpool[tid], &ZGT_Sh->mutexpool[tid]);
}

// signals the conditional broadcast

void *finish_operation(long tid) {
	ZGT_Sh->condset[tid]--;														//set condset[tid] to 0 and signal
	pthread_cond_broadcast(&ZGT_Sh->condpool[tid]);								//other waiting threads of same tx
	pthread_mutex_unlock(&ZGT_Sh->mutexpool[tid]);
}

void *open_logfile_for_append() {
	if ((logfile = fopen(ZGT_Sh->logfile, "a")) == NULL) {
		printf("\nCannot open log file for append:%s\n", ZGT_Sh->logfile);
		fflush(stdout);
		exit(1);
	}
}
