; stage1.asm
;

global _start
section .text

BITS 16

%define BOOT_SEGMENT 0x07c0
%define STACK_SEGMENT 0x0700
%define STACK_SIZE 4096

%define MEM_MAP_START 0x1004
%define MEM_MAP_ELEM_SIZ 0x18 ; acpi adds an additional dword

struc dapacket
    dp_size:        resb 1 ; size of packet
    dp_zero:        resb 1 ; always 0
    dp_numblocks:   resw 1 ; number of blocks to transfer
    dp_buffer_lo:   resw 1 ; transfer buffer (real mode addressing)
    dp_buffer_hi:   resw 1
    dp_start_lba:   resq 1 ; starting absolute block number (LBA)
endstruc

_start:
    cli
    push word   STACK_SEGMENT + STACK_SIZE
    push word   BOOT_SEGMENT
    pop  word   ds
    pop  word   ss
    push word   0x00
    pop         es

    mov  word   si, msg_loading
    call        print

    call        load_stage2
    cmp         eax, 0
    jnz         .error
    mov         ecx, 0xd000
    mov         eax, STAGE2_SIZE
    call        ecx

    .error:
    mov word    si, msg_error
    call        print
    hlt


; Prints a string to screen.
; The string-address must be loaded in si
print:
    mov byte    ah, 0x0e
    mov word    bx, 0x0007
.loop:
    lodsb
    int         0x10
    cmp byte    al, 0x0d
    jne         .loop
    ret

; read from disk.
; es:di address to write contents to
; ax: block to start read from (LBA)
; cx number of blocks to transfer
load_stage2:
    mov word    si, dap
    mov word    [si + dp_buffer_hi], 0
    mov word    [si + dp_buffer_lo], 0xd000
    mov word    [si + dp_start_lba], 1
    mov word    [si + dp_numblocks], STAGE2_SIZE
    mov word    ax, 0x4200
    mov byte    dl, 0x80 ; drive number
    int         0x13
    jc          .error
    test        ah, ah ; ah is 0 on success
    jz          .return
    mov byte    ah, 0xbb ; something weird is going on - 0xbb means undefined error
    jmp         .return
.error:
    clc ; clear carry bit
    mov         eax, 1
.return:
    ret

msg_loading: db "Loading stage2 bootloader..", 0x0a, 0x0d
msg_error: db "There was an error when loading stage2 bootloader..", 0x0a, 0x0d

align 4
dap: ; structure used for transferring bytes from disk into memory
istruc dapacket
    at dp_size, db dapacket_size
    at dp_zero, db 0
    at dp_numblocks, dw 0
    at dp_buffer_lo, dw 0
    at dp_buffer_hi, dw 0
    at dp_start_lba, dq 0
iend



times 446 - ($-$$) db 0x00

; A FAT32 partition. start = 62 (lba), end= 62 + 0x776b92 = 7826384
db 0x80, 0x21, 0x03, 0x00, 0x0b, 0x53, 0xd6, 0xfa
db 0x00, 0x08, 0x00, 0x00, 0x00, 0x78, 0x77, 0x00


times 64-16 db 0x00
db 0x55, 0xaa
