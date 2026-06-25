/* ============================================================================
 *  jogo_funcs.c  -  TODA a logica do Platformer 3D.
 *
 *  Contem: inicializacao, geracao procedural do nivel, fisica (gravidade,
 *  pulo, coyote time, jump buffer, drag), colisao (gChaoEm + AABB lateral),
 *  camera orbital, renderizacao (plataformas, jogador, estrelas, ceu, HUD),
 *  entrada (teclado/mouse) e maquina de estados do jogo.
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

/* "toast" temporario p/ avisos rapidos (ex: "Todas as estrelas!") */
char         g_msg_toast[64]     = "";
float        g_msg_toast_tempo   = -1.0f;   /* <0 = sem mensagem ativa     */

/* ----------------------------------------------------------------------------
 *  INICIALIZACAO
 * -------------------------------------------------------------------------- */
/* Configura o estado fixo do OpenGL (profundidade, luz, material) e prepara
 * o primeiro nivel, deixando o jogo na tela de menu. */
void gInicializa(void)
{
    GLfloat amb[4]  = { 0.35f, 0.35f, 0.40f, 1.0f };
    GLfloat dif[4]  = { 0.85f, 0.85f, 0.80f, 1.0f };
    GLfloat spec[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);          /* necessario p/ o gradiente do ceu */

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

    glEnable(GL_COLOR_MATERIAL);      /* glColor3f -> material ambiente+difuso */
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    memset(g_teclas, 0, sizeof(g_teclas));
    g_arrastando = 0;
    g_estado     = ESTADO_MENU;
    g_semente    = (unsigned int)time(NULL);
    gInicializaJogo();
}

/* Reinicia toda a partida (nivel + jogador + camera + contadores). */
void gInicializaJogo(void)
{
    g_pontos             = 0;
    g_vidas              = VIDAS_INICIAIS;
    g_estrelas_coletadas = 0;
    g_reiniciando        = 0;
    g_tempo_reinicio     = 0;
    g_coyote             = 0;
    g_buffer_pulo        = 0;
    g_plat_suporte       = -1;
    g_dificuldade        = 1.0f;
    g_msg_toast_tempo    = -1.0f;
    g_msg_toast[0]       = '\0';
    g_jogador.pulo_duplo_ativo = 0;

    gInicializaNivel(g_semente);

    g_jogador.x = 0.0f; g_jogador.z = 0.0f;
    g_jogador.y = g_nivel[0].y + 0.05f;
    g_jogador.vel_x = g_jogador.vel_y = g_jogador.vel_z = 0.0f;
    g_jogador.yaw = 0.0f;
    g_jogador.no_chao = 1;
    g_jogador.vivo = 1;
    g_jogador.frame_passo = 0;

    g_camera.alvo_x = 0.0f;
    g_camera.alvo_y = g_jogador.y + 1.0f;
    g_camera.alvo_z = 0.0f;
    g_camera.yaw = 0.0f;
    g_camera.pitch = -25.0f;
    g_camera.distancia = 12.0f;
}

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

        /* saida (2 primeiras) e aproximacao da meta sempre estaticas */
        if (i <= 2 || i == NUM_PLATAFORMAS - 2) {
            p->tipo = 0;
        } else {
            r = rand() % 100;
            if      (r < 20) p->tipo = 1;             /* movel  */
            else if (r < 38) p->tipo = 2;             /* fragil */
            else             p->tipo = 0;             /* estatica */
            /* evita duas frageis seguidas (corredor impossivel) */
            if (p->tipo == 2 && g_nivel[i - 1].tipo == 2) p->tipo = 0;
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

/* Teste de "parede": 1 se o corpo do jogador sobrepoe a lateral da caixa da
 * plataforma. Retorna 0 quando o jogador esta em pe no topo (sem sobreposicao
 * vertical de corpo) — esses casos sao tratados como chao, nao como parede. */
int gColide(Plataforma *p)
{
    GLfloat px0, px1, pz0, pz1, ptop, pbot, jbot, jtop;
    if (!p->ativa) return 0;
    px0 = p->x - p->largura * 0.5f - RAIO_JOGADOR;
    px1 = p->x + p->largura * 0.5f + RAIO_JOGADOR;
    pz0 = p->z - p->profundidade * 0.5f - RAIO_JOGADOR;
    pz1 = p->z + p->profundidade * 0.5f + RAIO_JOGADOR;
    ptop = p->y;
    pbot = p->y - ESPESSURA_PLAT;
    jbot = g_jogador.y;
    jtop = g_jogador.y + ALTURA_JOGADOR;
    if (jtop <= pbot + 0.05f || jbot >= ptop - 0.05f) return 0;
    if (g_jogador.x <= px0 || g_jogador.x >= px1) return 0;
    if (g_jogador.z <= pz0 || g_jogador.z >= pz1) return 0;
    return 1;
}

/* Integra a posicao do jogador e resolve as colisoes:
 *  - horizontal: AABB, empurrando pelo eixo de menor penetracao
 *  - vertical:  via gChaoEm (pouso vindo de cima)
 *  - coleta de estrelas por proximidade. */
void gVerificaColisoes(void)
{
    int i;
    GLfloat old_y, new_y, chao;

    /* ---- horizontal: integra X/Z e resolve colisao lateral (AABB) ---- */
    g_jogador.x += g_jogador.vel_x * DT;
    g_jogador.z += g_jogador.vel_z * DT;

    for (i = 0; i < NUM_PLATAFORMAS; i++) {
        Plataforma *p = &g_nivel[i];
        GLfloat px0, px1, pz0, pz1;
        GLfloat penXe, penXd, penZf, penZt, minX, minZ;
        if (!gColide(p)) continue;

        px0 = p->x - p->largura * 0.5f - RAIO_JOGADOR;
        px1 = p->x + p->largura * 0.5f + RAIO_JOGADOR;
        pz0 = p->z - p->profundidade * 0.5f - RAIO_JOGADOR;
        pz1 = p->z + p->profundidade * 0.5f + RAIO_JOGADOR;

        penXe = g_jogador.x - px0;   /* empurrar p/ -X */
        penXd = px1 - g_jogador.x;   /* empurrar p/ +X */
        penZf = g_jogador.z - pz0;   /* empurrar p/ -Z */
        penZt = pz1 - g_jogador.z;   /* empurrar p/ +Z */
        minX  = (penXe < penXd) ? penXe : penXd;
        minZ  = (penZf < penZt) ? penZf : penZt;

        if (minX < minZ) {
            g_jogador.x = (penXe < penXd) ? px0 : px1;
            g_jogador.vel_x = 0.0f;
        } else {
            g_jogador.z = (penZf < penZt) ? pz0 : pz1;
            g_jogador.vel_z = 0.0f;
        }
    }

    /* ---- vertical: gravidade ja aplicada em vel_y; pouso via gChaoEm ---- */
    old_y = g_jogador.y;                        /* y antes de mover            */
    new_y = g_jogador.y + g_jogador.vel_y * DT;
    chao  = gChaoEm(g_jogador.x, g_jogador.z);  /* topo sob os pes (e suporte) */

    g_jogador.no_chao = 0;
    if (chao > -900.0f && g_jogador.vel_y <= 0.0f &&
        new_y <= chao && old_y >= chao - 0.5f) {
        /* pousou em cima da plataforma */
        g_jogador.y = chao;
        g_jogador.vel_y = 0.0f;
        g_jogador.no_chao = 1;

        if (g_plat_suporte >= 0) {
            Plataforma *p = &g_nivel[g_plat_suporte];
            if (p->tipo == 2 && p->timer_fragil < 0.0f)
                p->timer_fragil = FRAG_TEMPO;   /* aciona a fragil ao pisar */
            if (p->eh_meta && g_estado == ESTADO_JOGANDO) {
                g_estado = ESTADO_VITORIA;
                g_pontos += PONTOS_VITORIA;
            }
        }
    } else {
        g_jogador.y = new_y;
    }

    /* ---- coleta de estrelas por proximidade ---- */
    for (i = 0; i < g_total_estrelas; i++) {
        Estrela *e = &g_estrelas[i];
        GLfloat dx, dy, dz;
        if (e->coletada) continue;
        dx = e->x - g_jogador.x;
        dy = e->y - (g_jogador.y + 0.6f);
        dz = e->z - g_jogador.z;
        if (dx * dx + dy * dy + dz * dz < 1.0f) {
            e->coletada = 1;
            g_estrelas_coletadas++;
            g_pontos += 100;

            /* todas as estrelas da arena coletadas: aviso temporario,
             * sem alterar o estado do jogo (continua ESTADO_JOGANDO) */
            if (g_estrelas_coletadas == g_total_estrelas && g_total_estrelas > 0) {
                g_estado = ESTADO_ESCOLHA_RECOMPENSA;
            }
        }
    }
}

/* Loop fisico: roda a ~60 Hz com passo fixo (DT). */
void gTimer(int valor)
{
    int i;
    (void)valor;

    g_tempo += DT;

    /* aviso temporario (toast): conta regressivamente e se desliga sozinho */
    if (g_msg_toast_tempo >= 0.0f) {
        g_msg_toast_tempo -= DT;
        if (g_msg_toast_tempo < 0.0f) g_msg_toast_tempo = -1.0f;
    }

    if (g_estado == ESTADO_JOGANDO) {
        /* --- 1. plataformas: moveis oscilam, frageis contam o tempo --- */
        for (i = 0; i < NUM_PLATAFORMAS; i++) {
            Plataforma *p = &g_nivel[i];
            if (!p->ativa) continue;
            if (p->tipo == 1) {
                float novo = p->base_z + sinf(g_tempo * 1.5f + p->fase) * 1.6f;
                p->delta_z = novo - p->z;   /* delta usado p/ carregar o jogador */
                p->delta_x = 0.0f;
                p->z = novo;
            } else {
                p->delta_x = 0.0f;
                p->delta_z = 0.0f;
            }
            if (p->tipo == 2 && p->timer_fragil >= 0.0f) {
                p->timer_fragil -= DT;
                if (p->timer_fragil <= 0.0f) p->ativa = 0;
            }
        }

        /* carrega o jogador junto da plataforma movel onde esta apoiado */
        if (g_plat_suporte >= 0 && g_jogador.no_chao &&
            g_nivel[g_plat_suporte].ativa && g_nivel[g_plat_suporte].tipo == 1) {
            g_jogador.x += g_nivel[g_plat_suporte].delta_x;
            g_jogador.z += g_nivel[g_plat_suporte].delta_z;
        }

        if (g_reiniciando) {
            /* --- delay de reinicio: respawna ou encerra em game over --- */
            g_tempo_reinicio++;
            if (g_tempo_reinicio >= TICKS_REINICIO) {
                g_reiniciando = 0;
                if (g_vidas <= 0) {
                    g_estado = ESTADO_GAME_OVER;
                } else {
                    int j;
                    /* rearma as plataformas frageis (e garante que tudo
                     * que nao e frageis-ja-removido continue ativo) */
                    for (j = 0; j < NUM_PLATAFORMAS; j++) {
                        if (g_nivel[j].tipo == 2) {
                            g_nivel[j].ativa        = 1;
                            g_nivel[j].timer_fragil = -1.0f;
                        }
                    }
                    g_jogador.x = 0.0f; g_jogador.z = 0.0f;
                    g_jogador.y = g_nivel[0].y + 0.05f;
                    g_jogador.vel_x = g_jogador.vel_y = g_jogador.vel_z = 0.0f;
                    g_jogador.no_chao = 1; g_jogador.vivo = 1;
                    g_coyote = 0; g_buffer_pulo = 0;
                    g_plat_suporte = -1;
                }
            }
        } else {
            /* --- 2. entrada -> velocidade alvo, relativa ao yaw da camera --- */
            float yawR = g_camera.yaw * GRAUS_PARA_RAD;
            float fx = -sinf(yawR), fz = -cosf(yawR); /* "frente" da camera no plano XZ */
            float rx = -fz,         rz =  fx;         /* "direita" da camera            */
            float mvx = 0.0f, mvz = 0.0f, len;
            float alvo_vx = 0.0f, alvo_vz = 0.0f, resp;

            if (g_teclas['w']) { mvx += fx; mvz += fz; }
            if (g_teclas['s']) { mvx -= fx; mvz -= fz; }
            if (g_teclas['d']) { mvx += rx; mvz += rz; }
            if (g_teclas['a']) { mvx -= rx; mvz -= rz; }

            len = sqrtf(mvx * mvx + mvz * mvz);
            if (len > 0.001f) {
                mvx /= len; mvz /= len;
                alvo_vx = mvx * VEL_MOVIMENTO;
                alvo_vz = mvz * VEL_MOVIMENTO;
                g_jogador.yaw = atan2f(mvx, mvz) * RAD_PARA_GRAUS; /* corpo encara o movimento */
                g_jogador.frame_passo++;
            }
            /* lerp p/ a velocidade alvo: responde mais rapido no chao que no ar */
            resp = g_jogador.no_chao ? DRAG_CHAO : DRAG_AR;
            g_jogador.vel_x += (alvo_vx - g_jogador.vel_x) * resp;
            g_jogador.vel_z += (alvo_vz - g_jogador.vel_z) * resp;

            /* --- 3. gravidade --- */
            g_jogador.vel_y += GRAVIDADE * DT;

            /* --- 4. coyote time + jump buffer ---
             * coyote: alguns ticks apos sair da borda ainda permitem pular.
             * buffer: pulo pressionado um pouco antes de tocar o chao "espera". */
            if (g_jogador.no_chao)      g_coyote = FRAMES_COYOTE;
            else if (g_coyote > 0)      g_coyote--;
            if (g_buffer_pulo > 0)      g_buffer_pulo--;

            /* Recarrega o pulo duplo ao tocar no chao */
            if (g_jogador.no_chao) {
                g_jogador.pulos_restantes = 1;
            }

            if (g_buffer_pulo > 0) {
                if (g_jogador.no_chao || g_coyote > 0) {
                    /* Pulo normal do chao */
                    g_jogador.vel_y   = VEL_PULO;
                    g_jogador.no_chao = 0;
                    g_coyote      = 0;
                    g_buffer_pulo = 0;
                } else if (g_jogador.pulo_duplo_ativo && g_jogador.pulos_restantes > 0) {
                    /* Pulo duplo no ar */
                    g_jogador.vel_y   = VEL_PULO; /* Zera a inercia de queda e pula de novo */
                    g_jogador.pulos_restantes--;
                    g_buffer_pulo = 0;
                }
            }

            /* --- 5. integra + colide --- */
            gVerificaColisoes();

            /* --- 6. caiu no vazio --- */
            if (g_jogador.y < QUEDA_LIMITE) {
                g_vidas--;
                g_jogador.vivo   = 0;
                g_reiniciando    = 1;
                g_tempo_reinicio = 0;
            }
        }
    }

    gAtualizaCamera();
    glutPostRedisplay();
    glutTimerFunc(16, gTimer, 0);
}

/* ----------------------------------------------------------------------------
 *  CAMERA
 * -------------------------------------------------------------------------- */
/* A camera orbital persegue o jogador suavemente (lerp por tick).
 * yaw/pitch/distancia sao controlados pelo mouse; a posicao do olho e
 * recalculada a partir do alvo em gDesenha. */
void gAtualizaCamera(void)
{
    Vec3 atual = { g_camera.alvo_x, g_camera.alvo_y, g_camera.alvo_z };
    Vec3 dest  = { g_jogador.x, g_jogador.y + 1.0f, g_jogador.z };
    Vec3 novo  = vec3Lerp(atual, dest, 0.08f);   /* fator de seguimento */
    g_camera.alvo_x = novo.x;
    g_camera.alvo_y = novo.y;
    g_camera.alvo_z = novo.z;
}

/* ----------------------------------------------------------------------------
 *  DESENHO
 * -------------------------------------------------------------------------- */
/* Caixa axis-aligned com 6 quads e normais explicitas (base de quase tudo). */
static void gCaixa(GLfloat x0, GLfloat y0, GLfloat z0,
                   GLfloat x1, GLfloat y1, GLfloat z1)
{
    glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);  glVertex3f(x0,y1,z0); glVertex3f(x0,y1,z1); glVertex3f(x1,y1,z1); glVertex3f(x1,y1,z0);
        glNormal3f(0,-1, 0);  glVertex3f(x0,y0,z0); glVertex3f(x1,y0,z0); glVertex3f(x1,y0,z1); glVertex3f(x0,y0,z1);
        glNormal3f(1, 0, 0);  glVertex3f(x1,y0,z0); glVertex3f(x1,y1,z0); glVertex3f(x1,y1,z1); glVertex3f(x1,y0,z1);
        glNormal3f(-1,0, 0);  glVertex3f(x0,y0,z0); glVertex3f(x0,y0,z1); glVertex3f(x0,y1,z1); glVertex3f(x0,y1,z0);
        glNormal3f(0, 0, 1);  glVertex3f(x0,y0,z1); glVertex3f(x1,y0,z1); glVertex3f(x1,y1,z1); glVertex3f(x0,y1,z1);
        glNormal3f(0, 0,-1);  glVertex3f(x0,y0,z0); glVertex3f(x0,y1,z0); glVertex3f(x1,y1,z0); glVertex3f(x1,y0,z0);
    glEnd();
}

/* Texto centralizado horizontalmente (usado em menus e mensagens grandes). */
static void gTextoCentro(GLfloat y, const char *texto)
{
    int w = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char *)texto);
    gDesenhaMensagem((g_largura - w) * 0.5f, y, texto);
}

/* Render principal: ceu, cena 3D (camera orbital) e HUD 2D, conforme o estado. */
void gDesenha(void)
{
    int i;
    GLfloat yawR, elev, horiz, ex, ey, ez;
    GLfloat luz_pos[4] = { 0.4f, 1.0f, 0.35f, 0.0f };  /* luz direcional */

    if (g_estado == ESTADO_GAME_OVER)   glClearColor(0.30f, 0.05f, 0.05f, 1.0f);
    else if (g_estado == ESTADO_VITORIA) glClearColor(0.05f, 0.30f, 0.10f, 1.0f);
    else                                 glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* ---- menu: apenas texto ---- */
    if (g_estado == ESTADO_MENU) {
        glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        gluOrtho2D(0.0, (double)g_largura, 0.0, (double)g_altura);
        glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
        glColor3f(1.0f, 0.95f, 0.5f);
        gTextoCentro(g_altura * 0.64f, "PLATFORMER 3D");
        glColor3f(0.85f, 0.9f, 1.0f);
        gTextoCentro(g_altura * 0.50f, "WASD = mover     ESPACO = pular");
        gTextoCentro(g_altura * 0.44f, "Botao direito = girar camera     Roda = zoom");
        gTextoCentro(g_altura * 0.38f, "R = novo nivel     ESC = sair");
        glColor3f(0.5f, 1.0f, 0.6f);
        gTextoCentro(g_altura * 0.26f, "ENTER ou ESPACO para comecar");
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
        glutSwapBuffers();
        return;
    }

    /* ceu so no jogo normal (vitoria/gameover usam a cor de fundo tingida) */
    if (g_estado == ESTADO_JOGANDO)
        gDesenhaFundo();

    /* ---- camera orbital: olho calculado a partir de alvo + yaw + pitch ---- */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    yawR  = g_camera.yaw * GRAUS_PARA_RAD;
    elev  = -g_camera.pitch * GRAUS_PARA_RAD;        /* pitch [-60,-10] -> elev [10,60] */
    horiz = g_camera.distancia * cosf(elev);
    ex = g_camera.alvo_x + horiz * sinf(yawR);
    ez = g_camera.alvo_z + horiz * cosf(yawR);
    ey = g_camera.alvo_y + g_camera.distancia * sinf(elev);
    gluLookAt(ex, ey, ez,
              g_camera.alvo_x, g_camera.alvo_y, g_camera.alvo_z,
              0.0f, 1.0f, 0.0f);

    glLightfv(GL_LIGHT0, GL_POSITION, luz_pos);      /* luz fixa no mundo */

    /* ---- cena ---- */
    for (i = 0; i < NUM_PLATAFORMAS; i++) gDesenhaPlataforma(&g_nivel[i]);
    for (i = 0; i < g_total_estrelas;  i++) gDesenhaEstrela(&g_estrelas[i]);
    if (!g_reiniciando) gDesenhaJogador();           /* some durante o reinicio */
    
    if (g_estado == ESTADO_ESCOLHA_RECOMPENSA) {
        /* Desativa 3D temporariamente para desenhar o menu por cima da cena */
        glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        gluOrtho2D(0.0, (double)g_largura, 0.0, (double)g_altura);
        glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
        
        /* Fundo semi-transparente escurecido (requer GL_BLEND habilitado na inicializacao se quiser transparencia real, senao fica preto opaco) */
        glColor3f(0.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
            glVertex2f(0, 0); glVertex2f(g_largura, 0);
            glVertex2f(g_largura, g_altura); glVertex2f(0, g_altura);
        glEnd();

        glColor3f(1.0f, 0.85f, 0.20f);
        gTextoCentro(g_altura * 0.70f, "TODAS AS ESTRELAS COLETADAS!");
        
        glColor3f(1.0f, 1.0f, 1.0f);
        gTextoCentro(g_altura * 0.55f, "ESCOLHA SUA RECOMPENSA:");
        
        glColor3f(0.5f, 1.0f, 0.6f);
        gTextoCentro(g_altura * 0.45f, "[1] Pulo Duplo");
        
        glColor3f(0.3f, 0.7f, 1.0f);
        gTextoCentro(g_altura * 0.38f, "[2] Congelar Plataformas (Mudar tudo para Estatica)");
        
        glColor3f(1.0f, 0.5f, 0.5f);
        gTextoCentro(g_altura * 0.31f, "[3] Vida Extra (+1)");

        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
        glutSwapBuffers();
        return; /* Pausa a renderizacao normal */
    }

    gDesenhaHUD();
    glutSwapBuffers();
}

/* Prepara projecao 2D (0..1) sem profundidade e desenha o ceu como quad. */
void gDesenhaFundo(void)
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    gDesenhaCeu();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

/* Gradiente de ceu falso: base clara -> topo azul (precisa de GL_SMOOTH). */
void gDesenhaCeu(void)
{
    glBegin(GL_QUADS);
        glColor3f(0.62f, 0.82f, 0.96f); glVertex2f(0.0f, 0.0f);
        glColor3f(0.62f, 0.82f, 0.96f); glVertex2f(1.0f, 0.0f);
        glColor3f(0.16f, 0.34f, 0.62f); glVertex2f(1.0f, 1.0f);
        glColor3f(0.16f, 0.34f, 0.62f); glVertex2f(0.0f, 1.0f);
    glEnd();
}

/* Desenha uma plataforma como caixa, com cor por tipo (e pisca se fragil). */
void gDesenhaPlataforma(Plataforma *p)
{
    GLfloat x0, x1, z0, z1, y0, y1;
    if (!p->ativa) return;

    if (p->eh_meta) {
        glColor3f(1.0f, 0.85f, 0.20f);                 /* dourado (meta) */
    } else if (p->tipo == 1) {
        glColor3f(0.30f, 0.70f, 0.40f);                /* verde (movel)  */
    } else if (p->tipo == 2) {
        /* laranja; pisca quando faltam menos de 0.8 s */
        if (p->timer_fragil >= 0.0f && p->timer_fragil < 0.8f &&
            ((int)(p->timer_fragil * 12.0f) & 1))
            glColor3f(1.0f, 0.80f, 0.55f);
        else
            glColor3f(0.90f, 0.50f, 0.20f);
    } else {
        glColor3f(0.40f, 0.50f, 0.60f);                /* cinza azulado  */
    }

    x0 = p->x - p->largura      * 0.5f; x1 = p->x + p->largura      * 0.5f;
    z0 = p->z - p->profundidade * 0.5f; z1 = p->z + p->profundidade * 0.5f;
    y1 = p->y; y0 = p->y - ESPESSURA_PLAT;
    gCaixa(x0, y0, z0, x1, y1, z1);
}

/* Boneco low poly inspirado no "Steve" (estilo Minecraft): cabeca cubica
 * grande, torso reto (camisa), bracos e pernas em caixas finas que balancam
 * em oposicao durante a caminhada. Paleta propria (nao reproduz a skin
 * oficial), so a linguagem visual "em blocos". Encara g_jogador.yaw. */
void gDesenhaJogador(void)
{
    float bob = sinf(g_jogador.frame_passo * 0.4f) * 0.05f;  /* leve balanco */
    float sw  = sinf(g_jogador.frame_passo * 0.4f) * 0.18f;  /* passada (pernas) */
    float aw  = sinf(g_jogador.frame_passo * 0.4f) * 0.22f;  /* passada (bracos, oposta) */

    glPushMatrix();
        glTranslatef(g_jogador.x, g_jogador.y, g_jogador.z);
        glRotatef(g_jogador.yaw, 0.0f, 1.0f, 0.0f);

        /* ---- torso ("camisa") ---- */
        glColor3f(0.20f, 0.65f, 0.60f);                      /* verde-agua */
        gCaixa(-0.26f, 0.50f + bob, -0.16f,  0.26f, 1.10f + bob, 0.16f);

        /* ---- cabeca (cubica, proporcionalmente grande) ---- */
        glColor3f(0.85f, 0.65f, 0.50f);                      /* pele       */
        gCaixa(-0.26f, 1.10f + bob, -0.26f,  0.26f, 1.62f + bob, 0.26f);

        /* "rosto": faixa mais escura na frente p/ sugerir olhos sem textura */
        glColor3f(0.30f, 0.22f, 0.18f);
        gCaixa(-0.26f, 1.32f + bob, -0.265f, 0.26f, 1.42f + bob, -0.255f);

        /* ---- bracos (balancam em oposicao as pernas) ---- */
        glColor3f(0.85f, 0.65f, 0.50f);                      /* pele       */
        gCaixa(-0.40f, 0.55f + bob, -0.10f + aw, -0.26f, 1.08f + bob, 0.10f + aw);
        gCaixa( 0.26f, 0.55f + bob, -0.10f - aw,  0.40f, 1.08f + bob, 0.10f - aw);

        /* ---- pernas ("calca") ---- */
        glColor3f(0.20f, 0.25f, 0.55f);                      /* azul       */
        gCaixa(-0.24f, 0.10f, -0.13f + sw, -0.02f, 0.50f, 0.13f + sw);
        gCaixa( 0.02f, 0.10f, -0.13f - sw,  0.24f, 0.50f, 0.13f - sw);

        /* ---- sapatos ---- */
        glColor3f(0.35f, 0.35f, 0.38f);                      /* cinza      */
        gCaixa(-0.25f, 0.0f, -0.16f + sw, -0.01f, 0.10f, 0.17f + sw);
        gCaixa( 0.01f, 0.0f, -0.16f - sw,  0.25f, 0.10f, 0.17f - sw);
    glPopMatrix();
}

/* Estrela: octaedro amarelo girando em torno de Y (animacao por g_tempo). */
void gDesenhaEstrela(Estrela *e)
{
    const float s = 0.32f;
    if (e->coletada) return;
    glPushMatrix();
        glTranslatef(e->x, e->y, e->z);
        glRotatef(g_tempo * 90.0f, 0.0f, 1.0f, 0.0f);  /* ~g_tempo*2 rad/s */
        glColor3f(1.0f, 0.9f, 0.2f);
        glBegin(GL_TRIANGLES);
            /* topo (0,s,0) com 4 vertices do equador */
            glNormal3f( 1, 1, 1); glVertex3f(0,s,0); glVertex3f(s,0,0); glVertex3f(0,0,s);
            glNormal3f(-1, 1, 1); glVertex3f(0,s,0); glVertex3f(0,0,s); glVertex3f(-s,0,0);
            glNormal3f(-1, 1,-1); glVertex3f(0,s,0); glVertex3f(-s,0,0); glVertex3f(0,0,-s);
            glNormal3f( 1, 1,-1); glVertex3f(0,s,0); glVertex3f(0,0,-s); glVertex3f(s,0,0);
            /* base (0,-s,0) */
            glNormal3f( 1,-1, 1); glVertex3f(0,-s,0); glVertex3f(0,0,s); glVertex3f(s,0,0);
            glNormal3f(-1,-1, 1); glVertex3f(0,-s,0); glVertex3f(-s,0,0); glVertex3f(0,0,s);
            glNormal3f(-1,-1,-1); glVertex3f(0,-s,0); glVertex3f(0,0,-s); glVertex3f(-s,0,0);
            glNormal3f( 1,-1,-1); glVertex3f(0,-s,0); glVertex3f(s,0,0); glVertex3f(0,0,-s);
        glEnd();
    glPopMatrix();
}

/* HUD 2D (glOrtho em pixels): estrelas, vidas, pontos e mensagens de estado. */
void gDesenhaHUD(void)
{
    char buf[96];
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0.0, (double)g_largura, 0.0, (double)g_altura);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(buf, "Estrelas: %d / %d", g_estrelas_coletadas, g_total_estrelas);
    gDesenhaMensagem(20.0f, (GLfloat)g_altura - 30.0f, buf);
    sprintf(buf, "Vidas: %d", g_vidas);
    gDesenhaMensagem(20.0f, (GLfloat)g_altura - 54.0f, buf);
    sprintf(buf, "Pontos: %d", g_pontos);
    gDesenhaMensagem(20.0f, (GLfloat)g_altura - 78.0f, buf);

    /* aviso temporario (toast): centralizado perto do topo, com fade-out
     * suave nos ultimos 0.5s antes de desaparecer */
    if (g_msg_toast_tempo >= 0.0f) {
        GLfloat alpha = (g_msg_toast_tempo < 0.5f) ? (g_msg_toast_tempo / 0.5f) : 1.0f;
        glColor3f(0.4f + 0.6f * alpha, 1.0f, 0.5f + 0.5f * alpha);
        gTextoCentro((GLfloat)g_altura - 110.0f, g_msg_toast);
    }

    /* "Reiniciando..." pisca a cada 8 ticks durante o delay */
    if (g_reiniciando && ((g_tempo_reinicio / 8) & 1)) {
        glColor3f(1.0f, 0.6f, 0.3f);
        gTextoCentro(g_altura * 0.5f, "Reiniciando...");
    }

    if (g_estado == ESTADO_VITORIA) {
        glColor3f(1.0f, 0.95f, 0.4f);
        gTextoCentro(g_altura * 0.60f, "GOAL!  VOCE VENCEU!");
        glColor3f(1.0f, 1.0f, 1.0f);
        sprintf(buf, "Pontuacao final: %d", g_pontos);
        gTextoCentro(g_altura * 0.50f, buf);
        gTextoCentro(g_altura * 0.42f, "R = jogar de novo (nova seed)");
    } else if (g_estado == ESTADO_GAME_OVER) {
        glColor3f(1.0f, 0.5f, 0.5f);
        gTextoCentro(g_altura * 0.58f, "GAME OVER");
        glColor3f(1.0f, 1.0f, 1.0f);
        gTextoCentro(g_altura * 0.48f, "R = tentar de novo (nova seed)");
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

/* Escreve uma string com bitmap font na posicao 2D (x,y) atual. */
void gDesenhaMensagem(GLfloat x, GLfloat y, const char *texto)
{
    const char *p;
    glRasterPos2f(x, y);
    for (p = texto; *p; p++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
}

/* ----------------------------------------------------------------------------
 *  ENTRADA / JANELA
 * -------------------------------------------------------------------------- */
/* Tecla pressionada. Mantem o estado em g_teclas[] (WASD continuos) e trata
 * acoes pontuais: pulo (jump buffer), reinicio com nova seed, iniciar, sair. */
void gTeclado(unsigned char tecla, int x, int y)
{
    (void)x; (void)y;
    if (tecla >= 'A' && tecla <= 'Z') tecla = (unsigned char)(tecla + 32); /* normaliza */
    g_teclas[tecla] = 1;

    if (tecla == 27) exit(0);                       /* ESC = sair */

    if (tecla == 'r') {                             /* R = novo nivel (nova seed) */
        g_semente = (unsigned int)time(NULL);
        gInicializaJogo();
        g_estado = ESTADO_JOGANDO;
        return;
    }

    if (tecla == ' ') {
        if (g_estado == ESTADO_MENU)         g_estado = ESTADO_JOGANDO;
        else if (g_estado == ESTADO_JOGANDO) g_buffer_pulo = FRAMES_BUFFER_PULO;
    }

    if (tecla == 13 && g_estado == ESTADO_MENU)     /* ENTER = comecar */
        g_estado = ESTADO_JOGANDO;
    
    if (g_estado == ESTADO_ESCOLHA_RECOMPENSA) {
        if (tecla == '1') {
            g_jogador.pulo_duplo_ativo = 1;
            g_estado = ESTADO_JOGANDO;
        }
        else if (tecla == '2') {
            int i;
            for (i = 0; i < NUM_PLATAFORMAS; i++) {
                if (g_nivel[i].tipo == 1 || g_nivel[i].tipo == 2) {
                    g_nivel[i].tipo = 0; /* Transforma moveis e frageis em estaticas */
                }
            }
            g_estado = ESTADO_JOGANDO;
        }
        else if (tecla == '3') {
            g_vidas++;
            g_estado = ESTADO_JOGANDO;
        }
        return;
    }
}

/* Tecla liberada: limpa o estado em g_teclas[]. */
void gTeclaSolta(unsigned char tecla, int x, int y)
{
    (void)x; (void)y;
    if (tecla >= 'A' && tecla <= 'Z') tecla = (unsigned char)(tecla + 32);
    g_teclas[tecla] = 0;
}

/* Botao direito inicia/encerra o drag de orbita; roda do mouse faz zoom. */
void gMouseBotao(int botao, int estado, int x, int y)
{
    if (botao == GLUT_RIGHT_BUTTON) {
        if (estado == GLUT_DOWN) {
            g_arrastando = 1;
            g_posicao_x  = x;
            g_posicao_y  = y;
        } else {
            g_arrastando = 0;
        }
    }
    /* roda do mouse (freeglut: botoes 3 = cima, 4 = baixo) ajusta a distancia */
    if (botao == 3 && estado == GLUT_DOWN) g_camera.distancia -= 1.0f;
    if (botao == 4 && estado == GLUT_DOWN) g_camera.distancia += 1.0f;
    if (g_camera.distancia < 8.0f)  g_camera.distancia = 8.0f;
    if (g_camera.distancia > 16.0f) g_camera.distancia = 16.0f;
}

/* Arrasto com o botao direito gira a camera; pitch fica preso entre -60 e -10. */
void gMouseMov(int x, int y)
{
    if (g_arrastando) {
        int dx = x - g_posicao_x;
        int dy = y - g_posicao_y;
        g_camera.yaw   += dx * 0.30f;
        g_camera.pitch -= dy * 0.30f;
        if (g_camera.pitch < -60.0f) g_camera.pitch = -60.0f;
        if (g_camera.pitch > -10.0f) g_camera.pitch = -10.0f;
        g_posicao_x = x;
        g_posicao_y = y;
    }
}

void gRedimensiona(int largura, int altura)
{
    g_largura = largura;
    g_altura  = (altura > 0) ? altura : 1;
    glViewport(0, 0, g_largura, g_altura);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)g_largura / (double)g_altura, 0.1, 500.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}