
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
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
//由于schedule上来让p_proc_ready++,这样第一轮就从B进程开始了，为了避免这一问题，第一次执行指令不加，因此加入标记位
int is_first = 1;

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
// 执行信号量P操作
//P操作代表着为进程分配资源。即为将信号量的值减一。
PUBLIC void do_sys_p(SEMAPHORE* semaphore){
    //每次请求资源让信号量的值减少1（分配资源）
    semaphore -> number--;
    //如果确实还有资源可分配，不需要做后续处理。
    if (semaphore->number >= 0) {
        return;
    }
    //如果没有资源可分配了。（即正在使用的资源达到了上限）就把这个进程加入到信号量的进程数组中。
    if(semaphore -> number < 0){
        p_proc_ready->incoming_somaphore = semaphore;
        //将进程指针加入信号量的进程数组中，数组中的都是还未分配资源的进程。
        semaphore -> list[semaphore -> end] = p_proc_ready;
        //循环利用数组
        semaphore -> end = (semaphore -> end + 1) % SEMAPHORE_SIZE;
        // 重新进行进程调度
        //即：这次这个进程全部处理完了，要开始选下一个要做的进程了。
        schedule();
    }
}
// 执行信号量V操作
PUBLIC void do_sys_v(SEMAPHORE* semaphore){
    //进程做完了释放资源就是将信号量的值加一
    semaphore -> number ++;
    //如果释放完的值还小于等于零，那这里要补分配上。
    if(semaphore -> number <= 0){
        // 取出等待队列的第一个进程
        PROCESS* p = semaphore -> list[semaphore -> start];
        //把这一进程所需要的信号量改为0，这样下一次schedule就会为它分配资源了。
        p -> incoming_somaphore = 0;
        //把队首的元素去掉，并循环利用数组
        semaphore -> start = (semaphore -> start + 1) % SEMAPHORE_SIZE;
    }
}
PUBLIC int sys_get_ticks()
{
	return ticks;
}

// 进程调度
PUBLIC void schedule() {
    //schedule的实质是移动p_proc_ready指针
    int nowtick = 0;
    SEMAPHORE* incoming_semaphore = 0;//新的信号量
    int can_ready = 1;//是否能够继续就绪
    while (1) {
        //保证第一次是从进程A开始分配，因此要先减一个
        if (is_first) {
            p_proc_ready--;is_first = 0;
        }
        //计算现在的时刻
        nowtick = get_ticks();
        //指向下一个进程
        p_proc_ready++;
        //一圈一圈指，超过了再调从头开始指
        if (p_proc_ready >= proc_table + NR_TASKS) {
            p_proc_ready = proc_table;
        }
        //当前进程还没处理的信号量列表。
        incoming_semaphore = p_proc_ready->incoming_somaphore;
        //判断进程是否可以被执行
        if (p_proc_ready->next_ready <= nowtick)can_ready = 1;
        else can_ready = 0;
        //incoming_semaphore==0说明该进程已经等到了所需要的全部信号量，can_ready==1说明该进程可以被执行。
        //如果同时具备上述两个条件说明可以执行该进程，就将p_proc_ready指针停在该进程上，不再进行其他操作。
        //如果有还没处理的进程，直接进入下一轮，指针指向下一个进程即可。
        if (incoming_semaphore == 0 && can_ready == 1) {
            break;
        }
    }
}
/*
备注：每一轮系统执行都是先执行一次schedule，根据schedule所得到的指针执行对应进程的方法，而后运行sys_no_schedule_c。
运行Reader_A等方法的P操作的资源不够时会修改incoming_somaphore,V操作会将incoming_somaphore清零。
sys_no_schedule_c会修改next_ready,schedule方法根据这两个指标共同决定下一轮要不要在这个进程上停留。另外，所有的地方
都是修改标记，唯一时间上的停留是F进程执行过程中会空一个时间片。
*/
// 不分配时间片来切换，这部分被汇编调用
PUBLIC void sys_no_schedule_c(int i){
    //该方法表示已经处理了该进程，与每次Reader_A等各进程所执行的方法被调用完成之后调用。
    //该方法只修改指针和进程的标记，没有时间上的延迟。
    p_proc_ready->next_ready = get_ticks() + i / (1000 / HZ);
    // schedule的作用是移动p_proc_ready指针，选择下一个要处理的进程。
    schedule();
}


