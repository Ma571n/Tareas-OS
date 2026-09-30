#!/bin/bash
# pruebas del planificador
# se corre desde la carpeta del proyecto: bash pruebas/pruebas.sh

echo "1) plan.txt con K=3 (deberian completarse las 6)"
./planificador plan.txt 3 | tail -2

echo
echo "2) plan.txt con K=1, una actividad a la vez"
./planificador plan.txt 1 | tail -2

echo
echo "3) falla la actividad 2 (se cancelan la 4, 5 y 6)"
FALLAR=2 ./planificador plan.txt 3 | tail -8

echo
echo "4) plan con ciclo"
./planificador pruebas/ciclo.txt 2 | tail -3

echo
echo "5) dependencia que no existe"
./planificador pruebas/dep_inexistente.txt 2

echo
echo "6) id repetido"
./planificador pruebas/id_repetido.txt 2

echo
echo "7) actividad sin tiempo pero con dependencias"
./planificador pruebas/sin_tiempo.txt 2 | grep "Deps"

echo
echo "8) 10000 actividades con K=200"
# cada actividad depende de la mitad de su numero (i/2), queda como un arbol
rm -f /tmp/plan_10000.txt
for i in $(seq 1 10000); do
    if [ $i -eq 1 ]; then
        echo "$i : act$i : 2 :" >> /tmp/plan_10000.txt
    else
        echo "$i : act$i : 2 : $((i / 2))" >> /tmp/plan_10000.txt
    fi
done
time ./planificador /tmp/plan_10000.txt 200 | tail -2

echo
echo "9) 10000 actividades, con algunas fallando (3%)"
FALLA_PCT=3 ./planificador /tmp/plan_10000.txt 200 | tail -2

echo
echo "Ctrl+C se prueba a mano: ./planificador pruebas/largas.txt 3 y apretar Ctrl+C"