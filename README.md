Tarea 1 "El planificador dieciochero"
-----------------------------------------------------------------------
Integrantes:
-Matías Ignacio Neira Guzmán
-Martín Alejandro Mondaca Labarca
-----------------------------------------------------------------------
Descripcion:
Programa en C que lee un plan de actividades (plan.txt) con duraciones y dependencias, y las ejecuta en el orden correcto. Cada actividad es un proceso hijo (fork), se comunican por pipes y el programa reacciona a señales (Ctrl+C). Nunca hay más de K actividades corriendo a la vez.
-----------------------------------------------------------------------
Estructura:
-main.c: valida los argumentos y llama al resto.

-recetas/: archivos .h (anticucho.h, empanada_pino.h, choripan.h).

-parrilla/: archivos .c (anticucho.c, empanada_pino.c, choripan.c).

-pruebas/: planes de prueba y scripts.

-plan.txt: plan de ejemplo.
-----------------------------------------------------------------------
Compilar:
Desde la carpeta del proyecto:
"make"
Compila con gcc -Wall -Wextra -std=c17 y deja el ejecutable planificador. Para borrar lo compilado: make clean. El -lpthread del Makefile está solo porque lo pide la rúbrica; no usamos hilos.
-----------------------------------------------------------------------
Ejecutar
./planificador plan.txt K [prob_fallo]

K: máximo de actividades a la vez (entero mayor a 0).
prob_fallo (opcional): de 0 a 100, probabilidad de que cada actividad falle a propósito, para demostrar el manejo de errores. También se puede usar la variable FALLA_PCT.
FALLAR=2,4 fuerza que fallen esas actividades. Ejemplo:

FALLAR=2 ./planificador plan.txt 3

Código de salida: 0 todo bien, 1 hubo fallas o errores en el plan, 130 Ctrl+C.
-----------------------------------------------------------------------
Formato de plan.txt
Una actividad por línea: ID : Nombre : tiempo_ms : dependencias

Ejemplo: 4: asar_longaniza: 800: 1,2

Las dependencias van separadas por coma, con o sin corchetes.
Si el tiempo está vacío, se asigna uno aleatorio entre 100 y 5000 ms.
Las líneas vacías o que parten con # se ignoran.
-----------------------------------------------------------------------
Funciones principales:
-crear_grafo / liberar_grafo: crean y liberan el grafo de actividades.

-picar_pino_plan: lee y valida plan.txt y arma el grafo.

-prender_parrilla: motor principal. Lanza actividades, espera, propaga mensajes, maneja fallas y Ctrl+C, e imprime el resumen.

-resolver_dependencias: pasa los IDs a posiciones (tabla hash) y detecta IDs repetidos o dependencias inexistentes.

-lanzar_actividad / ejecutar_hijo: crean los pipes y el proceso hijo, y lo que hace cada hijo.

-quemar_rama: cancela las actividades que dependían de una que falló.
-----------------------------------------------------------------------
Diseño:
-Concurrencia: un proceso por actividad, creado solo cuando sus dependencias terminaron y hay cupo (menos de K vivos).

-Pipes: el hijo le avisa al padre que terminó y el padre le entrega los mensajes (insumos) a la actividad dependiente al crearla. No se le escribe directo al dependiente porque todavía no existe.

-Sin busy-waiting ni race conditions: el padre espera con sigsuspend, con SIGINT y SIGCHLD bloqueados, así no se pierden señales y no gasta CPU.

-Tabla hash: para buscar dependencias rápido con 10000 actividades.

-Errores: si una actividad falla (error, señal o no envía su mensaje), solo se cancelan las que dependían de ella; el resto sigue.

-Ctrl+C: el manejador solo levanta una bandera; el programa manda SIGTERM a los hijos, espera que terminen (sin zombies) y sale con código 130.

-Planes malos: ID repetido, dependencia inexistente o línea inválida dan error antes de ejecutar. Un ciclo se informa al final como actividades sin ejecutar.
-----------------------------------------------------------------------
Pruebas:
Con el programa compilado: bash pruebas/correr_pruebas.sh
Corre 17 pruebas automáticas: distintos K, fallas, planes inválidos, Ctrl+C y 10000 actividades.