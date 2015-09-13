
#ifndef INT_LIST_H

	#define INT_LIST_H

	/* ---------------------------------------------------
	 *   Somewhat modified typical singly linked list
	 * ---------------------------------------------------
	 */

	// Forward declaration
	typedef struct IntNode IntNode ;

	// Linked list node
	struct IntNode {
		
		IntNode *next;
		int val;

	};

	// Linked list
	typedef struct {
	
		IntNode *head;
		IntNode *tail;

	} IntList;

	// Check if list is empty
	int emptyInt(IntList *l) {
		if(l->head == NULL)
			return 1;
		else 
			return 0;
	}

	// Check if a value exists in list
	int existsInt(int key, IntList l) {
		IntNode *cur = l.head;

		while(cur != NULL) {
			if(cur->val == key)
				return 1;

			cur = cur->next;
		}

		return 0;
	}

	// Push a value to back of list
	void setInt(int key, IntList *l) {
		IntNode *node = malloc(sizeof(IntNode));

		node->val = key;
		node->next = NULL;
		
		if(l->head == NULL) 
			l->head = node;

		if(l->tail == NULL)
			l->tail = node;
		else {
			l->tail->next = node;
			l->tail = node;
		}

	}

	// Delete all instances of a value in list
	void deleteInt(int key, IntList *l) {
		
		if(emptyInt(l))
			return;

		IntNode *cur = l->head;
		IntNode *prev = NULL;

		while(cur != NULL) {
			
			if(cur->val == key) {
				
				if(prev == NULL) 
					l->head = cur->next;
				else 
					prev->next = cur->next;

				free(cur);
			}

			prev = cur;
			cur = cur->next;
		}

	}

	void printList(IntList l) {

		IntNode *cur = l.head;

		while(cur != NULL) {
			printf("%d ", cur->val);
			cur = cur->next;
		}

		puts("");

	}

#endif
