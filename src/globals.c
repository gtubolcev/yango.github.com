

#include "globals.h"
#include <gtk/gtk.h>


GdkPixmap *pixmap = NULL;
//GdkPixbuf *pixbuf_tracks = NULL; //pixmap for tracks
GtkWidget *window1 = NULL;
GtkWidget *window2 = NULL;
GtkWidget *map_drawable = NULL;

GtkWidget *dialog1 = NULL;
GtkWidget *dialog8 = NULL;

GtkWidget *window3 = NULL;
GtkWidget *menu1 = NULL;


char *global_track_dir = NULL;

int global_x = 890;
int global_y = 515;
int global_zoom = 3;
		
int mouse_dx = 0; 
int mouse_dy = 0;
		
int global_drawingarea_width  = 0;
int global_drawingarea_height = 0;

int global_speed_unit	= 0;	
int global_alt_unit	= 0;	
int global_latlon_unit	= 0;	


gps_data_t *gpsdata = NULL;

GSList		*trackpoint_list = NULL;
GSList		*friends_list = NULL;
GSList		*photo_list = NULL;
GSList		*poi_list = NULL;
float		trip_distance = 0;
double		trip_maxspeed = 0;
double		trip_time = 0;
double		trip_starttime = 0;
gboolean	trip_counter_on = TRUE;
gboolean	trip_logger_on = FALSE;

trackpoint_t	global_myposition;




gchar		*global_curr_reponame = NULL;
int		global_repo_cnt = 0;
GSList 		*global_repo_list = NULL;
GSList		*global_curr_repo = NULL;
GSList		*global_curr_trf_repo = NULL;
GConfClient	*global_gconfclient = NULL;

gboolean	global_auto_download = TRUE;

//----------flags----------------------------
gboolean trf_old=FALSE;//флаг запроса изменения показателя старости
gboolean trf_next=FALSE;//флаг запроса на загрузку свежих пробок
//----------------------------------------------------------
gboolean	pixbuf_is_null = FALSE;//нету такого тайла в хеше
gboolean	trf_is_failed = FALSE;//ошибка загрузки пробки по причине отсутствия
extern gboolean host_failed = FALSE;//ошибка разрешения имени хоста, скорее всего нет инета
GHashTable *ht = NULL;
int destroy_all_threads=0;
//--------------Traffic download & show----------------------
GSList *showed_tiles = NULL;
int traffic_old_factor=0;
gchar		*curr_trf_name = NULL;
GSList 		*global_trf_list = NULL;
gboolean	global_trf_auto = TRUE;
gboolean	global_trf_show= TRUE;
//--------------Traffic download & show----------------------
gboolean	global_mapmode = TRUE;
gboolean	global_autocenter = FALSE;
gboolean	global_reconnect_gpsd = TRUE;
int		global_tiles_in_dl_queue = 0;

gboolean	global_show_friends = FALSE;
gboolean	global_show_pois = FALSE;
gboolean	global_show_photos = FALSE;
int		global_poi_cat = 0;

gboolean	global_wp_on = FALSE;
waypoint_t	global_wp;

gboolean	global_ffupdate_auto = FALSE;
int		global_ffupdate_interval = 900000;
double		global_ffupdate_interval_minutes = 15;
gboolean 	global_fftimer_running = FALSE;
gboolean	global_ffcm_public = TRUE;
gboolean	global_ffcm_registered = TRUE;
gboolean	global_ffcm_friends = TRUE;
gboolean	global_ffcu_public = TRUE;
gboolean	global_ffcu_registered = TRUE;
gboolean	global_ffcu_friends = TRUE;

gchar		*global_server	= NULL;
gchar		*global_port	= NULL;

gchar		*global_home_dir = NULL;
gchar		*tangogps_dir = NULL;

int		mouse_x = 0;
int		mouse_y = 0;
