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

/*** src type: some GUI code ***/

#include "dvdisaster.h"

#include "rs02-includes.h"

#ifdef WITH_GUI_YES

/***
 *** Reset the verify output window
 ***/

void ResetRS02VerifyWindow(Method *self)
{  RS02Widgets *wl = (RS02Widgets*)self->widgetList;

   GuiSetLabelText(wl->cmpImageSectors, "");
   GuiSetLabelText(wl->cmpImageMd5Sum, "");
   GuiSetLabelText(wl->cmpEccHeaders, "");
   GuiSetLabelText(wl->cmpDataSection, "");
   GuiSetLabelText(wl->cmpCrcSection, "");
   GuiSetLabelText(wl->cmpEccSection, "");
   GuiSetLabelText(wl->cmpImageResult, "");

   GuiSetLabelText(wl->cmpEccCreatedBy, "dvdisaster");
   GuiSetLabelText(wl->cmpEccMethod, "");
   GuiSetLabelText(wl->cmpEccRequires, "");
   GuiSetLabelText(wl->cmpEccMediumSectors, "");
   GuiSetLabelText(wl->cmpEcc1Name, _("Data checksum:"));
   GuiSetLabelText(wl->cmpEcc2Name, _("CRC checksum:"));
   GuiSetLabelText(wl->cmpEcc3Name, _("Ecc checksum:"));
   GuiSetLabelText(wl->cmpEcc1Msg, "");
   GuiSetLabelText(wl->cmpEcc2Msg, "");
   GuiSetLabelText(wl->cmpEcc3Msg, "");

   GuiSwitchAndSetFootline(wl->cmpEccNotebook, 0, NULL, NULL);

   wl->lastPercent = 0;

   GuiFillSpiral(wl->cmpSpiral, &transparent);
   if (wl->cmpSpiral->widget)
      gtk_widget_queue_draw(wl->cmpSpiral->widget);
}

/***
 *** Manage the image spiral
 ***/

/*
 * Update part of the spiral
 */

typedef struct _spiral_idle_info
{  Spiral *cmpSpiral;
   GdkRGBA *segColor;
   int from, to;
} spiral_idle_info;

static gboolean spiral_idle_func(gpointer data)
{  spiral_idle_info *sii = (spiral_idle_info*)data;
   int i;

   for(i=sii->from; i<=sii->to; i++)
     GuiSetSpiralSegmentColor(sii->cmpSpiral, sii->segColor, 0, i-1);

   g_free(sii);
   return FALSE;
}

static void add_verify_values(Method *method, int percent, 
			       gint64 newMissing, gint64 newCrcErrors)
{
   if(percent < 0 || percent > VERIFY_IMAGE_SEGMENTS)
     return;

   RS02Widgets *wl = (RS02Widgets*)method->widgetList;
   spiral_idle_info *sii = g_malloc(sizeof(spiral_idle_info));

   sii->cmpSpiral = wl->cmpSpiral;

   sii->segColor = Closure->greenSector;
   if(newCrcErrors) sii->segColor = Closure->yellowSector;
   if(newMissing) sii->segColor = Closure->redSector;

   sii->from = wl->lastPercent+1;
   sii->to   = percent;

   wl->lastPercent = percent;

   g_idle_add(spiral_idle_func, sii);
}

/*
 * Redraw whole spiral
 */

static void redraw_spiral(cairo_t *cr, RS02Widgets *wl)
{  int x = wl->cmpSpiral->mx - wl->cmpSpiral->diameter/2 + 10;

   GuiDrawSpiralLabel(cr, wl->cmpSpiral, wl->cmpLayout,
		      _("Good sectors"), Closure->greenSector, x, 1);

   GuiDrawSpiralLabel(cr, wl->cmpSpiral, wl->cmpLayout,
		      _("Sectors with CRC errors"), Closure->yellowSector, x, 2);

   GuiDrawSpiralLabel(cr, wl->cmpSpiral, wl->cmpLayout,
		      _("Missing sectors"), Closure->redSector, x, 3);

   GuiDrawSpiral(cr, wl->cmpSpiral);
}

/*
 * draw event handler for the spiral
 */

static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{  RS02Widgets *wl = (RS02Widgets*)data;
   GtkAllocation a = {0};
   gtk_widget_get_allocation(widget, &a);
   int w,h,size;

   /* Finish spiral initialization */

   if(!wl->cmpLayout)
   {  GuiSetSpiralWidget(wl->cmpSpiral, widget);
      wl->cmpLayout = gtk_widget_create_pango_layout(widget, NULL);
   }

   GuiSetText(wl->cmpLayout, _("Missing sectors"), &w, &h);
   size = wl->cmpSpiral->diameter + 20 + 3*(10+h);  /* approx. size of spiral + labels */

   wl->cmpSpiral->mx = a.width / 2;
   wl->cmpSpiral->my = (wl->cmpSpiral->diameter + a.height - size)/2;

   redraw_spiral(cr, wl);   /* Redraw the spiral */

   return TRUE;
}

/***
 *** Create the notebook contents for the verify output
 ***/

void CreateRS02VerifyWindow(Method *self, GtkWidget *parent)
{  RS02Widgets *wl = (RS02Widgets*)self->widgetList;
   GtkWidget *sep,*notebook,*grid,*grid2,*ignore,*lab,*frame,*d_area;

   wl->cmpHeadline = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(wl->cmpHeadline), 0.0);
   gtk_widget_set_margin_start(wl->cmpHeadline, 5);
   gtk_box_pack_start(GTK_BOX(parent), wl->cmpHeadline, FALSE, FALSE, 3);

   sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
   gtk_box_pack_start(GTK_BOX(parent), sep, FALSE, FALSE, 0);

   sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
   gtk_box_pack_start(GTK_BOX(parent), sep, FALSE, FALSE, 0);

   grid = gtk_grid_new();
   gtk_widget_set_margin_start(grid, 5);
   gtk_widget_set_margin_end(grid, 5);
   gtk_widget_set_margin_top(grid, 5);
   gtk_widget_set_margin_bottom(grid, 5);
   gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
   gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
   gtk_container_set_border_width(GTK_CONTAINER(grid), 5);
   gtk_box_pack_start(GTK_BOX(parent), grid, TRUE, TRUE, 0);

   /*** Image info */

   frame = gtk_frame_new(_utf("Image file summary"));
   gtk_widget_set_hexpand(frame, TRUE);
   gtk_grid_attach(GTK_GRID(grid), frame, 1, 1, 1, 1);

   grid2 = gtk_grid_new();
   gtk_widget_set_margin_start(grid2, 5);
   gtk_widget_set_margin_end(grid2, 5);
   gtk_widget_set_margin_top(grid2, 2);
   gtk_widget_set_margin_bottom(grid2, 2);
   gtk_grid_set_column_spacing(GTK_GRID(grid2), 5);
   gtk_grid_set_row_spacing(GTK_GRID(grid2), 4);
   gtk_container_set_border_width(GTK_CONTAINER(grid2), 5);
   gtk_container_add(GTK_CONTAINER(frame), grid2);

   lab = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Medium sectors:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 1, 1, 1);
   lab = wl->cmpImageSectors = gtk_label_new("0");
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 1, 1, 1);

   lab = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Data checksum:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 2, 1, 1);
   lab = wl->cmpImageMd5Sum = gtk_label_new("0");
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 2, 1, 1);

   lab = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Ecc headers:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 3, 1, 1);
   lab = wl->cmpEccHeaders = gtk_label_new(".");
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 3, 1, 1);

   lab = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Data section:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 4, 1, 1);
   lab = wl->cmpDataSection = gtk_label_new(".");
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 4, 1, 1);

   lab = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Crc section:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 5, 1, 1);
   lab = wl->cmpCrcSection = gtk_label_new(".");
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 6, 1, 1);

   lab = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Ecc section:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 6, 1, 1);
   lab = wl->cmpEccSection= gtk_label_new(".");
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 6, 1, 1);

   lab = wl->cmpImageResult = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_widget_set_margin_top(lab, 2);
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 5, 2, 1);

   /*** Image spiral */

   frame = gtk_frame_new(_utf("Image state"));
   gtk_widget_set_vexpand(frame, TRUE);
   gtk_grid_attach(GTK_GRID(grid), frame, 2, 1, 1, 2);

   wl->cmpSpiral = GuiCreateSpiral(&transparent, 10, 5, VERIFY_IMAGE_SEGMENTS);
   d_area = wl->cmpDrawingArea = gtk_drawing_area_new();
   gtk_widget_set_size_request(d_area, wl->cmpSpiral->diameter+20, -1);
   gtk_container_add(GTK_CONTAINER(frame), d_area);
   g_signal_connect(G_OBJECT(d_area), "draw", G_CALLBACK(draw_cb), (gpointer)wl);

   /*** Ecc data info */

   frame = gtk_frame_new(_utf("Error correction data"));
   gtk_widget_set_hexpand(frame, TRUE);
   gtk_widget_set_vexpand(frame, TRUE);
   gtk_grid_attach(GTK_GRID(grid), frame, 1, 2, 1, 1);

   notebook = wl->cmpEccNotebook = gtk_notebook_new();
   gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
   gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
   gtk_container_add(GTK_CONTAINER(frame), notebook);

   ignore = gtk_label_new(NULL);
   lab = gtk_label_new("");
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook), lab, ignore);

   grid2 = gtk_grid_new();
   gtk_widget_set_margin_start(grid2, 5);
   gtk_widget_set_margin_end(grid2, 5);
   gtk_widget_set_margin_top(grid2, 2);
   gtk_widget_set_margin_bottom(grid2, 2);
   gtk_grid_set_column_spacing(GTK_GRID(grid2), 5);
   gtk_grid_set_row_spacing(GTK_GRID(grid2), 4);
   ignore = gtk_label_new("ecc info");
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid2, ignore);
   gtk_container_set_border_width(GTK_CONTAINER(grid2), 5);

   lab = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Created by:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 1, 1, 1);
   lab = wl->cmpEccCreatedBy = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 1, 1, 1);

   lab = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Method:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 2, 1, 1);
   lab = wl->cmpEccMethod = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 2, 1, 1);

   lab = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Requires:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 3, 1, 1);
   lab = wl->cmpEccRequires = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 3, 1, 1);

   lab = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Medium sectors:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 4, 1, 1);
   lab = wl->cmpEccMediumSectors = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 4, 1, 1);

   lab = wl->cmpEcc1Name = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Data checksum:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 5, 1, 1);
   lab = wl->cmpEcc1Msg = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 5, 1, 1);

   lab = wl->cmpEcc2Name = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("CRC checksum:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 6, 1, 1);
   lab = wl->cmpEcc2Msg = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 6, 1, 1);

   lab = wl->cmpEcc3Name = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   GuiSetLabelText(lab, _("Ecc checksum:"));
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 7, 1, 1);
   lab = wl->cmpEcc3Msg = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_grid_attach(GTK_GRID(grid2), lab, 2, 7, 1, 1);

   lab = wl->cmpEccResult = gtk_label_new(NULL);
   gtk_label_set_xalign(GTK_LABEL(lab), 0.0);
   gtk_widget_set_margin_top(lab, 2);
   gtk_grid_attach(GTK_GRID(grid2), lab, 1, 8, 2, 1);
}
#endif /* WITH_GUI_YES */

/***
 *** Check the consistency of the augmented image
 ***/

/* 
 * housekeeping
 */

typedef struct
{  Image *image;
   EccHeader *eh;
   RS02Layout *lay;
   RS02Widgets *wl;
   Bitmap *map;
   guint32 *crcBuf;
   gint8   *crcValid;
   unsigned char crcSum[16];
} verify_closure;

static void cleanup(gpointer data)
{  verify_closure *cc = (verify_closure*)data;

   UnregisterCleanup();

   GuiAllowActions(TRUE);

   if(cc->image) CloseImage(cc->image);
   if(cc->lay) g_free(cc->lay);
   if(cc->map) FreeBitmap(cc->map);
   if(cc->crcBuf) g_free(cc->crcBuf);
   if(cc->crcValid) g_free(cc->crcValid);
   
   g_free(cc);

   GuiExitWorkerThread();
}

/***
 *** Read the crc portion and descramble it from ecc block order
 *** into ascending sector order. 
 */

static void read_crc(verify_closure *cc, RS02Layout *lay)
{  EccHeader *eh = cc->eh;
   struct MD5Context crc_md5;
   gint64 block_idx[256];
   guint32 crc_buf[512];
   gint64 crc_sector,s;
   int i,crc_idx;
   int crc_valid = 1;
   int unrecoverable_sectors = 0;
   
   /* Allocate buffer for ascending sector order CRCs */

   cc->crcBuf   = g_malloc(2048 * lay->crcSectors);
   cc->crcValid = g_malloc(512 * lay->crcSectors);
   MD5Init(&crc_md5);

   /* First sector containing crc data */

   if(!LargeSeek(cc->image->file, 2048*(lay->dataSectors+2)))
     Stop(_("Failed seeking to sector %" PRId64 " in image: %s"), 
	  lay->dataSectors+2, strerror(errno));
   crc_sector = lay->dataSectors+2;

   /* Initialize ecc block index pointers.
      The first CRC set (of lay->ndata checksums) relates to
      ecc block lay->firstCrcLayerIndex + 1. */

   for(s=0, i=0; i<lay->ndata; s+=lay->sectorsPerLayer, i++)
     block_idx[i] = s + lay->firstCrcLayerIndex + 1;

   crc_idx = 512;  /* force crc buffer reload */

   /* Cycle through the ecc blocks and sort CRC sums in
      ascending sector numbers. */

   for(s=0; s<lay->sectorsPerLayer; s++)
   {  gint64 si = (s + lay->firstCrcLayerIndex + 1) % lay->sectorsPerLayer;

      /* Wrap the block_idx[] ptrs at si == 0 */

      if(!si)
      {  gint64 bs;

         for(bs=0, i=0; i<lay->ndata; bs+=lay->sectorsPerLayer, i++)
	   block_idx[i] = bs;
      }

      /* Go through all data sectors of current ecc block */

      for(i=0; i<lay->ndata; i++)
      {
	 if(block_idx[i] < lay->dataSectors)  /* only data sectors have CRCs */
	 {  
	    /* Refill crc cache if needed */
	    
	    if(crc_idx >= 512)
	    {  int err;
 
	       if(LargeRead(cc->image->file, crc_buf, 2048) != 2048)
		  Stop(_("problem reading crc data: %s"), strerror(errno));

	       err = CheckForMissingSector((unsigned char*)crc_buf, crc_sector, eh->mediumFP, eh->fpSector);
	       if(err != SECTOR_PRESENT)
		 ExplainMissingSector((unsigned char*)crc_buf, crc_sector, err, SOURCE_IMAGE, &unrecoverable_sectors);

	        crc_sector++;
	        crc_valid = (err == SECTOR_PRESENT);
		
	        MD5Update(&crc_md5, (unsigned char*)crc_buf, 2048);
		crc_idx = 0;
	    }

	    /* Sort crc into appropriate place */

	    cc->crcBuf[block_idx[i]]   = crc_buf[crc_idx];
       	    cc->crcValid[block_idx[i]] = crc_valid;
	    crc_idx++;
	    block_idx[i]++;
	 }
      }
   }

   MD5Final(cc->crcSum, &crc_md5);
}

/*
 * Prognosis for correctability
 */

static int prognosis(verify_closure *vc, gint64 missing, gint64 expected)
{  int j,eccblock;
   int worst_ecc = 0;
   gint64 damaged_sectors = 0, damaged_eccsecs = 0;
   gint64 correctable = 0;
   gint64 recoverable;

   for(eccblock=0; eccblock<vc->lay->sectorsPerLayer; eccblock++)
   {  int count = 255;

      for(j=0; j<255; j++)
      {  gint64 sector = RS02SectorIndex(vc->lay, j, eccblock);

	/* padding sectors are always correctable
	   and the first ecc header does not take part in error correction */

	 if(   sector < 0  
	    || sector  == vc->lay->firstEccHeader
	    || sector  == vc->lay->firstEccHeader + 1)
	 {  count--;
	    continue;  
	 }

	 if(GetBit(vc->map, sector))
	   count--;
	 else damaged_sectors++;
      }

      if(count>0)                damaged_eccsecs++;
      if(count>worst_ecc)        worst_ecc = count; 
      if(count<=vc->lay->nroots) correctable += count;
   }

   recoverable = expected - missing + correctable;

   if(damaged_sectors > 0)
   {  int percentage  = (1000*recoverable) / expected;

      PrintLog(_("- erasure counts   :  avg =  %.1f; worst = %d per ecc block.\n"),
	      (double)damaged_sectors/(double)damaged_eccsecs,worst_ecc);

      PrintLog(_("- prognosis        : %" PRId64 " of %" PRId64 " sectors recoverable (%d.%d%%)\n"),
	       recoverable, expected, percentage/10, percentage%10);

      if(Closure->guiMode)
      {  GuiSetLabelText(vc->wl->cmpEcc1Name, "");
	 GuiSetLabelText(vc->wl->cmpEcc1Msg,  "");
	 GuiSetLabelText(vc->wl->cmpEcc2Name, _("Erasure counts:"));
	 GuiSetLabelText(vc->wl->cmpEcc3Name, _("Prognosis:"));

	 GuiSetLabelText(vc->wl->cmpEcc2Msg,
			 _("<span %s>avg =  %.1f; worst = %d per ecc block.</span>"),
			 worst_ecc <= vc->lay->nroots ? Closure->greenMarkup : Closure->redMarkup,
			 (double)damaged_sectors/(double)damaged_eccsecs,worst_ecc);

	 GuiSetLabelText(vc->wl->cmpEcc3Msg,
			 _("<span %s>%" PRId64 " of %" PRId64 " sectors recoverable (%d.%d%%)</span>"),
			 recoverable < expected ? Closure->redMarkup : Closure->greenMarkup,
			 recoverable, expected, percentage/10, percentage%10);
      }
   }	      

   /* Why the first test?
      if(damaged_sectors && worst_ecc <= vc->lay->nroots && recoverable >= expected) */
   if(worst_ecc <= vc->lay->nroots && recoverable >= expected)
        return TRUE;
   else return FALSE;
}

/*
 * The verify action
 */

void RS02Verify(Image *image)
{  verify_closure *cc = g_malloc0(sizeof(verify_closure));
   Method *self = FindMethod("RS02");
   RS02Widgets *wl = self->widgetList;
   EccHeader *eh;
   RS02Layout *lay;
   struct MD5Context image_md5;
   struct MD5Context ecc_md5;
   struct MD5Context meta_md5;
   unsigned char ecc_sum[16];
   unsigned char medium_sum[16];
   char data_digest[33], hdr_digest[33], digest[33];
   gint64 s, crc_idx;
   int last_percent = 0;
   unsigned char buf[2048];
   gint64 first_missing, last_missing;
   gint64 total_missing = 0;
   gint64 data_missing = 0;
   gint64 crc_missing = 0;
   gint64 ecc_missing = 0;
   gint64 new_missing = 0;
   gint64 new_crc_errors = 0;
   gint64 data_crc_errors = 0;
   gint64 hdr_missing, hdr_crc_errors;
   gint64 hdr_ok,hdr_pos,hdr_correctable;
   gint64 ecc_sector,expected_sectors;
   int ecc_md5_failure = FALSE;
   int ecc_slice;
   int major,minor,micro;
   char *unstable="";
   char method[5];
   char *img_advice = NULL;
   char *ecc_advice = NULL;
   int try_it;
   int unrecoverable_sectors = 0;
   
   /*** Prepare for early termination */

   RegisterCleanup(_("Check aborted"), cleanup, cc);
   cc->wl = wl;

   /* extract some important information */

   eh  = cc->eh  = image->eccHeader;
   lay = cc->lay = RS02LayoutFromImage(image);

   expected_sectors = lay->eccSectors+lay->dataSectors;
   if(!eh->inLast)      /* 0.66 pre-releases did not set this */
     eh->inLast = 2048;

   cc->image = image;
   cc->map = CreateBitmap0(expected_sectors);

   /*** Print information on image size */

   GuiSetLabelText(wl->cmpHeadline, "<big>%s</big>\n<i>%s</i>",
		   _("Checking the image file."),
		   _("Image contains error correction data."));

   PrintLog("\n%s: ",Closure->imageName);
   PrintLog(_("present, contains %" PRId64 " medium sectors.\n"),image->sectorSize);

   if(Closure->guiMode)
   {  if(expected_sectors == image->sectorSize)
      {  GuiSetLabelText(wl->cmpImageSectors, "%" PRId64, image->sectorSize);
      }
      else
      {  GuiSetLabelText(wl->cmpImageSectors, "<span %s>%" PRId64 "</span>", 
			 Closure->redMarkup, image->sectorSize);
	 if(expected_sectors > image->sectorSize)
	      img_advice = g_strdup_printf(_("<span %s>Image file is %" PRId64 " sectors shorter than expected.</span>"), Closure->redMarkup, expected_sectors - image->sectorSize);
	 else img_advice = g_strdup_printf(_("<span %s>Image file is %" PRId64 " sectors longer than expected.</span>"), Closure->redMarkup, image->sectorSize - expected_sectors);
      }
   }

   /*** Check integrity of the ecc headers */

   hdr_ok = hdr_missing = hdr_crc_errors = hdr_correctable = 0;
   hdr_pos = lay->firstEccHeader;

   while(hdr_pos < expected_sectors)
   {  EccHeader eh;

      if(hdr_pos < image->sectorSize)
      {  int n;

	 if(!LargeSeek(image->file, 2048*hdr_pos))
	   Stop(_("Failed seeking to ecc header at %" PRId64 ": %s\n"), hdr_pos, strerror(errno));

	 n = LargeRead(image->file, &eh, sizeof(EccHeader));
	 if(n != sizeof(EccHeader))
	   Stop(_("Failed reading ecc header at %" PRId64 ": %s\n"), hdr_pos, strerror(errno));

	 /* Missing header blocks are always recoverable by copying information
	    from the surviving headers */

	 if(CheckForMissingSector((unsigned char*)&eh, hdr_pos, cc->eh->mediumFP, cc->eh->fpSector))
	    hdr_correctable++;
	 if(CheckForMissingSector(2048+((unsigned char*)&eh), hdr_pos+1, cc->eh->mediumFP, cc->eh->fpSector))
	    hdr_correctable++;

      }
      else memset(&eh, 0, sizeof(EccHeader));

      if(!strncmp((char*)eh.cookie, "*dvdisaster*", 12))
      {  guint32 recorded_crc = eh.selfCRC;
 	 guint32 real_crc;

#ifdef HAVE_BIG_ENDIAN
	 eh.selfCRC = 0x47504c00;
#else
	 eh.selfCRC = 0x4c5047;
#endif

         real_crc = Crc32((unsigned char*)&eh, sizeof(EccHeader));

	 if(real_crc == recorded_crc)
	 {   hdr_ok++;
	     SetBit(cc->map, hdr_pos);
	 }
	 else
	 {  hdr_crc_errors++; 
	 }
      }
      else hdr_missing++;

      if(hdr_pos == lay->firstEccHeader)
	   hdr_pos = (lay->protectedSectors + lay->headerModulo - 1) & ~(lay->headerModulo-1);
      else hdr_pos += lay->headerModulo;

      if(Closure->guiMode)
      {  if(!hdr_crc_errors && !hdr_missing)
	 {  GuiSetLabelText(wl->cmpEccHeaders, _("complete"));
	 }
         else
	 {  GuiSetLabelText(wl->cmpEccHeaders,
			    _("<span %s>%" PRId64 " ok, %" PRId64 " CRC errors, %" PRId64 " missing</span>"),
			    Closure->redMarkup, hdr_ok, hdr_crc_errors, hdr_missing);
	 }
      }
   }

   /* take shortcut in quick mode */

   if(Closure->quickVerify)
   {  PrintLog(_("* quick mode        : image NOT scanned\n"));
      goto continue_with_ecc;
   }

   /*** Read the CRC portion */ 

   read_crc(cc, lay);

   /*** Check the data portion of the image file for the
	"dead sector marker" and CRC errors */
   
   if(!LargeSeek(image->file, 0))
     Stop(_("Failed seeking to start of image: %s\n"), strerror(errno));

   MD5Init(&image_md5);
   MD5Init(&ecc_md5);
   MD5Init(&meta_md5);

   first_missing = last_missing = -1;
   crc_idx = 0;

   ecc_sector = 0;
   ecc_slice  = 0;

   for(s=0; s<expected_sectors; s++)
   {  int percent,current_missing;
      int defective = 0;

      /* Check for user interruption */

      if(Closure->stopActions)   
      {  if(Closure->stopActions == STOP_CURRENT_ACTION) /* suppress memleak warning when closing window */
	 {  GuiSetLabelText(wl->cmpImageResult, 
			    _("<span %s>Aborted by user request!</span>"),
			    Closure->redMarkup);
	 }
         goto terminate;
      }

      /* Read the next sector */

      if(s < image->sectorSize)  /* image may be truncated */
      {  int n = LargeRead(image->file, buf, 2048);
         if(n != 2048)
	    Stop(_("premature end in image (only %d bytes): %s\n"),n,strerror(errno));
      }
      else CreateMissingSector(buf, s, eh->mediumFP, eh->fpSector, "padding beyond the image");

      if(s < lay->dataSectors)
      {  if(s < lay->dataSectors - 1)
	      MD5Update(&image_md5, buf, 2048);
	 else MD5Update(&image_md5, buf, eh->inLast);
      }

      /* Look for the dead sector marker */

      current_missing = CheckForMissingSector(buf, s, eh->mediumFP, eh->fpSector);
      if(current_missing != SECTOR_PRESENT)
	ExplainMissingSector(buf, s, current_missing, SOURCE_IMAGE, &unrecoverable_sectors);

      if(current_missing)
      {  if(first_missing < 0) first_missing = s;
         last_missing = s;
	 total_missing++;
	 new_missing++;
	 if(s < lay->dataSectors) data_missing++;
	 else if(s >= lay->dataSectors + 2 && s < lay->protectedSectors) crc_missing++;
	 else ecc_missing++;
	 defective = TRUE;
      }

      /* Report dead sectors. Combine subsequent missing sectors into one report. */

      if(!current_missing || s==expected_sectors-1)
      {  if(first_missing>=0)
	 {   if(first_missing == last_missing)
	           PrintCLI(_("* missing sector   : %" PRId64 "\n"), first_missing);
	     else PrintCLI(_("* missing sectors  : %" PRId64 " - %" PRId64 "\n"), first_missing, last_missing);
	     first_missing = -1;
	 }
      }

      /* If the image sector is from the data portion and it was readable, 
	 test its CRC sum */

      if(s < lay->dataSectors && !current_missing)
      {  guint32 crc = Crc32(buf, 2048);

	 if(cc->crcValid[crc_idx] && crc != cc->crcBuf[crc_idx])
	 {  PrintCLI(_("* CRC error, sector: %" PRId64 "\n"), s);
	    data_crc_errors++;
	    new_crc_errors++;
	    defective = TRUE;
	 }
      }
      crc_idx++;

      if(!defective)
	SetBit(cc->map, s);

      /* Calculate the ecc checksum */

      if(s == RS02EccSectorIndex(lay, ecc_slice, ecc_sector))
      {  MD5Update(&ecc_md5, buf, 2048);
	 ecc_sector++;
	 if(ecc_sector >= lay->sectorsPerLayer)
	 {  MD5Final(ecc_sum, &ecc_md5); 
	    MD5Init(&ecc_md5);
	    MD5Update(&meta_md5, ecc_sum, 16);

	    ecc_sector = 0;
	    ecc_slice++;
	 }
      }

      if(Closure->guiMode) 
	    percent = (VERIFY_IMAGE_SEGMENTS*(s+1))/expected_sectors;
      else  percent = (100*(s+1))/expected_sectors;

      if(last_percent != percent) 
      {  PrintProgress(_("- testing sectors  : %3d%%") ,percent);
#ifdef WITH_GUI_YES
	 if(Closure->guiMode)
	 {  add_verify_values(self, percent, new_missing, new_crc_errors); 
	    if(data_missing || data_crc_errors)
	    {  GuiSetLabelText(wl->cmpDataSection, 
			       _("<span %s>%" PRId64 " sectors missing; %" PRId64 " CRC errors</span>"),
			       Closure->redMarkup, data_missing, data_crc_errors);
	    }
	    if(crc_missing)
	    {  GuiSetLabelText(wl->cmpCrcSection, 
			       _("<span %s>%" PRId64 " sectors missing</span>"),
			       Closure->redMarkup, crc_missing);
	    }
	    if(ecc_missing)
	    {  GuiSetLabelText(wl->cmpEccSection, 
			       _("<span %s>%" PRId64 " sectors missing</span>"),
			       Closure->redMarkup, ecc_missing);
	    }
	 }
#endif /* WITH_GUI_YES */
	 last_percent = percent;
	 new_missing = new_crc_errors = 0;
      }
   }

   /* Complete damage summary */

   if(Closure->guiMode)
   {  if(data_missing || data_crc_errors)
      {  GuiSetLabelText(wl->cmpDataSection, 
			 _("<span %s>%" PRId64 " sectors missing; %" PRId64 " CRC errors</span>"),
			 Closure->redMarkup, data_missing, data_crc_errors);
      }
      if(crc_missing)
      {  GuiSetLabelText(wl->cmpCrcSection, 
			 _("<span %s>%" PRId64 " sectors missing</span>"),
			 Closure->redMarkup, crc_missing);
      }
      if(ecc_missing)
      {  GuiSetLabelText(wl->cmpEccSection, 
			 _("<span %s>%" PRId64 " sectors missing</span>"),
			 Closure->redMarkup, ecc_missing);
      }
   }

   /* The image md5sum is only useful if all blocks have been successfully read. */

   MD5Final(medium_sum, &image_md5);
   AsciiDigest(data_digest, medium_sum);

   MD5Final(ecc_sum, &meta_md5); 
	    
   /* Do a resume of our findings */ 

   if(!total_missing && !hdr_missing && !hdr_crc_errors && !data_crc_errors)
      PrintLog(_("- good image       : all sectors present\n"
		 "- data md5sum      : %s\n"),data_digest);
   else
   {  gint64 total_crc_errors = data_crc_errors + hdr_crc_errors;

      if(!total_missing && !total_crc_errors)
         PrintLog(_("* suspicious image : contains damaged ecc headers\n"));
      else
      {  if(!total_crc_errors)
	   PrintLog(_("* BAD image        : %" PRId64 " sectors missing\n"), total_missing);
	 if(!total_missing)
	   PrintLog(_("* suspicious image : all sectors present, but %" PRId64 " CRC errors\n"), total_crc_errors);
	 if(total_missing && total_crc_errors)
	   PrintLog(_("* BAD image        : %" PRId64 " sectors missing, %" PRId64 " CRC errors\n"), 
		    total_missing, total_crc_errors);
      }

      PrintLog(_("  ... ecc headers    : %" PRId64 " ok, %" PRId64 " CRC errors, %" PRId64 " missing\n"),
		 hdr_ok, hdr_crc_errors, hdr_missing);
      PrintLog(_("  ... data section   : %" PRId64 " sectors missing; %" PRId64 " CRC errors\n"), 
	       data_missing, data_crc_errors);
      if(!data_missing)
	PrintLog(_("  ... data md5sum    : %s\n"), data_digest); 
      PrintLog(_("  ... crc section    : %" PRId64 " sectors missing\n"), crc_missing);
      PrintLog(_("  ... ecc section    : %" PRId64 " sectors missing\n"), ecc_missing);
   }

   if(Closure->guiMode)
   {  if(!data_missing && !data_crc_errors) 
      {                  GuiSetLabelText(wl->cmpDataSection, _("complete")); }
      if(!crc_missing)  { GuiSetLabelText(wl->cmpCrcSection, _("complete")); }
      if(!ecc_missing)  { GuiSetLabelText(wl->cmpEccSection, _("complete")); }
     
      GuiSetLabelText(wl->cmpImageMd5Sum, "%s", data_missing ? "-" : data_digest);

      if(img_advice) 
      {  GuiSetLabelText(wl->cmpImageResult, "%s", img_advice);
         g_free(img_advice);
      }
      else 
      {  if(!total_missing && !hdr_missing && !hdr_crc_errors && !data_crc_errors)
	 {  GuiSetLabelText(wl->cmpImageResult,
			    _("<span %s>Good image.</span>"),
			    Closure->greenMarkup);
	 }
	 else
	 {  GuiSetLabelText(wl->cmpImageResult,
			    _("<span %s>Damaged image.</span>"),
			    Closure->redMarkup);
	 }
      }
   }

   /*** Print some information on the ecc portion */
continue_with_ecc:
   PrintLog(_("\nError correction data: "));

   major = eh->creatorVersion/10000; 
   minor = (eh->creatorVersion%10000)/100;
   micro = eh->creatorVersion%100;

   /* Suppress (unstable) output in debug mode to facilitate regression tests */
   if((eh->methodFlags[3] & MFLAG_DEVEL) && !Closure->regtestMode)
     unstable=" (unstable)";

   if(micro)        
   {  char *format = "%s-%d.%d.%d%s";

      PrintLog(format, _("created by dvdisaster"), major, minor, micro, unstable);
      PrintLog("\n");

      GuiSwitchAndSetFootline(wl->cmpEccNotebook, 1,
			      wl->cmpEccCreatedBy, 
			      format, "dvdisaster",
			      major, minor, micro, unstable);
   }
   else
   {  char *format = "%s-%d.%d%s";

      PrintLog(format, _("created by dvdisaster"), major, minor, unstable);
      PrintLog("\n");

      GuiSwitchAndSetFootline(wl->cmpEccNotebook, 1,
			      wl->cmpEccCreatedBy,
			      format, "dvdisaster",
			      major, minor, unstable);
   }

   /* Error correction method */

   memcpy(method, eh->method, 4); method[4] = 0;

   PrintLog(_("- method           : %4s, %d roots, %4.1f%% redundancy.\n"),
	    method, eh->eccBytes, 
	    ((double)eh->eccBytes*100.0)/(double)eh->dataBytes);

   GuiSetLabelText(wl->cmpEccMethod, _("%4s, %d roots, %4.1f%% redundancy"),
		   method, eh->eccBytes, 
		   ((double)eh->eccBytes*100.0)/(double)eh->dataBytes);

   /* required dvdisaster version */

   if(Closure->version >= eh->neededVersion)
   {  PrintLog(_("- requires         : dvdisaster-%d.%d (good)\n"),
	       eh->neededVersion/10000,
	       (eh->neededVersion%10000)/100);


      GuiSetLabelText(wl->cmpEccRequires, "dvdisaster-%d.%d",
		      eh->neededVersion/10000,
		      (eh->neededVersion%10000)/100);
   }
   else 
   {  PrintLog(_("* requires         : dvdisaster-%d.%d (BAD)\n"
		 "* Warning          : The following output might be incorrect.\n"
		 "*                  : Please upgrade dvdisaster.\n"),
	       eh->neededVersion/10000,
	       (eh->neededVersion%10000)/100);


     if(Closure->guiMode)
     {  GuiSetLabelText(wl->cmpEccRequires, 
			"<span %s>dvdisaster-%d.%d</span>",
			Closure->redMarkup,
			eh->neededVersion/10000,
			(eh->neededVersion%10000)/100);
        if(!ecc_advice) 
	   ecc_advice = g_strdup_printf(_("<span %s>Please upgrade your version of dvdisaster!</span>"), Closure->redMarkup);
     }
     else
     {
        if(!ecc_advice) 
	   ecc_advice = g_strdup(_("Please upgrade your version of dvdisaster!"));
     }
   }

   /* Number of sectors medium is supposed to have */

   if(image->sectorSize == expected_sectors)
   {  PrintLog(_("- medium sectors   : %" PRId64 " / %" PRId64 " (good)\n"), 
	       expected_sectors, lay->dataSectors);

      GuiSetLabelText(wl->cmpEccMediumSectors, "%" PRId64 " / %" PRId64 "", 
		      expected_sectors, lay->dataSectors);
   }
   else 
   {  if(image->sectorSize > expected_sectors && image->sectorSize - expected_sectors <= 2)   
           PrintLog(_("* medium sectors   : %" PRId64 " (BAD, perhaps TAO/DAO mismatch)\n"),
		    expected_sectors);
      else PrintLog(_("* medium sectors   : %" PRId64 " (BAD)\n"),expected_sectors);

      if(Closure->guiMode)
      {  GuiSetLabelText(wl->cmpEccMediumSectors, 
			 "<span %s>%" PRId64 "</span>", Closure->redMarkup, expected_sectors);
	 if(!ecc_advice && image->sectorSize > expected_sectors)
	    ecc_advice = g_strdup_printf(_("<span %s>Image size does not match recorded size.</span>"), Closure->redMarkup);
      }
   }

   if(Closure->quickVerify)  /* take shortcut again */
     goto terminate;

   /* image md5sum as stored in the ecc header */

   AsciiDigest(hdr_digest, eh->mediumSum);

   if(!data_missing)
   {  int n = !memcmp(eh->mediumSum, medium_sum, 16);

      if(n) PrintLog(_("- data md5sum      : %s (good)\n"),hdr_digest);
      else  PrintLog(_("* data md5sum      : %s (BAD)\n"),hdr_digest);

      if(n)
      {  GuiSetLabelText(wl->cmpEcc1Msg, "%s", hdr_digest);
      }
      else  
      {  GuiSetLabelText(wl->cmpEcc1Msg, "<span %s>%s</span>", Closure->redMarkup, hdr_digest);
	 GuiSetLabelText(wl->cmpImageMd5Sum, "<span %s>%s</span>", Closure->redMarkup, data_digest);
      }
   }
   else 
   {  PrintLog(_("- data md5sum      : %s\n"), "-");

      GuiSetLabelText(wl->cmpEcc1Msg, "%s", "-");
   }

   /*** md5sum of the crc portion */

   AsciiDigest(digest, cc->crcSum);

   if(!crc_missing)
   {  if(!memcmp(eh->crcSum, cc->crcSum, 16))
      {  PrintLog(_("- crc md5sum       : %s (good)\n"),digest);
	 GuiSetLabelText(wl->cmpEcc2Msg, "%s", digest);
      }
      else 
      {  PrintLog(_("* crc md5sum       : %s (BAD)\n"),digest);
	 GuiSetLabelText(wl->cmpEcc2Msg, "<span %s>%s</span>", Closure->redMarkup, digest);
	 ecc_md5_failure = TRUE;
      }
   }
   else
   {  PrintLog(_("- crc md5sum       : %s\n"), "-");
     
      GuiSetLabelText(wl->cmpEcc2Msg, "%s", "-");
   }

   /*** meta md5sum of the ecc slices */

   AsciiDigest(digest, ecc_sum);

   if(!ecc_missing)
   {  if(!memcmp(eh->eccSum, ecc_sum, 16))
      {    PrintLog(_("- ecc md5sum       : %s (good)\n"),digest);
	   GuiSetLabelText(wl->cmpEcc3Msg, "%s", digest);
      }
      else 
      {    PrintLog(_("* ecc md5sum       : %s (BAD)\n"),digest);
	   GuiSetLabelText(wl->cmpEcc3Msg, "<span %s>%s</span>", Closure->redMarkup, digest);
	   ecc_md5_failure = TRUE;
      }
   }
   else
   {  PrintLog(_("- ecc md5sum       : %s\n"), "-");
     
      GuiSetLabelText(wl->cmpEcc3Msg, "%s", "-");
   }


   /*** Print final results */

   try_it = prognosis(cc, total_missing + data_crc_errors - hdr_correctable, expected_sectors);

   if(Closure->guiMode)
   {  if(ecc_advice) 
      {  GuiSetLabelText(wl->cmpEccResult, "%s", ecc_advice);
         g_free(ecc_advice);
      }
      else 
	if(!crc_missing && !ecc_missing && !hdr_missing && !hdr_crc_errors && !ecc_md5_failure)
	{  GuiSetLabelText(wl->cmpEccResult,
			   _("<span %s>Good error correction data.</span>"),
			   Closure->greenMarkup);
	}
        else
	{  if(try_it)
	   {  GuiSetLabelText(wl->cmpEccResult,
			      _("<span %s>Full data recovery is likely.</span>"),
			      Closure->greenMarkup);
	   }
	   else
	   {  GuiSetLabelText(wl->cmpEccResult,
			      _("<span %s>Full data recovery is NOT possible.</span>"),
			      Closure->redMarkup);
	   }
	}
   }

   /*** Close and clean up */

terminate:
   cleanup((gpointer)cc);
}
