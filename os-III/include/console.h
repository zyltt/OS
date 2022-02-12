
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
//这里修改了很多。
#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

typedef struct operation OPERATION;

struct operation
{
	
	char this_char;
	int pattern; // 0代表插入, 1表示删除，-1表示头结点
	OPERATION * last_operation;
	
};

// Ctrl缓冲区大小。即按了ctrl之后就要等组合键的输入
#define CTRL_LENGTH 500

/* CONSOLE */
// console 结构体
typedef struct s_console
{
	unsigned int	current_start_addr;		/* 当前显示到了什么位置	  */
	unsigned int	original_addr;			/* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;			/* 当前控制台占的显存大小 */
	unsigned int	cursor;					/* 当前光标位置 */
	int 			is_ctrl_z;				/* 当前是否是Ctrl-Z */
	char			chars[CTRL_LENGTH];		/* Ctrl缓冲区 */
}CONSOLE;


#define SCR_UP	1	/* scroll forward */
#define SCR_DN	-1	/* scroll backward */

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80
#define TAB_WIDTH			4

// 黑底白字
#define DEFAULT_CHAR_COLOR	0x07
// 黑底透明
#define NO_COLOR			0x00
// 黑底蓝字
#define BLUE_COLOR			0x01
// 黑底红字
#define RED_COLOR			0x0c

#endif /* _ORANGES_CONSOLE_H_ */
