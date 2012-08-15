CC = gcc
LD = ld
AS = nasm

CFLAGS = -Wall -nostdinc -I ../include -O0 -m32 -g -DSYS_LOADPOINT=$(SYS_LOADPOINT)
ASFLAGS = -f elf32 -Ox -DSYS_LOADPOINT=$(SYS_LOADPOINT)
LDFLAGS = -x -T kernel.ld

OBJS = keyboard.o disc.o

LIBPATH = ../lib
LIBS = $(LIBPATH)/string.o $(LIBPATH)/sync.o

all: $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	-$(RM) *.o
