%define ZERO 0
%define MAX 21;加法的最大位数
%define ADD_MAX 22;乘法结果的最大位数
%define DIGIT 20;指的是乘法列竖式子的重合的位数,即为需要标记是否进位的位数。
%define MUL_MAX 42
;注：2^64是二十位，因此本项目实现的加法乘法器最大位数为21位
section .data
    msg: db "Please input x and y: ",0Ah
    .len: equ $-msg
    newline: db 0Ah
    .len: equ $-newline
    negsign:db "-"
    .len:equ $-negsign
section .bss
    num1: resb MAX;num1代表第一个数
    num2: resb MAX;num2代表第二个数
    len1: resb 4;len1代表已经读取的第一个数的位数
    len2: resb 4;len2代表已经读取的第二个数的位数
    thisDigit: resb 1;每次读取一个字节
    zero_in_num1: resb 4;表示补零时x的0的个数
    zero_in_num2: resb 4;表示补零时y的0的个数
    add_temp1: resb 4
    add_temp2: resb 4
    add_result: resb ADD_MAX
    mul_temp1: resb 4;
    mul_temp2: resb 4;
    mul_temp3: resb 4;
    mul_res_temp: resb 4;
    flag1: resb 4
    flag2: resb 4
    xisnegative: resb 4;变量用于记录x的正负，0为正，1为负数
    yisnegative: resb 4;变量用于记录y的正负，0为正，1为负数
    x_larger_than_y:resb 4;变量用于记录xy的大小，x比y大即为1，相等为2，小为0
section .data
    mul_temp: db 0,0,0,0
    mul_temp_sum: db 0,0,0,0
    mul_result: db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;mulresult记录的是全乘法器的中间结果。
section .text
global main

main:
    mov ebp, esp; for correct debugging
    mov dword[len1],ZERO
    mov dword[len2],ZERO
    mov dword[xisnegative],ZERO
    mov dword[yisnegative],ZERO
    ;进行提示字符的输出，eax=4,ebx=1代表输出，
    mov eax,4
    mov ebx,1
    mov ecx,msg
    mov edx,msg.len
    int 80h
    ;返还调用的时候会把edx的值赋给eax
    ;下面开始读第一个数
readNum1:
    mov eax,3
    mov ebx,0
    mov ecx,thisDigit;直接写的代表取地址，加括号的代表取值。
    mov edx,1;每次读一位
    int 80h
    ;如果读到了符号的话就做一个标记
    cmp byte[thisDigit],45;45是负号
    jne x_is_positive
    mov dword[xisnegative],1
    jmp readNum1
x_is_positive:    
    ;这里的意思是读到空格为止。
    cmp byte[thisDigit],32;32是空格
    je readNum2
    ;没读到空格的时候，就把现在读的这个字符加上去   
    mov eax,num1;读num1的地址
    add eax,dword[len1];+len1为写入之后的地址。（地址加一为加一个b）
    mov bl,byte[thisDigit]
    mov byte[eax],bl;把eax指示的位置的后面一位赋值。
    inc dword[len1]
    jmp readNum1

readNum2:
    mov eax,3
    mov ebx,0
    mov ecx,thisDigit
    mov edx,1
    int 80h
    ;如果读到了符号的话就做一个标记
    cmp byte[thisDigit],45;45是负号
    jne y_is_positive
    mov dword[yisnegative],1
    jmp readNum2
y_is_positive:
    ;这里的意思是读到换行符为止。        
    cmp byte[thisDigit],0ah
    je init_count_x
    mov eax,num2
    add eax,dword[len2]
    mov bl,byte[thisDigit]
    mov byte[eax],bl;
    inc dword[len2]
    jmp readNum2
;下次从这里开始写


;对x进行扩展
init_count_x:
    mov eax,dword[len1]
    mov ecx,eax;把x的长度赋值给ecx
;下面的这一部分要把x从前面的位置往后移动max位。以利于负号扩展。
;如果x的长度是0，直接零扩展即可。
move_x:
    cmp ecx,ZERO
    je x_extend_0    
    mov eax,num1;eax保存num1的地址
    add eax,ecx
    dec eax;eax指向num1的还没复制的最后一位的地址
    mov bl,byte[eax];把num1最后一位的值取出来。
    add eax,MAX
    sub eax,dword[len1];新扩展了MAX位的空间，eax指向留出len1的长度的位置。
    mov byte[eax],bl
    dec ecx
    jmp move_x
;对x进行0扩展
x_extend_0:
;判断max和len1是否相等。如果相等就做完了，如果不相等就在前面多补一些0
    mov eax,dword[len1]
    mov ebx,MAX
    sub ebx,eax;这里ebx存的是还需要补零的个数。
x_continue_extend_0:        
    cmp ebx,0
    je init_count_y
    mov eax,num1
    add eax,ebx
    dec eax;eax这里存的是要补零的地址。
    mov byte[eax],'0'
    dec ebx
    jmp x_continue_extend_0



;对y进行扩展
init_count_y:
    mov eax,dword[len2]
    mov ecx,eax;把y的长度赋值给ecx
;下面的这一部分要把y从前面的位置往后移动max位。以利于负号扩展。
;如果y的长度是0，直接零扩展即可。
move_y:
    cmp ecx,ZERO
    je y_extend_0
    
    mov eax,num2;eax保存num2的地址
    add eax,ecx
    dec eax;eax指向num2的还没复制的最后一位的地址
    mov bl,byte[eax];把num2最后一位的值取出来。
    add eax,MAX
    sub eax,dword[len2];新扩展了MAX位的空间，eax指向留出len2的长度的位置。
    mov byte[eax],bl
    dec ecx
    jmp move_y
;对y进行0扩展
y_extend_0:
    ;判断max和len2是否相等。如果相等就做完了，如果不相等就在前面多补一些0
    mov eax,dword[len2]
    mov ebx,MAX
    sub ebx,eax;这里ebx存的是还需要补零的个数。
y_continue_extend_0:        
    cmp ebx,0
    je judge_x_larger_than_y
    mov eax,num2
    add eax,ebx
    dec eax;eax这里存的是要补零的地址。
    mov byte[eax],'0'
    dec ebx
    jmp y_continue_extend_0
    ;扩展完成后可以对xy进行大小比较
judge_x_larger_than_y:
    mov dword[x_larger_than_y],2;如果始终没有修改，即代表xy相等，即为2
    mov ecx,-1;ecx放局部变量i,程序写的不好，进去就得加一个，所以这里调成了-1
    
continue_judge:
    inc ecx    
    mov eax,num1;eax放num1的地址
    mov ebx,num2;ebx放num2的地址
    cmp ecx,MAX
    jnl init_add
    add eax,ecx;eax放num1要判断的位的地址
    add ebx,ecx;ebx放num2要判断的位的地址
    mov dl,byte[eax];
    mov dh,byte[ebx];分别把两个要判断的位取出来。
    cmp dl,dh
    je continue_judge
    jg x_is_larger_than_y
    jl x_is_less_than_y
    jmp judge_x_larger_than_y
x_is_larger_than_y:
    mov dword[x_larger_than_y],1
    jmp init_add
x_is_less_than_y:
    mov dword[x_larger_than_y],0
    jmp init_add
    




;加法（这里只是开始进行加法，具体是加法运算还是减法运算还没判断）
init_add:
    ;判断是做加法运算还是减法运算，结果只取一个正值。还做了一些变量的初始化。
    mov dword[add_temp1],MAX;按照位置一位一位的加，add_temp1里面记录的是还没有进行加法的位数。
    mov dh,0;dh用来记录进位,如果是做减法，就用来记录借位。
    mov eax,dword[xisnegative]
    mov ebx,dword[yisnegative]
    cmp eax,ebx
    ;同号相加
    je add_1
    ;没有跳转说明异号，根据绝对值大小进行两种减法。
    cmp dword[x_larger_than_y],1
    ;x>y作x-y
    je x_sub_y
    ;x<=y,y-x
    jmp y_sub_x
x_sub_y:
    sub_1:
    ;step1:进行计算
    cmp dword[add_temp1],ZERO;判断是否所有位数都处理完了，处理完了就结束
    je sub_result_carry
    mov eax,num1
    add eax,dword[add_temp1]
    dec eax;eax找到num1要处理的最后一位的地址
    
    mov ebx,num2
    add ebx,dword[add_temp1]
    dec ebx;ebx找到num2要处理的最后一位的地址
    
    ;一位一位相减判断
    mov cl,byte[eax]
    sub cl,'0';cl里面存的是num1这一位数字的数值
    mov dl,byte[ebx]
    sub dl,'0';dl里面存的是num2这一位数字的数值
    sub cl,dl
    sub cl,dh;两个数字的差减去借位，将结果存入cl中
    cmp cl,0
    jl borrow;如果数值小于0的话就要处理一下进位
    mov dh,0
sub_2:
    ;step2:进行结果字符串的还原
    add cl,'0';把结果的数值还原成字符
    mov eax,add_result
    add eax,dword[add_temp1]
    mov byte[eax],cl;把结果写入add_result的相应位置。由于加法要放一个最高位用来存进位，所以计算地址的时候差了一个1.
    dec dword[add_temp1]
    jmp sub_1
;减法借位
borrow:
    mov dh,1
    add cl,10
    jmp sub_2
sub_result_carry:
;虽然肯定没有借位，但为了和加法保持形式和位数上的一致性，还是做一个借位处理。
    add dh,'0'
    mov eax,add_result
    mov byte[eax],dh
    jmp print_add_result





y_sub_x:
reverse_sub_1:
    cmp dword[add_temp1],ZERO;判断是否所有位数都处理完了，处理完了就结束
    je reverse_sub_result_carry
    mov eax,num1
    add eax,dword[add_temp1]
    dec eax;eax找到num1要处理的最后一位的地址
    
    mov ebx,num2
    add ebx,dword[add_temp1]
    dec ebx;ebx找到num2要处理的最后一位的地址
    
    ;一位一位相减判断
    mov dl,byte[eax]
    sub dl,'0';dl里面存的是num1这一位数字的数值
    mov cl,byte[ebx]
    sub cl,'0';cl里面存的是num2这一位数字的数值
    sub cl,dl
    sub cl,dh;两个数字的差减去借位，将结果存入cl中
    cmp cl,0
    jl reverse_borrow;如果数值小于0的话就要处理一下进位
    mov dh,0
reverse_sub_2:
    add cl,'0';把结果的数值还原成字符
    mov eax,add_result
    add eax,dword[add_temp1]
    mov byte[eax],cl;把结果写入add_result的相应位置。由于加法要放一个最高位用来存进位，所以计算地址的时候差了一个1.
    dec dword[add_temp1]
    jmp reverse_sub_1
;减法借位
reverse_borrow:
    mov dh,1
    add cl,10
    jmp reverse_sub_2
reverse_sub_result_carry:
    ;虽然肯定没有借位，但为了和加法保持形式和位数上的一致性，还是做一个借位处理。
    add dh,'0'
    mov eax,add_result
    mov byte[eax],dh
    jmp print_add_result




;加法
add_1:
    cmp dword[add_temp1],ZERO;判断是否所有位数都处理完了，处理完了就开始处理最高位进位
    je result_carry
    mov eax,num1
    add eax,dword[add_temp1]
    dec eax;eax找到num1要处理的最后一位的地址
    
    mov ebx,num2
    add ebx,dword[add_temp1]
    dec ebx;ebx找到num2要处理的最后一位的地址
    
    ;一位一位相加判断
    mov cl,byte[eax]
    sub cl,'0';cl里面存的是num1这一位数字的数值
    mov dl,byte[ebx]
    sub dl,'0';dl里面存的是num2这一位数字的数值
    add cl,dl
    add cl,dh;两个数字的和加上进位，将结果存入cl中
    cmp cl,10
    jnl carry;如果数值超过10的话就要处理一下进位
    mov dh,0
add_2:
    add cl,'0';把结果的数值还原成字符
    mov eax,add_result
    add eax,dword[add_temp1]
    mov byte[eax],cl;把结果写入add_result的相应位置。由于这里要放一个最高位用来存进位，所以计算地址的时候差了一个1.
    dec dword[add_temp1]
    jmp add_1
;加法进位
carry:
    mov dh,1
    sub cl,10
    jmp add_2
result_carry:
    add dh,'0'
    mov eax,add_result
    mov byte[eax],dh
;到这里为止所有计算都完成了，后面的就是输出了。
print_add_result:
    ;跳转到这一步说明已经判断出结果不是全零，把结果打印出来即可。
    ;如果结果应该是负数，需要输出一个负号
    cmp dword[xisnegative],1;先通过x的正负进行一次跳转。
    je add_sign_print_x_negative
add_sign_print_x_positive:
    ;再根据y的正负进行相应的处理
    cmp dword[yisnegative],0
    je  judge_result_is_zero
    ;如果上面没有跳转说明y是负的，这时候再判断xy的绝对值大小
    cmp dword[x_larger_than_y],0
    ;x>=y说明是正的，不用输出负号
    jne judge_result_is_zero
    ;如果上面没有跳转说明x<y,要输出负号再跳转。
    mov eax,4
    mov ebx,1
    mov ecx,negsign;
    mov edx,negsign.len
    int 80h
    jmp judge_result_is_zero
add_sign_print_x_negative:
    ;根据y的符号分为x负y负，x负y正
    cmp dword[yisnegative],0
    je add_sign_print_y_positive
    ;如果这里也没有跳转，说明y也是负的，直接输出负号跳转即可。
    mov eax,4
    mov ebx,1
    mov ecx,negsign;
    mov edx,negsign.len
    int 80h
    jmp judge_result_is_zero
add_sign_print_y_positive:
    cmp dword[x_larger_than_y],1
    ;x<=y说明是正的，不用输出负号
    jne judge_result_is_zero
    ;如果上面没有跳转说明x<y,要输出负号再跳转。
    mov eax,4
    mov ebx,1
    mov ecx,negsign;
    mov edx,negsign.len
    int 80h
    jmp judge_result_is_zero
    
;消除0
judge_result_is_zero:
    ;判断以下结果是不是全是0，
    cmp dword[add_temp1],MAX;如果循环到这里说明全是0，判断最后一位如果还是0就输出0.
    je judge_last_is_zero
    mov eax,add_result
    add eax,dword[add_temp1];eax指向要判断的那一位的地址
    inc dword[add_temp1]
    mov cl,byte[eax];把这一位取出来
    cmp cl,'0';如果这一位是0的话就不输出
    je judge_result_is_zero
;上面判断这部分，如果确实到最后一位才有数字，会走judge_last_is_zero出口输出并结束。否则会往下走，输出全部的结果
print_add_num:
;复用这个变量，重新初始化一下。此时的add_temp1代表输出到了哪一位。
    mov eax,add_result
    add eax,dword[add_temp1]
    dec eax
    mov dword[add_temp2],eax
    mov eax,4
    mov ebx,1
    mov ecx,dword[add_temp2]
    mov edx,1
    int 80h
    cmp dword[add_temp1],ADD_MAX
    je init_mul
    inc dword[add_temp1]
    jmp print_add_num
judge_last_is_zero:
;判断最后一位是不是0，如果是，就输出一个0.
    mov eax,add_result
    add eax,dword[add_temp1]
    mov dword[add_temp2],eax
    mov eax,4
    mov ebx,1
    mov ecx,dword[add_temp2]
    mov edx,1
    int 80h
    jmp init_mul
    
    
    
    
    
    
;todo乘法还没写完    
;乘法开始
;采取的策略是先做乘法，最后加和再进位。
;下面两重循环的c语言版：
;for(int mul_temp1=DIGIT;mul_temp1>=0;mul_temp--)
;{  for(int mul_temp2=DIGIT;mul_temp2>=0;multemp2--){
;            int flag1=mul_temp1+mul_temp2;
;            int flag2=mul_temp1+mul_temp2+1;
;           int a=num1[mul_temp1]*num2[mul_temp2];
;           mul_res_temp[flag2]=
;
;
;
;
;
;
;                                                    }
;}
init_mul:
    ;初始化两个计数器，用于两重循环
    mov dword[mul_temp1],DIGIT
    mov dword[mul_temp2],DIGIT
;外层循环
loop_mul_1:
    cmp byte[mul_temp1],ZERO
    jl init_mul_res_temp
;循环用寄存器能快一些，这里使用esi和edi两个通用寄存器。
    mov edi,num1
    add edi,dword[mul_temp1];edi存的是num1的第mul_temp1位的地址。(即标记位的地址）
    ;内层循环
    loop_mul_2:
        mov esi,num2
        add esi,dword[mul_temp2];esi存的是num2的标记位的地址。
        
        mov al,byte[edi]
        mov bl,byte[esi]
        sub al,'0'
        sub bl,'0'
        mul bl;ax暂存每一位的乘积结果，实际只对al有效；即ax=al*bl
        
        mov ebx,mul_temp
        mov word[ebx],ax;mul_temp记录每一位的乘法结果
        
        mov ecx,dword[mul_temp1]
        add ecx,dword[mul_temp2]
        mov dword[flag1],ecx;flag1 = mul_temp1 + mul_temp2即为能放下加和部分结果的总位数。
        mov dword[flag2],ecx
        inc dword[flag2];flag2 = mul_temp1 + mul_temp2 + 1；多出一位防止溢出
        
        mov eax,mul_result
        add eax,dword[flag2];eax指向mulres的对应位。
        mov ebx,dword[mul_temp]
        add bl,byte[eax];下一位的bl是要把上一位的结果记下来的，相当于记住进位。
        mov eax,mul_temp_sum
        mov dword[mul_temp_sum],ebx;把multemp的值存入multempsum
        
        mov eax,dword[mul_temp_sum]
        mov ecx,10
        div cl
        mov edx,mul_result
        add edx,dword[flag1];找到要进位的那一位，角标为flag1
        add byte[edx],al;商在al里，余数载ah里。
        mov edx,mul_result
        add edx,dword[flag2]
        mov byte[edx],ah
        
        dec dword[mul_temp2]
        cmp dword[mul_temp2],ZERO
        jl dec_mul_temp1
        jmp loop_mul_2
    dec_mul_temp1:
        mov dword[mul_temp2],DIGIT
        dec byte[mul_temp1]
        jmp loop_mul_1
;初始化计数器mul_res_temp
init_mul_res_temp:
    call printLF;加和乘法之间要输出一个换行符。
    mov dword[mul_res_temp],ZERO
add_char_zero:
    cmp dword[mul_res_temp],MUL_MAX
    je init_mul_res_temp_1
    mov eax,mul_result
    add eax,dword[mul_res_temp]
    mov cl,byte[eax]
    add cl,'0'
    mov byte[eax],cl
    inc dword[mul_res_temp]
    jmp add_char_zero
;再次初始化一下mul_res_temp
init_mul_res_temp_1:
    mov dword[mul_res_temp],ZERO;反复使用这个变量就要反复初始。
    
;从这里开始判断是不是0，要不要加入负号的问题了
judge_mul_zero:
;共有print_mul_result和print_mul_last_zero两个出口，第一个出口用于还有多位输出，第二个出口用于只剩一位的最后判断和输出。
;为了避免0和-0的问题，在两个出口处分别加入负号输出的判断，其中若输出为0则不再输出负号。
    cmp dword[mul_res_temp],41
    je judge_mul_last_zero
    mov eax,mul_result
    add eax,dword[mul_res_temp]
    inc dword[mul_res_temp]
    mov cl,byte[eax]
    cmp cl,'0'
    je judge_mul_zero
    jmp print_mul_result
print_mul_result:
;先输出符号
    mov eax,dword[xisnegative]
    mov ebx,dword[yisnegative]
    cmp eax,ebx
    ;异号要输出一个负号
    jne mul_print_neg1
mul_continue_print:
    mov eax,mul_result
    add eax,dword[mul_res_temp]
    dec eax
    mov dword[mul_temp3],eax
    mov eax,4
    mov ebx,1
    mov ecx,dword[mul_temp3]
    mov edx,1
    int 80h
    cmp dword[mul_res_temp],MUL_MAX
    je print_lf
    inc dword[mul_res_temp]
    jmp mul_continue_print
judge_mul_last_zero:
    mov eax,mul_result
    add eax,dword[mul_res_temp]
    mov dword[mul_temp3],eax
    ;如果结果为0就输出一个0，不输出负号
    cmp dword[mul_temp3],'0'
    je mul_print_last_num
    ;如果不是0再看要不要输出负号。
    mov eax,dword[xisnegative]
    mov ebx,dword[yisnegative]
    cmp eax,ebx
    je mul_print_last_num
    ;如果异号就输出一个负号
    mov eax,4
    mov ebx,1
    mov ecx,negsign
    mov edx,negsign.len
mul_print_last_num:
        mov eax,4
    mov ebx,1
    mov ecx,dword[mul_temp3]
    mov edx,1
    int 80h
print_lf:
    call printLF
    mov eax,1
    mov ebx,0
    int 80h
printLF:
    mov eax,4
    mov ebx,1
    mov ecx,newline
    mov edx,newline.len
    int 80h
    ret
mul_print_neg1:
    mov eax,4
    mov ebx,1
    mov ecx,negsign
    mov edx,negsign.len
    int 80h
    jmp mul_continue_print