# Variables
CC = gcc
TARGET = build
SRC ?= Fuentes_Practica1/matrices.c
UTILS = utils/utils.c
N ?= 16
AUX ?=
T ?=
NP ?= 4  # Cantidad de procesos para MPI

CFLAGS ?= -O3 -lm
LDFLAGS ?=

# Default compilation
all: $(TARGET)

$(TARGET): $(UTILS) $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(UTILS) $(SRC) $(LDFLAGS)

single: clean
	$(CC) $(CFLAGS) -o $(TARGET) $(UTILS) $(SRC)

double: clean
	$(CC) $(CFLAGS) -DDOUBLE -o $(TARGET) $(UTILS) $(SRC)

pthread: clean
	$(CC) $(CFLAGS) -pthread -o $(TARGET) $(UTILS) $(SRC)

openmp: clean
	$(CC) $(CFLAGS) -fopenmp -o $(TARGET) $(UTILS) $(SRC)

mpi: clean
	mpicc $(CFLAGS) -DMPI -o $(TARGET) $(UTILS) $(SRC)

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

mpi-runCode: mpi
	@if [ -z "$(AUX)" ] && [ -z "$(T)" ]; then \
		echo "============================================"; \
		echo "Ejecutando con MPI: mpirun -np $(NP) ./$(TARGET) $(N)"; \
		echo "============================================"; \
		mpirun -np $(NP) ./$(TARGET) $(N); \
	elif [ -z "$(T)" ]; then \
		echo "============================================"; \
		echo "Ejecutando con MPI: mpirun -np $(NP) ./$(TARGET) $(N) $(AUX)"; \
		echo "============================================"; \
		mpirun -np $(NP) ./$(TARGET) $(N) $(AUX); \
	elif [ -z "$(AUX)" ]; then \
		echo "============================================"; \
		echo "Ejecutando con MPI: mpirun -np $(NP) ./$(TARGET) $(N) $(T)"; \
		echo "============================================"; \
		mpirun -np $(NP) ./$(TARGET) $(N) $(T); \
	else \
		echo "============================================"; \
		echo "Ejecutando con MPI: mpirun -np $(NP) ./$(TARGET) $(N) $(T) $(AUX)"; \
		echo "============================================"; \
		mpirun -np $(NP) ./$(TARGET) $(N) $(T) $(AUX); \
	fi

all-run: all run clean
single-run: single run clean
double-run: double run clean
pthread-run: pthread run clean
openmp-run: openmp run clean
mpi-run: mpi-runCode clean

clean:
	$(RM) $(TARGET)

.PHONY: all clean run single double pthread openmp mpi all-run single-run double-run pthread-run openmp-run mpi-run mpi-run-clean
