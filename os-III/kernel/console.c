
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
//这里与控制台相关，要进行大幅度的修改
/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);
PRIVATE int  print_color;
PRIVATE int  start_find_index;

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	// 被init_tty()调用
	//nr_tty:当前所使用的tty编号。
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	// 初始化颜色
	print_color = DEFAULT_CHAR_COLOR;
	// 初始化第一个查找元素位置
	start_find_index = 0;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */
	
	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;//当前控制台存储空间的大小
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;
	p_tty->p_console->is_ctrl_z = 0;
	
	position = 0;
	esc_postion = 0;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		// 除以二是因为屏幕上的一个字符代表2个字节
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		// out_char(p_tty->p_console, nr_tty + '0');
		// out_char(p_tty->p_console, '#');
	}
	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	// 检查是否是当前的控制台
	return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
			   ctrlZ
 *======================================================================*/
PRIVATE void ctrlZ(CONSOLE* p_con, char c){
//这实际上是一个正常的添加方法，在ctrl+Z里面会被用到，如果是添加模式下ctrl+Z，会把char先设为backspace。删除模式下就正常写。
	p_con->chars[position] = c;
	position++;
}

/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	// 向控制台输出字符
	//得到当前控制台的地址。
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	if(find_pattern == 2 && ch != '\1'){
		// 在结束模式下（即已经查完了标号红了，应该什么都不干就等着看有没有\1：esc要输入了）
		return;
	}

	// 0是插入操作
	// 这里是对ctrl+z的处理，如果现在是在插入，那么下一个字符就要删除。如果是删除，就正常插入字符。
	int ctrl_pattern = 0;
	// ctrl中存储的字符
	char next_char = '\b';

	switch(ch) {
		//输入的字符是回车。
		case '\n':
			if(!find_pattern){
				// 正常模式换行
				//如果是正常模式就换行。
				//如果还能写下一行不超内存限制
				if (p_con->cursor < p_con->original_addr +
						p_con->v_mem_limit - SCREEN_WIDTH) {
					//输出一个\n
					*p_vmem++ = '\n';
					*p_vmem++ = NO_COLOR;
					//把光标放到下一行开头
					p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
						((p_con->cursor - p_con->original_addr) /
						SCREEN_WIDTH + 1);
				}
			}else{
				//如果是在查找模式下输入的回车。
				// 执行查找，并且变色
				if(find_pattern == 1){
					// 查找字符串长度
					//start_find_index存的是开始查找的光标位置。
					int length = p_con->cursor - start_find_index;
					// 第一个char指针
					u8* current_char = (u8*)(V_MEM_BASE);
					// 外层指针
					int i = 0;
					// unsigned int & int
					//一个字是两个地址，因此转换要乘以2。下面的临界条件是查找的地址空间范围。
					while(i <= (int)(start_find_index - p_con->original_addr - length) * 2){
						// 如果发现第一个字符相等
						if(*(current_char + i) == *(current_char + start_find_index * 2)){
							int tmp = 1;
							int is_match = 1;
							// 比接下来所有的字符，temp表示已经比对了的相等的字符数。
							while(tmp < length){
								if(*(current_char + i + tmp * 2) != *(current_char + (start_find_index + tmp) * 2)){
									is_match=0;
									break;
								}
								tmp++;
							}
							tmp = 0;
							//到这里字符匹配过程完成。
							// 对已经匹配的进行染色
							if(is_match){
								while(tmp < length){
									// 不修改\t
									//两个两个地址染色是因为一个字是两位，第一位代表值，第二位代表输的方式。
									if(*(current_char + i + tmp * 2) == '\t'){
										tmp++;
										continue;
									}
									*(current_char + i + tmp * 2 + 1) = RED_COLOR;
									tmp++;
								}
							}
						}
						i += 2;
					}
					// 进入等待结束模式
					find_pattern = 2;
				}
			}
			break;
		//新的一个模块，对于回退键的处理。
		case '\b':
			//ctrl_pattern代表的0代表插入，1代表删除。
			ctrl_pattern = 1;
			if (p_con->cursor > p_con->original_addr) {
				//如果
				if(p_con->cursor % SCREEN_WIDTH == 0){
					//检查换行。
					//一次删除可以删除所有的\0
					int i = 2;
					while((*(p_vmem-i) != '\n' && *(p_vmem-i) == '\0')){
						p_con->cursor--;
						i += 2;
						//最多一次删一行
						if((i/2 - 1) % SCREEN_WIDTH == 0){
							break;
						}
					}
					// 消除换行符
					//指针一边减，一边修改console里面保存的要输出的东西。
					*(p_vmem-i) = ' ';
					*(p_vmem-i + 1) = print_color;
					p_con->cursor--;
					// 不必删除上一行末尾
					next_char = '\n';
					//即告诉原有的状态，在输入的话就要换行了。
					break; // 脱离case
				}
				// 检查有无tab：删除Tab
				if(*(p_vmem - 2*TAB_WIDTH) == '\t'){
					int i = 0;
					while(i <= 2 * TAB_WIDTH){
						*(p_vmem - i) = ' ';
						*(p_vmem - i + 1) = print_color;
						p_con->cursor--;
						i+=2;
					}
					p_con->cursor++;
					next_char = '\t';
					break;// 脱离case
				}
				// 没有空格也没有tab，正常删除
				p_con->cursor--;
				next_char = *(p_vmem-2);
				*(p_vmem-2) = ' ';
				*(p_vmem-1) = print_color;
			}
			break;
		//一个新的模块：Tab的插入。
		case '\t':
			if (p_con->cursor >= p_con->original_addr) {
				// 插入的Tab是黑底透明的
				//把第一位设为无颜色的\t，其它位数均设成无颜色的空格即可。
				*p_vmem++ = '\t';
				*p_vmem++ = NO_COLOR;
				p_con->cursor++;
				int i = 1;
				while(i < TAB_WIDTH){
					*p_vmem++ = ' ';
					*p_vmem++ = NO_COLOR;
					p_con->cursor ++;
					i++;
				}
			}
			break;
		//一个新的模块：输入Esc，即保护模式的进入和退出。
		case '\1':// Esc
			// Esc 进入/退出
			if(find_pattern == 0){
				//在正常模式下输入Esc
				// 进入查找模式
				//改变find_pattern
				find_pattern = find_pattern ? 0 : 1;
				print_color = find_pattern ? RED_COLOR : DEFAULT_CHAR_COLOR;
				//记录现有光标的地址即为开始查找的位置。
				start_find_index = p_con->cursor;
				//记录现在ctrl+Z栈的位置。
				esc_postion = position;
			}else if(find_pattern == 2){
				//如果是结束模式，即已经查找完毕。
				// 结束Esc模式，恢复颜色
				u8* current_char = (u8*)(V_MEM_BASE);
				// 双指针
				unsigned int i = 0;
				//对于每一个字都遍历一下
				while(i < (p_con->cursor - p_con->original_addr ) * 2){
					//恢复的时候要把查找模式下要显示的所有字全都删除。
					if( p_con->original_addr * 2 + i >= start_find_index * 2){
						*(current_char + i) = ' ';
					}
					//查找模式输入字母末尾的\t和\n也要删去，并且把颜色改回来。
					if(*(current_char + i) == '\t'){
						if(i >= (start_find_index - p_con->original_addr) * 2){
							*(current_char + i) = ' ';
							*(current_char + i + 1) = DEFAULT_CHAR_COLOR;
						}
					}else if(*(current_char + i) != '\n'){
						*(current_char + i + 1) = DEFAULT_CHAR_COLOR;
					}
					i+=2;
				
				}
				p_con->cursor = start_find_index;
				//把光标返回至进入查找模式时的位置。
				// 解决光标问题
				//输入的地址指针和颜色说明也要改一下。
				p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
				*(p_vmem + 1) = DEFAULT_CHAR_COLOR;
				//重置模式标记
				find_pattern = 0;
				print_color = DEFAULT_CHAR_COLOR;

				// ctrl+Z的栈也要还原。
				position = esc_postion;
			}
			break;
		case '\2':
		//一个新的模块，对于ctrl+Z的处理。
			ctrl_pattern = -1;
			// 处理的过程中不能打断，因此要添加一个锁。
			p_con->is_ctrl_z=1;
			//如果栈里还有东西
			if(position >= 1){
				// 划分开查找模式和正常模式
				int tmp = 1;
				//查找模式什么都不做
				if(find_pattern == 1){
					if(position <= esc_postion + 1){
						tmp = 0;
					}
				}
				//temp是1即为非查找模式，要进行上一个操作的撤销。
				if(tmp == 1){
					//退栈
					position--;
					//把字符输出
					out_char(p_con, p_con->chars[position]);
				}
			}
			// 解除锁
			p_con->is_ctrl_z=0;

			break;
		//一个新的模块，输入其他普通字符。
		default:
			// 输出一般字符
			if (p_con->cursor <
					p_con->original_addr + p_con->v_mem_limit - 1) {
				//把要输出的内容放在console的对应空间。
				*p_vmem++ = ch;
				//指定要输出的颜色。
				*p_vmem++ = print_color;
				*(p_vmem+1) = print_color;
				p_con->cursor++;
			}
			break;
	}
	// 不是ctrl-z以及没有锁（即不在处理ctrl相关内容），则添加
	if(ctrl_pattern != -1 && p_con->is_ctrl_z == 0){
		position = position % CTRL_LENGTH;
		ctrlZ(p_con, next_char);
	}
	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
	//超过一屏的大小就要滚屏。
		scroll_screen(p_con, SCR_DN);
	}
//调用flush方法就是用设定好的所有缓存重新刷屏。
	flush(p_con);
}
//刷屏
/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
        set_cursor(p_con->cursor);
        set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	// 调整光标位置
	//本身就有的方法
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
//原有的方法
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
//原有的方法
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	// 切换控制台函数
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			// 向上滚屏调整
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			// 向下滚屏调整
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}


/*======================================================================*
                           clear
*======================================================================*/
PUBLIC void clear(CONSOLE* p_con)
{
	// 清空屏幕
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	int i = 2;
	//当光标不在初始位置的时候
    while(p_con->cursor != p_con->original_addr){
		//把所有的字和颜色都刷一遍
		*(p_vmem-i) = '\0';
		*(p_vmem-i+1) = DEFAULT_CHAR_COLOR;
		i+=2;
		//把光标指针还原。
		p_con->cursor--;
	}
	// 清空ctrl缓冲区
	//所有相关的状态标记都还原。
	position = 0;
	esc_postion = 0;
	flush(p_con);
}