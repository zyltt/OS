global print

; print(pointer: char*): void
print:
	mov ecx, dword[esp + 4]
	mov eax, ecx
	; edx ÊÇ³¤¶È
	mov edx, 0

strLength:
	cmp [ecx], byte 0
	je strFinished
	inc edx
	inc ecx

	jmp strLength

strFinished:
	mov ecx, eax
	mov eax, 4
	mov ebx, 1
	int 80h
	ret
	
