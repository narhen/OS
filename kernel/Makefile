CC = gcc
LD = ld
AS = nasm

CFLAGS = -Wall -nostdinc -I ../include -O0 -m32 -g -DSYS_LOADPOINT=$(SYS_LOADPOINT)
ASFLAGS = -f elf32 -Ox -DSYS_LOADPOINT=$(SYS_LOADPOINT)
LDFLAGS = -x -T kernel.ld

KERNOBJS = start.o entry.o init.o page.o slab.o scheduler.o pid.o kthread.o \
	kpanic.o print.o interrupt.o

LIBPATH = ../lib
LIBS = $(LIBPATH)/string.o $(LIBPATH)/sync.o

DRIVERPATH = ../drivers
DRIVERS = $(DRIVERPATH)/keyboard.o $(DRIVERPATH)/disc.o

FSPATH = ../fs
FS = $(FSPATH)/fs.o

all: $(KERNOBJS)
	$(LD) $(LDFLAGS) $(KERNOBJS) $(LIBS) $(DRIVERS) $(FS) -o image.elf
	objcopy -S -O binary -R .comment -R .shstrtab image.elf image

page.o:
	$(CC) $(CFLAGS) -c page.c

.c.o:
	$(CC) $(CFLAGS) -c $<

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	-$(RM) image{,.elf} start_sym
	-$(RM) *.o
