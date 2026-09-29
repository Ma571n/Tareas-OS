CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -Irecetas

SRCS = main.c parrilla/anticucho.c
OBJS = $(SRCS:.c=.o)
TARGET = planificador

# compilar todo el proyecto
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# atajo pa probar de una con el plan.txt y K=3
run: $(TARGET)
	./$(TARGET) plan.txt 3

# borro los .o y el ejecutable pa dejar limpia la carpeta
clean:
	rm -f parrilla/*.o *.o $(TARGET)

.PHONY: all run clean