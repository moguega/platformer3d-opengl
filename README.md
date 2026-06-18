# Platformer 3D — OpenGL Legado (FFP) + freeglut

Jogo de plataforma 3D *low poly* escrito em **C puro**, usando apenas **OpenGL
em modo de compatibilidade** (`glBegin`/`glEnd`, sem shaders) e **GLUT/freeglut**
para janela, entrada e loop principal. Sem nenhuma dependência externa além de
OpenGL, GLU, GLUT e a biblioteca padrão do C.

O objetivo: atravessar plataformas flutuantes geradas proceduralmente, coletar
estrelas e alcançar a plataforma dourada final.

---

## Estrutura do projeto

Exatamente **3 arquivos** de código (mesma organização do projeto de referência):

| Arquivo         | Conteúdo                                                                 |
|-----------------|--------------------------------------------------------------------------|
| `main.h`        | includes, `#define`s, biblioteca matemática 3D inline (`Vec3`/`Mat4`), structs, `extern` dos globais e protótipos |
| `main.c`        | apenas `glutInit`, criação da janela, registro de callbacks e `glutMainLoop` |
| `jogo_funcs.c`  | **toda** a lógica: inicialização, geração de nível, física, colisão, câmera, renderização, entrada e estados |

Convenções: funções com prefixo `g` (ex.: `gDesenha`, `gChaoEm`, `gTimer`) e
globais com prefixo `g_` (ex.: `g_jogador`, `g_nivel`, `g_camera`).

---

## Dependências

- Compilador C (GCC/MinGW, Clang ou MSVC)
- OpenGL + GLU (normalmente já presentes no sistema)
- **freeglut** (ou GLUT)

### Instalando o freeglut

- **MSYS2 / MinGW (UCRT64)** — usado neste repositório:
  ```bash
  pacman -S mingw-w64-ucrt-x86_64-freeglut
  ```
- **Debian / Ubuntu:**
  ```bash
  sudo apt install freeglut3-dev
  ```
- **Fedora:**
  ```bash
  sudo dnf install freeglut-devel
  ```

---

## Compilação

**Linux:**
```bash
gcc main.c jogo_funcs.c -o jogo -lGL -lGLU -lglut -lm && ./jogo
```

**Windows (MinGW / MSYS2):**
```bash
gcc main.c jogo_funcs.c -o jogo.exe -lopengl32 -lglu32 -lfreeglut -lm
./jogo.exe
```

> No MSYS2 use o terminal **UCRT64** (ou garanta que `…/ucrt64/bin` esteja no
> `PATH`) para que `libfreeglut.dll` seja encontrada em tempo de execução.

---

## Controles

| Tecla / Mouse            | Ação                                            |
|--------------------------|-------------------------------------------------|
| `W` `A` `S` `D`          | Mover (relativo à direção da câmera)            |
| `Espaço`                 | Pular (com *coyote time* e *jump buffer*)       |
| Botão **direito** + arrastar | Girar a câmera orbital                       |
| Roda do mouse            | Zoom (distância 8–16)                            |
| `Enter` / `Espaço`       | Começar (no menu)                               |
| `R`                      | Reiniciar com um novo nível (nova *seed*)       |
| `Esc`                    | Sair                                            |

---

## Mecânicas

- **Geração procedural:** 33 plataformas a partir de uma *seed* (`srand`).
  Plataforma inicial 6×6 segura; final 4×4 dourada (a meta).
- **Tipos de plataforma:**
  - `0` estática (cinza azulado)
  - `1` móvel (verde, oscila com `sinf`) — carrega o jogador junto
  - `2` frágil (laranja, some ~1,5 s após ser pisada; pisca antes de cair)
- **Estrelas:** ~25% das plataformas têm uma estrela giratória para coletar.
- **Física (passo fixo de 16 ms):** gravidade, pulo, *coyote time*
  (`FRAMES_COYOTE`), *jump buffer* (`FRAMES_BUFFER_PULO`), *drag* diferente no
  chão e no ar, colisão vertical via `gChaoEm` e colisão lateral AABB pelo eixo
  de menor penetração.
- **Câmera orbital:** segue o jogador com *lerp* suave; *yaw* pelo mouse,
  *pitch* travado entre −60° e −10°.
- **Estados:** Menu → Jogando → Vitória / Game Over. Cair no vazio custa uma
  vida; sem vidas, é Game Over.

---

## Build com warnings (recomendado durante o desenvolvimento)

```bash
gcc -Wall -Wextra main.c jogo_funcs.c -o jogo.exe -lopengl32 -lglu32 -lfreeglut -lm
```
O projeto compila sem *warnings* com `-Wall -Wextra`.
