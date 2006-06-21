/*	$OpenBSD: uhci_cardbus.c,v 1.3 2006/06/21 11:27:03 fkr Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/pci/pcidevs.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/uhcireg.h>
#include <dev/usb/uhcivar.h>

int	uhci_cardbus_match(struct device *, void *, void *);
void	uhci_cardbus_attach(struct device *, struct device *, void *);
int	uhci_cardbus_detach(struct device *, int);

struct uhci_cardbus_softc {
	uhci_softc_t		sc;
	cardbus_chipset_tag_t	sc_cc;
	cardbus_function_tag_t	sc_cf;
	cardbus_devfunc_t	sc_ct;
	void 			*sc_ih;		/* interrupt vectoring */
};

struct cfattach uhci_cardbus_ca = {
	sizeof(struct uhci_cardbus_softc), uhci_cardbus_match,
	    uhci_cardbus_attach, uhci_cardbus_detach, uhci_activate
};

#define CARDBUS_INTERFACE_UHCI PCI_INTERFACE_UHCI
#define cardbus_findvendor pci_findvendor
#define cardbus_devinfo pci_devinfo

int
uhci_cardbus_match(struct device *parent, void *match, void *aux)
{
	struct cardbus_attach_args *ca = (struct cardbus_attach_args *)aux;

	if (CARDBUS_CLASS(ca->ca_class) == CARDBUS_CLASS_SERIALBUS &&
	    CARDBUS_SUBCLASS(ca->ca_class) == CARDBUS_SUBCLASS_SERIALBUS_USB &&
	    CARDBUS_INTERFACE(ca->ca_class) == CARDBUS_INTERFACE_UHCI)
		return (1);
 
	return (0);
}

void
uhci_cardbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct uhci_cardbus_softc *sc = (struct uhci_cardbus_softc *)self;
	struct cardbus_attach_args *ca = aux;
	cardbus_devfunc_t ct = ca->ca_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	cardbusreg_t csr;
	usbd_status r;
	const char *vendor;
	const char *devname = sc->sc.sc_bus.bdev.dv_xname;

	/* Map I/O registers */
	if (Cardbus_mapreg_map(ct, PCI_CBIO, CARDBUS_MAPREG_TYPE_IO, 0,
			   &sc->sc.iot, &sc->sc.ioh, NULL, &sc->sc.sc_size)) {
		printf("%s: can't map io space\n", devname);
		return;
	}

	/* Disable interrupts, so we don't get any spurious ones. */
	bus_space_write_2(sc->sc.iot, sc->sc.ioh, UHCI_INTR, 0);

	sc->sc_cc = cc;
	sc->sc_cf = cf;
	sc->sc_ct = ct;
	sc->sc.sc_bus.dmatag = ca->ca_dmat;

	(ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_IO_ENABLE);
	(ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_BM_ENABLE);

	/* Enable the device. */
	csr = cardbus_conf_read(cc, cf, ca->ca_tag,
				CARDBUS_COMMAND_STATUS_REG);
	cardbus_conf_write(cc, cf, ca->ca_tag, CARDBUS_COMMAND_STATUS_REG,
		       csr | CARDBUS_COMMAND_MASTER_ENABLE
			   | CARDBUS_COMMAND_IO_ENABLE);

	sc->sc_ih = cardbus_intr_establish(cc, cf, ca->ca_intrline,
					   IPL_USB, uhci_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n", devname);
		return;
	}
	printf(": irq %d\n", ca->ca_intrline);

	/* Set LEGSUP register to its default value. */
	cardbus_conf_write(cc, cf, ca->ca_tag, PCI_LEGSUP,
			   PCI_LEGSUP_USBPIRQDEN);

	switch(cardbus_conf_read(cc, cf, ca->ca_tag, PCI_USBREV) & PCI_USBREV_MASK) {
	case PCI_USBREV_PRE_1_0:
		sc->sc.sc_bus.usbrev = USBREV_PRE_1_0;
		break;
	case PCI_USBREV_1_0:
		sc->sc.sc_bus.usbrev = USBREV_1_0;
		break;
	case PCI_USBREV_1_1:
		sc->sc.sc_bus.usbrev = USBREV_1_1;
		break;
	default:
		sc->sc.sc_bus.usbrev = USBREV_UNKNOWN;
		break;
	}

	uhci_run(&sc->sc, 0);			/* stop the controller */
						/* disable interrupts */
	bus_space_barrier(sc->sc.iot, sc->sc.ioh, 0, sc->sc.sc_size,
	    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);
	bus_space_write_2(sc->sc.iot, sc->sc.ioh, UHCI_INTR, 0);

	/* Figure out vendor for root hub descriptor. */
	vendor = cardbus_findvendor(ca->ca_id);
	sc->sc.sc_id_vendor = CARDBUS_VENDOR(ca->ca_id);
	if (vendor)
		strlcpy(sc->sc.sc_vendor, vendor, sizeof (sc->sc.sc_vendor));
	else
		snprintf(sc->sc.sc_vendor, sizeof(sc->sc.sc_vendor),
		    "vendor 0x%04x", CARDBUS_VENDOR(ca->ca_id));
	
	r = uhci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: init failed, error=%d\n", devname, r);
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		return;
	}

	/* Attach usb device. */
	sc->sc.sc_child = config_found((void *)sc, &sc->sc.sc_bus,
				       usbctlprint);
}

int
uhci_cardbus_detach(struct device *self, int flags)
{
	struct uhci_cardbus_softc *sc = (struct uhci_cardbus_softc *)self;
	struct cardbus_devfunc *ct = sc->sc_ct;
	int rv;

	rv = uhci_detach(&sc->sc, flags);
	if (rv)
		return (rv);

	if (sc->sc_ih != NULL) {
		cardbus_intr_disestablish(sc->sc_cc, sc->sc_cf, sc->sc_ih);
		sc->sc_ih = NULL;
	}

	if (sc->sc.sc_size) {
		Cardbus_mapreg_unmap(ct, PCI_CBIO, sc->sc.iot,
		    sc->sc.ioh, sc->sc.sc_size);
		sc->sc.sc_size = 0;
	}

	return (0);
}
