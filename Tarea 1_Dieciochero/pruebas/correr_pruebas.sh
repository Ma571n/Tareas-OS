#!/bin/bash
# corre las pruebas y dice si pasaron o no
# se usa desde la carpeta del proyecto: bash pruebas/correr_pruebas.sh

bien=0
mal=0

# recibe el nombre de la prueba y 0 si paso
resultado() {
    if [ $2 -eq 0 ]; then
        echo "[OK] $1"
        bien=$((bien + 1))
    else
        echo "[FALLO] $1"
        mal=$((mal + 1))
    fi
}

# plan de ejemplo
salida=$(./planificador plan.txt 3)
echo "$salida" | grep -q "Completadas: 6"
resultado "plan.txt con K=3 completa las 6 actividades" $?

salida=$(./planificador plan.txt 1)
echo "$salida" | grep -q "Completadas: 6"
resultado "plan.txt con K=1 completa las 6 actividades" $?

# el insumo le llega a la actividad que depende
salida=$(./planificador plan.txt 3)
echo "$salida" | grep -A1 "\[6\] recibe" | grep -q "armar_choripan"
resultado "servir_mesa recibe el mensaje de armar_choripan" $?

# tiempo vacio
salida=$(./planificador pruebas/sin_tiempo.txt 2)
echo "$salida" | grep -q "\[b\].*Deps (1): a"
resultado "tiempo vacio no borra las dependencias" $?

# nunca mas de K procesos hijos
max=0
./planificador pruebas/largas.txt 2 > /dev/null &
pid=$!
for i in $(seq 1 20); do
    n=$(pgrep -P $pid | wc -l)
    if [ $n -gt $max ]; then max=$n; fi
    sleep 0.1
done
wait $pid
[ $max -le 2 ]
resultado "con K=2 hubo como maximo $max hijos a la vez" $?

# fallas
salida=$(FALLAR=2 ./planificador plan.txt 3)
echo "$salida" | grep -q "Completadas: 2 | Fallidas/Canceladas: 4"
resultado "si falla la 2 se cancelan solo la 4, 5 y 6" $?

salida=$(FALLAR=3 ./planificador plan.txt 3)
echo "$salida" | grep -q "Completadas: 3 | Fallidas/Canceladas: 3"
resultado "si falla la 3 se cancelan solo la 5 y 6" $?

# planes malos
salida=$(./planificador pruebas/ciclo.txt 2)
echo "$salida" | grep -q "Completadas: 1 | Fallidas/Canceladas: 0 | Sin ejecutar: 2"
resultado "plan con ciclo avisa de las actividades sin ejecutar" $?

./planificador pruebas/dep_inexistente.txt 2 > /dev/null 2>&1
[ $? -ne 0 ]
resultado "dependencia que no existe da error" $?

./planificador pruebas/id_repetido.txt 2 > /dev/null 2>&1
[ $? -ne 0 ]
resultado "id repetido da error" $?

./planificador pruebas/linea_invalida.txt 2 > /dev/null 2>&1
[ $? -ne 0 ]
resultado "linea con formato malo da error" $?

./planificador plan.txt 0 > /dev/null 2>&1
[ $? -ne 0 ]
resultado "K=0 da error" $?

# Ctrl+C
./planificador pruebas/largas.txt 3 > /tmp/salida_ctrlc.txt 2>&1 &
pid=$!
sleep 1
kill -INT $pid
wait $pid
codigo=$?
sleep 0.3
[ $codigo -eq 130 ]
resultado "Ctrl+C termina el programa (codigo 130)" $?

[ $(grep -c CANCELADA /tmp/salida_ctrlc.txt) -eq 3 ]
resultado "Ctrl+C cancela las 3 actividades que estaban corriendo" $?

[ -z "$(pgrep -x planificador)" ]
resultado "despues de Ctrl+C no quedan procesos" $?

# 10000 actividades: cada una depende de la mitad de su numero
rm -f /tmp/plan_10000.txt
echo "1 : act1 : 2 :" > /tmp/plan_10000.txt
for i in $(seq 2 10000); do
    echo "$i : act$i : 2 : $((i / 2))" >> /tmp/plan_10000.txt
done

salida=$(./planificador /tmp/plan_10000.txt 200)
echo "$salida" | grep -q "Completadas: 10000"
resultado "10000 actividades con K=200 se completan todas" $?

salida=$(FALLA_PCT=3 ./planificador /tmp/plan_10000.txt 200)
echo "$salida" | grep -q "RESUMEN DE LA FONDA"
resultado "10000 actividades con fallas: el programa termina bien" $?

echo
echo "pasaron $bien, fallaron $mal"