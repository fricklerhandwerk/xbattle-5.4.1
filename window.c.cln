#include <stdio.h>

#include "constant.h"
  
/**** x include files ****/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

#include "extern.h"

#define	SEA_VALUE_MAX		0.95

/******************************************************************************
  open_xwindow (xwindow, hue_title, bw_title)

  Open the window specified by <xwindow> and its fields, with title <bw_title>
  or <hue_title>, depending on whether b&w or color display.  This routine also
  handles everything else that has to do with graphics initialization, such as
  setting up colors, cell drawings, stipples, and so on.
******************************************************************************/

open_xwindow (xwindow, hue_title, bw_title)
  xwindow_type *xwindow;
  char *hue_title,
       *bw_title;
{
  int i, j, k,
      index,
      side,
      flag,
      current_side,
      level,
      pixel_count,
      hill_tone_count,
      forest_tone_count,
      sea_tone_count,
      limit;

  double fraction,
         value;

  shape_type *shape;

  XPoint small_points[MAX_POINTS+1],
         size_bound;
  XColor xcolor_side[MAX_HUES+2],
         xcolor_inverse[MAX_HUES+2],
         xcolor_mark[2],
         *xcolor_terrain;
  GC gc_and,
     gc_all,
     gc_none,
     gc_on, gc_off,
     *hue_terrain;

  static Pixmap stipple[MAX_HUES+1];

  /** Set some convenience variables **/

  hill_tone_count = Config->value_int_all[OPTION_HILL_TONES];
  forest_tone_count = Config->value_int_all[OPTION_FOREST_TONES];
  sea_tone_count = Config->value_int_all[OPTION_SEA_TONES];

  pixel_count = 0;

  current_side = Config->player_to_side[xwindow->player];

  /** Create the basic window **/

  create_xwindow (xwindow, hue_title, bw_title, current_side);

  /** Allocate hue GCs **/

  xwindow->hue = (GC *)(malloc(sizeof(GC)*(Config->side_count+1)));
  xwindow->hue_inverse = (GC *)(malloc(sizeof(GC)*(Config->side_count+1)));
  xwindow->hue_mark = (GC *)(malloc(sizeof(GC)*(2)));
  xwindow->hue_terrain = (GC *)(malloc(sizeof(GC)*
		(Config->level_max - Config->level_min + 2)));
  xwindow->hue_terrain -= Config->level_min;

  /** Allocate terrain pixmaps, set global pointer with offset to sea-level **/

  for (index=0; index<Board->shape_count; index++)
  {
    xwindow->terrain[index] = (Pixmap *)(malloc(sizeof(Pixmap)*
		(Config->level_max - Config->level_min + 2)));
    xwindow->terrain[index] -= Config->level_min;
  }

  xcolor_terrain = (XColor *)(malloc(sizeof(XColor)*
		(Config->level_max - Config->level_min + 2)));
  xcolor_terrain -= Config->level_min;

  hue_terrain = xwindow->hue_terrain;


  /** If color display, load colormap ***/

  if (xwindow->depth >= 8)
  {
    /** Set each player's color **/

    for (side=0; side<Config->side_count; side++)
    {
      /** If its a custom color, set RGB from color list, else set RGB	**/
      /** using color name and X function XParseColor().		**/

      if (Config->side_to_hue[side] >= 0)
      {
        xcolor_side[side].red =  
			Config->palette[Config->side_to_hue[side]][0]<<8;
        xcolor_side[side].green =
			Config->palette[Config->side_to_hue[side]][1]<<8;
        xcolor_side[side].blue = 
			Config->palette[Config->side_to_hue[side]][2]<<8;

        /** If custom color, directly set inverse index **/

        index = Config->hue_to_inverse[Config->side_to_hue[side]];
      }

      /** Set rest of normal color **/

      xcolor_side[side].flags = DoRed | DoGreen | DoBlue;
      xcolor_side[side].pixel = pixel_count++;

      /** Set rest of inverse color **/

      xcolor_inverse[side].red =   Config->palette[index][0]<<8;
      xcolor_inverse[side].green = Config->palette[index][1]<<8;
      xcolor_inverse[side].blue =  Config->palette[index][2]<<8;
      xcolor_inverse[side].flags = DoRed | DoGreen | DoBlue;
      xcolor_inverse[side].pixel = pixel_count++;
    }

    /** Set special marking colors (used to be inverse background) **/

    index = match_color_name ("mark", 0);

    xcolor_mark[0].flags =	DoRed | DoGreen | DoBlue;
    xcolor_mark[0].pixel =	pixel_count++;
    xcolor_mark[0].red =	Config->palette[index][0]<<8;
    xcolor_mark[0].green =	Config->palette[index][1]<<8;
    xcolor_mark[0].blue =	Config->palette[index][2]<<8;

    index = match_color_name ("border", 0);

    xcolor_mark[1].flags =	DoRed | DoGreen | DoBlue;
    xcolor_mark[1].pixel =	pixel_count++;
    xcolor_mark[1].red =	Config->palette[index][0]<<8;
    xcolor_mark[1].green =	Config->palette[index][1]<<8;
    xcolor_mark[1].blue =	Config->palette[index][2]<<8;

    /** This will color the seas **/

    for (level=Config->level_min; level<0; level++)
    {
      if (Config->enable[OPTION_SEA_BLOCK][current_side])
        index = 0;
      else
        index = -level - 1;

      xcolor_terrain[level].flags =	DoRed | DoGreen | DoBlue;
      xcolor_terrain[level].pixel =	pixel_count++;
      xcolor_terrain[level].red =	Config->palette_sea[index][0]<<8;
      xcolor_terrain[level].green =	Config->palette_sea[index][1]<<8;
      xcolor_terrain[level].blue =	Config->palette_sea[index][2]<<8;
    }

    for (level=0; level<=Config->level_max; level++)
    {
      /** Set hill tones **/

      if (Config->enable_all[OPTION_HILLS])
      {
        /** Use a linear interpolation through color space **/

        xcolor_terrain[level].flags =	DoRed | DoGreen | DoBlue;
        xcolor_terrain[level].pixel =	pixel_count++;
        xcolor_terrain[level].red =	Config->palette_hills[level][0]<<8;
        xcolor_terrain[level].green =	Config->palette_hills[level][1]<<8;
        xcolor_terrain[level].blue =	Config->palette_hills[level][2]<<8;
      }
      else if (Config->enable_all[OPTION_FOREST])
      {
        /** Use a linear interpolation through color space **/

        xcolor_terrain[level].flags =	DoRed | DoGreen | DoBlue;
        xcolor_terrain[level].pixel =	pixel_count++;
        xcolor_terrain[level].red =	Config->palette_forest[level][0]<<8;
        xcolor_terrain[level].green =	Config->palette_forest[level][1]<<8;
        xcolor_terrain[level].blue =	Config->palette_forest[level][2]<<8;
      }
      else
      {
        /** Set basic background color **/

        xcolor_terrain[0].flags =	DoRed | DoGreen | DoBlue;
        xcolor_terrain[0].pixel =	pixel_count++;
        xcolor_terrain[0].red =		Config->palette[0][0]<<8;
        xcolor_terrain[0].green =	Config->palette[0][1]<<8;
        xcolor_terrain[0].blue =	Config->palette[0][2]<<8;
      }
    }

    /** Terrain Config->level_max+1 is the "unknown" terrain in MAP modes **/

    index = match_color_name ("map", 0);

    xcolor_terrain[level].flags =	DoRed | DoGreen | DoBlue;
    xcolor_terrain[level].pixel =	pixel_count++;
    xcolor_terrain[level].red =		Config->palette[index][0]<<8;
    xcolor_terrain[level].green =	Config->palette[index][1]<<8;
    xcolor_terrain[level].blue =	Config->palette[index][2]<<8;

    /** Either store colors in new or old colormap **/

#if USE_NEW_COLORMAP
    XStoreColors (xwindow->display, xwindow->cmap,
			xcolor_side, Config->side_count);
    XStoreColors (xwindow->display, xwindow->cmap,
			xcolor_inverse, Config->side_count);
    XStoreColors (xwindow->display, xwindow->cmap,
			xcolor_mark, 2);
    XStoreColors (xwindow->display, xwindow->cmap,
			xcolor_terrain + Config->level_min,
			Config->level_max - Config->level_min + 2);
#else
    flag = FALSE;

    for (i=0; i<Config->side_count; i++)
      if (!XAllocColor (xwindow->display, xwindow->cmap, &xcolor_side[i]))
        flag = TRUE;

    for (i=0; i<(Config->side_count+1); i++)
      if (!XAllocColor (xwindow->display, xwindow->cmap, &xcolor_inverse[i]))
        flag = TRUE;

    for (i=0; i<2; i++)
      if (!XAllocColor (xwindow->display, xwindow->cmap, &xcolor_mark[i]))
        flag = TRUE;

    for (level=Config->level_min; level<=Config->level_max+1; level++)
      if (!XAllocColor (xwindow->display, xwindow->cmap, &xcolor_terrain[level]))
        flag = TRUE;

    if (flag)
      throw_warning ("Could not allocate all color cells\n%s",
		"         Kill other colorful applications or recompile with USE_NEW_COLORMAP");
#endif

    for (side=0; side<Config->side_count; side++)
      xwindow->draw_letter[side] = FALSE;
  }

  /** Raise window **/

  XMapRaised (xwindow->display, xwindow->window);

  /** Create GCs for each side and link to hues **/

  for (side=0; side<Config->side_count; side++)
  {
    xwindow->hue[side] = XCreateGC (xwindow->display, xwindow->window, 0, 0);
    XSetFunction (xwindow->display, xwindow->hue[side], GXcopy);
    xwindow->hue_inverse[side] =
			XCreateGC (xwindow->display, xwindow->window, 0, 0);
    XSetFunction (xwindow->display, xwindow->hue_inverse[side], GXcopy);
  }

  /** Create mark GCs and link to hues **/

  for (i=0; i<2; i++)
  {
    xwindow->hue_mark[i] = XCreateGC (xwindow->display, xwindow->window, 0, 0);
    XSetFunction (xwindow->display, xwindow->hue_mark[i], GXcopy);
  }

  /** Set the special purpose GCs for pixmap manipulation **/

  xwindow->gc_flip  = XCreateGC (xwindow->display, xwindow->window, 0, 0);
  XSetFunction (xwindow->display, xwindow->gc_flip,  GXinvert);

  xwindow->gc_clear  = XCreateGC (xwindow->display, xwindow->window, 0, 0);
  XSetFunction (xwindow->display, xwindow->gc_clear, GXandInverted);

  xwindow->gc_or = XCreateGC (xwindow->display, xwindow->window, 0, 0);
  XSetFunction (xwindow->display, xwindow->gc_or, GXor);

  /** If b&w display, create black and white GCs **/

  if (xwindow->depth == 1)
  {
    gc_on = XCreateGC (xwindow->display, xwindow->window, 0, 0);
    XSetFunction (xwindow->display, gc_on, GXcopy);

    gc_off = XCreateGC (xwindow->display, xwindow->window, 0, 0);
    XSetFunction (xwindow->display, gc_off, GXcopy);
  }

  /** Create terrain GCs and link to hues **/

  for (level=Config->level_min; level<=Config->level_max+1; level++)
  {
    hue_terrain[level] = XCreateGC (xwindow->display, xwindow->window, 0, 0);
    XSetFunction (xwindow->display, hue_terrain[level], GXcopy);
  }

  /** Set drawing GCs fow b&w displays **/

  if (xwindow->depth == 1)
  { 
    /** For each side, set b&w GCs (with inverses) **/

    for (side=0; side<Config->side_count; side++)
    {
      XSetForeground (xwindow->display, xwindow->hue[side],
                      BlackPixel (xwindow->display, xwindow->screen));
      XSetBackground (xwindow->display, xwindow->hue[side],
                      WhitePixel (xwindow->display, xwindow->screen));

      XSetForeground (xwindow->display, xwindow->hue_inverse[side],
                      BlackPixel (xwindow->display, xwindow->screen));
      XSetBackground (xwindow->display, xwindow->hue_inverse[side],
                      WhitePixel (xwindow->display, xwindow->screen));
    }

    /** Set background GC **/

    XSetForeground (xwindow->display, xwindow->hue_terrain[0],
                    BlackPixel (xwindow->display, xwindow->screen));
    XSetBackground (xwindow->display, xwindow->hue_terrain[0],
                    WhitePixel (xwindow->display, xwindow->screen));

    /** Set invisible terrain GC **/

    XSetForeground (xwindow->display, xwindow->hue_terrain[1],
                    BlackPixel (xwindow->display, xwindow->screen));
    XSetBackground (xwindow->display, xwindow->hue_terrain[1],
                    WhitePixel (xwindow->display, xwindow->screen));

    /** Set marking GC **/

    XSetForeground (xwindow->display, xwindow->hue_mark[0],
                    BlackPixel (xwindow->display, xwindow->screen));
    XSetBackground (xwindow->display, xwindow->hue_mark[0],
                    WhitePixel (xwindow->display, xwindow->screen));

    XSetForeground (xwindow->display, xwindow->hue_mark[1],
                    BlackPixel (xwindow->display, xwindow->screen));
    XSetBackground (xwindow->display, xwindow->hue_mark[1],
                    WhitePixel (xwindow->display, xwindow->screen));

    /** Set basic on and off GCs **/

    XSetForeground (xwindow->display, gc_on,
                    BlackPixel (xwindow->display, xwindow->screen));
    XSetBackground (xwindow->display, gc_on,
                    WhitePixel (xwindow->display, xwindow->screen));
    XSetForeground (xwindow->display, gc_off,
                    WhitePixel (xwindow->display, xwindow->screen));
    XSetBackground (xwindow->display, gc_off,
                    BlackPixel (xwindow->display, xwindow->screen));
  }

  /** Set drawing GCs for color displays **/

  if (xwindow->depth >= 8)
  {
    /** Set colors for each side **/

    for (side=0; side<Config->side_count; side++)
    {
      XSetForeground (xwindow->display, xwindow->hue[side],
			xcolor_side[side].pixel);
      XSetBackground (xwindow->display, xwindow->hue[side],
			xcolor_inverse[side].pixel);
      XSetForeground (xwindow->display, xwindow->hue_inverse[side],
			xcolor_inverse[side].pixel);
      XSetBackground (xwindow->display, xwindow->hue_inverse[side],
			xcolor_side[side].pixel);
    }

    /** Set mark colors **/

    for (i=0; i<2; i++)
    {
      XSetForeground (xwindow->display, xwindow->hue_mark[i],
			xcolor_mark[i].pixel);
      XSetBackground (xwindow->display, xwindow->hue_mark[i],
			xcolor_terrain[0].pixel);
    }

    /** Set colors for each terrain level **/

    for (level=Config->level_min; level<=Config->level_max+1; level++)
    {
      XSetForeground (xwindow->display, hue_terrain[level],
			xcolor_terrain[level].pixel);
      XSetBackground (xwindow->display, hue_terrain[level],
			xcolor_terrain[0].pixel);
    }
  }
  else
  {
    /** Set all the stipple patterns for b&w display **/

    init_stipple (xwindow->display, xwindow->window, stipple);

    /** Set stipple for each side **/

    for (side=0; side<Config->side_count; side++)
    {
      XSetStipple (xwindow->display,
		xwindow->hue[side], stipple[Config->side_to_bw[side]]);
      XSetFillStyle (xwindow->display,
		xwindow->hue[side], FillOpaqueStippled);
      XSetStipple (xwindow->display,
		xwindow->hue_inverse[side],
		stipple[Config->bw_to_inverse[Config->side_to_bw[side]]]);
      XSetFillStyle (xwindow->display,
		xwindow->hue_inverse[side], FillOpaqueStippled);
    }

    /** Set background stipple **/

    XSetStipple (xwindow->display,
		xwindow->hue_terrain[0], stipple[0]);
    XSetFillStyle (xwindow->display,
		xwindow->hue_terrain[0], FillOpaqueStippled);

    /** Set invisible background stipple **/

    XSetStipple (xwindow->display,
		xwindow->hue_terrain[1], stipple[0]);
    XSetFillStyle (xwindow->display,
		xwindow->hue_terrain[1], FillOpaqueStippled);

    /** Set mark stipple **/

    XSetStipple (xwindow->display, xwindow->hue_mark[0],
		stipple[Config->bw_to_inverse[0]]);
    XSetFillStyle (xwindow->display, xwindow->hue_mark[0], FillOpaqueStippled);

    XSetStipple (xwindow->display, xwindow->hue_mark[1],
		stipple[Config->bw_to_inverse[1]]);
    XSetFillStyle (xwindow->display, xwindow->hue_mark[1], FillOpaqueStippled);
  }

  /** Create a work space pixmap (for double buffering), first finding	**/
  /** the maximum size of all the board shapes.				**/

  size_bound.x = 0;
  size_bound.y = 0;
  for (index=0; index<Board->shape_count; index++)
  {
    shape = Board->shapes[current_side][index];
    if (shape->size_bound.x > size_bound.x)
      size_bound.x = shape->size_bound.x;
    if (shape->size_bound.y > size_bound.y)
      size_bound.y = shape->size_bound.y;
  }

  xwindow->work_space = XCreatePixmap (xwindow->display, xwindow->window,
		size_bound.x, size_bound.y, xwindow->depth);

  /** If needed, create a backing pixmap (for full double buffering) **/

  if (shape->copy_method == COPY_BACK)
    xwindow->backing_space = XCreatePixmap (xwindow->display, xwindow->window,
		xwindow->size_window.x, xwindow->size_window.y, xwindow->depth);

  /** Create the individual terrain pixmaps **/

  for (index=0; index<Board->shape_count; index++)
  {
    shape = Board->shapes[current_side][index];

    for (level=Config->level_min; level<=Config->level_max+1; level++)
      xwindow->terrain[index][level] =
		XCreatePixmap (xwindow->display, xwindow->window,
		shape->size_bound.x, 2*shape->size_bound.y, xwindow->depth);
  }

  /** If a b&w display, initialize error diffused terrain "colors" **/

  if (xwindow->depth == 1)
  {
    for (index=0; index<Board->shape_count; index++)
    {
      shape = Board->shapes[current_side][index];

      init_terrain_pixmaps (xwindow, shape, gc_on, gc_off, index);
    }
  }

  /** Set up temporary GCs for pixmap manipulation **/

  gc_and  = XCreateGC (xwindow->display, xwindow->window, 0, 0);
  XSetFunction (xwindow->display, gc_and, GXand);

  gc_all  = XCreateGC (xwindow->display, xwindow->window, 0, 0);
  XSetFunction (xwindow->display, gc_all, GXset);

  gc_none  = XCreateGC (xwindow->display, xwindow->window, 0, 0);
  XSetFunction (xwindow->display, gc_none, GXclear);

  /** For each cell shape **/

  for (index=0; index<Board->shape_count; index++)
  {
    shape = Board->shapes[current_side][index];

    /** For each terrain level **/

    for (level=Config->level_min; level<=Config->level_max+1; level++)
    {
      /** Fill top frame with appropriate terrain color **/

      shape->points[0].x = shape->corner_vertex.x;
      shape->points[0].y = shape->corner_vertex.y;

      if (xwindow->depth >= 8)
        XFillPolygon (xwindow->display, xwindow->terrain[index][level],
		xwindow->hue_terrain[level],
		shape->points, shape->point_count-1,
		Convex, CoordModePrevious);

      if (Config->enable[OPTION_GRID][side])
        XDrawLines (xwindow->display, xwindow->terrain[index][level],
		xwindow->hue_mark[0],
		shape->points, shape->point_count,
		CoordModePrevious);
      else
        XDrawLines (xwindow->display, xwindow->terrain[index][level],
		xwindow->hue_terrain[level],
		shape->points, shape->point_count,
		CoordModePrevious);

      /** Fill bottom frame with gc_none to "black-out" everything **/

      XFillRectangle (xwindow->display, xwindow->terrain[index][level], gc_none,
		0, shape->size_bound.y,
		shape->size_bound.x, shape->size_bound.y);

      /** Draw polygonal template in bottom frame with gc_all **/

      shape->points[0].x = shape->corner_vertex.x;
      shape->points[0].y = shape->corner_vertex.y + shape->size_bound.y;

      XFillPolygon (xwindow->display, xwindow->terrain[index][level], gc_all,
		shape->points, shape->point_count-1,
		Convex, CoordModePrevious);

      XDrawLines (xwindow->display, xwindow->terrain[index][level], gc_all,
		shape->points, shape->point_count,
		CoordModePrevious);

      /** If using blocky sea method **/

      if (Config->enable[OPTION_SEA_BLOCK][current_side] &&
		level < 0 && level > Config->level_min)
      {
        /** Draw terrain 0 part of cell in top frame **/

        shape->points[0].x = shape->corner_vertex.x;
        shape->points[0].y = shape->corner_vertex.y;

        if (xwindow->depth >= 8)
          XFillPolygon (xwindow->display, xwindow->terrain[index][level],
		hue_terrain[0],
		shape->points, shape->point_count-1,
		Convex, CoordModePrevious);

        if (Config->enable[OPTION_GRID][side])
          XDrawLines (xwindow->display, xwindow->terrain[index][level],
		xwindow->hue_mark[0],
		shape->points, shape->point_count,
		CoordModePrevious);
        else
          XDrawLines (xwindow->display, xwindow->terrain[index][level],
		xwindow->hue_terrain[0],
		shape->points, shape->point_count,
		CoordModePrevious);

        /** Compute fraction which should be sea filled **/

        fraction = ((double)(-level-1))*
		((1.0 - SEA_BLOCK_MIN)/((double)(sea_tone_count-1))) +
		SEA_BLOCK_MIN;

        /** Set upper left vertex of sea part of hex **/

        small_points[0].x = shape->corner_vertex.x + (int)((1.0-fraction)*
		(shape->center_bound.x - shape->corner_vertex.x) + 0.5);
        small_points[0].y = shape->corner_vertex.y + (int)((1.0-fraction)*
		(shape->center_bound.y - shape->corner_vertex.y) + 0.5);

        /** Set relative vertices of sea part of cell **/

        for (k=1; k<shape->point_count; k++)
        {
          if (shape->points[k].x < 0)
            small_points[k].x = (int)(fraction * shape->points[k].x - 0.5);
          else
            small_points[k].x = (int)(fraction * shape->points[k].x + 0.5);

          if (shape->points[k].y < 0)
            small_points[k].y = (int)(fraction * shape->points[k].y - 0.5);
          else
            small_points[k].y = (int)(fraction * shape->points[k].y + 0.5);
        }

        /** Draw small sea polygon **/

        if (xwindow->depth >= 8)
          XFillPolygon (xwindow->display, xwindow->terrain[index][level],
		hue_terrain[level],
		small_points, shape->point_count-1,
		Convex, CoordModePrevious);
      }

      /** Copy the cell from the lower position to upper position **/

      XCopyArea (xwindow->display, xwindow->terrain[index][level],
		xwindow->terrain[index][level], gc_and,
		0, shape->size_bound.y,
		shape->size_bound.x, shape->size_bound.y,
		0, 0);
    }
  }

  /** Load information font **/

  xwindow->font_struct = XLoadQueryFont (xwindow->display, Config->font);

  /** If can't find font, use default **/

  if (xwindow->font_struct == 0)
  {
    throw_warning ("Could not find font %s, using default", Config->font);
    xwindow->font_struct = XQueryFont (xwindow->display,
		XGContextFromGC(DefaultGC(xwindow->display,xwindow->screen)));
  }
  else
  {
    /** Else found font, link font to each side GC **/

    for (side=0; side<Config->side_count; side++)
    {
      XSetFont (xwindow->display,
		xwindow->hue[side], xwindow->font_struct->fid);
      XSetFont (xwindow->display,
		xwindow->hue_inverse[side], xwindow->font_struct->fid);
    }
    XSetFont (xwindow->display, xwindow->hue_mark[0], xwindow->font_struct->fid);
    XSetFont (xwindow->display, xwindow->hue_mark[1], xwindow->font_struct->fid);
    XSetFont (xwindow->display, xwindow->gc_flip,  xwindow->font_struct->fid);
  }

  /** Get font with and height **/

  XGetFontProperty (xwindow->font_struct, XA_QUAD_WIDTH, &xwindow->char_width);
  XGetFontProperty (xwindow->font_struct, XA_CAP_HEIGHT, &xwindow->char_height);

  /** If a b&w display, check to see if letters should be drawn in cells **/

  if (xwindow->depth == 1)
  {
    for (side=0; side<Config->side_count; side++)
    {
      if (Config->side_to_letter[side][0])
      {
        xwindow->draw_letter[side] = TRUE;
        strcpy (xwindow->letter[side], Config->side_to_letter[side]);
      }
      else
        xwindow->draw_letter[side] = FALSE;
    }
  }

  /** Map window to make it visible **/

  if (xwindow->depth >= 8)
    XMapWindow (xwindow->display, xwindow->window);
}



/******************************************************************************
  create_xwindow (xwindow, hue_title, bw_title, current_side)

  Perform the actual window creation as specfied by <xwindow>.
******************************************************************************/

create_xwindow (xwindow, hue_title, bw_title, current_side)
  xwindow_type *xwindow;
  char *hue_title,
       *bw_title;
  int current_side;
{
  XSizeHints hint;
  XWMHints xwm_hint;
  Visual *visual;
  unsigned long valuemask;
  XSetWindowAttributes attrib;
  XVisualInfo vinfo;
  long event_mask,
       full_depth;
  Atom wm_delete_window;

  /** Open display and screen **/

  xwindow->display = XOpenDisplay (xwindow->display_name);
  if (xwindow->display == NULL)
    throw_error ("Cannot open display %s", xwindow->display_name);

  xwindow->screen  = DefaultScreen (xwindow->display);

  /** Set player viewing flags **/

  xwindow->open  =	TRUE;
  xwindow->watch  =	FALSE;

  /** Set window position and size hints for window manager **/

  hint.x =	Config->enable[OPTION_MANPOS][current_side] ?
			-1 : Config->value_int_all[OPTION_XPOS];
  hint.y =	Config->enable[OPTION_MANPOS][current_side] ?
			-1 : Config->value_int_all[OPTION_YPOS];
  hint.width =	xwindow->size_window.x;
  hint.height =	xwindow->size_window.y;

  if (hint.x < 0 || hint.y < 0)
    hint.flags = ( PPosition | PSize);
  else
    hint.flags = (USPosition | PSize);

  /** Set bitplane depth to default **/

  xwindow->depth = DefaultDepth (xwindow->display, xwindow->screen);
  full_depth = xwindow->depth;

  /** Get a visual **/

  visual = DefaultVisual (xwindow->display, xwindow->screen);

  /** Try to force the display to 8 planes **/

  if (xwindow->depth != 8)
  {
    if (XMatchVisualInfo
		(xwindow->display, xwindow->screen, 8, PseudoColor, &vinfo))
    {
      visual = vinfo.visual;
      xwindow->depth = 8;
    }
  }

  /** If couldn't find an 8 bit visual, try a 16 bit visual **/

  if (xwindow->depth != 8)
  {
    if (XMatchVisualInfo
		(xwindow->display, xwindow->screen, 16, PseudoColor, &vinfo))
    {
      visual = vinfo.visual;
      xwindow->depth = 16;
    }
  }

  /** If have less than 8 planes, just use a single plane **/

  if (xwindow->depth < 8)
    xwindow->depth = 1;

  /** If a full set of depth planes (color) **/

  if(xwindow->depth >= 8)
  {
    /** create color map using visual **/

#if USE_NEW_COLORMAP
    xwindow->cmap = XCreateColormap (xwindow->display,
			RootWindow (xwindow->display, xwindow->screen),
			visual, AllocAll);
#else
    /** Allocate existing colormap **/

    if (full_depth > 8)
    {
      /** XBattle won't work for 24 bit displays with no optional	**/
      /** visuals with 8 or 16 bits.  This may be the case on some PCs.	**/
      /** In these cases, it may be possible to reconfigure the display	**/
      /** to 8 or 16 bits.						**/

      if (xwindow->depth > 16)
      {
        throw_warning ("No PseudoColor visual available\n%s",
		"         Try changing display to <= 16 bits");
        throw_error ("Unable to continue without colormap", NULL);
      }

      /** If display is 24 bit, have to create a dedicated colormap **/

      xwindow->cmap = XCreateColormap (xwindow->display,
			RootWindow (xwindow->display, xwindow->screen),
			visual, AllocNone);
    }
    else
      xwindow->cmap = DefaultColormap (xwindow->display, xwindow->screen);
#endif
    
    /** Set attributes **/

    valuemask = CWBackPixel | CWBorderPixel | CWBitGravity | CWColormap;
    attrib.background_pixel = BlackPixel (xwindow->display, xwindow->screen);
    attrib.border_pixel = BlackPixel (xwindow->display, xwindow->screen);
    attrib.bit_gravity = CenterGravity;
    attrib.colormap = xwindow->cmap;

    /** Create the x window **/

    xwindow->window = XCreateWindow (xwindow->display,
		RootWindow (xwindow->display, xwindow->screen),
		hint.x, hint.y, hint.width,
		hint.height, DEFAULT_BORDER, xwindow->depth,
		InputOutput, visual, valuemask, &attrib);
  }
  else
  {
    /** Else create b&w window **/

    xwindow->window = XCreateSimpleWindow (xwindow->display,
		DefaultRootWindow (xwindow->display),
		hint.x, hint.y,
		hint.width, hint.height, DEFAULT_BORDER,
		BlackPixel (xwindow->display, xwindow->screen),
		WhitePixel (xwindow->display, xwindow->screen));
  }

  /** Set up system to gracefully recover from errors **/

  wm_delete_window = XInternAtom (xwindow->display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(xwindow->display, xwindow->window, &wm_delete_window, 1);

  /** Set standard properties (from hints) for window manager **/

  if (xwindow->depth >= 8)
    XSetStandardProperties (xwindow->display, xwindow->window, hue_title,
			hue_title, None, NULL, None, &hint);
  else if (xwindow->depth == 1)
    XSetStandardProperties (xwindow->display, xwindow->window, bw_title,
			bw_title, None, NULL, None, &hint);

  /** Set window manager hints **/

  xwm_hint.flags = (InputHint|StateHint);  
  xwm_hint.input = TRUE;
  xwm_hint.initial_state = NormalState;
  XSetWMHints (xwindow->display, xwindow->window, &(xwm_hint));

  /** Make window sensitive selected events **/

  if (Config->enable_all[OPTION_BOUND])
    event_mask = ButtonPressMask|ButtonReleaseMask|KeyPressMask|ExposureMask;
  else
    event_mask = ButtonPressMask|KeyPressMask|ExposureMask;
  XSelectInput (xwindow->display, xwindow->window, event_mask);
}



/******************************************************************************
  close_xwindow (xwindow)

  Gracefully close a window.
******************************************************************************/

close_xwindow (xwindow)
  xwindow_type *xwindow;
{
  XDestroyWindow (xwindow->display, xwindow->window);
  XCloseDisplay (xwindow->display);
}



/******************************************************************************
  init_stipple (display, window, stipple)

  Create the stipple patterns which are used on b&w displays.
******************************************************************************/

init_stipple (display, window, stipple)
  Display *display;
  Window window;
  Pixmap stipple[];
{
  int i;

  for (i=0; i<Config->hue_count; i++)
  {
    if (Config->hue_has_bw[i])
      stipple[i] = XCreateBitmapFromData (display, window,
				Config->palette_gray[i], 8, 8);
  }
}



/******************************************************************************
  init_terrain_pixmaps (xwindow, shape, gc_on, gc_off, index)

  Using an error diffusion algorithm, initialize the terrain patterns for b&w
  displays.  The number of distinct patterns is governed by the terrain limits,
  with <gc_on> and <gc_off> providing the GCs for on and off bits respectively.
******************************************************************************/

init_terrain_pixmaps (xwindow, shape, gc_on, gc_off, index)
  xwindow_type *xwindow;
  shape_type *shape;
  GC gc_on, gc_off;
  int index;
{
  int x, y,
      tone_count,
      level,
      enable_quasi;

  double value,
         quasi,
         sea_value_min,
         target[2*MAX_SQUARESIZE+DIFFUSE_BUFFER]
			[2*MAX_SQUARESIZE+DIFFUSE_BUFFER],
         error;

  /** Set the number of diffused tones to create **/ 

  tone_count = Config->level_max;
  sea_value_min = Config->value_double_all[OPTION_SEA_VALUE];

  /** For each desired level of greytone **/

  for (level=Config->level_min; level<=Config->level_max; level++)
  {
    /** Compute desired graylevel in [0,1] **/

    if (level < 0)
    {
      if (Config->level_min == -1)
        value = sea_value_min;
      else
        value = sea_value_min - (1 + level)*
		(sea_value_min - SEA_VALUE_MAX)/(Config->level_min+1);
    }
    else if (Config->enable_all[OPTION_FOREST])
      value = DIFFUSE_MAX_LEVEL - DIFFUSE_SPAN +
		level * DIFFUSE_SPAN/tone_count;
    else if (Config->level_max == 0)
      value = 0.5;
    else
      value = DIFFUSE_MAX_LEVEL - level * DIFFUSE_SPAN/tone_count;

    /** Enable quasi-random error diffusion if many levels **/

    if (level < 0)
      enable_quasi = TRUE;
    else if (tone_count <= DIFFUSE_QUASI_THRESHOLD)
      enable_quasi = FALSE;
    else
      enable_quasi = TRUE;

    /** Set the target gray values in a buffered grid **/

    for (x=0; x<shape->size_bound.x+DIFFUSE_BUFFER; x++)
      for (y=0; y<shape->size_bound.y+DIFFUSE_BUFFER; y++)
        target[x][y] = value;

    /** For each pixel do threshold and diffuse error **/

    for (x=0; x<(shape->size_bound.x + DIFFUSE_BUFFER); x++)
    {
      for (y=0; y<(shape->size_bound.y + DIFFUSE_BUFFER); y++)
      {
        /** Draw target point into pixmap with appropriate GC, update grid **/

        if (target[x][y] > DIFFUSE_MEAN_LEVEL)
        {
          if (x>=DIFFUSE_BUFFER_HALF &&
			x<(shape->size_bound.x + DIFFUSE_BUFFER_HALF) &&
			y>=DIFFUSE_BUFFER_HALF &&
			y<(shape->size_bound.y + DIFFUSE_BUFFER_HALF))
          {
            XDrawPoint (xwindow->display, xwindow->terrain[index][level],
			gc_on,
			x - DIFFUSE_BUFFER_HALF, y - DIFFUSE_BUFFER_HALF);
          }
          error = target[x][y] - 1.0;
        }
        else
        {
          if (x>=DIFFUSE_BUFFER_HALF &&
			x<(shape->size_bound.x + DIFFUSE_BUFFER_HALF) &&
			y>=DIFFUSE_BUFFER_HALF &&
			y<shape->size_bound.y + DIFFUSE_BUFFER_HALF)
          {
            XDrawPoint (xwindow->display, xwindow->terrain[index][level],
			gc_off,
			x - DIFFUSE_BUFFER_HALF, y - DIFFUSE_BUFFER_HALF);
          }
          error = target[x][y];
        }

        /** Enable randomness if over 5 levels (removes worms) **/

        if (enable_quasi)
          quasi = DIFFUSE_QUASI_FACTOR * get_random(100);
        else
          quasi = 0;

        /** Distribute error amongst the pixel neighbors, being careful **/
        /** to handle edge conditions properly.				**/

        if (y == 0)
        {
          if (x == shape->size_bound.x+DIFFUSE_BUFFER-1)
          {
            target[x][y+1] +=	DIFFUSE_1_SI * error;
          }
          else
          {
            target[x][y+1] +=	(DIFFUSE_3_SI + quasi) * error;
            target[x+1][y] +=	(DIFFUSE_3_IS + quasi) * error;
            target[x+1][y+1] += DIFFUSE_3_II * error;
          }
        }
        else if (y == shape->size_bound.y+DIFFUSE_BUFFER-1)
        {
          if (x != shape->size_bound.x+DIFFUSE_BUFFER-1)
          {
            target[x+1][y] +=	(DIFFUSE_2_IS + quasi) * error;
            target[x+1][y-1] +=	(DIFFUSE_2_ID - quasi) * error;
          }
        }
        else if (x == shape->size_bound.x+DIFFUSE_BUFFER-1)
        {
          target[x][y+1] += 	DIFFUSE_1_SI * error;
        }
        else
        {
          target[x][y+1] +=	(DIFFUSE_4_SI + quasi) * error;
          target[x+1][y] +=	(DIFFUSE_4_IS - quasi) * error;
          target[x+1][y+1] +=	DIFFUSE_4_II * error;
          target[x+1][y-1] +=	DIFFUSE_4_ID * error;
        }
      }
    }
  }
}
