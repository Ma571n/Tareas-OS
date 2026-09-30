#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../recetas/empanada_pino.h"

// saca espacios y saltos de linea al inicio y al final (modifica el string)
static char* limpiecita(char *s) {
    if (!s) return s;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    end[1] = '\0';
    return s;
}


Grafo* picar_pino_plan(const char *ruta_archivo) {
    FILE *f = fopen(ruta_archivo, "r");
    if (!f) {
        perror("Error al abrir el plan");
        return NULL;
    }

    Grafo *g = crear_grafo();
    if (!g) {
        fclose(f);
        return NULL;
    }

    char *linea = NULL;
    size_t cap_linea = 0;
    int nro = 0;
    int capacidad = 0;

    // getline crece solo: soporta lineas con miles de dependencias
    while (getline(&linea, &cap_linea, f) != -1) {
        nro++;
        char *txt = limpiecita(linea);
        if (txt[0] == '\0' || txt[0] == '#') continue; // lineas vacias o comentarios

        // el arreglo de actividades crece al doble para no hacer un realloc por linea
        if (g->total_actividades == capacidad) {
            int nueva = capacidad ? capacidad * 2 : 64;
            Actividad *tmp = realloc(g->actividades, sizeof(Actividad) * (size_t)nueva);
            if (!tmp) {
                perror("realloc actividades");
                goto error;
            }
            g->actividades = tmp;
            capacidad = nueva;
        }
        Actividad *act = &g->actividades[g->total_actividades];
        memset(act, 0, sizeof(Actividad));
        g->total_actividades++; // desde aqui la actividad esta en un estado valido para liberar

        // partir en hasta 4 campos usando los primeros tres ':'
        char *campos[4] = {NULL, NULL, NULL, NULL};
        char *p = txt;
        for (int k = 0; k < 4; k++) {
            campos[k] = p;
            if (k == 3) break;
            char *c = strchr(p, ':');
            if (!c) break;
            *c = '\0';
            p = c + 1;
        }

        char *id = limpiecita(campos[0]);
        char *nombre = campos[1] ? limpiecita(campos[1]) : NULL;
        if (!nombre || id[0] == '\0' || nombre[0] == '\0') {
            fprintf(stderr, "Error en %s, linea %d: formato invalido (ID : Nombre : tiempo_ms : dependencias)\n",
                    ruta_archivo, nro);
            goto error;
        }

        snprintf(act->id, sizeof(act->id), "%s", id);
        snprintf(act->nombre, sizeof(act->nombre), "%s", nombre);

        // si no hay tiempo valido se asigna uno aleatorio entre 100 y 5000 ms
        int t = 0;
        if (campos[2]) {
            char *t_str = limpiecita(campos[2]);
            if (t_str[0] != '\0') {
                char *resto;
                long v = strtol(t_str, &resto, 10);
                if (*resto == '\0' && v > 0 && v <= INT_MAX / 1000) t = (int)v;
            }
        }
        if (t <= 0) t = (rand() % 4901) + 100;
        act->tiempo_ms = t;
        act->estado = TA_CRUDO;

        // dependencias: lista separada por comas, con o sin corchetes
        act->dependencias_ids = NULL;
        act->num_dependencias = 0;
        if (campos[3]) {
            char *d_str = limpiecita(campos[3]);
            if (d_str[0] == '[') d_str++;
            size_t len = strlen(d_str);
            if (len > 0 && d_str[len - 1] == ']') d_str[len - 1] = '\0';
            d_str = limpiecita(d_str);

            if (d_str[0] != '\0') {
                size_t maximo = 1;
                for (char *q = d_str; *q; q++) if (*q == ',') maximo++;
                act->dependencias_ids = malloc(sizeof(char*) * maximo);
                if (!act->dependencias_ids) {
                    perror("malloc dependencias");
                    goto error;
                }
                char *dep_id = strtok(d_str, ",");
                while (dep_id) {
                    dep_id = limpiecita(dep_id);
                    if (dep_id[0] != '\0') {
                        char *copia = strdup(dep_id);
                        if (!copia) {
                            perror("strdup");
                            goto error;
                        }
                        act->dependencias_ids[act->num_dependencias++] = copia;
                    }
                    dep_id = strtok(NULL, ",");
                }
            }
        }
        act->dependencias_restantes = act->num_dependencias;
    }

    free(linea);
    fclose(f);
    return g;

error:
    free(linea);
    fclose(f);
    liberar_grafo(g);
    return NULL;
}