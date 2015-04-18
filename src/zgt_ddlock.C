// deadlock detection; NOT USED IN THIS PROJECT

#define NULL	0
#define TRUE	1
#define FALSE	0
#include "zgt_def.h"
#include "zgt_tx.h"

#include "zgt_extern.h"
#include "zgt_ddlock.h"

wait_for::wait_for() {
	zgt_tx *tp;
	zgt_tm *tm;
	node* np;
	wtable = NULL;

	if (zgt_p(SHARED_MEM_AVAIL) < 0) {
		Zgt_errno = ZGT_SEMA_P_OP_FAILS;
	};
	for (tp = (zgt_tx *) ZGT_Sh->lastr; tp != NULL; tp = tp->nextr) {
		if (tp->status != TR_WAIT)
			continue;
		np = new node;
		np->tid = tp->tid;
		np->sgno = tp->sgno;
		np->obno = tp->obno;
		np->lockmode = tp->lockmode;
		np->semno = tp->semno;
		np->next = (wtable != NULL) ? wtable : NULL;
		np->parent = NULL;
		np->next_s = NULL;
		np->level = 0;
		wtable = np;
	}
	if (zgt_v(SHARED_MEM_AVAIL) < 0) {
		Zgt_errno = ZGT_SEMA_V_OP_FAILS;
	};
}

int wait_for::deadlock() {
	node* np;
	zgt_tx *tp;
	if (zgt_p(SHARED_MEM_AVAIL) < 0) {
		Zgt_errno = ZGT_SEMA_P_OP_FAILS;
		return (-1);
	};
	for (np = wtable; np != NULL; np = np->next) {
		if (np->level == 0) {
			head = new node;
			head->tid = np->tid;
			head->sgno = np->sgno;
			head->obno = np->obno;
			head->lockmode = np->lockmode;
			head->semno = np->semno;
			head->next = NULL;
			head->parent = NULL;
			head->next_s = NULL;
			head->level = 1;

			found = FALSE;
			traverse(head);
			if ((found == TRUE) && (victim != NULL)) {

				for (tp = (zgt_tx *) ZGT_Sh->lastr; tp != NULL; tp = tp->nextr) {
					if (tp->tid == victim->tid)
						break;
				}
				tp->status = TR_ABORT;
				if (zgt_v(SHARED_MEM_AVAIL) < 0) {
					Zgt_errno = ZGT_SEMA_V_OP_FAILS;
					return (-1);
				};
				tp->end_tx();
				if (zgt_p(SHARED_MEM_AVAIL) < 0) {
					Zgt_errno = ZGT_SEMA_P_OP_FAILS;
					return (-1);
				};
				// the v operation may not free the intended process
				// one needs to be more careful here
				if (zgt_v(victim->semno) < 0) {
					Zgt_errno = ZGT_SEMA_V_OP_FAILS;
					return (-1);
				};
				if (victim->lockmode == 'X') {
					//free the semaphore
				}
				// should free all locks assoc. with tid here
				continue;
			}
		}
	}
	if (zgt_v(SHARED_MEM_AVAIL) < 0) {
		Zgt_errno = ZGT_SEMA_V_OP_FAILS;
		return (-1);
	};
	return (0);
}

int wait_for::traverse(node* p) {
	node *q, *last = NULL;
	zgt_tx *tp;
	zgt_hlink *sp;

	// find thw owners of the lock being waited on

	for (sp = ZGT_Ht->find(p->sgno, p->obno); sp != NULL;) {
		// has tid been visited before?
		if (visited(sp->tid) == TRUE) {
			if ((q = location(sp->tid)) != NULL) {
				// is tid one of the ancestor?
				if (q->level > 0) {
					// found a cycle
					victim = choose_victim(p, q);
					found = TRUE;
					return (0);
				}
				// it has an uncle with the same tid.  omit the uncle
				else if (q->level == 0)
					q->level = -1;
				// tid already been processed
				else
					continue;
			}
		}
		q = new node;
		q->next_s = (last == NULL) ? NULL : last;
		q->next = (head == NULL) ? NULL : head;

		last = q;
		head = q;
		q->tid = sp->tid;
		q->sgno = sp->sgno;
		q->obno = sp->obno;
		q->lockmode = sp->lockmode;

		// mark on the wtable
		for (q = wtable; q != NULL; q = q->next)
			if (q->tid == sp->tid)
				q->level = 1;
		while (sp = sp->next) {
			if ((sp->sgno == p->sgno) && (sp->obno == p->obno))
				break;
		}

	}

	if (last == NULL)
		return (0);
	for (q = last; q != NULL;) {
		if (q->level >= 0) {

			for (tp = (zgt_tx *) ZGT_Sh->lastr; tp != NULL; tp = tp->nextr) {
				if (tp->tid == q->tid)
					break;
			}
			if (tp->status == TR_WAIT) {
				q->sgno = tp->sgno;
				q->obno = tp->obno;
				q->lockmode = tp->lockmode;
				q->semno = tp->semno;
				q->parent = p;
				q->level = p->level + 1;
				traverse(q);
				if (found == TRUE)
					return (0);
			}
		}
		delete last;
		last = q->next_s;
		q = last;
	}
}
;

int wait_for::visited(long t) {
	node* n;
	for (n = head; n != NULL; n = n->next)
		if (t == n->tid)
			return (TRUE);

	return (FALSE);
}
;

node * wait_for::location(long t) {
	node* n;
	for (n = head; n != NULL; n = n->next)
		if ((t == n->tid) && (n->level >= 0))
			return (n);
	return (NULL);
}

node* wait_for::choose_victim(node* p, node* q) {
	node* n;
	int total;
	zgt_tx *tp;
	//for (n=p; n != q->parent; n=n->parent) {
	//	printf("%d  ",n->tid);
	//}
	//printf("\n");
	for (n = p; n != q->parent; n = n->parent) {
		if (n->lockmode == 'X')
			return (n);
		else {
			for (total = 0, tp = (zgt_tx *) ZGT_Sh->lastr; tp !=
			NULL; tp = tp->nextr) {
				if (tp->semno == n->semno)
					total++;
			}

			if (total == 1)
				return (n);
		}
		// for the moment, we need to show that there is at least one
		// candidate process that is associated with a semaphore waited by
		// exactly one process.  The code has to be modified if we can't 
		// show this.
	}
	return (NULL);
}
