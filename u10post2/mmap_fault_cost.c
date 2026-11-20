/*
 * mmap_fault_cost.c -- Mide el costo de un minor page fault individual
 * (Unidad 10, Parte 2).
 *
 * Un mmap anonimo reserva solo direcciones virtuales: el kernel no asocia
 * marcos fisicos hasta que el proceso toca cada pagina por primera vez
 * (demand paging), y en ese momento ocurre un minor page fault.
 *
 * Compilar:  gcc -o mmap_fault_cost mmap_fault_cost.c
 * Ejecutar:  ./mmap_fault_cost
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>

#define N_PAGES 200                         /* paginas a tocar */

/* Diferencia en nanosegundos entre dos lecturas de clock_gettime */
static double diff_ns(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) * 1e9 + (b.tv_nsec - a.tv_nsec);
}

int main(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    size_t total = (size_t)N_PAGES * page_size;

    /* mmap anonimo: reserva direcciones virtuales sin RAM fisica asociada */
    char *base = mmap(NULL, total, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) { perror("mmap"); return 1; }

    struct timespec t0, t1;
    double sum_first = 0, sum_second = 0;

    for (int i = 0; i < N_PAGES; i++) {
        char *page = base + (size_t)i * page_size;

        /* Primer toque: la pagina aun no tiene marco fisico -> minor page fault */
        clock_gettime(CLOCK_MONOTONIC, &t0);
        page[0] = 1;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        sum_first += diff_ns(t0, t1);

        /* Segundo toque: la pagina ya esta mapeada, no hay fault */
        clock_gettime(CLOCK_MONOTONIC, &t0);
        page[0] = 2;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        sum_second += diff_ns(t0, t1);
    }

    printf("Paginas tocadas:                             %d (de %ld bytes)\n",
           N_PAGES, page_size);
    printf("Costo promedio primer toque (con fault): %10.1f ns\n", sum_first / N_PAGES);
    printf("Costo promedio segundo toque (sin fault): %9.1f ns\n", sum_second / N_PAGES);
    printf("Overhead atribuible al page fault:        %9.1f ns\n",
           (sum_first - sum_second) / N_PAGES);

    munmap(base, total);
    return 0;
}
