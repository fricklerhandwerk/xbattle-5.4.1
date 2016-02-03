

#include "constant.h"
  
/**** x include files ****/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

#if FIXED_USE_LONGJMP
#include <setjmp.h>
#endif

#include "extern.h"

char Blank[] = {"                                                                                                                                                                     "};

/**** global variables ****/

xwindow_type		*XWindow[MAX_PLAYERS];
config_info		*Config;
board_type		*Board;
