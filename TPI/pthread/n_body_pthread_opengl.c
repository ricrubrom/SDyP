// Compilar:
//		gcc -o n_body_simple_NOGL n_body_simple.c -lm
// Ejecutar:
//		./n_body_simple_NOGL <nro de cuerpos> <DT> <Pasos>

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include "../utils/utils.h"

double tIni, tFin, tTotal;

//
// Constantes para OpenGL
//
#define KEY_ESC 27
#define ANCHO 1920
#define ALTO 1080
#define false 0
#define true 1

//
// Constantes para Algoritmo de gravitacion
//
#define PI (3.141592653589793)
#define G 6.673e-11
#define ESTRELLA 0
#define POLVO 1
#define H2 2 // Hidrogeno molecular

// ===============
// ===== CPU =====
// ===============

//
// Estructuras y variables para Algoritmo de gravitacion
//
typedef struct cuerpo cuerpo_t;
struct cuerpo
{
  double masa;
  double px;
  double py;
  double pz;
  double vx;
  double vy;
  double vz;
  double r;
  double g;
  double b;
  int cuerpo;
};

double *fuerza_totalX, *fuerza_totalY, *fuerza_totalZ;
double *fuerza_localX, *fuerza_localY, *fuerza_localZ;
float toroide_alfa;
float toroide_theta;
float toroide_incremento;
float toroide_lado;
float toroide_r;
float toroide_R;

cuerpo_t *cuerpos;
float delta_tiempo = 1.0f; // Intervalo de tiempo, longitud de un paso
int pasos;
int N;
int T;

pthread_barrier_t barrier;
pthread_mutex_t mutex;

//
// Funciones para Algoritmo de gravitacion
//

void calcularFuerzas(cuerpo_t *cuerpos, int N, int dt, int idt)
{
  int cuerpo1, cuerpo2;
  float dif_X, dif_Y, dif_Z;
  float distancia;
  double F;

  for (cuerpo1 = idt; cuerpo1 < N - 1; cuerpo1 += T)
  {
    int idc1 = idt * N + cuerpo1;
    for (cuerpo2 = cuerpo1 + 1; cuerpo2 < N; cuerpo2++)
    {
      int idc2 = idt * N + cuerpo2;
      if ((cuerpos[cuerpo1].px == cuerpos[cuerpo2].px) && (cuerpos[cuerpo1].py == cuerpos[cuerpo2].py) && (cuerpos[cuerpo1].pz == cuerpos[cuerpo2].pz))
        continue;

      dif_X = cuerpos[cuerpo2].px - cuerpos[cuerpo1].px;
      dif_Y = cuerpos[cuerpo2].py - cuerpos[cuerpo1].py;
      dif_Z = cuerpos[cuerpo2].pz - cuerpos[cuerpo1].pz;

      distancia = sqrt(dif_X * dif_X + dif_Y * dif_Y + dif_Z * dif_Z);

      F = (G * cuerpos[cuerpo1].masa * cuerpos[cuerpo2].masa) / (distancia * distancia);

      dif_X *= F;
      dif_Y *= F;
      dif_Z *= F;

      fuerza_localX[idc1] += dif_X;
      fuerza_localY[idc1] += dif_Y;
      fuerza_localZ[idc1] += dif_Z;

      fuerza_localX[idc2] -= dif_X;
      fuerza_localY[idc2] -= dif_Y;
      fuerza_localZ[idc2] -= dif_Z;
    }
  }
}

void moverCuerpos(cuerpo_t *cuerpos, int N, int dt, int idt)
{
  // Acumular fuerzas locales en un temporal
  double fx = 0.0, fy = 0.0, fz = 0.0;
  for (int cuerpo = idt; cuerpo < N; cuerpo += T)
  {
    for (int k = 0; k < T; k++)
    {
      fx += fuerza_localX[k * N + cuerpo];
      fy += fuerza_localY[k * N + cuerpo];
      fz += fuerza_localZ[k * N + cuerpo];
      fuerza_localX[k * N + cuerpo] = 0.0; // Limpiar fuerzas locales
      fuerza_localY[k * N + cuerpo] = 0.0;
      fuerza_localZ[k * N + cuerpo] = 0.0;
    }

    // Calcular aceleración (F = ma)
    double inv_masa = (cuerpos[cuerpo].masa > 0.0f) ? 1.0f / cuerpos[cuerpo].masa : 0.0f;
    double ax = fx * inv_masa;
    double ay = fy * inv_masa;
    double az = fz * inv_masa;

    // float ax = fx / cuerpos[cuerpo].masa;
    // float ay = fy / cuerpos[cuerpo].masa;
    // float az = fz / cuerpos[cuerpo].masa;

    // Actualizar velocidad y posición
    cuerpos[cuerpo].vx += ax * dt;
    cuerpos[cuerpo].vy += ay * dt;
    cuerpos[cuerpo].vz += az * dt;
    cuerpos[cuerpo].px += cuerpos[cuerpo].vx * dt;
    cuerpos[cuerpo].py += cuerpos[cuerpo].vy * dt;
    cuerpos[cuerpo].pz += cuerpos[cuerpo].vz * dt;

    fx = 0.0;
    fy = 0.0;
    fz = 0.0;
  }
}

void gravitacionCPU(cuerpo_t *cuerpos, int N, int dt, int idt)
{
  calcularFuerzas(cuerpos, N, dt, idt);
  pthread_barrier_wait(&barrier);
  moverCuerpos(cuerpos, N, dt, idt);
  pthread_barrier_wait(&barrier);
}

void inicializarEstrella(cuerpo_t *cuerpo, int i, float n)
{

  cuerpo->masa = 0.001 * 8;

  if ((toroide_alfa + toroide_incremento) >= 2 * M_PI)
  {
    toroide_alfa = 0;
    toroide_theta += toroide_incremento;
  }
  else
  {
    toroide_alfa += toroide_incremento;
  }

  cuerpo->px = (toroide_R + toroide_r * cos(toroide_alfa)) * cos(toroide_theta);
  cuerpo->py = (toroide_R + toroide_r * cos(toroide_alfa)) * sin(toroide_theta);
  cuerpo->pz = toroide_r * sin(toroide_alfa);

  cuerpo->vx = 0.0;
  cuerpo->vy = 0.0;
  cuerpo->vz = 0.0;

  cuerpo->r = 1.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->g = 1.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->b = 1.0; //(float )rand()/(RAND_MAX+1.0);
}

void inicializarPolvo(cuerpo_t *cuerpo, int i, float n)
{

  cuerpo->masa = 0.001 * 4;

  if ((toroide_alfa + toroide_incremento) >= 2 * M_PI)
  {
    toroide_alfa = 0;
    toroide_theta += toroide_incremento;
  }
  else
  {
    toroide_alfa += toroide_incremento;
  }

  cuerpo->px = (toroide_R + toroide_r * cos(toroide_alfa)) * cos(toroide_theta);
  cuerpo->py = (toroide_R + toroide_r * cos(toroide_alfa)) * sin(toroide_theta);
  cuerpo->pz = toroide_r * sin(toroide_alfa);

  cuerpo->vx = 0.0;
  cuerpo->vy = 0.0;
  cuerpo->vz = 0.0;

  cuerpo->r = 1.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->g = 0.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->b = 0.0; //(float )rand()/(RAND_MAX+1.0);
}

void inicializarH2(cuerpo_t *cuerpo, int i, float n)
{

  cuerpo->masa = 0.001;

  if ((toroide_alfa + toroide_incremento) >= 2 * M_PI)
  {
    toroide_alfa = 0;
    toroide_theta += toroide_incremento;
  }
  else
  {
    toroide_alfa += toroide_incremento;
  }

  cuerpo->px = (toroide_R + toroide_r * cos(toroide_alfa)) * cos(toroide_theta);
  cuerpo->py = (toroide_R + toroide_r * cos(toroide_alfa)) * sin(toroide_theta);
  cuerpo->pz = toroide_r * sin(toroide_alfa);

  cuerpo->vx = 0.0;
  cuerpo->vy = 0.0;
  cuerpo->vz = 0.0;

  cuerpo->r = 1.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->g = 1.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->b = 1.0; //(float )rand()/(RAND_MAX+1.0);
}

void inicializarCuerpos(cuerpo_t *cuerpos, int N)
{
  int cuerpo;
  float n = N;

  toroide_alfa = 0.0;
  toroide_theta = 0.0;
  toroide_lado = sqrt(N);
  toroide_incremento = 2 * M_PI / toroide_lado;
  toroide_r = 1.0;
  toroide_R = 2 * toroide_r;

  srand(3);

  for (cuerpo = 0; cuerpo < N; cuerpo++)
  {

    fuerza_totalX[cuerpo] = 0.0;
    fuerza_totalY[cuerpo] = 0.0;
    fuerza_totalZ[cuerpo] = 0.0;

    cuerpos[cuerpo].cuerpo = (rand() % 3);

    if (cuerpos[cuerpo].cuerpo == ESTRELLA)
    {
      inicializarEstrella(&cuerpos[cuerpo], cuerpo, n);
    }
    else if (cuerpos[cuerpo].cuerpo == POLVO)
    {
      inicializarPolvo(&cuerpos[cuerpo], cuerpo, n);
    }
    else if (cuerpos[cuerpo].cuerpo == H2)
    {
      inicializarH2(&cuerpos[cuerpo], cuerpo, n);
    }
  }

  cuerpos[0].masa = 2.0e2;
  cuerpos[0].px = 0.0;
  cuerpos[0].py = 0.0;
  cuerpos[0].pz = 0.0;
  cuerpos[0].vx = -0.000001;
  cuerpos[0].vy = -0.000001;
  cuerpos[0].vz = 0.0;

  cuerpos[1].masa = 1.0e1;
  cuerpos[1].px = -1.0;
  cuerpos[1].py = 0.0;
  cuerpos[1].pz = 0.0;
  cuerpos[1].vx = 0.0;
  cuerpos[1].vy = 0.0001;
  cuerpos[1].vz = 0.0;
}

void finalizar(void)
{
  free(cuerpos);
  free(fuerza_totalX);
  free(fuerza_totalY);
  free(fuerza_totalZ);
  free(fuerza_localX);
  free(fuerza_localY);
  free(fuerza_localZ);
  pthread_barrier_destroy(&barrier);
}

void *thread(void *arg)
{
  int idt = *(int *)arg;
  int paso;
  for (paso = 0; paso < pasos; paso++)
  {
    gravitacionCPU(cuerpos, N, delta_tiempo, idt);
  }

  pthread_exit(NULL);
}

// ==================
// ===== OpenGL =====
// ==================

//
// Variables OpenGL
//
double alfa = 0.0;

// Para angulo de rotacion y direccion de la camara
float angle = 0.0;
float camAngleX = 0;
float camAngleY = 0;
float distancia = 10;
int ejes = 1;

// Vector actual que representa la direccion de la camara
float lx = 0.0f, lz = -1.0f;
// posicion XZ de la camara
float x = 0.0f, z = 5.0f;

int oldX = 0, oldY = 0;
int rotate = false;

//
// Funciones OpenGL
//

// Funcion que se llama cada vez que se quiere dibujar nuevamente en la pantalla
// Se llama cada vez que se produce el evento render
void GL_camara()
{
  float camX, camY, camZ;

  // Camara mirando al origen (pickObjX,pickObjY,pickObjZ) = (0,0,0)
  float pickObjX = 0.0;
  float pickObjY = 0.0;
  float pickObjZ = 0.0;

  camX = distancia * sin(camAngleX);
  camY = distancia * sin(camAngleY);
  camZ = distancia * cos(camAngleY) * cos(camAngleX);

  // Ubicar la camara
  gluLookAt(camX, camY, camZ,             // Posicion de la camara
            pickObjX, pickObjY, pickObjZ, // Mirando al punto
            0.0, 1.0, 0.0);               // Up vector
}

void GL_dibujarCuerpos(void)
{
  int i;

  for (i = 0; i < N; i++)
  {
    glPushMatrix();
    glTranslatef(cuerpos[i].px, cuerpos[i].py, cuerpos[i].pz);
    glColor3f(cuerpos[i].r, cuerpos[i].g, cuerpos[i].b);

    if (cuerpos[i].cuerpo == ESTRELLA)
    {
      glutSolidSphere(0.02, 20, 20);
    }
    else if (cuerpos[i].cuerpo == H2)
    {
      glutSolidSphere(0.005, 20, 20);
    }
    else
    {
      glutSolidSphere(0.01, 20, 20);
    }
    glPopMatrix();
  }

  // Aca se Llama a la funcion que calcula un paso del algoritmo
  tIni = dwalltime();
  // gravitacionCPU(cuerpos, N, delta_tiempo);
  tFin = dwalltime();
  tTotal += (tFin - tIni);

  pasos--;
  if (pasos == 0)
  {
    printf("Tiempo en segundos: %f\n", tTotal);
    finalizar();
    exit(0);
  }
}

void GL_dibujar(void)
{
  // Borra el color y los buffers de profundidad
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Reiniciar la matriz de transformaciones
  glLoadIdentity();

  // ubica la camara
  GL_camara();

  // Dibuja los ejes de coordenadas (si estan habilitados)
  if (ejes)
  {
    glBegin(GL_LINES);
    glColor3f(1.0, 0.0, 0.0);
    glVertex3d(0, 0, 0);
    glVertex3d(5.0, 0.0, 0.0);

    glColor3f(0.0, 1.0, 0.0);
    glVertex3d(0, 0, 0);
    glVertex3d(0.0, 5.0, 0.0);

    glColor3f(0.0, 0.0, 1.0);
    glVertex3d(0, 0, 0);
    glVertex3d(0, 0, 5.0);

    glEnd();
  }

  // Dibuja
  glPushMatrix();
  GL_dibujarCuerpos();
  glPopMatrix();

  glutSwapBuffers();
}

void GL_cambioDeDimensionDeVentana(int w, int h)
{

  // Evita que se divida por cero cuando la ventana es muy chica
  if (h == 0)
    h = 1;
  float ratio = w * 1.0 / h;

  // Usa la matriz de proyecion
  glMatrixMode(GL_PROJECTION);

  // Reset matriz
  glLoadIdentity();

  // Configura el viewport para la ventana completa
  glViewport(0, 0, w, h);

  // Configura la perspectiva correcta
  gluPerspective(45.0f, ratio, 0.1f, 100.0f);

  // Modelview
  glMatrixMode(GL_MODELVIEW);
}

// Funcion de inicializacion
void GL_inicio(void)
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glOrtho(-10, 10, -10, 10, -10, 10);
}

void GL_teclado(unsigned char key, int x, int y)
{
  double denominador = 50.0;
  double grados = PI / denominador;
  switch (key)
  {
  case 'a':
    if (alfa + grados >= 2 * PI)
      alfa = (alfa + grados) - 2 * PI;
    else
      alfa += grados;
    break;
  case '+':
    distancia--;
    break;
  case '-':
    distancia++;
    break;
  case 'e':
    if (ejes == 1)
    {
      ejes = 0;
    }
    else
    {
      ejes = 1;
    }
    break;
  case KEY_ESC:
    finalizar();
    exit(0); // Sale de la aplicacion si se presiona 'Esc'
  }
  glutPostRedisplay();
}

void GL_teclasEspeciales(int key, int x, int y)
{
  double denominador = 50.0;
  double grados = PI / denominador;

  switch (key)
  {
  case GLUT_KEY_RIGHT:
    if (camAngleX - grados < 0)
      camAngleX = (camAngleX - grados) + 2 * PI;
    else
      camAngleX -= grados;
    break;
  case GLUT_KEY_LEFT:
    if (camAngleX + grados >= 2 * PI)
      camAngleX = (camAngleX + grados) - 2 * PI;
    else
      camAngleX += grados;
    break;
  case GLUT_KEY_UP:
    if (camAngleY - grados <= -PI / 2)
      camAngleY = -PI / 2 + 0.001;
    else
      camAngleY -= grados;
    break;
  case GLUT_KEY_DOWN:
    if (camAngleY + grados >= PI / 2)
      camAngleY = PI / 2 - 0.001;
    else
      camAngleY += grados;
    break;
  }

  glutPostRedisplay();
}

void GL_OnMouseDown(int button, int state, int x, int y)
{
  rotate = false;
  if (button == GLUT_LEFT_BUTTON)
  {
    oldX = x;
    oldY = y;
    rotate = true;
  }
}

void GL_OnMouseMove(int x, int y)
{

  if (rotate)
  {
    camAngleX -= (x - oldX) * 0.01f;
    camAngleY += (y - oldY) * 0.01f;
  }

  oldX = x;
  oldY = y;
  glutPostRedisplay();
}

void procesoOpenGL(int argc, char *argv[])
{
  // Inicializa la libreria glut
  glutInit(&argc, argv);
  // Se va a usar doble buffer, paleta RGB
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  // Define la ventana de visualizacion
  glutInitWindowSize(ANCHO, ALTO);

  // Posicionar la ventana
  glutInitWindowPosition(0, 0);
  // Se crea la ventana cuyo nombre en la barra de titulo es lo que viene en argv[0]
  glutCreateWindow(argv[0]);

  // Funcion personalizada que inicializa parametros
  GL_inicio();

  // Define cual es la funcion de control de renderizado
  //  Se llama cada vez que se quiere dibujar nuevamente en la pantalla (cada vez que se produce el evento render)
  glutDisplayFunc(GL_dibujar);
  glutReshapeFunc(GL_cambioDeDimensionDeVentana);
  glutIdleFunc(GL_dibujar);

  // Define cuales son las funciones que atenderan los eventos del teclado
  glutKeyboardFunc(GL_teclado);
  glutSpecialFunc(GL_teclasEspeciales);

  // Define cuales son las funciones que atenderan los eventos del mouse
  glutMouseFunc(GL_OnMouseDown);
  glutMotionFunc(GL_OnMouseMove);

  // El programa espera aca
  glutMainLoop();
}

int main(int argc, char *argv[])
{

  if (argc < 5)
  {
    printf("Ejecutar: %s <nro. de cuerpos> <DT> <pasos> <Threads>\n", argv[0]);
    return -1;
  }

  N = atoi(argv[1]);
  delta_tiempo = atof(argv[2]);
  pasos = atoi(argv[3]);

  T = atoi(argv[4]);
  pthread_t threads[T];
  int threads_ids[T];

  pthread_barrier_init(&barrier, NULL, T);
  pthread_mutex_init(&mutex, NULL);

  cuerpos = (cuerpo_t *)malloc(sizeof(cuerpo_t) * N);
  fuerza_totalX = (double *)malloc(sizeof(double) * N);
  fuerza_totalY = (double *)malloc(sizeof(double) * N);
  fuerza_totalZ = (double *)malloc(sizeof(double) * N);

  fuerza_localX = (double *)calloc(N * T, sizeof(double));
  fuerza_localY = (double *)calloc(N * T, sizeof(double));
  fuerza_localZ = (double *)calloc(N * T, sizeof(double));

  inicializarCuerpos(cuerpos, N);

  tIni = dwalltime();
  int i;
  for (i = 0; i < T; i++)
  {
    threads_ids[i] = i;
    pthread_create(&threads[i], NULL, &thread, (void *)&threads_ids[i]);
  }

  // Proceso OpenGL
  procesoOpenGL(argc, argv);

  for (i = 0; i < T; i++)
  {
    pthread_join(threads[i], NULL);
  }

  tFin = dwalltime();
  tTotal = tFin - tIni;

  printf("VALORES FINALES:\n");
  for (int i = 0; i < N; i++)
  {
    printf("Cuerpo %d: px=%.15f, py=%.15f, pz=%.15f\n",
           i,
           cuerpos[i].px,
           cuerpos[i].py,
           cuerpos[i].pz);
  }
  printf("\n\n\n");

  printf("Tiempo en segundos: %.15f\n", tTotal);

  finalizar();
  return (0);
}