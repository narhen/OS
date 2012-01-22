global irq0_entry, page_fault_entry, syscall_entry
extern schedule, _yield, page_fault, syscall_table

section .text

BITS 32

%define SEND_EOI ; \
    mov     al, 0x20 ; \
    mov     dx, 0x20 ; \
    outb    dx, al


irq0_entry:
    pushad
    pushfd
    sub     esp, 108
    fnsave  [esp]
    SEND_EOI
    call    _yield
    frstor  [esp]
    add     esp, 108
    popfd
    popad
    iret

page_fault_entry:
    push    eax ; save original eax
    mov     eax, esp
    pushad
    push    eax ; pointer to arguments
    call    page_fault
    add     esp, 4
    popad
    pop     eax ; restore origninal eax
    add     esp, 4 ; need to pop the error code from stack, before iretd
    iretd

syscall_entry:
    push    edi
    push    esi
    push    ebx
    push    edx
    push    ecx
    shl     eax, 2
    add     eax, syscall_table
    call    eax
    add     esp, 20
    iretd
