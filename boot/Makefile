AS = nasm
ASFLAGS = -f bin -Ox -DSYS_LOADPOINT=$(SYS_LOADPOINT) -DSYS_BLOCKS=$(SYS_BLOCKS)

boot: boot.asm

%: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

test: boot
	qemu -hda $?

clean:
	-$(RM) boot
