; boot.asm
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

struc _gdt
    .limit_lo:      resw 1 ; limit 0-15
    .base_lo:       resw 1 ; base 0-15
    .base_mid:      resb 1 ; base 16-23
    .access:        resb 1 ; access
    .lim_hi_flags:  resb 1 ; limit 16-19 (4 lsb), and flags (4 msb)
    .base_hi:       resb 1 ; base 24-31
endstruc

_start:
    cli
    push word   STACK_SEGMENT + STACK_SIZE
    push word   BOOT_SEGMENT
    pop  word   ds
    pop  word   ss
    push word   0x00
    pop         es

    call        mem_detect_e820
    mov dword   ecx, MEM_MAP_START - 4 ; store return value at this address
    mov dword   [es:ecx], eax ; es is 0 here

    mov  word   si, msg_loading
    call        print
    mov word    di, SYS_LOADPOINT ; destination -
    mov word    ax, 1 ; block to start read from
    mov word    cx, SYS_BLOCKS ; number of blocks to read
    call        read
    mov word    si, msg_done
    call        print

    mov dword   ebx, SYS_BLOCKS
    call        load_gdt
    ; enable A20 line
    in          al, 0x92
    or          al, 0x02
    out         0x92, al

    mov dword   eax, cr0
    or  dword   eax, 1
    mov dword   cr0, eax
    jmp         0x08:SYS_LOADPOINT

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

;print_hex:
;    push word   bp
;    mov  word   bp, sp
;    push dword  ecx
;    push word   dx
;    sub         sp, 0x100
;    lea         ecx, [bp - 0x08]
;    mov         dx, 0x00
;    mov byte    [ecx], 0x0d
;    dec         ecx
;    mov byte    [ecx], 0x0a
;    dec         ecx
;.loop:
;    mov         dx, ax
;    and         dx, 0x0f
;    cmp         dx, 0x0a
;    jge         .here
;    add         dx, 0x30
;    jmp         .skip
;.here:
;    sub         dx, 0x0a
;    add         dx, 0x61
;.skip:
;    mov byte    [ecx], dl
;    dec         cx
;    shr         ax, 4
;    test        ax, ax
;    jnz         .loop
;    mov byte    [ecx], 'x'
;    mov byte    [ecx - 1], '0'
;    push        si
;    lea         si, [ecx - 1]
;    call        print
;    pop         si
;    add         sp, 0x100
;    pop word    dx
;    pop dword   ecx
;    pop word    bp
;    ret

; Reads the memory map with BIOS interrupt 0x15.
; Store the list, starting at address 0x0100:0x0000.
; Return the number of elements in the list in eax. (each element is 24 bytes)
; on error -1 is returned.
;
; offset | what
; -------+-----------------
; 0      | address (qword)
; 8      | length in bytes (qword)
; 16     | type (dword) - 1: normal usable, 2: reserved unusable
;        |                3: ACPI reclaimable, 4: ACPI NVS memory, 5: bad memory
; 20     | ACPI 3.0 info (dword)
mem_detect_e820:
    mov dword   eax, 0xe820
    xor dword   ebx, ebx
    mov dword   edx, 0x534d4150 ; magic number
    mov dword   ecx, MEM_MAP_ELEM_SIZ
    push word   0x00
    push word   MEM_MAP_START
    pop  word   di
    pop  word   es
    int         0x15
    cmp dword   eax, edx
    jne         .error
    cmp dword   ebx, 0x00 ; list is only 1 element
    jne         .loop
    mov dword   eax, 1
    jmp         .return
.loop:
    add         di, MEM_MAP_ELEM_SIZ
    mov dword   eax, 0xe820
    mov dword   ecx, MEM_MAP_ELEM_SIZ
    int         0x15
    cmp dword   [di + 0x10], 0x05
    jne         .good
    mov dword   [di + 0x10], 0x02
.good:
    cmp dword   ebx, 0x00
    jne         .loop
    xor dword   eax, eax
    xor dword   edx, edx
    mov word    ax, di
    sub word    ax, MEM_MAP_START
    mov dword   ecx, MEM_MAP_ELEM_SIZ
    div         ecx
    jmp     .return
.error:
    mov dword   eax, -1
.return:
    clc ; clear carry bit. (set on last int 0x15)
    ret

dap:
istruc dapacket
    at dp_size, db dapacket_size
    at dp_zero, db 0
    at dp_numblocks, dw 0
    at dp_buffer_lo, dw 0
    at dp_buffer_hi, dw 0
iend

; read from disk.
; es:di address to write contents to
; ax: block to start read from (LBA)
; cx number of blocks to transfer
read:
    mov word    si, dap
    mov word    [si + dp_buffer_hi], es
    mov word    [si + dp_buffer_lo], di
    mov word    [si + dp_start_lba], ax
    mov word    [si + dp_numblocks], cx
    mov byte    ah, 0x42
    mov byte    dl, 0x80 ; drive number
    int         0x13
    jc          .error
    test        ah, ah ; ah is 0 on success
    jz          .return
    mov byte    ah, 0xbb ; something weird is going on - 0xbb means undefined error
    jmp         .return
.error:
    clc ; clear carry bit
.return:
    ret

load_gdt:
    xor dword   eax, eax
    mov word    ax, ds
    shl dword   eax, 4
    add dword   eax, gdt_start
    mov dword   [gdt_descriptor + 2], eax
    mov dword   eax, gdt_end
    sub dword   eax, gdt_start
    mov word    [gdt_descriptor], ax
    lgdt        [gdt_descriptor]
    ret

msg_loading: db "Loading kernel..", 0x0a, 0x0d
msg_done: db "Done.. Transferring control to kernel", 0x0a, 0x0d

gdt_descriptor:
    dw    0 ; size
    dd    0 ; address

gdt_start:
istruc  _gdt ; dummy
    at  _gdt.limit_lo,      dw 0x00
    at  _gdt.base_lo,       dw 0x00
    at  _gdt.base_mid,      db 0x00
    at  _gdt.access,        db 0x00
    at _gdt.lim_hi_flags,   db 0x00
    at _gdt.base_hi,        db 0x00
iend
istruc  _gdt ; ring 0 code
    at  _gdt.limit_lo,      dw 0xffff ; limit == 4 GiB
    at  _gdt.base_lo,       dw 0x0000 ; base == 0x00
    at  _gdt.base_mid,      db 0x00
    at  _gdt.access,        db 0x9a ; present, executable, readable
    at _gdt.lim_hi_flags,   db 0xcf ; flags: granularity == 4 KiB,
                                    ;        32-bit protected mode
    at _gdt.base_hi,        db 0x00
iend
istruc  _gdt ; ring 0 data
    at  _gdt.limit_lo,      dw 0xffff ; limit == 4 GiB
    at  _gdt.base_lo,       dw 0x0000 ; base == 0x00
    at  _gdt.base_mid,      db 0x00
    at  _gdt.access,        db 0x92 ; present, readable, writable
    at _gdt.lim_hi_flags,   db 0xcf ; flags: granularity == 4 KiB,
                                    ;        32-bit protected mode
    at _gdt.base_hi,        db 0x00
iend
gdt_end:

times 446 - ($-$$) db 0x00

db 0x80, 0x01, 0x01, 0x00, 0x83, 0xfe, 0xff, 0xff
db 0x3f, 0x00, 0x00, 0x00, 0x02, 0x03, 0xa5, 0x0b

times 64-16 db 0x00
db 0x55, 0xAA
