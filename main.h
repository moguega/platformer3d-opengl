/* ============================================================================
 *  main.h  -  Platformer 3D Low Poly (OpenGL legado / FFP + freeglut)
 *
 *  Contem: includes, #defines, biblioteca matematica 3D inline (Vec3/Mat4),
 *  structs do jogo, declaracoes "extern" dos globais e os prototipos de todas
 *  as funcoes implementadas em jogo_funcs.c.
 *
 *  Toda a logica vive em jogo_funcs.c. O main.c apenas inicializa o GLUT.
 * ========================================================================== */
#ifndef MAIN_H
#define MAIN_H
#define ESTADO_ESCOLHA_RECOMPENSA 4

#ifdef _WIN32
#include <windows.h>   /* precisa vir antes de gl.h no Windows (APIENTRY) */
#endif

#include <GL/glut.h>   /* freeglut: ja inclui gl.h e glu.h                 */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ----------------------------------------------------------------------------
 *  CONSTANTES
 * -------------------------------------------------------------------------- */
#define PI               3.14159265358979f
#define GRAUS_PARA_RAD   (PI / 180.0f)
#define RAD_PARA_GRAUS   (180.0f / PI)

/* Limites aproximados do mundo (usados em clamps de geracao). */
#define MUNDO_W          200.0f
#define MUNDO_H          100.0f
#define MUNDO_D          200.0f

/* Estados de jogo. */
#define ESTADO_MENU      0
#define ESTADO_JOGANDO   1
#define ESTADO_VITORIA   2
#define ESTADO_GAME_OVER 3

/* Conteudo do nivel. */
#define NUM_PLATAFORMAS  33
#define NUM_ESTRELAS     16
#define PONTOS_VITORIA   1000

/* Fisica (passo fixo de ~16 ms). */
#define DT               0.016f
#define GRAVIDADE        -22.0f   /* unidades/s^2                            */
#define VEL_PULO         13.0f    /* velocidade vertical inicial do pulo     */
#define VEL_MOVIMENTO    8.0f     /* velocidade horizontal alvo              */
#define DRAG_CHAO        0.30f    /* responsividade no chao (lerp p/ alvo)   */
#define DRAG_AR          0.12f    /* responsividade no ar (mais "flutuante") */
#define FRAMES_COYOTE    6        /* ticks de coyote time apos sair da borda */
#define FRAMES_BUFFER_PULO 6      /* ticks de antecipacao do pulo            */

/* Dimensoes / tolerancias auxiliares. */
#define ESPESSURA_PLAT   0.6f
#define RAIO_JOGADOR     0.40f
#define ALTURA_JOGADOR   1.30f
#define FRAG_TEMPO       1.5f     /* segundos ate plataforma fragil sumir    */
#define TICKS_REINICIO   90       /* ~1.5 s de delay no reinicio             */
#define VIDAS_INICIAIS   3
#define QUEDA_LIMITE     -15.0f

/* ----------------------------------------------------------------------------
 *  MATH3D  -  tipos e funcoes inline (static inline => sem warning se nao usar)
 * -------------------------------------------------------------------------- */
typedef struct { GLfloat x, y, z; } Vec3;
typedef struct { GLfloat m[16];   } Mat4;   /* column-major, igual ao OpenGL */

static inline Vec3 vec3Add(Vec3 a, Vec3 b)
{ Vec3 r = { a.x + b.x, a.y + b.y, a.z + b.z }; return r; }

static inline Vec3 vec3Sub(Vec3 a, Vec3 b)
{ Vec3 r = { a.x - b.x, a.y - b.y, a.z - b.z }; return r; }

static inline Vec3 vec3Scale(Vec3 a, GLfloat s)
{ Vec3 r = { a.x * s, a.y * s, a.z * s }; return r; }

static inline GLfloat vec3Dot(Vec3 a, Vec3 b)
{ return a.x * b.x + a.y * b.y + a.z * b.z; }

static inline Vec3 vec3Cross(Vec3 a, Vec3 b)
{ Vec3 r = { a.y * b.z - a.z * b.y,
             a.z * b.x - a.x * b.z,
             a.x * b.y - a.y * b.x }; return r; }

static inline Vec3 vec3Normalize(Vec3 a)
{
    GLfloat n = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    if (n < 1e-6f) { Vec3 z = { 0.0f, 0.0f, 0.0f }; return z; }
    return vec3Scale(a, 1.0f / n);
}

static inline Vec3 vec3Lerp(Vec3 a, Vec3 b, GLfloat t)
{ Vec3 r = { a.x + (b.x - a.x) * t,
             a.y + (b.y - a.y) * t,
             a.z + (b.z - a.z) * t }; return r; }

static inline GLfloat vec3Dist(Vec3 a, Vec3 b)
{
    GLfloat dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static inline Mat4 mat4Identidade(void)
{
    Mat4 r = {{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }};
    return r;
}

/* Projecao perspectiva (mesma convencao de gluPerspective). fovy em graus. */
static inline Mat4 mat4Perspectiva(GLfloat fovy, GLfloat aspect,
                                   GLfloat znear, GLfloat zfar)
{
    Mat4 r = {{ 0 }};
    GLfloat f = 1.0f / tanf((fovy * GRAUS_PARA_RAD) * 0.5f);
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (zfar + znear) / (znear - zfar);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zfar * znear) / (znear - zfar);
    return r;
}

/* LookAt (mesma convencao de gluLookAt). */
static inline Mat4 mat4LookAt(Vec3 eye, Vec3 alvo, Vec3 up)
{
    Vec3 f = vec3Normalize(vec3Sub(alvo, eye));
    Vec3 s = vec3Normalize(vec3Cross(f, up));
    Vec3 u = vec3Cross(s, f);
    Mat4 r = mat4Identidade();
    r.m[0] = s.x; r.m[4] = s.y; r.m[8]  = s.z;
    r.m[1] = u.x; r.m[5] = u.y; r.m[9]  = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -vec3Dot(s, eye);
    r.m[13] = -vec3Dot(u, eye);
    r.m[14] =  vec3Dot(f, eye);
    return r;
}

/* Rotacao em torno do eixo Y (graus). */
static inline Mat4 mat4RotY(GLfloat graus)
{
    GLfloat a = graus * GRAUS_PARA_RAD, c = cosf(a), s = sinf(a);
    Mat4 r = mat4Identidade();
    r.m[0] = c;  r.m[8]  = s;
    r.m[2] = -s; r.m[10] = c;
    return r;
}

/* ----------------------------------------------------------------------------
 *  STRUCTS DO JOGO
 * -------------------------------------------------------------------------- */
typedef struct {
    GLfloat x, y, z;            /* posicao (y = altura dos pes)              */
    GLfloat vel_x, vel_y, vel_z;
    GLfloat yaw;                /* direcao do corpo (graus)                  */
    int     no_chao;            /* 1 = tocando uma plataforma                */
    int     vivo;
    int     frame_passo;        /* contador p/ animacao de caminhada         */
    int     pulo_duplo_ativo;  /* 1 = desbloqueou a habilidade */
    int     pulos_restantes;   /* contador para o pulo no ar   */
} Jogador;

typedef struct {
    GLfloat x, y, z;            /* centro em XZ; y = topo da plataforma       */
    GLfloat largura;            /* extensao em X                              */
    GLfloat profundidade;       /* extensao em Z                              */
    int     tipo;               /* 0=estatica 1=movel 2=fragil                */
    int     tem_estrela;
    GLfloat timer_fragil;       /* <0 nao acionada; senao seg. restantes      */
    /* auxiliares */
    GLfloat base_x, base_z;     /* origem da oscilacao (tipo movel)           */
    GLfloat fase;               /* defasagem do sinf                          */
    GLfloat delta_x, delta_z;   /* deslocamento do ultimo tick (carry)        */
    int     ativa;              /* 0 = removida (fragil expirada)             */
    int     eh_meta;            /* 1 = plataforma final (dourada)             */
} Plataforma;

typedef struct {
    GLfloat alvo_x, alvo_y, alvo_z;
    GLfloat yaw;                /* graus, orbita horizontal                   */
    GLfloat pitch;              /* graus, [-60, -10]                          */
    GLfloat distancia;          /* [8, 16]                                    */
} Camera;

typedef struct {
    GLfloat x, y, z;
    int     coletada;
} Estrela;

/* ----------------------------------------------------------------------------
 *  GLOBAIS  (definidos em jogo_funcs.c)
 * -------------------------------------------------------------------------- */
extern Jogador    g_jogador;
extern Plataforma g_nivel[NUM_PLATAFORMAS];
extern Estrela    g_estrelas[NUM_ESTRELAS];
extern Camera     g_camera;

extern int          g_estado;
extern int          g_pontos;
extern int          g_vidas;
extern int          g_total_estrelas;       /* estrelas realmente posicionadas */
extern int          g_estrelas_coletadas;
extern int          g_largura, g_altura;    /* tamanho da janela               */
extern int          g_reiniciando;
extern int          g_tempo_reinicio;       /* contador de ticks no reinicio   */
extern float        g_dificuldade;
extern float        g_tempo;                /* tempo acumulado (animacoes)     */
extern unsigned int g_semente;

extern int          g_coyote;               /* ticks de coyote restantes       */
extern int          g_buffer_pulo;          /* ticks de jump buffer restantes  */
extern int          g_plat_suporte;         /* indice da plataforma de apoio   */

extern int          g_teclas[256];          /* estado das teclas (held)        */
extern int          g_arrastando;           /* drag do botao direito ativo     */
extern int          g_posicao_x, g_posicao_y; /* ultima posicao do mouse       */

extern char         g_msg_toast[64];        /* texto do aviso temporario        */
extern float        g_msg_toast_tempo;      /* <0 = sem aviso; senao seg. restantes */
#define MSG_TOAST_DURACAO 2.5f              /* segundos visivel                  */


/* ----------------------------------------------------------------------------
 *  PROTOTIPOS  (todos implementados em jogo_funcs.c)
 * -------------------------------------------------------------------------- */
/* inicializacao */
void    gInicializa(void);
void    gInicializaJogo(void);
void    gInicializaNivel(unsigned int semente);

/* atualizacao / fisica */
void    gTimer(int valor);
void    gVerificaColisoes(void);
GLfloat gChaoEm(GLfloat x, GLfloat z);
int     gColide(Plataforma *p);
void    gAtualizaCamera(void);

/* desenho */
void    gDesenha(void);
void    gDesenhaFundo(void);
void    gDesenhaCeu(void);
void    gDesenhaPlataforma(Plataforma *p);
void    gDesenhaJogador(void);
void    gDesenhaEstrela(Estrela *e);
void    gDesenhaHUD(void);
void    gDesenhaMensagem(GLfloat x, GLfloat y, const char *texto);

/* entrada / janela */
void    gTeclado(unsigned char tecla, int x, int y);
void    gTeclaSolta(unsigned char tecla, int x, int y);
void    gMouseBotao(int botao, int estado, int x, int y);
void    gMouseMov(int x, int y);
void    gRedimensiona(int largura, int altura);

#endif /* MAIN_H */