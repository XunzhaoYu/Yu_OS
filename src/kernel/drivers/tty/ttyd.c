#include "debug.h"
#include "tty.h"
#include "irq.h"

void
ttyd(void) {
	Message m;
	int i;

	lock();
	for (i = 0; i < NR_TTY; i ++) {
		hal_register(ttynames[i], TTY, i);
	}
	unlock();

	while (1) {
		receive(ANY, &m);
		if (m.src == MSG_HWINTR) {
			//printk("MSG_HWINTR!\n");
			switch (m.type) {
				case MSG_TTY_GETKEY:
					readkey();
					break;
				case MSG_TTY_UPDATE:
					update_banner();
					break;
			}
		} else {
			DevMessage *msg;
			switch(m.type) {
				case MSG_DEVRD:
					//printk("read!\n");
					read_request((DevMessage*)&m);
					break;
				case MSG_DEVWR:
					//printk("write!\n");
					msg = (DevMessage*)&m;
					if (msg->dev_id >= 0 && msg->dev_id < NR_TTY) {
						printk("device id :%d!\n",msg->dev_id);
						int i;
						for (i = 0; i < msg->count; i ++) {
							consl_writec(&ttys[msg->dev_id], ((char*)msg->buf)[i]);
						}
						consl_sync(&ttys[msg->dev_id]);
					}
					m.type = msg->count;
					//printk("device id :%d!\n",msg->dev_id);
					send(m.src, &m);
					break;
			}
		}
	}
}

