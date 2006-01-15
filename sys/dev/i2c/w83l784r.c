/*	$OpenBSD: w83l784r.c,v 1.1 2006/01/15 12:20:14 kettenis Exp $	*/

/*
 * Copyright (c) 2006 Mark Kettenis
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
#include <sys/sensors.h>

#include <dev/i2c/i2cvar.h>

/* W83L784R registers */
#define W83L784R_VCORE		0x20
#define W83L784R_VBAT		0x21
#define W83L784R_3_3V		0x22
#define W83L784R_VCC		0x23
#define W83L784R_TEMP1		0x27
#define W83L784R_FAN1		0x28
#define W83L784R_FAN2		0x29
#define W83L784R_CONFIG		0x40
#define W83L784R_FANDIV		0x49
#define W83L784R_T23ADDR	0x4b
#define W83L784R_CHIPID		0x4e

#define W83L784R_TEMP23		0x00

/* W83L785R registers */
#define W83L785R_1_5V		0x21
#define W83L785R_2_5V		0x22
#define W83L785R_3_3V		0x23
#define W83L785R_TEMP2		0x26
#define W83L785R_FANDIV		0x47

/* Chip IDs */
#define WBENV_CHIPID_W83L784R		0x50
#define WBENV_CHIPID_W83L785R		0x60
#define WBENV_CHIPID_W83L785TS_L	0x70

#define WBENV_MAX_SENSORS  9

struct wbenv_softc;

struct wbenv_sensor {
	char *desc;
	enum sensor_type type;
	int reg;
	void (*refresh)(struct wbenv_softc *, int);
};

struct wbenv_softc {
	struct device sc_dev;

	i2c_tag_t sc_tag;
	i2c_addr_t sc_addr[3];

	struct sensor sc_sensors[WBENV_MAX_SENSORS];
	struct wbenv_sensor *sc_wbenv_sensors;
	int sc_numsensors;
};

int	wbenv_match(struct device *, void *, void *);
void	wbenv_attach(struct device *, struct device *, void *);

void	wbenv_setup_sensors(struct wbenv_softc *, struct wbenv_sensor *);
void	wbenv_refresh(void *);

void	wbenv_refresh_volts(struct wbenv_softc *, int);
void	wbenv_refresh_temp(struct wbenv_softc *, int);
void	w83l784r_refresh_temp(struct wbenv_softc *, int);
void	w83l784r_refresh_fanrpm(struct wbenv_softc *, int);
void	w83l785r_refresh_fanrpm(struct wbenv_softc *, int);

u_int8_t wbenv_readreg(struct wbenv_softc *, u_int8_t);
void	wbenv_writereg(struct wbenv_softc *, u_int8_t, u_int8_t);

struct cfattach wbenv_ca = {
	sizeof(struct wbenv_softc), wbenv_match, wbenv_attach
};

struct cfdriver wbenv_cd = {
	NULL, "wbenv", DV_DULL
};

struct wbenv_sensor w83l784r_sensors[] =
{
	{ "VCore", SENSOR_VOLTS_DC, W83L784R_VCORE, wbenv_refresh_volts },
	{ "VBAT", SENSOR_VOLTS_DC, W83L784R_VBAT, wbenv_refresh_volts },
	{ "+3.3V", SENSOR_VOLTS_DC, W83L784R_3_3V, wbenv_refresh_volts },
	{ "+5V", SENSOR_VOLTS_DC, W83L784R_VCC, wbenv_refresh_volts },
	{ "Temp1", SENSOR_TEMP, W83L784R_TEMP1, wbenv_refresh_temp },
	{ "Temp2", SENSOR_TEMP, 1, w83l784r_refresh_temp },
	{ "Temp3", SENSOR_TEMP, 2, w83l784r_refresh_temp },
	{ "Fan1", SENSOR_FANRPM, W83L784R_FAN1, w83l784r_refresh_fanrpm },
	{ "Fam2", SENSOR_FANRPM, W83L784R_FAN2, w83l784r_refresh_fanrpm },

	{ NULL }
};

struct wbenv_sensor w83l785r_sensors[] =
{
	{ "VCore", SENSOR_VOLTS_DC, W83L784R_VCORE, wbenv_refresh_volts },
	{ "+1.5V", SENSOR_VOLTS_DC, W83L785R_1_5V, wbenv_refresh_volts },
	{ "+2.5V", SENSOR_VOLTS_DC, W83L785R_2_5V, wbenv_refresh_volts },
	{ "+3.3V", SENSOR_VOLTS_DC, W83L785R_3_3V, wbenv_refresh_volts },
	{ "Temp1", SENSOR_TEMP, W83L784R_TEMP1, wbenv_refresh_temp },
	{ "Temp2", SENSOR_TEMP, W83L785R_TEMP2, wbenv_refresh_temp },
	{ "Fan1", SENSOR_FANRPM, W83L784R_FAN1, w83l785r_refresh_fanrpm },
	{ "Fam2", SENSOR_FANRPM, W83L784R_FAN1, w83l785r_refresh_fanrpm },

	{ NULL }
};

struct wbenv_sensor w83l785ts_l_sensors[] =
{
	{ "Temp", SENSOR_TEMP, W83L784R_TEMP1, wbenv_refresh_temp },

	{ NULL }
};

int
wbenv_match(struct device *parent, void *match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (strcmp(ia->ia_name, "w83l784r") == 0 ||
	    strcmp(ia->ia_name, "w83l785r") == 0 ||
	    strcmp(ia->ia_name, "w83l785ts-l") == 0)
		return (1);
	return (0);
}

void
wbenv_attach(struct device *parent, struct device *self, void *aux)
{
	struct wbenv_softc *sc = (struct wbenv_softc *)self;
	struct i2c_attach_args *ia = aux;
	u_int8_t cmd, data, config;
	int i;

	sc->sc_tag = ia->ia_tag;
	sc->sc_addr[0] = ia->ia_addr;

	iic_acquire_bus(sc->sc_tag, 0);

	cmd = W83L784R_CHIPID;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr[0], &cmd, sizeof cmd, &data, sizeof data, 0)) {
		iic_release_bus(sc->sc_tag, 0);
		printf(": cannot read chip ID register\n");
		return;
	}

	iic_release_bus(sc->sc_tag, 0);

	switch (data) {
	case WBENV_CHIPID_W83L784R:
		printf(": W83L784R\n");
		wbenv_setup_sensors(sc, w83l784r_sensors);
		break;
	case WBENV_CHIPID_W83L785R:
		printf(": W83L784R\n");
		wbenv_setup_sensors(sc, w83l785r_sensors);
		goto start;;
	case WBENV_CHIPID_W83L785TS_L:
		printf(": W83L784TS-L\n");
		wbenv_setup_sensors(sc, w83l785ts_l_sensors);
		goto start;;
	default:
		printf(": unknownd Winbond chip (ID 0x%x)\n", data);
		return;
	}

	iic_acquire_bus(sc->sc_tag, 0);

	cmd = W83L784R_T23ADDR;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr[0], &cmd, sizeof cmd, &data, sizeof data, 0)) {
		iic_release_bus(sc->sc_tag, 0);
		printf(": cannot read address register\n");
		return;
	}

	iic_release_bus(sc->sc_tag, 0);

	sc->sc_addr[1] = 0x48 + (data & 0x7);
	sc->sc_addr[2] = 0x48 + ((data >> 4) & 0x7);

	/* Make the bus scan ignore the satellites. */
	iic_ignore_addr(sc->sc_addr[1]);
	iic_ignore_addr(sc->sc_addr[2]);

 start:
	if (sensor_task_register(sc, wbenv_refresh, 5)) {
		printf("%s: unable to register update task\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* Start the monitoring loop */
	config = wbenv_readreg(sc, W83L784R_CONFIG);
	wbenv_writereg(sc, W83L784R_CONFIG, config | 0x01);

	/* Add sensors */
	for (i = 0; i < sc->sc_numsensors; ++i)
		SENSOR_ADD(&sc->sc_sensors[i]);
}

void
wbenv_setup_sensors(struct wbenv_softc *sc, struct wbenv_sensor *sensors)
{
	int i;

	for (i = 0; sensors[i].desc; i++) {
		strlcpy(sc->sc_sensors[i].device, sc->sc_dev.dv_xname,
		    sizeof(sc->sc_sensors[i].device));
		sc->sc_sensors[i].type = sensors[i].type;
		strlcpy(sc->sc_sensors[i].desc, sensors[i].desc,
		     sizeof(sc->sc_sensors[i].desc));
		sc->sc_numsensors++;
	}
	sc->sc_wbenv_sensors = sensors;
}

void
wbenv_refresh(void *arg)
{
	struct wbenv_softc *sc = arg;
	int i;

	iic_acquire_bus(sc->sc_tag, 0);

	for (i = 0; i < sc->sc_numsensors; i++)
		sc->sc_wbenv_sensors[i].refresh(sc, i);

	iic_release_bus(sc->sc_tag, 0);
}

void
wbenv_refresh_volts(struct wbenv_softc *sc, int n)
{
	struct sensor *sensor = &sc->sc_sensors[n];
	int data;

	data = wbenv_readreg(sc, sc->sc_wbenv_sensors[n].reg);
	sensor->value = (data << 4);
	sensor->value *= 1000;
}

void
wbenv_refresh_temp(struct wbenv_softc *sc, int n)
{
	struct sensor *sensor = &sc->sc_sensors[n];
	int sdata;

	sdata = wbenv_readreg(sc, sc->sc_wbenv_sensors[n].reg);
	if (sdata & 0x80)
		sdata -= 0x100;
	sensor->value = sdata * 1000000 + 273150000;
}

void
w83l784r_refresh_temp(struct wbenv_softc *sc, int n)
{
	struct sensor *sensor = &sc->sc_sensors[n];
	int16_t sdata;
	u_int8_t cmd = 0;

	iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr[sc->sc_wbenv_sensors[n].reg],
	    &cmd, sizeof cmd, &sdata, sizeof sdata, 0);
	sensor->value = (sdata >> 7) * 500000 + 273150000;
}

void
w83l784r_refresh_fanrpm(struct wbenv_softc *sc, int n)
{
	struct sensor *sensor = &sc->sc_sensors[n];
	int data, divisor;

	data = wbenv_readreg(sc, W83L784R_FANDIV);
	if (sc->sc_wbenv_sensors[n].reg == W83L784R_FAN1)
		divisor = data & 0x07;
	else
		divisor = (data >> 4) & 0x07;

	data = wbenv_readreg(sc, sc->sc_wbenv_sensors[n].reg);
	if (data == 0xff || data == 0x00) {
		sensor->flags |= SENSOR_FINVALID;
		sensor->value = 0;
	} else {
		sensor->flags &= ~SENSOR_FINVALID;
		sensor->value = 1350000 / (data << divisor);
	}
}

void
w83l785r_refresh_fanrpm(struct wbenv_softc *sc, int n)
{
	struct sensor *sensor = &sc->sc_sensors[n];
	int data, divisor;

	data = wbenv_readreg(sc, W83L785R_FANDIV);
	if (sc->sc_wbenv_sensors[n].reg == W83L784R_FAN1)
		divisor = data & 0x07;
	else
		divisor = (data >> 4) & 0x07;

	data = wbenv_readreg(sc, sc->sc_wbenv_sensors[n].reg);
	if (data == 0xff || data == 0x00) {
		sensor->flags |= SENSOR_FINVALID;
		sensor->value = 0;
	} else {
		sensor->flags &= ~SENSOR_FINVALID;
		sensor->value = 1350000 / (data << divisor);
	}
}

u_int8_t
wbenv_readreg(struct wbenv_softc *sc, u_int8_t reg)
{
	u_int8_t data;

	iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr[0], &reg, sizeof reg, &data, sizeof data, 0);

	return data;
}

void
wbenv_writereg(struct wbenv_softc *sc, u_int8_t reg, u_int8_t data)
{
	iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_addr[0], &reg, sizeof reg, &data, sizeof data, 0);
}
