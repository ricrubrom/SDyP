# Variables
CC = gcc
# CFLAGS = -fopenmp -Wall -O2
# CFLAGS = -pthread 
TARGET = build
SRC ?= Fuentes_Practica1/matrices.c
N ?= 16
BS ?= -1

# Compilar el programa
all: $(TARGET)

# Target for default compilation
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Target for single precision
single: clean
	$(CC) -O2 -lm -o $(TARGET) $(SRC)

# Target for double precision
double: clean
	$(CC) -O2 -DDOUBLE -lm -o $(TARGET) $(SRC)

# Ejecutar el programa con argumentos
run:
	@echo "Ejecutando con N=$(N), SRC=$(SRC), BS=$(BS)"
	@if [ -z "$(BS)" ] || [ "$(BS)" = "-1" ]; then \
		./$(TARGET) $(N); \
	else \
		./$(TARGET) $(N) $(BS); \
	fi

all-run: all run clean
single-run: single run
double-run: double run

# Limpiar los archivos generados
clean:
	$(RM) $(TARGET)

.PHONY: all clean run single double all-run single-run double-run
