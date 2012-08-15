AS = nasm
ASFLAGS = -f bin -Ox -DSYS_LOADPOINT=$(SYS_LOADPOINT)

boot: stage2.o stage1.o
	@cat stage1 stage2 > boot

stage2.o: fat.asm
	$(AS) $(ASFLAGS) -o boot.tmp boot.asm
	@../build/mkimg ./boot.tmp > stage2
	-$(RM) boot.tmp

stage1.o:
	$(AS) $(ASFLAGS) -DSTAGE2_SIZE=$(shell echo `wc -c stage2 | cut -f1 -d ' '` / 512 | bc) -o stage1.tmp stage1.asm
	@../build/mkimg ./stage1.tmp > stage1
	-$(RM) stage1.tmp
	
test: boot
	qemu -hda $?

clean:
	-$(RM) boot stage1 stage2
