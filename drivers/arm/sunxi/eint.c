/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <arch/gic.h>

#define VERMILLION_INTERNALS
#include <vermillion/hal/eint.h>
#include <vermillion/util/mem.h>
#include <vermillion/util/types.h>

#include <vermillion/util/debug.h>

#define EINT_CFG(c, n, i) \
    *(volatile uint32_t *)(c + (n * 0x20) + (0x4 * i))
#define EINT_CTL(c, n)    \
    *(volatile uint32_t *)(c + (n * 0x20) + 0x10)
#define EINT_STA(c, n)    \
    *(volatile uint32_t *)(c + (n * 0x20) + 0x14)
#define EINT_DEB(c, n)    \
    *(volatile uint32_t *)(c + (n * 0x20) + 0x18)

/* Driver definition */

struct eint
{
    uint32_t base;
    uint8_t int_ports;

    uint16_t irqs[2];
    void (*handlers[64])(void *), *args[64];
};

static struct eint eints[2] = {0};

static void
handler(void *arg)
{
    struct eint *eint = arg;

    for (uint8_t i = 0; i < eint->int_ports; i++)
    {
        for (uint8_t j = 0; j < 32; j++)
        {
            if (EINT_STA(eint->base, i) & (1 << j))
            {
                EINT_STA(eint->base, i) = (1 << j);

                uint16_t idx = (i * 32) + j;
                if (eint->handlers[idx])
                    eint->handlers[idx](eint->args[idx]);
            }
        }
    }
}

static bool
trigger(void *ctx, uint8_t port, uint8_t slot, uint32_t fields,
        void (*handler)(void *), void *arg)
{
    bool ret = true;

    struct eint *eint = ctx;
    if (port < eint->int_ports && slot < 32)
    {
        eint->handlers[(port * 32) + slot] = handler;
        eint->args    [(port * 32) + slot] = arg;

        /* Clear spurious interrupt */
        EINT_STA(eint->base, port) = (1 << slot);

        if (handler)
            EINT_CTL(eint->base, port) |=  (1 << slot);
        else
            EINT_CTL(eint->base, port) &= ~(1 << slot);

        uint8_t reg = slot / 8, pos = slot % 8;
        uint32_t data = (fields & 0x7) << (pos * 4);
        uint32_t mask = 0xf            << (pos * 4);
        EINT_CFG(eint->base, port, reg) &= ~mask;
        EINT_CFG(eint->base, port, reg) |= (data & mask);
    }
    else
        ret = false;

    return ret;
}

static bool
count(void *ctx, uint8_t *ports, uint8_t *slots)
{
    struct eint *eint = ctx;
    *ports = eint->int_ports;
    *slots = 32;

    return true;
}

static const drv_eint sunxi_eint =
{
    .trigger = trigger, .count = count,
};

/* Device creation */

extern dev_eint
sunxi_eint_init(uint8_t id)
{
    struct eint *ret = NULL;

    if (id < sizeof(eints) / sizeof(struct eint))
    {
        ret = &(eints[id]);
        switch (id)
        {
            case 0:
                ret->base      = 0x01c20a00;
                ret->int_ports = 2;
                ret->irqs[0]   = 43;
                ret->irqs[1]   = 49;
                break;
            case 1:
                ret->base      = 0x01f02e00;
                ret->int_ports = 1;
                ret->irqs[0]   = 77;
                break;
        }

        for (int i = 0; i < ret->int_ports; i++)
        {
            /* Remove hardware debounce */
            EINT_DEB(ret->base, i) |= 1;
            EINT_DEB(ret->base, i) &= ~0x70;
            /* Set interrupt in the GIC */
            gic_config(ret->irqs[i], handler, ret, false, true);
        }
    }

    return (dev_eint){.driver = &sunxi_eint, .context = ret};
}

extern void
sunxi_eint_clean(dev_eint *e)
{
    if (e)
    {
        struct eint *eint = e->context;

        for (uint8_t i = 0; i < eint->int_ports; i++)
            gic_config(eint->irqs[i], NULL, NULL, false, true);

        e->context = NULL;
    }
}
