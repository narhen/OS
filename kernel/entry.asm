global irq0_entry, page_fault_entry, syscall_entry
extern schedule, _yield, page_fault, syscall_table
extern kpanic

section .text

BITS 32

;%define ISR_PROLOGUE \
;    pusha \
;    push    ds \
;    push    fs \
;    push    gs \
;    push    es

;%define ISR_EPILOGUE \
;    pop     es \
;    pop     gs \
;    pop     fs \
;    pop     ds \
;    popa \
;    iret

;%define SEND_EOI \
;    mov     al, 0x20 \
;    mov     dx, 0x20 \
;    out     dx, al


irq0_entry:
    ;ISR_PROLOGUE ; 'dis shit aint working yo
    pusha
    push    ds
    push    fs
    push    gs
    push    es

    sub     esp, 108
    fnsave  [esp]

    ;SEND_EOI
    mov     al, 0x20
    mov     dx, 0x20
    out     dx, al

    call    _yield
    frstor  [esp]
    add     esp, 108

    ;ISR_EPILOGUE
    pop     es
    pop     gs
    pop     fs
    pop     ds
    popa
    iret


syscall_entry:
    ;ISR_PROLOGUE
    pusha
    push    ds
    push    fs
    push    gs
    push    es

    push    edi ; 5. argument
    push    esi ; 4. argument
    push    ebx ; 3. argument
    push    edx ; 2. argument
    push    ecx ; 1. argument
    shl     eax, 2
    add     eax, syscall_table
    call    eax
    add     esp, 20

    ;ISR_EPILOGUE
    pop     es
    pop     gs
    pop     fs
    pop     ds
    popa
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

extern double_fault
global double_fault_entry
double_fault_entry:
    call    double_fault
    hlt
