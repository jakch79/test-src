/*	$OpenBSD: hilkbd.c,v 1.1 2003/02/11 19:39:30 miod Exp $	*/
/*
 * Copyright (c) 2003, Miodrag Vallat.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/hil/hilreg.h>
#include <dev/hil/hilvar.h>
#include <dev/hil/hildevs.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <dev/hil/hilkbdmap.h>

struct hilkbd_softc {
	struct device	sc_dev;

	int		sc_code;
	int		sc_numleds;
	int		sc_ledstate;

	struct device	*sc_wskbddev;
};

#define	HILKBD_LEDS	0xf0	/* keyboard has leds */
#define	HILKBD_NLEDS	0x70	/* keyboard led mask */

int	hilkbdprobe(struct device *, void *, void *);
void	hilkbdattach(struct device *, struct device *, void *);

struct cfdriver hilkbd_cd = {
	NULL, "hilkbd", DV_DULL
};

struct cfattach hilkbd_ca = {
	sizeof(struct hilkbd_softc), hilkbdprobe, hilkbdattach
};

int	hilkbd_enable(void *, int);
void	hilkbd_set_leds(void *, int);
int	hilkbd_ioctl(void *, u_long, caddr_t, int, struct proc *);

const struct wskbd_accessops hilkbd_accessops = {
	hilkbd_enable,
	hilkbd_set_leds,
	hilkbd_ioctl,
};

void	hilkbd_cngetc(void *, u_int *, int *);
void	hilkbd_cnpollc(void *, int);
void	hilkbd_cnbell(void *, u_int, u_int, u_int);

const struct wskbd_consops hilkbd_consops = {
	hilkbd_cngetc,
	hilkbd_cnpollc,
	hilkbd_cnbell,
};

struct wskbd_mapdata hilkbd_keymapdata = {
	hilkbd_keydesctab,
#ifdef HILKBD_LAYOUT
	HILKBD_LAYOUT,
#else
	KB_US,
#endif
};

void	hilkbd_bell(struct hil_softc *, u_int, u_int, u_int);
void	hilkbd_callback(void *, u_int, u_int8_t *);
void	hilkbd_decode(u_int8_t, u_int8_t, u_int *, int *);

int
hilkbdprobe(struct device *parent, void *match, void *aux)
{
	struct hil_attach_args *ha = aux;

	if (ha->ha_type != HIL_DEVICE_KEYBOARD)
		return (0);

	return (1);
}

void
hilkbdattach(struct device *parent, struct device *self, void *aux)
{
	struct hilkbd_softc *sc = (void *)self;
	struct hil_attach_args *ha = aux;
	struct wskbddev_attach_args a;
	u_int8_t db, layoutcode;

	sc->sc_code = ha->ha_code;

	/*
	 * Put the keyboard in raw mode
	 */
	db = 0;
	send_hil_cmd((struct hil_softc *)parent, HIL_WRITEKBDSADR, &db, 1, NULL);

	/*
	 * Determine the keyboard language configuration, but don't
	 * override a user-specified setting.
	 */
	layoutcode = ha->ha_id & (MAXHILKBDLAYOUT - 1);
#ifndef HILKBD_LAYOUT
	if (layoutcode < MAXHILKBDLAYOUT &&
	    hilkbd_layouts[layoutcode] != -1)
		hilkbd_keymapdata.layout = hilkbd_layouts[layoutcode];
#endif

	printf(", layout %x", layoutcode);

	/*
	 * Interpret the extended id information, if any
	 */
	if (ha->ha_infolen > 2) {
		if (ha->ha_info[2] & HILKBD_LEDS) {
			sc->sc_numleds = (ha->ha_info[2] & HILKBD_NLEDS) >> 4;
			printf(", %d leds", sc->sc_numleds);
		}
	}

	hil_callback_register((struct hil_softc *)parent, ha->ha_code,
	    hilkbd_callback, sc);

	printf("\n");

	a.console = ha->ha_flags;
	a.keymap = &hilkbd_keymapdata;
	a.accessops = &hilkbd_accessops;
	a.accesscookie = sc;

	if (a.console) {
		wskbd_cnattach(&hilkbd_consops, sc, &hilkbd_keymapdata);
	}

	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
}

int
hilkbd_enable(void *v, int on)
{
	return (0);
}

void
hilkbd_set_leds(void *v, int leds)
{
	struct hilkbd_softc *sc = v;
	int changemask;

	if (sc->sc_numleds == 0)
		return;

	changemask = leds ^ sc->sc_ledstate;
	if (changemask == 0)
		return;

	/* We do not handle more than 3 leds here */
	if (changemask & WSKBD_LED_SCROLL)
		send_hildev_cmd((struct hil_softc *)sc->sc_dev.dv_parent,
		    sc->sc_code,
		    (leds & WSKBD_LED_SCROLL) ? HIL_PROMPT1 : HIL_ACK1);
	if (changemask & WSKBD_LED_NUM)
		send_hildev_cmd((struct hil_softc *)sc->sc_dev.dv_parent,
		    sc->sc_code,
		    (leds & WSKBD_LED_NUM) ? HIL_PROMPT2 : HIL_ACK2);
	if (changemask & WSKBD_LED_CAPS)
		send_hildev_cmd((struct hil_softc *)sc->sc_dev.dv_parent,
		    sc->sc_code,
		    (leds & WSKBD_LED_CAPS) ? HIL_PROMPT3 : HIL_ACK3);

	sc->sc_ledstate = leds;
}

int
hilkbd_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct hilkbd_softc *sc = v;

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_HIL;
		return 0;
	case WSKBDIO_SETLEDS:
		hilkbd_set_leds(v, *(int *)data);
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = sc->sc_ledstate;
		return 0;
	case WSKBDIO_COMPLEXBELL:
#define	d ((struct wskbd_bell_data *)data)
		hilkbd_bell((struct hil_softc *)sc->sc_dev.dv_parent,
		    d->pitch, d->period, d->volume);
#undef d
		return 0;
	}

	return -1;
}

void
hilkbd_cngetc(void *v, u_int *type, int *data)
{
	struct hil_softc *sc = v;
	u_int8_t c, stat;

	/* XXX This should really filter on keyboard events only */
	for (;;) {
		if (hil_poll_data(sc, &stat, &c) != -1)
			break;
	}
	hilkbd_decode(stat, c, type, data);
}

void
hilkbd_cnpollc(void *v, int on)
{
	struct hil_softc *sc = v;

	hil_set_poll(sc, on);
}

void
hilkbd_cnbell(void *v, u_int pitch, u_int period, u_int volume)
{
	struct hil_softc *sc = v;

	hilkbd_bell(sc, pitch, period, volume);
}

void
hilkbd_bell(struct hil_softc *sc, u_int pitch, u_int period, u_int volume)
{
	u_int8_t buf[2];

	/* XXX there could be at least a pitch -> HIL pitch conversion here */
#define	BELLDUR		80	/* tone duration in msec (10-2560) */
#define	BELLFREQ	8	/* tone frequency (0-63) */
	buf[0] = ar_format(period - 10);
	buf[1] = BELLFREQ;
	send_hil_cmd(sc, HIL_SETTONE, buf, 2, NULL);
}

void
hilkbd_callback(void *v, u_int buflen, u_int8_t *buf)
{
	struct hilkbd_softc *sc = v;
	u_int type;
	int key;
	int i;

	if (buflen > 1 && *buf == HIL_KBDDATA) {
		for (i = 1, buf++; i < buflen; i++) {
			hilkbd_decode(0, *buf++, &type, &key);
			if (sc->sc_wskbddev != NULL)
				wskbd_input(sc->sc_wskbddev, type, key);
		}
	}
}

void
hilkbd_decode(u_int8_t stat, u_int8_t data, u_int *type, int *key)
{
	*type = (data & 1) ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;
	*key = data >> 1;
}
