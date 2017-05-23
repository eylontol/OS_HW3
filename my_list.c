
#include <pthread.h>
#include "my_list.h"

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
    if (!list) return;
    if(!set_bool_secured(&(list->valid), FALSE)) return;
    while(get_int_secured(&(list->nr_running)) != 1);
    Node* prev = list->head, *curr = list->head;
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
        
        // lock thread
        pthread_mutex_lock(&(t->m_read));
        pthread_mutex_lock(&(t->m_write));
        
        temp = ((linked_list_t *)(*(arr + i % n)))->head;
        
        while (temp->next)
            temp = temp->next;
        
        temp->next = t;
        inc_int_secured(&(((linked_list_t *)(*(arr + i % n)))->size));
        
        //unlock thread
        pthread_mutex_unlock(&(t->m_read));
        pthread_mutex_unlock(&(t->m_write));
    }
    
    list->head = NULL;
    goto out;

out_unlock:
    end_list_func(list);
out:
    return status == TRUE ? 0 : 1;
}

int list_remove(linked_list_t* list, int key) {
    
    bool status = FALSE;
    Node *t, *prev = NULL, *temp;
    if (!list || !list_find(list, key))
        goto out;
    
    start_list_func(list, &status);
    if (!status)
        goto out;
    
    list_for_each(list, t) {
        pthread_mutex_lock(&(t->m_read));
        if (!(t->key == key)){
            pthread_mutex_unlock(&(t->m_read));
            continue;
        }
        
lock_relevant_threads:
        // lock previous (if exists)
        if (prev) {
            pthread_mutex_lock(&(prev->m_read));
            pthread_mutex_lock(&(prev->m_write));
        }
        // lock next (if exists)
        if (t->next) {
            pthread_mutex_lock(&(t->next->m_read));
            pthread_mutex_lock(&(t->next->m_write));
        }
        // lock current
        pthread_mutex_lock(&(t->m_write));
        
remove:
        temp = t;
        // TODO: how to free data ?!
        free(t);
        if (prev)
            prev->next = t->next;
        else
            list->head = t->next;
        dec_int_secured(&(list->size));
        
unlock_relevant_threads:
        // unlock previous (if exists)
        if (prev) {
            pthread_mutex_unlock(&(prev->m_read));
            pthread_mutex_unlock(&(prev->m_write));
        }
        // unlock next (if exists)
        if (t->next) {
            pthread_mutex_unlock(&(t->next->m_read));
            pthread_mutex_unlock(&(t->next->m_write));
        }
        // unlock current
        pthread_mutex_unlock(&(t->m_write));
        pthread_mutex_unlock(&(t->m_read));
        
        // smart
        prev = t;
    }
    
    end_list_func(list);
out:
    return status == TRUE ? 0 : 1;
}

int list_size(linked_list_t* list) {
    int res = 0; // TODO: 0 or -1 ?
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
        
        goto out;
    }
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
            case CONTAINS:
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

int main(){
    
    return 0;
}
