#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "recetas/anticucho.h"
#include "recetas/empanada_pino.h"
#include "recetas/choripan.h"

int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL)); // semilla para los tiempos aleatorios

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <plan.txt> <K>\n", argv[0]);
        return 1;
    }

    char *archivo_plan = argv[1];
    char *resto;
    long k_leido = strtol(argv[2], &resto, 10);
    if (*resto != '\0' || k_leido <= 0 || k_leido > 1000000) {
        fprintf(stderr, "Error: K tiene que ser un entero mayor a 0.\n");
        return 1;
    }
    int limite_k = (int)k_leido;

    printf("Iniciando la fonda...\n");
    printf("Plan: %s | Concurrencia (K): %d\n\n", archivo_plan, limite_k);

    Grafo *g = picar_pino_plan(archivo_plan);
    if (!g) {
        fprintf(stderr, "Error: No se pudo cargar el plan.\n");
        return 1;
    }

    // si el plan no traia tareas no se sigue
    if (g->total_actividades == 0) {
        fprintf(stderr, "Error: El archivo del plan esta vacio o con formato malo.\n");
        liberar_grafo(g);
        return 1;
    }

    // se muestran las tareas leidas
    printf("--- Tareas Cargadas en el Grafo (%d) ---\n", g->total_actividades);
    for (int i = 0; i < g->total_actividades; i++) {
        printf("[%s] %s | Tiempo: %d ms | Deps (%d): ",
               g->actividades[i].id,
               g->actividades[i].nombre,
               g->actividades[i].tiempo_ms,
               g->actividades[i].num_dependencias);

        // se muestran las dependencias una por una
        if (g->actividades[i].num_dependencias == 0) {
            printf("Ninguna");
        } else {
            for (int j = 0; j < g->actividades[i].num_dependencias; j++) {
                printf("%s ", g->actividades[i].dependencias_ids[j]);
            }
        }
        printf("\n");
    }

    // se prende la parrilla (motor de ejecucion)
    int resultado = prender_parrilla(g, limite_k);

    liberar_grafo(g);
    return resultado;
}