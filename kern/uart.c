<<<<<<< HEAD
#include <stdint.h>

#include "uart.h"
#include "arm.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"
#include "console.h"

void
uart_putchar(int c)
{
    while (!(get32(AUX_MU_LSR_REG) & 0x20))
        ;
    put32(AUX_MU_IO_REG, c & 0xFF);
}

char
uart_intr()
{
    for (int stat; !((stat = get32(AUX_MU_IIR_REG)) & 1); )
        if ((stat & 6) == 4)
            cgetchar(get32(AUX_MU_IO_REG) & 0xFF);
}

void
uart_init()
{
    uint32_t selector, enables;

    /* initialize UART */
    enables = get32(AUX_ENABLES);
    enables |= 1;
    put32(AUX_ENABLES, enables);    /* enable UART1, AUX mini uart */
    put32(AUX_MU_CNTL_REG, 0);
    put32(AUX_MU_LCR_REG, 3);       /* 8 bits */
    put32(AUX_MU_MCR_REG, 0);
    put32(AUX_MU_IER_REG, 3 << 2 | 1);
    put32(AUX_MU_IIR_REG, 0xc6);    /* disable interrupts */
    put32(AUX_MU_BAUD_REG, 270);    /* 115200 baud */
    /* map UART1 to GPIO pins */
    selector = get32(GPFSEL1);
    selector&=~((7<<12)|(7<<15)); /* gpio14, gpio15 */
    selector|=(2<<12)|(2<<15);    /* alt5 */
    put32(GPFSEL1, selector);

    put32(GPPUD, 0);            /* enable pins 14 and 15 */
    delay(150);
    put32(GPPUDCLK0, (1<<14)|(1<<15));
    delay(150);
    put32(GPPUDCLK0, 0);        /* flush GPIO setup */
    put32(AUX_MU_CNTL_REG, 3);      /* enable Tx, Rx */
}
=======
#include <stdint.h>

#include "uart.h"
#include "arm.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"

void
uart_putchar(int c)
{
    while (!(get32(AUX_MU_LSR_REG) & 0x20))
        ;
    put32(AUX_MU_IO_REG, c & 0xFF);
}

char
uart_getchar()
{
    while (!(get32(AUX_MU_LSR_REG) & 0x01))
        ;
    return get32(AUX_MU_IO_REG) & 0xFF;
}

void
uart_init()
{
    uint32_t selector;

    selector = get32(GPFSEL1);
    selector &= ~(7<<12);                   /* Clean gpio14 */
    selector |= 2<<12;                      /* Set alt5 for gpio14 */
    selector &= ~(7<<15);                   /* Clean gpio15 */
    selector |= 2<<15;                      /* Set alt5 for gpio15 */
    put32(GPFSEL1, selector);

    put32(GPPUD, 0);
    delay(150);
    put32(GPPUDCLK0, (1<<14)|(1<<15));
    delay(150);
    put32(GPPUDCLK0, 0);

    /* Enable mini uart (this also enables access to it registers) */
    put32(AUX_ENABLES, 1);
    /* Disable auto flow control and disable receiver and transmitter (for now) */
    put32(AUX_MU_CNTL_REG, 0);
    /* Disable receive and transmit interrupts */
    put32(AUX_MU_IER_REG, 0);
    /* Enable 8 bit mode */
    put32(AUX_MU_LCR_REG, 3);
    /* Set RTS line to be always high */
    put32(AUX_MU_MCR_REG, 0);
    /* Set baud rate to 115200 */
    put32(AUX_MU_BAUD_REG, 270);
    /* Finally, enable transmitter and receiver */
    put32(AUX_MU_CNTL_REG, 3);
}
>>>>>>> ed2d85b... Lab1, main.c
