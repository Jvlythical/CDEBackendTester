#ifndef THREAD_QUEUE_H

	#define THREAD_QUEUE_H

	/* -------------------------------------
	 *   Essentially your typical queue
	 * -------------------------------------
	 */

	typedef struct  {
		int back;
		int front;
		int max;
		int size;
		TVMThreadID *ar;
	} queue_ar;

// Initialize values for the queue
	void tq_init(queue_ar *q, int max) {
		q->back = 0;
		q->front = -1;
		q->max = max;
		q->size = 0;
		q->ar = malloc(sizeof(int) * max);
	}

// Enqueue a value
	int tq_enqueue(TVMThreadID thread_id, queue_ar *q) {
		
		int front = q->front;

		while(front != q->back) {
			if(q->ar[front++] == thread_id)
				return 0;
		}

		if (q->back == q->max - 1)
			return -1;
		else {
			if (q->front == -1)
				q->front = 0;
	
			q->size += 1;
			q->ar[q->back++] = thread_id;
		}
		
		return 1;
	}

// Delete and return a value
	TVMThreadID tq_dequeue(queue_ar *q) {

			if (q->front == - 1 || q->front > q->back || q->size == 0) 
				 return -1;
			else {
				q->size -= 1;
				return q->ar[q->front++];
			}

	}
	

// Return a value 
	TVMThreadID tq_peek(queue_ar *q) {
		if (q->front == - 1 || q->front > q->back) 
			 return -1;
		else 
			return q->ar[q->front];
	}

// Returns true if queue is empty
	int tq_empty(queue_ar *q) {
		return q->size == 0;
	}

	void tq_delete(queue_ar *q) {
		free(q->ar);
	}

#endif
