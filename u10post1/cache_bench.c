/*
 * cache_bench.c -- Benchmark de la jerarquia de cache (Unidad 10, Parte 1)
 *
 * Compilar:  gcc -O0 -o cache_bench cache_bench.c
 * Ejecutar:  ./cache_bench
 *
 * Se compila con -O0 y los accesos usan "volatile" para que el compilador no
 * elimine las lecturas, que no tienen ningun efecto observable.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define REPEAT 100                  /* pasadas completas sobre el array */

/* Tamanos de array (bytes). Cada grupo apunta a un nivel de la jerarquia
 * detectada en el checkpoint 1: L1d = 32 KB, L2 = 4 MB, sin L3 expuesto. */
#define KB (1024UL)
#define MB (1024UL * KB)

/* Medir tiempo de acceso secuencial a un array de n_bytes (ns por byte) */
double bench_seq(size_t n_bytes) {
    volatile char *arr = (volatile char *)malloc(n_bytes);
    if (!arr) return -1.0;
    memset((void *)arr, 1, n_bytes);        /* toca las paginas antes de medir */

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int r = 0; r < REPEAT; r++)
        for (size_t i = 0; i < n_bytes; i++)
            (void)arr[i];                   /* acceso de lectura */

    clock_gettime(CLOCK_MONOTONIC, &t1);
    free((void *)arr);

    double ns = (t1.tv_sec - t0.tv_sec) * 1e9
              + (t1.tv_nsec - t0.tv_nsec);

    return ns / (REPEAT * (double)n_bytes); /* ns/byte */
}

/* Acceso aleatorio usando indices pre-mezclados (Fisher-Yates).
 * Cada acceso salta a una posicion impredecible: se pierde la localidad
 * espacial y el prefetch. Devuelve ns por acceso (un acceso = un int). */
double bench_rand(size_t n_bytes) {
    size_t n = n_bytes / sizeof(int);
    int *arr = (int *)malloc(n * sizeof(int));
    size_t *idx = (size_t *)malloc(n * sizeof(size_t));
    if (!arr || !idx) return -1.0;

    for (size_t i = 0; i < n; i++) { arr[i] = i; idx[i] = i; }
    /* Mezcla Fisher-Yates */
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        size_t tmp = idx[i]; idx[i] = idx[j]; idx[j] = tmp;
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    volatile long sum = 0;
    for (int r = 0; r < REPEAT; r++)
        for (size_t i = 0; i < n; i++)
            sum += arr[idx[i]];
    clock_gettime(CLOCK_MONOTONIC, &t1);

    free(arr); free(idx);
    double ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
    return ns / (REPEAT * (double)n);       /* ns/acceso */
}

/* Acceso con stride fijo: expone conflictos de asociatividad.
 * Si el stride es (tamano de cache / numero de vias), todos los accesos caen
 * en el mismo set y compiten por sus vias. Devuelve ns por acceso. */
double bench_stride(size_t n_bytes, size_t stride) {
    volatile char *arr = (volatile char *)malloc(n_bytes);
    if (!arr) return -1.0;
    memset((void *)arr, 1, n_bytes);

    size_t steps = n_bytes / stride;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int r = 0; r < REPEAT; r++)
        for (size_t i = 0; i < steps; i++)
            (void)arr[(i * stride) % n_bytes];

    clock_gettime(CLOCK_MONOTONIC, &t1);
    free((void *)arr);

    double ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
    return ns / (REPEAT * (double)steps);   /* ns/acceso */
}

int main(void) {
    size_t sizes[] = {
        4 * KB, 8 * KB, 16 * KB, 32 * KB,           /* region L1 */
        64 * KB, 128 * KB, 256 * KB,                /* region L2 */
        512 * KB, 1 * MB, 4 * MB,                   /* region L2 / L3 */
        16 * MB, 64 * MB                            /* region RAM */
    };

    int n = sizeof(sizes) / sizeof(sizes[0]);
    printf("%-12s %14s %14s\n", "Array (KB)", "seq ns/acceso", "rand ns/acceso");
    for (int i = 0; i < n; i++) {
        double seq  = bench_seq(sizes[i]);
        double rnd  = bench_rand(sizes[i]);
        printf("%-12zu %14.3f %14.3f\n", sizes[i] / 1024, seq, rnd);
        fflush(stdout);
    }

    /* Stride sobre 1 MB (cabe en la L2 de 4 MB detectada en el checkpoint 1).
     * 64 B   = una linea de cache.
     * 4096 B = tamano de pagina = 32 KB (L1d) / 8 vias: mismo set de la L1. */
    printf("\n%-12s %10s\n", "Stride", "ns/acceso");
    double t_line = bench_stride(1 * MB, 64);
    double t_page = bench_stride(1 * MB, 4096);
    printf("%-12s %10.3f\n", "64 B", t_line);
    printf("%-12s %10.3f\n", "4096 B", t_page);

    return 0;
}
