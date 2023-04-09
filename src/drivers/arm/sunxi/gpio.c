/*
This file is part of vermillion.

Vermillion is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published
by the Free Software Foundation, version 3.

Vermillion is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <_types.h>
#include <vermillion/drivers.h>

#define PN_CFG(c, n, i) *(volatile u32*)(c + (n * 0x24) + (0x4 * i))
#define PN_DAT(c, n)    *(volatile u32*)(c + (n * 0x24) + 0x10)
#define PN_DRV(c, n, i) *(volatile u32*)(c + (n * 0x24) + 0x14 + (0x4 * i))
#define PN_PUL(c, n, i) *(volatile u32*)(c + (n * 0x24) + 0x1C + (0x4 * i))

#define EINT_CFG(c, n, i) *(volatile u32*)(c + 0x200 + (n * 0x20) + (0x4 * i))
#define EINT_CTL(c, n)    *(volatile u32*)(c + 0x200 + (n * 0x20) + 0x10)
#define EINT_STA(c, n)    *(volatile u32*)(c + 0x200 + (n * 0x20) + 0x14)
#define EINT_DEB(c, n)    *(volatile u32*)(c + 0x200 + (n * 0x20) + 0x18)

#define PIO 0x01C20800
#define R_PIO 0x01F02C00
#define PORTX(c, n) (c + (n * 0x24) + 0x10)
#define PORTA PORTX(PIO, 0)
#define PORTC PORTX(PIO, 1)
#define PORTD PORTX(PIO, 2)
#define PORTE PORTX(PIO, 3)
#define PORTF PORTX(PIO, 4)
#define PORTG PORTX(PIO, 5)
#define PORTL PORTX(R_PIO, 0)

enum pin
{
    PA0 = 0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11,
    PA12, PA13, PA14, PA15, PA16, PA17, PA18, PA19, PA20, PA21,
    PC0 = 32, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10,
    PC11, PC12, PC13, PC14, PC15, PC16, PC17, PC18,
    PD0 = 64, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10,
    PD11, PD12, PD13, PD14, PD15, PD16, PD17,
    PE0 = 96, PE1, PE2, PE3, PE4, PE5, PE6, PE7, PE8, PE9, PE10,
    PE11, PE12, PE13, PE14, PE15,
    PF0 = 128, PF1, PF2, PF3, PF4, PF5, PF6,
    PG0 = 160, PG1, PG2, PG3, PG4, PG5, PG6, PG7, PG8, PG9, PG10,
    PG11, PG12, PG13,
    PL0 = 192, PL1, PL2, PL3, PL4, PL5, PL6, PL7, PL8, PL9, PL10,
    PL11
};

enum pin_config
{
    PIN_CFG_IN = 0,
    PIN_CFG_OUT,
    PIN_CFG_EX0,
    PIN_CFG_EX1,
    PIN_CFG_EX2,
    PIN_CFG_EX3,
    PIN_CFG_EX4,
    PIN_CFG_OFF
};

static inline void
pin_config(enum pin p, enum pin_config c)
{
    u32 base = PIO;
    if (p >= PL0)
    {
        base = R_PIO;
        p -= PL0;
    }

    u8 port = p / 32;
    u8 slot = p % 32;
    u8 reg = slot / 8;
    u8 pos = slot % 8;

    u32 data = c << (pos * 4);
    u32 mask = 0xf << (pos * 4);

    PN_CFG(base, port, reg) &= ~mask;
    PN_CFG(base, port, reg) |= (data & mask);
}

static inline bool
pin_read(enum pin p)
{
    u32 base = PIO;
    if (p >= PL0)
    {
        base = R_PIO;
        p -= PL0;
    }

    u8 port = p / 32;
    u8 slot = p % 32;
    return (PN_DAT(base, port) >> slot) & 0x1;
}

static inline void
pin_write(enum pin p, bool bit)
{
    u32 base = PIO;
    if (p >= PL0)
    {
        base = R_PIO;
        p -= PL0;
    }

    u8 port = p / 32;
    u8 slot = p % 32;

    if (bit)
        PN_DAT(base, port) |= 0x1 << slot;
    else
        PN_DAT(base, port) &= ~(0x1 << slot);
}

enum pin_level
{
    PIN_LEVEL0 = 0,
    PIN_LEVEL1,
    PIN_LEVEL2,
    PIN_LEVEL3
};

static inline void
pin_level(enum pin p, enum pin_level l)
{
    u32 base = PIO;
    if (p >= PL0)
    {
        base = R_PIO;
        p -= PL0;
    }

    u8 port = p / 32;
    u8 slot = p % 32;
    u8 reg = slot / 16;
    u8 pos = slot % 16;

    u32 data = l << (pos * 2);
    u32 mask = 0x3 << (pos * 2);

    PN_DRV(base, port, reg) &= ~mask;
    PN_DRV(base, port, reg) |= (data & mask);
}

enum pin_pull
{
    PIN_PULLOFF = 0,
    PIN_PULLUP,
    PIN_PULLDOWN
};

static inline void
pin_pull(enum pin p, enum pin_pull pl)
{
    u32 base = PIO;
    if (p >= PL0)
    {
        base = R_PIO;
        p -= PL0;
    }

    u8 port = p / 32;
    u8 slot = p % 32;
    u8 reg = slot / 16;
    u8 pos = slot % 16;

    u32 data = pl << (pos * 2);
    u32 mask = 0x3 << (pos * 2);

    PN_PUL(base, port, reg) &= ~mask;
    PN_PUL(base, port, reg) |= (data & mask);
}

#define PA_INT EINT_CTL(PORT_CTRL0, 0)
#define PG_INT EINT_CTL(PORT_CTRL0, 5)

enum eint
{
    PA_INT0 = 0, PA_INT1, PA_INT2, PA_INT3, PA_INT4, PA_INT5, PA_INT6,
    PA_INT7, PA_INT8, PA_INT9, PA_INT10, PA_INT11, PA_INT12, PA_INT13,
    PA_INT14, PA_INT15, PA_INT16, PA_INT17, PA_INT19, PA_INT20, PA_INT21,
    PG_INT0 = 32, PG_INT1, PG_INT2, PG_INT3, PG_INT4, PG_INT5, PG_INT6,
    PG_INT7, PG_INT8, PG_INT9, PG_INT10, PG_INT11, PG_INT12, PG_INT13
};

enum eint_config
{
    EINT_EDGE_H,
    EINT_EDGE_L,
    EINT_LEVEL_H,
    EINT_LEVEL_L,
    EINT_DOUBLE
};

static inline void
eint_config(enum eint i, enum eint_config c)
{
    u32 base = PIO;
    u8 port = i / 32;
    u8 slot = i % 32;
    u8 reg = slot / 8;
    u8 pos = slot % 8;

    u32 data = c << (pos * 4);
    u32 mask = 0xf << (pos * 4);

    EINT_CFG(base, port, reg) &= ~mask;
    EINT_CFG(base, port, reg) |= (data & mask);
}

static inline void
eint_control(enum eint i, bool status)
{
    u32 base = PIO;
    u8 port = i / 32;
    u8 slot = i % 32;

    if (status)
        EINT_CTL(base, port) |= 0x1 << slot;
    else
        EINT_CTL(base, port) &= ~(0x1 << slot);
}

static inline void
eint_ack(enum eint i)
{
    u32 base = PIO;
    u8 port = i / 32;
    u8 slot = i % 32;
    EINT_STA(base, port) |= 0x1 << slot;
}

static inline void
eint_debounce(enum eint i, u8 prescale, bool osc24m)
{
    u32 base = PIO;
    u8 port = i / 32;
    EINT_DEB(base, port) = (prescale & 0x7) << 4 | osc24m;
}

/* Exported functions */

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile u32*)(R_PRCM + 0x28)
static bool
init(void)
{
    APB0_GATE = 1;
    return true;
}

static void
clean(void)
{
    return;
}

static u32 gpio_ports[] = {PORTA, PORTC, PORTD, PORTE, PORTF, PORTG, PORTL};

static void
gpio_count(u8 *ports, u16 *pins, u16 *intrs)
{
    if (ports)
        *ports = sizeof(gpio_ports) / sizeof(u32);
    if (pins)
        *pins = PL11 + 1;
    if (intrs)
        *intrs = PG_INT13 + 1;
}

static void
gpio_write(u8 port, u32 data)
{
    if (port < (sizeof(gpio_ports) / sizeof(u32)))
        *((volatile u32 *)gpio_ports[port]) = data;
}

static u32
gpio_read(u8 port)
{
    u32 ret = 0;

    if (port < (sizeof(gpio_ports) / sizeof(u32)))
        ret = *((volatile u32 *)gpio_ports[port]);

    return ret;
}

static bool
gpio_cfgpin(u16 pin, u8 role, u8 pull)
{
    bool ret = (pin <= PL11);

    enum pin_config c = PIN_CFG_OFF;
    enum pin_pull pl = PIN_PULLOFF;
    if (ret)
    {
        switch (role)
        {
            case DRIVER_GPIO_OFF:
                break;
            case DRIVER_GPIO_IN:
                c = PIN_CFG_IN;
                break;
            case DRIVER_GPIO_OUT:
                c = PIN_CFG_OUT;
                break;
            default:
                c = role - DRIVER_GPIO_EXTRA;
                break;
        }

        switch (pull)
        {
            case DRIVER_GPIO_PULLOFF:
                break;
            case DRIVER_GPIO_PULLUP:
                pl = PIN_PULLUP;
                break;
            case DRIVER_GPIO_PULLDOWN:
                pl = PIN_PULLDOWN;
                break;
            default:
                ret = false;
                break;
        }
    }

    if (ret)
    {
        pin_config(pin, c);
        pin_pull(pin, pl);
    }

    return ret;
}

static void
gpio_set(u16 pin, bool value)
{
    if (pin <= PL11)
        pin_write(pin, value);
}

static bool
gpio_get(u16 pin)
{
    bool ret = false;

    if (pin <= PL11)
        ret = pin_read(pin);

    return ret;
}

static bool
gpio_cfgint(u16 intr, bool enable, u8 level)
{
    bool ret = (intr <= PG_INT13);

    enum eint_config c = EINT_EDGE_H;
    if (ret)
    {
        switch (level)
        {
            case DRIVER_GPIO_EDGE_H:
                break;
            case DRIVER_GPIO_EDGE_L:
                c = EINT_EDGE_L;
                break;
            case DRIVER_GPIO_LEVEL_H:
                c = EINT_LEVEL_H;
                break;
            case DRIVER_GPIO_LEVEL_L:
                c = EINT_LEVEL_L;
                break;
            case DRIVER_GPIO_DOUBLE:
                c = EINT_DOUBLE;
                break;
            default:
                ret = false;
                break;
        }
    }

    if (ret)
    {
        eint_control(intr, enable);
        eint_config(intr, c);
    }

    return ret;
}

static bool
gpio_ack(u16 intr)
{
    eint_ack(intr);
    return true;
}

static const struct driver sunxi_gpio =
{
    .name = "sunxi-gpio",
    .init = init, .clean = clean,
    .api = DRIVER_API_GENERIC,
    .type = DRIVER_TYPE_GPIO,
    .routines.gpio.count  = gpio_count,
    .routines.gpio.write  = gpio_write,
    .routines.gpio.read   = gpio_read,
    .routines.gpio.cfgpin = gpio_cfgpin,
    .routines.gpio.set    = gpio_set,
    .routines.gpio.get    = gpio_get,
    .routines.gpio.cfgint = gpio_cfgint,
    .routines.gpio.ack    = gpio_ack
};
driver_register(sunxi_gpio);
