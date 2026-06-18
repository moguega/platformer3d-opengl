/* ============================================================================
 *  jogo_funcs.c  -  TODA a logica do Platformer 3D.
 *
 *  Esqueleto inicial: globais definidos e funcoes em stub. As implementacoes
 *  sao preenchidas etapa a etapa (geracao de nivel, fisica, camera, render,
 *  estados de jogo).
 * ========================================================================== */
#include "main.h"

/* ----------------------------------------------------------------------------
 *  GLOBAIS
 * -------------------------------------------------------------------------- */
Jogador    g_jogador;
Plataforma g_nivel[NUM_PLATAFORMAS];
Estrela    g_estrelas[NUM_ESTRELAS];
Camera     g_camera;

int          g_estado            = ESTADO_MENU;
int          g_pontos            = 0;
int          g_vidas             = VIDAS_INICIAIS;
int          g_total_estrelas    = 0;
int          g_estrelas_coletadas = 0;
int          g_largura           = 1000;
int          g_altura            = 700;
int          g_reiniciando       = 0;
int          g_tempo_reinicio    = 0;
float        g_dificuldade       = 1.0f;
float        g_tempo             = 0.0f;
unsigned int g_semente           = 1u;

int          g_coyote            = 0;
int          g_buffer_pulo       = 0;
int          g_plat_suporte      = -1;

int          g_teclas[256];
int          g_arrastando        = 0;
int          g_posicao_x         = 0;
int          g_posicao_y         = 0;

/* ----------------------------------------------------------------------------
 *  INICIALIZACAO
 * -------------------------------------------------------------------------- */
void gInicializa(void) { /*STUB_INICIALIZA*/ }

void gInicializaJogo(void) { /*STUB_INICIALIZAJOGO*/ }

void gInicializaNivel(unsigned int semente) { (void)semente; /*STUB_NIVEL*/ }

/* ----------------------------------------------------------------------------
 *  FISICA / COLISAO
 * -------------------------------------------------------------------------- */
GLfloat gChaoEm(GLfloat x, GLfloat z) { (void)x; (void)z; return -999.0f; /*STUB_CHAO*/ }

int gColide(Plataforma *p) { (void)p; return 0; /*STUB_COLIDE*/ }

void gVerificaColisoes(void) { /*STUB_VERIFICACOLISOES*/ }

void gTimer(int valor)
{
    (void)valor;
    /*STUB_TIMER*/
    glutPostRedisplay();
    glutTimerFunc(16, gTimer, 0);
}

/* ----------------------------------------------------------------------------
 *  CAMERA
 * -------------------------------------------------------------------------- */
void gAtualizaCamera(void) { /*STUB_CAMERA*/ }

/* ----------------------------------------------------------------------------
 *  DESENHO
 * -------------------------------------------------------------------------- */
void gDesenha(void)
{
    /*STUB_DESENHA*/
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glutSwapBuffers();
}

void gDesenhaFundo(void) { /*STUB_FUNDO*/ }

void gDesenhaCeu(void) { /*STUB_CEU*/ }

void gDesenhaPlataforma(Plataforma *p) { (void)p; /*STUB_PLATAFORMA*/ }

void gDesenhaJogador(void) { /*STUB_JOGADOR*/ }

void gDesenhaEstrela(Estrela *e) { (void)e; /*STUB_ESTRELA*/ }

void gDesenhaHUD(void) { /*STUB_HUD*/ }

void gDesenhaMensagem(GLfloat x, GLfloat y, const char *texto)
{ (void)x; (void)y; (void)texto; /*STUB_MENSAGEM*/ }

/* ----------------------------------------------------------------------------
 *  ENTRADA / JANELA
 * -------------------------------------------------------------------------- */
void gTeclado(unsigned char tecla, int x, int y)
{ (void)tecla; (void)x; (void)y; /*STUB_TECLADO*/ }

void gTeclaSolta(unsigned char tecla, int x, int y)
{ (void)tecla; (void)x; (void)y; /*STUB_TECLASOLTA*/ }

void gMouseBotao(int botao, int estado, int x, int y)
{ (void)botao; (void)estado; (void)x; (void)y; /*STUB_MOUSEBOTAO*/ }

void gMouseMov(int x, int y)
{ (void)x; (void)y; /*STUB_MOUSEMOV*/ }

void gRedimensiona(int largura, int altura)
{
    g_largura = largura;
    g_altura  = (altura > 0) ? altura : 1;
    glViewport(0, 0, g_largura, g_altura);
    /*STUB_REDIMENSIONA*/
}
