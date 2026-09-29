#include <stdio.h>
#include <stdlib.h>
#include "../recetas/anticucho.h"

// pa pedir la memoria del grafo vacio
Grafo* crear_grafo(void) {
    Grafo *g = (Grafo*) malloc(sizeof(Grafo));
    if (!g) return NULL; // si no hay ram cague
    
    g->actividades = NULL;
    g->total_actividades = 0;
    return g;
}

// liberamos todo pa que no haya algun error por leaks
void liberar_grafo(Grafo *g) {
    if (!g) return;

    if (g->actividades) {
        for (int i = 0; i < g->total_actividades; i++) {
            if (g->actividades[i].dependencias_ids) {
                for (int j = 0; j < g->actividades[i].num_dependencias; j++) {
                    free(g->actividades[i].dependencias_ids[j]); // libero los string de dependencias
                }
                free(g->actividades[i].dependencias_ids);
            }
        }
        free(g->actividades); // libero el arreglo de actividades
    }
    free(g); // y libero la estructura del grafo
}