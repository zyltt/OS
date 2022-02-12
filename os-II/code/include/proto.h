
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "proc.h"

/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);

/* protect.c */
PUBLIC void	init_prot();
PUBLIC u32	seg2phys(u16 seg);

/* klib.c */
PUBLIC void	delay(int time);

/* kernel.asm */
void restart();

/* main.c 加入函数声明 */
void Reader_A();
void Reader_B();
void Reader_C();
void Writer_D();
void Writer_E();
void F();

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);
PUBLIC void milli_delay(int milli_sec);

/* proc.c */
PUBLIC  int     sys_get_ticks();        /* sys_call */
PUBLIC  void    sys_no_schedule_c(int i);
PUBLIC  void    do_sys_p(SEMAPHORE *);
PUBLIC  void    do_sys_v(SEMAPHORE *);

/* syscall.asm 系统调用 */
PUBLIC  void    sys_call();             /* int_handler */
PUBLIC  int     get_ticks();
PUBLIC  void    sys_p_call(SEMAPHORE *);
PUBLIC  void    sys_v_call(SEMAPHORE *);
PUBLIC  void    sys_no_schedule_x(int milli_seconds);
PUBLIC  void    sys_str_print_x(char *);

PUBLIC  void    sys_p_x(SEMAPHORE *);
PUBLIC  void    sys_v_x(SEMAPHORE *);
PUBLIC  void    sys_no_schedule_call(int i);
PUBLIC  void    sys_str_print_call(char *);

/* main.c */
PUBLIC  void    clear();