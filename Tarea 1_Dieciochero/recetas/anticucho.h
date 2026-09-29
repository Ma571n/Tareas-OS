#ifndef ANTICUCHO_H
#define ANTICUCHO_H

// estados de las tareas pa saber como van en la parrilla
typedef enum {
    TA_CRUDO,      // no ha empezado la wea
    TA_EN_FIERRO,  // se esta ejecutando
    TA_SERVIO,     // termino bien la tarea
    TA_QUEMAO      // dio error la tarea
} ComoTa;

// estructura de cada tarea del plan
typedef struct Actividad {
    char id[64];
    char nombre[128];
    int tiempo_ms;
    char **dependencias_ids; // de que tareas depende esta wea
    int num_dependencias;
    int dependencias_restantes; // las q faltan pa que pueda partir
    ComoTa estado;
} Actividad;

// el grafo completo con todas las tareas
typedef struct Grafo {
    Actividad *actividades;
    int total_actividades;
} Grafo;

Grafo* crear_grafo(void);
void liberar_grafo(Grafo *g);

#endif