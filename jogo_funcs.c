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

/* Gera o nivel de forma deterministica a partir de uma semente.
 * - plataforma 0: base 6x6 segura na origem
 * - plataformas 1..N-2: cursor avanca em X (gap 3..5), varia Y (-1..+3) e
 *   desvia Z (-3..+3); tipo sorteado (estatica / movel / fragil)
 * - plataforma N-1: meta 4x4 dourada
 * - ~25% das plataformas ganham uma estrela logo acima. */
void gInicializaNivel(unsigned int semente)
{
    int i;
    float cx, cy, cz, prev_right;

    srand(semente);
    g_total_estrelas = 0;
    /* marca todas as estrelas como "coletadas" => so as posicionadas aparecem */
    for (i = 0; i < NUM_ESTRELAS; i++) g_estrelas[i].coletada = 1;

    /* ---- plataforma inicial: 6x6, segura, na origem ---- */
    cx = 0.0f; cy = 0.0f; cz = 0.0f;
    g_nivel[0].x = 0.0f; g_nivel[0].y = 0.0f; g_nivel[0].z = 0.0f;
    g_nivel[0].largura = 6.0f; g_nivel[0].profundidade = 6.0f;
    g_nivel[0].tipo = 0; g_nivel[0].tem_estrela = 0; g_nivel[0].timer_fragil = -1.0f;
    g_nivel[0].base_x = 0.0f; g_nivel[0].base_z = 0.0f; g_nivel[0].fase = 0.0f;
    g_nivel[0].delta_x = 0.0f; g_nivel[0].delta_z = 0.0f;
    g_nivel[0].ativa = 1; g_nivel[0].eh_meta = 0;
    prev_right = 3.0f; /* borda direita da inicial (metade de 6) */

    /* ---- plataformas intermediarias ---- */
    for (i = 1; i < NUM_PLATAFORMAS - 1; i++) {
        Plataforma *p = &g_nivel[i];
        float w   = 2.2f + (rand() % 180) / 100.0f;   /* 2.2 .. 4.0 */
        float d   = 2.2f + (rand() % 180) / 100.0f;
        float gap = 3.0f + (rand() % 201) / 100.0f;   /* 3.0 .. 5.0 */
        int   dy  = (rand() % 5) - 1;                 /* -1 .. +3   */
        int   dz  = (rand() % 7) - 3;                 /* -3 .. +3   */
        int   r;

        cx  = prev_right + gap + w * 0.5f;            /* garante o gap entre bordas */
        prev_right = cx + w * 0.5f;
        cy += (float)dy;
        if (cy < -2.0f)  cy = -2.0f;
        if (cy > 14.0f)  cy = 14.0f;
        cz += (float)dz;
        if (cz < -12.0f) cz = -12.0f;
        if (cz > 12.0f)  cz = 12.0f;

        p->x = cx; p->y = cy; p->z = cz;
        p->largura = w; p->profundidade = d;
        p->base_x = cx; p->base_z = cz;
        p->fase = (rand() % 628) / 100.0f;            /* 0 .. ~2pi */
        p->delta_x = 0.0f; p->delta_z = 0.0f;
        p->ativa = 1; p->eh_meta = 0;
        p->timer_fragil = -1.0f;

        /* as 2 primeiras sempre estaticas (saida justa) */
        if (i <= 2) {
            p->tipo = 0;
        } else {
            r = rand() % 100;
            if      (r < 20) p->tipo = 1;             /* movel  */
            else if (r < 38) p->tipo = 2;             /* fragil */
            else             p->tipo = 0;             /* estatica */
        }

        /* ~25% com estrela acima da plataforma */
        p->tem_estrela = 0;
        if ((rand() % 100) < 26 && g_total_estrelas < NUM_ESTRELAS) {
            p->tem_estrela = 1;
            g_estrelas[g_total_estrelas].x = cx;
            g_estrelas[g_total_estrelas].y = cy + 1.3f;
            g_estrelas[g_total_estrelas].z = cz;
            g_estrelas[g_total_estrelas].coletada = 0;
            g_total_estrelas++;
        }
    }

    /* ---- plataforma final: 4x4 dourada (meta) ---- */
    {
        Plataforma *p = &g_nivel[NUM_PLATAFORMAS - 1];
        cx = prev_right + 4.0f + 2.0f;                /* gap + metade de 4 */
        p->x = cx; p->y = cy; p->z = cz;
        p->largura = 4.0f; p->profundidade = 4.0f;
        p->tipo = 0; p->tem_estrela = 0; p->timer_fragil = -1.0f;
        p->base_x = cx; p->base_z = cz; p->fase = 0.0f;
        p->delta_x = 0.0f; p->delta_z = 0.0f;
        p->ativa = 1; p->eh_meta = 1;
    }
}

/* ----------------------------------------------------------------------------
 *  FISICA / COLISAO
 * -------------------------------------------------------------------------- */
/* Consulta de chao: retorna o Y do topo da plataforma sob (x,z) que serve
 * de apoio para o jogador, ou -999 se nao houver nenhuma.
 * Considera apenas plataformas cujo topo esteja no nivel dos pes ou abaixo
 * (comportamento one-way: pousa-se vindo de cima). Tambem registra o indice
 * da plataforma escolhida em g_plat_suporte (usado p/ carregar moveis/fragil). */
GLfloat gChaoEm(GLfloat x, GLfloat z)
{
    int i, melhor = -1;
    GLfloat topo_melhor = -999.0f;
    const GLfloat tol = 0.05f;   /* tolerancia p/ "estar em pe" no topo */

    for (i = 0; i < NUM_PLATAFORMAS; i++) {
        Plataforma *p = &g_nivel[i];
        GLfloat hx, hz;
        if (!p->ativa) continue;
        hx = p->largura * 0.5f;
        hz = p->profundidade * 0.5f;
        if (x < p->x - hx || x > p->x + hx) continue;
        if (z < p->z - hz || z > p->z + hz) continue;
        if (p->y > g_jogador.y + tol) continue;      /* esta acima dos pes */
        if (p->y > topo_melhor) { topo_melhor = p->y; melhor = i; }
    }
    g_plat_suporte = melhor;
    return (melhor >= 0) ? topo_melhor : -999.0f;
}

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
