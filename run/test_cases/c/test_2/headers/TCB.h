#ifndef TCB_H
	#define TCB_H
	
	#include "../VirtualMachine.h"
	
	struct mutex;

	typedef struct {
		TVMThreadIDRef tid;
		TVMThreadState state;
		TVMThreadPriority prio;
		int8_t *sp;
		TVMMemorySize stack_size;
		TVMThreadEntry entry;
		void *param;
		SMachineContext *smc;
		TVMTick tick;
		IntList m_held_list;
		IntList m_wish_list;

		int f_result;

		// Possible hold a pointer or ID of mutext waiting on
		// Possibly a list of held mutexes
	} TCB;

#endif
