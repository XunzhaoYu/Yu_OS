#include "x86.h"
#include "adt/list.h"

#define MSG_HWINTR -1
#define ANY -1
struct Message {
pid_t src, dest;
int type;
char payload[MSG_SZ];
ListHead getq,freeq;
};
typedef struct Message Message;

void copy_message(Message* dst,Message* src);
void send(pid_t dst, Message *m);
void receive(pid_t dst, Message *m);
