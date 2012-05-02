CFLAGS= -g -Wall
OFLAGS= -c

all: squirrel nuts udpclient
squirrel: squirrel.o operations.o storage.o nose.o ft_recv.o help.o ft_send.o
	gcc $(CFLAGS) -o squirrel squirrel.o operations.o storage.o nose.o ft_recv.o help.o ft_send.o

nuts: nuts.o
	gcc $(CFLAGS) -o nuts nuts.o 

nuts.o: nuts.c
	gcc $(CFLAGS) $(OFLAGS) nuts.c

operations.o: operations.c storage.h nose.h ft_recv.h ft_send.h
	gcc $(CFLAGS) $(OFLAGS) operations.c

storage.o: storage.c storage.h
	gcc $(CFLAGS) $(OFLAGS) storage.c

squirrel.o: squirrel.c operations.h storage.h nose.h
	gcc $(CFLAGS) $(OFLAGS) squirrel.c

udpclient: udpclient.o
	gcc $(CFLAGS) -o udpclient udpclient.o 

udpclient.o: udpclient.c 
	gcc $(CFLAGS) $(OFLAGS) udpclient.c

nose.o: nose.c nose.h
	gcc $(CFLAGS) $(OFLAGS) nose.c

ft_recv.o: ft_recv.c ft_recv.h help.h nose.h storage.h
	gcc $(CFLAGS) $(OFLAGS) ft_recv.c

ft_send.o: ft_send.c ft_send.h help.h storage.h nose.h
	gcc $(CFLAGS) $(OFLAGS) ft_send.c

help.o: help.c help.h
	gcc $(CFLAGS) $(OFLAGS) help.c

clean:
	rm -f squirrel
	rm -f nuts
	rm -f udpclient
	rm -f *.o