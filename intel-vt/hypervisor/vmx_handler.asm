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
	    lea     rdx, done
	    vmwrite rcx, rdx
	    vmlaunch

	    pushfq
	    pop     rax

	    popfq
	    ret
done:
	    popfq
	    mov     rax, 0DEADBEEFh
	    ret
vmx_launch_cpu  endp

vmx_entrypoint  proc
        SAVE_GP

        sub     rsp, 68h

        movaps  xmmword ptr [rsp +  0h], xmm0
        movaps  xmmword ptr [rsp + 10h], xmm1
        movaps  xmmword ptr [rsp + 20h], xmm2
        movaps  xmmword ptr [rsp + 30h], xmm3
        movaps  xmmword ptr [rsp + 40h], xmm4
        movaps  xmmword ptr [rsp + 50h], xmm5

        mov     rcx, rsp
        sub     rsp, 20h
        call    vmexit_handler
        add     rsp, 20h

        movaps  xmm0, xmmword ptr [rsp +  0h]
        movaps  xmm1, xmmword ptr [rsp + 10h]
        movaps  xmm2, xmmword ptr [rsp + 20h]
        movaps  xmm3, xmmword ptr [rsp + 30h]
        movaps  xmm4, xmmword ptr [rsp + 40h]
        movaps  xmm5, xmmword ptr [rsp + 50h]

        add     rsp, 68h
        test    al, al
        jz      exit
        RESTORE_GP
        vmresume
        jmp     vmerror
exit:
        RESTORE_GP
        vmxoff
        jz      vmerror
        jc      vmerror
        push    r8
        popf
        mov     rsp, rdx
        push    rcx
        ret
vmerror:
        int     3

vmx_entrypoint   endp

end