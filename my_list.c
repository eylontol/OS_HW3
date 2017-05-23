
#include <pthread.h>

#define start_list_func(l,status_p) \
                            do {    \
                                if (!l || !get_bool_secured(&(l->valid))){\
                                    *status_p = 0;         \
                                    break;          \
                                }                   \
                                inc_int_secured(&(l->nr_running));  \
                                *status_p = 1;             \
                            }while(0)



#define end_list_func(l)    \
                            do {    \
                                if(l) dec_int_secured(&(l->nr_running));  \
                            }while(0)


typedef enum {FALSE, TRUE} bool;

typedef struct Node_t Node;
struct {
    int key;
    void *data;
    Node *next;
    pthread_mutex_t m_read, m_write;
}Node_t;

struct {
    Node *head;
    int_secured size, nr_running;
    bool_secured valid;
}linked_list_t;

typedef struct {
    bool val;
    pthread_mutex_t m_read, m_write;
}bool_secured;

static void init_bool_secured(bool_secured *p, bool val) {
    if (!p) return;
    p->val = val;
    pthread_mutex_init_protect(&(p->m_read));
    pthread_mutex_init_protect(&(p->m_write));
}

static bool get_bool_secured(bool_secured *p) {
    if (!p) return false;
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

typedef struct {
    int val;
    pthread_mutex_t m_read, m_write;
}int_secured;

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

static void dec_int_secured(int_secured *p) {
    if (!p) return;
    pthread_mutex_lock(&(p->m_read));
    pthread_mutex_lock(&(p->m_write));
    p->val--;
    pthread_mutex_unlock(&(p->m_read));
    pthread_mutex_unlock(&(p->m_write));
}

static void pthread_mutex_init_protect(pthread_mutex_t* mtx){
    pthread_mutexattr_t* attr;
    if (!mtx) return;
    pthread_mutexattr_init(attr, PTHREAD_PRIO_PROTECT);
    pthread_mutex_init(mtx, attr);
}

void list_free(linked_list_t* list){
    if (!list) return;
    if(!set_bool_secured(list->valid), FALSE) return;
    while(get_int_secured(list->nr_running));
    Node* prev = list->head, curr = list->head;
    do{
        curr = prev->next;
        free(prev);
        prev = curr;
    }while(prev);
    free(list);
}

int list_insert(linked_list_t* list, int key, void* data){ /* null ok? */
    bool valid;
    int res = 1;
    Node* curr, *new;
    start_list_func(list, &valid);
    if (!valid) goto out;
    if(list_find(list, key)) goto out_unlock;
    new = malloc(sizeof(*new()));
    if (!new) goto out_unlock;
    new->key=key; new->data = data;
    pthread_mutex_init_protect(&(new->m_read));
    pthread_mutex_init_protect(&(new->m_write));
    if (!list->head || list->head->key > key){
        new->next = list->head
        list->head = new;
        res = 0;
        inc_int_secured(list->size);
        goto out_unlock;
    }
    while (curr->next && curr->next->key < key) curr = curr->next;
    new->next = curr->next;
    curr->next = new;
    res = 0;
    inc_int_secured(list->size);
out_unlock:
    end_list_func(list);
out:
    return res;
    
}

int list_find(linked_list_t* list, int key){
    bool valid;
    int res = 0;
    start_list_func(list, &valid);
    if (!valid) goto out;
    Node* head = list->head;
    while(head && head->key < key) head = head->next;
    if (head && head->key == key) res = 1;
out_unlock:
    end_list_func(list);
out:
    return res;
}


#if 0
// Eylon
linked_list_t* list_alloc() {
    linked_list_t *l = malloc(sizeof(*l));
    if (!l)
        goto out;
    l->head = NULL;
    init_int_secured(&(l->size), 0);
    init_int_secured(&(l->nr_running), 0);
    init_bool_secured(&(l->valid), TRUE);
out:
    return l;
}

int list_split(linked_list_t* list, int n, linked_list_t** arr);
int list_remove(linked_list_t* list, int key) {
    
    int status = 1;
    if (!list || !list_find(list, key))
        goto out;
    
    start_list_func(l, &status);
    if (!status)
        goto out;
    
    
    
    end_list_func(l);
out:
    return status;
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
    }
out_unlock:
    end_list_func(list);
out:
    return res;
    
}

void list_batch(linked_list_t* list, int num_ops, op_t* ops)
{
    bool valid = FALSE;
    int tmp = 0;
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
            case FIND:
                ops->result = list_find(list, ops->key);
                break;
            case UPDATE:
                ops->result = list_update(list, ops->key, ops->data);
                break;
            case COMPUTE:
                ops->result = list_compute(list,ops->key, ops->compute_func, &tmp);
                if (ops->result == 0) ops->result = tmp;
                break;
        }
        ops++;
    }
out_unlock:
    end_list_func(list);
out:
    return;
}


