#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "recetas/anticucho.h"
#include "recetas/empanada_pino.h"

int main(int argc, char *argv[]) {
    srand(time(NULL)); // Semilla pa los tiempos aleatorios

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <plan.txt> <K>\n", argv[0]);
        return 1;
    }

    char *archivo_plan = argv[1];
    int limite_k = atoi(argv[2]);

    if (limite_k <= 0) {
        fprintf(stderr, "Error: El K tiene que ser mayor a 0.\n");
        return 1;
    }

    printf("Iniciando la fonda...\n");
    printf("Plan: %s | Concurrencia (K): %d\n\n", archivo_plan, limite_k);

    Grafo *g = picar_pino_plan(archivo_plan);
    if (!g) {
        fprintf(stderr, "Error: No se pudo cargar el plan.\n");
        return 1;
    }

    // si el plan no traia tareas no seguimos ejecucion
    if (g->total_actividades == 0) {
        fprintf(stderr, "Error: El archivo del plan esta vacio o con formato malo.\n");
        liberar_grafo(g);
        return 1;
    }

    // Mostramos que leyo la wea bien
    printf("--- Tareas Cargadas en el Grafo (%d) ---\n", g->total_actividades);
    for (int i = 0; i < g->total_actividades; i++) {
        printf("[%s] %s | Tiempo: %d ms | Deps (%d): ",
               g->actividades[i].id,
               g->actividades[i].nombre,
               g->actividades[i].tiempo_ms,
               g->actividades[i].num_dependencias);

        // mostramos las dependencias una por una pa ver si las saco bien
        if (g->actividades[i].num_dependencias == 0) {
            printf("Ninguna");
        } else {
            for (int j = 0; j < g->actividades[i].num_dependencias; j++) {
                printf("%s ", g->actividades[i].dependencias_ids[j]);
            }
        }
        printf("\n");
    }

    liberar_grafo(g);
    return 0;
}