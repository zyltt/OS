/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                              uC/OS-II
*                                            EXAMPLE CODE
*
* Filename : main.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_mem.h>
#include  <os.h>

#include  "app_cfg.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/
#define  TASK_STK_SIZE    128
#define  TASK_START_PRIO    5
// 定义任务的运行时间和周期
INT32S tasks[][2] = {
    {1,4},
    {2,5},
    {2,10}
};

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_STK  StartupTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE];

//三个任务
OS_STK        AppStartTaskStk1[TASK_STK_SIZE];
OS_STK        AppStartTaskStk2[TASK_STK_SIZE];
OS_STK        AppStartTaskStk3[TASK_STK_SIZE];
//将temp和event定义为全局变量，用于数据相关性
static INT8U temp;
OS_EVENT* event_1;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  StartupTask (void  *p_arg);
static  void  Task_1(void* p_arg);
static  void  Task_2(void* p_arg);
static  void  Task_3(void* p_arg);

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : none
*********************************************************************************************************
*/

int  main (void)
{
#if OS_TASK_NAME_EN > 0u
    CPU_INT08U  os_err;
#endif


    CPU_IntInit();

    Mem_Init();                                                 /* Initialize Memory Managment Module                   */
    CPU_IntDis();                                               /* Disable all Interrupts                               */
    CPU_Init();                                                 /* Initialize the uC/CPU services                       */

    OSInit();                                                   /* Initialize uC/OS-II                                  */
        //创建三个任务
    OSTaskCreate(Task_1, (void*)0, &AppStartTaskStk1[TASK_STK_SIZE - 1], 1);
    OSTaskCreate(Task_2, (void*)0, &AppStartTaskStk2[TASK_STK_SIZE - 1], 2);
    OSTaskCreate(Task_3, (void*)0, &AppStartTaskStk3[TASK_STK_SIZE - 1], 3);
    OSTaskCreateExt( StartupTask,                               /* Create the startup task                              */
                     0,
                    &StartupTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE - 1u],
                     APP_CFG_STARTUP_TASK_PRIO,
                     APP_CFG_STARTUP_TASK_PRIO,
                    &StartupTaskStk[0u],
                     APP_CFG_STARTUP_TASK_STK_SIZE,
                     0u,
                    (OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));

#if OS_TASK_NAME_EN > 0u
    OSTaskNameSet(         APP_CFG_STARTUP_TASK_PRIO,
                  (INT8U *)"Startup Task",
                           &os_err);
#endif
    OSStart();                                                  /* Start multitasking (i.e. give control to uC/OS-II)   */

    while (DEF_ON) {                                            /* Should Never Get Here.                               */
        ;
    }
}


/*
*********************************************************************************************************
*                                            STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'StartupTask()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

// 第一个任务
void  Task_1(void* p_arg)
{
	p_arg = p_arg;
	event_1 = OSSemCreate(1);
#if 0
	BSP_Init();                                                           */
#endif


#if OS_TASK_STAT_EN > 0
	OSStatInit();                                
#endif
//下面这部分是数据相关性的实现，在相关性时取消注释。
	int time = tasks[0][0];//任务执行时间
	int period = tasks[0][1];//任务执行的周期
	while (OS_TRUE)                                
	{     
		OSSemPend(event_1, 0, &temp);
		while (temp != OS_ERR_NONE) {}
		int start_time;
		start_time = OSTimeGet();
		while (OSTimeGet() - start_time < time);
		OSTimeDly(period - time);
		OSSemPost(event_1);
	}
//到这里截止
}
// 第二个任务
void  Task_2(void* p_arg)
{
	p_arg = p_arg;
	//event_1 = OSSemCreate(1);
#if 0
	BSP_Init();                                  
#endif


#if OS_TASK_STAT_EN > 0
	OSStatInit();                               
#endif
//下面这部分是数据相关性的实现。在相关性时取消注释。
	int time = tasks[1][0];//任务执行时间
	int period = tasks[1
	][1];//任务执行的周期
	while (OS_TRUE)                                 
	{
		//OS_Printf("Delay 1 second and print\n"); 
		//OSTimeDlyHMSM(0, 0, 1, 0);       
		//OSSemPend(event_1, 0, &temp);
		while (temp != OS_ERR_NONE) {}
		int start_time;
		start_time = OSTimeGet();
		while (OSTimeGet() - start_time < time);
		OSTimeDly(period - time);
		//OSSemPost(event_1);
	}
//到这里截止
}
// 第三个任务
void  Task_3(void* p_arg)
{
	p_arg = p_arg;
#if 0
	BSP_Init();                                
#endif


#if OS_TASK_STAT_EN > 0
	OSStatInit();                                
#endif
//下面这部分是数据相关性的实现，在相关性时取消注释。
	int time = tasks[2][0];//任务执行时间
	int period = tasks[2][1];//任务执行的周期
	while (OS_TRUE)                                 
	{
		while (temp != OS_ERR_NONE) {}
		int start_time;
		start_time = OSTimeGet();
		while (OSTimeGet() - start_time < time);
		OSTimeDly(period - time);
	}
//到这里截止
}
static  void  StartupTask (void *p_arg)
{
   (void)p_arg;

    OS_TRACE_INIT();                                            /* Initialize the uC/OS-II Trace recorder               */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif
    
    APP_TRACE_DBG(("uCOS-III is Running...\n\r"));

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
        OSTimeDlyHMSM(0u, 0u, 1u, 0u);
		APP_TRACE_DBG(("Time: %d\n\r", OSTimeGet()));
    }
}

