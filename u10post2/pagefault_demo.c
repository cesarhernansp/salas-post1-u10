/*
 * pagefault_demo.c -- Observa el crecimiento de VmRSS y los page faults al
 * tocar 100 MB reservados con malloc (Unidad 10, Parte 2).
 *
 * Compilar:  gcc -o pagefault_demo pagefault_demo.c
 * Ejecutar:  ./pagefault_demo
 *            /usr/bin/time -v ./pagefault_demo
 * Variante:  gcc -DSIN_THP -o pagefault_4k pagefault_demo.c   (paginas de 4 KB)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/prctl.h>

#define N_BYTES (100UL * 1024 * 1024)       /* 100 MB */

/* Imprime los campos de memoria del PROPIO proceso y sus minor faults.
 * La guia usa /proc/self/status dentro de system(), pero "self" seria el
 * proceso grep que lanza system(), no este programa: se usa el PID real. */
static void mostrar_memoria(const char *etapa) {
    char cmd[128];
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);

    printf("--- %s (minor faults del proceso: %ld) ---\n", etapa, ru.ru_minflt);
    fflush(stdout);                         /* para que el orden de la salida sea correcto */

    snprintf(cmd, sizeof(cmd),
             "grep -E \"VmPeak|VmSize|VmRSS\" /proc/%d/status", (int)getpid());
    system(cmd);
}

int main(void) {
#ifdef SIN_THP
    /* Variante (-DSIN_THP): desactiva las paginas enormes transparentes (THP)
     * solo para este proceso, para ver el demand paging con paginas de 4 KB.
     * Con THP activo el kernel puede cubrir el bloque con paginas de 2 MB y
     * el memset provoca muchos menos faults que 100 MB / 4 KB = 25 600. */
    prctl(PR_SET_THP_DISABLE, 1, 0, 0, 0);
#endif

    char *buf = (char *)malloc(N_BYTES);
    if (!buf) { perror("malloc"); return 1; }

    /* Antes de tocar la memoria: reservada (VmSize) pero no residente (VmRSS) */
    mostrar_memoria("antes del memset");

    /* Forzar page faults escribiendo en cada pagina */
    memset(buf, 0xAA, N_BYTES);

    /* Despues: VmRSS debe haber crecido ~100 MB */
    mostrar_memoria("despues del memset");

    free(buf);
    return 0;
}
