
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"

/* 进程表 */
PUBLIC	PROCESS			proc_table[NR_TASKS];

PUBLIC	char			task_stack[TOTAL_STACK_SIZE];

/* 任务列表 */
//任务列表里每一个进程由要调用的方法打头，因此一旦进程指针指向该进程，就会自动执行对应的方法。
PUBLIC	TASK	task_table[NR_TASKS] = {
                    {Reader_A, STACK_SIZEOF_A, "Reader_Task_A"},
					{Reader_B, STACK_SIZEOF_B, "Reader_Task_B"},
					{Reader_C, STACK_SIZEOF_C, "Reader_Task_C"},
                    {Writer_D, STACK_SIZEOF_D, "Writer_Task_D"},
                    {Writer_E, STACK_SIZEOF_E, "Writer_Task_E"},
                    {F, STACK_SIZEOF_F, "Normal_Task_F"}};

PUBLIC	irq_handler		irq_table[NR_IRQ];

PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {
    sys_get_ticks,
    sys_str_print_call,
    sys_p_call,
    sys_v_call,
    sys_no_schedule_call
};
