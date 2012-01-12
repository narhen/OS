OBJS = kernel.o boot.o

SYS_LOADPOINT = 0xd000
SLP = SYS_LOADPOINT=$(SYS_LOADPOINT)

image: $(OBJS)
	mv boot/boot ./image
	cat kernel/image >> image

boot.o: kernel.o
	$(MAKE) $(SLP) SYS_BLOCKS=$(shell echo `wc -c kernel/image | sed -e 's/[^0-9]//g'` / 512 | bc)  -C boot/

kernel.o:
	$(MAKE) -C lib
	$(MAKE) $(SLP) -C kernel

clean:
	$(MAKE) -C boot clean
	$(MAKE) -C kernel clean
	$(MAKE) -C lib clean
	-$(RM) image
