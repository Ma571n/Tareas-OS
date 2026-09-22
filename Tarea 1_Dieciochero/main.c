#include <stdio.h>
#include <stdlib.h>
#include "recetas/anticucho.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Error de uso. Formato correcto: %s <archivo_plan.txt> <K>\n", argv[0]);
        return 1;
    }

    char *archivo_plan = argv[1];
    int limite_k = atoi(argv[2]);
    
    FILE *archivo = fopen("plan.txt","r");
    char linea[256];
    while(fgets(linea, sizeof(linea), archivo) != NULL){
        printf("Leí esta linea: %s", linea);
    }
    fclose(archivo);


    printf("Iniciando planificador...\n");
    printf("Archivo a cargar: %s\n", archivo_plan);
    printf("Límite de concurrencia (K): %d\n", limite_k);
    
    return 0;
}