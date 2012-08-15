CC = gcc
CFLAGS = -Wall -nostdinc -I ../include -O0 -m32 -g

OBJS = fs.o fat32.o

all: $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-$(RM) *.o
