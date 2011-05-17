

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib/gprintf.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gps_functions.h"
#include "globals.h"
#include "support.h"
#include "tile_management.h"
#include "converter.h"

static GIOChannel *gpsd_io_channel =NULL;

static guint sid1,  sid3; 
guint watchdog;

gboolean
reset_gpsd_io()
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	
	global_reconnect_gpsd = TRUE;
	g_source_remove(watchdog);

	g_source_remove(sid1); 
	g_source_remove(sid3); 
	
	return FALSE;	
}

/* 
 * Изображение скорости
 * на карте
 */
void
osd_speed()
{
	PangoContext		*context = NULL;
	PangoLayout		*layout  = NULL;
	PangoFontDescription	*desc    = NULL;
	
	GdkColor color;
	GdkGC *gc;
	
	gchar *buffer;
	static int x = 10, y = 10;
	static int width = 0, height = 0;

	static double speed_tmp = 0;
	
	double unit_conv = 1;

	if(gpsdata && mouse_dx == 0 && mouse_dy == 0) 
	{
		switch (global_speed_unit)
		{
			case 0:
				unit_conv = 1.0;
				break;
			case 1 :
				unit_conv = 1.0/1.609344;
				break;
			case 2 :
				unit_conv = 1.0 / 1.852;
				break;		
		}
	
		buffer = g_strdup_printf("%.0f", gpsdata->fix.speed*3.6*unit_conv);
		
		context = gtk_widget_get_pango_context (map_drawable);
		layout  = pango_layout_new (context);
		desc    = pango_font_description_new();
		
		pango_font_description_set_size (desc, 40 * PANGO_SCALE);
		pango_layout_set_font_description (layout, desc);
		pango_layout_set_text (layout, buffer, strlen(buffer));
	
	
		gc = gdk_gc_new (map_drawable->window);
	
		color.red = (gpsdata->fix.speed*3.6*unit_conv > 60) ? 0xffff : 0;
		color.green = 0;
		color.blue = 0;
		
		gdk_gc_set_rgb_fg_color (gc, &color);
	
		if(speed_tmp != floor(gpsdata->fix.speed*3.6*unit_conv))
		{
			gdk_draw_drawable (
				map_drawable->window,
				map_drawable->style->fg_gc[GTK_WIDGET_STATE (map_drawable)],
				pixmap,
				0,0,
				0, 0,
				width+10,height+10);
		
			if(gpsdata->fix.speed>1.0 && gpsdata->valid) 
				gdk_draw_layout(map_drawable->window,
						gc,
						x, y,
						layout);
			
			pango_layout_get_pixel_size(layout, &width, &height);
		}
		
		speed_tmp = floor(gpsdata->fix.speed*3.6*unit_conv);
		
		g_free(buffer);
		pango_font_description_free (desc);
		g_object_unref (layout);
		g_object_unref (gc);
	}
}

void
set_label_nogps()
{
	static GtkLabel *label=NULL;
	static gchar buffer[256];
	int num_dl_threads = 0;

	if(label == NULL)
		label   = GTK_LABEL(lookup_widget(window1, "label4"));
	
	num_dl_threads = update_thread_number(0);
	if(num_dl_threads && !global_tiles_in_dl_queue)
	{	
		g_sprintf(buffer, 
			"<b>no GPS found</b> - <span foreground='#0000ff'><b>D%d</b></span>",
			num_dl_threads);
	}
	else if (num_dl_threads && global_tiles_in_dl_queue)
		g_sprintf(buffer, 
			"<b>no GPSD found</b> - <span foreground='#0000ff'><b>D%d</b></span> - <b>[%d]</b>",
			num_dl_threads, global_tiles_in_dl_queue);
	else
		g_sprintf(buffer,"<b>no GPS found</b>");
	
	
	gtk_label_set_label(label, buffer);

}

/* 
 * Set labels in bottom window bar (speed,lat,lon...)
 */
void
set_label()
{
	static GtkLabel *label=NULL,   *label31, *label38, *label39;
	static GtkLabel *label41, *label42, *label43, *label45;
	static GtkLabel *label66, *label68, *label70;
	static gchar buffer[256];
	static gchar numdl_buf[64], dl_buf[64], ff_buf[64], tr_buf[64];
	static gchar speedunit[5], distunit[3], altunit[3];
	int trip_hours, trip_minutes, trip_seconds;
	int num_dl_threads = 0;
	time_t time_sec;
	struct tm  *ts;
	double unit_conv = 1, unit_conv_alt = 1;
	
	osd_speed();
	
	if(global_speed_unit==1)
	{
		unit_conv = 1.0/1.609344;
		g_sprintf(speedunit, "%s","mph");
		g_sprintf(distunit, "%s", "m");
	}
	else if(global_speed_unit==2)
	{
		unit_conv = 1.0/1.852;
		g_sprintf(speedunit, "%s","kn");
		g_sprintf(distunit, "%s", "NM");
	}
	else
	{
		g_sprintf(speedunit, "%s","km/h");
		g_sprintf(distunit, "%s", "km");
	}
	
	
	if(global_alt_unit==1)
	{
		unit_conv_alt = 1.0/0.3048;
		g_sprintf(altunit, "%s", "ft");
	}
	else
		g_sprintf(altunit, "%s", "m");
	
	if(global_auto_download)
		g_sprintf(dl_buf, "%s", "");
	else
		g_sprintf(dl_buf, "%s", "<span foreground='#ff0000'><b>!</b></span>");
	
	if (global_fftimer_running)
		g_sprintf(ff_buf, "%s", "<span foreground='#00e000'><b>f</b></span>");
	else
		g_sprintf(ff_buf, "%s", "");
	
	if (trip_logger_on)
		g_sprintf(tr_buf, "%s", "<span foreground='#00e000'><b>t</b></span>");
	else
		g_sprintf(tr_buf, "%s", "");
	
	if(label == NULL)
	{
		label   = GTK_LABEL(lookup_widget(window1, "label4"));
		label45 = GTK_LABEL(lookup_widget(window1, "label45"));
		label41 = GTK_LABEL(lookup_widget(window1, "label41"));
		label31 = GTK_LABEL(lookup_widget(window1, "label31"));
		label38 = GTK_LABEL(lookup_widget(window1, "label38"));
		label39 = GTK_LABEL(lookup_widget(window1, "label39"));
		label42 = GTK_LABEL(lookup_widget(window1, "label42"));
		label43 = GTK_LABEL(lookup_widget(window1, "label43"));
		label66 = GTK_LABEL(lookup_widget(window1, "label66"));
		label68 = GTK_LABEL(lookup_widget(window1, "label68"));
		label70 = GTK_LABEL(lookup_widget(window1, "label70"));
	}


	
	
	
	num_dl_threads = update_thread_number(0);
	
	if(num_dl_threads && !global_tiles_in_dl_queue)
		g_sprintf(numdl_buf, "<span foreground='#0000ff'><b>D%d</b></span> ", 
			  num_dl_threads);
	else if (num_dl_threads && global_tiles_in_dl_queue)
		g_sprintf(numdl_buf, "<span foreground='#0000ff'><b>D%d</b></span>-%d ", 
			  num_dl_threads, global_tiles_in_dl_queue);
	else
		g_sprintf(numdl_buf, "%s", "");
	

	g_sprintf(buffer, 
		"%s%s%s%s<b>%4.1f</b> %s  %.0f° a:%.0f%s  t:%.3f%s  %d/%d/%.1f",
		numdl_buf,
		dl_buf,
		tr_buf,
		ff_buf,
		gpsdata->fix.speed * 3.6 * unit_conv,	speedunit,
		gpsdata->fix.track * unit_conv,
		gpsdata->fix.altitude * unit_conv_alt,	altunit,
		trip_distance * unit_conv,		distunit,
		gpsdata->satellites_inview,
		gpsdata->satellites_used,
		gpsdata->hdop);


	gtk_label_set_label(label, buffer);
	
	time_sec = (time_t)gpsdata->fix.time;
	ts = localtime(&time_sec);
	
	strftime(buffer, sizeof(buffer), "%a %Y-%m-%d %H:%M:%S ", ts); 
	gtk_label_set_label(label41,buffer);

	switch (global_latlon_unit)
	{
		case 0:
			g_sprintf(buffer, "%f - %f", gpsdata->fix.latitude, gpsdata->fix.longitude);
			break;
		case 1:
			g_sprintf(buffer, "%s   %s", 
				  latdeg2latmin(gpsdata->fix.latitude),
				  londeg2lonmin(gpsdata->fix.longitude));
			break;
		case 2:
			g_sprintf(buffer, "%s   %s", 
				  latdeg2latsec(gpsdata->fix.latitude),
				  londeg2lonsec(gpsdata->fix.longitude));
	}
	gtk_label_set_label(label31,buffer);
	
	
	g_sprintf(buffer, 
		"<b><span foreground='#0000ff'><span font_desc='40'>%.1f</span></span></b> %s", 
		gpsdata->fix.speed*3.6*unit_conv, speedunit);
	gtk_label_set_label(label38,buffer);

	
	g_sprintf(buffer, "%.1f %s", gpsdata->fix.altitude * unit_conv_alt, altunit);
	gtk_label_set_label(label39,buffer);
	
	
	g_sprintf(buffer, "%.1f° ", gpsdata->fix.track);
	gtk_label_set_label(label42,buffer);
	
	
	g_sprintf(buffer, "%d/%d    HDOP: %.1f ", 
			gpsdata->satellites_inview, gpsdata->satellites_used, gpsdata->hdop);
	gtk_label_set_label(label43,buffer);

	
	
	

	
	g_sprintf(buffer, "%.3f %s", trip_distance*unit_conv,distunit);
	gtk_label_set_label(label45,buffer);


	
	trip_hours   = trip_time / 3600;
	trip_minutes = ((int)trip_time%3600)/60;
	trip_seconds = (int)trip_time % 60;
	
	if (trip_seconds < 10 && trip_minutes < 10)
	{
		g_sprintf(buffer, "  %d:0%d:0%d",trip_hours,trip_minutes,trip_seconds);
	}
	else if (trip_seconds < 10)
		g_sprintf(buffer, "  %d:%d:0%d",trip_hours,trip_minutes,trip_seconds);
	else if (trip_minutes < 10)
		g_sprintf(buffer, "  %d:0%d:%d",trip_hours,trip_minutes,trip_seconds);
	else
		g_sprintf(buffer, "  %d:%d:%d",trip_hours,trip_minutes,trip_seconds);

	gtk_label_set_label(label66,buffer);

	
	g_sprintf(buffer, " %.1f %s", trip_distance*3600*unit_conv/(trip_time+2.0), speedunit);
	gtk_label_set_label(label68,buffer);

	
	g_sprintf(buffer, " %.1f %s", trip_maxspeed*3.6*unit_conv, speedunit);
	gtk_label_set_label(label70,buffer);
}

/* 
 * Get gps reciever information
 * time,day information
 * lat, lon, speed, 
 * track ------------ ??
 * creat gpsdata struct 
 */
void
parse_nmea_rmc(char *nmea)
{
	gchar **array;
	gchar lat_dec[3], lon_dec[4];
	double	lat_min, lat=0, lon_min,lon=0;
	gchar hour[3],min[3],sec[3],year[3],month[3],day[3];
	int i=0;
	
	typedef struct tm tm_t;
	
	static tm_t *tm = NULL;

	if(!tm) tm = g_new0(tm_t,1);
		
	array = g_strsplit(nmea,",",0);
	
	while (array[i]) i++;

	g_source_remove(watchdog);
	watchdog = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE,10000,reset_gpsd_io,NULL,NULL);
	
	if(i>=9)
	{
		if (strlen(array[1]) >= 6)
		{
			strncpy(hour, array[1],2);
			hour[2]='\0';
			strncpy(min, array[1]+2,2);
			min[2]='\0';
			strncpy(sec, array[1]+4,2);
			sec[2]='\0';
		}
		
		if (strlen(array[9]) == 6)
		{
			strncpy(day, array[9],2);
			day[2]='\0';
			strncpy(month, array[9]+2,2);
			month[2]='\0';
			strncpy(year, array[9]+4,2);
			year[2]='\0';

			tm->tm_sec = atoi(sec);
			tm->tm_min = atoi(min);
			tm->tm_hour= atoi(hour);
			tm->tm_mday= atoi(day);
			tm->tm_mon = atoi(month)-1;
			tm->tm_year= atoi(year)+100;
	
			gpsdata->fix.time = (double) mktime(tm);
		}
		
		if (strlen(array[3]) >= 9)
		{
			strncpy(lat_dec, array[3], 2);
			lat_dec[2]='\0';
			lat_min = atof(array[3]+2);
			lat = atof(lat_dec) + (lat_min/60);
			if (strcmp(array[4],"S")==0)
			{
				lat = -lat;
			}
		}
		
		if (strlen(array[5]) >= 10)
		{
			strncpy(lon_dec, array[5], 3);
			lon_dec[3]='\0';
			lon_min = atof(array[5]+3);
			lon = atof(lon_dec) + (lon_min/60);
			if (strcmp(array[6],"W")==0)
			{
				lon = -lon;
			}
		}
		
		gpsdata->valid = (strcmp(array[2],"A")==0) ? TRUE : FALSE;
		if(gpsdata->valid)
		{
			gpsdata->seen_vaild = TRUE;
			gpsdata->fix.latitude = lat;
			gpsdata->fix.longitude = lon;
			gpsdata->fix.speed = atof(array[7])*0.514444; 
			gpsdata->fix.track = atof(array[8]);
		}
	}
	g_strfreev(array);

}


/* 
 * altitude information
 * satellite used information
 */
void
parse_nmea_gga(char *nmea)
{		
	gchar **array;
	int i=0;


	array = g_strsplit(nmea,",",0);

	
	while (array[i]) 
		i++;

	if(i>=9)
	{
		gpsdata->satellites_used = atoi(array[7]);
		
		if(atoi(array[6])>0)  
		{
			gpsdata->hdop = atof(array[8]);
			gpsdata->fix.altitude = atof(array[9]);
		}
	}
	g_strfreev(array);

}

/* 
 * satellite view information
 */
void
parse_nmea_gsv(char *nmea)
{		
	gchar **array;
	int i=0;
	
	array = g_strsplit(nmea,",",0);

	while (array[i]) i++;
	
	if (i>=3)
		gpsdata->satellites_inview = atoi(array[3]);

	g_strfreev(array);

}


static gboolean
cb_gpsd_io_error(GIOChannel *src, GIOCondition condition, gpointer data)
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	g_free(gpsdata);
	gpsdata = NULL;
	g_source_remove(sid1); 
	g_source_remove(sid3); 
	
	return FALSE; 
}

/* 
 * get gpsdata struct
 */
static gboolean
cb_gpsd_data(GIOChannel *src, GIOCondition condition, gpointer data)
{
	gsize length;
	GError *error = NULL;
	gchar *str_return;
	GIOStatus status;

	status =  g_io_channel_read_line(
				gpsd_io_channel,
				&str_return,
				&length,
				NULL,
				&error);

	if(status == G_IO_STATUS_NORMAL)
	{		
		if (strncmp(str_return,"$GPGSV",6)==0)
		{
			parse_nmea_gsv(str_return);
		}
		else if(strncmp(str_return,"$GPGGA",6)==0)
		{
			parse_nmea_gga(str_return);
		}
		else if (strncmp(str_return,"$GPRMC",6)==0)
		{
			parse_nmea_rmc(str_return);
		}
	}
	else
	{
		if (error)
			printf("%s \n", error->message);
	}
	
	g_free(str_return);

	return TRUE;
}

void
get_gps()
{
	int res;
	static int sock = 0;
	int conn, len;
	char buffer[501]; 
	struct sockaddr_in server;
	char buffer_send[] = "r";

	if(!gpsdata  || global_reconnect_gpsd)
	{
		printf("reconnecting to gpsd\n");
				
		if (sock) sock = close(sock);
			
		server.sin_family	= AF_INET;
		server.sin_addr.s_addr	= inet_addr(global_server);
		server.sin_port		= htons (atoi(global_port));
		memset(&(server.sin_zero), '\0', 8);		
		
		sock = socket(AF_INET, SOCK_STREAM, 0);
		conn = connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr));
		if (conn < 0)
		{
			fprintf(stderr, "connection to gpsd FAILED \n");
		}
		else
		{
			fprintf(stderr, "connection to gpsd SUCCEEDED \n");
			
			global_reconnect_gpsd = FALSE;
			
			if(!gpsdata)
			{
				gpsdata = g_new0(gps_data_t,1);
			}
			
			len = write(sock, buffer_send, strlen(buffer_send));
			if (len < 0)
				perror("ERROR writing to socket");
			
			
			len = recv(sock, buffer, 500, 0);
			
			buffer[len]='\0'; 
			
			fprintf(stderr, "Rcvd: %s",buffer);
			
			watchdog = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE,10000,reset_gpsd_io,NULL,NULL);
			
			gpsd_io_channel = g_io_channel_unix_new(sock);
			g_io_channel_set_flags(gpsd_io_channel, G_IO_FLAG_NONBLOCK, NULL);
			
			sid1 = g_io_add_watch_full(gpsd_io_channel, G_PRIORITY_HIGH_IDLE+200, G_IO_ERR | G_IO_HUP, cb_gpsd_io_error, NULL, NULL);
			
			
			sid3 = g_io_add_watch_full(gpsd_io_channel, G_PRIORITY_HIGH_IDLE+200, G_IO_IN | G_IO_PRI, cb_gpsd_data, NULL, NULL);
		}
	}
	
	if(gpsdata)
	{
		res = 5;
		if (res==-1){
			printf("POLL ERROR \n");
			gpsdata=NULL;
		}
	}
	else printf("NOGPS");
}
