IFDEF RAX ; x64
; --------------------------------------------------------------
; https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=vs-2019
; --------------------------------------------------------------

; Подключаем стандартные макросы MASM
include ksamd64.inc

; --------------------------------------------------------------

;	extern "C" void FFTv0_float_ASM(
;		void/*complex<float>*/ *data,
;		uint64_t data_size,
;		uint64_t direction,
;		const void/*complex<float>*/ *phasors,
;		void/*complex<float>*/ *buffer);

ftForward textequ <0>

; Определяем короткие имена для "локальных" структур
SArgs textequ <FFTv0_float_ASM$Args>
SRegs textequ <FFTv0_float_ASM$SRegs>
SStack textequ <FFTv0_float_ASM$Stack>

; Структура с описанием параметров функции
SArgs struct
  ret_addr dq ?
  _arg_data dq ? ; в функцию передаётся в rcx
  _arg_data_size dq ? ; в функцию передаётся в rdx
  _arg_direction dq ? ; в функцию передаётся в r8
  _arg_phasors dq ? ; в функцию передаётся в r9
  arg_buffer dq ? ;
SArgs ends

; Структура с описанием сохраняемых регистров
SRegs struct
	saved_rbx dq ?
	saved_rbp dq ?
	saved_r14 dq ?
	saved_r13 dq ?
	saved_r12 dq ?
	saved_rsi dq ?
	saved_rdi dq ?
SRegs ends

; Структура с описанием структуры стека внутри функции
SStack struct
  regs SRegs <> ; затем идут сохранённые регистры (push/pop)
  args SArgs <> ; в конце идут параметры функции (и место под первые 4 параметра)
SStack ends

; Тело функции
.code
FFTv0_float_ASM PROC FRAME
	; prologue
	rex_push_reg rdi
	push_reg rsi
	push_reg r12
	push_reg r13
	push_reg r14
	push_reg rbp
	push_reg rbx
.ENDPROLOG

	; TODO: Функция переделана из x86-версии. Нужно произвести чистку.

	mov SStack.args._arg_data[rsp], rcx ; rcx используется для rep movsd
	mov SStack.args._arg_data_size[rsp], rdx
	;mov SStack.args._arg_direction[rsp], r8
r_direction textequ <r8> ; Параметр храним в регистре
	;mov SStack.args._arg_phasors[rsp], r9
r_phasors textequ <r9> ; Параметр храним в регистре

	; reordering
	;float mul = 1./sqrt( (float)dataSize);
	;long l, k, j;
	;if( direction==ftForward)
	;{
	; 	// forward
	; 	for( j=0; j<dataSize; j++)
	;	{
	;		long k = rev_1(j);
	;		if( k>=j)
	;		{
	;			complexF tmp = data[k];
	;			data[k] = mul*data[j];
	;			data[j] = mul*tmp;
	;		}
	;	}
	;}
	;else
	;{
	;	// reverse
	;	buffer[0] = mul*data[0];
	;	for( j=1; j<dataSize; j++)
	;		buffer[rev_1(dataSize-j)] = mul*data[j];
	;	memcpy( data, buffer, dataSize*sizeof(complexF));
	;}

	; finit ; changes exception modes etc. see "_fpreset()", "_control87(...)"
			; let's assume FPU to be in clear state on function entrance.
	fld1
	fild SStack.args._arg_data_size[rsp]
	fsqrt
	fdivp st(1), st

	; swapping for reverse transform
	cmp r_direction, ftForward
r_direction textequ <r_direction> ; Далее этот параметр не используется, регистр r8 освобождается
	je larevEnd

	mov rbp, SStack.args._arg_data[rsp]
	mov rsi, 1
	mov rdi, SStack.args._arg_data_size[rsp]
	dec rdi
larev1:
	mov rax, [rbp + rsi*8]
	mov rcx, [rbp + rdi*8]
	mov [rbp + rdi*8], rax
	mov [rbp + rsi*8], rcx
	inc rsi
	dec rdi
	cmp rdi, rsi
	jg larev1

larevEnd:
	; bit reverse reorder
	; Reverses the bit order of the "i",
	; e.g.: 0111001111101 -> 1011111001110
	mov r8, SStack.args.arg_buffer[rsp]
	mov rbp, SStack.args._arg_data_size[rsp]
	mov rsi, SStack.args._arg_data[rsp]
	xor rbx, rbx
larr0:
	mov rdx, rbp
	shr rdx, 1
	xor rdi, rdi
	mov rcx, rbx
larr1:
	shr rcx, 1
	rcl rdi, 1
	shr rdx, 1
	jnc larr1

	shl rdi, 3
	add rdi, r8
	fld dword ptr [rsi]
	fmul st(0), st(1)
	fstp dword ptr [rdi]
	fld dword ptr [rsi + 4]
	fmul st(0), st(1)
	fstp dword ptr[rdi + 4]
	add rsi, 8

	inc rbx
	cmp rbx, rbp
	jc larr0

	fstp st(0)
	; memcpy( data, pBPFBuf, size*sizeof(QComplex));
	mov rsi, SStack.args.arg_buffer[rsp]
	mov rdi, SStack.args._arg_data[rsp]
	mov rcx, SStack.args._arg_data_size[rsp]
	shl rcx, 1
	rep movsd

	;
	; transform
	;
	;long l, k, j;
	;for( l=2; l<=dataSize; l*=2)
	;	for( k=0; k<dataSize; k+=l)
	;		for( j=0; j<l/2; j++)
	;		{
	;			complexF t, t1;
	;			t = data[k+j];
	;			t1 = data[k+j+l/2]*phasors[j*(dataSize/l)];
	;			data[k+j] = t + t1;
	;			data[k+j+l/2] = t - t1;
	;		}

	mov rbx, SStack.args._arg_data_size[rsp]
	mov rbp, SStack.args._arg_data[rsp]

	;for( l = 2; l<=size; l*=2)
	;{
	;	for( k=0; k<size; k+=l)
	;		for( j=0; j<l/2; j++)
	;		{
	mov rcx, 8
	shl rbx, 2
	mov rdi, rbx
lbpf0:
		mov rax, rcx
		mov r11, rbp
		mov rsi, r_phasors
lbpf1:
			mov r10, rbp
			mov rdx, rdi
lbpf2:
				;t = data[k+j];
				;t1 = data[k+j+l/2];
				;data[k+j] = t + t1*W[j*(size/l)];
				;data[k+j+l/2] = t - t1*W[j*(size/l)];

				; t1=x+iy, W[]=u+iv, t1*W[]=p+iq
				; t0
				fld dword ptr [rbp + 4] ; t.im (in the NPX stack)
				fld dword ptr [rbp] ; t.re, t.im
				; t1
				add rbp, rcx
				fld dword ptr [rbp] ; x, nn, nn
				fld dword ptr [rbp + 4] ; y, x, nn, nn
				sub rbp, rcx

				; [pW]+j*size/l
				fld dword ptr [rsi] ; u, y, x, nn, nn
				fld dword ptr [rsi+4] ; v, u, y, x, nn, nn

				fld ST(1) ; u, v, u, y, x, nn, nn
				fmul ST(0), ST(4) ; x*u, v, u, y, x, nn, nn
				fld ST(1) ; v, x*u, v, u, y, x, nn, nn
				fmul ST(0), ST(4) ; y*v, x*u, v, u, y, x, nn, nn
				fsubp ST(1), ST(0) ; p, v, u, y, x, nn, nn
				fxch ST(3) ; y, v, u, p, x, nn, nn
				fmulp ST(2), ST(0) ; v, y*u, p, x, nn, nn
				fmulp ST(3), ST(0) ; y*u, p, x*v, nn, nn
				faddp ST(2), ST(0) ; p, q, t.re, t.im

				; save t0
				fld ST(0) ; p, p, q, t.re, t.im
				fadd ST(0), ST(3) ; F0.re, p, q, t.re, t.im
				fstp dword ptr [rbp] ; p, q, t.re, t.im
				fld ST(1) ; q, p, q, t.re, t.im
				fadd ST(0), ST(4) ; F0.im, p, q, t.re, t.im
				fstp dword ptr [rbp + 4] ; p, q, t.re, t.im
				; save t1
				fsubp ST(2), ST(0) ; q, F1.re, t.im
				fsubp ST(2), ST(0) ; F1.re, F1.im
				add rbp, rcx
				fstp dword ptr[rbp] ; F1.im
				fstp dword ptr[rbp + 4] ; empty

				add rbp, rcx
				sub rdx, rcx
			jnz lbpf2
			mov rbp, r10
			add rsi, rbx
			add rbp, 8
			sub rax, 8
		jnz lbpf1
		mov rbp, r11
		shl rcx, 1
		shr rbx, 1
		and rbx, NOT 07h
	jz lbpf3
	jmp lbpf0
lbpf3:

r_phasors textequ <r_phasors>
  ; epilogue
  pop rbx
  pop rbp
  pop r14
  pop r13
  pop r12
  pop rsi
  pop rdi
  ret

FFTv0_float_ASM ENDP

; "Удаляем" определения локальных имён
SArgs textequ <SArgs>
SRegs textequ <SRegs>
SStack textequ <SStack>

; --------------------------------------------------------------

ENDIF ; x64
END
