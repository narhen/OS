OBJS = drivers.o fs.o kernel.o

SYS_LOADPOINT = 0x100000
SLP = SYS_LOADPOINT=$(SYS_LOADPOINT)

image: $(OBJS)
	@mv kernel/image kernel.img

boot: build.o
	$(MAKE) $(SLP) -C boot

build.o:
	$(MAKE) -C build

kernel.o:
	$(MAKE) -C lib
	$(MAKE) $(SLP) -C kernel

drivers.o:
	$(MAKE) -C drivers

fs.o:
	$(MAKE) -C fs

clean:
	$(MAKE) -C boot clean
	$(MAKE) -C kernel clean
	$(MAKE) -C lib clean
	$(MAKE) -C drivers clean
	$(MAKE) -C build clean
	$(MAKE) -C fs clean
	-$(RM) kernel.img
