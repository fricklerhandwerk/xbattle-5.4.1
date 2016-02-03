#include <stdio.h>

#include "constant.h"
  
/**** x include files ****/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

#if USE_LONGJMP
#include <setjmp.h>
#endif

#include "extern.h"


/******************************************************************************
  my_error_handler (my_display, my_error)

  Handle those pesky errors which used to completely destroy the game.  Now
  they cause the game to dump the current board state, then exit gracefully.
******************************************************************************/

my_error_handler (my_display, my_error)
  Display *my_display;
  XErrorEvent *my_error;
{
  char message[MAX_LINE];

  /** Grab the system error message and print it **/

  XGetErrorText (my_display, my_error->error_code, message, MAX_LINE);
  throw_message ("ERROR: ", "Unrecoverable, code %s", message);

  /** Dump the board and exit **/

  fprintf (stderr, "   dumping board to %s\n", Config->file_store_map);
  dump_board (Config->file_store_map, FALSE);
  exit (0);
}



/******************************************************************************
  my_io_error_handler (my_display)

  Attempt to recover from an error caused by someone remotely executing one
  of the xwindows.  Tries to find the window that has been eliminated, and
  removes that player from the game.  Failing that, dumps the board.
******************************************************************************/

my_io_error_handler (my_display)
  Display *my_display;
{
  int player,
      bad_player;

  static int iteration=0;

  /** If this routine gets called many times, something is seriously wrong **/

  if (iteration < 20)
    iteration++;
  else
  {
/**
    fprintf (stderr, "EXTRA FATAL ERROR: display %s\n",
                                  XDisplayName (my_display));
**/
    throw_error ("Lost X-Window connection", NULL);
  }

  /** Match the error window with a particular player **/

  bad_player = -1;
  for (player=0; player<Config->player_count; player++)
    if (XWindow[player]->display == my_display)
      bad_player = player;

  /** Print error message and dump board **/

  fprintf (stderr, "FATAL ERROR: player %d\n", bad_player);
  fprintf (stderr, "    dumping board to %s\n", Config->file_store_map);
  dump_board (Config->file_store_map, FALSE);

  /** Continue only if error window found, disabling that player **/

  if (bad_player < 0)
    throw_error ("Cannot find resolve display death", NULL);
  else
    XWindow[bad_player]->open = FALSE;

  /** Return, in whatever manner is selected, to the main program **/

#if USE_LONGJMP
  longjmp (Config->saved_environment, 1);
  return (0);
#endif

#if USE_PROCEDURE
#if USE_UNIX
  run_unix_loop ();
#else
  run_generic_loop ();
#endif
#endif

#if !USE_FATAL_RECOVER
  exit (0);
#endif
}



/******************************************************************************
  throw_error (control_string, parameter_string)

  Print error message defined by <control_string> with argument
  <parameter_string> and exit the program.
******************************************************************************/

throw_error (control_string, parameter_string)
  char *control_string,
       *parameter_string;
{
  throw_message ("ERROR: ", control_string, parameter_string);
  exit (0);
}



/******************************************************************************
  throw_warning (control_string, parameter_string)

  Print warning message defined by <control_string> with argument
  <parameter_string>.
******************************************************************************/

throw_warning (control_string, parameter_string)
  char *control_string,
       *parameter_string;
{
  throw_message ("WARNING: ", control_string, parameter_string);
}



/******************************************************************************
  throw_message (type_string, control_string, parameter_string)

  Print generic message with prefix <type_string>, as defined by
  <control_string> with argument <parameter_string>.
******************************************************************************/

throw_message (type_string, control_string, parameter_string)
  char *type_string,
       *control_string,
       *parameter_string;
{
  char full_string[200],
       temp_string[200];

  if (parameter_string == NULL)
    sprintf (full_string, "%s%s\n", type_string, control_string);
  else
  {
    sprintf (temp_string, "%s%s\n", type_string, control_string);
    sprintf (full_string, temp_string, parameter_string);
  }

  fprintf (stderr, full_string);
  fflush (stderr);
}
