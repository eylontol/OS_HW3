
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
                            }while(0);



#define end_list_func(l)    \
                            do {    \
                                if(l) dec_int_secured(&(l->nr_running));  \
                            }while(0);

#define list_for_each(l, t)         \
                            for(t = l->head; t; t = t->next)

#define lock_node(n)        do {pthread_mutex_lock(&((n)->m));} while(0);

#define unlock_node(n)      do {pthread_mutex_unlock(&((n)->m));} while(0);

static void pthread_mutex_init_protect(pthread_mutex_t* mtx);

typedef enum {FALSE, TRUE} bool;

typedef struct {
    op_t* op;
    linked_list_t* list;
}argToFunc;

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
    pthread_mutex_t m;
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

int call_func(argToFunc* arg){
    int tmp;
    switch (((op_t *)(arg->op))->op){
        case INSERT:
            ((op_t *)(arg->op))->result = list_insert(arg->list, ((op_t *)(arg->op))->key, ((op_t *)(arg->op))->data);
            break;
        case REMOVE:
            ((op_t *)(arg->op))->result = list_remove(arg->list, ((op_t *)(arg->op))->key);
            break;
        case CONTAINS:
            ((op_t *)(arg->op))->result = list_find(arg->list, ((op_t *)(arg->op))->key);
            break;
        case UPDATE:
            ((op_t *)(arg->op))->result = list_update(arg->list, ((op_t *)(arg->op))->key, ((op_t *)(arg->op))->data);
            break;
        case COMPUTE:
            ((op_t *)(arg->op))->result = list_compute(arg->list,((op_t *)(arg->op))->key, ((op_t *)(arg->op))->compute_func, &tmp);
            ((op_t *)(arg->op))->data = (void *)tmp;
            break;
    }
    return 0;
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
    Node *prev, *curr;
    if (!list) return;
    start_list_func(list, &valid);
    if (!valid) return;
    if(!set_bool_secured(&(list->valid), FALSE)) return;
    while(get_int_secured(&(list->nr_running)) != 1);
    prev = list->head;
    curr = list->head;
    while(prev){
        curr = prev->next;
        pthread_mutex_destroy(&(prev->m));
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
    Node *curr, *new, *prev;
    start_list_func(list, &valid);
    if (!valid) goto out;
    if(list_find(list, key)) goto out_unlock;
    new = (Node*)malloc(sizeof(*new));
    if (!new) goto out_unlock;
    new->key = key;
    new->data = data;
    new->next = NULL;
    pthread_mutex_init_protect(&(new->m));
    
    lock_node(new);
    
    prev = NULL;
    if (!(list->head)) {
        list->head = new;
    }else {
        curr = list->head;
        lock_node(curr);
        if (curr->key >= key) {
            new->next = curr;
            list->head = new;
            unlock_node(curr);
        }else {
            while(curr) {
                if (curr->key >= key) {
                    
                    new->next = curr;
                    
                    if (prev) {
                        prev->next = new;
                        unlock_node(prev);
                    }
                    unlock_node(curr);
                    
                    goto update_list;
                }
                
                if(prev) unlock_node(prev);
                
                if (curr->next) lock_node(curr->next)
                else {
                    curr->next = new;
                    unlock_node(curr);
                    goto update_list;
                }
                
                prev = curr;
                curr = curr->next;
            }
        }
    }
update_list:
    unlock_node(new);
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
    Node* head;
    start_list_func(list, &valid);
    if (!valid) goto out;
    
    res = 0;
    
    if (!(list->head)) goto out_unlock;
    
    head = list->head;
    lock_node(head);
    
    do{
        if (head->key >= key) {
            if (head->key == key) res = 1;
            else res = 0;
            unlock_node(head);
            goto out_unlock;
        }
        if (head->next) lock_node(head->next);
        unlock_node(head);
        head = head->next;
    }while (head);
    
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
    Node *t, *prev = NULL, *next = NULL;
    if (!list || !list_find(list, key) || !(list->head))
    {
        goto out;
    }
    
    start_list_func(list, &status);
    if (!status)
        goto out;
    
    t = list->head;
    lock_node(t);
    
    while(t) {
        next = t->next;
        
        if (t->key == key) {
            
            if (next) lock_node(next);
            
            if (prev) prev->next = next;
            else list->head = next;
            
            pthread_mutex_destroy(&(t->m));
            free(t);
            dec_int_secured(&(list->size));
            
            if (prev) unlock_node(prev);
            if (next) unlock_node(next);
            
            goto out_unlock;
        }
        
        if (prev) unlock_node(prev);
        prev = t;
        t = t->next;
        if (!t) unlock_node(prev);
        
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
        lock_node(t);
        if (!(t->key == key)){
            unlock_node(t);
            continue;
        }

        *result = compute_func(t->data);
        
        unlock_node(t);
        
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
    if (!(list->head)) goto out_unlock;
    curr = list->head;
    lock_node(curr);
    while (curr){
        if (curr->key == key){
            curr->data = data;
            res = 0;
            unlock_node(curr);
            goto out_unlock;
        }
        if (curr->next) lock_node(curr->next);
        unlock_node(curr);
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
    int i;
    pthread_t *thread;
    argToFunc *args;
    
    start_list_func(list, &valid);
    if (valid == FALSE) goto out;
    
    if (num_ops <= 0) goto out_unlock;
    
    thread = (pthread_t *)malloc (num_ops * sizeof(*thread));
    if (!thread) goto out_unlock;
    
    args = (argToFunc *)malloc(num_ops * sizeof(*args));
    if (!args) {
        free(thread);
        goto out_unlock;
    }

    for (i = 0; i < num_ops; i++) {
        args[i].list = list;
        args[i].op = &ops[i];
        pthread_create(&thread[i], NULL, call_func, &args[i]);
    }
    for (i = 0; i < num_ops; i++) {
        pthread_join(thread[i], NULL);
    }
    
    free (thread);
    free (args);
    
out_unlock:
    end_list_func(list);
out:
    return;
}
