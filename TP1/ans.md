## A

Analizar el algoritmo matrices.c que resuelve la multiplicación de matrices
C=AB donde A, B y C son matrices cuadradas de $N*N$:

### i)

Comparar el tiempo de ejecución original con el tiempo de eliminar las funciones getValor y setValor. ¿Son necesarias estas funciones?¿Qué puede decirse del overhead introducido por los llamados a estas funciones?

- El overhead introducido por estas funciones es muy grande, pues para una matriz de $1024 * 1024$ este programa se demoro mas de 16 segundos menos

### ii)

Partiendo del código sin las funciones getValor y setValor, comparar la
solución que almacena A, B y C por filas con las solución que almacena
B por columnas. ¿Qué conclusión puede obtenerse de la diferencia en
los tiempos de ejecución? ¿Cómo se relaciona con el principio de
localidad?

- La ejecucion se demora mas cuando estan ordenadas por columnas. Esto se debe a que al hacer el producto de las matrices, A y C se recorren por filas, pero al estar ordenadas por columna, no se cumple el principio de localidad (?), lo cual genera muchos fallos de cache. Lo ideal seria que A y C

## B

Analizar los algoritmos que resuelven distintas operaciones sobre matrices de $N*N$:

- expMatrices1.c: dos versiones que resuelven la operación AB + AC + AD
- expMatrices2.c: dos versiones que resuelven la operación AB + CD
- expMatrices3.c: dos versiones que resuelven la operación BA + CAD

Ejecutar con distintos valores de N. Comparar los tiempos de ejecución de las dos
versiones en cada caso. ¿Qué versión es más rápida? ¿Por qué?

- Para los 3 casos es mas rapida la version con menos bucles, esto se debe a que se debe iterar menos, y se aprovecha mejor el principio de localidad.

## C

Implementar una solución a la multiplicación de matrices AA donde la matriz
A es de N\*N. Plantear dos estrategias:

1. Ambas matrices A están ordenadas en memoria por el mismo criterio (filas o columnas) .
2. La matriz A de la izquierda sigue un criterio de ordenación en memoria (por ej: filas). La matriz A de la derecha se construye (por ej: ordenada por columnas) a partir de modificar el criterio de ordenación en memoria de la matriz A izquierda. Se debe tener en cuenta el tiempo de construcción de la nueva matriz.

¿Cuál de las dos estrategias alcanza mejor rendimiento?

- La estrategia que alcanza mejor rendimiento es la segunda, pues se aprovecha mejor el principio de localidad, reduciendo asi los fallos de cache.

## D

Describir brevemente cómo funciona el algoritmo multBloques.c que resuelve la multiplicación de matrices cuadradas de $N*N$ utilizando la técnica por bloques de tamaño $BS*BS$. Ejecutar el algoritmo utilizando distintos tamaños de matrices y distintos tamaños de bloques, comparar los tiempos con respecto a la multiplicación de matrices optimizada del ejercicio 1a. Según el tamaño de las matrices y del bloque elegido ¿Cuál es más rápido? ¿Por qué? ¿Cuál sería el tamaño de bloque óptimo para un determinado tamaño de matriz?

- Para una matriz con N=1024, los tiempos de ejecucion fueron los siguientes:

  |  BS  | Tiempo (sg) |
  | :--: | :---------: |
  |  1   |    7.53     |
  |  2   |    4.98     |
  |  4   |    4.31     |
  |  8   |    4.11     |
  |  16  |    3.93     |
  |  32  |    4.02     |
  |  64  |    3.79     |
  | 128  |    3.97     |
  | 256  |    3.81     |
  | 512  |    4.16     |
  | 1024 |    4.06     |

## E

Implementar las soluciones para la multiplicación de matrices MU, ML, UM y LM. Donde M es una matriz de N\*N. L y U son matrices triangulares inferior y superior, respectivamente. Analizar los tiempos de ejecución para la solución que almacena los ceros de las triangulares respecto a la que no los almacena.

- Los tiempos obtenidos para una matriz con N=1024 son los siguientes (No se como cambiar el check, si es que siquiera deberia tenerlo):

| Operacion | Tiempo con ceros (sg) | Tiempo sin ceros(sg) |
| :-------: | :-------------------: | :------------------: | ------------------- |
|    MU     |         11.30         |         1.76         |
|    ML     |         10.37         |         1.92         |
|    UM     |         11.33         |         4.25         | (Preguntar despues) |
|    LM     |         12.40         |         3.10         |

