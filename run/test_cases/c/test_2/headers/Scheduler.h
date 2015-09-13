#ifndef SCHEDULER_H
	#define SCHEDULER_H
	
	extern queue_ar high_q;
	extern queue_ar mid_q;
	extern queue_ar low_q;

	extern unsigned int thread_ar_size;
	extern TCB *thread_ar[8];
	extern TVMThreadID active[8];

	extern TCB* idle_thread;

	extern int debug_flag;


/* ---------------------------------------------
 *   Runs a thread   
 *   	~ Set running to ready state
 *   	~ Place it back into ready queue
 *   	~ Set new thread as active
 *   	~ Set new thread to running
 *   	~ Switch context
 * ---------------------------------------------
 */

	void run_thread(TCB *cur, TCB *next) {

		if(cur == NULL)
			cur = idle_thread;
		
		// Threads should not be the same
		if(cur == next)
			return;
		
		// Next thread should not be in waiting state
		if( next->state == VM_THREAD_STATE_WAITING )
			return;

		if(debug_flag)
			printf("~~~ Switching from %d to %d\n", *(cur->tid), *(next->tid));

		// Move from running state to ready
		if(cur->state == VM_THREAD_STATE_RUNNING) {
			cur->state = VM_THREAD_STATE_READY;
			
			if(*(cur->tid) != idle_thread_id)
				enqueue_thread(cur);
		}
		
		// Remove thread from ready queue
		dequeue_thread(next);
		
		// Set active thread 
		active[0] = *(next->tid);
		next->state = VM_THREAD_STATE_RUNNING;

		// Switch thread context
		MachineContextSwitch(cur->smc, next->smc);

	}

/* ----------------------------------------------------
 *   Returns TCB with the highest priority
 *   	~ Checks ready queues for next thread
 *   	~ Returns idle thread if no thread is found
 * ----------------------------------------------------
 */

	TCB* getHighestPrio() {
		
		TVMThreadID thread = 0;
		
		// Try to get a thread
		if(!tq_empty(&high_q)) 
			thread = tq_peek(&high_q);
		else if (!tq_empty(&mid_q))
			thread = tq_peek(&mid_q);
		else if (!tq_empty(&low_q))
			thread = tq_peek(&low_q);
		
		TCB *next_ptr = getThread(thread, thread_ar, thread_ar_size);
		
		// If no more threads
		if(next_ptr == NULL) {
			
			if(debug_flag)
				printf("*** No more threads, returning idle\n");

			return idle_thread;
		}

		return next_ptr;
	}


/* -------------------------------------------------------
 *   Schedule based on highest priority
 *   	~ Get highest priority thread
 *   	~ Compare the thread to current thread
 * -------------------------------------------------------
 */

	void schedule() {
		
		TCB *cur_ptr = getThread(active[0], thread_ar, thread_ar_size);
		TCB *next_ptr = getHighestPrio();

		// If cur thread is idle, switch
		if(active[0] == idle_thread_id && next_ptr != NULL) {
			run_thread(idle_thread, next_ptr);
			return;
		}
		
		// Else check priority
		int lower_prio = (cur_ptr->prio < next_ptr->prio);
		int running = (cur_ptr->state == VM_THREAD_STATE_RUNNING);

		if(lower_prio || !running) 
			run_thread(cur_ptr, next_ptr);

	}

	void schedule_higher() {

		TCB *cur_ptr = getThread(active[0], thread_ar, thread_ar_size);
		TCB *next_ptr = getHighestPrio();

		int lower_prio = (cur_ptr->prio < next_ptr->prio);

		if(lower_prio) 
			run_thread(cur_ptr, next_ptr);

	}

// Forces a new thread to be scheduled
	void schedule_next() {
		TCB *cur_ptr = getThread(active[0], thread_ar, thread_ar_size);
		TCB *next_ptr = getHighestPrio();

		run_thread(cur_ptr, next_ptr);
	}

// Forces a specific thread to be scheduled
	void schedule_thread(TCB *t_ptr) {
		
		if(t_ptr == NULL)
			return;

		TCB *cur_ptr = getThread(active[0], thread_ar, thread_ar_size);

		run_thread(cur_ptr, t_ptr);

	}



#endif
