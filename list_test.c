/*
 * list_test.c
 *
 *  Created on: 23 במאי 2017
 *      Author: Omri
 */

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "my_list.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* please add these  functions  to the my_list.h and my_list.c for debugging purposes and for this test :) */
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*void printList(linked_list_t* list) {
	if (!list->size)
		return;
	node_t* itr;
	for (itr = list->head->next; itr != NULL; itr = itr->next) {
		printf("%d ", itr->key);
	}
	printf("\n");
	return;
}

void printIntData(linked_list_t* list) {
	if (!list->size)
		return;
	node_t* itr;
	for (itr = list->head->next; itr != NULL; itr = itr->next) {
		printf("%d ", *(int*) itr->data);
	}
	printf("\n");
	return;
}
*/
#define ASSERT_TEST(b) do { \
        if (!(b)) { \
                fprintf(stdout, "\nAssertion failed at %s:%d %s ",__FILE__,__LINE__,#b); \
                return false; \
        } \
} while (0)

#define RUN_TEST(test) do { \
        fprintf(stdout, "Running "#test"... "); \
        if (test()) { \
            fprintf(stdout, "[OK]\n");\
        } else { \
        	fprintf(stdout, "[Failed]\n"); \
        } \
} while(0)

bool LIST_ALLOC_TEST() {
	linked_list_t* newList = list_alloc();
	ASSERT_TEST(newList!=NULL);
	ASSERT_TEST(list_size(newList) == 0);
	list_free(newList);
	return true;
}

bool LIST_INSERT_TEST() {
	linked_list_t* newList = list_alloc();
	int v[12], *ptr[12], i = 0;
	for (i = 0; i < 10; i++) {
		v[i] = i * 2;
		ptr[i] = &v[i];
	}
	for (i = 0; i < 10; i++) {
		ASSERT_TEST(list_insert(newList, v[i], ptr[i]) == 0);
		ASSERT_TEST(list_size(newList) == i + 1);
	}
	v[10] = 1, v[11] = 11, ptr[10] = &v[10], ptr[11] = &v[11];
	ASSERT_TEST(list_insert(newList, v[10], ptr[10]) == 0);
	ASSERT_TEST(list_insert(newList, v[11], ptr[11]) == 0);
	ASSERT_TEST(list_size(newList) == 12);
	ASSERT_TEST(list_insert(newList, v[0], ptr[0]) != 0);
	ASSERT_TEST(list_insert(NULL,200,ptr[0]) != 0);
	ASSERT_TEST(list_insert(newList, -1, ptr[0]) == 0);
	ASSERT_TEST(list_insert(newList,200,NULL) == 0);
	printList(newList);
	list_free(newList);
	return true;
}

bool LIST_REMOVE_TEST() {
	linked_list_t* newList = list_alloc();
	int v[12], *ptr[12], i = 0;
	for (i = 0; i < 10; i++) {
		v[i] = i * 2;
		ptr[i] = &v[i];
	}
	for (i = 0; i < 10; i++) {
		list_insert(newList, v[i], ptr[i]);
	}
	for (i = 0; i < 10; i += 2) {
		ASSERT_TEST(list_remove(newList, v[i]) == 0);
	}
	ASSERT_TEST(list_size(newList) == 5);
	ASSERT_TEST(list_remove(newList, v[0]) != 0);
	ASSERT_TEST(list_remove(NULL,v[0]) != 0);
	ASSERT_TEST(list_remove(newList, -1) != 0);
	v[10] = 1, v[11] = 11, ptr[10] = &v[10], ptr[11] = &v[11];
	list_insert(newList, v[10], ptr[10]);
	ASSERT_TEST(list_size(newList) == 6);
	list_insert(newList, v[11], ptr[11]);
	ASSERT_TEST(list_remove(newList, v[11]) == 0);
	ASSERT_TEST(list_size(newList) == 6);
	ASSERT_TEST(list_remove(newList, v[10]) == 0);
	printList(newList);
	list_free(newList);
	return true;
}

bool LIST_FIND_TEST() {
	linked_list_t* newList = list_alloc();
	ASSERT_TEST(list_find(newList, 1) == 0);
	int v[10], *ptr[10], i = 0;
	for (i = 0; i < 10; i++) {
		v[i] = i;
		ptr[i] = &v[i];
	}
	for (i = 0; i < 10; i++) {
		list_insert(newList, v[i], ptr[i]);
	}
	for (i = 0; i < 10; i++) {
		ASSERT_TEST(list_find(newList, v[i]) == 1);
	}
	for (i = 0; i < 10; i += 2) {
		list_remove(newList, v[i]);
	}
	for (i = 0; i < 10; i++) {
		if (i % 2) {
			ASSERT_TEST(list_find(newList, v[i]) == 1);
		} else {
			ASSERT_TEST(list_find(newList, v[i]) == 0);
		}
	}
	ASSERT_TEST(list_find(newList, -1) == 0);
	ASSERT_TEST(list_find(NULL,1) == 0);
	printList(newList);
	list_free(newList);
	return true;
}

bool LIST_UPDATE_TEST() {
	linked_list_t* newList = list_alloc();
	int w[10], v[10], i = 0;
	for (i = 0; i < 10; i++) {
		v[i] = i * 2;
	}
	for (i = 0; i < 10; i++) {
		list_insert(newList, v[i], &v[i]);
	}
	printList(newList);
	for (i = 0; i < 10; i++) {
		w[i] = i;
		ASSERT_TEST(list_update(newList, v[i], &w[i]) == 0);
	}
	ASSERT_TEST(list_size(newList) == 10);
	ASSERT_TEST(list_update(newList, 1, &w[0]) != 0); //1 is not a valid key in the list
	ASSERT_TEST(list_update(newList,2,NULL)==0);
	list_free(newList);
	return true;
}

bool LIST_SPLIT_TEST() {
	linked_list_t* newList = list_alloc();
	int i = 0;
	int* v[60];
	for (i = 0; i < 60; i++) {
		v[i] = malloc(sizeof(int));
		*v[i] = i * 2;
	}
	for (i = 0; i < 60; i++) {
		list_insert(newList, *v[i], v[i]);
	}
	printList(newList);
	linked_list_t* listArr[3];
	ASSERT_TEST(list_split(newList, 3, listArr) == 0);
	for (i = 0; i < 3; i++) {
		printList(listArr[i]);
		printf("\n");
	}
	ASSERT_TEST(list_size(listArr[0])==20);
	ASSERT_TEST(list_size(listArr[1])==20);
	ASSERT_TEST(list_size(listArr[2])==20);
	ASSERT_TEST(list_split(NULL,3,listArr)!=0);
	ASSERT_TEST(list_split(newList, 0, listArr) != 0);
	ASSERT_TEST(list_split(newList,0,NULL)!=0);
	for (i = 0; i < 3; i++) {
		list_free(listArr[i]);
	}
	for (i = 0; i < 60; i++) {
		free(v[i]);
	}
	linked_list_t* list1 = list_alloc();
	linked_list_t* list2;
	ASSERT_TEST(list_size(list1) == 0);
	list_insert(list1,66,"Jon");
	list_insert(list1,44,"Sansa");
	list_insert(list1,55,"Arya");
	list_insert(list1,22,"Bran");
	list_insert(list1,11,"Rickon");
	list_insert(list1,33,"Robb");
	ASSERT_TEST(list_size(list1) == 6);
	ASSERT_TEST(list_split(list1,1,&list2)==0);
	ASSERT_TEST(list_size(list2)==6);
	list_free(list2);
	return true;
}

int retInt(void* num) {
	return *(int*) num;
}

int multiplyInt10(void* num) {

	int result = *(int*) num;
	result *= 10;
	return result;
}

int tryChangeData(void* num) {
	num = NULL;
	return 0;
}

bool LIST_COMPUTE_TEST() {
	linked_list_t* newList = list_alloc();
	int* v[10], i;
	for (i = 0; i < 10; i++) {
		v[i] = malloc(sizeof(int));
		*v[i] = i * 2;
	}
	for (i = 0; i < 10; i++) {
		list_insert(newList, i, v[i]);
	}
	printList(newList);
	int result;
	// Simplest test - just return the value
	ASSERT_TEST(list_compute(newList, 5, retInt, &result) == 0);
	ASSERT_TEST(result == 10);

	// Another test
	ASSERT_TEST(list_compute(newList, 5, multiplyInt10, &result) == 0);
	ASSERT_TEST(result == 100);
	//checking data didn't change!
	ASSERT_TEST(*v[5] == 10);
	ASSERT_TEST(list_compute(newList, 5, tryChangeData, &result) == 0);
	ASSERT_TEST(v[5]!=NULL);

	// Check the list didn't change
	printList(newList);

	// Check failure - key dosent exist
	ASSERT_TEST(list_compute(newList, -1, multiplyInt10, &result) != 0);
	ASSERT_TEST(list_compute(NULL,1,multiplyInt10,&result)!=0);
	ASSERT_TEST(list_compute(newList,1,NULL,&result)!=0);
	ASSERT_TEST(list_compute(newList,1,multiplyInt10,NULL)!=0);
	for (i = 0; i < 10; i++) {
		free(v[i]);
	}
	list_free(newList);
	return true;
}

bool LIST_BATCH_TEST() {
	linked_list_t* newList = list_alloc();
	int* v[10], i;
	int* w[3];
	for(i = 0; i < 3; i++){
		w[i] = malloc(sizeof(int));
		*w[i] = i*1000000*(-1);
	}
	for (i = 0; i < 10; i++) {
		v[i] = malloc(sizeof(int));
		*v[i] = i*2;
	}
	//checking list insert with batch
	op_t* op = malloc(10 * sizeof(op_t));
	for (i = 0; i < 10; i++) {
		op[i].data = v[i];
		op[i].key = i;
		op[i].op = INSERT;
	}
	list_batch(newList, 10, op);
	for (i = 0; i < 10; i++) {
		ASSERT_TEST(op[i].result == 0);
	}
	printList(newList);
	printIntData(newList);
	for (i = 0; i < 10; i += 2) {
		op[i].op = REMOVE;
	}
	list_batch(newList, 10, op);
	for (i = 0; i < 10; i++) {
		if (i % 2 == 0)
			ASSERT_TEST(op[i].result == 0);
		else
			ASSERT_TEST(op[i].result != 0);
	}
	printList(newList);
	printIntData(newList);
	for (i = 0; i < 2; i++) {
		op[i].op = CONTAINS;
		op[i].key = i;
	}
	for (i = 2; i < 5; i++) {
		op[i].op = UPDATE;
		op[i].key = i;
		op[i].data = w[i-2];
	}
	for(i = 5; i < 10; i++){
		op[i].op = COMPUTE;
		op[i].key = i;
		op[i].compute_func = multiplyInt10;
		op[i].data = (void*)malloc(sizeof(int));
	}
	list_batch(newList,10,op);
	printf("%d\n", *(int*)op[5].data);
	printf("%d\n", *(int*)op[7].data);
	printf("%d\n", *(int*)op[9].data);
	printList(newList);
	printIntData(newList);
	for(i = 0; i < 3; i++){
		free(w[i]);
	}
	for (i = 0; i < 10; i++) {
		free(v[i]);
	}
	for(i = 5; i < 10; i++){
		free(op[i].data);
	}
	free(op);
	list_free(newList);
	return true;
}

int main() {
	RUN_TEST(LIST_ALLOC_TEST);
	RUN_TEST(LIST_INSERT_TEST);
	RUN_TEST(LIST_REMOVE_TEST);
	RUN_TEST(LIST_FIND_TEST);
	RUN_TEST(LIST_UPDATE_TEST);
	RUN_TEST(LIST_SPLIT_TEST);
	RUN_TEST(LIST_COMPUTE_TEST);
	RUN_TEST(LIST_BATCH_TEST);
	return 0;
}


