// Ejercicio 2
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

int main(int argc, char *argv[])
{
	double x, scale;
	int i;
	int numThreads = atoi(argv[2]);
	int N = atoi(argv[1]);
	omp_set_num_threads(numThreads);
	scale = 2.78;
	x = 0.0;

// Se estan realizando operaciones matematicas en paralelo, por lo que el resultado puede variar
// Ademas, como es no deterministico, el valor de x puede variar de distintas formas, se puede solucionar poniendo ordered al for
#pragma omp parallel for shared(x) private(i) ordered
	for (i = 1; i <= N; i++)
	{
		printf("Thread %d: %d\n", omp_get_thread_num(), i);

#pragma omp atomic
		x += sqrt(i * scale) + 2 * x;
	}

	printf("\n Resultado: %f \n", x);

	return (0);
}
