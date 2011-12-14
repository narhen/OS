; start.asm - 32-bit kernel startup-code.
;
; protected mode is enabled at this point.
;
;

global _start
extern init

section .text

BITS 32

%define SCREEN_ADDR 0xb8000
%define PAGE_DIRECTORY 0x8000

%if PAGE_DIRECTORY + 4096 * 5 - 1 >= SYS_LOADPOINT
    %error "Not enough space for page directories and tables!"
%endif

struc _gdt
    .limit_lo:      resw 1 ; limit 0-15
    .base_lo:       resw 1 ; base 0-15
    .base_mid:      resb 1 ; base 16-23
    .access:        resb 1 ; access
    .lim_hi_flags:  resb 1 ; limit 16-19 (4 lsb), and flags (4 msb)
    .base_hi:       resb 1 ; base 24-31
endstruc

_start:
    mov     esp, 0x7000
    mov     ax, 0x10
    mov     ss, ax
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    call    clear_screen
    mov     esi, msg_welcome
    call    print
    call    setup_paging
    call    load_gdt
    ; reload segment regsters after we have changed gdt
    mov     ax, 0x10
    mov     ss, ax
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    call    available_ram
    push    eax ; third argument
    push dword 0x1004 ; second argument
    push dword 0x1000 ; first argument
    call    init
    jmp     $ ; Should really never return here

; set up paging and 1 to 1 mapping for the first 16 MB of RAM
setup_paging:
%define PERMISSION_BITS 0x07
    mov dword   [PAGE_DIRECTORY], (PAGE_DIRECTORY + 4096)
    or  dword   [PAGE_DIRECTORY], PERMISSION_BITS
    mov dword   [PAGE_DIRECTORY + 4], PAGE_DIRECTORY + (4096 * 2)
    or  dword   [PAGE_DIRECTORY + 4], PERMISSION_BITS
    mov dword   [PAGE_DIRECTORY + 8], PAGE_DIRECTORY + (4096 * 3)
    or  dword   [PAGE_DIRECTORY + 8], PERMISSION_BITS
    mov dword   [PAGE_DIRECTORY + 12], PAGE_DIRECTORY + (4096 * 4)
    or  dword   [PAGE_DIRECTORY + 12], PERMISSION_BITS

    mov     eax, 0xfff007 ; 16 MB
    mov     edi, PAGE_DIRECTORY + (4096 * 5) - 4 ; last entry in last page table
    std ; fill the tables backwards
.loop: ; map the 16 first megabytes
    stosd
    sub     eax, 4096
    jns     .loop ; while eax > 0 then jmp .loop

    mov     eax, PAGE_DIRECTORY
    mov     cr3, eax
    mov     eax, cr0
    or      eax, 0x80000000 ; set the PE flag in cr0
    mov     cr0, eax
    ret

db 0
print:
    xor     edx, edx
    mov     dl, [print - 1]
    mov     eax, edx
    inc     edx
    cmp     dl, 26
    jle     .over
    mov     dl, 0
.over:
    mov     [print - 1], dl
    mov     edx, 80
    mul     edx
    shl     eax, 1
    mov     ecx, eax
    mov     eax, 0x0100
.loop:
    lodsb
    cmp     al, 0x00
    je      .return
    mov     [SCREEN_ADDR + ecx], ax
    add     ecx, 2
    inc     ah
    cmp     ah, 0x10
    jne     .loop
    mov     ah, 1
    jmp     .loop
.return:
    ret

clear_screen:
    mov     ecx, 25*80+80
    mov     eax, 0x0700
    xor     edx, edx
.loop:
    mov     [SCREEN_ADDR + edx], ax
    add     edx, 2
    cmp     edx, ecx
    jnz     .loop
    ret

; returns max address in eax
available_ram:
    xor     ecx, ecx
    xor     eax, eax
    mov     edx, [0x1000] ; number of elements in the mem-map list
    mov     esi, 0x1004
.loop:
    cmp     ecx, edx
    jge     .return
    mov     ebx, [esi]
    add     ebx, [esi + 8]
    add     esi, 24
    cmp     eax, ebx
    jge     .over
    mov     eax, ebx
.over:
    inc     ecx
    jmp     .loop
.return:
    ret

; set up a new gdt
load_gdt:
    mov dword   eax, gdt_start
    mov dword   [gdt_descriptor + 2], eax
    mov dword   eax, gdt_end
    sub dword   eax, gdt_start
    mov word    [gdt_descriptor], ax
    lgdt        [gdt_descriptor]
    ret

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
istruc  _gdt ; ring 3 code 
    at  _gdt.limit_lo,      dw 0xffff ; limit == 4 GiB
    at  _gdt.base_lo,       dw 0x0000 ; base == 0x00
    at  _gdt.base_mid,      db 0x00
    at  _gdt.access,        db 0xfa ; present, executable, readable
    at _gdt.lim_hi_flags,   db 0xcf ; flags: granularity == 4 KiB,
                                    ;        32-bit protected mode
    at _gdt.base_hi,        db 0x00
iend
istruc  _gdt ; ring 3 data
    at  _gdt.limit_lo,      dw 0xffff ; limit == 4 GiB
    at  _gdt.base_lo,       dw 0x0000 ; base == 0x00
    at  _gdt.base_mid,      db 0x00
    at  _gdt.access,        db 0xf2 ; present, readable, writable
    at _gdt.lim_hi_flags,   db 0xcf ; flags: granularity == 4 KiB,
                                    ;        32-bit protected mode
    at _gdt.base_hi,        db 0x00
iend
times 10 dq 0 ; make space for 10 more, for the future
gdt_end:

msg_welcome: db "[kernel] Who dares enter my domain?!", 0x00
