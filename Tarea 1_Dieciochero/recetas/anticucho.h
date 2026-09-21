#ifndef Anticucho_H
#define Anticucho_H

typedef enum {
    Ta_Crudo,       // Aun no se cocciona
    Ta_En_Fierro,  // Se ta coccionando
    Ta_Servio,    //  Se sirvio
    Ta_Quemao    // Se quemo
} ComoTa;

typedef struct Actividad {
    char id[64];
    char nombre[128];
    int tiempo_ms;               // Si es 0 o negativo, asignaremos entre 100 y 5000 ms
    char **dependencias_ids;
    int num_dependencias;
    int dependencias_restantes;  // Cuantas dependencias faltan pa terminar
    ComoTa estado;
} Actividad;

typedef struct Grafo {
    Actividad *actividades;
    int total_actividades;
} Grafo;

Grafo* crear_grafo(void);
void liberar_grafo(Grafo *g);

#endif