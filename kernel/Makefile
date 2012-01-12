CC = gcc
LD = ld
AS = nasm

CFLAGS = -Wall -nostdinc -I ../include -O2 -m32 -g
ASFLAGS = -f elf32 -Ox -DSYS_LOADPOINT=$(SYS_LOADPOINT)
LDFLAGS = -x -T kernel.ld

KERNOBJS = start.o init.o page.o

all: $(KERNOBJS)
	$(LD) $(LDFLAGS) $(KERNOBJS) ../lib/string.o  -o start_sym
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
	-$(RM) image
	-$(RM) *.o
	$(MAKE) -C ../build clean
