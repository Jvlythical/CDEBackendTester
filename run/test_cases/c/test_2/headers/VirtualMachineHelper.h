#ifndef VIRTUAL_MACHINE_HELPER_H

	#define VIRTUAL_MACHINE_HELPER_H	
	
	extern queue_ar high_q;
	extern queue_ar mid_q;
	extern queue_ar low_q;
	
	extern unsigned int thread_ar_size;
	extern TCB *thread_ar[8];

	extern unsigned int sleep_ar_size;
	extern TCB* sleep_ar[8];

	extern TVMThreadID active[8];

	extern unsigned int mutex_ar_size;
	extern Mutex *mutex_ar[8];

	extern void schedule_next();
	extern void schedule();

	extern TCB* idle_thread;
	extern TVMThreadID idle_thread_id;

	extern int debug_flag;

	void printTCB(TCB *);
	void schedule_thread(TCB *);

/* ------------------------------
 *   For debugger purposes
 *   	~ Used as a breakpoint
 * ------------------------------
 */

	void marker() {
		int i = 0;

		// So gcc doesn't complain 
		i += 1;
	}

/* -----------------------------------------
 *   Enqueues thread based on priority
 *   	~ Sort based on high, mid, or low 
 * -----------------------------------------
 */

	void enqueue_thread(TCB *t_ptr) {

		if(t_ptr->state != VM_THREAD_STATE_READY) 
			return;

		switch(t_ptr->prio) {
			case VM_THREAD_PRIORITY_HIGH:
				tq_enqueue(*(t_ptr->tid), &high_q);
				break;
			case VM_THREAD_PRIORITY_NORMAL:
				tq_enqueue(*(t_ptr->tid), &mid_q);
				break;
			case VM_THREAD_PRIORITY_LOW:
				tq_enqueue(*(t_ptr->tid), &low_q);
				break;
		}

	}

/* -------------------------------------------
 *   Dequeues thread based on priority
 *   	~ Removes based on high, mid or low
 * -------------------------------------------
 */

	void dequeue_thread(TCB *t_ptr) {

		switch(t_ptr->prio) {
			case VM_THREAD_PRIORITY_HIGH:
				tq_dequeue(&high_q);
				break;
			case VM_THREAD_PRIORITY_NORMAL:
				tq_dequeue(&mid_q);
				break;
			case VM_THREAD_PRIORITY_LOW:
				tq_dequeue(&low_q);
				break;
		}
		
	}

// Deprecated, here for reference
	void alarm_callback(void *param) {
		int *tick = (int *) param;

		printf("%d", thread_ar_size);

		if(*tick != 0)
			(*tick)--;
	}

	int tryGrabMutex(TCB *t_ptr) {
		
		if(emptyInt(&t_ptr->m_wish_list))
			return 1;

		IntNode *head = t_ptr->m_wish_list.head;
		
		while(head != NULL) {

			int status = VMMutexAcquire(head->val, VM_TIMEOUT_IMMEDIATE);

			if(status == VM_STATUS_FAILURE) {
				
				if(debug_flag)
					printf("*** Thread %d failed to grab mutex %d\n", *(t_ptr->tid), head->val);

				VMThreadSleep(VM_TIMEOUT_IMMEDIATE);

				return 0;
			}

			head = head->next;

		}

		return 1;
	}

	void waitForMutex(TCB *t_ptr) {
		while(!tryGrabMutex(t_ptr)) {
			//waitForMutex(t_ptr);
			schedule_next();
		}
	}
	
/* -------------------------------------------------------
 *   Callback for every clock tick
 *   	~ Decrement sleep list ticks
 *   	~ Stick threads back into ready queue after nap
 * -------------------------------------------------------
 */

	void alarm_callback_test(void *param) {
		
		TCB **sleep_ar2 = (TCB **) param;

		TMachineSignalState OldState;
		MachineSuspendSignals(&OldState);
		
		int i = 0;
		
		for(; i < sleep_ar_size; ++i) {
			
			// Verify
			if(sleep_ar2[i] == NULL)
				continue;
			
		 	// Debug (If you want to see tick countdown)
		 	//printf("%d\n", sleep_ar[i]->tick);

			// Begin
			if(sleep_ar2[i]->tick != 0 )	{

				//printf("Thread %d is sleeping...\n", *(sleep_ar2[i]->tid));
				sleep_ar2[i]->tick--;

			} else {

				if(debug_flag)
					printf("*** Thread %d has awaken from its lovely nap\n", *(sleep_ar2[i]->tid));

				sleep_ar2[i]->state = VM_THREAD_STATE_READY;
				enqueue_thread(sleep_ar2[i]);
				sleep_ar2[i] = NULL;

			}

		}

		MachineResumeSignals(&OldState);
		
		schedule();

	}

	

/* -------------------------------------------
 *   Wrapper for entry function
 *   	~ Ensures termination after running
 * -------------------------------------------
 */

	void skeleton_entry(void *param) {

		TCB *t_ptr = getThread(active[0], thread_ar, thread_ar_size);

		if(t_ptr == NULL)
			t_ptr = idle_thread;
		
		MachineEnableSignals();
		t_ptr->entry(param);
		VMThreadTerminate(*(t_ptr->tid));

	}

/* -----------------------------------------------
 *   Idle thread
 *   	~ Print something to confirm it's there
 * -----------------------------------------------
 */

	void idle_entry(void *param) {
		int i = 0;

		while(1)
			i++;
	}

/* ---------------------------------------------------------
 *   Handles bunch of initializations
 *   	~ Initializes thread list
 *   	~ Initializes mutex list
 *   	~ Initializes priority queues (not the heap kind)
 *   	~ Creates idle thread
 * ---------------------------------------------------------
 */

	void initVM() {
		
		// Initializations
		initThreadAr(thread_ar, thread_ar_size);
		initThreadAr(sleep_ar, sleep_ar_size);

		initMutexAr(mutex_ar, mutex_ar_size);

		tq_init(&high_q, 50);
		tq_init(&mid_q, 50);
		tq_init(&low_q, 50);
		
		// Create idle thread 
		TMachineSignalState OldState;
		MachineSuspendSignals(&OldState);

			// Create TCB
			TCB *tcb = malloc(sizeof(TCB));
			TVMMemorySize memsize = 0x100000;
			TVMThreadEntry entry = (TVMThreadEntry) idle_entry;

			// Initialize TCB
			tcb->tid = malloc(sizeof(TVMThreadID));
			*(tcb->tid) = idle_thread_id;
			tcb->state = VM_THREAD_STATE_READY;
			tcb->prio = VM_THREAD_PRIORITY_LOW;
			tcb->sp = malloc(memsize);
			tcb->stack_size = memsize;
			tcb->entry = entry;
			tcb->param = NULL;
			tcb->smc = malloc(sizeof(SMachineContext));
			MachineContextCreate(tcb->smc, skeleton_entry, tcb->param, tcb->sp, tcb->stack_size);

			idle_thread = tcb;

		MachineResumeSignals(&OldState);
	}

/* --------------------------------------
 *   Free all mutexes held by thread
 *   	~ Iterate through held list
 *   	~ Set owner to NULL
 * --------------------------------------  	
 */

	void freeMutexes(TVMThreadID thread) {
		TCB *t_ptr = getThread(thread, thread_ar, thread_ar_size);

		if(t_ptr == NULL)
			return;

		IntList *l = &t_ptr->m_held_list;
		IntNode *head = l->head;

		while(head != NULL) {
			
			Mutex *m_ptr = getMutex(head->val, mutex_ar, mutex_ar_size);

			if(m_ptr == NULL)
				continue;
			else
				m_ptr->owner = NULL;

			head = head->next;

		}

	}

	void printTCB(TCB *tcb) {
	
		puts("========================");

		printf("TID: %d\n", *(tcb->tid));

		switch(tcb->state) {
			case VM_THREAD_STATE_DEAD:
				printf("STATE: dead\n");
				break;
			case VM_THREAD_STATE_WAITING:
				printf("STATE: waiting\n");
				break;
			case VM_THREAD_STATE_READY:
				printf("STATE: waiting\n");
				break;
			case VM_THREAD_STATE_RUNNING:
				printf("STATE: running\n");
				break;
			default:
				printf("STATE: NULL\n");
		}

		switch(tcb->prio) {
			case VM_THREAD_PRIORITY_LOW:
				printf("PRIO: low\n");
				break;
			case VM_THREAD_PRIORITY_NORMAL:
				printf("PRIO: mid\n");
				break;
			case VM_THREAD_PRIORITY_HIGH:
				printf("PRIO: high\n");
				break;
			default:
				printf("PRIO NULL\n");
		}

		printf("MUTEXES:\n");

		IntNode *head = tcb->m_held_list.head;
		
		while(head != NULL) {
			printf("%d ", head->val);
			head = head->next;
		}

		puts("");

		puts("========================");
	}

	int reassignMutex(Mutex *m) {
		queue_ar *q = m->wait_list;
		int t_id = (int) tq_peek(q);

		if(tq_empty(q))	
			return 0;

		TCB *t_ptr = getThread(t_id, thread_ar, thread_ar_size);

		if(debug_flag)
			printf("*** Thread %d has grabbed mutex %d\n", t_id, m->m_id);

		t_ptr->state = VM_THREAD_STATE_READY;
		deleteInt(m->m_id, &t_ptr->m_wish_list);
		m->owner = t_ptr->tid;
		enqueue_thread(t_ptr);
		
		return 1;
	}

/* ----------------------------------------------
 *   Machine file callback
 *   	~ Stashes result in f_result
 *   	~ Forces thread scheduling on callback
 * ----------------------------------------------
 */

	void m_file_callback(void *calldata, int result) {

		TCB *t_ptr = (TCB *) calldata;

		t_ptr->state = VM_THREAD_STATE_READY;
		t_ptr->f_result = result;

	}

#endif
