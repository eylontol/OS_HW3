
#include <pthread.h>

#define start_list_func(l,status_p) \
                            do {    \
                                if (!get_bool_secured(&(l->valid))){\
                                    *status_p = 0;         \
                                    break;          \
                                }                   \
                                inc_int_secured(&(l->nr_running));  \
                                *status_p = 1;             \
                            }while(0)

#define end_list_func(l)    \
                            do {    \
                                dec_int_secured(&(l->nr_running));  \
                            }while(0)


typedef enum {TRUE, FALSE} bool;

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

static bool get_bool_secured(int_secured *p) {
    if (!p) return false;
    int res;
    pthread_mutex_lock(&(p->m_write));
    res = p->val;
    pthread_mutex_unlock(&(p->m_write));
    return res;
}

static void set_bool_secured(int_secured *p, bool val) {
    if (!p) return;
    pthread_mutex_lock(&(p->m_write));
    p->val = val;
    pthread_mutex_unlock(&(p->m_write));
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
static int get_int_secured(int_secured *p) {
    if (!p) return -1;
    int res;
    pthread_mutex_lock(&(p->m_write));
    res = p->val;
    pthread_mutex_unlock(&(p->m_write));
    return res;
}

static void inc_int_secured(int_secured *p) {
    if (!p) return -1;
    pthread_mutex_lock(&(p->m_read));
    pthread_mutex_lock(&(p->m_write));
    p->val++;
    pthread_mutex_unlock(&(p->m_read));
    pthread_mutex_unlock(&(p->m_write));
}

static void dec_int_secured(int_secured *p) {
    if (!p) return -1;
    pthread_mutex_lock(&(p->m_read));
    pthread_mutex_lock(&(p->m_write));
    p->val--;
    pthread_mutex_unlock(&(p->m_read));
    pthread_mutex_unlock(&(p->m_write));
}

#if 0
// Eylon
linked_list_t* list_alloc() {
    linked_list_t *l = kmalloc(sizeof(*l));
    if (!l)
        goto out;
    l->head = NULL;
    set_int_secured(&(l->size), 0);
    set_int_secured(&(l->nr_running), 0);
    set_bool_secured(&(l->valid), TRUE);
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
int list_size(linked_list_t* list);
int list_compute(linked_list_t* list, int key,
                 int (*compute_func) (void *), int* result);

// Yuval
void list_free(linked_list_t* list);
int list_insert(linked_list_t* list, int key, void* data);
int list_find(linked_list_t* list, int key);
int list_update(linked_list_t* list, int key, void* data);
void list_batch(linked_list_t* list, int num_ops, op_t* ops);
#endif

