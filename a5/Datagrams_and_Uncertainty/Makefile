CFLAGS= -g

all: send1 recv1 send2 recv2 recv3 send-reliable recv-reliable recv-unreliable send-broadcast recv-broadcast primary

clean: 
	rm -rf send1 recv1 send2 recv2 recv3 send-reliable recv-reliable recv-unreliable send-broadcast recv-broadcast primary
primary: primary.c shared.h halligan.o halligan.h
	gcc $(CFLAGS) primary.c halligan.o -o primary
send1: send1.c shared.h
	gcc $(CFLAGS) send1.c  -o send1
send2: send2.c shared.h
	gcc $(CFLAGS) send2.c  -o send2
send-broadcast: send-broadcast.c shared.h
	gcc $(CFLAGS) send-broadcast.c halligan.o -o send-broadcast
recv-broadcast: recv-broadcast.c shared.h
	gcc $(CFLAGS) recv-broadcast.c halligan.o -o recv-broadcast
	
recv1: recv1.c shared.h halligan.h halligan.o
	gcc $(CFLAGS) recv1.c halligan.o -o recv1
recv2: recv2.c shared.h
	gcc $(CFLAGS) recv2.c halligan.o -o recv2
recv3: recv3.c shared.h
	gcc $(CFLAGS) recv3.c halligan.o -o recv3
send-reliable: send-reliable.c shared.h halligan.h halligan.o 
	gcc $(CFLAGS) send-reliable.c halligan.o -o send-reliable
recv-reliable: recv-reliable.c shared.h halligan.h halligan.o 
	gcc $(CFLAGS) recv-reliable.c halligan.o -o recv-reliable
recv-unreliable: recv-unreliable.c shared.h
	gcc $(CFLAGS) recv-unreliable.c halligan.o -o recv-unreliable

halligan.o: halligan.c
