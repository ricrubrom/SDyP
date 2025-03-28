# Variables
CC = gcc
TARGET = build
SRC ?= Fuentes_Practica1/matrices.c
UTILS = utils/utils.c
N ?= 16
AUX ?=
T?=
CFLAGS ?= -O2 -lm  # Optimization and math library
LDFLAGS ?=  # Additional linking flags if needed

# Default compilation
all: $(TARGET)

# Target for default compilation
$(TARGET): $(UTILS) $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(UTILS) $(SRC) $(LDFLAGS)

# Target for single precision
single: clean
	$(CC) $(CFLAGS) -o $(TARGET) $(UTILS) $(SRC)

# Target for double precision
double: clean
	$(CC) $(CFLAGS) -DDOUBLE -o $(TARGET) $(UTILS) $(SRC)

# Target for pthreads
pthread: clean
	$(CC) $(CFLAGS) -pthread -o $(TARGET) $(UTILS) $(SRC)

# Target for OpenMP
openmp: clean
	$(CC) $(CFLAGS) -fopenmp -o $(TARGET) $(UTILS) $(SRC)

# Ejecutar el programa con argumentos
run:
	@if [ -z "$(AUX)" ] && [ -z "$(T)" ]; then \
		echo "============================================"; \
		echo "Ejecutando: ./$(TARGET) $(N)"; \
		echo "============================================"; \
		./$(TARGET) $(N); \
	elif [ -z "$(T)" ]; then \
		echo "============================================"; \
		echo "Ejecutando: ./$(TARGET) $(N) $(AUX)"; \
		echo "============================================"; \
		./$(TARGET) $(N) $(AUX); \
	elif [ -z "$(AUX)" ]; then \
		echo "============================================"; \
		echo "Ejecutando: ./$(TARGET) $(N) $(T)"; \
		echo "============================================"; \
		./$(TARGET) $(N) $(T); \
	else \
		echo "============================================"; \
		echo "Ejecutando: ./$(TARGET) $(N) $(T) $(AUX)"; \
		echo "============================================"; \
		./$(TARGET) $(N) $(T) $(AUX); \
	fi


# Ejecutar con diferentes compilaciones
all-run: all run clean
single-run: single run clean
double-run: double run clean
pthread-run: pthread run clean
openmp-run: openmp run clean

# Limpiar los archivos generados
clean:
	$(RM) $(TARGET)

.PHONY: all clean run single double pthread openmp all-run single-run double-run pthread-run openmp-run
