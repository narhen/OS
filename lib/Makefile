CC = gcc
CFLAGS = -Wall -nostdinc -I ../include -O2 -m32

OBJS = sync.o string.o

all: $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-$(RM) *.o
