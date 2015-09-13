#ifndef MUTEX_H

	#define MUTEX_H

	typedef struct mutex {
		TVMMutexID m_id;
		TVMThreadIDRef owner;
		queue_ar *wait_list;
	} Mutex;

#endif
