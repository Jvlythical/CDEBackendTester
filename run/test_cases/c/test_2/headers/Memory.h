
#ifndef MEMORY_H
	#define MEMORY_H

	#include "../VirtualMachine.h"

	typedef struct {
		TVMMemoryPoolID id;
		TVMMemorySize size;
		TVMMemorySizeRef size_ref;
		void *base;

		// Free spaces
		// Sizes
		// Allocated spaces

	} Memory;

#endif
