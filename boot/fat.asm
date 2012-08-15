; fat.asm - contains code for loading kernel from a fat partition
;

%define BPB 0x200
%define FAT 0x400
%define TMP_CLUSTER 0x800

; functions from boot.asm
global copy, read

; functions from this file
global fat_load, fat_read

start_lba: dw 0 ; start sector of partition
start_fat: dw 0 ; start sector of the FATs
start_data: dw 0 ; start sector of data clusters
spc: db 0 ; sectors pr cluster

; @esi must contain the address to partition table entry, for a fat partition
fat_load:
    mov byte    al, [es:esi]
    cmp byte    al, 0x80
    jnz         .error ; not a bootable partition
    mov byte    al, [es:esi + 4]
    cmp byte    al, 0x0b
    jnz         .error ; not a fat partition

    mov word    di, es
    shl dword   edi, 4
    add dword   edi, BPB ; edi must be loaded with the absolute address before
                         ; the call to read
    xor         eax, eax
    mov word    ax, [es:esi + 8]
    mov         [start_lba], ax
    mov word    cx, 1
    call        read
    mov dword   edi, BPB
    mov         al, [es:edi + 13]
    mov         [spc], al

    call        check_validity
    test        eax, eax
    jz          .error ; invalid fat partition

    call        fat_version
    cmp dword   eax, 32
    jnz         .error ; not fat32.

    mov         eax, 0x1
    jmp         .return
.error:
    mov word    si, msg_invalid_fat_version
    call        print
    mov         eax, 0
.return:
    ret

msg_invalid_fat_version: db "Not a FAT32 system!", 0x0a, 0x0d
msg_file_not_found: db "Couldn't find '/boot/kernel'!", 0x0a, 0x0d

; checks if the partition is a fat partition
; returns 1 in eax if valid, 0 otherwise
check_validity:
    mov dword   esi, BPB
    cmp dword   [es:esi + 3], 0x6f646b6d ; 'mkdo'
    jnz         .error
    cmp dword   [es:esi + 7], 0x00736673 ; 'sfs'
    jnz         .error
    mov         eax, 1
    jmp         .return
.error:
    mov         eax, 0
.return:
    ret

; returns the fat version of the partition in eax (32, 16 or 12)
fat_version:
    mov dword   esi, BPB
    mov word    ax, [es:esi + 19]
    test word   ax, ax
    jnz         .correct
    mov dword   eax, [es:esi + 32]
.correct:
    shr dword   eax, 3
    cmp         eax, 4085
    jge         .not12
    mov         eax, 12
    jmp         .return
.not12:
    cmp         eax, 65525
    jge         .not16
    mov dword   eax, 16
.not16:
    mov         eax, 32
.return:
    ret

; checks if the name of the directory entry in esi is 'boot'
check_dir:
    sub         esi, 0x20
    cmp byte    [es:esi + 11], 0x0f
    jnz         .negative
    cmp dword   [es:esi + 1], 0x006f0062
    jnz         .negative
    cmp dword   [es:esi + 5], 0x0074006f
    jnz         .negative
    mov         eax, 0
    jmp         .return
.negative:
    mov         eax, 1
.return:
    add         esi, 0x20
    ret

; checks if the name of the directory entry in edi is 'kernel'
check_file:
    sub         edi, 0x20
    cmp byte    [es:edi + 11], 0x0f
    jnz         .negative
    cmp dword   [es:edi + 1], 0x0065006b
    jnz         .negative
    cmp dword   [es:edi + 5], 0x006e0072
    jnz         .negative
    cmp word    [es:edi + 9], 0x0065
    jnz         .negative
    cmp word    [es:edi + 14], 0x006c
    jnz         .negative
    mov         eax, 0
    jmp         .return
.negative:
    mov         eax, 1
.return:
    add         edi, 0x20
    ret

current_fat_sector: dw 0
current_cluster: dw 0
; returns the first sector of the next cluster in eax
get_next_cluster:
    push        edi
    xor         eax, eax
    mov         ax, [current_cluster]
    mov         ebx, eax
    shr         ebx, 7
    add         bx, [start_fat]
    cmp         bx, [current_fat_sector]
    jz          .gogo
    mov         ax, [current_fat_sector]
    add         ax, bx
    mov         [current_fat_sector], ax
    mov         ecx, 1
    mov         edi, FAT
    call        read
    mov         ax, [current_fat_sector]
.gogo:
    shl         eax, 2
    mov         edi, FAT
    add         edi, eax
    mov         eax, [es:edi]
    and         eax, 0x0fffffff
    mov         [current_cluster], ax
    cmp         eax, 0x0ffffff8
    jge         .error; no more clusters in file
    call        cluster_to_sector
    jmp         .return
.error:
    mov         eax, 0
.return:
    pop         edi
    ret

; @eax must contain cluster number
cluster_to_sector:
    push        ecx
    sub         eax, 2
    xor         ecx, ecx
    mov byte    cl, [spc]
    mul byte    ecx
    mov word    cx, [start_data]
    add         eax, ecx
    pop         ecx
    ret

; reads the file '/boot/kernel' into memory at address SYS_LOADPOINT
fat_read:
    mov         esi, BPB
    mov         eax, 0
    mov word    ax, [es:esi + 14]
    add         eax, [start_lba]
    mov         [start_fat], ax ; save the first sector of the first FAT
    mov         [current_fat_sector], ax
    mov word    di, es
    shl dword   edi, 4
    add         edi, FAT ; edi must be loaded with the absolute address before
                         ; the call to read
    mov         ecx, 1
    call        read ; read the first sector of the FAT
    mov         esi, BPB
    mov         eax, [es:esi + 36] ; fat size in sectors ; fat size in sectors
    xor         ecx, ecx
    mov byte    cl, [es:esi + 16]
    mul         ecx
    mov word    cx, [es:esi + 14]
    add         eax, ecx
    mov word    cx, [start_lba]
    add         eax, ecx
    mov         [start_data], ax ; save first datasector
    xor         ecx, ecx
    mov byte    cl, [spc] ; sectors per cluster
    mov word    di, es
    shl dword   edi, 4
    add dword   edi, TMP_CLUSTER
    call        read ; read the first cluster of the root directory
    mov         esi, TMP_CLUSTER
    mov         eax, [es:esi + 44] ; root cluster
    mov word    [current_cluster], ax

; search for a directory entry with the name "boot", in the root directory
.loop:
    cmp byte    [es:esi], 0x00
    jz          .next ; entry does not exist
    cmp byte    [es:esi], 0xe5
    jz          .next ; entry does not exist
    cmp byte    [es:esi + 11], 0x10
    jnz         .next ; we are looking for a directory
    call        check_dir
    test        eax, eax
    jz          .found_it
.next:
    add         esi, 0x20
    xor         eax, eax
    mov byte    al, [spc]
    mov dword   ecx, 512
    mul dword   ecx
    add         eax, TMP_CLUSTER
    cmp dword   esi, eax
    jl          .loop
    call        get_next_cluster
    test        eax, eax
    jz          .error ; couldn't find /boot
    mov         edi, TMP_CLUSTER
    xor         ecx, ecx
    mov byte    cl, [spc]
    mov dword   esi, TMP_CLUSTER
    jmp         .loop

.found_it:
    cmp word    [es:esi + 20], 0 ; check if high 16 bits of cluster is 0
    jnz         .error ; cannot read sector > 0xffff
    xor         eax, eax
    mov word    ax, [es:esi + 26]
    mov word    [current_cluster], ax
    call        cluster_to_sector
    push dword  TMP_CLUSTER
    mov word    di, es
    shl dword   edi, 4
    add dword   edi, TMP_CLUSTER
    xor dword   ecx, ecx
    mov byte    cl, [spc]
    call        read ; read the first cluster belonging to /boot
    pop         edi
.loop2:
    cmp byte    [es:edi], 0x00
    jz          .next2 ; entry does not exist
    cmp byte    [es:edi], 0xe5
    jz          .next2 ; entry does not exist
    cmp byte    [es:edi + 11], 0x20
    jnz         .next2
    call        check_file
    test        eax, eax
    jz          .found_it2
.next2:
    add         edi, 0x20
    xor         eax, eax
    mov byte    al, [spc]
    mov dword   ecx, 512
    mul dword   ecx
    add         eax, TMP_CLUSTER
    cmp dword   edi, eax
    jl          .loop2
    call        get_next_cluster
    test        eax, eax
    jz          .error ; couldn't find /boot/kernel
    mov         edi, TMP_CLUSTER
    xor         ecx, ecx
    mov byte    cl, [spc]
    mov dword   edi, TMP_CLUSTER
    jmp         .loop2
.found_it2:
; now we heave the directory entry for /boot/kernel
    mov         esi, edi
    xor         eax, eax
    mov         ax, [es:esi + 26] ; 16 LSBs of cluster
    mov         [current_cluster], ax
    call        cluster_to_sector
    mov         edx, [es:esi + 28] ; filesize
    mov         edi, SYS_LOADPOINT
    xor         ebx, ebx

; here we do the actual loading of the kernel
.load_loop:
    mov dword   ecx, 1
    call        read
    add         edi, 0x200
    inc         ebx
    inc         eax
    sub         edx, 0x200
    cmp         edx, 0
    js          .done
    cmp         ebx, [spc]
    jnz         .load_loop
    call        get_next_cluster
    xor         ebx, ebx
    jmp         .load_loop
.done:
    mov         eax, 0
    jmp         .return
.error:
    mov         si, msg_file_not_found
    call        print
    mov         eax, 1
.return:
    ret
