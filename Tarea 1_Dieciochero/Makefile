CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -Irecetas
LDLIBS = -lpthread

SRCS = main.c parrilla/anticucho.c parrilla/empanada_pino.c parrilla/choripan.c
OBJS = $(SRCS:.c=.o)
HDRS = $(wildcard recetas/*.h)
TARGET = planificador

# compilar todo el proyecto
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# atajo para probar con el plan.txt y K=3
run: $(TARGET)
	./$(TARGET) plan.txt 3

# borra los .o y el ejecutable
clean:
	rm -f parrilla/*.o *.o $(TARGET)

.PHONY: all run clean