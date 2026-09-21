#include <stdio.h>
#include <stdlib.h>
#include "../recetas/anticucho.h"

Grafo* crear_grafo(void) {
    Grafo *g = (Grafo*) malloc(sizeof(Grafo));
    if (!g) return NULL;
    g->actividades = NULL;
    g->total_actividades = 0;
    return g;
}

void liberar_grafo(Grafo *g) {
    if (!g) return;
    if (g->actividades) {
        for (int i = 0; i < g->total_actividades; i++) {
            if (g->actividades[i].dependencias_ids) {
                for (int j = 0; j < g->actividades[i].num_dependencias; j++) {
                    free(g->actividades[i].dependencias_ids[j]);
                }
                free(g->actividades[i].dependencias_ids);
            }
        }
        free(g->actividades);
    }
    free(g);
}