# Compilador y banderas exigidas en la tarea
CC = gcc
CFLAGS = -Wall -Wextra -std=c17

# Archivos fuente de tu proyecto
FUENTES = main.c parrilla/anticucho.c

# Nombre del archivo ejecutable que queremos crear
EJECUTABLE = planificador

# Regla principal: cómo compilar
$(EJECUTABLE): $(FUENTES)
	$(CC) $(CFLAGS) $(FUENTES) -o $(EJECUTABLE)

# Regla para limpiar (borrar) el ejecutable generado
clean:
	rm -f $(EJECUTABLE)