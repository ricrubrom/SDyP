# Trabajo Practico 1

## Ejercicio 1

### A)

Analizar el algoritmo matrices.c que resuelve la multiplicación de matrices
C=AB donde A, B y C son matrices cuadradas de $N*N$:

#### i)

Comparar el tiempo de ejecución original con el tiempo de eliminar las funciones getValor y setValor. ¿Son necesarias estas funciones?¿Qué puede decirse del overhead introducido por los llamados a estas funciones?

- El overhead introducido por estas funciones es muy grande, pues para una matriz de $1024 * 1024$ este programa se demoro mas de 16 segundos menos

#### ii)

Partiendo del código sin las funciones getValor y setValor, comparar la
solución que almacena A, B y C por filas con las solución que almacena
B por columnas. ¿Qué conclusión puede obtenerse de la diferencia en
los tiempos de ejecución? ¿Cómo se relaciona con el principio de
localidad?

- La ejecucion se demora mas cuando estan ordenadas por columnas. Esto se debe a que al hacer el producto de las matrices, A y C se recorren por filas, pero al estar ordenadas por columna, no se cumple el principio de localidad (?), lo cual genera muchos fallos de cache. Lo ideal seria que A y C

### B)

Analizar los algoritmos que resuelven distintas operaciones sobre matrices de $N*N$:

- expMatrices1.c: dos versiones que resuelven la operación AB + AC + AD
- expMatrices2.c: dos versiones que resuelven la operación AB + CD
- expMatrices3.c: dos versiones que resuelven la operación BA + CAD
  ------------------- |
  Ejecutar con distintos valores de N. Comparar los tiempos de ejecución de las dos
  versiones en cada caso. ¿Qué versión es más rápida? ¿Por qué?

- Para los 3 casos es mas rapida la version con menos bucles, esto se debe a que se debe iterar menos, y se aprovecha mejor el principio de localidad.

### C)

Implementar una solución a la multiplicación de matrices AA donde la matriz
A es de N\*N. Plantear dos estrategias:

1. Ambas matrices A están ordenadas en memoria por el mismo criterio (filas o columnas) .
2. La matriz A de la izquierda sigue un criterio de ordenación en memoria (por ej: filas). La matriz A de la derecha se construye (por ej: ordenada por columnas) a partir de modificar el criterio de ordenación en memoria de la matriz A izquierda. Se debe tener en cuenta el tiempo de construcción de la nueva matriz.

¿Cuál de las dos estrategias alcanza mejor rendimiento?

- La estrategia que alcanza mejor rendimiento es la segunda, pues se aprovecha mejor el principio de localidad, reduciendo asi los fallos de cache.

### D)

Describir brevemente cómo funciona el algoritmo multBloques.c que resuelve la multiplicación de matrices cuadradas de $N*N$ utilizando la técnica por bloques de tamaño $BS*BS$. Ejecutar el algoritmo utilizando distintos tamaños de matrices y distintos tamaños de bloques, comparar los tiempos con respecto a la multiplicación de matrices optimizada del ejercicio 1a. Según el tamaño de las matrices y del bloque elegido ¿Cuál es más rápido? ¿Por qué? ¿Cuál sería el tamaño de bloque óptimo para un determinado tamaño de matriz?

- Para una matriz con N=1024, los tiempos de ejecucion fueron los siguientes:

  |  BS  | Tiempo (sg) |
  | :--: | :---------: | ------------------- |
  |  1   |    7.53     | ------------------- |
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

### E) (Ejecutado en Escritorio)

Implementar las soluciones para la multiplicación de matrices MU, ML, UM y LM. Donde M es una matriz de N\*N. L y U son matrices triangulares inferior y superior, respectivamente. Analizar los tiempos de ejecución para la solución que almacena los ceros de las triangulares respecto a la que no los almacena.

- Los tiempos obtenidos para una matriz con N=1024 son los siguientes:

| Operacion | Tiempo con ceros (sg) | Tiempo sin ceros(sg) |
| :-------: | :-------------------: | :------------------: |
|    MU     |         2.33          |         1.17         |
|    ML     |         2.32          |         1.17         |
|    UM     |         2.33          |         1.17         |
|    LM     |         2.33          |         1.17         |

## Ejercicio 2 (Ejecutado en Escritorio)

El algoritmo fib.c resuelve la serie de Fibonacci para un numero N dado utilizando dos métodos, el método recursivo y el método iterativo. Analizar los tiempos de ambos
métodos ¿Cuál es más rápido? ¿Por qué?

- Los resultados obtenidos son:

  |  N  | Tiempo recursivo (sg) | Tiempo iterativo (sg) |
  | :-: | :-------------------: | :-------------------: |
  | 35  |         0.02          |         0.00          |
  | 40  |          0.2          |         0.00          |
  | 45  |         2.21          |         0.00          |
  | 50  |         24.49         |         0.00          |

  El metodo mas rapido es el iterativo. Esto se debe a que en el iterativo solo debe realizar un for loop hasta n, lo cual es rapido, en cambio en el recursivo, el overhead introducido por el llamado a la funcion es muy grande, demorando mucho la ejecucion.

  PD: los tiempos de ejecucion del recursivo son en realidad mucho mayores (N=50 fue mucho mas cercano a los 2 minutos).

## Ejercicio 3 (Ejecutado en Escritorio)

Analizar los algoritmos productoVectorialRegistro.c y productoVectorialSinRegistro.c. Ambos programas parten de dos conjuntos de N vectores matemáticos y realizan el producto vectorial uno a uno de acuerdo al orden de cada vector en el conjunto. ¿Cuál de las dos soluciones es más rápida? ¿Por qué?

- Utilizando un $N=2^{28}$ obtengo que el algoritmo productoVectorialRegistro.c se demora 1.39sg, mientras que productoVectorialSinRegistro.c se demora 0.93sg, por ende es mas rapido el segundo algoritmo. Esto se debe a que el acceso a memoria es mas directo, pues en vez de tener que acceder al registro y luego al valor deseado, se puede acceder directamente a este. (??)

## Ejercicio 4 (Ejecutado en Escritorio)

### A)

El algoritmo instrucciones1.c compara el tiempo de ejecución de las operaciones básicas suma (+), resta (-), multiplicación (\*) y división (/), aplicadas sobre los elementos que se encuentran en la misma posición de dos vectores x e y. ¿Qué análisis se puede hacer de cada operación? ¿Qué ocurre si los valores de los vectores x e y son potencias de 2?

- Los tiempos obtenidos al hacer estas operaciones para N=8192 y r=4096 son:

  | Operacion | Tiempo total (sg) | Tiempo promedio (sg) |
  | :-------: | :---------------: | :------------------: |
  |    \+     |      0.02493      |       0.01246        |
  |    \-     |      0.02505      |       0.01252        |
  |     /     |      0.02493      |       0.01247        |
  |    \*     |      0.02506      |       0.01253        |

  Se puede observar que todas estas operacion son basicamente igual de rapidas. (No se que responder en la segunda pregunta) (??)

### B)

En función del ejercicio anterior analizar el algoritmo instrucciones2.c que aplica dos operaciones distintas a cada elemento de un vector x.

- Los tiempos obtenidos al hacer estas operaciones para N=8192 y r=4096 son:

  | Operacion | Tiempo total (sg) | Tiempo promedio (sg) |
  | :-------: | :---------------: | :------------------: |
  |     /     |     0.0242130     |      0.0000029       |
  |    \*     |     0.0204448     |      0.0000025       |

  (No se que quiere que responda) (??)

### C)

El algoritmo modulo.c compara el tiempo de ejecución de dos versiones para obtener el resto de un cociente m (m potencia de 2) de los elementos enteros de un vector de tamaño N. ¿Qué análisis se puede hacer de las dos versiones?

- Los tiempos obtenidos para N=8192 y m=4096 son:

  |    Metodo    | Tiempo total (sg) |
  | :----------: | :---------------: |
  |      %       |     0.000017      |
  | Equivalencia |     0.000010      |

  Se puede observar que el metodo de Equivalencia es mas rapido que el de % (??)

## Ejercicio 5

