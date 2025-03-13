# Variables
CC = gcc
TARGET = build
SRC ?= Fuentes_Practica1/matrices.c
N ?= 16
AUX ?=

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
	@if [ -z "$(AUX)" ]; then \
		echo "============================================"; \
		echo "Ejecutando: ./$(TARGET) $(N)"; \
		echo "============================================"; \
		./$(TARGET) $(N); \
	else \
		echo "============================================"; \
		echo "Ejecutando: ./$(TARGET) $(N) $(AUX)"; \
		echo "============================================"; \
		./$(TARGET) $(N) $(AUX); \
	fi

all-run: all run clean
single-run: single run clean
double-run: double run clean

# Limpiar los archivos generados
clean:
	$(RM) $(TARGET)

.PHONY: all clean run single double all-run single-run double-run
