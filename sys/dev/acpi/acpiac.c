/* $OpenBSD: acpiac.c,v 1.3 2006/02/16 22:14:32 marco Exp $ */
/*
 * Copyright (c) 2005 Marco Peereboom <marco@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpidev.h>
#include <dev/acpi/amltypes.h>
#include <dev/acpi/dsdt.h>

int acpiac_match(struct device *, void *, void *);
void acpiac_attach(struct device *, struct device *, void *);

struct acpiac_softc {
	struct device		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	struct acpi_softc	*sc_acpi;
	struct aml_node		*sc_devnode;

	int			sc_ac_stat;
};

int acpiac_getsta(struct acpiac_softc *);

struct cfattach acpiac_ca = {
	sizeof(struct acpiac_softc), acpiac_match, acpiac_attach
};

struct cfdriver acpiac_cd = {
	NULL, "acpiac", DV_DULL
};

int
acpiac_match(struct device *parent, void *match, void *aux)
{
	struct acpi_attach_args *aa = aux;
	struct cfdata *cf = match;

	/* sanity */
	if (aa->aaa_name == NULL ||
	    strcmp(aa->aaa_name, cf->cf_driver->cd_name) != 0 ||
	    aa->aaa_table != NULL)
		return (0);

	return (1);
}

void
acpiac_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpiac_softc *sc = (struct acpiac_softc *)self;
	struct acpi_attach_args *aa = aux;

	sc->sc_acpi = (struct acpi_softc *)parent;
	sc->sc_devnode = aa->aaa_node->child;

	acpiac_getsta(sc); 

	printf(": ");

	if (sc->sc_ac_stat == PSR_ONLINE)
		printf("AC unit online");
	else if (sc->sc_ac_stat == PSR_OFFLINE)
		printf("AC unit offline");
	else
		printf("AC unit in unknown state");

	printf("\n");
}

int
acpiac_getsta(struct acpiac_softc *sc)
{
	struct aml_value res, env;
	struct acpi_context *ctx;

	memset(&res, 0, sizeof(res));
	memset(&env, 0, sizeof(env));

	ctx = NULL;
	if (aml_eval_name(sc->sc_acpi, sc->sc_devnode, "_STA", &res, &env)) {
		dnprintf(10, "%s: no _STA\n",
		    DEVNAME(sc));
		return (1);
	}

	dnprintf(40, "_STA value: %x\n", res.v_integer);

	if (aml_eval_name(sc->sc_acpi, sc->sc_devnode, "_PSR", &res, &env)) {
		dnprintf(10, "%s: no _PSR\n",
		    DEVNAME(sc));
		return (1);
	}

	sc->sc_ac_stat = aml_val2int(NULL, &res);

	return (0);
}
