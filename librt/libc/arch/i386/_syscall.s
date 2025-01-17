; MollenOS
; Copyright 2016, Philip Meulengracht
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
; x86-32 Syscall Assembly Routine

bits 32
segment .text

;Functions in this asm
global __syscall

; int _syscall(int Function, int Arg0, int Arg1, int Arg2, int Arg3, int Arg4)
__syscall:
    push ebp
    mov  ebp, esp
	push ebx
	push esi
	push edi

	mov eax, [ebp + 8]
	mov ebx, [ebp + 12]
	mov ecx, [ebp + 16]
	mov edx, [ebp + 20]
	mov esi, [ebp + 24]
	mov edi, [ebp + 28]
	int	60h

	pop edi
	pop esi
	pop ebx
    pop ebp
	ret 
