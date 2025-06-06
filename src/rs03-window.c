/*  dvdisaster: Additional error correction for optical media.
 *  Copyright (C) 2004-2017 Carsten Gnoerlich.
 *  Copyright (C) 2019-2021 The dvdisaster development team.
 *
 *  Email: support@dvdisaster.org
 *
 *  This file is part of dvdisaster.
 *
 *  dvdisaster is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  dvdisaster is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with dvdisaster. If not, see <http://www.gnu.org/licenses/>.
 */

/*** src type: only GUI code ***/

#ifdef WITH_GUI_YES
#include "dvdisaster.h"

#include "rs03-includes.h"

/***
 *** Forward declarations
 ***/

static void redraw_curve(cairo_t *cr, RS03Widgets*);
static void update_geometry(RS03Widgets*);

/***
 *** Encoding window
 ***/

void ResetRS03EncWindow(Method *method)
{  RS03Widgets *wl = (RS03Widgets*)method->widgetList;

   GuiSetProgress(wl->encPBar1, 0, 100);
   GuiSetProgress(wl->encPBar2, 0, 100);

   gtk_widget_hide(wl->encLabel2);
   gtk_widget_hide(wl->encPBar2);

   gtk_widget_hide(wl->encLabel3);
   gtk_widget_hide(wl->encLabel4);
   gtk_widget_hide(wl->encLabel5);
   gtk_widget_hide(wl->encThreads);
   gtk_widget_hide(wl->encPerformance);
   gtk_widget_hide(wl->encBottleneck);

   gtk_label_set_text(GTK_LABEL(wl->encFootline), "");
   gtk_label_set_text(GTK_LABEL(wl->encFootline2), "");
}

void CreateRS03EncWindow(Method *method, GtkWidget *parent)
{  GtkWidget *wid,*grid,*pbar,*sep;
   RS03Widgets *wl=method->widgetList;

   wl->encHeadline = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wl->encHeadline), 0.0);
   gtk_widget_set_margin_start(wl->encHeadline, 5);
   gtk_box_pack_start(GTK_BOX(parent), wl->encHeadline, FALSE, FALSE, 3);

   sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
   gtk_box_pack_start(GTK_BOX(parent), sep, FALSE, FALSE, 0);

   sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
   gtk_box_pack_start(GTK_BOX(parent), sep, FALSE, FALSE, 0);

   grid = gtk_grid_new();
   gtk_widget_set_margin_start(grid, 20);
   gtk_widget_set_margin_end(grid, 20);
   gtk_widget_set_margin_top(grid, 5);
   gtk_widget_set_margin_bottom(grid, 5);
   gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
   gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
   gtk_box_pack_start(GTK_BOX(parent), grid, FALSE, FALSE, 30);

   wl->encLabel1 = wid = gtk_label_new(NULL);
   gtk_label_set_markup(GTK_LABEL(wid),
			_utf("<b>1. Reserving space:</b>"));
   gtk_label_set_xalign(GTK_LABEL(wid), 0.0);
   gtk_widget_set_margin_top(wid, 15);
   gtk_widget_set_margin_bottom(wid, 15);
   gtk_grid_attach(GTK_GRID(grid), wid, 1, 1, 1, 1);

   pbar = wl->encPBar1 = gtk_progress_bar_new();
   gtk_widget_set_valign(pbar, GTK_ALIGN_CENTER);
   gtk_widget_set_hexpand(pbar, TRUE);
   gtk_grid_attach(GTK_GRID(grid), pbar, 2, 1, 1, 1);

   wl->encLabel2 = wid = gtk_label_new(NULL);
   gtk_label_set_markup(GTK_LABEL(wid),
			_utf("<b>2. Creating error correction data:</b>"));
   gtk_label_set_xalign(GTK_LABEL(wid), 0.0);
   gtk_widget_set_margin_top(wid, 15);
   gtk_widget_set_margin_bottom(wid, 15);
   gtk_grid_attach(GTK_GRID(grid), wid, 1, 2, 1, 1);

   pbar = wl->encPBar2 = gtk_progress_bar_new();
   gtk_widget_set_valign(pbar, GTK_ALIGN_CENTER);
   gtk_widget_set_hexpand(pbar, TRUE);
   gtk_grid_attach(GTK_GRID(grid), pbar, 2, 2, 1, 1);


   wl->encLabel3 = wid = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wid), 1.0);
   gtk_label_set_markup(GTK_LABEL(wid),_utf("<b>Encoder info:</b>"));
   gtk_grid_attach(GTK_GRID(grid), wid, 1, 3, 1, 1);
   
   wl->encThreads = wid = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wid), 0.0);
   gtk_grid_attach(GTK_GRID(grid), wid, 2, 3, 1, 1);

   wl->encLabel4 = wid = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wid), 1.0);
   gtk_label_set_markup(GTK_LABEL(wid),_utf("<b>Performance:</b>"));
   gtk_grid_attach(GTK_GRID(grid), wid, 1, 4, 1, 1);

   wl->encPerformance = wid = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wid), 0.0);
   gtk_grid_attach(GTK_GRID(grid), wid, 2, 4, 1, 1);

   wl->encLabel5 = wid = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wid), 1.0);
   gtk_label_set_markup(GTK_LABEL(wid),_utf("<b>State:</b>"));
   gtk_grid_attach(GTK_GRID(grid), wid, 1, 5, 1, 1);

   wl->encBottleneck = wid = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wid), 0.0);
   gtk_grid_attach(GTK_GRID(grid), wid, 2, 5, 1, 1);

   wl->encFootline = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wl->encFootline), 0.0);
   gtk_widget_set_margin_start(wl->encFootline, 20);
   gtk_box_pack_start(GTK_BOX(parent), wl->encFootline, FALSE, FALSE, 3);

   wl->encFootline2 = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wl->encFootline2), 0.0);
   gtk_widget_set_margin_start(wl->encFootline2, 20);
   gtk_box_pack_start(GTK_BOX(parent), wl->encFootline2, FALSE, FALSE, 3);
}

/***
 *** Fix window
 ***/

/*
 * Set the media size and ecc capacity
 */

static gboolean set_max_idle_func(gpointer data)
{  RS03Widgets *wl = (RS03Widgets*)data;

   gtk_widget_queue_draw(wl->fixCurve->widget);

   return FALSE;
}

void RS03SetFixMaxValues(RS03Widgets *wl, int data_bytes, int ecc_bytes, gint64 sectors)
{
   wl->dataBytes = data_bytes;
   wl->eccBytes  = ecc_bytes;
   wl->nSectors  = sectors;
   wl->fixCurve->maxX = 100;
   wl->fixCurve->maxY = ecc_bytes - (ecc_bytes % 5) + 5;

   g_idle_add(set_max_idle_func, wl);
}

/*
 * Update the corrected / uncorrected numbers
 */

static gboolean results_idle_func(gpointer data)
{  RS03Widgets *wl = (RS03Widgets*)data;

   GuiSetLabelText(wl->fixCorrected, _("Repaired: %" PRId64 ""), wl->corrected); 
   GuiSetLabelText(wl->fixUncorrected, _("Unrepairable: <span %s>%" PRId64 "</span>"),Closure->redMarkup, wl->uncorrected); 
   GuiSetLabelText(wl->fixProgress, _("Progress: %3d.%1d%%"), wl->percent/10, wl->percent%10);

   return FALSE;
}

void RS03UpdateFixResults(RS03Widgets *wl, gint64 corrected, gint64 uncorrected)
{
   wl->corrected = corrected;
   wl->uncorrected = uncorrected;

   g_idle_add(results_idle_func, wl);
}

/*
 * Update the error curve 
 */

static gboolean curve_idle_func(gpointer data)
{  RS03Widgets *wl = (RS03Widgets*)data;
   gtk_widget_queue_draw(wl->fixCurve->widget);

   return FALSE;
}

/* 
 * Add one new data point 
 */

void RS03AddFixValues(RS03Widgets *wl, int percent, int ecc_max)
{
   if(percent < 0 || percent > 1000)
     return;

   wl->fixCurve->ivalue[percent] = ecc_max;
   wl->percent = percent;
   g_idle_add(curve_idle_func, wl);
}
  
/*
 * Redraw the whole curve
 */

/* Calculate the geometry of the curve and spiral */

static void update_geometry(RS03Widgets *wl)
{  
   /* Curve geometry */ 

   GuiUpdateCurveGeometry(wl->fixCurve, "999", 20);

   /* Label positions in the foot line */

   gtk_box_set_child_packing(GTK_BOX(wl->fixFootlineBox), wl->fixCorrected,
			     TRUE, TRUE, wl->fixCurve->leftX, GTK_PACK_START);
   gtk_box_set_child_packing(GTK_BOX(wl->fixFootlineBox), wl->fixUncorrected, 
			     TRUE, TRUE, wl->fixCurve->leftX, GTK_PACK_START);
}

static void redraw_curve(cairo_t *cr, RS03Widgets *wl)
{  int y;

   /* Redraw the curve */

   GuiRedrawAxes(cr, wl->fixCurve);
   GuiRedrawCurve(cr, wl->fixCurve, wl->percent);

   /* Ecc capacity threshold line */

   y = GuiCurveY(wl->fixCurve, wl->eccBytes);  

   gdk_cairo_set_source_rgba(cr, Closure->greenSector);
   cairo_set_line_width(cr, 1.0);
   cairo_move_to(cr, wl->fixCurve->leftX-5.5, y+0.5);
   cairo_line_to(cr, wl->fixCurve->rightX+5.5, y+0.5);
   cairo_stroke(cr);
}

/*
 * Draw callback
 */

static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{  RS03Widgets *wl = (RS03Widgets*)data;

   update_geometry(wl);
   redraw_curve(cr, wl);

   return TRUE;
}

void ResetRS03FixWindow(Method *method)
{  RS03Widgets *wl = (RS03Widgets*)method->widgetList;

   gtk_notebook_set_current_page(GTK_NOTEBOOK(wl->fixNotebook), 0);

   GuiZeroCurve(wl->fixCurve);
   RS03UpdateFixResults(wl, 0, 0);

   if(wl->fixCurve && wl->fixCurve->widget)
      gtk_widget_queue_draw(wl->fixCurve->widget);

   wl->percent = 0;
   wl->lastPercent = 0;
}

/*
 * Create the Fix window contents
 */


void CreateRS03FixWindow(Method *method, GtkWidget *parent)
{  RS03Widgets *wl;
   GtkWidget *sep,*ignore,*d_area,*notebook,*hbox;

   if(!method->widgetList)
   {  wl = g_malloc0(sizeof(RS03Widgets));
      method->widgetList = wl;
   }
   else wl = method->widgetList;

   wl->fixHeadline = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wl->fixHeadline), 0.0);
   gtk_widget_set_margin_start(wl->fixHeadline, 5);
   gtk_box_pack_start(GTK_BOX(parent), wl->fixHeadline, FALSE, FALSE, 3);

   sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
   gtk_box_pack_start(GTK_BOX(parent), sep, FALSE, FALSE, 0);

   sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
   gtk_box_pack_start(GTK_BOX(parent), sep, FALSE, FALSE, 0);

   d_area = wl->fixDrawingArea = gtk_drawing_area_new();
   gtk_box_pack_start(GTK_BOX(parent), d_area, TRUE, TRUE, 0);
   g_signal_connect(G_OBJECT (d_area), "draw", G_CALLBACK(draw_cb), (gpointer)wl);
   
   notebook = wl->fixNotebook = gtk_notebook_new();
   gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
   gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
   gtk_box_pack_end(GTK_BOX(parent), notebook, FALSE, FALSE, 0);

   hbox = wl->fixFootlineBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
   gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);

   wl->fixCorrected = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wl->fixCorrected), 0.0);
   gtk_box_pack_start(GTK_BOX(hbox), wl->fixCorrected, TRUE, TRUE, 0);

   wl->fixProgress = gtk_label_new(NULL);
   gtk_box_pack_start(GTK_BOX(hbox), wl->fixProgress, TRUE, TRUE, 0);

   wl->fixUncorrected = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wl->fixUncorrected), 1.0);
   gtk_box_pack_start(GTK_BOX(hbox), wl->fixUncorrected, TRUE, TRUE, 0);

   ignore = gtk_label_new("progress_tab");
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook), hbox, ignore);

   wl->fixFootline = gtk_label_new("Footline");
   gtk_label_set_xalign(GTK_LABEL(wl->fixFootline), 0.0);
   gtk_widget_set_margin_start(wl->fixFootline, 5);
   ignore = gtk_label_new("footer_tab");
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook), wl->fixFootline, ignore);

   wl->fixCurve  = GuiCreateCurve(d_area, _("Errors/Ecc block"), "%d", 1000, CURVE_PERCENT);
   wl->fixCurve->enable = DRAW_ICURVE;
}

#endif /* WITH_GUI_YES */
