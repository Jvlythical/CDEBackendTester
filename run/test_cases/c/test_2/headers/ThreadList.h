#ifndef THREAD_LIST_H
	#define THREAD_LIST_H

	void initThreadAr(TCB **ar, int ar_size) {
		int i = 0;

		for(; i < ar_size; ++i) 
			ar[i] = NULL;
	}

	int resizeThreadAr(TCB **ar, int ar_size) {
		int num_ar_bytes = sizeof(TCB *) * ar_size;

		TCB **tmp_ptr = malloc(num_ar_bytes * 2);
		initThreadAr(tmp_ptr, ar_size * 2);
		
		memcpy(tmp_ptr, ar, num_ar_bytes);
		ar = tmp_ptr;

		return ar_size *= 2;
	}

	TCB* getThread(TVMThreadID thread, TCB **ar, int ar_size) {
		int i = 0;

		for(; i < ar_size; ++i) {
			if(ar[i] != NULL) {
				if(*(ar[i]->tid) == thread)
					return ar[i];
			}
		}
		
		return NULL;
	}

	int insertThread(TCB *thread, TCB **ar, int ar_size) {

		if(getThread(*(thread->tid), ar, ar_size) != NULL)
			return ar_size;

		int i = 0;
		
		// Find an empty spot for the thread
		for(; i < ar_size; ++i) {
			if(ar[i] == NULL) {
				ar[i] = thread;
				return ar_size;
			}
		}

		// If no empty spots are found, resize the array
		ar_size = resizeThreadAr(ar, ar_size);
		insertThread(thread, ar, ar_size);

		return ar_size;
	}

	int deleteThread(TVMThreadID thread, TCB **ar, int ar_size) {
		int i = 0;
		
		// Find the key in the haystack
		for(; i < ar_size; ++i) {
			if(ar[i] != NULL) {
				
				// If key exists, delete it
				if(*(ar[i]->tid) == thread) {
					free(ar[i]->smc);
					free(ar[i]);
					ar[i] = NULL;
					
					return 1;
				}

			}
		}

		return 0;
	}

	

#endif
