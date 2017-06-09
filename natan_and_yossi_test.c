#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "my_list.h"

int result;
sem_t sem_for_yosi_test;

int stop_and_wait_for_sem(void* param){
	printf("waiting for semaphore\n");
	if(sem_wait(&sem_for_yosi_test) != 0){
		printf("sem_wait FAILED\n");
		assert(0);
		result = 0;
	}
	printf("done waiting for semaphore\n");
	return 0;
}

void* list_compute_aux(void* param){
	linked_list_t* list_to_compute = (linked_list_t*)param;
	printf("calling compute\n");
	assert(list_find(list_to_compute, 400));
	int result;
	int res = list_compute(list_to_compute, 400, &stop_and_wait_for_sem, &result);
	assert(!res);
	printf("done calling compute\n");
	return NULL;
}

void* do_some_operation(void* param){
	printf("sleeping before doing operations\n");
	sleep(5);
	linked_list_t* cast_param = (linked_list_t*)param;
	int i;
	for(i = 0; i < 10; i++){
		if (list_insert(cast_param, i, NULL) == 0){
			printf("insert success and should be failed\n");
			assert(0);
			result = 0;
		}
	}
	for(i = 0; i < 10; i++){
		if (list_remove(cast_param, i) == 0){
			printf("remove success and should be failed\n");
			assert(0);
			result = 0;
		}
	}
	for(i = 0; i < 10; i++){
		if (list_find(cast_param, i) == 0){
			printf("list_find success and should be failed\n");
			assert(0);
			result = 0;
		}
	}
	for(i = 0; i < 10; i++){
		if (list_size(cast_param) == 0){
			printf("list_size success and should be failed\n");
			assert(0);
			result = 0;
		}
	}
	for(i = 0; i < 10; i++){
		if (list_update(cast_param, i, NULL) == 0){
			printf("list_update success and should be failed\n");
			assert(0);
			result = 0;
		}
	}

	for(i = 2; i < 3; i++){
		if (list_split(cast_param, i, NULL) == 0){
			printf("list_spilt success and should be failed\n");
			assert(0);
			result = 0;
		}
	}

			printf("list_free start\n");
			list_free(cast_param);
			printf("list_free end\n");


	if (sem_post(&sem_for_yosi_test)) {
		printf("error posting to semaphore in test\n");
	}
	//printf("finished operations and they all failed\n");
	return NULL;
}

int test2() {
	printf("**********************START TESTING****************\n");
	result = 1;
    linked_list_t* list2 = list_alloc();
	assert(list_size(list2) == 0);
	int i;
    for(i = 0; i < 500; i++){
    	list_insert(list2, i, NULL);
    }
	
	pthread_t thread3;
	pthread_t thread4;

	if(sem_init(&sem_for_yosi_test, 0, 0) != 0){
		printf("sem_init FAILD\n");
		assert(0);
		result = 0;
	}

    if(pthread_create(&thread3, NULL, &list_compute_aux, list2) != 0){
    	printf("pthread_create with thread1 FAILED\n");
    	assert(0);
    	result = 0;
    }

	sleep(3);
		
    if (pthread_create(&thread4, NULL, &do_some_operation, list2) != 0){
    	printf("pthread_create with thread2 FAILED\n");
    	assert(0);
    	result = 0;
    }
	
	printf("calling list_free\n");
    list_free(list2);
	printf("returned from list_free\n");
	pthread_join(thread3, NULL);
	printf("thread3 finished\n");
	pthread_join(thread4, NULL);
	if (result == 1){
		printf("****************TEST PASS***********************\n");
	}
	else{
		printf("****************TEST FAILED*********************\n");
	}
    return 0;
}

int main() {
	test2();
	return 0;
}
