#include "pf_all.h"
#include "raylib.h"
#include <string.h>

/* Direct stack access. Words registered with NumParams = 0 pop
** their own arguments; the interpreter runs SAVE_REGISTERS before
** ID_CALL_C (pf_inner.c), so both stack pointers in gCurrentTask
** are valid here. POP order is reverse of the stack comment:
** last-listed item comes off first. */
#define POP  POP_DATA_STACK
#define PUSH PUSH_DATA_STACK
#define FP_POP (*(gCurrentTask->td_FloatStackPtr++))
#define FP_STACK_DEPTH (gCurrentTask->td_FloatStackBase - gCurrentTask->td_FloatStackPtr)

/* Forth ( addr len ) -> C string. Ring of buffers so one call
** can take several strings without clobbering. */
static const char *fs2c( cell_t addr, cell_t len )
{
    static char bufs[4][512];
    static int i = 0;
    char *b = bufs[i];
    i = (i + 1) & 3;
    if( len > 511 ) len = 511;
    if( len < 0 ) len = 0;
    memcpy( b, (const char *) addr, (size_t) len );
    b[len] = '\0';
    return b;
}

/* Refuse to pop from a stack that doesn't hold enough values.
** Prints a message and skips the raylib call instead of reading
** garbage / segfaulting. */
static void rlNeed( const char *word, int nData, int nFloat )
{
    if( DATA_STACK_DEPTH < nData )
    {
        MSG("PANIC: "); MSG(word);
        MSG_NUM_D(" needs ", nData);
        MSG_NUM_D(" values on the data stack, has ", DATA_STACK_DEPTH);
        EXIT(1);
    }
    if( FP_STACK_DEPTH < nFloat )
    {
        MSG("PANIC: "); MSG(word);
        MSG_NUM_D(" needs ", nFloat);
        MSG_NUM_D(" values on the FLOAT stack, has ", FP_STACK_DEPTH);
        EXIT(1);
    }
}
#define NEED(word, nData, nFloat) rlNeed(word, nData, nFloat)

/****************************************************************
** Step 1: Put your own special glue routines here
**     or link them in from another file or library.
****************************************************************/
/* ( w h addr len -- ) */
static void rlInitWindow(void) {
  NEED("RL.INIT-WINDOW", 4, 0);
  cell_t len = POP, addr = POP, h = POP, w = POP;
  InitWindow((int)w, (int)h, fs2c(addr, len));
}

/* ( -- ) */
static void rlCloseWindow(void) {
  CloseWindow();
}

/* ( -- flag ) */
static void rlWindowShouldClose(void) {
  PUSH(WindowShouldClose() ? -1 : 0);
}

/* ( fps -- ) */
static void rlSetTargetFPS(void) {
  SetTargetFPS((int)POP);
}

/* ( -- ) */
static void rlBeginDrawing(void) {
  BeginDrawing();
}

/* ( -- ) */
static void rlEndDrawing(void) {
  EndDrawing();
}

/* ( r g b a -- ) */
static void rlClearBackground(void) {
  cell_t a = POP, b = POP, g = POP, r = POP;
  ClearBackground((Color){
    (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a
  });
}

/* ( x y w h r g b a -- ) */
static void rlDrawRectangle(void) {
  cell_t a = POP, b = POP, g = POP, r = POP;
  cell_t h = POP, w = POP, y = POP, x = POP;
  DrawRectangle((int)x, (int)y, (int)w, (int)h, (Color){
    (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a
  });
}

/* ---- Step 2: CustomFunctionTable ---------------------------- */
/* Do not change the name; the pForth kernel uses it.
** Order here MUST match the index order in Step 3. */
typedef struct {
    const char *name;
    CFunc0      func;
} RlWord;

static const RlWord rlWords[] = {
  { "RL.INIT-WINDOW", (CFunc0) rlInitWindow},
  { "RL.CLOSE-WINDOW",(CFunc0) rlCloseWindow},
  { "RL.WINDOW-SHOULD-CLOSE",(CFunc0) rlWindowShouldClose},
  { "RL.SET-FPS",(CFunc0) rlSetTargetFPS},
  { "RL.BEGIN-DRAWING",(CFunc0) rlBeginDrawing},
  { "RL.END-DRAWING", (CFunc0) rlEndDrawing},
  { "RL.CLEAR-BACKGROUND",(CFunc0) rlClearBackground},
  { "RL.DRAW-RECTANGLE", (CFunc0) rlDrawRectangle},
};

/* The kernel's table (name is sacred), filled by the loop below. */
CFunc0 CustomFunctionTable[ sizeof(rlWords) / sizeof(rlWords[0]) ];
 
/* Called by the kernel at EVERY startup (name is sacred). */
Err LoadCustomFunctionTable( void ) {
    int i;
    int count = sizeof(rlWords) / sizeof(rlWords[0]);
    for( i = 0; i < count; i++ )
        CustomFunctionTable[i] = rlWords[i].func;
    return 0;
}

/* ---- Step 3: register Forth words --------------------------- */
/* Do not change the name; the pForth kernel calls it.
** CreateGlueToC( NAME-IN-UPPER-CASE, index, return mode, #params )
** Indices must march in lockstep with the table above —
** this is also why the .dic must be rebuilt when this file
** changes (the dictionary stores these indices). */

/* Called by the kernel only during -i dictionary builds
** (name is sacred). Index = position in rlWords. */
Err CompileCustomFunctions( void ) {
    int i;
    Err err;
    int count = sizeof(rlWords) / sizeof(rlWords[0]);
    for( i = 0; i < count; i++ ) {
        err = CreateGlueToC( rlWords[i].name, i, C_RETURNS_VOID, 0 );
        if( err < 0 ) return err;
    }
    return 0;
}
