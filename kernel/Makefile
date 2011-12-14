CC = gcc
LD = ld
AS = nasm

CFLAGS = -Wall -nostdinc -I ../include -O2 -m32
ASFLAGS = -f elf32 -Ox -DSYS_LOADPOINT=$(SYS_LOADPOINT)
LDFLAGS = -s -x -T kernel.ld

KERNOBJS = start.o init.o

all: $(KERNOBJS)
	$(LD) $(LDFLAGS) $(KERNOBJS)  -o start
	objcopy -O binary -R .comment -R .shstrtab start
	$(MAKE) -C ../build
	../build/mkimg ./start > image
	$(RM) ./start

.c.o:
	$(CC) $(CFLAGS) -c $<

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	-$(RM) image
	-$(RM) *.o
	$(MAKE) -C ../build clean
