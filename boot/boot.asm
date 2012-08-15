; boot.asm
;

; functions from this file
global _start, copy, read

; functions from fat.asm
global fat_load, fat_read

section .text

BITS 16

%define STAGE2_SEGMENT 0x0d00
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
    push word   ax ; contains size of stage2 bootloader, in blocks
    xor         eax, eax
    push word   0x100
    push word   STAGE2_SEGMENT
    pop  word   ds
    pop  word   es


    mov  word   si, msg_loading
    call        print

    mov         edi, 0x1000
    mov         eax, 0
    mov         cx, 1
    call        read

    mov         esi, 0x1be ; address to the first entry in the partition table
    call        fat_load
    cmp         eax, 1
    jnz         error


    ; enter unreal mode
    push        ds
    in          al, 0x92
    or          al, 0x02
    out         0x92, al
    call        load_gdt
    mov dword   eax, cr0
    or  dword   eax, 1
    mov dword   cr0, eax
    mov         bx, 0x10
    mov         ds, bx
    and         al, 0xfe
    mov         cr0, eax
    pop         ds

    ; load the kernel
    call        fat_read ; returns 0 on success
    test        eax, eax
    jnz         error

    call        mem_detect_e820
    mov dword   ecx, MEM_MAP_START - 4 ; store return value at this address
    mov dword   [es:ecx], eax ; es is 0 here

    mov word    si, msg_done
    call        print

    call        load_gdt

    ; enable protected mode
    mov dword   eax, cr0
    or  dword   eax, 1
    mov dword   cr0, eax

    ; jump to kernel
    mov word    [0x00], 0xd0ff ; \xff\xd0 == call eax
    mov         eax, SYS_LOADPOINT
    mov         ebx, 0
    jmp         0x08:0xd000

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

; read from disk.
; edi: address to write contents to
; eax: block to start read from (LBA)
; cx: number of blocks to transfer
read:
    push        eax
    push        edi
    push        ebx
    push        edx
    push        esi
    mov         edx, edi
    mov         edi, 0xc000

.loop:
    call        load_dap ; prepare the Disk Address Packet
    push        eax
    push        edx

    mov word    ax, 0x4200
    mov byte    dl, 0x80 ; drive number
    int         0x13
    jc          error ; carry flag is set on error

    pop         edx
    mov         esi, edi
    call        copy ; increments edx as a side effect
    pop         eax
    inc         eax
    dec         ecx

    cmp         ecx, 0
    jne         .loop

    mov         eax, 0
    pop         esi
    pop         edx
    pop         ebx
    pop         edi
    pop         eax
    ret


error:
    mov         si, msg_error
    call        print
    hlt

; edi: address
; eax: block to read from
load_dap:
    push        edx

    mov         edx, edi
    mov word    si, dap
    mov word    [si + dp_buffer_lo], dx
    sar         edx, 16
    mov word    [si + dp_buffer_hi], dx
    mov dword   [si + dp_start_lba], eax
    mov word    [si + dp_numblocks], 1

    pop         edx
    ret


; copy 512 bytes from source to destination.
; edx: destination
; esi: source
copy:
    push        ds
    push        ecx
    push        edi
    push        eax

    push        0
    pop         ds
    mov         ecx, 512

.loop:
    mov dword   eax, [esi]
    mov byte    [edx], al
    inc         esi
    inc         edx
    dec         ecx
    cmp         ecx, 0
    jne         .loop

    pop     eax
    pop     edi
    pop     ecx
    pop     ds
    ret

%include "fat.asm"

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
msg_done: db "Done.. Starting kernel", 0x0a, 0x0d
msg_error: db "There was an error loading the kernel!", 0x0a, 0x0d

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
