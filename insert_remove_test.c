//
//  shafik_list_test.c
//  
//
//  Created by Shafik Nassar on 04/06/2017.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "my_list.h"

#define ASSERT_NON_ZERO(b) do { \
if ((b) == 0) { \
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
#define RUN_TEST(test) do { \
fprintf(stdout, "Running "#test"... "); \
if (test()) { \
fprintf(stdout, "[OK]\n");\
} else { \
fprintf(stdout, "[Failed]\n"); \
} \
} while(0)

// you can change the number of threads you want to test, which is equal to the number of inital elements we insert
#define NUM_THREADS     200


/* add this function to my_list.c and declare it in my_list.h so we can use it here - it's really good for debugging :)
    ** of course, make sure that the field/struct names match those that you have defined in your structs or just write your own list_print function
    you can simply go over the test and uncomment the calls for the function
 
 void list_print(linked_list_t* l){
        node* it=l->head;
        printf("\n Printing list: \n");
        for(int i=0; i<l->size; ++i){
            printf("key is: %d data is: %d\n", it->key, *(int*)(it->data));
            it=it->next;
        }
 }
 */

static int div20(void* data){
    int *d = (int*)data;
    return (*d/10);
}

bool insert_remove_test() {
    //init data for the list
    int a[NUM_THREADS];
    for(int i=0; i<NUM_THREADS; ++i)
        a[i] = 10*i;
    
    linked_list_t* list = list_alloc();
    
    //init data for the insert batch function
    op_t oper;
    if(!list) return false;
    op_t* insert_ops = (op_t*) malloc(sizeof(*insert_ops) * NUM_THREADS);
    if(insert_ops == NULL){ list_free(list); return false; }
    for(int i=0; i<NUM_THREADS; ++i) {
        oper.key = i;
        oper.data = a+i;
        oper.op = INSERT;
        oper.compute_func = div20;
        oper.result=-10;
        insert_ops[i] = oper;
    }
    list_batch(list, NUM_THREADS, insert_ops);
    ASSERT_TEST(list_size(list) == NUM_THREADS);
    
    //list_print(list);
    //printf("\n list size is %d \n", list_size(list));
    
    int b[NUM_THREADS];
    
    
    //init data for the remove batch function
    op_t* remove_ops = (op_t*) malloc(sizeof(*insert_ops) * NUM_THREADS);
    if(remove_ops == NULL){ list_free(list); return false; }
    for(int i=0; i<NUM_THREADS; ++i) {
        oper.key = i;
        oper.data = b+i; // has uninitialized value, but doesn't matter
        oper.op = REMOVE;
        oper.compute_func = div20;
        oper.result=-10;
        remove_ops[i] = oper;
    }
    list_batch(list, NUM_THREADS, remove_ops);
    ASSERT_TEST(list_size(list) == 0);
    //list_print(list);
    
    
    // insert again, now we will remove part of the nodes
    list_batch(list, NUM_THREADS, insert_ops);
    ASSERT_TEST(list_size(list) == NUM_THREADS);
    // removing the even keys
    for(int i=0; i<NUM_THREADS/2; ++i) {
        oper.key = i*2;
        oper.data = b+i; // has uninitialized value, but doesn't matter
        oper.op = REMOVE;
        oper.compute_func = div20;
        oper.result=-10;
        remove_ops[i] = oper;
    }
    list_batch(list, NUM_THREADS/2, remove_ops);
    ASSERT_TEST(list_size(list) == NUM_THREADS/2);

    //list_print(list);
    
    //now we will test a mixed batch of inserts and removes
    op_t* mix_ops = (op_t*) malloc(sizeof(*mix_ops) * NUM_THREADS);
    if(mix_ops == NULL){ list_free(list); return false; }
    for(int i=0; i<NUM_THREADS; ++i) {
        oper.key = i;
        oper.data = a+i; // has uninitialized value, but doesn't matter
        oper.op = (i%2? REMOVE : INSERT); //inserting the evens and removing the odds
        oper.compute_func = div20;
        oper.result=-10;
        mix_ops[i] = oper;
    }
    list_batch(list, NUM_THREADS, mix_ops);
    ASSERT_TEST(list_size(list) == NUM_THREADS/2);
    //list_print(list);
    
    return true;
    
}

int main(){
    RUN_TEST(insert_remove_test);
    return 0;
}

