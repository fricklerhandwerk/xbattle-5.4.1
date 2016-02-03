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
#include "edit.h"

/******************************************************************************
  edit_board ()

  Allows the user to interactively edit the board, placing troops, terrain,
  bases, and towns as desired.
******************************************************************************/

edit_board ()
{
  XEvent event;

  int player,
      current_side,
      base_side,
      value,
      textcount,
      tdir[MAX_DIRECTIONS];

  short max_value;

  char text[20];

  KeySym key;

  cell_type *cell;

  player = 0;
  current_side = Config->player_to_side[player];
  base_side = current_side;

  /** Draw the baseline board **/

  draw_board (player, TRUE);

  /** Loop forever **/

  while (TRUE)
  {
    XNextEvent (XWindow[player]->display, &event);

    cell = get_cell (event.xbutton.x, event.xbutton.y, tdir,
			base_side, FALSE);

    /** Find out what type of action to take **/

    switch (event.type)
    {
      /** Redraw whole board **/

      case Expose:
        draw_board (player, TRUE);
        break;

      /** Add terrain **/

      case ButtonPress:
        /** Decrement/increment terrain with left and middle buttons **/

        if (event.xbutton.button == Button1)
          cell->level -= 1;
        else if (event.xbutton.button == Button2)
          cell->level += 1;
        else
        {
          /** Else clear to default terrain with right button **/

          cell->level = 0;
        }

        /** Make sure terrain level within proper bounds **/

        if (cell->level < Config->level_min)
          cell->level = Config->level_min;
        else if (cell->level > Config->level_max)
          cell->level = Config->level_max;

        if (cell->level < 0)
        {
          if (cell->side != SIDE_NONE)
          {
            cell->value[cell->side] = 0;
            cell->side = SIDE_NONE;
          }

          cell->growth = 0;
          cell->old_growth = 0;
          cell->angle = 0;
        }

        /** Need to redraw entire polygon, not just erase contents **/

        cell->redraw_status = REDRAW_FULL;

        /** Draw the newly altered cell **/

        draw_cell (cell, player, TRUE);

        cell->redraw_status = REDRAW_NORMAL;
        break;

      case KeyPress:
        
        textcount = XLookupString((XKeyEvent *)&event, text, 10, &key, NULL);
        if (textcount == 0)
          break;

        switch (text[0])
        {
          /** Create city, town, or village **/

          case EDIT_CREATE_CITY:
          case EDIT_CREATE_TOWN:
          case EDIT_CREATE_VILLAGE:

            if (cell->level < 0)
              break;
          
            if (text[0] == EDIT_CREATE_CITY)
              cell->growth = TOWN_MAX;
            else if (text[0] == EDIT_CREATE_TOWN)
              cell->growth = (TOWN_MAX-TOWN_MIN)/2 + TOWN_MIN;
            else
              cell->growth = TOWN_MIN;

            cell->old_growth = cell->growth;
            cell->angle = ANGLE_FULL;
            break;

          /** Partially scuttle a town **/

          case EDIT_SCUTTLE_TOWN:

            if (cell->old_growth > TOWN_MIN)
            {
              cell->growth = 0;
              cell->angle -= EDIT_SCUTTLE_STEP;

              if (cell->angle < ANGLE_ROUND_DOWN)
              {
                cell->growth = 0;
                cell->old_growth = 0;
                cell->angle = 0;
              }
            }
            break;

          /** Partially build a town **/

          case EDIT_BUILD_TOWN:

            if (cell->old_growth > TOWN_MIN)
            {
              cell->angle += EDIT_BUILD_STEP;

              if (cell->angle > ANGLE_ROUND_UP)
              {
                cell->growth = cell->old_growth;
                cell->angle = ANGLE_FULL;
              }
            }
            break;

          /** Shrink a town **/

          case EDIT_SHRINK_TOWN:

            if (cell->growth > TOWN_MIN)
            {
              cell->growth *= EDIT_SHRINK_FACTOR;
              cell->old_growth = cell->growth;
            }

            if (cell->growth < TOWN_MIN)
            {
              cell->growth = 0;
              cell->old_growth = 0;
              cell->angle = 0;
            }
            break;

          /** Enlarge a town **/

          case EDIT_ENLARGE_TOWN:

            if (cell->growth > TOWN_MIN)
            {
              cell->growth *= EDIT_ENLARGE_FACTOR;
              cell->old_growth = cell->growth;
            }

            if (cell->growth > TOWN_MAX)
            {
              cell->growth = TOWN_MAX;
              cell->old_growth = TOWN_MAX;
            }
            break;

          /** Rotate active side to next side **/

          case EDIT_ROTATE_SIDE:

            current_side += 1;
            if (current_side >= Config->side_count)
              current_side = 0;
            break;

          /** Rotate troop in cell to next side **/

          case EDIT_ROTATE_TROOPS:

            if (cell->side != SIDE_NONE)
            {
              value = cell->value[cell->side];
              cell->value[cell->side] = 0;
              cell->side += 1;
              if (cell->side >= Config->side_count)
                cell->side = 0;
              cell->value[cell->side] = value;
            }
            break;

          /** Add (X-'0') troops of current_side to cell **/

          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
          case '0':
          case EDIT_INCREMENT_TROOPS:
          case EDIT_DECREMENT_TROOPS:

            if (cell->level < 0)
              break;

            if (cell->side == SIDE_NONE)
              cell->side = current_side;

            max_value =
		Board->shapes[cell->side][cell->shape_index]->max_value;

            if (text[0] == EDIT_INCREMENT_TROOPS)
              cell->value[cell->side] += 1;
            else if (text[0] == EDIT_DECREMENT_TROOPS)
            {
              if (cell->value[cell->side] > 0)
                cell->value[cell->side] -= 1;
            }
            else
              cell->value[cell->side] =
		(text[0] - '0')*max_value/9;


            if (cell->value[cell->side] == 0)
              cell->side = SIDE_NONE;
            else if (cell->value[cell->side] > max_value)
                cell->value[cell->side] = max_value;
            break;

          /** Dump the board to a file **/

          case EDIT_DUMP_BOARD:

            dump_board (Config->file_store_map, Config->use_brief_load);
            draw_board (player, TRUE);
            break;

          /** Exit **/

          case EDIT_EXIT:

            exit (0);
            break;
        }

        /** Draw the newly altered cell **/

        if (cell != NULL)
          draw_cell (cell, player, TRUE);

        break;
    }
  }
}
