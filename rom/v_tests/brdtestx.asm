; 🐟 Вектор-06ц: тест сброса бордюра в 0 при установке
;    порта 02 на вывод
;    Ожидаемая картинка:
; 	8 строк во всю ширь
;	8 строк только на ширину растровой части экрана, черный бордюр
; 	повторяется 8 раз разными оттенками печального цвета

        	.binfile brdtestx.rom
        	.download bin  
        	.tape v06c-rom 	

МАКОВКА		equ $80d9
STRIPE_HEIGHT	equ 16

        	.org 100h

start:
		xra	a
		out	10h
		lxi	sp,100h
		mvi	a,0C3h
		sta	0
		lxi	h, Ещё
		shld	1

		mvi	a,0C9h
		sta	38h
		ei
		hlt

		lxi	h, colors+15
colorset:
		mvi	a, 88h
		out	0
		mvi	c, 15
colorset1:	mov	a, c
		out	2
		mov	a, m
		out	0Ch
		dcx	h
		out	0Ch
		out	0Ch
		dcr	c
		out	0Ch
		out	0Ch
		out	0Ch
		jp	colorset1
		mvi	a,255
		out	3


Ещё:
		call	вам_темно

		lxi h, МАКОВКА
		mvi e, $8
следующая_полоска:
		push h
		push d
		call полоска
полоска
		pop d
		pop h
		mov a, l
		sui STRIPE_HEIGHT
		mov l, a
		dcr e
		jp следующая_полоска
синхра
		ei
		hlt
		mvi a, 0
		out 2
		xthl
		xthl
		xthl
		xthl

		call тормоз
		call тормоз
		call тормоз
		call тормоз
		call тормоз
		call тормоз
		call тормоз
		call тормоз
		call тормоз
		call тормоз
		
		mvi b, 8
бордюр:
		mov a, b
		out 2
		call тормоз
		nop
		nop
		nop
		nop
		nop
		mvi a, $88
		out 0
		call тормоз
		dcr b
		jp бордюр
		jmp синхра


		; нарисовать горизонтальную полоску
		; HL = столбец/строка
		; E = цвет
полоска:
		mov a, e
		ral 
		ral
		ral
		ral
		mov e, a

		mvi a, 4
полоска_слой:
		push psw 	; счетчик слоев полежит на стеке


		; взять старший бит из E
		mov a, e
		ral
		mov e, a
		; и расширить его на весь D
		mvi d, $ff
		jc $+5
		mvi d, 0

		mvi c, 32	; заполняем все 32 колонки
полоска_вертикаль
		mvi b, STRIPE_HEIGHT
полоскин_столбец:
		mov m, d
		dcr l
		dcr b
		jp полоскин_столбец
		
		mov a, l 	; следующий столбец
		adi STRIPE_HEIGHT+1
		mov l, a
		inr h

		dcr c
		jnz полоска_вертикаль
		; HL уже показывает на следующий слой, удобно
		pop psw
		dcr a
		jnz полоска_слой

		ret

		; одна строка = 192 куклесов, 8 строк = 1536
		; (примерно)
тормоз:		; == 192
		mvi c, 33	; 12

туго:
		in 0 		; 12
		in 1		; 12
		dcr c 		; 8
		jnz туго	; 12 		= 176

		ret		; 16

вам_темно:
		lxi	h,08000h
		mvi	e,$00
		xra	a
темно_ц:
		mov	m,e
		inx	h
		cmp	h
		jnz	темно_ц
		ret

colors:
		.db 00000000b,00001001b,00010010b,00011011b,00100100b,00101101b,00110110b,00111111b
		.db 11111111b,00001001b,00010010b,00011011b,00100100b,00101101b,00110110b,00111111b