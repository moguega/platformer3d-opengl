/* ============================================================================
 *  main.c  -  Ponto de entrada minimo.
 *
 *  Apenas inicializa o GLUT, cria a janela, registra os callbacks e entra
 *  no loop principal. Toda a logica esta em jogo_funcs.c.
 * ========================================================================== */
#include "main.h"

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition(120, 80);
    glutInitWindowSize(1000, 700);
    glutCreateWindow("Platformer 3D");

    /* callbacks */
    glutDisplayFunc(gDesenha);
    glutReshapeFunc(gRedimensiona);
    glutKeyboardFunc(gTeclado);
    glutKeyboardUpFunc(gTeclaSolta);   /* libera teclas seguradas (WASD)     */
    glutMouseFunc(gMouseBotao);
    glutMotionFunc(gMouseMov);
    glutIgnoreKeyRepeat(1);            /* evita auto-repeat do teclado        */

    gInicializa();
    glutTimerFunc(16, gTimer, 0);
    glutMainLoop();
    return 0;
}
