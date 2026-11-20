# salas-post1-u10 — Memorias del computador: jerarquía de caché y memoria virtual

**Autor:** César Hernán Salas Peñaranda  
**Curso:** Arquitectura de Computadores — Unidad 10, post-contenido (laboratorio en dos partes)  
**Repositorio único:** `salas-post1-u10`

Este repositorio reúne las dos partes del laboratorio de memoria en lenguaje C sobre Linux. La Parte 1 mide con un benchmark propio el efecto de la jerarquía de caché sobre la latencia de acceso, incluido un patrón con stride fijo que expone conflictos de asociatividad. La Parte 2 examina la memoria virtual de un proceso Linux a través de `/proc`, el comportamiento del page fault agregado y el costo de un page fault individual con `mmap`.

## Entorno de trabajo

| Componente | Detalle |
|---|---|
| Sistema anfitrión | Windows 11 Home, procesador Intel Core Ultra 5 226V, 16 GB de RAM |
| Hipervisor | VirtualBox 7.2.22 |
| Máquina virtual | `U10-Alpine`: 2 CPU, 2048 MB de RAM, red NAT, sin disco |
| Sistema invitado | Alpine Linux 3.24.4 (kernel 6.18.60-lts) arrancado en modo *live* desde el ISO, con `root` sin contraseña |
| Compilador | `gcc` 15.2.0 (musl libc 1.2.6) |
| Herramientas | `pmap`, `vmstat` y `free` (`procps-ng`), `/usr/bin/time` (GNU `time`), `lscpu` (`util-linux`) |

La guía recomienda Ubuntu 20.04 o equivalente. El estudiante reutilizó el ISO de Alpine que ya tenía de la Unidad 5 y creó una VM nueva y desechable, sin tocar la del laboratorio anterior. Alpine usa **musl** en lugar de glibc, así que en los mapas de memoria de la Parte 2 la librería de C aparecerá como `libc.musl-x86_64.so.1` y el cargador como `ld-musl-x86_64.so.1`, en lugar de `libc.so` y `ld-linux.so`.

Los paquetes se instalaron con `apk` desde los repositorios oficiales de Alpine:

```
apk add gcc musl-dev procps-ng time util-linux
```

Los archivos `.c` se escriben en el anfitrión, dentro de este repositorio, y la VM los descarga desde un servidor HTTP local temporal que solo la VM alcanza (`10.0.2.2`). Los resultados se leen en la consola de la VM y se capturan con `VBoxManage controlvm screenshotpng`, que captura únicamente la pantalla de la VM.

## Estructura del repositorio

```
salas-post1-u10/
├── README.md
├── u10post1/       # Parte 1: cache_bench.c
├── u10post2/       # Parte 2: mymaps.c, pagefault_demo.c, mmap_fault_cost.c
└── capturas/       # capturas de la consola de la VM, nombradas pN_cpM_descripcion.png
```

---

# Parte 1 — Benchmark de jerarquía de caché

## Paso 1 — Configuración del entorno y detección de caché

El estudiante creó el directorio de trabajo y consultó la topología de caché que el kernel expone en `/sys/devices/system/cpu/cpu0/cache/`:

```
mkdir -p ~/u10post1 && cd ~/u10post1
for i in 0 1 2 3; do
   echo -n "Cache level $i: "
   cat /sys/devices/system/cpu/cpu0/cache/index${i}/size 2>/dev/null || echo "N/A"
done
```

Además de los tamaños, leyó la geometría de cada nivel (nivel, tipo, vías, tamaño de línea y número de sets), porque la decisión técnica del Paso 5 depende de ella.

| Nivel | Tipo | Tamaño | Vías | Línea | Sets |
|---|---|---|---|---|---|
| L1 (`index0`) | Datos | 32 KB | 8 | 64 B | 64 |
| L1 (`index1`) | Instrucciones | 64 KB | 8 | 64 B | 128 |
| L2 (`index2`) | Unificada | 4096 KB (4 MB) | 16 | 64 B | 4096 |
| L3 (`index3`) | — | N/A | — | — | — |

**Observaciones.**

- La caché L1 de datos es de 32 KB, de 8 vías y con líneas de 64 bytes: 32 KB / (8 vías × 64 B) = 64 sets. Es exactamente la geometría de ejemplo que usa la guía en las decisiones técnicas.
- **La VM no expone una caché L3**: `index3` no existe, y por eso el bucle de la guía imprime `N/A` en el nivel 3. Los valores son los que el hipervisor presenta al invitado, que pueden diferir de la jerarquía física del procesador; el anfitrión (Windows) informa 8 MB de L3 para este procesador, pero esa información no llega a la VM. Esto se tiene en cuenta al interpretar los saltos del benchmark.
- `lscpu` confirma que el invitado corre sobre un hipervisor (`KVM`) y que ve 2 CPU.

**Checkpoint 1.** Tamaños detectados: L1d = 32 KB, L2 = 4 MB, L3 = no expuesta por la VM.

![Tamaños de caché por nivel](capturas/p1_cp1_caches_sysfs.png)
![Geometría de la caché y lscpu](capturas/p1_cp1_caches_geometria.png)

## Paso 2 — Benchmark de latencia por tamaño de array, acceso secuencial (`cache_bench.c`)

`cache_bench.c` reserva un array de `n_bytes`, lo toca con `memset` antes de medir (para que las páginas ya estén en RAM) y lo recorre secuencialmente 100 veces (`REPEAT`) leyendo un byte por iteración. El tiempo se mide con `clock_gettime(CLOCK_MONOTONIC)` y se reporta en **ns por byte**. El puntero es `volatile` y el programa se compila con `-O0`, para que el compilador no elimine unas lecturas que no tienen efecto observable. Los tamaños se expresan con las constantes `KB` y `MB`, agrupados con un comentario por nivel de caché.

```
gcc -O0 -o cache_bench cache_bench.c && ./cache_bench
```

### Resultados

Se ejecutó el benchmark tres veces seguidas (ns/byte). La primera es la de la captura.

| Array (KB) | Región | Corrida 1 | Corrida 2 | Corrida 3 | Promedio |
|---|---|---|---|---|---|
| 4 | L1 | 0.865 | 2.838 | 1.421 | 1.708 |
| 8 | L1 | 0.706 | 1.145 | 1.351 | 1.067 |
| 16 | L1 | 0.648 | 1.242 | 0.518 | 0.803 |
| 32 | L1 (límite) | 0.656 | 1.070 | 0.663 | 0.796 |
| 64 | L2 | 0.703 | 1.025 | 0.736 | 0.821 |
| 128 | L2 | 0.655 | 0.877 | 0.567 | 0.700 |
| 256 | L2 | 0.671 | 0.719 | 0.528 | 0.639 |
| 512 | L2 | 0.669 | 0.724 | 0.535 | 0.643 |
| 1024 | L2 | 0.672 | 0.720 | 0.540 | 0.644 |
| 4096 | L2 (límite) / L3 | 0.559 | 0.639 | 0.588 | 0.595 |
| 16384 | RAM | 0.536 | 0.549 | 0.543 | 0.543 |
| 65536 | RAM | 0.540 | 0.580 | 0.553 | 0.558 |

### Interpretación

Los puntos de transición esperados según el checkpoint 1 son 32 KB (fin de L1d) y 4 MB (fin de L2). **En esta máquina no se observan los saltos de latencia que anticipa la guía**: el costo por byte es prácticamente constante, entre 0.5 y 0.9 ns, y las corridas más ruidosas solo muestran valores altos en los arrays más pequeños (4 y 8 KB), que se atribuyen al arranque en frío de la medición (la CPU todavía está subiendo de frecuencia y el kernel de la VM está atendiendo otras tareas), no a un efecto de la jerarquía de caché. Los valores incluso bajan ligeramente con el tamaño. Las causas más probables son:

- **Localidad espacial.** Una línea de caché tiene 64 bytes y el recorrido es byte a byte, de modo que de cada 64 accesos solo uno cae en una línea nueva; el costo del fallo se reparte entre 64 lecturas.
- **Prefetch de hardware.** Un recorrido secuencial es el patrón más fácil de predecir: el prefetcher del procesador trae las líneas siguientes antes de que se pidan, y oculta la latencia de L2, L3 y RAM.
- **Costo del propio bucle con `-O0`.** Sin optimización, cada iteración carga y guarda `i` y `n_bytes` en la pila además de leer el byte `volatile`. Ese costo fijo, del orden de medio nanosegundo por byte, es mayor que lo que aporta el nivel de memoria desde el que se sirve cada línea.
- **Entorno virtualizado.** La VM presenta su propia jerarquía de caché y comparte el procesador físico con Windows, lo que añade ruido a las medidas.

**Checkpoint 2.** El programa compila sin errores con `gcc -O0` y mide la latencia secuencial con `clock_gettime`, pero la tabla **no muestra saltos discernibles** que se correspondan con L1, L2 y L3. El estudiante lo documenta tal como se observó y lo explica con la localidad espacial y el prefetching, en lugar de forzar un resultado.

![Compilación de cache_bench](capturas/p1_cp2_compilacion.png)
![Tabla de latencia secuencial](capturas/p1_cp2_tabla_secuencial.png)
