
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

extern disp_str
extern do_sys_p
extern do_sys_v
extern sys_no_schedule_c

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_sys_str_print	equ 1 
_NR_p 				equ 2
_NR_v				equ 3
_NR_sys_no_schedule		equ 4

INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks

global 	sys_str_print_x
global 	sys_str_print_call

global 	sys_p_x
global 	sys_v_x
global 	sys_p_call
global 	sys_v_call

global 	sys_no_schedule_x
global 	sys_no_schedule_call

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

sys_str_print_x:
	; 发起中断
	mov eax, _NR_sys_str_print
	push ebx
	mov ebx, [esp+8]
	int INT_VECTOR_SYS_CALL
	pop ebx
	ret

sys_str_print_call:
	; 执行中断
	pusha 
	push ebx
	call disp_str
	pop ebx
	popa
	ret

sys_p_x:
	; 发起中断
	mov eax, _NR_p
	push ebx
	mov ebx, [esp+8]
	int INT_VECTOR_SYS_CALL
	pop ebx
	ret

sys_p_call:
	; 信号量执行P操作
	pusha
	push ebx
	call do_sys_p
	pop ebx
	popa
	ret

sys_v_x:
	; 发起中断
	mov eax, _NR_v
	push ebx
	mov ebx, [esp+8]
	int INT_VECTOR_SYS_CALL
	pop ebx
	ret

sys_v_call:
	; 信号量执行V操作
	pusha
	push ebx
	call do_sys_v
	pop ebx
	popa
	ret

sys_no_schedule_x:
	; 发起中断
	mov eax, _NR_sys_no_schedule
	push ebx
	mov ebx, [esp+8]
	int INT_VECTOR_SYS_CALL
	pop ebx
	ret

sys_no_schedule_call:
	push ebx
	call sys_no_schedule_c
	pop ebx
	ret
	