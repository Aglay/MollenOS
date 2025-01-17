; MollenOS
;
; Copyright 2011, Philip Meulengracht
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation?, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.
;
;
; x86-64 Signal Code
;

bits 64
segment .text

global __signalentry
extern StdInvokeSignal

; void __signalentry(context_t* context (rcx), int signal (rdx), void* argument (r8), reg_t flags (r9))
; Fixup stack and call directly into the signal handler
__signalentry:
    ; Prepare stack alignment
    mov rbx, 0x28 ; rbx register is non-volatile, so we use this for calculation
    mov rax, rsp
    and rax, 0xF
    jnz .skip_padding
    add rbx, 0x08
    
    .skip_padding:
    	sub rsp, rbx
    	call StdInvokeSignal ; (context_t*, size_t, void*, void*)
    	add rsp, rbx
	
    ; Restore initial state and switch the handler stack to next one stored
    pop rdi
    pop rsi
    pop rbp
    add rsp, 8 ; skip rsp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    
    ; get the user-rsp from the stack, it will be offset 5
    mov rsp, [rsp + (5 * 8)]
    ret
