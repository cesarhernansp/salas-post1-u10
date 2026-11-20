#!/bin/sh
# costo_lectura.sh -- Auxiliar de demostracion (no es parte de la guia).
# Mide cuanto tarda leer N veces la informacion de memoria de un proceso con
# distintas fuentes: /proc/PID/maps, /proc/PID/smaps, smaps_rollup y pmap.
#
# Uso: sh costo_lectura.sh PID [N]

PID=$1
N=${2:-200}
[ -z "$PID" ] && { echo "uso: sh costo_lectura.sh PID [N]"; exit 1; }

echo "PID $PID: $(wc -l < /proc/$PID/maps) regiones, $N lecturas por fuente"
for cmd in "cat /proc/$PID/maps" \
           "cat /proc/$PID/smaps" \
           "cat /proc/$PID/smaps_rollup" \
           "pmap $PID" \
           "pmap -x $PID"; do
    /usr/bin/time -f "%e s   %C" sh -c \
        "i=0; while [ \$i -lt $N ]; do $cmd >/dev/null; i=\$((i+1)); done"
done
