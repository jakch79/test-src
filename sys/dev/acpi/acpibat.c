/* $OpenBSD: acpibat.c,v 1.54 2009/03/11 21:54:15 jordan Exp $ */
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
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/sensors.h>

#include <machine/bus.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpidev.h>
#include <dev/acpi/amltypes.h>
#include <dev/acpi/dsdt.h>

int	acpibat_match(struct device *, void *, void *);
void	acpibat_attach(struct device *, struct device *, void *);

struct cfattach acpibat_ca = {
	sizeof(struct acpibat_softc), acpibat_match, acpibat_attach
};

struct cfdriver acpibat_cd = {
	NULL, "acpibat", DV_DULL
};

const char *acpibat_hids[] = { ACPI_DEV_CMB, 0 };

void	acpibat_monitor(struct acpibat_softc *);
void	acpibat_refresh(void *);
int	acpibat_getbif(struct acpibat_softc *);
int	acpibat_getbst(struct acpibat_softc *);
int	acpibat_notify(struct aml_node *, int, void *);

int
acpibat_match(struct device *parent, void *match, void *aux)
{
	struct acpi_attach_args	*aa = aux;
	struct cfdata		*cf = match;

	/* sanity */
	return (acpi_matchhids(aa, acpibat_hids, cf->cf_driver->cd_name));
}

void
acpibat_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpibat_softc	*sc = (struct acpibat_softc *)self;
	struct acpi_attach_args	*aa = aux;
	int64_t			sta;

	sc->sc_acpi = (struct acpi_softc *)parent;
	sc->sc_devnode = aa->aaa_node;

	if (aml_evalinteger(sc->sc_acpi, sc->sc_devnode, "_STA", 0, NULL, &sta)) {
		dnprintf(10, "%s: no _STA\n", DEVNAME(sc));
		return;
	}

	if ((sta & STA_BATTERY) != 0) {
		sc->sc_bat_present = 1;
		acpibat_getbif(sc);
		acpibat_getbst(sc);

		printf(": %s", sc->sc_devnode->name);
		if (sc->sc_bif.bif_model[0])
			printf(" model \"%s\"", sc->sc_bif.bif_model);
		if (sc->sc_bif.bif_serial[0])
			printf(" serial %s", sc->sc_bif.bif_serial);
		if (sc->sc_bif.bif_type[0])
			printf(" type %s", sc->sc_bif.bif_type);
		if (sc->sc_bif.bif_oem[0])
			printf(" oem \"%s\"", sc->sc_bif.bif_oem);
		printf("\n");
	} else {
		sc->sc_bat_present = 0;
		printf(": %s not present\n", sc->sc_devnode->name);
	}

	/* create sensors */
	acpibat_monitor(sc);

	/* populate sensors */
	acpibat_refresh(sc);

	aml_register_notify(sc->sc_devnode, aa->aaa_dev,
	    acpibat_notify, sc, ACPIDEV_POLL);
}

void
acpibat_monitor(struct acpibat_softc *sc)
{
	int			type;

	/* assume _BIF and _BST have been called */
	strlcpy(sc->sc_sensdev.xname, DEVNAME(sc),
	    sizeof(sc->sc_sensdev.xname));

	type = sc->sc_bif.bif_power_unit ? SENSOR_AMPHOUR : SENSOR_WATTHOUR;

	strlcpy(sc->sc_sens[0].desc, "last full capacity",
	    sizeof(sc->sc_sens[0].desc));
	sc->sc_sens[0].type = type;
	sensor_attach(&sc->sc_sensdev, &sc->sc_sens[0]);
	sc->sc_sens[0].value = sc->sc_bif.bif_last_capacity * 1000;

	strlcpy(sc->sc_sens[1].desc, "warning capacity",
	    sizeof(sc->sc_sens[1].desc));
	sc->sc_sens[1].type = type;
	sensor_attach(&sc->sc_sensdev, &sc->sc_sens[1]);
	sc->sc_sens[1].value = sc->sc_bif.bif_warning * 1000;

	strlcpy(sc->sc_sens[2].desc, "low capacity",
	    sizeof(sc->sc_sens[2].desc));
	sc->sc_sens[2].type = type;
	sensor_attach(&sc->sc_sensdev, &sc->sc_sens[2]);
	sc->sc_sens[2].value = sc->sc_bif.bif_low * 1000;

	strlcpy(sc->sc_sens[3].desc, "voltage", sizeof(sc->sc_sens[3].desc));
	sc->sc_sens[3].type = SENSOR_VOLTS_DC;
	sensor_attach(&sc->sc_sensdev, &sc->sc_sens[3]);
	sc->sc_sens[3].value = sc->sc_bif.bif_voltage * 1000;

	strlcpy(sc->sc_sens[4].desc, "battery unknown",
	    sizeof(sc->sc_sens[4].desc));
	sc->sc_sens[4].type = SENSOR_INTEGER;
	sensor_attach(&sc->sc_sensdev, &sc->sc_sens[4]);
	sc->sc_sens[4].value = sc->sc_bst.bst_state;

	strlcpy(sc->sc_sens[5].desc, "rate", sizeof(sc->sc_sens[5].desc));
	sc->sc_sens[5].type = SENSOR_INTEGER;
	sensor_attach(&sc->sc_sensdev, &sc->sc_sens[5]);
	sc->sc_sens[5].value = sc->sc_bst.bst_rate;

	strlcpy(sc->sc_sens[6].desc, "remaining capacity",
	    sizeof(sc->sc_sens[6].desc));
	sc->sc_sens[6].type = type;
	sensor_attach(&sc->sc_sensdev, &sc->sc_sens[6]);
	sc->sc_sens[6].value = sc->sc_bst.bst_capacity * 1000;

	strlcpy(sc->sc_sens[7].desc, "current voltage",
	    sizeof(sc->sc_sens[7].desc));
	sc->sc_sens[7].type = SENSOR_VOLTS_DC;
	sensor_attach(&sc->sc_sensdev, &sc->sc_sens[7]);
	sc->sc_sens[7].value = sc->sc_bst.bst_voltage * 1000;

	sensordev_install(&sc->sc_sensdev);
}

void
acpibat_refresh(void *arg)
{
	struct acpibat_softc	*sc = arg;
	int			i;

	dnprintf(30, "%s: %s: refresh\n", DEVNAME(sc),
	    sc->sc_devnode->name);

	if (!sc->sc_bat_present) {
		for (i = 0; i < 8; i++) {
			sc->sc_sens[i].value = 0;
			sc->sc_sens[i].status = SENSOR_S_UNSPEC;
			sc->sc_sens[i].flags = SENSOR_FINVALID;
		}
		/* override state */
		strlcpy(sc->sc_sens[4].desc, "battery removed",
		    sizeof(sc->sc_sens[4].desc));
		return;
	}

	/*
	 * XXX don't really need _BIF but keep it here in case we
	 * miss an insertion/removal event
	 */
	acpibat_getbif(sc);
	acpibat_getbst(sc);

	/* _BIF values are static, sensor 0..3 */
	if (sc->sc_bif.bif_last_capacity == BIF_UNKNOWN) {
		sc->sc_sens[0].value = 0;
		sc->sc_sens[0].status = SENSOR_S_UNKNOWN;
		sc->sc_sens[0].flags = SENSOR_FUNKNOWN;
	} else {
		sc->sc_sens[0].value = sc->sc_bif.bif_last_capacity * 1000;
		sc->sc_sens[0].status = SENSOR_S_UNSPEC;
		sc->sc_sens[0].flags = 0;
	}
	sc->sc_sens[1].value = sc->sc_bif.bif_warning * 1000;
	sc->sc_sens[1].flags = 0;
	sc->sc_sens[2].value = sc->sc_bif.bif_low * 1000;
	sc->sc_sens[2].flags = 0;
	if (sc->sc_bif.bif_voltage == BIF_UNKNOWN) {
		sc->sc_sens[3].value = 0;
		sc->sc_sens[3].status = SENSOR_S_UNKNOWN;
		sc->sc_sens[3].flags = SENSOR_FUNKNOWN;
	} else {
		sc->sc_sens[3].value = sc->sc_bif.bif_voltage * 1000;
		sc->sc_sens[3].status = SENSOR_S_UNSPEC;
		sc->sc_sens[3].flags = 0;
	}

	/* _BST values are dynamic, sensor 4..7 */
	sc->sc_sens[4].status = SENSOR_S_OK;
	sc->sc_sens[4].flags = 0;
	if (sc->sc_bif.bif_last_capacity == BIF_UNKNOWN ||
	    sc->sc_bst.bst_capacity == BST_UNKNOWN) {
		sc->sc_sens[4].status = SENSOR_S_UNKNOWN;
		sc->sc_sens[4].flags = SENSOR_FUNKNOWN;
		strlcpy(sc->sc_sens[4].desc, "battery unknown",
		    sizeof(sc->sc_sens[4].desc));
	} else if (sc->sc_bst.bst_capacity >= sc->sc_bif.bif_last_capacity)
		strlcpy(sc->sc_sens[4].desc, "battery full",
		    sizeof(sc->sc_sens[4].desc));
	else if (sc->sc_bst.bst_state & BST_DISCHARGE)
		strlcpy(sc->sc_sens[4].desc, "battery discharging",
		    sizeof(sc->sc_sens[4].desc));
	else if (sc->sc_bst.bst_state & BST_CHARGE)
		strlcpy(sc->sc_sens[4].desc, "battery charging",
		    sizeof(sc->sc_sens[4].desc));
	else if (sc->sc_bst.bst_state & BST_CRITICAL) {
		strlcpy(sc->sc_sens[4].desc, "battery critical",
		    sizeof(sc->sc_sens[4].desc));
		sc->sc_sens[4].status = SENSOR_S_CRIT;
	} else
		strlcpy(sc->sc_sens[4].desc, "battery idle",
		    sizeof(sc->sc_sens[4].desc));
	sc->sc_sens[4].value = sc->sc_bst.bst_state;

	if (sc->sc_bst.bst_rate == BST_UNKNOWN) {
		sc->sc_sens[5].value = 0;
		sc->sc_sens[5].status = SENSOR_S_UNKNOWN;
		sc->sc_sens[5].flags = SENSOR_FUNKNOWN;
	} else {
		sc->sc_sens[5].value = sc->sc_bst.bst_rate;
		sc->sc_sens[5].status = SENSOR_S_UNSPEC;
		sc->sc_sens[5].flags = 0;
	}

	if (sc->sc_bst.bst_capacity == BST_UNKNOWN) {
		sc->sc_sens[6].value = 0;
		sc->sc_sens[6].status = SENSOR_S_UNKNOWN;
		sc->sc_sens[6].flags = SENSOR_FUNKNOWN;
	} else {
		sc->sc_sens[6].value = sc->sc_bst.bst_capacity * 1000;
		sc->sc_sens[6].flags = 0;

		if (sc->sc_bst.bst_capacity < sc->sc_bif.bif_low)
			/* XXX we should shutdown the system */
			sc->sc_sens[6].status = SENSOR_S_CRIT;
		else if (sc->sc_bst.bst_capacity < sc->sc_bif.bif_warning)
			sc->sc_sens[6].status = SENSOR_S_WARN;
		else
			sc->sc_sens[6].status = SENSOR_S_OK;
	}

	if (sc->sc_bst.bst_voltage == BST_UNKNOWN) {
		sc->sc_sens[7].value = 0;
		sc->sc_sens[7].status = SENSOR_S_UNKNOWN;
		sc->sc_sens[7].flags = SENSOR_FUNKNOWN;
	} else {
		sc->sc_sens[7].value = sc->sc_bst.bst_voltage * 1000;
		sc->sc_sens[7].status = SENSOR_S_UNSPEC;
		sc->sc_sens[7].flags = 0;
	}
}

int
acpibat_getbif(struct acpibat_softc *sc)
{
	struct aml_value	res;
	int			rv = EINVAL;

	if (!sc->sc_bat_present) {
		memset(&sc->sc_bif, 0, sizeof(sc->sc_bif));
		return (0);
	}

	if (aml_evalname(sc->sc_acpi, sc->sc_devnode, "_BIF", 0, NULL, &res)) {
		dnprintf(10, "%s: no _BIF\n", DEVNAME(sc));
		goto out;
	}

	if (res.length != 13) {
		dnprintf(10, "%s: invalid _BIF, battery info not saved\n",
		    DEVNAME(sc));
		goto out;
	}

	sc->sc_bif.bif_power_unit = aml_val2int(res.v_package[0]);
	sc->sc_bif.bif_capacity = aml_val2int(res.v_package[1]);
	sc->sc_bif.bif_last_capacity = aml_val2int(res.v_package[2]);
	sc->sc_bif.bif_technology = aml_val2int(res.v_package[3]);
	sc->sc_bif.bif_voltage = aml_val2int(res.v_package[4]);
	sc->sc_bif.bif_warning = aml_val2int(res.v_package[5]);
	sc->sc_bif.bif_low = aml_val2int(res.v_package[6]);
	sc->sc_bif.bif_cap_granu1 = aml_val2int(res.v_package[7]);
	sc->sc_bif.bif_cap_granu2 = aml_val2int(res.v_package[8]);

	strlcpy(sc->sc_bif.bif_model, aml_val_to_string(res.v_package[9]),
		sizeof(sc->sc_bif.bif_model));
	strlcpy(sc->sc_bif.bif_serial, aml_val_to_string(res.v_package[10]),
		sizeof(sc->sc_bif.bif_serial));
	strlcpy(sc->sc_bif.bif_type, aml_val_to_string(res.v_package[11]),
		sizeof(sc->sc_bif.bif_type));
	strlcpy(sc->sc_bif.bif_oem, aml_val_to_string(res.v_package[12]),
		sizeof(sc->sc_bif.bif_oem));

	dnprintf(60, "power_unit: %u capacity: %u last_cap: %u tech: %u "
	    "volt: %u warn: %u low: %u gran1: %u gran2: %d model: %s "
	    "serial: %s type: %s oem: %s\n",
	    sc->sc_bif.bif_power_unit,
	    sc->sc_bif.bif_capacity,
	    sc->sc_bif.bif_last_capacity,
	    sc->sc_bif.bif_technology,
	    sc->sc_bif.bif_voltage,
	    sc->sc_bif.bif_warning,
	    sc->sc_bif.bif_low,
	    sc->sc_bif.bif_cap_granu1,
	    sc->sc_bif.bif_cap_granu2,
	    sc->sc_bif.bif_model,
	    sc->sc_bif.bif_serial,
	    sc->sc_bif.bif_type,
	    sc->sc_bif.bif_oem);

	rv = 0;
out:
	aml_freevalue(&res);
	return (rv);
}

int
acpibat_getbst(struct acpibat_softc *sc)
{
	struct aml_value	res;
	int			rv = EINVAL;

	if (!sc->sc_bat_present) {
		memset(&sc->sc_bst, 0, sizeof(sc->sc_bst));
		return (0);
	}

	if (aml_evalname(sc->sc_acpi, sc->sc_devnode, "_BST", 0, NULL, &res)) {
		dnprintf(10, "%s: no _BST\n", DEVNAME(sc));
		goto out;
	}

	if (res.length != 4) {
		dnprintf(10, "%s: invalid _BST, battery status not saved\n",
		    DEVNAME(sc));
		goto out;
	}

	sc->sc_bst.bst_state = aml_val2int(res.v_package[0]);
	sc->sc_bst.bst_rate = aml_val2int(res.v_package[1]);
	sc->sc_bst.bst_capacity = aml_val2int(res.v_package[2]);
	sc->sc_bst.bst_voltage = aml_val2int(res.v_package[3]);

	dnprintf(60, "state: %u rate: %u cap: %u volt: %u ",
	    sc->sc_bst.bst_state,
	    sc->sc_bst.bst_rate,
	    sc->sc_bst.bst_capacity,
	    sc->sc_bst.bst_voltage);

	rv = 0;
out:
	aml_freevalue(&res);
	return (rv);
}

/* XXX it has been observed that some systems do not propagate battery
 * insertion events up to the driver.  What seems to happen is that DSDT
 * does receive an interrupt however the originator bit is not set.
 * This seems to happen when one inserts a 100% full battery.  Removal
 * of the power cord or insertion of a not 100% full battery breaks this
 * behavior and all events will then be sent upwards.  Currently there
 * is no known work-around for it.
 */

int
acpibat_notify(struct aml_node *node, int notify_type, void *arg)
{
	struct acpibat_softc	*sc = arg;
	int64_t			sta;
	int			present;

	dnprintf(10, "acpibat_notify: %.2x %s\n", notify_type,
	    sc->sc_devnode->name);

	/* Check if installed state of battery has changed */
	if (aml_evalinteger(sc->sc_acpi, node, "_STA", 0, NULL, &sta) == 0) {
 		present = sta & STA_BATTERY;
		if (!sc->sc_bat_present && present) {
			printf("%s: %s inserted\n", DEVNAME(sc),
			    sc->sc_devnode->name);
			sc->sc_bat_present = 1;
		}
		else if (sc->sc_bat_present && !present) {
			printf("%s: %s removed\n", DEVNAME(sc),
			    sc->sc_devnode->name);
			sc->sc_bat_present = 0;
		}
	}
	switch (notify_type) {
	case 0x80:	/* _BST changed */
		break;
	case 0x81:	/* _BIF changed */
		break;
	default:
		break;
	}

	acpibat_refresh(sc);

	return (0);
}
