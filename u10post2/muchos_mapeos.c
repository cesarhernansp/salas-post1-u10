/*
 * muchos_mapeos.c -- Auxiliar de demostracion (no es parte de la guia).
 *
 * Crea un proceso con cientos de regiones en /proc/PID/maps para poder medir
 * el costo de leer maps, smaps y pmap -x en un proceso "grande". Cada region
 * es un mapeo de dos paginas cuya segunda pagina pasa a PROT_NONE, de modo
 * que el kernel no puede fusionar los mapeos vecinos en una sola region.
 *
 * Compilar:  gcc -o muchos_mapeos muchos_mapeos.c
 * Ejecutar:  ./muchos_mapeos 500 &
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

int main(int argc, char **argv) {
    int n = (argc > 1) ? atoi(argv[1]) : 500;
    long pg = sysconf(_SC_PAGESIZE);

    for (int i = 0; i < n; i++) {
        char *p = mmap(NULL, 2 * pg, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) { perror("mmap"); return 1; }
        p[0] = 1;                       /* la primera pagina queda residente */
        mprotect(p + pg, pg, PROT_NONE);/* parte el mapeo en dos regiones */
    }

    printf("PID %d con unas %d regiones adicionales\n", getpid(), 2 * n);
    fflush(stdout);
    for (;;) pause();                   /* sigue vivo hasta que se le envie una senal */
}
