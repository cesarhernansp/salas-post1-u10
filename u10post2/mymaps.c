/*
 * mymaps.c -- Muestra donde vive cada tipo de dato en el espacio de
 * direcciones del proceso y espera con getchar() para que se pueda consultar
 * /proc/PID/maps mientras el proceso sigue vivo (Unidad 10, Parte 2).
 *
 * Compilar:  gcc -o mymaps mymaps.c
 * Usar:      ./mymaps &
 *            PID=$!
 *            cat /proc/$PID/maps
 *            kill $PID
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int global_data = 42;               /* segmento .data (inicializado) */
const char *msg = "hola";           /* el puntero va en .data; la cadena, en .rodata */

int main(void) {
    void *heap = malloc(1024 * 1024);   /* 1 MB en el heap */

    printf("PID: %d\n", getpid());
    printf("stack var addr:  %p\n", (void *)&heap);
    printf("heap addr:       %p\n", heap);
    printf("global addr:     %p\n", (void *)&global_data);
    printf("rodata addr:     %p\n", (const void *)msg);
    printf("Presione Enter para continuar...\n");

    getchar();                          /* mantiene vivo el proceso */
    free(heap);
    return 0;
}
