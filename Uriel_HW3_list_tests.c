#include "my_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define LIST_FOR_EACH(list) for(int i = 0; i < list_size((list)) ; ++i)

// #####################################################
//					Tests macros
// #####################################################

#define ASSERT_NON_ZERO(b) do { \
if ((b) == 0) { \
	fprintf(stdout, "\nAssertion failed at %s:%d %s ",__FILE__,__LINE__,#b); \
	return false; \
} \
} while (0)

#define ASSERT_NOT_NULL(b) do { \
if ((b) == NULL) { \
	fprintf(stdout, "\nAssertion failed at %s:%d %s ",__FILE__,__LINE__,#b); \
	return false; \
} \
} while (0)

#define ASSERT_ZERO(b) do { \
if ((b) != 0) { \
	fprintf(stdout, "\nAssertion failed at %s:%d %s ",__FILE__,__LINE__,#b); \
	return false; \
} \
} while (0)

#define ASSERT_TEST(b) do { \
if (!(b)) { \
	fprintf(stdout, "\nAssertion failed at %s:%d %s ",__FILE__,__LINE__,#b); \
	return false; \
} \
} while (0)

#define RUN_TEST(test, res) do { \
fprintf(stdout, "Running "#test"... "); \
if (test()) { \
	fprintf(stdout, "[OK]\n");\
} else { \
	fprintf(stdout, "[Failed]\n"); \
	res = false; \
} \
} while(0)

// #####################################################
//					Other defines
// #####################################################

#define FAILED -1

#define FOR_IN(i, n) for ((i)=0; (i) < (n); ++(i))
#define FOR_IN_REVERSE(i, n) for ((i)=(n)-1; (i) >= 0; --(i))

#define SOME_SIZE 12
#define SOME_VALUES {1,  2,  3,  4,  5, 6, 7, 8, 100, -234,  0, 9}
#define SOME_KEYS   {0, -4, 10, 13, 12, 6, 7, 3, 300,  234, -7, 120}

#define N 35

#define MEASURE_INIT struct timeval t1, t2; \
					 bool _measureBlock; 
#define MEASURE_BLOCK for (gettimeofday(&t1, NULL), _measureBlock = true; \
						  _measureBlock; _measureBlock = false, gettimeofday(&t2, NULL))

#define MEASURE_MS ((t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0)

#define RESET_SIGNAL(mutex, s) do { \
				pthread_mutex_lock(&mutex); \
				s = 0; \
				pthread_mutex_unlock(&mutex); \
			} while(0)

#define SIGNAL_ONCE(mutex, cond, s) do { \
				pthread_mutex_lock(&mutex); \
				s = 1; \
				pthread_cond_signal(&cond); \
				pthread_mutex_unlock(&mutex); \
			} while(0)

#define SIGNAL_OTHERS(mutex, cond, s) do { \
				pthread_mutex_lock(&mutex); \
				s = 1; \
				pthread_cond_broadcast(&cond); \
				pthread_mutex_unlock(&mutex); \
			} while(0)

#define WAIT_FOR_SIGNAL(mutex, cond, s) do { \
				pthread_mutex_lock(&mutex); \
				while(s == 0) \
					pthread_cond_wait(&cond, &mutex); \
				pthread_mutex_unlock(&mutex); \
			} while(0) 

#define SLEEP(t) do { \
			int __x = 0; \
			while(__x < (t)*2) { \
				usleep(500000); \
				printf("."); \
				fflush(stdout); \
				__x++; \
			} \
		} while(0) 

// #####################################################
//					End of other defines
// #####################################################

pthread_mutex_t _lock1, _lock2;
pthread_cond_t _cond1, _cond2;
int _shared1, _shared2;

typedef struct param_t
{
	linked_list_t* list;
	int key;
	int(*compute_func) (void *);
	void* data;
	bool checkResult;
	int expectedResult;
} param_t;

typedef struct person_t {
	int age;
	char name[10];
} *Person;

int my_random(int min, int max) {
	int rnd = rand();
	return rnd % (max - min) + min;
}

int computeIntValue(void* value) {
	return (int)(long long)value;
}

int computeIntPtrValue(void* intPtr) {
	int* value = (int*)intPtr;
	return *value;
}

int computeLittleSleep(void* value) {
	sleep(1);
	return -1;
}

int computeSignalAndWait(void* value) {
	// signal the main thread: "I've arrived!"
	SIGNAL_OTHERS(_lock1, _cond1, _shared1); 

	// wait for the main thread to allow me to continue
	WAIT_FOR_SIGNAL(_lock2, _cond2, _shared2);

	return -2;
}

bool t_compute(void *param) {
	param_t *p = param;
	int data;
	int computeResult = list_compute(p->list, p->key, p->compute_func, &data);
	if(p->checkResult) {
		if (p->expectedResult == FAILED) {
			ASSERT_NON_ZERO(computeResult);
		}
		else {
			ASSERT_TEST(computeResult == p->expectedResult);
		}
		ASSERT_TEST(data == (int)(long long)p->data);
	}
	return true;
}

bool t_remove(void *param) {
	param_t *p = param;
	int res = list_remove(p->list, p->key);
	if(p->checkResult) {
		if (p->expectedResult == FAILED) {
			ASSERT_NON_ZERO(res);
		}
		else {
			ASSERT_TEST(res == p->expectedResult);
		}
	}
	return true;
}

bool t_insert(void *param) {
	param_t *p = param;

	int res = list_insert(p->list, p->key, p->data);
	if(p->checkResult) {
		if (p->expectedResult == FAILED) {
			ASSERT_NON_ZERO(res);
		}
		else {
			ASSERT_TEST(res == p->expectedResult);
		}
	}
	return true;
}

bool t_split(void *param) {
	param_t *p = param;
	int n = (int)(long long)p->data;
	linked_list_t **lists = malloc(n * sizeof(*lists));
	ASSERT_NOT_NULL(lists);

	int splitResult = list_split(p->list, n, lists);
	if(p->checkResult) {
		if (p->expectedResult == FAILED) {
			ASSERT_NON_ZERO(splitResult);
		}
		else {
			ASSERT_TEST(splitResult == p->expectedResult);
		}
	}

	int i;
	if (splitResult == 0) {
		FOR_IN(i, n) {
			list_free(lists[i]);
		}
	}
	free(lists);
	return true;
}

bool t_free(void *param) {
	linked_list_t* list = param;
	list_free(list);
	return true;
}

// #####################################################
//					End of Helper methods
// #####################################################

bool sanity_InsertRemove() {
	linked_list_t *list = list_alloc();
	ASSERT_NOT_NULL(list);
	
	int value = 1;
	int values[] = SOME_VALUES;
	int keys[] = SOME_KEYS;
	int i;
	
	// Insert:
	ASSERT_NON_ZERO(list_insert(NULL, 1, &value));
	FOR_IN(i, SOME_SIZE) {
		ASSERT_ZERO(list_insert(list, keys[i], values + i));
	}
	FOR_IN(i, SOME_SIZE) {
		ASSERT_NON_ZERO(list_insert(list, keys[i], values + i));
	}
	
	// Remove:
	ASSERT_NON_ZERO(list_remove(NULL, 1));
	FOR_IN_REVERSE(i, SOME_SIZE) {
		ASSERT_ZERO(list_remove(list, keys[i]));
	}
	FOR_IN_REVERSE(i, SOME_SIZE) {
		ASSERT_NON_ZERO(list_remove(list, keys[i]));
	}
	
	// Insert again:
	FOR_IN(i, SOME_SIZE) {
		ASSERT_ZERO(list_insert(list, keys[i], values + i));
	}
	
	// Free:
	list_free(list);
	
	// Try with complex structure
	list = list_alloc();
	ASSERT_NOT_NULL(list);
	Person persons = malloc(sizeof(*persons) * N);
	FOR_IN(i, N) {
		persons[i].age = (i + N/3) % N;
		strcpy(persons[i].name, "name");
	}
	
	// Insert:
	FOR_IN(i, N) {
		ASSERT_ZERO(list_insert(list, persons[i].age, persons + i));
	}
	FOR_IN_REVERSE(i, N) {
		ASSERT_NON_ZERO(list_insert(list, persons[i].age, persons + i));
	}
	
	// Remove:
	FOR_IN(i, N) {
		ASSERT_ZERO(list_remove(list, persons[i].age));
	}
	
	// Insert again:
	FOR_IN(i, N) {
		ASSERT_ZERO(list_insert(list, persons[i].age, persons + i));
	}
	
	// Free:
	list_free(list);
	
	free(persons);

	// Try to free empty list:
	list = list_alloc();
	ASSERT_NOT_NULL(list);
	list_free(list);

	list = list_alloc();
	ASSERT_NOT_NULL(list);
	ASSERT_ZERO(list_insert(list, 1, (void*)(long long)1));
	ASSERT_ZERO(list_remove(list, 1));
	list_free(list);

	return true;
}

bool sanity_Update() {
	linked_list_t *list = list_alloc();
	ASSERT_NOT_NULL(list);
	
	int value = 1;
	int i;
	int values[] = SOME_VALUES;
	int keys[] = SOME_KEYS;
	Person persons = malloc(sizeof(*persons) * N);
	FOR_IN(i, N) {
		persons[i].age = (i + N/3) % N;
		strcpy(persons[i].name, "name");
	}
	
	// Insert:
	FOR_IN(i, SOME_SIZE) {
		ASSERT_ZERO(list_insert(list, keys[i], values + i));
	}
	
	// Update:
	FOR_IN_REVERSE(i, 5) {
		ASSERT_ZERO(list_update(list, keys[i], persons + i));
	}
	
	// Failed updates:
	ASSERT_NON_ZERO(list_update(NULL, 1, &value));
	ASSERT_ZERO(list_remove(list, keys[0]));
	ASSERT_NON_ZERO(list_update(list, keys[0], values));
	
	// Free:
	list_free(list);
	
	free(persons);
	return true;
}

bool sanity_Find() {
	linked_list_t *list = list_alloc();
	ASSERT_NOT_NULL(list);
	
	int values[] = SOME_VALUES;
	int keys[] = SOME_KEYS;
	struct person_t person;
	person.age = 10;
	strcpy(person.name, "hi");
	
	int i;
	
	// Insert:
	FOR_IN(i, SOME_SIZE) {
		ASSERT_ZERO(list_insert(list, keys[i], values + i));
	}
	ASSERT_ZERO(list_insert(list, -100, &person));
	
	// Find:
	FOR_IN_REVERSE(i, SOME_SIZE) {
		ASSERT_TEST(list_find(list, keys[i]) == 1);
	}
	ASSERT_TEST(list_find(list, -100) == 1);
	
	
	// Failed finds:
	int list_find_res = list_find(NULL, 1);
	ASSERT_TEST(list_find_res != 0 && list_find_res != 1);
	ASSERT_ZERO(list_remove(list, keys[0]));
	ASSERT_ZERO(list_find(list, keys[0]));
	
	// Free:
	list_free(list);
	return true;
}

bool sanity_Size() {
	linked_list_t *list = list_alloc();
	ASSERT_NOT_NULL(list);
	
	int values[] = SOME_VALUES;
	int keys[] = SOME_KEYS;
	int i, n = SOME_SIZE;
	
	// Insert:
	FOR_IN(i, SOME_SIZE) {
		ASSERT_ZERO(list_insert(list, keys[i], values + i));
	}
	
	ASSERT_TEST(list_size(list) == n);
	FOR_IN_REVERSE(i, 3) {
		ASSERT_ZERO(list_remove(list, keys[i]));
		ASSERT_TEST(list_size(list) == --n);
	}
	
	ASSERT_TEST(list_size(NULL) < 0);
	
	// Free:
	list_free(list);
	return true;
}

bool sanity_Split_aux(int n, int desired_size) {
	linked_list_t *list = list_alloc();
	ASSERT_NOT_NULL(list);
	
	int i;
	int values[] = SOME_VALUES;
	int keys[] = SOME_KEYS;
	
	// Insert:
	FOR_IN(i, SOME_SIZE) {
		ASSERT_ZERO(list_insert(list, keys[i], values + i));
	}
	
	linked_list_t** arr = malloc(n * sizeof(*arr));
	FOR_IN(i, n) {
		arr[i] = NULL;
	}
	
	ASSERT_ZERO(list_split(list, n, arr));
	
	FOR_IN(i, n) {
		if(i >= SOME_SIZE) {
			ASSERT_NOT_NULL(arr[i]);
		}
		else {
			ASSERT_TEST(list_size(arr[i]) == desired_size);
		}
	}
	
	// Free:
	FOR_IN(i, n) {
		list_free(arr[i]);
	}
	
	// list_free(list); no need!! split already have done that
	free(arr);
	return true;
}

bool sanity_Split() {
	ASSERT_TEST(sanity_Split_aux(2*SOME_SIZE, 1));
	ASSERT_TEST(sanity_Split_aux(SOME_SIZE, 1));
	ASSERT_TEST(sanity_Split_aux(1, SOME_SIZE));
	ASSERT_TEST(sanity_Split_aux(3, SOME_SIZE/3));
	return true;
}

bool sanity_Compute() {
	linked_list_t* list = list_alloc();
	ASSERT_NOT_NULL(list);
	
	int val = 10, res, *intPtr, i;
	list_insert(list, 1, (void*)(long long)1);
	list_insert(list, 2, &val);

	// Check failors
	ASSERT_NON_ZERO(list_compute(NULL, 1, computeIntValue, &res)); // NULL list
	ASSERT_NON_ZERO(list_compute(list, 0, computeIntValue, &res)); // not found
	ASSERT_NON_ZERO(list_compute(list, 1, NULL, &res));	// NULL function
	ASSERT_NON_ZERO(list_compute(list, 1, computeIntValue, NULL)); // NULL res

	ASSERT_ZERO(list_compute(list, 1, computeIntValue, &res));
	ASSERT_TEST(res == 1);

	ASSERT_ZERO(list_compute(list, 2, computeIntPtrValue, &res));
	ASSERT_TEST(res == val);

	for (i = 5; i < 20; i++) {
		ASSERT_ZERO(list_insert(list, i, &i));
	}

	// ALL values should hold pointer to i
	ASSERT_ZERO(list_compute(list, 5, computeIntPtrValue, &res));
	ASSERT_TEST(res == i);
	ASSERT_ZERO(list_compute(list, 14, computeIntPtrValue, &res));
	ASSERT_TEST(res == i);

	// AGAIN, who cares
	ASSERT_ZERO(list_compute(list, 1, computeIntValue, &res));
	ASSERT_TEST(res == 1);
	ASSERT_ZERO(list_compute(list, 2, computeIntPtrValue, &res));
	ASSERT_TEST(res == val);

	list_free(list);
	return true;
}

#define INSERT_REMOVE_BLK_SIZE 20
#define TOTAL_OPS_SIZE 27
bool async_Batch() {
	linked_list_t* list = list_alloc();
	ASSERT_NOT_NULL(list);

	int i, res, otherValue = -100;

	for (i = 0; i < INSERT_REMOVE_BLK_SIZE; i += 2) {
		ASSERT_ZERO(list_insert(list, i, &i));
	}
	ASSERT_ZERO(list_insert(list, 21, &i));
	ASSERT_ZERO(list_insert(list, 22, &i));
	ASSERT_ZERO(list_insert(list, 23, &i));
	ASSERT_ZERO(list_insert(list, -1, &i));
	ASSERT_ZERO(list_insert(list, -5, &otherValue));

	ASSERT_TEST(list_size(list) == INSERT_REMOVE_BLK_SIZE/2 + 5);
	
	op_t operations[TOTAL_OPS_SIZE];
	operations[0].key = -1;
	operations[0].op = COMPUTE;
	operations[0].compute_func = computeLittleSleep;

	for (i = 0; i < INSERT_REMOVE_BLK_SIZE; i++) {
		int opIndex = i + 1;
		operations[opIndex].key = i;
		operations[opIndex].data = &i;
		if (i % 2 == 0) {
			operations[opIndex].op = REMOVE;
		}
		else {
			operations[opIndex].op = INSERT;
		}
	}

	// initialize extra operations:
	// 20: update key:21
	// 21: contains key:21
	// 22: compute key:23
	// 23: compute key:-5
	// 24: compute key:-5
	// 25: contains key:50 - should fail
	i = TOTAL_OPS_SIZE - 6;
	operations[i].op = UPDATE;
	operations[i].key = 21;
	operations[i++].data = &otherValue;

	int containsIndex = i;
	operations[i].op = CONTAINS;
	operations[i++].key = 21;

	int computeIndex = i;
	operations[i].op = COMPUTE;
	operations[i].key = 23;
	operations[i++].compute_func = computeLittleSleep;
	
	operations[i].op = COMPUTE;
	operations[i].key = -5;
	operations[i++].compute_func = computeLittleSleep;
	
	operations[i].op = COMPUTE;
	operations[i].key = -5;
	operations[i++].compute_func = computeIntPtrValue;
	
	operations[i].op = CONTAINS;
	operations[i++].key = 50;

	MEASURE_INIT
	MEASURE_BLOCK {
		list_batch(list, TOTAL_OPS_SIZE, operations);
	}

	double elapsedTime = MEASURE_MS;

	// all the sleeps should be concurrent
	if(elapsedTime >= 2100.0) { 
		printf("\nAssertion failed at %d: elapsedTime:%f should be less than 2100.\n", __LINE__, elapsedTime);
		printf("If you are running with VALGRIND you can ignore this error.\n");
	}

	ASSERT_TEST(list_size(list) == INSERT_REMOVE_BLK_SIZE/2 + 5);

	FOR_IN(i, TOTAL_OPS_SIZE) {
		if (i == containsIndex) {
			ASSERT_TEST(operations[i].result == 1);
		} else {
			// the last "contains" should return zero too which means not found.
			ASSERT_ZERO(operations[i].result);
		}
	}
	
	ASSERT_TEST((int)(long long)(operations[computeIndex].data) == -1); 
	ASSERT_TEST((int)(long long)(operations[computeIndex + 1].data) == -1); 
	ASSERT_TEST((int)(long long)(operations[computeIndex + 2].data) == otherValue); 

	ASSERT_ZERO(list_compute(list, 21, computeIntPtrValue, &res));
	ASSERT_TEST(res == otherValue); // after the update the value should be otherValue instead of i

	list_free(list);

	return true;
}

#define RECORDS_SIZE 1000
bool async_InsertRemove() {
	linked_list_t* list = list_alloc();
	ASSERT_NOT_NULL(list);
	pthread_t threads[RECORDS_SIZE/2];
	param_t p[RECORDS_SIZE/2];
	op_t ops[RECORDS_SIZE/2];
	int i;

	printf(" Inserting and removing %d records. ", RECORDS_SIZE);

	// #### INSERT RECORDS_SIZE records ###################
	FOR_IN(i, RECORDS_SIZE) {
		if(i<RECORDS_SIZE/2) {
			p[i].list = list;
			p[i].checkResult = true;
			p[i].key = i;
			p[i].data = (void*)(long long)i;
			p[i].expectedResult = 0;
		} else {
			ops[i-(RECORDS_SIZE/2)].key = i;
			ops[i-(RECORDS_SIZE/2)].op = INSERT;
			ops[i-(RECORDS_SIZE/2)].data = (void*)(long long)i;
		}
	}

	FOR_IN(i, RECORDS_SIZE/2) {
		pthread_create(threads + i, NULL, t_insert, p + i);
	}
	
	list_batch(list, RECORDS_SIZE/2, ops);

	FOR_IN(i, RECORDS_SIZE/2) {
		bool insertRes;
		pthread_join(threads[i], &insertRes);
		ASSERT_TEST(insertRes);
	}

	ASSERT_TEST(list_size(list) == RECORDS_SIZE);
	
	// #### REMOVE RECORDS_SIZE records ###################
	FOR_IN(i, RECORDS_SIZE) {
		if(i<RECORDS_SIZE/2) {
			p[i].list = list;
			p[i].checkResult = true;
			p[i].key = i;
			p[i].expectedResult = 0;
		} else {
			ops[i-(RECORDS_SIZE/2)].key = i;
			ops[i-(RECORDS_SIZE/2)].op = REMOVE;
		}
	}

	FOR_IN(i, RECORDS_SIZE/2) {
		pthread_create(threads + i, NULL, t_remove, p + i);
	}
	list_batch(list, RECORDS_SIZE/2, ops);
	
	FOR_IN(i, RECORDS_SIZE/2) {
		bool removeRes;
		pthread_join(threads[i], &removeRes);
		ASSERT_TEST(removeRes);
	}

	ASSERT_TEST(list_size(list) == 0);

	list_free(list);
	return true;
}

bool async_ComputeWaitAndRemove() {
	linked_list_t* list = list_alloc();
	ASSERT_NOT_NULL(list);
	int i;
	FOR_IN(i, 10) {
		ASSERT_ZERO(list_insert(list, i, (void*)(long long)i));
	}

	// prepare threads
	pthread_t threads[5];
	param_t p[5];
	FOR_IN(i, 5) {
		p[i].list = list;
		p[i].checkResult = true;
	}

	// compute
	p[0].key = 2;
	p[0].compute_func = computeSignalAndWait;
	p[0].expectedResult = 0;
	p[0].data = (void*)(long long)-2;
	// remove
	p[1].key = 1;
	p[1].expectedResult = 0;
	// remove
	p[2].key = 2;
	p[2].expectedResult = 0;
	// remove
	p[3].key = 3;
	p[3].expectedResult = 0;
	// remove
	p[4].key = 4;
	p[4].expectedResult = 0;

	// reset signals
	RESET_SIGNAL(_lock1, _shared1);
	RESET_SIGNAL(_lock2, _shared2);

	// start compute and wait it to be ready
	pthread_create(threads, NULL, t_compute, p);
	WAIT_FOR_SIGNAL(_lock1, _cond1, _shared1);
	
	// start removes
	for (i = 1; i < 5; i++) {
		pthread_create(threads + i, NULL, t_remove, p + i);
	}
	
	SLEEP(4); // wait reasonable time

	SIGNAL_OTHERS(_lock2, _cond2, _shared2); // let all continue
	
	// CHECK
	bool computeOk;
	pthread_join(threads[0], &computeOk);
	ASSERT_TEST(computeOk);
	for (i = 1; i < 5; i++) {
		int removeOk;
		pthread_join(threads[i], &removeOk);
		ASSERT_TEST(removeOk);
	}
	
	list_free(list);
	return true;
}

bool async_ComputeAndFree_aux(bool nice, bool split) {
	printf("\n");

	linked_list_t* list = list_alloc();
	ASSERT_NOT_NULL(list);
	int i;
	FOR_IN(i, 10) {
		ASSERT_ZERO(list_insert(list, i, (void*)(long long)i));
	}

	// prepare threads
	pthread_t threads[5];
	param_t p[5];
	FOR_IN(i, 5) {
		p[i].list = list;		
		p[i].checkResult = true;
	}
	
	// compute
	p[0].key = 2;
	p[0].compute_func = computeSignalAndWait;
	p[0].expectedResult = 0;
	p[0].checkResult = true;
	p[0].data = (void*)(long long)-2;
	
	// for split
	p[1].data = (void*)(long long)2;
	p[1].expectedResult = 0;

	// insert
	p[2].key = 1;
	p[2].data = &i;
	p[2].expectedResult = FAILED;
	// remove
	p[3].key = 2;
	p[3].expectedResult = FAILED;
	// remove
	p[4].key = 3;
	p[4].expectedResult = FAILED;

	// reset signals
	RESET_SIGNAL(_lock1, _shared1);
	RESET_SIGNAL(_lock2, _shared2);

	// start compute and wait it to be ready
	printf(" -> Creating compute task and waitig for it to be ready...\n");
	pthread_create(threads, NULL, t_compute, p);
	WAIT_FOR_SIGNAL(_lock1, _cond1, _shared1);
	
	printf(" -> Creating free task.\n");
	// start free
	if(split) {
		pthread_create(threads + 1, NULL, t_split, p + 1);
	}
	else {
		pthread_create(threads + 1, NULL, t_free, list);
	}
	
	printf(" -> waiting a little...");
	SLEEP(4); // wait reasonable time
	printf("\n");

	printf(" -> creating new tasks (all should fail).\n");
	// start others:
	pthread_create(threads + 2, NULL, t_insert, p + 2);
	for (i = 3; i < 5; i++) {
		pthread_create(threads + i, NULL, t_remove, p + i);
	}

	if(!nice) {
		// - THIS CAN CAUSE A DEADLOCK DEPENDS ON LIST IMPLEMENTATION -
		// others should fail imidietly!!!!
		for (i = 2; i < 5; i++) {
			int ok;
			pthread_join(threads[i], &ok);
			ASSERT_TEST(ok);
		}
	}

	if(nice) {
		printf(" -> waiting a little more...");
		SLEEP(2);
		printf("\n");
	}

	printf(" -> let compute to finish.\n");
	SIGNAL_OTHERS(_lock2, _cond2, _shared2); // let all continue
	
	// CHECK
	bool computeOk, freeOk;
	pthread_join(threads[0], &computeOk);
	ASSERT_TEST(computeOk);

	printf(" -> now free should be finished.\n");
	pthread_join(threads[1], &freeOk);
	ASSERT_TEST(freeOk);

	if(nice) {
		// others should fail
		for (i = 2; i < 5; i++) {
			int ok;
			pthread_join(threads[i], &ok);
			ASSERT_TEST(ok);
		}
	}
	printf(" -> all good :)\n");
	// list_free(list); no need!!
	return true;
}

bool async_ComputeAndFree_nice() {
	return async_ComputeAndFree_aux(true, false);
}

bool async_ComputeAndFree_bad() {
	return async_ComputeAndFree_aux(false, false);
}

bool async_ComputeAndSplit_nice() {
	return async_ComputeAndFree_aux(true, true);
}

bool async_ComputeAndSplit_bad() {
	return async_ComputeAndFree_aux(false, true);
}

void init_locks() {
	pthread_mutex_init(&_lock1, NULL);
	pthread_mutex_init(&_lock2, NULL);
	pthread_cond_init(&_cond1, NULL);
	pthread_cond_init(&_cond2, NULL);
	pthread_cond_signal(&_cond1);
	pthread_cond_signal(&_cond2);
}

void destroy_locks() {
	pthread_mutex_destroy(&_lock1, NULL);
	pthread_mutex_destroy(&_lock2, NULL);
	pthread_cond_destroy(&_cond1, NULL);
	pthread_cond_destroy(&_cond2, NULL);
}

int main(){
	bool res = true;
	init_locks();
	srand(time(NULL));

	RUN_TEST(sanity_InsertRemove, res);
	RUN_TEST(sanity_Update, res);
	RUN_TEST(sanity_Find, res);
	RUN_TEST(sanity_Size, res);
	RUN_TEST(sanity_Split, res);
	RUN_TEST(sanity_Compute, res);
	if(!res) {
		printf("\nYou didn't pass the basic tests, \n");
		printf("\tpress Enter if you want to continue. To stop press CTRL+C\n");
		while (getchar() != '\n') ;
	}
	RUN_TEST(async_Batch, res);
	RUN_TEST(async_InsertRemove, res);
	RUN_TEST(async_ComputeAndFree_nice, res);
	RUN_TEST(async_ComputeAndSplit_nice, res);
	RUN_TEST(async_ComputeAndFree_bad, res);
	RUN_TEST(async_ComputeAndSplit_bad, res);
	destroy_locks();
	return 0;
}
