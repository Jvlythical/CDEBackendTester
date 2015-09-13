#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

// Provided by professor
#include "VirtualMachine.h"
#include "Machine.h"

extern TVMMainEntry VMLoadModule(const char *);
extern  void VMUnloadModule(void);

// Added by us
#include "headers/IntList.h"
#include "headers/TCB.h"
#include "headers/ThreadQueue.h"
#include "headers/ThreadList.h"

#include "headers/Mutex.h"
#include "headers/MutexList.h"

#include "headers/VirtualMachineHelper.h"
#include "headers/Scheduler.h"

// Stores thread information
unsigned int t_primary_key = 7;
unsigned int thread_ar_size = 8;
TCB *thread_ar[8];

// Keep track of thread states
TVMThreadID active[8];
unsigned int sleep_ar_size = 8;
TCB *sleep_ar[8];

// Queue of priorities
queue_ar high_q;
queue_ar mid_q;
queue_ar low_q;

// Stores mutex information
unsigned int m_primary_key = 7;
unsigned int mutex_ar_size = 8;
Mutex *mutex_ar[8];

// Idle thread
TVMThreadID idle_thread_id = 3;
TCB* idle_thread;

int debug_flag = 0;

 /* ------------------------------------------------ 
 	*  Starts the VM
 	*  	Order recommended by the professor
	*  	1. VMLoadModule
	*  	2. MachineInitialize
	*  	3. MachineRequestAlarm
	*  	4. MachineEnableSignals
	*  	5. VMMain (whatever VMLoadModule returned)
	* -------------------------------------------------
	*/

TVMStatus VMStart(int tickms, int machinetickms, int argc, char *argv[])
{

	int success = 0;

	initVM();	

	TVMMainEntry e = VMLoadModule(argv[0]);
	MachineInitialize(machinetickms );

	TVMThreadID tid = t_primary_key++;

	// Create TCB
	TCB *tcb = malloc(sizeof(TCB));

	// Initialize TCB
	tcb->tid = &tid;
	tcb->state = VM_THREAD_STATE_RUNNING;
	tcb->prio = VM_THREAD_PRIORITY_LOW;
	tcb->smc = malloc(sizeof(SMachineContext));
	
	active[0] = tid;
	insertThread(tcb, thread_ar, thread_ar_size);
	
	if(e != NULL) {
		
		MachineRequestAlarm(tickms * 1000, alarm_callback_test, sleep_ar);
		MachineEnableSignals();
		
		e(argc, argv);
		success = 1;

	} 
		
	VMUnloadModule();
	MachineTerminate();

	if(success)
		return VM_STATUS_SUCCESS;
	else
		return VM_STATUS_FAILURE;

};

TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid)
{
		
	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);
		
		// Verify
		if(entry == NULL || tid == NULL)
			return VM_STATUS_ERROR_INVALID_PARAMETER;

		// Begin
		*tid = t_primary_key++;
		
		// Create TCB
		TCB *tcb = malloc(sizeof(TCB));

		// Initialize TCB
		tcb->tid = tid;
		tcb->state = VM_THREAD_STATE_DEAD;
		tcb->prio = prio;
		tcb->sp = malloc(memsize);
		tcb->stack_size = memsize;
		tcb->entry = entry;
		tcb->param = param;
		tcb->smc = malloc(sizeof(SMachineContext));
		
		// Save TCB
		insertThread(tcb, thread_ar, thread_ar_size);

	MachineResumeSignals(&OldState);

	return VM_STATUS_SUCCESS;
};

TVMStatus VMThreadDelete(TVMThreadID thread)
{

	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);

		TCB *t_ptr = getThread(thread, thread_ar, thread_ar_size);
		
		// Verfiy
		if(t_ptr == NULL) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_ID;
		}
		
		if(t_ptr->state != VM_THREAD_STATE_DEAD) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_STATE;
		}

		// Begin
		deleteThread(thread, thread_ar, thread_ar_size);

	MachineResumeSignals(&OldState);

	return VM_STATUS_SUCCESS;
};

/* ------------------------------
 *   Activates a dead thread
 *   	~ Create context
 *   	~ Set state to ready
 *   	~ Enqueue thread
 *   	~ Call scheduler
 * ------------------------------
 */

TVMStatus VMThreadActivate(TVMThreadID thread)
{
	
	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);

		TCB *t_ptr = getThread(thread, thread_ar, thread_ar_size);
		
		// Verify 
		if(t_ptr == NULL) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_ID;
		}

		if(t_ptr->state != VM_THREAD_STATE_DEAD) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_STATE;
		}

		// Create thread context
		MachineContextCreate(t_ptr->smc, skeleton_entry, t_ptr->param, t_ptr->sp, t_ptr->stack_size);
	
		// Enqueue the thread in ready queue
		t_ptr->state = VM_THREAD_STATE_READY;
		enqueue_thread(t_ptr);

		// Call the scheduler
		schedule();

	MachineResumeSignals(&OldState);

	return VM_STATUS_SUCCESS;

};

TVMStatus VMThreadTerminate(TVMThreadID thread)
{
	
	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);

		TCB *t_ptr = getThread(thread, thread_ar, thread_ar_size);
		
		// Verify 
		if(t_ptr == NULL)
			return VM_STATUS_ERROR_INVALID_ID;
		
		if(t_ptr->state == VM_THREAD_STATE_DEAD )
			return VM_STATUS_ERROR_INVALID_STATE;

		// Begin
		t_ptr->state = VM_THREAD_STATE_DEAD;

		// Free mutexes held by thread
		freeMutexes(thread);

		// If the terminated thread was running, schedule
		if(thread == active[0])
			schedule_next();

	MachineResumeSignals(&OldState);

	return VM_STATUS_SUCCESS;
};

TVMStatus VMThreadID(TVMThreadIDRef threadref)
{
	
	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);

		if(threadref == NULL) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		
		*threadref = active[0];

	MachineResumeSignals(&OldState);

	return VM_STATUS_SUCCESS;
};

TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref)
{
	
	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);

		TCB *t_ptr = getThread(thread, thread_ar, thread_ar_size);
		
		// Verify
		if(stateref == NULL) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}

		if(t_ptr == NULL) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_ID;
		}

		// Begin
		*stateref = t_ptr->state;

	MachineResumeSignals(&OldState);

	return VM_STATUS_SUCCESS;
};

/* ------------------------------------
 *   Puts active thread to sleep
 *   	~ Suspend signals
 *   	~ Set sleep amount
 *   	~ Stick thread in sleep list
 *   	~ Resume signals
 *   	~ Sleep
 * -------------------------------------
 */

TVMStatus VMThreadSleep(TVMTick tick)
{

	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);

		// Verify
		if(tick == VM_TIMEOUT_INFINITE) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}

		TCB *t_ptr = getThread(active[0], thread_ar, thread_ar_size);

		if(tick == VM_TIMEOUT_IMMEDIATE) {
			
			if(debug_flag)
				printf("*** Thread %d going to sleep for 0 ticks\n", *(t_ptr->tid));

			// Insert thread into sleep list
			t_ptr->tick = 0;
			t_ptr->state = VM_THREAD_STATE_WAITING;
			insertThread(t_ptr, sleep_ar, sleep_ar_size);

			// Schedule next thread
			schedule_next();
			
			MachineResumeSignals(&OldState);
			return VM_STATUS_SUCCESS;

		} 

		t_ptr->tick = tick;
		
		t_ptr->state = VM_THREAD_STATE_WAITING;
		insertThread(t_ptr, sleep_ar, sleep_ar_size);

	MachineResumeSignals(&OldState);

	while(t_ptr->tick > 0);

	return VM_STATUS_SUCCESS;
};

/* ------------------------------------
 *   Create a Mutex
 *   	~ Allocate a mutex
 *   	~ Initialize the mutex
 *   	~ Insert mutex into an array
 * ------------------------------------
 */ 

TVMStatus VMMutexCreate(TVMMutexIDRef mutexref)
{
	
	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);

		// Verify
		if(mutexref == NULL) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}

		// Begin
		Mutex *m_ptr = malloc(sizeof(Mutex));

		// Initialize mutex
		m_ptr->m_id = m_primary_key++; 
		m_ptr->owner = NULL;
		m_ptr->wait_list = malloc(sizeof(queue_ar));
		tq_init(m_ptr->wait_list, 50);

		insertMutex(m_ptr, mutex_ar, mutex_ar_size);

		// Return
		*mutexref = m_ptr->m_id;

	MachineResumeSignals(&OldState);

	return VM_STATUS_SUCCESS;
	
};

TVMStatus VMMutexDelete(TVMMutexID mutex)
{
	
	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);

		Mutex *m_ptr = getMutex(mutex, mutex_ar, mutex_ar_size);
		
		// Verify
		if(m_ptr == NULL) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_ID;
		}

		if(m_ptr->owner != NULL) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_STATE;
		}

		// Begin
		tq_delete(m_ptr->wait_list);
		free(m_ptr);
		m_ptr = NULL;

	MachineResumeSignals(&OldState);

	return VM_STATUS_SUCCESS;
};

TVMStatus VMMutexQuery(TVMMutexID mutex, TVMThreadIDRef ownerref)
{
	
	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);

		Mutex *m_ptr = getMutex(mutex, mutex_ar, mutex_ar_size);
		
		// Verify
		if(m_ptr == NULL) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_ID;
		}

		if(ownerref == NULL)  {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}

		// Begin
		*ownerref = *(m_ptr->owner);

	MachineResumeSignals(&OldState);
	
	return VM_STATUS_SUCCESS;
};

TVMStatus VMMutexAcquire(TVMMutexID mutex, TVMTick timeout)
{
	
	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);

		Mutex *m_ptr = getMutex(mutex, mutex_ar, mutex_ar_size);
		
		// Verify
		if(m_ptr == NULL) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_ID;
		}
		
		// Begin
		TCB *t_ptr = getThread(active[0], thread_ar, thread_ar_size);
		TVMThreadID t_id = *(t_ptr->tid);

		int is_free = (m_ptr->owner == NULL);
		int next_in_line = ( tq_peek(m_ptr->wait_list) == t_id ); 	
		int line_empty = (tq_empty(m_ptr->wait_list));

		// Check if thread can grab mutex
		if(is_free && (next_in_line || line_empty)) {
			
			if(debug_flag)
				printf("*** Thread %d has grabbed mutex %d\n", t_id, mutex);

			m_ptr->owner = t_ptr->tid;
			setInt(mutex, &t_ptr->m_held_list);
			deleteInt(mutex, &t_ptr->m_wish_list);

		} else {
			
			// If timeout not immediate, wish list mutex
			if(timeout != VM_TIMEOUT_IMMEDIATE) {
			
				tq_enqueue(t_id, m_ptr->wait_list);

				if(debug_flag)
						printf("*** Thread %d wish listing mutex %d\n", t_id, mutex);

				setInt(mutex, &t_ptr->m_wish_list);
				
			}

			MachineResumeSignals(&OldState);
			
			switch(timeout) {
				case VM_TIMEOUT_IMMEDIATE:
					return VM_STATUS_FAILURE;
				case VM_TIMEOUT_INFINITE:
					waitForMutex(t_ptr);
					break;
				default:
					VMThreadSleep(timeout);
					VMMutexAcquire(mutex, VM_TIMEOUT_IMMEDIATE);
			}

		}

	MachineResumeSignals(&OldState);

	return VM_STATUS_SUCCESS;

};

TVMStatus VMMutexRelease(TVMMutexID mutex)
{
	
	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);

		Mutex *m_ptr = getMutex(mutex, mutex_ar, mutex_ar_size);
		TCB *t_ptr = getThread(active[0], thread_ar, thread_ar_size);

		// Verify
		if(m_ptr == NULL) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_ID;
		}
			
		if(!existsInt(mutex, t_ptr->m_held_list)) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_ERROR_INVALID_STATE;
		}

		// Begin
		m_ptr->owner = NULL;
		deleteInt(mutex, &t_ptr->m_held_list);
		
		if(debug_flag)
			printf("*** Thread %d released mutex %d\n", *(t_ptr->tid), mutex);
			
		reassignMutex(m_ptr);
		schedule();

	MachineResumeSignals(&OldState);

	return VM_STATUS_SUCCESS;
};


TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor)
{

	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);
	
		// Verify
		if (filename == NULL || filedescriptor == NULL)
			return VM_STATUS_ERROR_INVALID_PARAMETER;
	
		// Begin
		TCB *t_ptr = getThread(active[0], thread_ar, thread_ar_size);
		
		MachineFileOpen(filename, flags, mode, m_file_callback, (void *) t_ptr);
		
		// Block thread and schedule next
		VMThreadSleep(VM_TIMEOUT_IMMEDIATE);

		// If invalid fd
		if (t_ptr->f_result < 0) {
    	
    	MachineResumeSignals(&OldState);
			return VM_STATUS_FAILURE;

		} else
			*filedescriptor = t_ptr->f_result;

	MachineResumeSignals(&OldState);
	
	return VM_STATUS_SUCCESS;

} // VMFileOpen

TVMStatus VMFileClose(int filedescriptor)
{

	TMachineSignalState OldState;
  MachineSuspendSignals(&OldState);

		TCB *t_ptr = getThread(active[0], thread_ar, thread_ar_size);
		
		MachineFileClose(filedescriptor, m_file_callback, (void*) t_ptr);

		// Block thread and schedule next
		VMThreadSleep(VM_TIMEOUT_IMMEDIATE);
	
  MachineResumeSignals(&OldState);
	
	if(t_ptr->f_result < 0) 
    return VM_STATUS_FAILURE;
  else
  	return VM_STATUS_SUCCESS;

} //TVMStatus VMFileClose


TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset)
{

	TMachineSignalState OldState;
  MachineSuspendSignals(&OldState);
  
		TCB *t_ptr = getThread(active[0], thread_ar, thread_ar_size);
		
		MachineFileSeek(filedescriptor, offset, whence, m_file_callback, (void*) t_ptr);
		
		// Block thread and schedule next
		VMThreadSleep(VM_TIMEOUT_IMMEDIATE);

		// If error with file seek
		if(t_ptr->f_result < 0) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_FAILURE;
		}
		
		// Else return offset
		if (newoffset != NULL)
			*newoffset = t_ptr->f_result;

  MachineResumeSignals(&OldState);

  return VM_STATUS_SUCCESS;

} //TVMStatus VMFileseek


TVMStatus VMFileRead(int filedescriptor, void *data, int *length)
{
	TMachineSignalState OldState;
	MachineSuspendSignals(&OldState);
	
		if(data == NULL || length == NULL)
    	return VM_STATUS_ERROR_INVALID_PARAMETER;

		TCB *t_ptr = getThread(active[0], thread_ar, thread_ar_size);

    MachineFileRead(filedescriptor, data, *length, m_file_callback, (void*) t_ptr);

		// Block thread and schedule next
		VMThreadSleep(VM_TIMEOUT_IMMEDIATE);

		// If error with file read
		if(t_ptr->f_result < 0) {
			MachineResumeSignals(&OldState);
			return VM_STATUS_FAILURE;
		} else
  		*length = t_ptr->f_result;
  
  MachineResumeSignals(&OldState);
  
  return VM_STATUS_SUCCESS;

} //TVMStatus VMFieRead

TVMStatus VMFileWrite(int filedescriptor, void *data, int *length) 
{
	
	TMachineSignalState OldState;
  MachineSuspendSignals(&OldState);

		if(data == NULL || length == NULL)
			return VM_STATUS_ERROR_INVALID_PARAMETER;

		TCB *t_ptr = getThread(active[0], thread_ar, thread_ar_size);

		MachineFileWrite(filedescriptor, data, *length, m_file_callback, (void*) t_ptr);

		// Block thread and schedule next
		VMThreadSleep(VM_TIMEOUT_IMMEDIATE);

		if(t_ptr->f_result < 0) {
  		MachineResumeSignals(&OldState);
  		return VM_STATUS_FAILURE;
  	} else
  		*length = t_ptr->f_result;
			

  MachineResumeSignals(&OldState);

	return VM_STATUS_FAILURE;
}

