global _locking

_locking:
    push ebp; saving the previous value of the ebp
    mov ebp, esp
    mov ebx, [ebp+8] 
    mov eax, 1 ;setting a register
    xchg eax, ebx ;putting ebx into eax (setting the lock to true) and returning the previous lock situation in ebx
    pop ebp ;restoring the ebp reg
    ret