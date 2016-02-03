
#include "macro.h"

typedef unsigned char	n_char;
typedef signed char	s_char;

/**** window structure ****/

typedef struct{
    Display 		*display;
    Window 		window;
    Window 		drawable;
    XFontStruct 	*font_struct;
    GC 			*hue,
                        *hue_inverse,
                        *hue_terrain,
                        *hue_mark,
                        gc_flip,
                        gc_clear,
                        gc_or;
    Pixmap		*terrain[MAX_SHAPES],
			work_space,
			backing_space;
    Colormap 		cmap;
    XPoint		size_window,
			size_play,
			size_text,
			offset_play,
			offset_text;
    int 		depth,
                        screen;
    int			open,
                        watch,
                        player;
    int			text_y_pos[MAX_PLAYERS];
    char		draw_letter[MAX_SIDES+1];
    char		letter[MAX_SIDES+1][2];
    unsigned long	char_width, char_height;
    char		display_name[80];
} xwindow_type;

/**** game cell structure ****/
typedef struct CellType {
    s_char 		level;			/** load assured **/	
    s_char 		growth;			/** load assured **/
    s_char 		old_growth;		/** load assured **/
    short		angle;			/** load assured **/
    s_char 		x, y;			/** load assured **/
    s_char 		side;
    s_char 		move;
    s_char		any_march;
    s_char		march_side;
    s_char		march_count;
    s_char		side_count;
    s_char		lowbound;
    s_char		manage_update;
    s_char		manage_x, manage_y, manage_dir;
    s_char	 	old_side;
    s_char		age;
    s_char		shape_index;
    s_char		redraw_status;
    s_char		outdated;
    s_char		old_value;
    s_char 		*dir;
    s_char		*march;
    s_char		*march_type;
    s_char		*march_dir;
    s_char 		*value;
    s_char		*seen;
    s_char		*draw_level;
    struct CellType	*connect[MAX_DIRECTIONS];
    short		*x_center;
    short		*y_center;
} cell_type;


typedef struct
{
    int			build_count,
			troop_count,
			cell_count;
} statistic_type;


typedef struct
{
    XPoint		center_bound,
			center_erase,
			center_vertex,
			center_rectangle,
			corner_erase,
			corner_vertex,
			size_bound,
			size_erase,
			size_rectangle,
			helper;


    short		max_value,
			max_max_value,
			troop_to_size[MAX_MAXVAL],
			growth_to_radius[TOWN_MAX+1],
			circle_bound,
			side,
			area;

    n_char 		direction_count,
			direction_factor,
			angle_offset,
			use_secondary;

    n_char 		troop_shape,
			erase_shape,
			copy_method,
			erase_method;

    short		horizon_even[200][2], horizon_odd[200][2],
			horizon_counts[10];

    n_char 		point_count;
    XPoint		points[MAX_POINTS];

    XPoint		arrow_source[MAX_DIRECTIONS][2][3],
			arrow_dester[MAX_DIRECTIONS][2][3],
			arrow_source_x[MAX_MAXVAL][MAX_DIRECTIONS][2],
			arrow_dester_x[MAX_MAXVAL][MAX_DIRECTIONS][2],
			march_source[MAX_DIRECTIONS][4],
			march_dester[MAX_DIRECTIONS][4];

    s_char		chart[2*MAX_SQUARESIZE][2*MAX_SQUARESIZE][2];
} shape_type;

typedef struct {
    short		size_x, size_y;	

    int			cell_count;
    cell_type		*cells[MAX_BOARDSIZE][MAX_BOARDSIZE],
			*list[MAX_BOARDSIZE*MAX_BOARDSIZE];

    short		shape_count;
    shape_type		*shapes[MAX_SIDES][MAX_SHAPES];

    XPoint		size[MAX_SIDES];
} board_type;


typedef struct
{
  XPoint		matrix[MAX_SELECT_SIZE][MAX_SELECT_SIZE];
  XPoint		dimension,
			multiplier,
			offset;
} select_type;


typedef struct
{ 
  n_char		enable_all[OPTION_COUNT], 
			enable[OPTION_COUNT][MAX_SIDES],
			enable_terrain; 
 
  int			value_int_all[OPTION_COUNT],
			value_int[OPTION_COUNT][MAX_SIDES]; 
  double		value_double_all[OPTION_COUNT],
			value_double[OPTION_COUNT][MAX_SIDES]; 

  short			side_count,
			player_count,
			load_side_count,
			hue_count,
			bw_count,
			direction_count;

  short			player_to_side[MAX_PLAYERS],
			side_to_hue[MAX_SIDES],
			side_to_bw[MAX_SIDES],
			hue_to_inverse[MAX_HUES],
			bw_to_inverse[MAX_BWS];
  char			side_to_letter[MAX_SIDES][2],
			side_to_hue_name[MAX_SIDES][MAX_NAME],
			side_to_bw_name[MAX_SIDES][MAX_NAME];

  char			hue_name[MAX_HUES+1][25];
  n_char		palette[MAX_HUES+1][3];
  short			palette_forest[MAX_FOREST_TONES][3],
			palette_hills[MAX_HILL_TONES][3],
			palette_sea[MAX_FOREST_TONES][3];
  n_char		palette_gray[MAX_BWS+1][8];
  n_char		hue_has_bw[MAX_HUES+1];

  char			message_all[MAX_TEXT],
			message_single[MAX_PLAYERS][MAX_TEXT];

  char 			file_store[MAX_NAME],
			file_replay[MAX_NAME],
			file_map[MAX_NAME],
			file_store_map[MAX_NAME];
  n_char		use_brief_load;

  int			max_value[MAX_SIDES],
			max_max_value,
			text_offset,
			delay,
			fill_number,
			tile_type,
			level_min,
			level_max;

  short			board_x_size,
			board_y_size,
			center_size,
			march_size,
			text_size,
  			cell_size[MAX_SIDES];

  int			view_range[MAX_SIDES],
			view_range_max;

  n_char		is_paused;

  int			dir[MAX_PLAYERS][MAX_DIRECTIONS],
       		        dir_type[MAX_PLAYERS],
       		        dir_factor[MAX_PLAYERS];

  int			old_x[MAX_PLAYERS],
			old_y[MAX_PLAYERS];

  char			font[200];

  double		*move_slope[MAX_SIDES],
			move_hinder[MAX_SIDES][MAX_HILL_TONES+1],
			move_shunt[MAX_SIDES][MAX_MAXVAL+2],
			move_moves[MAX_DIRECTIONS+1];

  FILE			*fp;

  select_type		*selects[MAX_SIDES];

  statistic_type	*stats[MAX_SIDES];

#if USE_LONGJMP
  jmp_buf		saved_environment;
#endif
#if USE_UNIX
  fd_set		disp_fds;
#endif

} config_info;

/**** extern variables ****/

extern char Blank[];

/**** global variables ****/

extern xwindow_type	*XWindow[MAX_PLAYERS];
extern board_type	*Board;
extern config_info	*Config;

extern cell_type	*get_cell();
