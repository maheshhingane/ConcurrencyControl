/* this is currently NOT used in your project; ignore this file for the time being */

#define TRUE	1
#include <stddef.h>
#define FALSE	0

struct node {
	long tid;
	long sgno;
	long obno;
	char lockmode;
	int semno;
	int level;
	node*	next;
	node*	next_s;
	node*	parent;
};
class wait_for {
	node*	head;
	node*	wtable;
	int	found;
	node*	victim;
	int visited(long);
	node* location(long);
	int traverse(node *);
	node* choose_victim(node *, node *);
	public:
	int deadlock();
	wait_for();
	~wait_for(){};
};
