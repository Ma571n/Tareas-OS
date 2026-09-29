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

    // Mostramos que leyo la wea bien
    printf("--- Tareas Cargadas en el Grafo (%d) ---\n", g->total_actividades);
    for (int i = 0; i < g->total_actividades; i++) {
        printf("[%s] %s | Tiempo: %d ms | Deps restantes: %d\n",
               g->actividades[i].id,
               g->actividades[i].nombre,
               g->actividades[i].tiempo_ms,
               g->actividades[i].dependencias_restantes);
    }

    liberar_grafo(g);
    return 0;
}