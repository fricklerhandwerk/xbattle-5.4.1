#include <stdio.h>
  
/**** x include files ****/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

#if USE_LONGJMP
#include <setjmp.h>
#endif

#include "global.h"

#if USE_UNIX
#include <sys/types.h>
#include <sys/time.h>
#endif

#include "keyboard.h"


/******************************************************************************
  main (argc,argv)

  Orchestrates the whole shebang.
******************************************************************************/

main (argc,argv)
  int argc;
  char *argv[];
{ 
  int my_error_handler(),
      my_io_error_handler();

  /** Initialize default values **/

  init_defaults();

  /** Parse the command line **/

  load_options (argc, argv);

  /** Initialize the entire board **/

  init_board ();

  /** Set up global XWindows[] **/

  set_windows ();

  /** Edit the board, if edit enabled **/

  if (Config->enable_all[OPTION_EDIT])
    edit_board ();

  /** Replay a game, if replay enabled **/

  if (Config->enable_all[OPTION_REPLAY])
    replay_game ();

  /** Set up some error handler stuff **/

  XSetErrorHandler (my_error_handler);
  XSetIOErrorHandler (my_io_error_handler);

  /** Run the appropriate event loop **/

#if USE_UNIX
  run_unix_loop ();
#else
  run_generic_loop ();
#endif
}

  
#if USE_UNIX
/******************************************************************************
  run_unix_loop ()

  Controls the event loop which is the basis of xbattle, handling X events
  and board updates as necessary.  Assumes the host actually is running Unix,
  using several important timing functions and error recovery schemes.
******************************************************************************/

run_unix_loop ()
{
  int player,
      update_timer;

  XEvent event;

  int fd,
      maxfd;
  int selectback;
  unsigned long new_time,
                old_time,
                extra_time,
                target_time,
                running_time;
  fd_set rfds;
  struct timeval tv_timeout,
                 tv_current,
                 tv_timer_base;
  struct timezone tz_timezone;

  /** Set up time between updates **/

  extra_time = (Config->delay/10000)*1000000 + (Config->delay%10000)*100;

  /** Set up FD stuff which we'll use for generation of timing signals **/
  
  maxfd = 0;
  FD_ZERO (&(Config->disp_fds));

  for (player=0; player<Config->player_count; player++)
  {
    if (XWindow[player]->open)
    {
      if ((fd = ConnectionNumber (XWindow[player]->display)) > maxfd)
        maxfd = fd;
      FD_SET (fd, &(Config->disp_fds)); 
    }
  }  

#if USE_TIMER
  running_time = 0;
  update_timer = FALSE;
  gettimeofday (&tv_timer_base, &tz_timezone);
#endif

  /** Establish baseline time **/

  gettimeofday (&tv_current, &tz_timezone);
  old_time = (tv_current.tv_sec%10000)*1000000 + tv_current.tv_usec;
  target_time = old_time + extra_time;

#if USE_LONGJMP
  setjmp (Config->saved_environment);
#endif

  /** Loop forever **/

  while (TRUE)
  {
    /** Initialize a timeout event and wait for any event **/

    rfds = Config->disp_fds;
    tv_timeout.tv_sec = (target_time-old_time)/1000000;
    tv_timeout.tv_usec = (target_time-old_time)%1000000;

    selectback = select (maxfd+1, &rfds, NULL, NULL, &tv_timeout);

    /** Mark the time when event received **/

    gettimeofday (&tv_current, &tz_timezone);
    new_time = (tv_current.tv_sec%10000)*1000000 + tv_current.tv_usec;

    if (new_time < old_time)
      target_time = new_time - 1;

    /** Coarsely process the event to determine what type it is **/

    switch (selectback)
    {
      case -1:

        perror ("select()");
        exit (1);
        break;

      /** A real X event **/

      case 0:

#if USE_TIMER
        if (tv_current.tv_sec - tv_timer_base.tv_sec != running_time)
        {
          running_time = tv_current.tv_sec - tv_timer_base.tv_sec;
          update_timer = TRUE;
        }
        else
          update_timer = FALSE;
#endif
        /** For each player with open window **/

        for (player=0; player<Config->player_count; player++)
        {
          if (XWindow[player]->open)
          {
#if USE_TIMER
            if (update_timer)
              draw_timer (running_time, player);
#endif
            /** Process all queued X events **/

            XFlush (XWindow[player]->display);
            while (XWindow[player]->open &&
		(XEventsQueued (XWindow[player]->display,QueuedAfterReading)>0))
            {
	      XNextEvent (XWindow[player]->display, &event);
	      process_event (event, player);
            }

#if USE_MULTIFLUSH
            if (XWindow[player]->open)
              XFlush (XWindow[player]->display);
#endif
          }
        }
        break;

      /** Some type of other select() event? **/

      default:

        /** For each player with open window **/

        for (player=0; player<Config->player_count; player++)
        {
          if (XWindow[player]->open)
          {
            if (FD_ISSET((fd=ConnectionNumber(XWindow[player]->display)),
				&rfds))
            {
              while (XWindow[player]->open &&
				(XEventsQueued (XWindow[player]->display,
				QueuedAfterReading)>0))
              {
	        XNextEvent (XWindow[player]->display, &event);
	        process_event (event, player);
              }

#if USE_MULTIFLUSH
              XFlush (XWindow[player]->display);
#endif

              if (!XWindow[player]->open)
                FD_CLR (fd, &(Config->disp_fds));
            }
          }
        }
        break;
    }

    /** Set new target time for timeout **/

    if (new_time > target_time)
    {
      if (!Config->enable_all[OPTION_STEP])
        update_board ();
      target_time = new_time + extra_time;
    }
    old_time = new_time;  

#if USE_TIMER
    if (update_timer)
      update_timer = FALSE;
#endif
  }
}



#else
/******************************************************************************
  run_generic_loop ()

  Controls the event loop which is the basis of xbattle, handling X events
  and board updates as necessary.  Uses a brute force delay method to arrive
  at roughly synchronous board updates.
******************************************************************************/

run_generic_loop ()
{
  int i, t,
      player;

  XWindowAttributes attribs;

  XEvent event;

#if USE_LONGJMP
  setjmp (Config->saved_environment);
#endif

  t = 0;
  while (TRUE)
  {
    /** Check for events **/

    for (player=0; player<Config->player_count; player++)
    {
      if (!XWindow[player]->open)
        continue;

      if (XEventsQueued (XWindow[player]->display, QueuedAfterReading) > 0)
      {
	XNextEvent (XWindow[player]->display, &event);
	process_event (event, player);
      }
      else
        XFlush (XWindow[player]->display);
    }
    
    /** Update the game board **/

    if (t++ > Config->delay)
    {
      t = 0;
      update_board ();
    }
  }
}
#endif



/******************************************************************************
  process_event (event, player)

  Handles each <event> from <player>, regardless of type.
******************************************************************************/

process_event (event, player)
  XEvent event;
  int player;
{
  int i, j,
      side,
      textcount,
      tdir[MAX_DIRECTIONS],
      xlow, xhigh, ylow, yhigh,
      control,
      is_shifted;
  char text[10];

  KeySym key;

  Atom wm_protocols, wm_delete_window;

  XEvent dummy_event;

  cell_type *cell;

  is_shifted = FALSE;
  side = Config->player_to_side[player];
  
  /** Process event **/

  switch (event.type)
  {
    case GraphicsExpose:
    case NoExpose:
      break;

    /** Screen has been exposed **/

    case Expose:
      while (XCheckMaskEvent (event.xexpose.display, ExposureMask,
							 &dummy_event));

      draw_board (player, FALSE);
      break;
    
    /** Mouse button has been pressed **/

    case ButtonPress:

      if (XWindow[player]->watch)
        break;

#if USE_PAUSE
      if (Config->is_paused)
        break;
#endif
      
      switch (event.xbutton.button)
      {
        /** Left or middle button **/

        case Button1:
        case Button2:
          is_shifted = event.xbutton.state & ShiftMask;

          /** If shift down, run march command **/

          if (Config->enable[OPTION_MARCH][side] && is_shifted)
          {
            cell = get_cell (event.xbutton.x, event.xbutton.y, tdir,
				side, is_shifted);

            if (cell == NULL)
              break;
				
            run_march (cell, player, side, cell->x, cell->y,
				event.xbutton.button, tdir);
            break;
          }

          /** Get cell and direction vector array **/

          cell = get_cell (event.xbutton.x, event.xbutton.y,
		Config->dir[player], side, is_shifted);

          if (cell == NULL)
            break;

          /** Set corner coords if bound enabled **/

          if (Config->enable[OPTION_BOUND][side])
          {
            Config->old_x[player] = cell->x;
            Config->old_y[player] = cell->y;
            break;
          }
        
          /** Install direction vectors if appropriate **/

          if (cell->side == side ||
		(!Config->enable[OPTION_DISRUPT][side] &&
		cell->side==SIDE_FIGHT &&
		cell->old_side == side))
          {
#if USE_ALT_MOUSE
            if (event.xbutton.button == Button1)
            {
	      set_move_on (cell, Config->dir[player], 0);
              Config->dir_type[player] = MOVE_SET;
            }
            else
            {
	      set_move_off (cell, Config->dir[player], 0);
              Config->dir_type[player] = MOVE_FORCE;
            }
#else
            if (event.xbutton.button == Button1)
            {
	      set_move (cell, Config->dir[player], 0);
              Config->dir_type[player] = MOVE_SET;
            }
            else
            {
	      set_move_force (cell, Config->dir[player], 0);
              Config->dir_type[player] = MOVE_FORCE;
            }
#endif
            Config->dir_factor[player] =
		Board->shapes[side][cell->shape_index]->direction_factor;

            /** Draw the cell to each display **/

            draw_multiple_cell (cell);
          }

          /** If cell had a march command, kill it **/

          if (Config->enable[OPTION_MARCH][side])
            if (cell->march[side] || cell->any_march)
            {
              if (cell->any_march == MARCH_ACTIVE &&
			cell->march_side == side)
              {
                cell->any_march = FALSE;
                cell->march[side] = FALSE;
              }
              else
              {
                cell->march[side] = FALSE;
                cell->outdated = OUTDATE_ALL;
                draw_multiple_cell (cell);
              }
            }

          break;

        /** Right button **/
  
        case Button3:

          /** If artillery, paratroops, or repeat is enabled **/
  
          if (Config->enable[OPTION_ARTILLERY][side] ||
			Config->enable[OPTION_PARATROOPS][side] ||
			Config->enable[OPTION_REPEAT][side])
          {
            /** Get cell and direction vector array **/

            cell = get_cell (event.xbutton.x, event.xbutton.y, tdir,
			side, is_shifted);
            if (cell == NULL)
              break;

            /** If side occupies cell **/

            if (cell->side == side)
            {
              is_shifted = event.xbutton.state & ShiftMask;
              control = event.xbutton.state & ControlMask;

              /** If shift down, run paratroops **/

              if (is_shifted && Config->enable[OPTION_PARATROOPS][side])
                run_shoot (cell, player, event.xbutton.x, event.xbutton.y,
					TRUE, FALSE);
              else if (control && Config->enable[OPTION_ARTILLERY][side])
              {
                /** Else if control down, run paratroops **/

                run_shoot (cell, player, event.xbutton.x, event.xbutton.y,
					TRUE, TRUE);
              }
              else if (Config->enable[OPTION_REPEAT][side]) 
              {
                /** Else repeat last direction command **/

#if USE_ALT_MOUSE
                if (Config->dir_type[player] == MOVE_SET)
	          set_move_on (cell, Config->dir[player],
					Config->dir_factor[player]);
                else
	          set_move_off (cell, Config->dir[player],
					Config->dir_factor[player]);
#else
                if (Config->dir_type[player] == MOVE_SET)
	          set_move (cell, Config->dir[player],
					Config->dir_factor[player]);
                else
	          set_move_force (cell, Config->dir[player],
					Config->dir_factor[player]);
#endif
                /** Draw the cell to each display **/

                draw_multiple_cell (cell);
              }
            }
          }
          break;
      }
      break;

    /** Mouse button has been released **/

    case ButtonRelease:

      if (!Config->enable[OPTION_BOUND][side])
        break;

      if ((Config->old_x[player] == -1) && (Config->old_y[player] == -1))
        break;

      /** Get cell and direction vector array **/

      cell = get_cell (event.xbutton.x, event.xbutton.y, tdir,
			side, is_shifted);

      if (cell == NULL)
        break;

      /** Rectify grid indices **/

      if (cell->y >= Config->board_y_size)
        cell->y = Config->board_y_size-1;
      if (cell->x >= Config->board_x_size)
        cell->x = Config->board_x_size-1;

      /** Swap corners if not left->right, top->bottom **/

      if (Config->old_x[player] < cell->x)
      {
        xlow = Config->old_x[player]; 
        xhigh = cell->x;
      }
      else
      {
        xlow = cell->x;
        xhigh = Config->old_x[player]; 
      }

      if (Config->old_y[player] < cell->y)
      {
        ylow = Config->old_y[player]; 
        yhigh = cell->y;
      }
      else
      {
        ylow = cell->y;
        yhigh = Config->old_y[player]; 
      }

      switch (event.xbutton.button)
      {
        /** Left or middle button **/

        case Button1:
        case Button2:

          /** For every cell in bounded range **/

          for (i=xlow; i<=xhigh; i++)
          {
            for (j=ylow; j<=yhigh; j++)
            {
              cell = CELL2(i,j);

              /** Install direction vectors when appropriate **/

              if (cell->side == side ||
                     (!Config->enable[OPTION_DISRUPT][side] &&
				cell->side==SIDE_FIGHT &&
				cell->old_side == side))
              {
#if USE_ALT_MOUSE
                if (event.xbutton.button == Button1)
	          set_move_on (cell, Config->dir[player], 0);
                else
	          set_move_off (cell, Config->dir[player], 0);
#else
                if (event.xbutton.button == Button1)
	          set_move (cell, Config->dir[player], 0);
                else
	          set_move_force (cell, Config->dir[player], 0);
#endif

                draw_multiple_cell (cell);

              }
            }
          }

          break;

        case Button3:
          break;
      }
      break;
  
    /** Keyboard key has been released **/

    case KeyPress:

      textcount = XLookupString ((XKeyEvent *)(&event), text, 10, &key, NULL);
      if (textcount != 0)
      {
        /** If keypress within playing board **/

        if (event.xbutton.x < XWindow[player]->size_play.x &&
        	event.xbutton.y < XWindow[player]->size_play.y)
        {
          /** Get cell and direction vector array **/

          cell = get_cell (event.xbutton.x, event.xbutton.y,
			Config->dir[player], side, is_shifted);

          if (cell == NULL)
            break;

          if (XWindow[player]->watch)
            break;
          switch (text[0])
          {
            /** Set a keyboard direction command **/

            case KEY_DIR_0:
            case KEY_DIR_1:
            case KEY_DIR_2:
            case KEY_DIR_3:
            case KEY_DIR_4:
            case KEY_DIR_5:

              if (cell->side == side)
              {
                for (i=0; i<Config->direction_count; i++)
                  Config->dir[player][i] = 0;

                switch (text[0])
                {
                  case KEY_DIR_0:
                    Config->dir[player][0] = 1;
                    break;
                  case KEY_DIR_1:
                    Config->dir[player][1] = 1;
                    break;
                  case KEY_DIR_2:
                    Config->dir[player][2] = 1;
                    break;
                  case KEY_DIR_3:
                    Config->dir[player][3] = 1;
                    break;
                  case KEY_DIR_4:
                    Config->dir[player][4] = 1;
                    break;
                  case KEY_DIR_5:
                    Config->dir[player][5] = 1;
                    break;
                }

               /** ALTER: need way to choose force/set **/

	        set_move_force (cell, Config->dir[player], 0);
                draw_multiple_cell (cell);
              }

              break;

#if USE_PAUSE
            /** Pause play **/

            case KEY_PAUSE:
              Config->is_paused = TRUE;
              break;

            /** Resume play **/

            case KEY_QUIT:
              Config->is_paused = FALSE;
              break;
#endif
            /** Attack cell **/

            case KEY_ATTACK:

              run_attack (cell, side);
              break;

            /** Zero out cell's direction vectors **/

            case KEY_ZERO:

              if (cell->side == side)
                run_zero (cell);
              break;

            /** Zero out cell's management command **/

            case KEY_UNMANAGE:

              if (cell->side == side)
                cell->manage_update = FALSE;
              break;

            /** Send out some paratroops **/

            case KEY_PARATROOPS:

              if (!Config->enable[OPTION_PARATROOPS][side])
                break;
              cell = get_cell (event.xbutton.x, event.xbutton.y, tdir,
				side, is_shifted);
              if (cell->side == side)
                run_shoot (cell, player, event.xbutton.x, event.xbutton.y,
			TRUE, FALSE);
              break;

            /** Manage some paratroops **/

            case KEY_PARATROOPS_MANAGE:

              if (!Config->enable[OPTION_PARATROOPS][side])
                break;
              if (!Config->enable[OPTION_MANAGE][side])
                break;
              cell = get_cell (event.xbutton.x, event.xbutton.y, tdir,
				side, is_shifted);

              if (cell->side == side)
              {
                if (cell->manage_update == MANAGE_PARATROOP)
                {
                  cell->manage_update = FALSE;
                }
                else
                {
                  run_shoot (cell, player, event.xbutton.x, event.xbutton.y,
				TRUE, FALSE);
                  cell->manage_dir = -1;
                  cell->manage_update = MANAGE_PARATROOP;
                }
              }
              break;

            /** Shoot an artillery shell **/

            case KEY_ARTILLERY:

              if (!Config->enable[OPTION_ARTILLERY][side])
                break;
              cell = get_cell (event.xbutton.x, event.xbutton.y, tdir,
				side, is_shifted);
              if (cell->side == side)
                run_shoot (cell, player, event.xbutton.x, event.xbutton.y,
				TRUE, TRUE);
              break;

            /** Manage shoot an artillery shell **/

            case KEY_ARTILLERY_MANAGE:

              if (!Config->enable[OPTION_ARTILLERY][side])
                break;
              if (!Config->enable[OPTION_MANAGE][side])
                break;
              cell = get_cell (event.xbutton.x, event.xbutton.y, tdir,
				side, is_shifted);
              if (cell->side == side)
              {
                if (cell->manage_update == MANAGE_ARTILLERY)
                {
                  cell->manage_update = FALSE;
                }
                else
                {
                  run_shoot (cell, player, event.xbutton.x, event.xbutton.y,
				TRUE, TRUE);
                  cell->manage_dir = -1;
                  cell->manage_update = MANAGE_ARTILLERY;
                }
              }
              break;

            /** Dig a cell **/

            case KEY_DIG:

              if (Config->enable[OPTION_DIG][side])
                if (cell->side == side && cell->value[cell->side] >
				Config->value_int_all[OPTION_DIG_COST])
                  run_dig (cell);
              break;

            /** Manage dig a cell **/

            case KEY_DIG_MANAGE:

              if (!Config->enable[OPTION_DIG][side])
                break;
              if (!Config->enable[OPTION_MANAGE][side])
                break;

              if (cell->side == side)
              {
                if (cell->manage_update == MANAGE_DIG)
                  cell->manage_update = FALSE;
                else
                {
                  cell->manage_update = MANAGE_DIG;
                  run_dig (cell);
                }
              }
              break;

            /** Fill a cell **/

            case KEY_FILL:

              if (Config->enable[OPTION_FILL][side])
                if (cell->side == side && cell->value[cell->side] >
				Config->value_int_all[OPTION_FILL_COST] &&
				cell->move <= 1)
                  run_fill (cell);
              break;

            /** Manage fill a cell **/

            case KEY_FILL_MANAGE:

              if (!Config->enable[OPTION_FILL][side])
                break;
              if (!Config->enable[OPTION_MANAGE][side])
                break;

              if (cell->side == side)
              {
                if (cell->manage_update == MANAGE_FILL)
                  cell->manage_update = FALSE;
                else
                {
                  cell->manage_update = MANAGE_FILL;
                  run_fill (cell);
                }
              }
              break;

            /** Scuttle a city **/

            case KEY_SCUTTLE:

              if (Config->enable[OPTION_SCUTTLE][side])
                if (cell->side == side)
                  run_scuttle (cell);
              break;

            /** Build a city **/
  
            case KEY_BUILD:

              if (Config->enable[OPTION_BUILD][side])
                if (cell->side == side && cell->angle < ANGLE_FULL)
                  run_build (cell, side);
              break;

            /** Manage build a city **/

            case KEY_BUILD_MANAGE:

              if (!Config->enable[OPTION_MANAGE][side])
                break;
              if (Config->enable[OPTION_BUILD][side])
                if (cell->side == side)
                {
                  if (cell->manage_update == MANAGE_CONSTRUCTION)
                    cell->manage_update = FALSE;
                  else
                  {
                    cell->manage_dir = -1;
                    cell->manage_update = MANAGE_CONSTRUCTION;
                  }
                }
              break;

            /** Reserve troops in a cell **/

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':

              if (Config->enable[OPTION_RESERVE][side])
                if (cell->side == side)
                  run_reserve (cell, player, side, text);
              break;

            case KEY_STEP:
              if (Config->enable_all[OPTION_STEP])
                update_board ();
              break;
  
            default:
              break;
          }
        }
        else
        {
          /** Else mouse not in playing board, treat keypress as message **/

          draw_message (text,textcount,side,player);
        }
      }
      break;

    /** Handle the case of an untimely killed window **/

    case ClientMessage:

      wm_protocols = XInternAtom (event.xclient.display, "WM_PROTOCOLS", False);
      wm_delete_window =
		XInternAtom (event.xclient.display, "WM_DELETE_WINDOW", False);
      if ((event.xclient.message_type == wm_protocols) && 
		(event.xclient.data.l[0] == wm_delete_window))
        remove_player (player);
      break;

    default:
      break;
  }
}



/******************************************************************************
  cell_type *
  get_cell (win_x, win_y, dir, cell_size, shift)

  Gets the grid cell corresponding to the window position (<winx>,<winy>).
  Also determine what type of direction vectors should be activated if
  <dir> is not NULL.  Returns pointer to cell.
******************************************************************************/

cell_type *
get_cell (win_x, win_y, dir, side, shift)
  int win_x, win_y,
      dir[MAX_DIRECTIONS],
      side,
      shift;
{
  int i,
      board_x, board_y,
      select_x, select_y,
      off_x, off_y;

  shape_type *shape;
  cell_type *cell, *cell2;
  select_type *select;

  /** Set selection chart **/

  select = Config->selects[side];

  /** Determine which selection block coordinate falls in **/

  board_x = (win_x /
	(select->dimension.x - select->offset.x)) * select->multiplier.x;
  board_y = (win_y /
	(select->dimension.y - select->offset.x)) * select->multiplier.y;

  /** Determine offset into selection chart **/

  select_x = win_x % (select->dimension.x - select->offset.x);
  select_y = win_y % (select->dimension.y - select->offset.x);

  /** Determine cell index offset from selection chart **/

  board_x += select->matrix[select_y][select_x].x;
  board_y += select->matrix[select_y][select_x].y;

  /** If invalid cell, return NULL **/

  if (board_x < 0 || board_y < 0 ||
	board_x >= Config->board_x_size || board_y >= Config->board_y_size)
    return (NULL);

  /** Set cell and its shape **/

  cell = CELL2 (board_x, board_y);
  shape = Board->shapes[side][cell->shape_index];

  /** Determine offset of coordinate from upper left corner of cell **/

  off_x = win_x - cell->x_center[side] + shape->center_bound.x;
  off_y = win_y - cell->y_center[side] + shape->center_bound.y;

  /** If routine should set direction vectors **/

  if (dir != NULL)
  {
    /** Zero old direction vectors **/

    for (i=0; i<Config->direction_count; i++)
      dir[i] = 0;

    /** If direction chart indicates a valid vector **/

    if (shape->chart[off_x][off_y][0] >= 0)
    {
      dir[shape->chart[off_x][off_y][0]] = 1;

      /** If secondary vector should be set **/

      if ((shift || shape->use_secondary) && shape->chart[off_x][off_y][1] >= 0)
        dir[shape->chart[off_x][off_y][1]] = 1;
    }
  }

  return (cell);
}



/******************************************************************************
  set_windows ()

  Sets up each of the global XWindow[] structures, initializing the necessary
  variables and actually opening the associate windows.
******************************************************************************/

set_windows ()
{
  int i,
      player,
      side;

  char hue_title[MAX_LINE],
       bw_title[MAX_LINE];

  shape_type *shape;
  xwindow_type *xwindow;

  /** For each player **/

  for (player=0; player<Config->player_count; player++)
  {
    xwindow = XWindow[player];

    xwindow->player = player;

    /** Set offset to playing area of window **/

    xwindow->offset_play.x = 0;
    xwindow->offset_play.y = 0;

    /** Set playing area size **/

    side = Config->player_to_side[player];
    xwindow->size_play.x = Board->size[side].x;
    xwindow->size_play.y = Board->size[side].y;

    xwindow->offset_text.x = 0;
    xwindow->offset_text.y = 2*xwindow->offset_play.y + xwindow->size_play.y;

    xwindow->size_window.x = 2*xwindow->offset_play.x + xwindow->size_play.x;
    xwindow->size_text.x = xwindow->size_window.x;

    /** Set position for text messages **/

#if USE_MULTITEXT
    for (side=0; side<Config->side_count; side++)
      xwindow->text_y_pos[side] = xwindow->offset_text.y + 12 + 16*side;

    xwindow->size_text.y = Config->side_count*Config->text_size;
#else
    xwindow->text_y_pos[0] = xwindow->offset_text.y + 12;
    xwindow->text_y_pos[1] = xwindow->offset_text.y + 28;

    xwindow->size_text.y = 2*Config->text_size;
#endif

    xwindow->size_window.y = xwindow->offset_text.y + xwindow->size_text.y;

    /** Set title at top of window **/

    side = Config->player_to_side[player];

    if (strcmp (Config->side_to_bw_name[side], Config->side_to_hue_name[side]))
    {
      sprintf (hue_title, "xbattle: %s (%s) side",
		Config->side_to_bw_name[side], Config->side_to_hue_name[side]);
      sprintf (bw_title, "xbattle: %s (%s) side",
		Config->side_to_hue_name[side], Config->side_to_bw_name[side]);
    }
    else
    {
      sprintf (hue_title, "xbattle: %s side", Config->side_to_hue_name[side]);
      sprintf (bw_title, "xbattle: %s side", Config->side_to_bw_name[side]);
    }

    if (Config->side_to_letter[side][0])
      strcpy (hue_title, bw_title);

    /** Open the window **/
 
    open_xwindow (xwindow, hue_title, bw_title);

    /** If window is b&w and using any form of terrain, must disallow	**/
    /** most ways of drawing since terrain hues are not defined.	**/

    if (xwindow->depth == 1 &&
		(Config->level_max > 0 || Config->level_min < 0))
    {
      if (Config->value_int[OPTION_DRAW][side] != DRAW_PIXMAP)
      {
        Config->value_int[OPTION_DRAW][side] = DRAW_MASKING;
        for (i=0; i<Board->shape_count; i++)
          shape_set_draw_method (Board->shapes[side][i], side, FALSE);
      }
    }
  }
}



/******************************************************************************
  remove_player (current_player)

  Completely eliminates <current_player> from the game.  This basically just
  involves freeing up all the allocated resources and setting a flag to let
  the system know not to send any more drawing commands to this display.
******************************************************************************/

remove_player (current_player)
  int current_player;
{
  int i, j,
      side,
      player,
      done,
      limit;

  char line[512],
       end_signal;

  XEvent event;

#if USE_UNIX
  int fd;
#endif

  if (XWindow[current_player]->open == FALSE)
    return (-1);

  /** Flush the remainder of the player's events **/

  while (XEventsQueued (XWindow[current_player]->display,
		QueuedAfterReading) > 0)
  {
    XNextEvent (XWindow[current_player]->display, &event);
    process_event (event, current_player);
  }

#if USE_UNIX
  fd = ConnectionNumber (XWindow[current_player]->display);
#endif

  /** Set flags indicating player's absence **/

  XWindow[current_player]->open = FALSE;
  done = TRUE;
  for (player=0; player<Config->player_count; player++)
    if (XWindow[player]->open && !XWindow[player]->watch)
      done = FALSE;
  
  /** Free up all constructs **/

  for (side=0; side<Config->side_count; side++)
  {
    XFreeGC (XWindow[current_player]->display,
			XWindow[current_player]->hue[side]);
    XFreeGC (XWindow[current_player]->display,
			XWindow[current_player]->hue_inverse[side]);
  }
  XFreeGC (XWindow[current_player]->display,
			XWindow[current_player]->hue_mark[0]);

  for (j=Config->level_min; j<=Config->level_max; j++)
  {
    for (i=0; i<Board->shape_count; i++)
      XFreePixmap (XWindow[current_player]->display,
			XWindow[current_player]->terrain[i][j]);
  }

  close_xwindow (XWindow[current_player]);

  /** If there are no more players remaining **/

  if (done)
  {
    /** Could be some people watching, so kill them **/

    for (player=0; player<Config->player_count; player++)
    {
      if (XWindow[player]->open)
        remove_player (player);
    }

    /** Send end of game signal if storing game **/

    if (Config->enable_all[OPTION_STORE] || Config->enable_all[OPTION_REPLAY])
    {
      if (Config->enable_all[OPTION_STORE])
      {
        end_signal = REPLAY_TERMINATE;
        fwrite (&end_signal, sizeof(char), 1, Config->fp);
      }
      fclose (Config->fp);
    }
    exit (0);
  }
  else
  {
    /** Else there are still players, show quit message on displays **/

    side = Config->player_to_side[current_player];
    sprintf (line, "%s has quit the game", Config->side_to_hue_name[side]);
    draw_message (line, strlen(line), side, current_player);

    /** Ring bells of remaining players **/

    for (player=0; player<Config->player_count; player++)
      if (XWindow[player]->open)
        XBell (XWindow[player]->display, 100);
  }

#if USE_UNIX
  FD_CLR (fd, &(Config->disp_fds)); 
#endif
}
