#ifndef EMPANADA_PINO_H
#define EMPANADA_PINO_H

// agarramos la misma wea de anticucho pa no repetir codigo
#include "anticucho.h" 

// lee el plan.txt y llena el grafo con las tareas
Grafo* picar_pino_plan(const char *ruta_archivo);

#endif