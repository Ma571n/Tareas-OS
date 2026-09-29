#include <stdio.h>
#include <stdlib.h>
#include "recetas/anticucho.h"
#include "recetas/empanada_pino.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <plan.txt> <K>\n", argv[0]);
        return 1;
    }

    char *archivo_plan = argv[1];
    int limite_k = atoi(argv[2]);

    if (limite_k <= 0) {
        fprintf(stderr, "Error: El K tiene que ser un numero mayor a 0 po.\n");
        return 1;
    }

    printf("Iniciando la fonda...\n");
    printf("Plan: %s | Concurrencia (K): %d\n", archivo_plan, limite_k);

    // Cargar y procesar el plan
    Grafo *g = picar_pino_plan(archivo_plan);
    if (!g) {
        fprintf(stderr, "Error: No se pudo cargar el archivo del plan.\n");
        return 1;
    }

    liberar_grafo(g);
    return 0;
}