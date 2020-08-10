/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2020 Chris Moller

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <values.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <plplot.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-svg.h>
#include <apl/libapl.h>
				
#include "aplvis.h"
#include "curves.h"

static char            *newfn;
static FILE            *newout;
static int              newfd;
static GFileMonitor    *monitor_file;
static gulong           monitor_sigid;
static GtkWidget       *status;
static gint             granularity;


#define DEFAULT_WIDTH  480
#define DEFAULT_HEIGHT 320
gint             width           = DEFAULT_WIDTH;
gint             height          = DEFAULT_HEIGHT;
GtkWidget       *window          = NULL;
GtkWidget       *da              = NULL;
GtkWidget       *title              = NULL;
GtkAdjustment   *gran_adj	 = NULL;
GtkWidget       *gran_spin	 = NULL;


static gboolean
da_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  PLPointer pbuf = NULL;
  cairo_surface_t *surface = NULL;
  guint width = gtk_widget_get_allocated_width (widget);
  guint height = gtk_widget_get_allocated_height (widget);
  pbuf = g_malloc0 (4 * width * height);
  if (pbuf) {
    plsdev ("memcairo");
    plsmema ((PLINT)width, (PLINT)height, pbuf);
    surface =
      cairo_image_surface_create_for_data (pbuf,
                                           CAIRO_FORMAT_ARGB32,
                                           (PLINT)width,
                                           (PLINT)height,
                                           4 * (PLINT)width);
    cairo_set_source_surface (cr, surface, 0, 0);

#if 1
    cairo_rectangle (cr, 0.0, 0.0, (PLFLT)width, (PLFLT)height);
    cairo_set_source_rgba (cr, 0.5, 0.6, 0.7, 1.0);
#else
    plinit();
    int cx;
    for (cx = 0; cx < 16; cx++)
      plscol0a (cx,
                trunc (255.0 * base_colours[cx].red),
                trunc (255.0 * base_colours[cx].green),
                trunc (255.0 * base_colours[cx].blue),
                base_colours[cx].alpha);


    /* set bg */
    cairo_rectangle (cr, 0.0, 0.0, (PLFLT)width, (PLFLT)height);
    cairo_set_source_rgba (cr,
                           bg_colour.red,
                           bg_colour.green,
                           bg_colour.blue,
                           bg_colour.alpha);
    cairo_fill (cr);

    plcol0 (0);                 // sets axes
    if (xvec) {
      plenv (xmin, xmax, ymin, ymax, 0, 0);
      plcol0 (7);                       // sets axes
      pllab (x_label, y_label, gtk_entry_get_text (GTK_ENTRY (title)));
      plssym (0.0, 0.75);
      plcol0 (5);               // sets curves
      plline (count, xvec, yvec);
    }

    plend ();
#endif


    cairo_paint (cr);
    g_free (pbuf);
  }
  else {                // fixme error
  }

  return GDK_EVENT_STOP;
}

static void
spin_changed_cb (GtkSpinButton *spin_button,
                 gpointer       user_data)
{
  //  expression_activate_cb (NULL, NULL);
}

#if 0
static void
go_button_cb (GtkButton *button,
              gpointer   user_data)
{
  expression_activate_cb (NULL, NULL);
}
#endif

static void
curves_button_cb (GtkButton *button,
		  gpointer   user_data)
{
  curves_screen ();
}

static void
go_button_cb (GtkButton *button,
              gpointer   user_data)
{
  //  expression_activate_cb (NULL, NULL);
}


static void
aplvis_quit (GtkWidget *object, gpointer data)
{
  unlink (newfn);
  gtk_main_quit ();
}


static void
build_menu (GtkWidget *vbox)
{
  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *item;

  menubar = gtk_menu_bar_new();

  /********* file menu ********/

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label (_ ("File"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), item);

  item = gtk_menu_item_new_with_label (_ ("Save..."));
#if 0
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (save_dialogue), NULL);
#endif
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  item = gtk_menu_item_new_with_label (_ ("Load..."));
#if 0
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (load_dialogue), NULL);
#endif
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_label (_ ("Quit"));
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (aplvis_quit), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);


  /********* edit ********/

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label (_ ("Preferences"));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  gtk_menu_shell_append (GTK_MENU_SHELL (menubar), item);

  item = gtk_menu_item_new_with_label (_ ("Markup"));
#if 0
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (markup_dialogue), NULL);
#endif
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);


  /********* end of menus ********/

  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (menubar), FALSE, FALSE, 2);
}

static GtkWidget *
ctl_panel ()
{
  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);

  /************* granularity **********/

  GtkWidget *frame = gtk_frame_new (_ ("Granularity"));
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (frame), FALSE, FALSE, 2);
  gran_adj = gtk_adjustment_new ((gdouble)granularity,
                                 -10.0,
                                 1024,
                                 1.0,   // gdouble step_increment,
                                 5.0,   // gdouble page_increment,
                                 10.0); // gdouble page_size);
  gran_spin = gtk_spin_button_new (gran_adj, 1, 4);
  g_signal_connect (gran_spin, "value-changed",
                    G_CALLBACK (spin_changed_cb), NULL);
  gtk_container_add (GTK_CONTAINER (frame), gran_spin);

  /************* curves button **********/

  GtkWidget *curves_button = gtk_button_new_with_label (_ ("Curves"));
  g_signal_connect (curves_button, "clicked",
                    G_CALLBACK (curves_button_cb), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (curves_button),
		      FALSE, FALSE, 2);

  /************* go button **********/

  GtkWidget *go_button = gtk_button_new_with_label (_ ("Go"));
  g_signal_connect (go_button, "clicked",
                    G_CALLBACK (go_button_cb), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (go_button), FALSE, FALSE, 2);


  return vbox;
}

static void
monitor_changed (GFileMonitor      *monitor,
                 GFile             *file,
                 GFile             *other_file,
                 GFileMonitorEvent  event_type,
                 gpointer           user_data)
{
  static off_t offset = 0;
  g_signal_handler_block (monitor_file, monitor_sigid);
  struct stat statbuf;
  fstat (fileno (newout), &statbuf);
  off_t size = statbuf.st_size;
  if (size > offset) {
    gchar *bfr = g_malloc0 (16 + (size_t)(size-offset));
    int fd = open (newfn, O_RDONLY);
    lseek (fd, offset, SEEK_SET);
    read (fd, bfr, (size_t)size);
    char *p = bfr;
    for (; *p; p++) if (*p == '\n' || *p == '^') *p = ' ';
    char *msg = g_strdup_printf ("APL error: %s\n", bfr);
    gtk_label_set_text (GTK_LABEL (status), msg);
    while (*--p == ' ') *p = 0;
    g_free (bfr);
    g_free (msg);
    close (fd);
    offset = size;
  }

  g_signal_handler_unblock (monitor_file, monitor_sigid);
}


static void
sigint_handler (int sig, siginfo_t *si, void *data)
{
  unlink (newfn);
  exit (1);
  // fixme -- detect term and die
}

int
main (int ac, char *av[])
{
  struct sigaction action;
  action.sa_sigaction = sigint_handler;
  sigemptyset (&action.sa_mask);
  sigaction (SIGINT, &action, NULL);
  sigaction (SIGQUIT, &action, NULL);
  sigaction (SIGTERM, &action, NULL);

  asprintf (&newfn, "/tmp/aplvis-%d-%d.log", (int)getuid (), (int)getpid ());
  newfd = memfd_create (newfn, 0);
  newout = freopen(newfn, "a+", stdout);
  stdout = newout;

  GFile *gf = g_file_new_for_path (newfn);
  monitor_file = g_file_monitor_file (gf,
                             G_FILE_MONITOR_NONE, // GFileMonitorFlags flags,
                             NULL,              // GCancellable *cancellable,
                             NULL);             // GError **error);
  g_file_monitor_set_rate_limit (monitor_file, 200);
  monitor_sigid = g_signal_connect (G_OBJECT(monitor_file), "changed",
				    G_CALLBACK (monitor_changed), NULL);

  GOptionEntry entries[] =
    {
#if 0
     { "setvar", 'v', 0, G_OPTION_ARG_STRING_ARRAY,
       &vars, "Set variable.", NULL },
#endif
     { NULL }
  };

  GOptionContext *context = g_option_context_new ("string string string...");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  init_libapl ("apl", 0);

  gtk_init (&ac, &av);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#if 0
  g_signal_connect (window, "key-press-event",
                    G_CALLBACK (da_key_cb), NULL);
#endif
  g_signal_connect (window, "destroy", G_CALLBACK (aplvis_quit), NULL);
  gtk_window_set_title (GTK_WINDOW (window), _ ("Visualiser"));

  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  build_menu (vbox);



  /******** center grid ********/

  GtkWidget *grid = gtk_grid_new ();
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (grid), TRUE, TRUE, 2);
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), FALSE);
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), FALSE);


  /************* title *************/

  gint row = 0;
  gint col = 0;

  title = gtk_entry_new ();
#if 0
  g_signal_connect (title, "activate",
                    G_CALLBACK (expression_activate_cb), NULL);
#endif
  gtk_grid_attach (GTK_GRID (grid), title, col, row, 1, 1);
  gtk_entry_set_placeholder_text (GTK_ENTRY (title),  _ ("Title"));

  col ++;

  gtk_grid_attach (GTK_GRID (grid), ctl_panel (), 2, 0, 1, 3);

  /***** drawing area ****/

  row = 1;
  col = 0;

  da = gtk_drawing_area_new ();
  gtk_widget_set_size_request (da, width, height);
  g_signal_connect (da, "draw",
                    G_CALLBACK (da_draw_cb), NULL);
  gtk_widget_set_hexpand (da, TRUE);
  gtk_widget_set_vexpand (da, TRUE);
  gtk_grid_attach (GTK_GRID (grid), da, col, row, 1, 1);

  /********** status bar ******/

  row = 2;
  col = 0;

  status = gtk_label_new ("status");
  gtk_grid_attach (GTK_GRID (grid), status, col++, row, 1, 1);

  /******* end grid ******/
  
  if (ac > 1) {
#if 0
    load_file (av[1]);
    expression_activate_cb (NULL, NULL);
#endif
  }

  gtk_widget_show_all (window);
  gtk_main ();

}
