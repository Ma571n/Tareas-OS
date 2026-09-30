#ifndef ANTICUCHO_H
#define ANTICUCHO_H

#include <sys/types.h>

#define MSG_MAX 80

// estados de las tareas para saber como van en la parrilla
typedef enum {
    TA_CRUDO,      // no ha empezado
    TA_EN_FIERRO,  // se esta ejecutando
    TA_SERVIO,     // termino bien
    TA_QUEMAO      // fallo o fue cancelada
} ComoTa;

// estructura de cada tarea del plan
typedef struct Actividad {
    char id[64];
    char nombre[128];
    int tiempo_ms;
    char **dependencias_ids;    // ids (texto) de las que depende esta actividad
    int num_dependencias;
    int dependencias_restantes; // las que faltan para que pueda partir
    ComoTa estado;

    // campos para la ejecucion con fork/pipe
    pid_t pid;                  // PID del proceso hijo mientras corre
    int pipe_fd[2];             // pipe hijo -> padre con el mensaje de termino [0]=leer [1]=escribir
    int *dependientes_idx;      // indices de las actividades que dependen de esta
    int num_dependientes;       // cuantas actividades dependen de esta
    int *dependencias_idx;      // indices de las dependencias (misma informacion que dependencias_ids)
    char mensaje[MSG_MAX];      // mensaje de termino, se entrega a los dependientes como insumo
    int simular_falla;          // si es 1 el hijo termina con error (para probar el aislamiento)
} Actividad;

// el grafo completo con todas las tareas
typedef struct Grafo {
    Actividad *actividades;
    int total_actividades;
} Grafo;

Grafo* crear_grafo(void);
void liberar_grafo(Grafo *g);

#endif