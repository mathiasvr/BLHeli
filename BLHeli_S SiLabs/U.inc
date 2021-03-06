;**** **** **** **** ****
;
; BLHeli program for controlling brushless motors in helicopters and multirotors
;
; Copyright 2011, 2012 Steffen Skaug
; This program is distributed under the terms of the GNU General Public License
;
; This file is part of BLHeli.
;
; BLHeli is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; BLHeli is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with BLHeli.  If not, see <http://www.gnu.org/licenses/>.
;
;**** **** **** **** ****
;
; Hardware definition file "U".
; MA MC CC MB RC L0 L1 L2     X  Cc Bc Ac Cp Bp Ap X     
;
;**** **** **** **** ****



;*********************
; Device SiLabs EFM8BB1x/2x
;*********************
IF MCU_48MHZ == 0
$include (SI_EFM8BB1_Defs.inc)
ELSE
$include (SI_EFM8BB2_Defs.inc)
ENDIF


;**** **** **** **** ****
; Uses internal calibrated oscillator set to 24/48Mhz
;**** **** **** **** ****

;**** **** **** **** ****
; Constant definitions
;**** **** **** **** ****
$include (layoutTag.inc)	; defines Eep_ESC_Layout and Eep_ESC_MCU tags

TEMP_LIMIT			EQU 	49	; Temperature measurement ADC value for which main motor power is limited at 80degC (low byte, assuming high byte is 1)
TEMP_LIMIT_STEP		EQU	9	; Temperature measurement ADC value increment for another 10degC

;**** **** **** **** ****
; Bootloader definitions
;**** **** **** **** ****
RTX_PORT		  		EQU 	P0		; Receive/Transmit port
RTX_MDOUT				EQU	P0MDOUT	; Set to 1 for PUSHPULL				
RTX_MDIN				EQU	P0MDIN	; Set to 1 for DIGITAL				
RTX_SKIP				EQU	P0SKIP	; Set to 1 for SKIP				
RTX_PIN				EQU 	3		; RTX pin

SIGNATURE_001			EQU	0E8h		; Device signature
IF MCU_48MHZ == 0
SIGNATURE_002			EQU	0B1h
ELSE
SIGNATURE_002			EQU	0B2h
ENDIF


;*********************
; PORT 0 definitions *
;*********************  
Mux_A		EQU	7	;i
Mux_C		EQU	6	;i
Comp_Com  	EQU	5	;i
Mux_B		EQU	4	;i
Rcp_In		EQU	3	;i
LED_0		EQU	2	;i
LED_1		EQU	1	;i
LED_2		EQU	0	;i

P0_DIGITAL	EQU	NOT((1 SHL Mux_A)+(1 SHL Mux_B)+(1 SHL Mux_C)+(1 SHL Comp_Com))
P0_INIT		EQU	0FFh
P0_PUSHPULL	EQU	(1 SHL LED_0)+(1 SHL LED_1)+(1 SHL LED_2)
P0_SKIP		EQU	0FFh

Get_Rcp_Capture_Values MACRO
	anl	TCON, #0EFh				; Disable timer0
	mov	Temp1, TL0				; Get timer0 values
	mov	Temp2, TH0
IF MCU_48MHZ == 1
	mov	Temp3, Timer0_X
	jnb	TCON_TF0, ($+4)			; Check if interrupt is pending
	inc	Temp3					; If it is pending, then timer has already wrapped
ENDIF
	mov	TL0, #0					; Reset timer0
	mov	TH0, #0
IF MCU_48MHZ == 1
	mov	Timer0_X, #0
ENDIF
	orl	TCON, #10h				; Enable timer0 again
IF MCU_48MHZ == 1
	mov	A, Clock_Set_At_48MHz
	jnz	Get_Rcp_End
	clr	C
	mov	A, Temp1
	rlc	A
	mov	Temp1, A
	mov	A, Temp2
	rlc	A
	mov	Temp2, A
	mov	A, Temp3
	rlc	A
	mov	Temp3, A
Get_Rcp_End:
ENDIF
ENDM
Decode_Dshot_2Msb MACRO
	movx	A, @DPTR
	mov	Temp6, A
	clr	C
	subb	A, Temp5					; Subtract previous timestamp
	clr	C
	subb	A, Temp1
	jc	t1_int_msb_fail			; Check that bit is longer than minimum

	subb	A, Temp1					; Check if bit is zero or one
	mov	A, Temp4					; Shift bit into data byte
	rlc	A
	mov	Temp4, A
	inc	DPL						; Next bit
	movx	A, @DPTR
	mov	Temp5, A
	clr	C
	subb	A, Temp6
	clr	C
	subb	A, Temp1
	jc	t1_int_msb_fail

	subb	A, Temp1
	mov	A, Temp4
	rlc	A
	mov	Temp4, A
	inc	DPL
ENDM
Decode_Dshot_2Lsb MACRO
	movx	A, @DPTR
	mov	Temp6, A
	clr	C
	subb	A, Temp5					; Subtract previous timestamp
	clr	C
	subb	A, Temp1
	jc	t1_int_lsb_fail			; Check that bit is longer than minimum

	subb	A, Temp1					; Check if bit is zero or one
	mov	A, Temp3					; Shift bit into data byte
	rlc	A
	mov	Temp3, A
	inc	DPL						; Next bit
	movx	A, @DPTR
	mov	Temp5, A
	clr	C
	subb	A, Temp6
	clr	C
	subb	A, Temp1
	jc	t1_int_lsb_fail

	subb	A, Temp1
	mov	A, Temp3
	rlc	A
	mov	Temp3, A
	inc	DPL
ENDM
Initialize_PCA MACRO
	mov	PCA0CN0, #40h				; PCA enabled
	mov	PCA0MD, #08h				; PCA clock is system clock
IF FETON_DELAY == 0
IF MCU_48MHZ == 0
	mov	PCA0PWM, #82h				; PCA ARSEL set and 10bits pwm
ELSE
	mov	PCA0PWM, #83h				; PCA ARSEL set and 11bits pwm
ENDIF
	mov	PCA0CENT, #00h				; Edge aligned pwm
ELSE
IF MCU_48MHZ == 0
	mov	PCA0PWM, #81h				; PCA ARSEL set and 9bits pwm
ELSE
	mov	PCA0PWM, #82h				; PCA ARSEL set and 10bits pwm
ENDIF
	mov	PCA0CENT, #03h				; Center aligned pwm
ENDIF
ENDM
Set_Pwm_Polarity MACRO
	mov	PCA0POL, #02h				; Damping inverted, pwm noninverted
ENDM
Enable_Power_Pwm_Module MACRO
IF FETON_DELAY == 0
	mov	PCA0CPM0, #4Ah				; Enable comparator of module, enable match, set pwm mode
ELSE
	mov	PCA0CPM0, #42h				; Enable comparator of module, set pwm mode
ENDIF
ENDM
Enable_Damp_Pwm_Module MACRO
IF FETON_DELAY == 0
	mov	PCA0CPM1, #00h				; Disable
ELSE
	mov	PCA0CPM1, #42h				; Enable comparator of module, set pwm mode
ENDIF
ENDM

Set_Power_Pwm_Regs MACRO byteL, byteH
	mov	PCA0CPL0, byteL
	mov	PCA0CPH0, byteH
ENDM		
Set_Damp_Pwm_Regs MACRO byteL, byteH
	mov	PCA0CPL1, byteL
	mov	PCA0CPH1, byteH
ENDM
	
Clear_COVF_Interrupt MACRO
	anl	PCA0PWM, #0DFh
ENDM
Clear_CCF_Interrupt MACRO
	anl	PCA0CN0, #0FEh
ENDM
Enable_COVF_Interrupt MACRO
	orl	PCA0PWM, #40h
ENDM
Enable_CCF_Interrupt MACRO
	orl	PCA0CPM0,#01h
ENDM
Disable_COVF_Interrupt MACRO
	anl	PCA0PWM, #0BFh
ENDM
Disable_CCF_Interrupt MACRO
	anl	PCA0CPM0,#0FEh
ENDM



;*********************
; PORT 1 definitions *
;*********************
;			EQU	7	;i
CcomFET		EQU	6	;o
BcomFET		EQU	5	;o
AcomFET		EQU	4	;o
CpwmFET		EQU	3	;i
BpwmFET		EQU	2	;o 
ApwmFET		EQU	1	;o
;			EQU	0	;o

P1_DIGITAL	EQU	(1 SHL ApwmFET)+(1 SHL BpwmFET)+(1 SHL CpwmFET)+(1 SHL AcomFET)+(1 SHL BcomFET)+(1 SHL CcomFET)
P1_INIT		EQU 	00h		
P1_PUSHPULL	EQU	(1 SHL ApwmFET)+(1 SHL BpwmFET)+(1 SHL CpwmFET)+(1 SHL AcomFET)+(1 SHL BcomFET)+(1 SHL CcomFET)
P1_SKIP		EQU	7Fh

ApwmFET_on MACRO
	setb	P1.ApwmFET
IF FETON_DELAY == 0
	setb	P1.AcomFET
ENDIF
ENDM
ApwmFET_off MACRO
IF FETON_DELAY != 0
	clr	P1.ApwmFET
ELSE
	clr	P1.AcomFET
ENDIF
ENDM
BpwmFET_on MACRO
	setb	P1.BpwmFET
IF FETON_DELAY == 0
	setb	P1.BcomFET
ENDIF
ENDM
BpwmFET_off MACRO
IF FETON_DELAY != 0
	clr	P1.BpwmFET
ELSE
	clr	P1.BcomFET
ENDIF
ENDM
CpwmFET_on MACRO
	setb	P1.CpwmFET
IF FETON_DELAY == 0
	setb	P1.CcomFET
ENDIF
ENDM
CpwmFET_off MACRO
IF FETON_DELAY != 0
	clr	P1.CpwmFET
ELSE
	clr	P1.CcomFET
ENDIF
ENDM
All_pwmFETs_Off MACRO
IF FETON_DELAY != 0
	clr	P1.ApwmFET
	clr	P1.BpwmFET
	clr	P1.CpwmFET
ELSE
	clr	P1.AcomFET
	clr	P1.BcomFET
	clr	P1.CcomFET
ENDIF
ENDM

AcomFET_on MACRO
IF FETON_DELAY == 0
	clr	P1.ApwmFET
ENDIF
	setb	P1.AcomFET
ENDM
AcomFET_off MACRO
	clr	P1.AcomFET
ENDM
BcomFET_on MACRO
IF FETON_DELAY == 0
	clr	P1.BpwmFET
ENDIF
	setb	P1.BcomFET
ENDM
BcomFET_off MACRO
	clr	P1.BcomFET
ENDM
CcomFET_on MACRO
IF FETON_DELAY == 0
	clr	P1.CpwmFET
ENDIF
	setb	P1.CcomFET
ENDM
CcomFET_off MACRO
	clr	P1.CcomFET
ENDM
All_comFETs_Off MACRO
	clr	P1.AcomFET
	clr	P1.BcomFET
	clr	P1.CcomFET
ENDM

Set_Pwm_A MACRO
IF FETON_DELAY == 0
	setb	P1.AcomFET
	mov	P1SKIP, #7Dh
ELSE
	mov	P1SKIP, #6Dh
ENDIF
ENDM
Set_Pwm_B MACRO
IF FETON_DELAY == 0
	setb	P1.BcomFET
	mov	P1SKIP, #7Bh
ELSE
	mov	P1SKIP, #5Bh
ENDIF
ENDM
Set_Pwm_C MACRO
IF FETON_DELAY == 0
	setb	P1.CcomFET
	mov	P1SKIP, #77h
ELSE
	mov	P1SKIP, #37h
ENDIF
ENDM
Set_Pwms_Off MACRO
	mov	P1SKIP, #7Fh
ENDM

Set_Comp_Phase_A MACRO
	mov	CMP0MX, #75h	; Set comparator multiplexer to phase A
ENDM
Set_Comp_Phase_B MACRO
	mov	CMP0MX, #45h	; Set comparator multiplexer to phase B
ENDM
Set_Comp_Phase_C MACRO
	mov	CMP0MX, #65h	; Set comparator multiplexer to phase C
ENDM
Read_Comp_Out MACRO
	mov	A, CMP0CN0	; Read comparator output
ENDM


;*********************
; PORT 2 definitions *
;*********************
DebugPin		EQU	0	;o

P2_PUSHPULL	EQU	(1 SHL DebugPin)


;**********************
; MCU specific macros *
;**********************
Interrupt_Table_Definition MACRO
CSEG AT 0				; Code segment start
	jmp reset
CSEG AT 03h			; Int0 interrupt	
	jmp	int0_int	
IF MCU_48MHZ == 1
CSEG AT 0Bh			; Timer0 overflow interrupt
	jmp	t0_int
ENDIF
CSEG AT 13h			; Int1 interrupt
	jmp	int1_int
CSEG AT 1Bh			; Timer1 overflow interrupt
	jmp	t1_int
CSEG AT 2Bh			; Timer2 overflow interrupt
	jmp	t2_int	
CSEG AT 5Bh			; Pca interrupt
	jmp	pca_int
CSEG AT 73h			; Timer3 overflow/compare interrupt
	jmp	t3_int	
ENDM

Initialize_Xbar MACRO
	mov	XBR2, #40h	; Xbar enabled			
	mov	XBR1, #02h	; CEX0 and CEX1 routed to pins		
ENDM

Initialize_Comparator MACRO
	mov	CMP0CN0, #80h	; Comparator enabled, no hysteresis
	mov	CMP0MD, #00h	; Comparator response time 100ns
ENDM
Initialize_Adc MACRO
	mov	REF0CN, #0Ch	; Set vdd (3.3V) as reference. Enable temp sensor and bias
IF MCU_48MHZ == 0
	mov	ADC0CF, #59h	; ADC clock 2MHz, PGA gain 1
ELSE
	mov	ADC0CF, #0B9h	; ADC clock 2MHz, PGA gain 1
ENDIF
	mov	ADC0MX, #10h	; Select temp sensor input
	mov	ADC0CN0, #80h	; ADC enabled 
	mov	ADC0CN1, #01h	; Common mode buffer enabled
ENDM
Start_Adc MACRO
	mov	ADC0CN0, #90h	; ADC start
ENDM
Read_Adc_Result MACRO
	mov	Temp1, ADC0L
	mov	Temp2, ADC0H
ENDM
Stop_Adc MACRO
ENDM
Set_RPM_Out MACRO
ENDM
Clear_RPM_Out MACRO
ENDM
Set_MCU_Clk_24MHz MACRO
	mov	CLKSEL, #13h		; Set clock to 24MHz
	mov	SFRPAGE, #10h
	mov	PFE0CN, #00h		; Set flash timing for 24MHz
	mov	SFRPAGE, #00h
	mov	Clock_Set_At_48MHz, #0
ENDM
Set_MCU_Clk_48MHz MACRO
	mov	SFRPAGE, #10h
	mov	PFE0CN, #30h		; Set flash timing for 48MHz
	mov	SFRPAGE, #00h
	mov	CLKSEL, #03h		; Set clock to 48MHz
	mov	Clock_Set_At_48MHz, #1
ENDM
Set_LED_0 MACRO
	clr	P0.LED_0
ENDM
Clear_LED_0 MACRO
	setb	P0.LED_0
ENDM
Set_LED_1 MACRO
	clr	P0.LED_1
ENDM
Clear_LED_1 MACRO
	setb	P0.LED_1
ENDM
Set_LED_2 MACRO
	clr	P0.LED_2
ENDM
Clear_LED_2 MACRO
	setb	P0.LED_2
ENDM
Set_LED_3 MACRO
ENDM
Clear_LED_3 MACRO
ENDM
