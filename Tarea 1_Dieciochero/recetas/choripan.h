#ifndef CHORIPAN_H
#define CHORIPAN_H

#include "anticucho.h"

// ejecuta el plan completo con fork/pipe y control de concurrencia K
// prob_falla es el porcentaje (0 a 100) de que una actividad falle a proposito
int prender_parrilla(Grafo *g, int K, double prob_falla);

#endif