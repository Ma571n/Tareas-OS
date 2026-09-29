#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../recetas/empanada_pino.h"

// pa sacarle los espacios y saltos de linea feos
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
        perror("Error al abrir el plan.txt");
        return NULL;
    }

    Grafo *g = crear_grafo();
    if (!g) {
        fclose(f);
        return NULL;
    }

    char linea[512];
    while (fgets(linea, sizeof(linea), f)) {
        char *txt = limpiecita(linea);
        if (strlen(txt) == 0 || txt[0] == '#') continue; // ignoro lineas vacias o comentarios

        // agrandamos el arreglo de actividades pa meter la nueva
        g->actividades = realloc(g->actividades, sizeof(Actividad) * (g->total_actividades + 1));
        Actividad *act = &g->actividades[g->total_actividades];
        memset(act, 0, sizeof(Actividad));

        // partimos la linea usando los dos puntos
        char *id = strtok(txt, ":");
        char *nombre = strtok(NULL, ":");
        char *tiempo = strtok(NULL, ":");
        char *deps = strtok(NULL, ":");

        if (!id || !nombre) continue;

        snprintf(act->id, sizeof(act->id), "%s", limpiecita(id));
        snprintf(act->nombre, sizeof(act->nombre), "%s", limpiecita(nombre));

        // si no le ponen tiempo le asignamos un numero random entre 100 y 5000 ms
        int t = 0;
        if (tiempo) {
            char *t_str = limpiecita(tiempo);
            if (strlen(t_str) > 0) t = atoi(t_str);
        }
        if (t <= 0) {
            t = (rand() % 4901) + 100;
        }
        act->tiempo_ms = t;
        act->estado = TA_CRUDO;

        // aca sacamos las dependencias
        act->dependencias_ids = NULL;
        act->num_dependencias = 0;

        if (deps) {
            char *d_str = limpiecita(deps);
            if (d_str[0] == '[') d_str++;
            int len = strlen(d_str);
            if (len > 0 && d_str[len - 1] == ']') d_str[len - 1] = '\0';

            d_str = limpiecita(d_str);
            if (strlen(d_str) > 0) {
                char *dep_id = strtok(d_str, ",");
                while (dep_id) {
                    dep_id = limpiecita(dep_id);
                    if (strlen(dep_id) > 0) {
                        act->dependencias_ids = realloc(act->dependencias_ids, sizeof(char*) * (act->num_dependencias + 1));
                        act->dependencias_ids[act->num_dependencias] = strdup(dep_id);
                        act->num_dependencias++;
                    }
                    dep_id = strtok(NULL, ",");
                }
            }
        }
        act->dependencias_restantes = act->num_dependencias;
        g->total_actividades++;
    }

    fclose(f);
    return g;
}