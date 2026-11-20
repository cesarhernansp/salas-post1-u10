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

int main(void) {
    size_t sizes[] = {
        4 * KB, 8 * KB, 16 * KB, 32 * KB,           /* region L1 */
        64 * KB, 128 * KB, 256 * KB,                /* region L2 */
        512 * KB, 1 * MB, 4 * MB,                   /* region L2 / L3 */
        16 * MB, 64 * MB                            /* region RAM */
    };

    int n = sizeof(sizes) / sizeof(sizes[0]);
    printf("%-12s %10s\n", "Array (KB)", "ns/byte");
    for (int i = 0; i < n; i++) {
        double lat = bench_seq(sizes[i]);
        printf("%-12zu %10.3f\n", sizes[i] / 1024, lat);
    }

    return 0;
}
