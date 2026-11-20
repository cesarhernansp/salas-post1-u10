#!/bin/sh
# smaps_resumen.sh -- Resume /proc/PID/smaps: una fila por region con los
# campos que usa el checkpoint 2 (valores en KB). Es un auxiliar de lectura,
# no forma parte de la guia: el volcado completo de smaps no cabe en una pantalla.
#
# Uso: sh smaps_resumen.sh [PID]      (sin argumento, el propio shell)

PID=${1:-$$}

awk -v pid="$PID" '
function salida(   m) {
    m = n
    if (length(m) > 26) m = substr(m, length(m) - 25)
    printf "%-6s %-26s %6d %5d %8d %8d %8d %8d %6d\n", p, m, s, r, sc, sd, pc, pd, ref
}
BEGIN {
    printf "smaps del PID %s (KB)\n", pid
    printf "%-6s %-26s %6s %5s %8s %8s %8s %8s %6s\n", \
           "Perm", "Mapeo", "Size", "Rss", "Sh_Clean", "Sh_Dirty", "Pr_Clean", "Pr_Dirty", "Ref"
}
/^[0-9a-f]+-[0-9a-f]+ / {
    if (n != "") salida()
    p = $2; n = $6; if (n == "") n = "[anon]"
}
/^Size:/          { s = $2 }
/^Rss:/           { r = $2 }
/^Shared_Clean:/  { sc = $2 }
/^Shared_Dirty:/  { sd = $2 }
/^Private_Clean:/ { pc = $2 }
/^Private_Dirty:/ { pd = $2 }
/^Referenced:/    { ref = $2 }
END { salida() }
' /proc/$PID/smaps
