/* $OpenBSD: lcd.c,v 1.1 2004/04/21 15:23:52 aoyama Exp $ */
/* $NetBSD: lcd.c,v 1.2 2000/01/07 05:13:08 nisimura Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Taken from NetBSD/luna68k */

/*
 * XXX
 * Following code segments are subject to change.
 * XXX
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#define PIO1_MODE_OUTPUT	0x84
#define PIO1_MODE_INPUT		0x94

#define POWER	0x10

#define ENABLE	0x80
#define DISABLE	0x00

#define WRITE_CMD	(0x00 | 0x00)
#define WRITE_DATA	(0x00 | 0x40)
#define READ_BUSY	(0x20 | 0x00)
#define READ_DATA	(0x20 | 0x40)

#define LCD_INIT	0x38
#define LCD_ENTRY	0x06
#define LCD_ON		0x0c
#define LCD_CLS		0x01
#define LCD_HOME	0x02
#define LCD_LOCATE(X, Y)	(((Y) & 1 ? 0xc0 : 0x80) | ((X) & 0x0f))

struct pio {
	volatile u_int8_t portA;
        volatile unsigned : 24;
	volatile u_int8_t portB;
        volatile unsigned : 24;
	volatile u_int8_t portC;
        volatile unsigned : 24;
	volatile u_int8_t cntrl;
        volatile unsigned : 24;
};

void lcdbusywait(void);
void lcdput(int);
void lcdctrl(int);
void lcdshow(const char *);
void greeting(void);
			       /* "1234567890123456" */
static const char lcd_boot_message1[] = "OpenBSD/luna88k ";
static const char lcd_boot_message2[] = "   SX-9100/DT   ";

void
lcdbusywait()
{
	struct pio *p1 = (struct pio *)0x4D000000;
	int msb, s;

	s = splhigh();
	p1->cntrl = PIO1_MODE_INPUT;
	p1->portC = POWER | READ_BUSY | ENABLE;
	splx(s);

	do {
		msb = p1->portA & ENABLE;
		delay(5);
	} while (msb != 0);

	s = splhigh();
	p1->portC = POWER | READ_BUSY | DISABLE;
	splx(s);
}	

void
lcdput(cc)
	int cc;
{
	struct pio *p1 = (struct pio *)0x4D000000;
	int s;

	lcdbusywait();

	s = splhigh();
	p1->cntrl = PIO1_MODE_OUTPUT;

	p1->portC = POWER | WRITE_DATA | ENABLE;
	p1->portA = cc;
	p1->portC = POWER | WRITE_DATA | DISABLE;
	splx(s);
}

void
lcdctrl(cc)
	int cc;
{
	struct pio *p1 = (struct pio *)0x4D000000;
	int s;

	lcdbusywait();

	s = splhigh();
	p1->cntrl = PIO1_MODE_OUTPUT;

	p1->portC = POWER | WRITE_CMD | ENABLE;
	p1->portA = cc;
	p1->portC = POWER | WRITE_CMD | DISABLE;
	splx(s);
}

void
lcdshow(s)
	const char *s;
{
	int cc;

	while ((cc = *s++) != '\0')
		lcdput(cc);
}

void
greeting()
{
	lcdctrl(LCD_INIT);
	lcdctrl(LCD_ENTRY);
	lcdctrl(LCD_ON);

	lcdctrl(LCD_CLS);
	lcdctrl(LCD_HOME);

	lcdctrl(LCD_LOCATE(0, 0));
	lcdshow(lcd_boot_message1);
	lcdctrl(LCD_LOCATE(0, 1));
	lcdshow(lcd_boot_message2);
}
