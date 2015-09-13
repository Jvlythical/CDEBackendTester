#ifndef MUTEX_LIST_H
	#define MUTEX_LIST_H

	void initMutexAr(Mutex **ar, int ar_size) {
		int i = 0;

		for(; i < ar_size; ++i) 
			ar[i] = NULL;
	}

	int resizeMutexAr(Mutex **ar, int ar_size) {
		int num_ar_bytes = sizeof(Mutex *) * ar_size;

		Mutex **tmp_ptr = malloc(num_ar_bytes * 2);
		initMutexAr(tmp_ptr, ar_size * 2);
		
		memcpy(tmp_ptr, ar, num_ar_bytes);
		ar = tmp_ptr;

		return ar_size *= 2;
	}

	Mutex* getMutex(TVMMutexID m_id, Mutex **ar, int ar_size) {
		int i = 0;

		for(; i < ar_size; ++i) {
			if(ar[i] != NULL) {
				if(ar[i]->m_id == m_id)
					return ar[i];
			}
		}
		
		return NULL;
	}

	int insertMutex(Mutex *mutex, Mutex **ar, int ar_size) {

		if(getMutex(mutex->m_id, ar, ar_size) != NULL)
			return ar_size;

		int i = 0;
		
		// Find an empty spot for the thread
		for(; i < ar_size; ++i) {
			if(ar[i] == NULL) {
				ar[i] = mutex;
				return ar_size;
			}
		}

		// If no empty spots are found, resize the array
		ar_size = resizeMutexAr(ar, ar_size);
		insertMutex(mutex, ar, ar_size);

		return ar_size;
	}

	int deleteMutex(TVMMutexID m_id, Mutex **ar, int ar_size) {
		int i = 0;
		
		// Find the key in the haystack
		for(; i < ar_size; ++i) {
			if(ar[i] != NULL) {
				
				// If key exists, delete it
				if(ar[i]->m_id == m_id) {
					free(ar[i]);
					ar[i] = NULL;
					
					return 1;
				}

			}
		}

		return 0;
	}

#endif
