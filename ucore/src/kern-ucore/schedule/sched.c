#include <list.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <stdio.h>
#include <assert.h>
#include <sched_RR.h>
#include <sched_MLFQ.h>
#include <sched_mpRR.h>
#include <kio.h>
#if defined(ARCH_RISCV64) || defined(ARCH_SOC)
#include <smp.h>
#else
#include <mp.h>
#endif
#include <trap.h>
#if !defined(ARCH_RISCV64) && !defined(ARCH_SOC)
#include <sysconf.h>
#endif
#include <spinlock.h>
extern struct cpu cpus[];

static spinlock_s stupid_lock;
static struct sched_class *sched_class;
// static DEFINE_PERCPU_NOINIT(struct run_queue, runqueues);

//static struct run_queue *rq;

static const int MAX_MOVE_PROC_NUM = 100;

static inline void move_run_queue(int src_cpu_id, int dst_cpu_id, struct proc_struct *proc) {
    struct run_queue *s_rq = &cpus[src_cpu_id].rqueue;
    struct run_queue *d_rq = &cpus[dst_cpu_id].rqueue;
    sched_class->dequeue(s_rq, proc);

    proc->cpu_affinity = dst_cpu_id; 
    sched_class->enqueue(d_rq, proc);
}

static inline int min(int a, int b) {
    if (a < b) return a;
    else return b;
}

static inline void load_balance()
{
    for (int i = 0; i < NCPU; ++i) {
        spinlock_acquire(&cpus[i].rqueue_lock);
    }
	#if defined(ARCH_RISCV64) || defined(ARCH_SOC)
	int lcpu_count = NCPU;
	#else
	int lcpu_count = sysconf.lcpu_count;
	#endif
    int load_sum = 0, load_max = -1, my_load = 0;
    int max_id = 0;
    
   
    for (int i = 0; i < lcpu_count; ++i) {
        int load = (&(cpus[i].rqueue)) -> proc_num;
        load_sum += load;
        if (load > load_max) {
            load_max = load;
            max_id = i;
        }
    }
    load_sum /= lcpu_count;
    
    {
        load_max = (&cpus[max_id].rqueue) -> proc_num;
        my_load = (&cpus[myid()].rqueue) -> proc_num;
        int needs = min((int)(load_max - load_sum), (int)(load_sum - my_load));
        needs = min(needs, MAX_MOVE_PROC_NUM);
        if (needs > 3 && myid() != max_id) {
            kprintf("===========%d %d %d======\n", myid(), max_id, needs);
            struct proc_struct* procs_moved[MAX_MOVE_PROC_NUM];//TODO: max proc num in rq
            int num = sched_class->get_proc(&cpus[max_id].rqueue, procs_moved, needs);
            for (int i = 0; i < num; ++i) {
				assert(procs_moved[i]->pid >= NCPU);
				move_run_queue(max_id, myid(), procs_moved[i]);
			}
        }
    }
    for (int i = NCPU - 1; i >= 0; i--) {
        spinlock_release(&cpus[i].rqueue_lock);
    }
}

extern findRQ(struct run_queue *rq);

static inline void sched_class_enqueue(struct proc_struct *proc)
{
	if (proc != idleproc) {
		struct run_queue *rq = &mycpu()->rqueue;
		if(proc->flags & PF_PINCPU){
			#if !defined(ARCH_RISCV64) && !defined(ARCH_SOC)
			assert(proc->cpu_affinity >= 0 
					&& proc->cpu_affinity < sysconf.lcpu_count);
			#else
			assert(proc->cpu_affinity >= 0 
					&& proc->cpu_affinity < NCPU);
			#endif
			rq = &cpus[proc->cpu_affinity].rqueue;
		}
        assert(proc->cpu_affinity == myid());

        spinlock_acquire(&mycpu()->rqueue_lock);
		sched_class->enqueue(rq, proc);
        spinlock_release(&mycpu()->rqueue_lock);

	}
}

static inline void sched_class_enqueue_after_wakeup(struct proc_struct *proc)
{
	if (proc != idleproc) {
		struct run_queue *rq = proc->rq;
		if(proc->flags & PF_PINCPU){
			#if !defined(ARCH_RISCV64) && !defined(ARCH_SOC)
			assert(proc->cpu_affinity >= 0 
					&& proc->cpu_affinity < sysconf.lcpu_count);
			#else
			assert(proc->cpu_affinity >= 0 
					&& proc->cpu_affinity < NCPU);
			#endif
		}
		if (rq == NULL) {
			rq = &cpus[proc->cpu_affinity].rqueue;
			proc->cpu_affinity = myid();			
			assert(proc->cpu_affinity == myid());
		} else {
			proc->cpu_affinity = findRQ(rq);						
			if (proc->cpu_affinity != findRQ(rq)) {
				kprintf("proc->cpu_affinity: %d\n", proc->cpu_affinity);
				kprintf("rq: %d\n", findRQ(rq));
				kprintf("proc->pid: %d\n", proc->pid);
			}
        	assert(proc->cpu_affinity == findRQ(rq));
		}

        spinlock_acquire(&(cpus[findRQ(rq)]).rqueue_lock);
		sched_class->enqueue(rq, proc);
        spinlock_release(&(cpus[findRQ(rq)]).rqueue_lock);

	}
}

static inline void sched_class_dequeue(struct proc_struct *proc)
{
	struct run_queue *rq = &mycpu()->rqueue;
	sched_class->dequeue(rq, proc);
}

static inline struct proc_struct *sched_class_pick_next(void)
{
	struct run_queue *rq = &mycpu()->rqueue;
	struct proc_struct* ans = sched_class->pick_next(rq);
    return ans;
}

static void sched_class_proc_tick(struct proc_struct *proc)
{
	spinlock_acquire(&stupid_lock);
	if (proc != idleproc) {
		struct run_queue *rq = &mycpu()->rqueue;
	    
        spinlock_acquire(&mycpu()->rqueue_lock);
		sched_class->proc_tick(rq, proc);
        spinlock_release(&mycpu()->rqueue_lock);
	} else {
		proc->need_resched = 1;
	}
	spinlock_release(&stupid_lock);
}

//static struct run_queue __rq[NCPU];

void sched_init(void)
{
	int id = myid();
	//rq = __rq;
	//list_init(&(__rq[0].rq_link));
	struct run_queue *rq0 = &cpus[id].rqueue;
	list_init(&(rq0->rq_link));
    list_init(&(cpus[id].timer_list.tl));
    spinlock_init(&cpus[id].rqueue_lock);
	spinlock_init(&stupid_lock);
	rq0->max_time_slice = 8;

	int i;
	for (i = 0; i < NCPU; i++) {//TODO: use NCPU in riscv only
		if(i == id)
			continue;
		struct run_queue *rqi = &cpus[i].rqueue;
		list_init(&(rqi->rq_link));
        list_init(&(cpus[i].timer_list.tl));
        spinlock_init(&cpus[i].rqueue_lock);
		// list_add_before(&(rq0->rq_link), 
		// 		&(rqi->rq_link));
		// yzjc: I don't know what you are doing here
		// MLFQ does not want this
		rqi->max_time_slice = rq0->max_time_slice;
	}

#ifdef UCONFIG_SCHEDULER_MLFQ
	sched_class = &MLFQ_sched_class;
#elif defined UCONFIG_SCHEDULER_RR
	sched_class = &RR_sched_class;
#else
	sched_class = &MPRR_sched_class;
#endif
	for (i = 0; i < NCPU; i++) {
		struct run_queue *rqi = &cpus[i].rqueue;
		sched_class->init(rqi);
	}

	kprintf("sched class: %s\n", sched_class->name);
}

void stop_proc(struct proc_struct *proc, uint32_t wait)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	spinlock_acquire(&stupid_lock);
	proc->state = PROC_SLEEPING;
	proc->wait_state = wait;
	spinlock_acquire(&mycpu()->rqueue_lock);
	if (!list_empty(&(proc->run_link))) {
		sched_class_dequeue(proc);
	}
	spinlock_release(&mycpu()->rqueue_lock);
	spinlock_acquire(&stupid_lock);
	local_intr_restore(intr_flag);
}

void wakeup_proc(struct proc_struct *proc)
{
	spinlock_acquire(&proc->lock);
	assert(proc->state != PROC_ZOMBIE);
	bool intr_flag;
	local_intr_save(intr_flag);
	spinlock_acquire(&stupid_lock);
	{
		if (proc->state != PROC_RUNNABLE) {
			// kprintf("W1 %d\n", proc->pid);
			proc->state = PROC_RUNNABLE;
			proc->wait_state = 0;
			if (proc != current) {
				#if defined(ARCH_RISCV64) || defined(ARCH_SOC)
				assert(proc->pid >= NCPU);
				#else
				assert(proc->pid >= sysconf.lcpu_count);
				#endif
				sched_class_enqueue_after_wakeup(proc);
			}
		} else {
			warn("wakeup runnable process.\n");
		}
	}
	spinlock_release(&stupid_lock);
	local_intr_restore(intr_flag);
	spinlock_release(&proc->lock);
}

int try_to_wakeup(struct proc_struct *proc)
{
	spinlock_acquire(&proc->lock);
	assert(proc->state != PROC_ZOMBIE);
	int ret;
	bool intr_flag;
	local_intr_save(intr_flag);
	spinlock_acquire(&stupid_lock);
	{
		if (proc->state != PROC_RUNNABLE) {
			proc->state = PROC_RUNNABLE;
			proc->wait_state = 0;
			if (proc != current) {
				proc->cpu_affinity = myid();
				sched_class_enqueue_after_wakeup(proc);
			}
			ret = 1;
		} else {
			ret = 0;
		}
		struct proc_struct *next = proc;
		while ((next = next_thread(next)) != proc) {
			if (next->state == PROC_SLEEPING
			    && next->wait_state == WT_SIGNAL) {
				next->state = PROC_RUNNABLE;
				next->wait_state = 0;
				if (next != current) {
					next->cpu_affinity = myid();
					sched_class_enqueue_after_wakeup(next);
				}
			}
		}
	}
	spinlock_release(&stupid_lock);
	local_intr_restore(intr_flag);
	spinlock_release(&proc->lock);
	return ret;
}

#include <vmm.h>

void schedule(void)
{
	kprintf("Scheduling! for cpu %d\n", myid());
	/* schedule in irq ctx is not allowed */
	assert(!ucore_in_interrupt());
	bool intr_flag;
	struct proc_struct *next;

	local_intr_save(intr_flag);
	spinlock_acquire(&stupid_lock);

	#if defined(ARCH_RISCV64) || defined(ARCH_SOC)
	int lcpu_count = NCPU;
	#else
	int lcpu_count = sysconf.lcpu_count;
	#endif
	{
		current->need_resched = 0;
		load_balance();

        spinlock_acquire(&mycpu()->rqueue_lock);
        {
            next = sched_class_pick_next();
            if (next != NULL){
                sched_class_dequeue(next);
            }
            else if(current->state == PROC_RUNNABLE)
				next = current;
			else
				next = idleproc;

        }
        spinlock_release(&mycpu()->rqueue_lock);
		
        next->runs++;
		spinlock_release(&stupid_lock);
		if (next != current){
			kprintf("CPUid: %d, PID: %d\n", myid(), next->pid);
			// kprintf("%s, %d\n", next->name, next->pid);
			proc_run(next);
		}
	}
	local_intr_restore(intr_flag);
}

static void __add_timer(timer_t * timer, int cpu_id)
{
	assert(timer->expires > 0 && timer->proc != NULL);
	assert(list_empty(&(timer->timer_link)));
	list_entry_t *le = list_next(&cpus[cpu_id].timer_list.tl);
	while (le != &cpus[cpu_id].timer_list.tl) {
		timer_t *next = le2timer(le, timer_link);
		if (timer->expires < next->expires) {
			next->expires -= timer->expires;
			break;
		}
		timer->expires -= next->expires;
		le = list_next(le);
	}
	list_add_before(le, &(timer->timer_link));
}

void add_timer(timer_t * timer)
{
	bool intr_flag;
	spin_lock_irqsave(&mycpu()->timer_list.lock, intr_flag);
	{
		__add_timer(timer, myid());
	}
	spin_unlock_irqrestore(&mycpu()->timer_list.lock, intr_flag);
}

static void __del_timer(timer_t * timer, int cpu_id)
{
	if (!list_empty(&(timer->timer_link))) {
		if (timer->expires != 0) {
			list_entry_t *le =
				list_next(&(timer->timer_link));
			if (le != &cpus[cpu_id].timer_list.tl) {
				timer_t *next =
					le2timer(le, timer_link);
				next->expires += timer->expires;
			}
		}
		list_del_init(&(timer->timer_link));
	}
}

void del_timer(timer_t * timer)
{

	bool intr_flag;
	spin_lock_irqsave(&mycpu()->timer_list.lock, intr_flag);
	{
		__del_timer(timer, myid());
	}
	spin_unlock_irqrestore(&mycpu()->timer_list.lock, intr_flag);
}

void run_timer_list(void)
{
	bool intr_flag;
	spin_lock_irqsave(&mycpu()->timer_list.lock, intr_flag);
	{
		list_entry_t *le = list_next(&mycpu()->timer_list.tl);
		if (le != &mycpu()->timer_list.tl) {
			timer_t *timer = le2timer(le, timer_link);
			assert(timer->expires != 0);
			timer->expires--;
			while (timer->expires == 0) {
				le = list_next(le);
				if (__ucore_is_linux_timer(timer)) {
					struct __ucore_linux_timer *lt =
					    &(timer->linux_timer);

					spin_unlock_irqrestore(&mycpu()->timer_list.lock, intr_flag);
					if (lt->function)
						(lt->function) (lt->data);
					spin_lock_irqsave(&mycpu()->timer_list.lock, intr_flag);

					__del_timer(timer, timer->proc->cpu_affinity);
					kfree(timer);
					continue;
				}
				struct proc_struct *proc = timer->proc;
				if (proc->wait_state != 0) {
					assert(proc->wait_state &
					       WT_INTERRUPTED);
				} else {
					warn("process %d's wait_state == 0.\n",
					     proc->pid);
				}

				wakeup_proc(proc);

				__del_timer(timer, timer->proc->cpu_affinity);
				if (le == &mycpu()->timer_list.tl) {
					break;
				}
				timer = le2timer(le, timer_link);
			}
		}
		sched_class_proc_tick(current);
	}
	spin_unlock_irqrestore(&mycpu()->timer_list.lock, intr_flag);
}

void post_switch(void)
{
    struct proc_struct* prev = mycpu()->prev;
    spinlock_acquire(&prev->lock);
    if (prev->state == PROC_RUNNABLE && prev->pid >= NCPU && list_empty(&prev->run_link))
    {
        prev->cpu_affinity = myid();
		sched_class_enqueue(prev);
    }
    spinlock_release(&prev->lock);
}