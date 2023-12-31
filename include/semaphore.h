#include "adt/list.h"
#include "x86.h"

struct Semaphore {
    int count;
    ListHead queue;
};
typedef struct Semaphore Semaphore;

void new_sem(Semaphore *sem, int value);
void P(Semaphore *sem);
void V(Semaphore *sem); 
