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
  store_draw_cell (cell)

  Draw <cell> to the currently open store file.  Actually, just store enough
  information for replay can show the drawing.
******************************************************************************/

store_draw_cell (cell)
  cell_type *cell;
{
  int i,
      side; 
  s_char side_first, value_first,
         side_second, value_second;

  /** Store basic cell information **/
               
  fwrite (&(cell->x),		sizeof(s_char), 1, Config->fp);
  fwrite (&(cell->y),		sizeof(s_char), 1, Config->fp);
  fwrite (&(cell->level),	sizeof(s_char), 1, Config->fp);
  fwrite (&(cell->redraw_status), sizeof(s_char), 1, Config->fp);
  if (cell->redraw_status == REDRAW_BLANK)
    return;

  fwrite (&(cell->side), sizeof(s_char), 1, Config->fp);

  /** If a fighting cell, have to store extra info **/

  if (cell->side == SIDE_FIGHT)
  {
    /** Find two strongest sides **/

    side_first = -1;
    side_second = -1;
    value_first = 0;
    for (side=0; side<Config->side_count; side++)
    {  
      if (cell->value[side] > value_first)
      {
        value_second = value_first;
        side_second = side_first;

        value_first = cell->value[side];
        side_first = side;
      }
      else if (cell->value[side] > value_second)
      {
        value_second = cell->value[side];
        side_second = side;
      }
    }  

    /** Store info on two strongest sides **/

    fwrite (&side_first,	sizeof(s_char), 1, Config->fp);
    fwrite (&value_first,	sizeof(s_char), 1, Config->fp);
    fwrite (&side_second,	sizeof(s_char), 1, Config->fp);
    fwrite (&value_second,	sizeof(s_char), 1, Config->fp);
  }
  else if (cell->side != SIDE_NONE)
    fwrite (&(cell->value[cell->side]), sizeof(s_char), 1, Config->fp);

  /** Store town information **/

  fwrite (&(cell->angle), sizeof(short), 1, Config->fp);
  if (cell->angle > 0)
  {
    if (cell->angle < ANGLE_FULL)
      fwrite (&(cell->old_growth), sizeof(s_char), 1, Config->fp);
    else
      fwrite (&(cell->growth), sizeof(s_char), 1, Config->fp);
  }

  /** Store direction vector information **/

  for (i=0; i<Config->direction_count; i++)
    fwrite (&(cell->dir[i]), sizeof(s_char), 1, Config->fp);
}



/******************************************************************************
  replay_game ()

  Load and replay an entire game from the previously opened Config->fp.
******************************************************************************/

replay_game ()
{
  int i, j,
      side,
      player,
      ipos, jpos,
      count,
      done, mini_done,
      textcount;

  char type,
       text[10];

  s_char x, y;
  s_char side_first, value_first,
         side_second, value_second;

  KeySym key;

  cell_type *cell;

  XEvent event;

  done = FALSE;

  /** While there are still load-draws left to process **/

  for (count=0; !done; count++)
  {
    /** This is the hack to get REPLAY message to work.  Note that the	**/
    /** number of loops to drawtime (500 below) appears to have an	**/
    /** allowable minimum of 2 or 3 hundred.				**/

    /** Draw the REPLAY message at the bottom of the screen **/

    if (count == 500)
    {
      for (player=0; player<Config->player_count; player++)
      {
#if USE_MULTITEXT
        for (side=0; side<Config->side_count; side++)
          XDrawImageString(XWindow[player]->display,XWindow[player]->window,
		XWindow[player]->hue_terrain[0],
		Config->text_offset, XWindow[player]->text_y_pos[side],
		Config->message_all,strlen(Config->message_all));
#else
        XDrawImageString(XWindow[player]->display,XWindow[player]->window,
		XWindow[player]->hue_terrain[0], Config->text_offset,
		XWindow[player]->text_y_pos[0],
		Config->message_all,strlen(Config->message_all));
        XDrawImageString(XWindow[player]->display,XWindow[player]->window,
		XWindow[player]->hue_terrain[0], Config->text_offset,
		XWindow[player]->text_y_pos[1],
		Config->message_all,strlen(Config->message_all));
#endif
        XSync (XWindow[player]->display, 0);
        XFlush(XWindow[player]->display);
      }
    }

    /** Load the cell information **/

    fread (&x, sizeof(s_char), 1, Config->fp);

    if (x == REPLAY_TERMINATE)
    {
      for (player=0; player<Config->player_count; player++)
        remove_player (player);

      exit (0);
    }

    fread (&y, sizeof(s_char), 1, Config->fp);
    cell = CELL2(x,y);

    fread (&(cell->level), sizeof(s_char), 1, Config->fp);
    fread (&(cell->redraw_status), sizeof(s_char), 1, Config->fp);

    if (cell->redraw_status == REDRAW_BLANK)
    {
    }
    else
    {
      fread (&(cell->side), sizeof(s_char), 1, Config->fp);

      /** If fighting cell, load appropriate info **/

      if (cell->side == SIDE_FIGHT)
      {
        fread (&side_first,	sizeof(s_char), 1, Config->fp);
        fread (&value_first,	sizeof(s_char), 1, Config->fp);
        fread (&side_second,	sizeof(s_char), 1, Config->fp);
        fread (&value_second,	sizeof(s_char), 1, Config->fp);

        for (i=0; i<Config->side_count; i++)
          cell->value[i] = 0;

        cell->value[side_first] = value_first;
        cell->value[side_second] = value_second;
      }  
      else if (cell->side != SIDE_NONE)
        fread (&(cell->value[cell->side]), sizeof(s_char), 1, Config->fp);

      fread (&(cell->angle), sizeof(short), 1, Config->fp);

      /** If town, load appropriate info **/

      if (cell->angle > 0)
      {
        if (cell->angle < ANGLE_FULL)
          fread (&(cell->old_growth), sizeof(s_char), 1, Config->fp);
        else
          fread (&(cell->growth), sizeof(s_char), 1, Config->fp);
      }

      for (i=0; i<Config->direction_count; i++)
        fread (&(cell->dir[i]), sizeof(s_char), 1, Config->fp);
    }

    /** For each player (window) **/

    for (player=0; player<Config->player_count; player++)
    {
      /** Process any XEvents **/

      while (XEventsQueued(XWindow[player]->display, QueuedAfterReading))
      {
        XNextEvent (XWindow[player]->display, &event);
        switch (event.type)
        {
          /** Process keystroke to see if player wants to quit **/

          case KeyPress:

            mini_done = FALSE;
            while (!mini_done)
            {
              XNextEvent (XWindow[player]->display, &event);
              if (event.type == KeyPress)
              {
                textcount = XLookupString ((XKeyEvent *)&event,
				text, 10, &key, NULL);
                if (textcount != 0 && player == 0)
                {
                  if (text[0] == CTRL_C || text[0] == CTRL_Q)
                  {
                    for (i=0; i<Config->player_count; i++)
                      close_xwindow (XWindow[i]);
                    exit (0);
                  }
                }
                mini_done = TRUE;
              }
            }
            break;
        }
      }

      /** Draw the cell **/

      draw_cell (cell, player, TRUE);

      /** Sync every so often **/

      if ((count % REPLAY_UPDATE) == 0)
        XSync (XWindow[player]->display, 0);
    }
  }

  while (TRUE);
}


/******************************************************************************
  store_parameters ()

  Store relevant game parameters to the previously opened Config->fp.
******************************************************************************/

store_parameters ()
{
  int i,
      side;

  fwrite (&Config->board_x_size,	sizeof(short), 1, Config->fp);
  fwrite (&Config->board_y_size,	sizeof(short), 1, Config->fp);
  fwrite (&Config->level_min,		sizeof(int), 1, Config->fp);
  fwrite (&Config->level_max,		sizeof(int), 1, Config->fp);
  fwrite (&Config->enable_all[OPTION_HEX], sizeof(n_char), 1, Config->fp);
  fwrite (&Config->value_int[OPTION_CELL][0], sizeof(int), 1, Config->fp);
  fwrite (&Config->side_count,		sizeof(short), 1, Config->fp);

  /** Store the whole palette **/

  fwrite (&Config->hue_count, sizeof(short), 1, Config->fp);
  for (i=0; i<Config->hue_count; i++)
    fwrite (Config->palette[i], sizeof(int), 3, Config->fp);

  for (side=0; side<Config->side_count; side++)
  {
    fwrite (&Config->side_to_hue[side],		sizeof(short), 1, Config->fp);
    fwrite (&Config->hue_to_inverse[side],	sizeof(short), 1, Config->fp);
    fwrite (&Config->side_to_letter[side][0],	sizeof(char), 1, Config->fp);
  }
}



/******************************************************************************
  load_parameters ()

  Load relevant game parameters from the previously opened Config->fp.
******************************************************************************/

load_parameters ()
{
  int i,
      side,
      player;

  int cell_size;

  char line[MAX_LINE],
       dummy;

  fread (&Config->board_x_size,		sizeof(short), 1, Config->fp);
  fread (&Config->board_y_size,		sizeof(short), 1, Config->fp);
  fread (&Config->level_min,		sizeof(int), 1, Config->fp);
  fread (&Config->level_max,		sizeof(int), 1, Config->fp);
  fread (&Config->enable_all[OPTION_HEX], sizeof(n_char), 1, Config->fp);
  fread (&cell_size,			sizeof(int), 1, Config->fp);
  fread (&Config->side_count,		sizeof(short), 1, Config->fp);

  /** Reconcile options with command line **/

  if (Config->level_min < 0)
    Config->enable_all[OPTION_SEA] = TRUE;

  if (Config->level_max > 0)
  {
    if (!Config->enable_all[OPTION_HILLS] &&
			!Config->enable_all[OPTION_HILLS])
      Config->enable_all[OPTION_HILLS] = TRUE;
  }

  Config->value_int_all[OPTION_HILL_TONES] = Config->level_max+1;
  Config->value_int_all[OPTION_FOREST_TONES] = Config->level_max+1;
  Config->value_int_all[OPTION_SEA_TONES] = -Config->level_min;

  for (player=0; player<Config->player_count; player++)
  {
    side = Config->player_to_side[player]; 
    if (!Config->enable[OPTION_CELL][side])
    {
      Config->value_int[OPTION_CELL][side] = cell_size;
      Config->cell_size[side] = cell_size;
    }
  }

  /** Load the whole palette **/

  fread (&Config->hue_count, sizeof(short), 1, Config->fp);
  for (i=0; i<Config->hue_count; i++)
    fread (Config->palette[i], sizeof(int), 3, Config->fp);

  for (side=0; side<Config->side_count; side++)
  {
    fread (&Config->side_to_hue[side],		sizeof(short), 1, Config->fp);
    fread (&Config->hue_to_inverse[side],	sizeof(short), 1, Config->fp);
    fread (&Config->side_to_letter[side][0],	sizeof(char), 1, Config->fp);
  }
}


/******************************************************************************
  game_stats ()

  Generate some elementary statistics on the state of the game and display
  them to stdout.
******************************************************************************/

game_stats ()
{
  int i, j,
      side,
      force[MAX_SIDES],
      coverage[MAX_SIDES];

  for (side=0; side<Config->side_count; side++)
  {
    force[side] = 0;
    coverage[side] = 0;
  }

  /** Compute the number of troops and cells for each side **/

  for (i=0; i<Config->board_x_size; i++)
    for (j=0; j<Config->board_y_size; j++)
    {
      side = CELL2(i,j)->side;
      if (side != SIDE_FIGHT && side != SIDE_NONE)
      {
        force[side] += CELL2(i,j)->value[side];
        coverage[side]++;
      }
    }

  /** Print statistics to stdout **/

  for (side=0; side<Config->side_count; side++)
    printf ("%s:  %3d cells   %5d force\n", Config->side_to_hue_name[side],
			coverage[side], force[side]);
  printf ("\n");
}
