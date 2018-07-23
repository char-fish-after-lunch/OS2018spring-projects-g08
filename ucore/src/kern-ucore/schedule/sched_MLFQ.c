#include <types.h>
#include <list.h>
#include <proc.h>
#include <assert.h>
#include <sched.h>
#include <sched_RR.h>
#include <sched_MLFQ.h>

static struct sched_class *sched_class;

static void MLFQ_init(struct run_queue *rq)
{
	sched_class = &RR_sched_class;
	list_entry_t *list = &(rq->rq_link), *le = list;
	do {
		sched_class->init(le2rq(le, rq_link));
		le = list_next(le);
	} while (le != list);
}

extern struct cpu cpus[];

int findRQ(struct run_queue *rq) {
	for (int i = 0; i < NCPU; i++) {
		if (rq == &(cpus[i].rqueue)) return i;
	}
	panic("rq is not in cpus.rqueue\n");
}

static void MLFQ_enqueue(struct run_queue *rq, struct proc_struct *proc)
{
	assert(list_empty(&(proc->run_link)));
	// struct run_queue *nrq = rq;
	// if (proc->rq != NULL && proc->time_slice == 0) {
	// 	nrq = le2rq(list_next(&(proc->rq->rq_link)), rq_link);
	// 	if (nrq == rq) {
	// 		nrq = proc->rq;
	// 	}
	// }
	// sched_class->enqueue(nrq, proc);
	sched_class->enqueue(rq, proc);
}

static void MLFQ_dequeue(struct run_queue *rq, struct proc_struct *proc)
{
	assert(!list_empty(&(proc->run_link)) && rq == proc->rq);
	sched_class->dequeue(proc->rq, proc);
}

static struct proc_struct *MLFQ_pick_next(struct run_queue *rq)
{
	struct proc_struct *next;
	list_entry_t *list = &(rq->rq_link), *le = list;
	do {
		if ((next = sched_class->pick_next(le2rq(le, rq_link))) != NULL) {
			break;
		}
		le = list_next(le);
	} while (le != list);
	return next;
}

static void MLFQ_proc_tick(struct run_queue *rq, struct proc_struct *proc)
{
	sched_class->proc_tick(proc->rq, proc);
}

#ifndef ARCH_SOC
//TODO: use MLFQ
static double MLFQ_get_load (struct run_queue * rq) {
    return sched_class->get_load(rq);
}
#endif

//TODO: use MLFQ
static int MLFQ_get_proc(struct run_queue* rq, struct proc_struct* procs_moved[], int needs) {
    return sched_class->get_proc(rq, procs_moved, needs);
}

struct sched_class MLFQ_sched_class = {
	.name = "MLFQ_scheduler",
	.init = MLFQ_init,
	.enqueue = MLFQ_enqueue,
	.dequeue = MLFQ_dequeue,
	.pick_next = MLFQ_pick_next,
	.proc_tick = MLFQ_proc_tick,
#ifndef ARCH_SOC
    .get_load = MLFQ_get_load,
#endif
    .get_proc = MLFQ_get_proc,
};
