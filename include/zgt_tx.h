#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <pthread.h>

/* hash node data structure; see the figure in the description */

struct zgt_hlink
{
  char lockmode;
  long sgno;
  long obno;
  long tid;
  pthread_t pid;

  zgt_hlink *next;        //links nodes hashed to the same bucket
  zgt_hlink *nextp;       //links nodes of the same transaction
};


// tx object; objno and lockmode are used to indicate the object on which this 
// tx is waiting. pid is used to store thrid. sgno is always 1. If Tx 1 is 
// waiting for Tx 2, then the thread of Tx 1 will wait on semno 2 and this semno
// will be stored in the tx object corresponding to Tx2

 
class zgt_tx {

 public:
  
  friend class zgt_ht;
  friend class wait_for;
   

  long tid;         //need a friend class here
  pthread_t pid;
  long sgno;
  long obno;
  char status;
  char lockmode;
  int semno;
  zgt_hlink *head;           // head of lock table
  zgt_hlink *others_lock(zgt_hlink *, long, long); 
  zgt_tx *nextr;
  
  public :
    
  int free_locks();
  int remove_tx();
  //zgt_tx *get_tx(long);
  void print_wait();
  void print_lock();
  long get_tid(){return tid;}
  long set_tid(long t){tid = t; return tid;}
  char get_status() {return status;}
  int set_lock(long, long, long, char);
  int end_tx();
  int cleanup();
  zgt_tx(long,char,pthread_t);
  //  void *start_operation(long);  //starts opeartion by doing conditional wait
  //void *finish_operation(long); // finishes abn operation by removing conditional wait
  //void *open_logfile_for_append(); //opens log file for writing
  //void *do_commit_abort(long, char); // performs commit or abort (the code is same for us)

  void perform_readWrite(long, long, char);
  int  setTx_semno(long, int);
  void print_tm();
  zgt_tx(){};
};


// The Zeitgeist encapsulation object hash table class

class zgt_ht
{

  friend class zgt_tx;
  friend class wait_for;
    
 public:

  /*  methods  */
  
  zgt_hlink *find (long, long); //find the obj in hash table 
  zgt_hlink *findt (long, long, long); //find the tx obj belongs to
  int add ( zgt_tx *, long, long, char); //add an obj for a tx to hash table
  int remove ( zgt_tx *, long, long);  //remove a lock entry
  void print_ht();  // prints the hash table
  
  /*  constructors & destructors  */
  
  zgt_ht (int ht_size = ZGT_DEFAULT_HASH_TABLE_SIZE);
  ~zgt_ht ();
  
 private:
  
  int  mask;
  int size;	
  int hashing(long sgno, long obno)   // hashing function
    {return((++sgno)*obno)&mask;}

};







