CC = gcc
LD = ld
AS = nasm

CFLAGS = -Wall -nostdinc -I ../include -O2 -m32 -g -DSYS_LOADPOINT=$(SYS_LOADPOINT)
ASFLAGS = -f elf32 -Ox -DSYS_LOADPOINT=$(SYS_LOADPOINT)
LDFLAGS = -x -T kernel.ld

KERNOBJS = start.o entry.o init.o page.o slab.o scheduler.o pid.o kthread.o \
	kpanic.o print.o interrupt.o

LIBPATH = ../lib
LIBS = $(LIBPATH)/string.o $(LIBPATH)/sync.o

all: $(KERNOBJS)
	$(LD) $(LDFLAGS) $(KERNOBJS) $(LIBS) -o start_sym
	objcopy -O binary -R .comment -R .shstrtab start_sym start
	$(MAKE) -C ../build
	../build/mkimg ./start > image
	$(RM) ./start

page.o:
	$(CC) $(CFLAGS) -c page.c

.c.o:
	$(CC) $(CFLAGS) -c $<

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	-$(RM) image start_sym
	-$(RM) *.o
	$(MAKE) -C ../build clean
