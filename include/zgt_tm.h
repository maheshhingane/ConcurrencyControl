#include <stdio.h>
#include <string>
#include <stdlib.h>
#include "zgt_tx.h"
#include <iostream>
#define MAX_ITEMS 10
#define MAX_TRANSACTIONS 15
#define MAX_FILENAME  50

using namespace std;

/* data item associated with each Tx. The values are incremented to simulate read and write */

class item {
  public:
	int value;

	item(int k){
	  value=k;
	};

	~item();
 };

/* main tm data structure for the project */
class zgt_tm{
	
	public:
	
	friend class zgt_tx;
	friend class zgt_ht;

	long lastid;
	zgt_tx *lastr;
	zgt_hlink *head[ZGT_DEFAULT_HASH_TABLE_SIZE];
	pthread_mutex_t mutexpool[MAX_TRANSACTIONS+1];
	pthread_cond_t condpool[MAX_TRANSACTIONS+1];
	int condset[MAX_TRANSACTIONS+1];
	item *objarray[MAX_ITEMS];
	int  optime[MAX_TRANSACTIONS+1];
  	int sem;
	char *logfile;

/*Added for serialization. One integer is reserved for each transaction. */
/*Every statement within a transaction is given SEQNUM[tid]-- and every statement */
/* waits on that count instead of 0.*/
        int SEQNUM[MAX_TRANSACTIONS+1];

	public:
	
		zgt_tm();
        void openlog(string lfile);
		int BeginTx(long tid);
		int CommitTx(long tid);
		int AbortTx(long tid);
		int TxRead(long tid,long obno);
		int TxWrite(long tid,long obno);
		int ddlockDet();
		int chooseVictim();
		~zgt_tm();
};
//Added a structure to pass arguemnts to threads. The count is used to seralize 
//between various statments in one transaction.

typedef struct thread_arg
{
        long tid;
        long obnum;
        int count;
}THREAD_ARG;
