// semaphore functions are given to you. YOu have to implement tx functions. 
// The param structure is important to pass parameters to the threas 

extern  void *begintx(void *);
extern  void *readtx(void *);
extern  void *writetx(void *);
extern  void *aborttx(void *);
extern  void *committx(void *);
extern  zgt_tx* get_tx(long);
extern  int zgt_init_sema(int);
extern void  zgt_init_sema_0(int);
extern void  zgt_init_sema_rest(int);

extern int  zgt_p(int);
extern int  zgt_v(int);
extern int  zgt_nwait(int);
//extern int zgt_nwait1(int);
//extern void  exit(int);

extern zgt_ht *ZGT_Ht;
//extern zgt_tm *ZGT_Sh;
extern zgt_tx *ZGT_Tx;
#define READWRITE 0

extern int errno;


extern key_t ZGT_Key_sem;
extern int ZGT_Semid ;
extern int ZGT_Nsema ;

extern int system(char *);
extern int ZGT_Initp;
extern int Zgt_errno;

struct param
{
  long tid, obno,count;
};
