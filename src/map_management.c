#include <stdlib.h>
#include <sys/stat.h>

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gstring.h>
#include <glib/gprintf.h>
#include <gconf/gconf.h>
#include <math.h>
#include "support.h"
#include "globals.h"
#include "map_management.h"
#include "converter.h"
#include "tile_management.h"
#include "callbacks.h"
#include "friends.h"
#include "gps_functions.h"
#include "geo_photos.h"
#include "poi.h"
#include "wp.h"

#define ERR_ZOOM -1
#define ERR_LOAD -2
#define ERR_REPO -3
#define ERR_DOWN -4
#define LOAD_OK  1

static GtkWidget	*drawingarea11 = NULL;


void
view_tile(data_of_thread /*tile_threads* data*/ *local)
{
	gchar filename[256];

	local->x_glob=global_x;
	local->y_glob=global_y;//Чтобы отследить изменилось ли положение карты
	local->zoom=global_zoom;
	local->repo=global_curr_repo->data;
	number_threads = update_thread_number(1);
	printf("\n\n # of threads: %i \n\n", number_threads );

	if(!pixmap)
	{
		printf("no drawable -> NULL\n");
		return;
	}
	int mercator_x,mercator_y;	
//--------Yandex offset----------
	mercator_offset(local->zoom, local->x_glob, local->y_glob, &mercator_x, &mercator_y);
//--------Yandex offset----------

	printf("---repo dir: %s \n",g_strdup(local->repo->dir));

   	if (strcasestr(local->repo->name,"Yandex")!=NULL)
	{
		local->offset_x = - (mercator_x) % TILESIZE;
		local->offset_y = - (mercator_y) % TILESIZE;
		local->x =  floor((float)mercator_x / (float)TILESIZE) + local->i;
		local->y =  floor((float)mercator_y / (float)TILESIZE) + local->j;
	}
	else
	{
		local->offset_x = - global_x % TILESIZE;
		local->offset_y = - global_y % TILESIZE;
		local->x =  floor((float)(local->x_glob) / (float)TILESIZE) + local->i;
		local->y =  floor((float)(local->y_glob) / (float)TILESIZE) + local->j;
	}
	if (local->offset_x > 0) local->offset_x -= 256;
	if (local->offset_y > 0) local->offset_y -= 256;
	local->offset_x += TILESIZE*local->i;
	local->offset_y += TILESIZE*local->j;

//	if (load_tile(&local))
//	{
//number_threads = update_thread_number(-1);
//		return;
//	}
	load_tile(local);
/*
gdk_threads_enter();
	showed_tiles=g_slist_append(showed_tiles,filename);
gdk_threads_leave();
*/
	gchar* key;
	key = g_strdup_printf("%s/%d/%d/%d", local->repo->dir, local->zoom, local->x, local->y);
	g_hash_table_replace(ht,key,"MAP showing okay");

//----------Trafic visualization---------------
	if (global_trf_show)
	{
		printf("\n----------Trafic visualization---------------\n");
		printf("\n\nshow=%d ,auto=%d ,down=%d\n",global_trf_show,global_trf_auto,global_auto_download);

		if (strcasestr(curr_trf->name,"Yandex")!=NULL)
		{
			local->offset_x = - (mercator_x) % TILESIZE;
			local->offset_y = - (mercator_y) % TILESIZE;
			local->x =  floor((float)mercator_x / (float)TILESIZE) + local->i;
			local->y =  floor((float)mercator_y / (float)TILESIZE) + local->j;
		}
		else
		{
			local->offset_x = - local->x_glob % TILESIZE;
			local->offset_y = - local->y_glob % TILESIZE;
			local->x =  floor((float)(local->x_glob) / (float)TILESIZE) + local->i;
			local->y =  floor((float)(local->y_glob) / (float)TILESIZE) + local->j;
		}
		if (local->offset_x > 0) local->offset_x -= 256;
		if (local->offset_y > 0) local->offset_y -= 256;
		local->offset_x += TILESIZE*local->i;
		local->offset_y += TILESIZE*local->j;

		if (load_trf(local)==LOAD_OK)
		{
			load_tile(local);
			load_trf(local);
		}
//----------Trafic visualization---------------
	}

	if (number_threads==1)
	{
		printf ("load_tracks returned %d",load_tracks(local));
		printf ("load_tracks returned %d",load_current_track(local));
	}

//g_object_unref(data);
number_threads = update_thread_number(-1);
}

int
load_current_track(data_of_thread *local)
{
	printf("* load current track()\n");
	GSList *list;
	int pixel_x, pixel_y, x,y, last_x = 0, last_y = 0;
	float lat, lon;
	GdkColor color;

gdk_threads_enter();
	GdkGC *gc;
	gboolean is_line = FALSE;
	
	gc = gdk_gc_new(pixmap);                  
	color.green = 0;
	color.blue = 0;
	color.red = 60000;
	gdk_gc_set_rgb_fg_color(gc, &color);
	gdk_gc_set_line_attributes(gc,
		5, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);

	for(list = trackpoint_list; list != NULL; list = list->next)
	{
		trackpoint_t *tp = list->data;
	
		lat = tp->lat;
		lon = tp->lon;
		printf("track_point_lat = %d, trackpoint_lon = %d\n",lat,lon);
		
		pixel_x = lon2pixel(global_zoom, lon);
		pixel_y = lat2pixel(global_zoom, lat);
		
		x = pixel_x - global_x;
		y = pixel_y - global_y;

		if(is_line)
		{
			gdk_draw_line (pixmap, gc, x, y, last_x, last_y);
			gtk_widget_queue_draw_area (
				map_drawable, 
				x-4, y-4,
				8,8);
		printf("draw_track_point_x = %d, draw_trackpoint_y = %d, draw_trackpoint_last_x = %d, draw_trackpoint_last_y = %d\n",x,y,last_x,last_y);
		}
		
		last_x = x;
		last_y = y;
		
		is_line = TRUE;
	}
gdk_threads_leave();
	return 0;
}

int
load_tracks(data_of_thread *local)
{
	printf("* load tracks()\n");
	GSList *list;
	int pixel_x, pixel_y, x,y, last_x = 0, last_y = 0;
	float lat, lon;
	GdkColor color;


gdk_threads_enter();
	GdkGC *gc;
	gboolean is_line = FALSE;
	
	gc = gdk_gc_new(pixmap);                  
	color.green = 50000;
	color.blue = 0;
	color.red = 0;
	gdk_gc_set_rgb_fg_color(gc, &color);
	gdk_gc_set_line_attributes(gc,
		5, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);

	for(list = loaded_track; list != NULL; list = list->next)
	{
		trackpoint_t *tp = list->data;
	
		lat = tp->lat;
		lon = tp->lon;
		
		pixel_x = lon2pixel(global_zoom, lon);
		pixel_y = lat2pixel(global_zoom, lat);
		
		x = pixel_x - global_x;
		y = pixel_y - global_y;

		if(is_line)
		{
			gdk_draw_line (pixmap, gc, x, y, last_x, last_y);
			gtk_widget_queue_draw_area (
				map_drawable, 
				x-4, y-4,
				8,8);
		}
		
		last_x = x;
		last_y = y;
		
		is_line = TRUE;
	}
gdk_threads_leave();

//gdk_threads_enter();
//	gdk_draw_pixbuf (
//			pixmap,
//			gc_map,
//			pixbuf_tracks,
//			0,0,
//			local->offset_x-(global_x-local->x_glob),local->offset_y-(global_y-local->y_glob),
//			TILESIZE,TILESIZE,
//			GDK_RGB_DITHER_NONE, 0, 0);
//
//	drawingarea11 = lookup_widget(window1, "drawingarea1");
//	
//	gtk_widget_queue_draw_area (
//		drawingarea11, 
//		local->offset_x-(global_x-local->x_glob),local->offset_y-(global_y-local->y_glob),
//		TILESIZE,TILESIZE);
//gdk_threads_leave();
return 0;
}

int
load_tile(data_of_thread *local)
{
GdkPixbuf	*pixbuf=NULL;
GError		*error = NULL;
GdkGC		*gc_map = NULL;

//static GtkWidget	*drawingarea11 = NULL;
	gchar filename[256];

	printf("* load tile()\n");
	
/*	if(gc_map)
		g_object_unref(gc_map);
*/	
	g_sprintf(filename, "%s/%u/%u/%u.png", local->repo->dir, local->zoom, local->x, local->y);
	printf("IMG: %s\n",filename);

	pixbuf = gdk_pixbuf_new_from_file (
			filename,
			&error);

		GtkWidget *widget;
			
		widget = lookup_widget(window1, "drawingarea1");
			
	if(pixbuf == NULL || ((local->x < 0) || (local->y < 0) || (local->x > exp(local->zoom * M_LN2)) || (local->y > exp(local->zoom * M_LN2))))
	{
		printf("*** Not downloading tile \n");
gdk_threads_enter();
		gdk_draw_rectangle (
			pixmap,
			widget->style->white_gc,
			TRUE,
			local->offset_x, local->offset_y,
			256,
			256);
						
		gtk_widget_queue_draw_area (
			widget, 
			local->offset_x,local->offset_y,256,256);
gdk_threads_leave();
		
		printf("load_tile: error loading png\n");
		error = NULL;

		if (global_auto_download && !host_failed)
		{
			download_tile(local->repo,local->zoom,local->x,local->y);
			if (local->zoom!=global_zoom) return ERR_ZOOM;
			if (local->repo!=global_curr_repo->data) return ERR_REPO;
			pixbuf = gdk_pixbuf_new_from_file (       // ?????????
					filename,
					&error);
			if (pixbuf==NULL)
				return ERR_LOAD;
		}
		else
		{
			return ERR_LOAD;
		}
	}	
gdk_threads_enter();
	gdk_draw_pixbuf (
			pixmap,
			gc_map,
			pixbuf,
			0,0,
			local->offset_x-(global_x-local->x_glob),local->offset_y-(global_y-local->y_glob),
			TILESIZE,TILESIZE,
			GDK_RGB_DITHER_NONE, 0, 0);

		g_object_unref (pixbuf);
	
	drawingarea11 = lookup_widget(window1, "drawingarea1");
	
	gtk_widget_queue_draw_area (
		drawingarea11, 
		local->offset_x-(global_x-local->x_glob),local->offset_y-(global_y-local->y_glob),
		TILESIZE,TILESIZE);
gdk_threads_leave();
return 0;
}

void
fill_tiles_pixel(	int pixel_x,
			int pixel_y,
			int zoom)
{
	int i,j, tile_count_x, tile_count_y;
	gboolean success = FALSE;
	GError **error = NULL;
//		if (gpsdata!=NULL)
//		{
//			printf("time=%f\n",gpsdata->fix.time);
//			global_time = gpsdata->fix.time;
//		}
//		else	
//		
	
	printf("*** %s(): xyz %d %d %d, time=%d\n",__PRETTY_FUNCTION__,global_x,global_y,global_zoom,global_time);
	
	if (showed_tiles)
	{
		while(g_slist_length(showed_tiles))
		{
			showed_tiles=g_slist_delete_link(showed_tiles,showed_tiles);
			printf("Длина списка - %d\n",g_slist_length(showed_tiles));
		}
	}
	
	tile_count_x = map_drawable->allocation.width / TILESIZE + 2;
	tile_count_y = map_drawable->allocation.height / TILESIZE + 2;
	
	for (i=0; i<tile_count_x;i++)
	{
		for (j=0;  j<tile_count_y; j++)
		{
				key = g_strdup_printf("%s/%d/%d/%d", curr_trf->dir, zoom, i, j);
				gchar* found=g_hash_table_lookup(ht,key);
				printf("found = %s\n",found);
//			if (!found)
				{
					g_hash_table_replace(ht,key,"load_tile()");
//-------------------create thread_data------------------
//					tile_threads *thread_data = g_new0(tile_threads, 1);
					data_of_thread *thread_data = g_new0(data_of_thread, 1);
					thread_data->i = i;
					thread_data->j = j;
//-------------------create thread_data------------------

					if (!g_thread_create(&view_tile, thread_data, FALSE, NULL) != 0)
					{	
						g_warning("can't create VIEW TILE thread");
					}
				}
		}
	}
	
/*	
	
		if (offset_x > 0) offset_x -= 256;
		if (offset_y > 0) offset_y -= 256;
		global_x = pixel_x;
		global_y = pixel_y;
		global_zoom = zoom;
	
		offset_xn = offset_x; 
		offset_yn = offset_y;
	
		width  = map_drawable->allocation.width;
		height = map_drawable->allocation.height;

		tiles_nx = floor((width  - offset_x) / TILESIZE) + 1;
		tiles_ny = floor((height - offset_y) / TILESIZE) + 1;
	
	   	if (strcasestr(curr_trf->name,"Yandex")!=NULL)
		{
			tile_x0 =  floor((float)(pixel_x + merkator_offset_x) / (float)TILESIZE);
			tile_y0 =  floor((float)(pixel_y + merkator_offset_y) / (float)TILESIZE);
		}
		else
		{
			tile_x0 =  floor((float)(pixel_x) / (float)TILESIZE);
			tile_y0 =  floor((float)(pixel_y) / (float)TILESIZE);
		}

		for (i=tile_x0; i<(tile_x0+tiles_nx);i++)
		{
			for (j=tile_y0;  j<(tile_y0+tiles_ny); j++)
			{
				printf("---i,j,x,y,offsetx,y: %d,%d -- %d,%d -- %d,%d\n",i,j,pixel_x,pixel_y,offset_x,offset_y);

				key = g_strdup_printf("%s/%d/%d/%d", curr_trf->dir, zoom, i, j);
				gchar* found=g_hash_table_lookup(ht,key);
				printf("found = %s\n",found);
	
//--------------------create thread_data--------------------
	tile_threads *thread_data = g_new0(tile_threads, 1);
	thread_data->x = i;
	thread_data->y = j;
	thread_data->offset_x = offset_xn;
	thread_data->offset_y = offset_yn;
//--------------------create thread_data--------------------

//		if (!g_thread_create(&load_trf, thread_data, FALSE, NULL) != 0)
//		{	
//			g_warning("can't create DL thread");
//		}
		
//						load_trf(thread_data);
				offset_yn += TILESIZE;
			}
			offset_xn += TILESIZE;
			offset_yn = offset_y;
		}	

//		if (trf_old) traffic_old_factor++;
		if (global_trf_auto)
		{
			printf ("До загрузки пробок осталось %d секунд\n", (traffic_time + 250 +240 - global_time));
			g_timeout_add_seconds ((traffic_time + 250 - global_time), auto_load_trf_timer, NULL);
		}
	}	
*/
//----------Trafic visualization---------------

	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/global_x",
				global_x,
				error);
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/global_y",
				global_y,
				error);
	success = gconf_client_set_int(
				global_gconfclient, 
				GCONF"/global_zoom",
				global_zoom,
				error);

}

void
fill_tiles_latlon(	float lat,
			float lon,
			int zoom)
{
	int pixel_x, pixel_y;
	
	
	pixel_x = lon2pixel(zoom, lon);

	pixel_y = lat2pixel(zoom, lat);

 
	printf("fill_tiles_latlon(): lat %f  %i -- lon %f  %i\n",lat,pixel_y,lon,pixel_x);
	
	
	fill_tiles_pixel (pixel_x, pixel_y, zoom);
}

void
set_mapcenter(	float lat,
		float lon,
		int zoom)
{
	int pixel_x, pixel_y;
	
	lat = deg2rad(lat);
	lon = deg2rad(lon);
	
	
	pixel_x = lon2pixel(zoom, lon);
	pixel_y = lat2pixel(zoom, lat);

	printf("fill_tiles_latlon(): lat %f  %i -- lon %f  %i\n",
	lat,pixel_y,lon,pixel_x);
	
	osd_speed();
	global_x = pixel_x - global_drawingarea_width/2; 
	global_y = pixel_y - global_drawingarea_height/2; 
	global_zoom = zoom;
	fill_tiles_pixel (	pixel_x - global_drawingarea_width/2, 
				pixel_y - global_drawingarea_height/2,
				zoom);
	print_track();
	paint_friends();
	paint_photos();
	paint_pois();
	paint_wp();
	osd_speed(); 


}



void
fill_tiles_latlon_hack(	float lat,
			float lon,
			int zoom)
{
	int pixel_x, pixel_y;
	
	
	pixel_x = lon2pixel(zoom, lon);

	pixel_y = lat2pixel(zoom, lat);

	printf("fill_tiles_latlon(): lat %f  %i -- lon %f  %i\n",
	lat,pixel_y,lon,pixel_x);
	
	fill_tiles_pixel (pixel_x - 240, pixel_y - 300, zoom);
}



void mercator_offset(int zoom, int pixel_x, int pixel_y, int* mercator_x, int* mercator_y)
{
//--------------Yandex repository offset ------------------------
	    {
            float lat=rad2deg(pixel2lat((float)zoom,pixel_y));
            float lon=rad2deg(pixel2lon((float)zoom,pixel_x));
//----Shmuma autor---
            float tmp = tan (M_PI_4 + deg2rad (lat) / 2.0);
            float pow_tmp = pow (tan (M_PI_4 +  asin (YANDEX_E * sin (deg2rad (lat))) / 2.0), YANDEX_E);
            *mercator_x = (int)((YANDEX_Rn * deg2rad (lon) + YANDEX_A) * YANDEX_F)>>(23-zoom);
            *mercator_y = (int)((YANDEX_A - (YANDEX_Rn * log (tmp / pow_tmp))) * YANDEX_F)>>(23-zoom);
        }
//--------------Yandex repository offset ------------------------
}

//--------------Traffic download--------------------------------
int
load_trf(data_of_thread *local)
{

GdkPixbuf	*pixbuf = NULL;
GError		*error = NULL;
GdkGC		*gc_map = NULL;

//static GtkWidget	*drawingarea11 = NULL;

	printf("* load_trf()\n");
	
	gchar* filename;

	gchar* key;

	filename = g_strdup_printf("%s/%d/%d/%d.png", curr_trf->dir, local->zoom, local->x, local->y);

	pixbuf = gdk_pixbuf_new_from_file (
			filename,
			&error);
	if(pixbuf)
	{
		gdk_threads_enter();
			gdk_draw_pixbuf (
					pixmap,
					gc_map,
					pixbuf,
					0,0,
					local->offset_x-(global_x-local->x_glob),local->offset_y-(global_y-local->y_glob),
					TILESIZE,TILESIZE,
					GDK_RGB_DITHER_NONE, 0, 0);
		
		
			g_object_unref (pixbuf);
				
			drawingarea11 = lookup_widget(window1, "drawingarea1");
			
			gtk_widget_queue_draw_area (
				drawingarea11, 
				local->offset_x-(global_x-local->x_glob),local->offset_y-(global_y-local->y_glob),
				TILESIZE,TILESIZE);
		gdk_threads_leave();
	}	

	if (global_trf_auto && !host_failed)
	{
//Информация о загрузке данного тайла пробок
	key = g_strdup_printf("%s/%d/%d/%d_time", curr_trf->dir, local->zoom, local->x, local->y);

	static time_t	load_time=0; //время, когда был загружен tile пробок
	time_t* temp;
	temp= g_hash_table_lookup(ht,key);
	if (temp==0)
		{
			struct stat file_info;
			if (!stat(filename,&file_info) && pixbuf)
				load_time=file_info.st_mtime;
			else load_time=0;
		}
	else
		load_time=*temp;

				global_time=time(NULL);
				struct tm*  local_time;
				local_time=localtime(&global_time);//local time of this machine
				printf ("%d:%d:%d\n", local_time->tm_hour,  local_time->tm_min,  local_time->tm_sec);
				int temptime=local_time->tm_min*60+local_time->tm_sec;//Прошло времени с момента начала нового часа
				traffic_time=global_time-temptime+(int)(temptime/240)*240-240;
				printf("Возраст пробок %i\n", global_time-traffic_time);//время с момента создания новых карт движения, т.е. возраст последних пробок на яндексе.
				printf("global_time = %i\n", global_time);
				printf("traffic_time = %i\n", traffic_time);
				printf("load_time = %i\n", load_time);
		if (traffic_time > load_time) // Появились новые тайлы пробок
		{
			printf ("Закачиваем пробки\n");
			time_t *temp_time=malloc(sizeof(time_t));
			memcpy(temp_time,&traffic_time,sizeof(temp_time));
			g_hash_table_replace(ht,key,temp_time);
			download_tile(curr_trf,local->zoom,local->x,local->y);
			if (local->zoom!=global_zoom) return ERR_ZOOM;
			pixbuf = gdk_pixbuf_new_from_file (
					filename,
					&error);
			if (!pixbuf) return ERR_LOAD;
			else 
			{
				g_object_unref (pixbuf);
				return LOAD_OK;
			}
		}
//неправильно Если буфер пустой (мы уже закачиваем)
/*		if (found!=NULL)
		if (!host_failed && strcmp(found,"The requested URL returned error: 404")==0 && traffic_old_factor<5) //карта не была загружена, инет есть	
			{	
//написать обнуление хеша,где 			The requested URL returned error: 404`
		//		traffic_old_factor++;
				printf ("found = %s\n",found);
				printf ("Закачиваем старые пробки\n");
				trf_old=TRUE;
				printf ("показатель старости пробок - %d",traffic_old_factor);
				pixbuf_is_null = TRUE;
				download_tile(curr_trf,global_zoom,x,y);
			}	
*/	}
	
//	if(pixbuf == NULL)
//	{
//		printf("PIXBUF: error loading png\n");
//		error = NULL;
//	}
//	else
//		printf("PIXBUF: loading okay\n");
//	
//
//gdk_threads_enter();
//	gdk_draw_pixbuf (
//			pixmap,
//			gc_map,
//			pixbuf,
//			0,0,
//			local->offset_x-(global_x-local->x_glob),local->offset_y-(global_y-local->y_glob),
//			TILESIZE,TILESIZE,
//			GDK_RGB_DITHER_NONE, 0, 0);
//
//
//	g_object_unref (pixbuf);
//		
//	drawingarea11 = lookup_widget(window1, "drawingarea1");
//	
//	gtk_widget_queue_draw_area (
//		drawingarea11, 
//		local->offset_x-(global_x-local->x_glob),local->offset_y-(global_y-local->y_glob),
//		TILESIZE,TILESIZE);
//gdk_threads_leave();
return 0;
}
//--------------Traffic download--------------------------------
//--------------Traffic autodownloadtimer--------------------------------
gboolean auto_load_trf_timer()
{
	repaint_all();

	return FALSE;
}
//--------------Traffic autodownloadtimer--------------------------------

