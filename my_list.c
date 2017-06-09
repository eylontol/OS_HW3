
#include <pthread.h>
#include <stdlib.h>
#include "my_list.h"
#include <stdio.h>
#include <stdlib.h>


#define start_list_func(l,status_p) \
                            do {    \
                                if (!l || !get_bool_secured(&(l->valid))){\
                                    *status_p = FALSE;         \
                                    break;          \
                                }                   \
                                inc_int_secured(&(l->nr_running));  \
                                *status_p = TRUE;             \
                            }while(0)



#define end_list_func(l)    \
                            do {    \
                                if(l) dec_int_secured(&(l->nr_running));  \
                            }while(0)

#define list_for_each(l, t)         \
                            for(t = l->head; t; t = t->next)
static void pthread_mutex_init_protect(pthread_mutex_t* mtx);

typedef enum {FALSE, TRUE} bool;

typedef struct {
    int val;
    pthread_mutex_t m_read, m_write;
}int_secured;

typedef struct {
    bool val;
    pthread_mutex_t m_read, m_write;
}bool_secured;

typedef struct Node_t Node;
struct Node_t{
    int key;
    void *data;
    Node *next;
    pthread_mutex_t m_read, m_write;
};

struct linked_list_t{
    Node *head;
    int_secured size, nr_running;
    bool_secured valid;
};

static void init_bool_secured(bool_secured *p, bool val) {
    if (!p) return;
    p->val = val;
    pthread_mutex_init_protect(&(p->m_read));
    pthread_mutex_init_protect(&(p->m_write));
}

static void destroy_bool_secured(bool_secured *p) {
    if (!p) return;
    pthread_mutex_destroy(&(p->m_read));
    pthread_mutex_destroy(&(p->m_write));
}

static bool get_bool_secured(bool_secured *p) {
    if (!p) return FALSE;
    int res;
    pthread_mutex_lock(&(p->m_write));
    res = p->val;
    pthread_mutex_unlock(&(p->m_write));
    return res;
}

static bool set_bool_secured(bool_secured *p, bool val) {
    bool status = FALSE;
    if (!p)
        goto out;
    pthread_mutex_lock(&(p->m_write));
    pthread_mutex_lock(&(p->m_read));
    if (!p->val)
        goto out_unlock;
    p->val = val;
    
    status = TRUE;
out_unlock:
    pthread_mutex_unlock(&(p->m_read));
    pthread_mutex_unlock(&(p->m_write));
out:
    return status;
}



static void set_int_secured(int_secured *p, int val) {
    if (!p) return;
    pthread_mutex_lock(&(p->m_write));
    p->val = val;
    pthread_mutex_unlock(&(p->m_write));
}

static void init_int_secured(int_secured *p, int val) {
    if (!p) return;
    p->val = val;
    pthread_mutex_init_protect(&(p->m_read));
    pthread_mutex_init_protect(&(p->m_write));
}

static void destroy_int_secured(int_secured *p) {
    if (!p) return;
    pthread_mutex_destroy(&(p->m_read));
    pthread_mutex_destroy(&(p->m_write));
}


static int get_int_secured(int_secured *p) {
    if (!p) return -1;
    int res;
    pthread_mutex_lock(&(p->m_write));
    res = p->val;
    pthread_mutex_unlock(&(p->m_write));
    return res;
}

static void inc_int_secured(int_secured *p) {
    if (!p) return;
    pthread_mutex_lock(&(p->m_read));
    pthread_mutex_lock(&(p->m_write));
    p->val++;
    pthread_mutex_unlock(&(p->m_read));
    pthread_mutex_unlock(&(p->m_write));
}


static void pthread_mutex_init_protect(pthread_mutex_t* mtx){
    pthread_mutexattr_t attr;
    if (!mtx) return;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
    pthread_mutex_init(mtx, &attr);
}



static void dec_int_secured(int_secured *p) {
    if (!p) return;
    pthread_mutex_lock(&(p->m_read));
    pthread_mutex_lock(&(p->m_write));
    p->val--;
    pthread_mutex_unlock(&(p->m_read));
    pthread_mutex_unlock(&(p->m_write));
}

void list_free(linked_list_t* list){
    bool valid;
    if (!list) return;
    start_list_func(list, &valid);
    if (!valid) return;
    if(!set_bool_secured(&(list->valid), FALSE)) return;
    while(get_int_secured(&(list->nr_running)) != 1);
    Node *prev = list->head, *curr = list->head;
    while(prev){
        curr = prev->next;
        pthread_mutex_destroy(&(prev->m_read));
        pthread_mutex_destroy(&(prev->m_write));
        free(prev);
        prev = curr;
    }
    destroy_int_secured(&(list->size));
    destroy_int_secured(&(list->nr_running));
    destroy_bool_secured(&(list->valid));
    free(list);
//    end_list_func(list);
}

int list_insert(linked_list_t* list, int key, void* data){ /* null ok? */
    bool valid;
    int res = 1;
    Node *curr, *new;
    start_list_func(list, &valid);
    if (!valid) goto out;
    if(list_find(list, key)) goto out_unlock;
    new = (Node*)malloc(sizeof(*new));
    if (!new) goto out_unlock;
    new->key=key; new->data = data;
    pthread_mutex_init_protect(&(new->m_read));
    pthread_mutex_init_protect(&(new->m_write));
    if (!list->head || list->head->key > key){
        new->next = list->head;
        list->head = new;
        res = 0;
        inc_int_secured(&(list->size));
        goto out_unlock;
    }
    curr = list->head;
    while (curr->next && curr->next->key < key) curr = curr->next;
    new->next = curr->next;
    curr->next = new;
    res = 0;
    inc_int_secured(&(list->size));
out_unlock:
    end_list_func(list);
out:
    return res;
    
}

int list_find(linked_list_t* list, int key){
    bool valid;
    int res = -1;
    start_list_func(list, &valid);
    if (!valid) goto out;
    res = 0;
    Node* head = list->head;
    while(head && head->key < key) head = head->next;
    if (head && head->key == key) res = 1;
    else res = 0;
out_unlock:
    end_list_func(list);
out:
    return res;
}

linked_list_t* list_alloc() {
    linked_list_t *l = (linked_list_t*)malloc(sizeof(*l));
    if (!l)
        goto out;
    l->head = NULL;
    init_int_secured(&(l->size), 0);
    init_int_secured(&(l->nr_running), 0);
    init_bool_secured(&(l->valid), TRUE);
out:
    return l;
}

int list_split(linked_list_t* list, int n, linked_list_t** arr) {
    bool status = FALSE;
    int i;
    Node *t, *temp;
    linked_list_t *temp_list;
    
    if (!list || !arr || n <= 0)
        goto out;
    
    start_list_func(list, &status);
    if (!status)
        goto out;
    if (!set_bool_secured(&(list->valid), FALSE))
        goto out_unlock;
    
    while(get_int_secured(&(list->nr_running)) != 1);
    
    // init lists
    for (i = 0; i < n; ++i) {
        *(arr + i) = list_alloc();
        if (!*(arr + i)) {
            while (i--)
                list_free(*(arr + i));
            goto out_unlock;
        }
    }
    
    status = TRUE;
    i = 0;
    
    // insert
    list_for_each(list, t) {
        
        temp_list = ((linked_list_t *)(*(arr + i % n)));
        temp = temp_list->head;
        
        list_insert(temp_list, t->key, t->data);

        

        i++;
    }
    
    list_free(list);
    
    goto out;

out_unlock:
    end_list_func(list);
out:
    return status == TRUE ? 0 : 1;
}

int list_remove(linked_list_t* list, int key) {
    
    bool status = FALSE;
    Node *t, *prev = NULL, *next;
    if (!list || !list_find(list, key))
        goto out;
    
    start_list_func(list, &status);
    if (!status)
        goto out;
    
    list_for_each(list, t) {
        
        next = t->next;
        
        // lock previous (if exists)
        if (prev) {
            pthread_mutex_lock(&(prev->m_write));
        }

        // lock next (if exists)
        if (next) {
            pthread_mutex_lock(&(next->m_read));
        }
        
        

        pthread_mutex_lock(&(t->m_read));
        
        if (!(t->key == key)){
            if (prev) {
                pthread_mutex_unlock(&(prev->m_write));
            }
            
            // lock next (if exists)
            if (next) {
                pthread_mutex_unlock(&(next->m_read));
            }
            pthread_mutex_unlock(&(t->m_read));
            prev = t;
            continue;
        }
        
        // lock write
        
        if (prev) {
            pthread_mutex_lock(&(prev->m_write));
        }
        if (next) {
            pthread_mutex_lock(&(next->m_write));
        }
        
        pthread_mutex_lock(&(t->m_write));
        
remove:
//        pthread_mutex_unlock(&(t->m_write));
//        pthread_mutex_unlock(&(t->m_read));
        pthread_mutex_destroy(&(t->m_read));
        pthread_mutex_destroy(&(t->m_write));
        free(t);
        
        if (prev)
            prev->next = next;
        else
            list->head = next;
        
        dec_int_secured(&(list->size));
        
        
        // unlock previous (if exists)
        if (prev) {
            pthread_mutex_unlock(&(prev->m_read));
            pthread_mutex_unlock(&(prev->m_write));
        }
        // unlock next (if exists)
        if (next) {
            pthread_mutex_unlock(&(next->m_read));
            pthread_mutex_unlock(&(next->m_write));
        }
        
        goto out_unlock;
    }
out_unlock:
    end_list_func(list);
out:
    return status == TRUE ? 0 : 1;
}

int list_size(linked_list_t* list) {
    int res = -1;
    bool status = FALSE;
    if (!list)
        goto out;
    start_list_func(list, &status);
    if(!status)
        goto out;
    res = get_int_secured((&(list->size)));
    end_list_func(list);
out:
    return res;
}
int list_compute(linked_list_t* list, int key,
                 int (*compute_func) (void *), int* result) {
    bool status = FALSE;
    if (!list || !compute_func || !result || !list_find(list, key))
        goto out;
    
    start_list_func(list, &status);
    if (!status)
        goto out;
    
    status = TRUE;
    
    Node *t;
    list_for_each(list, t) {
        pthread_mutex_lock(&(t->m_read));
        if (!(t->key == key)){
            pthread_mutex_unlock(&(t->m_read));
            continue;
        }
        pthread_mutex_lock(&(t->m_write));
        
        *result = compute_func(t->data);
        
        pthread_mutex_unlock(&(t->m_read));
        pthread_mutex_unlock(&(t->m_write));
        
        goto out_unlock;
    }
out_unlock:
    end_list_func(list);
out:
    return status == TRUE ? 0 : 1;
}

int list_update(linked_list_t* list, int key, void* data)
{
    bool valid = FALSE;
    int res = 1;
    Node* curr;
    start_list_func(list, &valid);
    if (valid == FALSE) goto out;
    curr = list->head;
    while (curr){
        if (curr->key == key){
            curr->data = data;
            res = 0;
            goto out_unlock;
        }
        curr = curr->next;
    }
out_unlock:
    end_list_func(list);
out:
    return res;
}



void list_batch(linked_list_t* list, int num_ops, op_t* ops)
{
    bool valid = FALSE;
    int tmp;
    start_list_func(list, &valid);
    if (valid == FALSE) goto out;
    if (num_ops <= 0) goto out_unlock;
    while (num_ops-- > 0){
        switch(ops->op){
            case INSERT:
                ops->result = list_insert(list, ops->key, ops->data);
                break;
            case REMOVE:
                ops->result = list_remove(list, ops->key);
                break;
            case CONTAINS:
                ops->result = list_find(list, ops->key);
                break;
            case UPDATE:
                ops->result = list_update(list, ops->key, ops->data);
                break;
            case COMPUTE:
                ops->result = list_compute(list,ops->key, ops->compute_func, &tmp);
                *((int *)(ops->data)) = tmp;
                break;
        }
        ops++;
    }
out_unlock:
    end_list_func(list);
out:
    return;
}

