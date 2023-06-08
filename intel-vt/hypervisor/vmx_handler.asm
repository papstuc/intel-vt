extern vmexit_handler : proc

.code

SAVE_GP macro
        push    rax
        push    rcx
        push    rdx
        push    rbx
        push    rbp
        push    rsi
        push    rdi
        push    r8
        push    r9
        push    r10
        push    r11
        push    r12
        push    r13
        push    r14
        push    r15
endm

RESTORE_GP macro
        pop     r15
        pop     r14
        pop     r13
        pop     r12
        pop     r11
        pop     r10
        pop     r9
        pop     r8
        pop     rdi
        pop     rsi
        pop     rbp
        pop     rbx
        pop     rdx
        pop     rcx
        pop     rax
endm

vmx_launch_cpu  proc
	    pushfq
        mov     rcx, 0681Ch
	    vmwrite rcx, rsp
	    mov     rcx, 0681Eh
	    lea     rdx, vm_launched
	    vmwrite rcx, rdx
	    vmlaunch
	    pushfq
	    pop     rax
	    popfq
	    ret

vm_launched:
	    popfq
	    mov     rax, 0DEADBEEFh
	    ret
vmx_launch_cpu  endp

vmx_entrypoint  proc
        SAVE_GP
        sub     rsp, 108h
        movaps  xmmword ptr [rsp +  0h], xmm0
        movaps  xmmword ptr [rsp + 010h], xmm1
        movaps  xmmword ptr [rsp + 020h], xmm2
        movaps  xmmword ptr [rsp + 030h], xmm3
        movaps  xmmword ptr [rsp + 040h], xmm4
        movaps  xmmword ptr [rsp + 050h], xmm5
        movaps  xmmword ptr [rsp + 060h], xmm6
        movaps  xmmword ptr [rsp + 070h], xmm7
        movaps  xmmword ptr [rsp + 080h], xmm8
        movaps  xmmword ptr [rsp + 090h], xmm9
        movaps  xmmword ptr [rsp + 0A0h], xmm10
        movaps  xmmword ptr [rsp + 0B0h], xmm11
        movaps  xmmword ptr [rsp + 0C0h], xmm12
        movaps  xmmword ptr [rsp + 0D0h], xmm13
        movaps  xmmword ptr [rsp + 0E0h], xmm14
        movaps  xmmword ptr [rsp + 0F0h], xmm15
        mov     rcx, rsp
        sub     rsp, 20h
        call    vmexit_handler
        add     rsp, 20h
        movaps  xmm0, xmmword ptr [rsp +  0h]
        movaps  xmm1, xmmword ptr [rsp + 010h]
        movaps  xmm2, xmmword ptr [rsp + 020h]
        movaps  xmm3, xmmword ptr [rsp + 030h]
        movaps  xmm4, xmmword ptr [rsp + 040h]
        movaps  xmm5, xmmword ptr [rsp + 050h]
        movaps  xmm6, xmmword ptr [rsp + 060h]
        movaps  xmm7, xmmword ptr [rsp + 070h]
        movaps  xmm8, xmmword ptr [rsp + 080h]
        movaps  xmm9, xmmword ptr [rsp + 090h]
        movaps  xmm10, xmmword ptr [rsp + 0A0h]
        movaps  xmm11, xmmword ptr [rsp + 0B0h]
        movaps  xmm12, xmmword ptr [rsp + 0C0h]
        movaps  xmm13, xmmword ptr [rsp + 0D0h]
        movaps  xmm14, xmmword ptr [rsp + 0E0h]
        movaps  xmm15, xmmword ptr [rsp + 0F0h]
        add     rsp, 108h
        RESTORE_GP
        vmresume
        int 3
vmx_entrypoint   endp

end