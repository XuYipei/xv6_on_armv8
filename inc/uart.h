<<<<<<< HEAD
#ifndef INC_UART_H
#define INC_UART_H

void uart_init();
void uart_intr();
void uart_putchar(int);
int  uart_getchar();

#endif
=======
#ifndef INC_UART_H
#define INC_UART_H

void uart_init(void);
void uart_putchar(int);
char uart_getchar();

#endif
>>>>>>> ed2d85b... Lab1, main.c
