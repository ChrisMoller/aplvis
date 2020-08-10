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

#include "aplvis.h"

static GtkListStore *curves_store = NULL;
static GtkWidget *curves_view = NULL;

enum
  {
   INDEPENDENT_X_RADIO_COLUMN,
   INDEPENDENT_Z_RADIO_COLUMN,
   LABEL_COLUMN,
   EXPRESSION_COLUMN,
   N_COLUMNS
  };

enum
  {
   CURVES_RESPONSE_ADD = 1
  };

typedef union {
  unsigned int ui;
  struct {
    short row_selected;
    short column_nr;
  } parts_s;
} radio_u;
   
static void
add_curve ()
{
   GtkWidget *dialogue
    = gtk_dialog_new_with_buttons (_ ("Add curve"),
				   GTK_WINDOW (window),
				   GTK_DIALOG_MODAL
				   | GTK_DIALOG_DESTROY_WITH_PARENT,
				   "_OK", GTK_RESPONSE_ACCEPT,
				   "_Cancel", GTK_RESPONSE_CANCEL,
				   NULL);
   gtk_window_set_position (GTK_WINDOW (dialogue), GTK_WIN_POS_MOUSE);
   gtk_dialog_set_default_response (GTK_DIALOG (dialogue),
				    GTK_RESPONSE_ACCEPT);
  GtkWidget *vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialogue));

  GtkWidget *elbl =  gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (elbl),  _ ("Curve Label"));
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (elbl), FALSE, FALSE, 4);

  GtkWidget *expr =  gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (expr),  _ ("APL Expression"));
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (expr), FALSE, FALSE, 4);
  gtk_widget_show_all (dialogue);

  gint response = gtk_dialog_run (GTK_DIALOG (dialogue));
  if (response == GTK_RESPONSE_ACCEPT) {
    GtkTreeIter   iter;
    gtk_list_store_append (curves_store, &iter);
    gtk_list_store_set (curves_store, &iter,
			INDEPENDENT_X_RADIO_COLUMN, FALSE,
			INDEPENDENT_Z_RADIO_COLUMN, FALSE,
			LABEL_COLUMN,
			gtk_entry_get_text (GTK_ENTRY (elbl)),
			EXPRESSION_COLUMN,
			gtk_entry_get_text (GTK_ENTRY (expr)),
			-1);
  }
  gtk_widget_destroy (dialogue);
}

static gboolean
curves_store_radio_clear_foreach (GtkTreeModel *model,
				  GtkTreePath *path,
				  GtkTreeIter *iter,
				  gpointer data)
{
  gint which_col = GPOINTER_TO_INT (data);
  gtk_list_store_set (GTK_LIST_STORE (model), iter,
		      which_col, FALSE, -1);	      
		      
  return FALSE;
}

static gboolean
curves_store_radio_foreach (GtkTreeModel *model,
                            GtkTreePath *path,
                            GtkTreeIter *iter,
                            gpointer data)
{
  radio_u radio_spec;
  radio_spec.ui = GPOINTER_TO_INT (data);
  
  gint *indices = gtk_tree_path_get_indices (path);
  
  gboolean val =
    (*indices == (gint)radio_spec.parts_s.row_selected) ? TRUE : FALSE;
  gtk_list_store_set (GTK_LIST_STORE (model), iter,
		      (gint)radio_spec.parts_s.column_nr, val,
		      -1);	      
		      
  return FALSE;
}

static void
radio_toggle (GtkCellRendererToggle *cell_renderer,
	      gchar                 *path,
	      gpointer               user_data)
{
  GtkTreeIter  iter;
  gint which_col = GPOINTER_TO_INT (user_data);
  gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (curves_store),
                                       &iter, path);
  
  gint *indices = gtk_tree_path_get_indices (gtk_tree_path_new_from_string (path));

  radio_u radio_spec = {.parts_s.row_selected = *indices,
			.parts_s.column_nr = which_col};

  gtk_tree_model_foreach (GTK_TREE_MODEL (curves_store),
			  curves_store_radio_foreach,
			  GINT_TO_POINTER (radio_spec.ui));
}

static void
radio_clicked (GtkTreeViewColumn *treeviewcolumn,
               gpointer           user_data)
{
  gtk_tree_model_foreach (GTK_TREE_MODEL (curves_store),
			  curves_store_radio_clear_foreach,
			  user_data);
}

void
curves_screen ()
{
  if (!curves_store) {
    curves_store
      = gtk_list_store_new (N_COLUMNS,
			    G_TYPE_BOOLEAN,
			    G_TYPE_BOOLEAN,
			    G_TYPE_STRING,
			    G_TYPE_STRING);
    
    /***** dummy data ******/
    GtkTreeIter   iter;
    gtk_list_store_append (curves_store, &iter);
    gtk_list_store_set (curves_store, &iter,
			INDEPENDENT_X_RADIO_COLUMN, FALSE,
			INDEPENDENT_Z_RADIO_COLUMN, FALSE,
			LABEL_COLUMN, "label 1",
			EXPRESSION_COLUMN, "expression 1",
			-1);

    gtk_list_store_append (curves_store, &iter);
    gtk_list_store_set (curves_store, &iter,
			INDEPENDENT_X_RADIO_COLUMN, FALSE,
			INDEPENDENT_Z_RADIO_COLUMN, FALSE,
			LABEL_COLUMN, "label 2",
			EXPRESSION_COLUMN, "expression 2",
			-1);

    gtk_list_store_append (curves_store, &iter);
    gtk_list_store_set (curves_store, &iter,
			INDEPENDENT_X_RADIO_COLUMN, FALSE,
			INDEPENDENT_Z_RADIO_COLUMN, FALSE,
			LABEL_COLUMN, "label 3",
			EXPRESSION_COLUMN, "expression 3",
			-1);
  
    /**** end dummy data ******/
  }

  curves_view
    = gtk_tree_view_new_with_model (GTK_TREE_MODEL (curves_store));

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;


  /****** x radio *******/

  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (G_OBJECT (renderer), "toggled",
		    G_CALLBACK (radio_toggle),
		    GINT_TO_POINTER (INDEPENDENT_X_RADIO_COLUMN));
  gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer),
					TRUE);
  column =
    gtk_tree_view_column_new_with_attributes (_ ("X axis"),
					      renderer,
					      "active",
					      INDEPENDENT_X_RADIO_COLUMN,
					      NULL);
  gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);
  g_signal_connect (G_OBJECT (column), "clicked",
		    G_CALLBACK (radio_clicked),
		    GINT_TO_POINTER (INDEPENDENT_X_RADIO_COLUMN));
  gtk_tree_view_append_column (GTK_TREE_VIEW (curves_view), column);
    

  /****** z radio *******/


  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (G_OBJECT (renderer), "toggled",
		    G_CALLBACK (radio_toggle),
		    GINT_TO_POINTER (INDEPENDENT_Z_RADIO_COLUMN));
  gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer),
				      TRUE);
  column =
    gtk_tree_view_column_new_with_attributes (_ ("Z axis"),
					      renderer,
					      "active",
					      INDEPENDENT_Z_RADIO_COLUMN,
					      NULL);
  gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), TRUE);
  g_signal_connect (G_OBJECT (column), "clicked",
		    G_CALLBACK (radio_clicked),
		    GINT_TO_POINTER (INDEPENDENT_Z_RADIO_COLUMN));
  gtk_tree_view_append_column (GTK_TREE_VIEW (curves_view), column);

    /****** label *******/
    
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (_("Label"),
					      renderer,
					      "text", LABEL_COLUMN,
					      NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (curves_view), column);

  /****** expression *******/

  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (_("Expression"),
					      renderer,
					      "text", EXPRESSION_COLUMN,
					      NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (curves_view), column);
  
  GtkWidget *dialogue
    = gtk_dialog_new_with_buttons (_ ("Expressions"),
				   GTK_WINDOW (window),
				   GTK_DIALOG_MODAL
				   | GTK_DIALOG_DESTROY_WITH_PARENT,
				   "_Add",   CURVES_RESPONSE_ADD,
				   "_Close", GTK_RESPONSE_CLOSE,
				   NULL);
  gtk_widget_set_size_request (dialogue, 400, 200);
  gtk_window_set_position (GTK_WINDOW (dialogue), GTK_WIN_POS_MOUSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialogue),
                                   GTK_RESPONSE_CANCEL);
  GtkWidget *vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialogue));
  
  GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (scroll), TRUE, TRUE, 4);

  gtk_container_add (GTK_CONTAINER (scroll), curves_view);


  gtk_widget_show_all (dialogue);

  gboolean run = TRUE;

  while (run) {
    gint response = gtk_dialog_run (GTK_DIALOG (dialogue));

    if (response == CURVES_RESPONSE_ADD) add_curve ();
    else run = FALSE;
  }
  gtk_widget_destroy (dialogue);
}
