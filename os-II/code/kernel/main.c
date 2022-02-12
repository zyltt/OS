
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "stdlib.h"
// 闪屏闪的太快了，因此每次输入指令要停一下看看，还能防止程序执行时间造成的干扰。
#define SLEEP_TIME 5000
// 阅读者上限
#define READER_MAX 		3
// 每个时间片的长度
#define Time_Chip 10 * 50000 / HZ
// 读者优先还是写者优先 0是读者优先,1是写者优先,2是读写公平。
#define FIRST_MODE 		2


//定义彩色所需要的一系列颜色常量。
//蓝色
#define BLUE 0x01
//绿色
#define GREEN 0x02
//青色
#define CYAN 0x03
//红色 
#define RED 0x04
//银色
#define LIGHT_GRAY 0x07
//淡蓝色
#define LIGHT_BLUE 0x09
//黄色
#define YELLOW 0x0E
//白色
#define WHITE 0x0F
void initSemaphore(SEMAPHORE* semaphore, int number);
//正在执行的读进程数
int reader_cnt = 0;
//正在执行的写进程数
int writer_cnt = 0;
SEMAPHORE mutex1, mutex2, mutex3, w, r;



/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{	
	//让写者有限从D开始。
	if (FIRST_MODE == 1)p_proc_ready += 3;

	disp_str("-----\"kernel_main\" begins-----\n");
	//创造原任务表和进程表的指针
	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	//找到栈顶指针
	char*		p_task_stack	= task_stack + TOTAL_STACK_SIZE;
	//初始化ldt指针
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	/* 初始化进程表 */
	//对于每一个进程而言
	for (i = 0; i < NR_TASKS; i++) {
		//初始化每一个进程名
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		//初始化每一个进程pid
		p_proc->pid = i;			// pid

		/************************************************
		*************************************************
		以下部分是原有的，初始化每一个进程的相应的系统变量
		*************************************************
		*/

		//初始化每一个进程的ldt指针。
		p_proc->ldt_sel = selector_ldt;
		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		/*******************************************************
		这里添加了两个属性，初始化了每一次进程所需要等待的信号量和下一次最早就绪的时间。
		*******************************************************/
		p_proc->next_ready = 0;
		p_proc->incoming_somaphore = 0;
		//一个进程初始化完了栈也要指向下一个进程，因此这里要退一次栈。
		p_task_stack -= p_task->stacksize;
		//因为要初始化每一个进程，因此一个进程初始化完了就把指针加一初始化下一个进程。
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	// 避免反复嵌套循环无法跳出
	k_reenter = 0;
	// 初始化时间
	ticks = 0;
	// 初始化显示行：每输出一行都要改动该变量。
	disp_number = 0;

	/***************************************************
	下面是初始化各个信号量
	***************************************************/
	//注：读信号量最多占有最大读者数个资源，其余的信号量最多占有一个资源。
	initSemaphore(&mutex1, READER_MAX);
	initSemaphore(&mutex2, 1);
	initSemaphore(&mutex3, 1);
	initSemaphore(&w, 1);
	initSemaphore(&r, 1);
	//初始化当前的读者数和写者数
	reader_cnt = 0;
	writer_cnt = 0;

	// 清屏
	clear();
	disp_pos = 0;

	/* 扳机 */
	//即将进程表加载到系统中，开始执行这些进程。
	//注：修改p_proc_ready指针即代表加载到系统中。
	p_proc_ready	= proc_table;
	/*****************************************
	下面这部分是原有的和时钟和中断相关的初始化。
	******************************************/
	// 8253 PIT
	out_byte(TIMER_MODE, RATE_GENETATOR);
	out_byte(TIMER0, (u8) (TIMER_FREQ)/ HZ);
	out_byte(TIMER0, (u8) ((TIMER_FREQ)/ HZ >> 8));

    put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
    enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

	//操作系统启动第一个进程时入口
	restart();

	while(1){}
}


//上文调用的信号量初始化方法。
void initSemaphore(SEMAPHORE* semaphore, int number){
	semaphore->number = number;
	semaphore->start = 0;
	semaphore->end = 0;
}




void print_line(char*name, char * str, int color){
	disp_color_str(name, color);
	disp_color_str(str, color);
	sys_str_print_x("\n");
	//每输出一行就要把已输出的行数加一。
	disp_number++;
}

/************************************************************************
                               read_in_readmode 读者优先的读
 ************************************************************************/
void read_in_readmode(char*name,  int time){
	while (1) {
		sys_p_x(&mutex1);
		if (reader_cnt == 0) {
			sys_p_x(&w);
		}
		reader_cnt++;
		print_line(name, " starts reading ... ", GREEN);
		print_line(name, " is reading ... ", CYAN);
		milli_delay(time*Time_Chip);
		print_line(name, " stop reading ... ", RED);

		sys_v_x(&mutex1);
		reader_cnt--;
		if (reader_cnt == 0) {
			sys_v_x(&w);
		}
	}
}

/**************************************************************************
                               read_in_writemode 写者优先的读
 ************************************************************************/
void read_in_writemode(char*name, int time){
	while(1){
		sys_p_x(&mutex3);
		sys_p_x(&r);
		sys_p_x(&mutex1);
		reader_cnt++;
		if(reader_cnt == 1){
			sys_p_x(&w);
		}
		sys_v_x(&mutex1);
		sys_v_x(&r);

		print_line(name, " starts reading ... ", GREEN);
		print_line(name, " is reading ... ", CYAN);
		milli_delay(time * Time_Chip);
		print_line(name, " stop reading ... ", RED);
		reader_cnt--;
		if(reader_cnt == 0){
			sys_v_x(&w);
		}
		sys_v_x(&mutex1);
	}
}

/*********************************************************************
                               write_in_readmode 读者优先的写
 **********************************************************************/
void write_in_readmode(char*name, int time){
	while(1){
		sys_p_x(&w);
		print_line(name, " starts writing ... ", GREEN);
		print_line(name, " is writing ... ", CYAN);
		milli_delay(time * Time_Chip);
		print_line(name, " finishs writing ... ", RED);
		sys_v_x(&w);
	}
}

/**************************************************************************
                               write_in_writemode 写者优先的写
 **************************************************************************/
void write_in_writemode(char*name, int time){
	while(1){
		sys_p_x(&mutex2);
		writer_cnt++;
		if(writer_cnt == 1){
			sys_p_x(&r);
		}
		sys_v_x(&mutex2);
		sys_p_x(&w);

		print_line(name, " starts writing ... ", LIGHT_BLUE);
		print_line(name, " is writing ... ", LIGHT_GRAY);
		milli_delay(time * Time_Chip);
		print_line(name, " stop writing ... ", YELLOW);
		sys_v_x(&w);
		sys_p_x(&mutex2);
		writer_cnt--;
		if(writer_cnt == 0){
			sys_v_x(&r);
		}
		sys_v_x(&mutex2);
	}
}
/**************************************
									读写公平
**************************************/
void read_in_fairmode(char* name, int time) {
	while (1) {
		sys_p_x(&w);
		sys_p_x(&mutex1);
		if (reader_cnt == 0) {
			sys_p_x(&r);
		}
		reader_cnt++;
		sys_v_x(&w);

		print_line(name, " starts reading ... ", LIGHT_BLUE);
		print_line(name, " is reading ... ", LIGHT_GRAY);
		milli_delay(time * Time_Chip);
		print_line(name, " stop reading ... ", YELLOW);
		reader_cnt--;
		if (reader_cnt == 0) {
			sys_v_x(&r);
		}
		sys_v_x(&mutex1);
	}
}



void write_in_fairmode(char* name, int time) {
	while (1) {
		sys_p_x(&w);
		sys_p_x(&r);
		print_line(name, " starts writing ... ", LIGHT_BLUE);
		print_line(name, " is writing ... ", LIGHT_GRAY);
		milli_delay(time * Time_Chip);
		print_line(name, " stop writing ... ", YELLOW);
		sys_v_x(&r);
		sys_v_x(&w);
	}
}



//Reader_A
void Reader_A()
{	
	if(FIRST_MODE==0){
		read_in_readmode("Reader_A", 2);
	}
	if(FIRST_MODE==1){
		read_in_writemode("Reader_A", 2);
	}
	if (FIRST_MODE == 2) {
		read_in_fairmode("Reader_A", 2);
	}
}

//Reader_B
void Reader_B()
{
	if (FIRST_MODE == 0) {
		read_in_readmode("Reader_B", 3);
	}
	if (FIRST_MODE == 1) {
		read_in_writemode("Reader_B", 3);
	}
	if (FIRST_MODE == 2) {
		read_in_fairmode("Reader_B", 3);
	}
}
//Reader_C
void Reader_C()
{
	if (FIRST_MODE == 0) {
		read_in_readmode("Reader_C", 3);
	}
	if (FIRST_MODE == 1) {
		read_in_writemode("Reader_C", 3);
	}
	if (FIRST_MODE == 2) {
		read_in_fairmode("Reader_C", 3);
	}
}
//Writer_D
void Writer_D()
{
	if (FIRST_MODE == 0) {
		write_in_readmode("Writer_D", 3);
	}
	if (FIRST_MODE == 1) {
		write_in_writemode("Writer_D", 3);
	}
	if (FIRST_MODE == 2) {
		write_in_fairmode("Writer_D", 3);
	}
}
//Writer_E
void Writer_E()
{
	if (FIRST_MODE == 0) {
		write_in_readmode("Writer_E", 4);
	}
	if (FIRST_MODE == 1) {
		write_in_writemode("Writer_E", 4);
	}
	if (FIRST_MODE == 2) {
		write_in_fairmode("Writer_E", 4);
	}
}


//普通进程F

void F(){
	while(1){
		//普通进程的输出信息一律使用白色
		//输出读者信息。
		if(reader_cnt <= READER_MAX && reader_cnt > 0){	
			char* number = "0";
			number[0] = '0'+ reader_cnt;
			sys_str_print_x("F : reading now......\n");
			disp_number++;
			sys_str_print_x("The number of readers is : ");
			sys_str_print_x(number);
			sys_str_print_x("\n");
			//每输出一行信息就将disp_number+1。
			disp_number++;
		}
		//输出写的提示信息
		else
		{
			sys_str_print_x("F : writing now......\n");
			disp_number++;
		}
		//不占用时间片，只起到一个计时的效果。
		sys_no_schedule_x(Time_Chip);
	}
}


//清屏
void clear(){
	u8 *base = (u8 *) V_MEM_BASE;
	for(int i = 0; i < V_MEM_SIZE; i += 2){
		base[i] = ' ';
		base[i + 1] = DEFAULT_CHAR_COLOR;
	}
	//把光标的位置和待输出行的数量都清零。
	disp_pos = 0;
	disp_number = 0;
}
