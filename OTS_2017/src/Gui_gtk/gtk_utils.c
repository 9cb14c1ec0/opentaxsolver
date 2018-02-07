/********************************************************************************/
/* Gtk_Utils.c - Re-usable convenience functions for making Gtk-based 		*/
/*  applications.  Useful calls for common GUI widgets, makes creating new	*/
/*  Gtk programs simple, quick, easier, and more robust and maintainable.	*/
/*										*/
/* To compile Gtk programs, add:						*/
/*	  `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0`		*/
/*  ... to your compile command.						*/
/*										*/
/* Version 1.2 - 1-27-2014							*/
/********************************************************************************/
#include "gtk_utils.h"
GtkWidget *outer_window=0;


/********************************************************************************/
/* strcpy_safe - Copy src string to dst string, upto maxlen characters.         */
/* Safer than strncpy, because it does not fill destination string,             */
/* but only copies up to the length needed.  Src string should be               */
/* null-terminated, and must-be if its allocated length is shorter than maxlen. */
/* Up to maxlen-1 characters are copied to dst string. The dst string is always */
/* null-terminated.  The dst string should be pre-allocated to at least maxlen  */
/* bytes.  However, this function will work safely for dst arrays that are less */
/* than maxlen, as long as the null-terminated src string is known to be        */
/* shorter than the allocated length of dst, just like regular strcpy.          */
/********************************************************************************/
void strcpy_safe( char *dst, const char *src, int maxlen )
{ 
  int j=0, oneless;
  oneless = maxlen - 1;
  while ((j < oneless) && (src[j] != '\0')) { dst[j] = src[j];  j++; }
  dst[j] = '\0';
}




/* ------------- GTK Text / Label Routines ----------------- */

GtkWidget *make_label( GtkWidget *panel, int xpos, int ypos, const char *text )
{
 GtkWidget *bpanel, *label;
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 label = gtk_label_new( text );
 gtk_container_add( GTK_CONTAINER( bpanel ), label );
 return label;
}

void modify_label( GtkWidget *label, const char *newtext )
{
 gtk_label_set_text( (GtkLabel *)label, newtext );
}

void set_widget_color( GtkWidget *label, const char *color_value )	/* Specify colors as: "#rrggbb" in hex 00-ff. */
{
  GdkColor color;
  gdk_color_parse( color_value, &color );
  gtk_widget_modify_fg( label, GTK_STATE_NORMAL, &color );
}

void set_background_color( GtkWidget *label, const char *color_value )   /* Specify colors as: "#rrggbb" in hex 00-ff. */
{
  GdkColor color;
  gdk_color_parse( color_value, &color );
  gtk_widget_modify_bg( label, GTK_STATE_NORMAL, &color );
}

GtkWidget *make_sized_label( GtkWidget *panel, int xpos, int ypos, const char *text, float fontsize )
{
 GtkWidget *bpanel, *label;
 char *tmptxt;
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 label = gtk_label_new( "" );
 tmptxt = (char *)malloc( strlen(text) + 100 );
 sprintf( tmptxt, "<span font_desc=\"%g", fontsize );
 strcat( tmptxt, "\">" );  strcat( tmptxt, text );  strcat( tmptxt, "</span>" );
 gtk_label_set_markup( (GtkLabel *)label, tmptxt );
 free( tmptxt );
 gtk_container_add( GTK_CONTAINER( bpanel ), label );
 return label;
}



/* ------------- End GTK Text / Label Routines ----------------- */




/* ------------- GTK Button Routines ----------------- */

GtkWidget *make_button( GtkWidget *panel, int xpos, int ypos, const char *label, void callback(GtkWidget *, void *), void *data )
{
 GtkWidget *bpanel, *button;

 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 button = gtk_button_new_with_label( label );
 if (callback != 0)
  gtk_signal_connect( GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC( callback ), data );
 gtk_container_add( GTK_CONTAINER( bpanel ), button );
 return button;
}

GtkWidget *make_button_wicon( GtkWidget *panel, int xpos, int ypos, const char **icon, void callback(GtkWidget *, void *), void *data )
{  /* Be sure to somewhere set:  g_object_set( gtk_settings_get_default(), "gtk-button-images", TRUE, NULL); */
 GtkWidget *bpanel, *button, *image;	/* To use, "#include" an xpm image file of icon. */ 
 GdkPixbuf *pixbuf;			/*  Make sure variable-name at top of data declaration matches "icon" name */
					/*  called in this function.  (You call it with that variable name.) */
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 button = gtk_button_new();
 if (callback != 0)
  gtk_signal_connect( GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC( callback ), data );
 gtk_container_add( GTK_CONTAINER( bpanel ), button );
 pixbuf = gdk_pixbuf_new_from_xpm_data( (const char **)icon );
 image = gtk_image_new_from_pixbuf( pixbuf );
 gtk_button_set_image( (GtkButton *)button, image );
 return button;
}

GtkWidget *make_button_wsizedcolor_text( GtkWidget *panel, int xpos, int ypos, const char *text, float fontsize, 
					 const char *color_value, void callback(GtkWidget *, void *), void *data )
{
 GtkWidget *bpanel, *button, *label;
 GdkColor color;
 char *tmptxt;
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 button = gtk_button_new();
 label = gtk_label_new( "" );
 tmptxt = (char *)malloc( strlen(text) + 100 );
 sprintf( tmptxt, "<span font_desc=\"%g", fontsize );
 strcat( tmptxt, "\">" );  strcat( tmptxt, text );   strcat( tmptxt, "</span>" );
 gtk_label_set_markup( (GtkLabel *)label, tmptxt );
 if (strcmp( color_value, "#000000" ) != 0)
  {
   gdk_color_parse( color_value, &color );
   gtk_widget_modify_fg( label, GTK_STATE_NORMAL, &color );
  }  
 free( tmptxt );
 gtk_container_add( GTK_CONTAINER( button ), label );
 if (callback != 0)
  gtk_signal_connect( GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC( callback ), data );
 gtk_container_add( GTK_CONTAINER( bpanel ), button );
 return button;
}



/* ------------- End GTK Button Routines ----------------- */




/* ------------- GTK Radio Button Routines ----------------- */

GtkWidget *make_radio_button( GtkWidget *panel, GtkWidget *group, int xpos, int ypos, const char *label, void callback(GtkWidget *, void *), void *data )
{
 GtkWidget *bpanel, *button;

 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 button = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON( group ), label );
 if (callback != 0)
  gtk_signal_connect( GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC( callback ), data );
 gtk_container_add( GTK_CONTAINER( bpanel ), button );
 return button;
}

void set_radio_button( GtkWidget *radio_button )
{
 gtk_toggle_button_set_active( (GtkToggleButton *)radio_button, 1 );
}
/* ------------- End GTK Radio Button Routines ----------------- */




/* ------------- GTK Toggle Button Routines ----------------- */

GtkWidget *make_toggle_button( GtkWidget *panel, int xpos, int ypos, const char *label, int state, void callback(GtkWidget *, void *), void *data )
{
 GtkWidget *bpanel, *button;

 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 button = gtk_check_button_new_with_label( label );
 if (callback != 0)
  gtk_signal_connect( GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC( callback ), data );
 gtk_container_add( GTK_CONTAINER( bpanel ), button );
 gtk_toggle_button_set_active( (GtkToggleButton *)button, state );
 return button;
}

void set_toggle_button( GtkWidget *toggle_button, int state )
{
 gtk_toggle_button_set_active( (GtkToggleButton *)toggle_button, state );
}
/* ------------- End GTK Toggle Button Routines ----------------- */




/* ------------- GTK Form-Box (Text-Entry Box) Routines ----------------- */
 float charsperpix=0.109;
 GtkWidget *current_formbox;

GtkEntry *make_formbox( GtkWidget *panel, int xpos, int ypos, int nchars_wide, const char *text, int maxlen, 
		       void callback(GtkWidget *, void *), void *data )
{
 GtkWidget *formbox, *bpanel;
 formbox = gtk_entry_new();
 gtk_entry_set_max_length( GTK_ENTRY(formbox), maxlen );
 gtk_entry_set_text( GTK_ENTRY(formbox), text );
 bpanel = gtk_fixed_new();
 current_formbox = bpanel;
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 gtk_entry_set_width_chars( GTK_ENTRY(formbox), nchars_wide );
 if (callback != 0)
  gtk_signal_connect( GTK_OBJECT(formbox), "activate", GTK_SIGNAL_FUNC( callback ), data );
 gtk_container_add( GTK_CONTAINER( bpanel ), formbox );
 return GTK_ENTRY(formbox);
}


GtkEntry *make_formbox_bypix( GtkWidget *panel, int xpos, int ypos, int npix_wide, const char *text, int maxlen, 
		       void callback(GtkWidget *, void *), void *data )
{
 int nchars;
 GtkEntry *nfrmbx;
 GtkRequisition actual;
 nchars = charsperpix * npix_wide;
 nfrmbx = make_formbox( panel, xpos, ypos, nchars, text, maxlen, callback, data );
 gtk_widget_size_request( (GtkWidget *)nfrmbx, &actual );   /* Find the box's actual size. */
 if ((actual.width > npix_wide + 1) || (npix_wide - actual.width > 1.0 / charsperpix))
  {
   gtk_widget_destroy( current_formbox );
   charsperpix = charsperpix * (float)npix_wide / (float)(actual.width);
   nchars = charsperpix * npix_wide;
   nfrmbx = make_formbox( panel, xpos, ypos, nchars, text, maxlen, callback, data );
  }
 return nfrmbx;
}


void modify_formbox( GtkEntry *formbox, const char *text )
{
 gtk_entry_set_text( formbox, text );
}


char *get_formbox( GtkEntry *formbox )
{
 return (char *)gtk_entry_get_text( formbox );		/* Returns string poiner, which must NOT be freed. */
}


void get_formbox_text( GtkEntry *formbox, char *rtrnstrng, int maxlen )
{
 char *tmpstrptr;
 tmpstrptr = (char *)gtk_entry_get_text( formbox );
 strcpy_safe( rtrnstrng, tmpstrptr, maxlen );
}


	/*--- Example Usage:
	 GtkEntry *formbox1;

	 SetFormBox_Routine()
	  {
	   int nchars;
	   nchars = 0.109 * frmbx_wdth_pix;	// Good rule of thumb.
	   formbox1 = make_formbox( panel, xpos, ypos, nchars, "0", 1024, action_routine, 0 );
	  }

	 Retreival_routine()
	  {
	   char *word;
	   word = get_formbox( formbox1 );
	   printf("Text = '%s'\n", word );
	  }
	---*/

/* ------------- GTK Form-Box (Text-Entry Box) Routines ----------------- */


/* ------------- GTK Text-Edit Box Routines ----------------- */

GtkTextView *make_text_edit_box( GtkWidget *panel, int xpos, int ypos, int width, int height, const char *text )       /* Multi-line text entry/edit widget. */
{
 GtkWidget *bpanel, *scroll_window, *frame;
 GtkTextView *textview;
 GtkTextBuffer *buffer;

 textview = (GtkTextView *)gtk_text_view_new();
 buffer = gtk_text_view_get_buffer( textview );
 gtk_text_buffer_set_text( buffer, text, -1 );
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 gtk_widget_set_size_request( bpanel, width, height );

 scroll_window = gtk_scrolled_window_new( NULL, NULL );
 gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scroll_window ), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS );
 gtk_widget_set_size_request( scroll_window, width, height );

 frame = gtk_frame_new("");
 gtk_frame_set_shadow_type( (GtkFrame *)frame, GTK_SHADOW_OUT );
 gtk_container_add( GTK_CONTAINER( scroll_window ), (GtkWidget *)textview );
 gtk_container_add( GTK_CONTAINER( frame ), scroll_window );
 gtk_container_add( GTK_CONTAINER( bpanel ), frame );
 return textview;
}


void modify_text_edit_box( GtkTextView *textview, const char *text )
{
 GtkTextBuffer *buf;
 buf = gtk_text_view_get_buffer( textview );
 gtk_text_buffer_set_text( buf, text, -1 );
}


void get_text_edit_box( GtkTextView *textview, char *rtrnstrng, int maxlen )
{
 GtkTextBuffer *buf;
 GtkTextIter start, end;
 char *tmpstrptr;

 buf = gtk_text_view_get_buffer( textview );
 gtk_text_buffer_get_bounds( buf, &start, &end );
 tmpstrptr = gtk_text_buffer_get_text( buf, &start, &end, 0 );
 strcpy_safe( rtrnstrng, tmpstrptr, maxlen );
}


        /*--- Example Usage:
         GtkTextView *txtedt;

         MakeTextEditWindow_Routine()
          {
           txtedt = make_text_edit_box( panel, 2, 2, 200, 100, "Initial text\n is here." );
          }

         Retreival_routine()
          {
           char word[1000];
           get_text_edit_box( txtedt, word, 1000 );
           printf("Text = '%s'\n", word );
          }
        ---*/

/* ------------- GTK Text-Edit-Box Routines ----------------- */



/* ------------- GTK Slider Routines ----------------- */
 
GtkWidget *make_slider( GtkWidget *panel, int xpos, int ypos, int size, char orien, double min, double initval, double max, \
			void callback(GtkWidget *, void *), void *data )
{ /* Read double value with:  x = gtk_range_get_value( GTK_RANGE( slider ) );	*/
 GtkWidget *bpanel, *slider;

 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 if (orien == 'h')
  {
   slider = gtk_hscale_new_with_range( min, max, 0.05 * (max - min) );
   gtk_widget_set_size_request( slider, size, 40 );
  }
 else
  {
   slider = gtk_vscale_new_with_range( min, max, 0.1 * (max - min) );
   gtk_widget_set_size_request( slider, 40, size );
  }
 if (callback != 0)
  gtk_signal_connect( GTK_OBJECT(slider), "value-changed", GTK_SIGNAL_FUNC( callback ), data );
 gtk_container_add( GTK_CONTAINER( bpanel ), slider );
 gtk_range_set_value( GTK_RANGE(slider), initval );
 return slider;
}

	/*--- Example Usage:
	void slide_adj( GtkWidget *widget, gpointer data )
	{
	 double x;
	 x = gtk_range_get_value( GTK_RANGE(widget) );
	 printf("Slider Adjusted '%g'\n", x );
	}

	...
	make_slider( panel, xpos, ypos, size, 'h', minv, initval, maxval, slide_adj, 0 );
	---*/

void adjust_slider( GtkWidget *slider, double newvalue )
{
 gtk_range_set_value( GTK_RANGE(slider), newvalue ); 
}

/* ------------- End GTK Slider Routines ----------------- */




/* ------------- GTK FormBoxWithSuggestor ------------------- */

GtkSpinButton *make_FormBoxWithSuggestor( GtkWidget *panel, int xpos, int ypos, float min, 
					  float initval, float max, float step, int ndigits )
{
  GtkWidget *bpanel;
  GtkAdjustment *spinner_adj;
  GtkSpinButton *spinbox;
  spinner_adj = (GtkAdjustment *)gtk_adjustment_new( initval, min, max, step, 3, 0 );
  spinbox = (GtkSpinButton *)gtk_spin_button_new( spinner_adj, step, ndigits );
  bpanel = gtk_fixed_new();
  gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
  gtk_container_add( GTK_CONTAINER( bpanel ), (GtkWidget *)spinbox );
  return spinbox;
}


GtkSpinButton *make_FormBoxWithSuggestor_bypix( GtkWidget *panel, int xpos, int ypos, int w1, float min, 
						float initval, float max, float step, int ndigits )
{
  GtkWidget *bpanel;
  GtkAdjustment *spinner_adj;
  GtkSpinButton *spinbox;
  spinner_adj = (GtkAdjustment *)gtk_adjustment_new( initval, min, max, step, 3, 0 );
  spinbox = (GtkSpinButton *)gtk_spin_button_new( spinner_adj, step, ndigits );
  gtk_widget_set_size_request( (GtkWidget *)spinbox, w1, 25);
  bpanel = gtk_fixed_new();
  gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
  gtk_container_add( GTK_CONTAINER( bpanel ), (GtkWidget *)spinbox );
  return spinbox;
}


float get_FormBoxWithSuggestor_Value( GtkSpinButton *spinbox )
{ return gtk_spin_button_get_value( spinbox ); }


void set_FormBoxWithSuggestor_value( GtkSpinButton *spinbox, float value )
{ gtk_spin_button_set_value( spinbox, value ); }


/*---
 make_combobox - Creates a kind of text-box that the user cannot type into, but has
    a selector on the right, from which the user can pick any alphanumeric setting.
---*/
GtkWidget *make_combobox( GtkWidget *panel, int xpos, int ypos, int width )
{
 GtkWidget *bpanel, *combobox;
 combobox = (GtkWidget *)gtk_combo_box_text_new();
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 gtk_widget_set_size_request( combobox, width, 25 );
 gtk_container_add( GTK_CONTAINER( bpanel ), combobox );
 return combobox;
}

void add_combo_suggestion( GtkWidget *combobox, char *itemtext )
{
 gtk_combo_box_text_append_text( (GtkComboBoxText *)combobox, itemtext );
}

void select_combo_suggestion( GtkWidget *combobox, int item )
{
 gtk_combo_box_set_active( (GtkComboBox *)combobox, item );
}

	/* Example to read value from:   frmbx1 = make_combobox( panel, x, y, 30 );
		word = gtk_combo_box_text_get_active_text( (GtkComboBoxText *)frmbx1 );
	*/


/*---
 make_formbox_wcombo - Like "make_combobox" above, but users can also type their
  own values into the text box, as well as select them.
---*/
GtkWidget *make_formbox_wcombo( GtkWidget *panel, int xpos, int ypos, int width )
{
 GtkWidget *bpanel, *combobox;
 combobox = gtk_combo_box_entry_new_text();
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 gtk_widget_set_size_request( combobox, width, 25 );
 gtk_container_add( GTK_CONTAINER( bpanel ), combobox );
 return combobox;
}

void add_form_suggestion( GtkWidget *combobox, char *text )
{
 gtk_combo_box_append_text( GTK_COMBO_BOX( combobox ), text );
}

	/* Example to read value from:   frmbx2 = make_formbox_wcombo( panel, x, y, 30 );
		word = strdup( gtk_entry_get_text( GTK_ENTRY( gtk_bin_get_child( GTK_BIN( frmbx2 ) ) ) ) );
	*/ 


/* ------------- End GTK FormBoxWithSuggestor ------------------- */





/* ------------- GTK Menu Routines ----------------- */
GtkWidget *most_recent_menu=0;

GtkWidget *make_menu( GtkWidget *tool_bar, const char *label )	/* For use on automatically placed menu-bars. */
{
 GtkWidget *new_menu, *sub_menu;
 new_menu = gtk_menu_item_new_with_label( label );
 gtk_menu_bar_append( GTK_MENU_BAR( tool_bar ), new_menu );
 sub_menu = gtk_menu_new();
 gtk_menu_item_set_submenu( GTK_MENU_ITEM( new_menu ), sub_menu );
 return sub_menu;
}


GtkWidget *make_menu_button( GtkWidget *panel, int xpos, int ypos, const char *label )  /* For use anywhere on a window. */
{
 GtkWidget *new_menu, *sub_menu, *bpanel;
 bpanel = gtk_menu_bar_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 new_menu = gtk_menu_item_new_with_label( label );
 most_recent_menu = new_menu;
 gtk_container_add( GTK_CONTAINER( bpanel ), new_menu );
 sub_menu = gtk_menu_new();
 gtk_menu_item_set_submenu( GTK_MENU_ITEM( new_menu ), sub_menu );
 return sub_menu;
}


GtkWidget *add_menu_item( GtkWidget *menu, const char *label, void callback(GtkWidget *, void *), void *data )
{
 GtkWidget *menuitem;
 menuitem = gtk_menu_item_new_with_label( label );
 if (callback != 0)
  gtk_signal_connect( GTK_OBJECT(menuitem), "activate", GTK_SIGNAL_FUNC( callback ), data );
 gtk_menu_append( GTK_MENU(menu), menuitem );
 return menuitem;
}


GtkWidget *add_submenu( GtkWidget *menu, const char *label )
{
 GtkWidget *menuitem, *submenu;
 menuitem = gtk_menu_item_new_with_label( label );
 gtk_menu_append( GTK_MENU(menu), menuitem );
 submenu = gtk_menu_new();
 gtk_menu_item_set_submenu( GTK_MENU_ITEM( menuitem ), submenu );
 return submenu;
}


void modify_menu_label( GtkWidget *menu_label, const char *newtext )	 /* A 'menu_label' is what is returned by add_menu_item(). */
{
 gtk_menu_item_set_label( (GtkMenuItem *)menu_label, newtext );
}

/* ----------- End GTK Menu Routines ---------------- */




/* ------------- GTK Selection List Routines ----------------- */
GtkWidget *most_recent_selector_box=0, *LastBpanel;

GtkTreeStore *make_selection_list( GtkWidget *panel, int xpos, int ypos, int width, int height, const char *column_titles,
				  void callback(GtkWidget *, void *), void dclick_callback(GtkWidget *, void *), void *data )
{
 GtkWidget *bpanel, *scroll_window, *tree; 
 GtkTreeStore *list;
 GtkCellRenderer *renderer;
 GtkTreeViewColumn *column;
 GtkTreeSelection *select;

 list = gtk_tree_store_new( 1, G_TYPE_STRING );
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 gtk_widget_set_size_request( bpanel, width, height );
 LastBpanel = bpanel;

 scroll_window = gtk_scrolled_window_new( NULL, NULL );
 most_recent_selector_box = scroll_window;
 gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scroll_window ), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS );
 gtk_widget_set_size_request( scroll_window, width, height );
 gtk_container_add( GTK_CONTAINER( bpanel ), scroll_window );

 tree = gtk_tree_view_new_with_model( GTK_TREE_MODEL( list ) );
 renderer = gtk_cell_renderer_text_new();
 column = gtk_tree_view_column_new_with_attributes( column_titles, renderer, "text", 0, NULL );
 gtk_tree_view_append_column( GTK_TREE_VIEW(tree), column );
 if (dclick_callback != 0)
  g_signal_connect( G_OBJECT( tree ), "row-activated", G_CALLBACK( dclick_callback ), data );

 gtk_container_add( GTK_CONTAINER( scroll_window ), tree );
 select = gtk_tree_view_get_selection( GTK_TREE_VIEW(tree) );
 gtk_tree_selection_set_mode( select, GTK_SELECTION_SINGLE );
 if (callback != 0)
  g_signal_connect( G_OBJECT(select), "changed", G_CALLBACK( callback ), data );

 /* Cluge: Create blank entry at top of list, since first entry is always preselected and cannot be selected by user. */
 //  gtk_tree_store_append( list, &iter, 0 );
 //  gtk_tree_store_set( list, &iter, 0, "", -1 );

 return list;
}

void append_selection_list( GtkTreeStore *list, GtkTreeIter *iter, const char *item )
{	/* To use, just declare a "GtkTreeIter iter;", and pass its address in. */
 gtk_tree_store_append( list, iter, 0 );
 gtk_tree_store_set( list, iter, 0, item, -1 );
}

void append_selection_child( GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *child, char *item )
{ /* Must declare "GtkTreeIter iter, child;", and pass their addresses in. */
 gtk_tree_store_append( list, child, iter );
 gtk_tree_store_set( list, child, 0, item, -1 );
}


char *get_selection_from_list( GtkWidget *selection )
{
 GtkTreeIter iter;
 GtkTreeModel *model;
 char *selected_item;

 if (gtk_tree_selection_get_selected( (GtkTreeSelection *)selection, &model, &iter))
  {
   gtk_tree_model_get(model, &iter, 0, &selected_item, -1);
   return selected_item;
  }
 else return 0;
}


/* Example Usage:
	{
	 GtkTreeStore *mylist;
	 GtkTreeIter iter;

	 mylist = make_selection_list( panel, 10, 20, 80, 100, myfunction, 0 );
	 for (j=0; j<10; j++)
	  append_selection_list( mylist, &iter, text[j] );
	}

       void myfunction( GtkWidget *wdg, void *data )
	{
	 char *text;
	 text = get_selection_from_list( wdg );
	 printf("Selected: '%s'\n", text );
	}
*/

GtkTreeStore *make_multicolumn_selection_list( GtkWidget *panel, int xpos, int ypos, int width, int height, 
		int ncols, const char *column_titles[],
                void callback(GtkWidget *, void *), void dclick_callback(GtkWidget *, void *), void *data )
{
 GtkWidget *bpanel, *scroll_window, *tree; 
 GtkTreeStore *list;
 GtkCellRenderer *renderer;
 GtkTreeViewColumn *column;
 GtkTreeSelection *select;
 int j;
 switch (ncols)
  { /* This will support up to 10 columns.  Extend if needed. */
   case 1:  list = gtk_tree_store_new( ncols, G_TYPE_STRING );	break;
   case 2:  list = gtk_tree_store_new( ncols, G_TYPE_STRING, G_TYPE_STRING );	break;
   case 3:  list = gtk_tree_store_new( ncols, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );	break;
   case 4:  list = gtk_tree_store_new( ncols, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );	break;
   case 5:  list = gtk_tree_store_new( ncols, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );	break;
   case 6:  list = gtk_tree_store_new( ncols, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );	break;
   case 7:  list = gtk_tree_store_new( ncols, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );	break;
   case 8:  list = gtk_tree_store_new( ncols, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );	break;
   case 9:  list = gtk_tree_store_new( ncols, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );	break;
   case 10:  list = gtk_tree_store_new( ncols, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );	break;
   default:  printf("Number of columns %d not supported.\n", ncols );  exit(1); 
  }
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 gtk_widget_set_size_request( bpanel, width, height );
 // LastBpanel = bpanel;
 scroll_window = gtk_scrolled_window_new( NULL, NULL );
 gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scroll_window ), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS );
 gtk_widget_set_size_request( scroll_window, width, height );
 gtk_scrolled_window_set_shadow_type( (GtkScrolledWindow *)scroll_window, GTK_SHADOW_IN );
 gtk_container_add( GTK_CONTAINER( bpanel ), scroll_window );
 tree = gtk_tree_view_new_with_model( GTK_TREE_MODEL( list ) );
 for (j=0; j < ncols; j++)
  {
   renderer = gtk_cell_renderer_text_new();
   column = gtk_tree_view_column_new_with_attributes( column_titles[j], renderer, "text", j, NULL );
   gtk_tree_view_append_column( GTK_TREE_VIEW(tree), column );
  }
 if (dclick_callback != 0)
  g_signal_connect( G_OBJECT(tree), "row-activated", G_CALLBACK( dclick_callback ), data );
 gtk_container_add( GTK_CONTAINER( scroll_window ), tree );
 select = gtk_tree_view_get_selection( GTK_TREE_VIEW(tree) );
 gtk_tree_selection_set_mode( select, GTK_SELECTION_SINGLE );
 if (callback != 0)
  g_signal_connect( G_OBJECT(select), "changed", G_CALLBACK( callback ), data );
 return list;
}

void add_multicolumn_selection_item( GtkTreeStore *selst, int ncols, char *items[] )
{
 GtkTreeIter iter;
 gtk_tree_store_append( selst, &iter, 0 );
 switch (ncols)
  { /* This will support up to 10 columns.  Extend if needed. */
   case 1: gtk_tree_store_set( selst, &iter, 0, items[0], -1 );  break;
   case 2: gtk_tree_store_set( selst, &iter, 0, items[0], 1, items[1], -1 );  break;
   case 3: gtk_tree_store_set( selst, &iter, 0, items[0], 1, items[1], 2, items[2], -1 );  break;
   case 4: gtk_tree_store_set( selst, &iter, 0, items[0], 1, items[1], 2, items[2], 3, items[3], -1 );  break;
   case 5: gtk_tree_store_set( selst, &iter, 0, items[0], 1, items[1], 2, items[2], 3, items[3], 4, items[4], -1 );  break;
   case 6: gtk_tree_store_set( selst, &iter, 0, items[0], 1, items[1], 2, items[2], 3, items[3], 4, items[4], 5, items[5], -1 );  break;
   case 7: gtk_tree_store_set( selst, &iter, 0, items[0], 1, items[1], 2, items[2], 3, items[3], 4, items[4], 5, items[5], 6, items[6], -1 );  break;
   case 8: gtk_tree_store_set( selst, &iter, 0, items[0], 1, items[1], 2, items[2], 3, items[3], 4, items[4], 5, items[5], 6, items[6], 7, items[7], -1 );  break;
   case 9: gtk_tree_store_set( selst, &iter, 0, items[0], 1, items[1], 2, items[2], 3, items[3], 4, items[4], 5, items[5], 6, items[6], 7, items[7], 8, items[8], -1 );  break;
   case 10: gtk_tree_store_set( selst, &iter, 0, items[0], 1, items[1], 2, items[2], 3, items[3], 4, items[4], 5, items[5], 6, items[6], 7, items[7], 8, items[8], 9, items[9], -1 );  break;
   default:  printf("Number of columns %d not supported.\n", ncols );  exit(1); 
  }
}

	/* Example Usage:
	   char *MY_Selected_item=0, *MY_Selected_list=0;
	   int MY_column_to_get=2;	// --- Set this to get the column you need.

	   void selected_item_by_column( GtkWidget *wdg, void *data )
	   {
	    GtkTreeIter iter;
	    GtkTreeModel *model;
	    if (!done_init) return;
	    if (gtk_tree_selection_get_selected( GTK_TREE_SELECTION(wdg), &model, &iter ))
	     {
	      if (MY_Selected_item != 0) free( MY_Selected_item );
	      gtk_tree_model_get( model, &iter, MY_column_to_get, &MY_Selected_item,  -1 );
	      MY_Selected_list = data;
	      printf("Column%d = %s, for %s\n", MY_, MY_Selected_item, MY_Selected_list );
	     }
	   }

	   void dclicked_item( GtkWidget *wdg, void *data )
	   { printf("Dclicked '%s', for %s.\n", MY_Selected_item, MY_Selected_list ); }

	  ... 
	  GtkTreeStore *selst;
	  const char *headings[]={ "T1", "D2", "P3" };
	  char *items[];
	  selst = make_multicolumn_selection_list( winpanel, 200, 30, 180, 150, 3, headings, 
                                         selected_item, dclicked_item, "chartA" );
	  items[0] = "Fries";  items[1] = "Shake";  items[2] = "Cola";
	  add_multicolumn_selection_item( selst, 3, iems );
	  items[0] = "Coffee";  items[1] = "Donut";  items[2] = "Beer";
	  add_multicolumn_selection_item( selst, 3, iems );
	  ...
	*/

/* ------------- End GTK Selection List Routines ----------------- */


/* ------------- Sub-Panel Frames ----------------- */

GtkWidget *make_tabbed_panel_frame( GtkWidget *panel, int xpos, int ypos )
{
 GtkWidget *bpanel, *tabbed_panel;
 tabbed_panel = gtk_notebook_new();
 gtk_notebook_set_tab_pos( GTK_NOTEBOOK(tabbed_panel), GTK_POS_TOP );
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 gtk_container_add( GTK_CONTAINER( bpanel ), tabbed_panel );
 return tabbed_panel;
}


GtkWidget *add_tabbed_panel( GtkWidget *tabbed_panel, int width, int height, char *tab_name )
{ /* Adds a tabbed-panel to a tabbed_panel_frame. */
 GtkWidget *bpanel, *label;
 bpanel = gtk_fixed_new();
 gtk_widget_set_size_request( bpanel, width, height );
 label = gtk_label_new( tab_name ); /* Tab-name. */
 gtk_notebook_append_page( GTK_NOTEBOOK(tabbed_panel), bpanel, label );
 return bpanel;
}


GtkWidget *make_framed_panel( GtkWidget *panel, int xpos, int ypos, int width, int height, char *frame_name )
{ /* Creates an outlined rectangular area for adding widgets inside it. */
 GtkWidget *bpanel, *frame;
 frame = gtk_frame_new( frame_name );
 // gtk_container_set_border_width( GTK_CONTAINER(frame), 10 );
 gtk_widget_set_size_request( frame, width, height );
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED(panel), bpanel, xpos, ypos );
 gtk_container_add( GTK_CONTAINER(bpanel), frame );
 return bpanel;
}


/* ------------- Sub-Window or (Detachable) Pop-up Window Making Routines ----------------- */

int  window_position_policy=GTK_WIN_POS_CENTER_ON_PARENT,
     top_window_position_policy=GTK_WIN_POS_CENTER;

void close_any_window( GtkWidget *widget, gpointer data )
{
 GtkWidget *(*win0) = data;
 if (data == 0) { printf("Close_Any_Window: Zero data\n"); return; }
 if (*win0 != 0) gtk_widget_destroy( *win0 );
 *win0 = 0;
}


int killed_any_window( GtkWidget *widget, GdkEvent *event, gpointer data )
{
 GtkWidget *(*win0) = data;
 *win0 = 0;
 return 0;      /* Returning "0" causes window to be destroyed. */
}


void place_window_center() { window_position_policy = GTK_WIN_POS_CENTER_ON_PARENT; }
void place_window_atmouse() { window_position_policy = GTK_WIN_POS_MOUSE; }


GtkWidget *make_window( int width, int height, const char *title, GtkWidget **winptr )
{	/* You must call "show_wind(winptr)" after creating all widgets under this window!! */
 GtkWidget *winframe;
 if (*winptr != 0) gtk_widget_destroy( *winptr );
 *winptr = gtk_window_new( GTK_WINDOW_TOPLEVEL );
 gtk_widget_set_size_request( *winptr, width, height );
 gtk_window_set_transient_for( GTK_WINDOW( *winptr ), GTK_WINDOW( outer_window ) );
 gtk_window_set_position( GTK_WINDOW( *winptr ), window_position_policy );	/* Other option(s): GTK_WIN_POS_CENTER, GTK_WIN_POS_MOUSE, GTK_WIN_POS_CENTER_ON_PARENT. */
 gtk_window_set_title( GTK_WINDOW( *winptr ), title );
 g_signal_connect( GTK_OBJECT( *winptr ), "delete-event", G_CALLBACK( killed_any_window ), (gpointer)winptr );
 winframe = gtk_fixed_new();
 gtk_container_add( GTK_CONTAINER( *winptr ), winframe );
 gtk_window_set_resizable( GTK_WINDOW( *winptr ), 0 );
 return winframe;
}


	/* New window and Register user's own killed callback.  It must return 0. */
GtkWidget *make_window_wkill( int width, int height, const char *title, GtkWidget *(*winptr), int callback(GtkWidget *, void *) )
{	/* You must call "show_wind(winptr)" after creating all widgets under this window!! */
 GtkWidget *winframe;
 if (*winptr != 0) gtk_widget_destroy( *winptr );
 *winptr = gtk_window_new( GTK_WINDOW_TOPLEVEL );
 gtk_widget_set_size_request( *winptr, width, height );
 gtk_window_set_transient_for( GTK_WINDOW( *winptr ), GTK_WINDOW( outer_window ) );
 gtk_window_set_position( GTK_WINDOW( *winptr ), window_position_policy );	/* Other option(s): GTK_WIN_POS_CENTER, GTK_WIN_POS_MOUSE, GTK_WIN_POS_CENTER_ON_PARENT. */
 gtk_window_set_title( GTK_WINDOW( *winptr ), title );
 gtk_signal_connect( GTK_OBJECT( *winptr ), "delete_event", GTK_SIGNAL_FUNC( callback ), winptr );
 winframe = gtk_fixed_new();
 gtk_container_add( GTK_CONTAINER( *winptr ), winframe );
 gtk_window_set_resizable( GTK_WINDOW( *winptr ), 0 );
 return winframe;
}

	/* Like above, but set horz/vert-scroll to 1 or 0, to enable or disable respective scroll. */
GtkWidget *make_scrolled_window_wkill( int width, int height, const char *title, GtkWidget *(*winptr), 
				      int horzscroll, int vertscroll, int callback(GtkWidget *, void *) )
{	/* You must call "show_wind(winptr)" after creating all widgets under this window!! */
 GtkWidget *winframe, *swin;
 if (*winptr != 0) gtk_widget_destroy( *winptr );
 *winptr = gtk_window_new( GTK_WINDOW_TOPLEVEL );
 gtk_widget_set_size_request( *winptr, width, height );
 gtk_window_set_transient_for( GTK_WINDOW( *winptr ), GTK_WINDOW( outer_window ) );
 gtk_window_set_position( GTK_WINDOW( *winptr ), window_position_policy );	/* Other option(s): GTK_WIN_POS_CENTER, GTK_WIN_POS_MOUSE, GTK_WIN_POS_CENTER_ON_PARENT. */
 gtk_window_set_title( GTK_WINDOW( *winptr ), title );
 if (callback != 0)
  gtk_signal_connect( GTK_OBJECT( *winptr ), "delete_event", GTK_SIGNAL_FUNC( callback ), winptr );
 else
  gtk_signal_connect( GTK_OBJECT( *winptr ), "delete_event", GTK_SIGNAL_FUNC( killed_any_window ), winptr );
 gtk_window_set_resizable( GTK_WINDOW( *winptr ), 0 );
 winframe = gtk_fixed_new();
 swin = gtk_scrolled_window_new( 0, 0 );
 if (horzscroll) horzscroll = GTK_POLICY_ALWAYS; else horzscroll = GTK_POLICY_NEVER;
 if (vertscroll) vertscroll = GTK_POLICY_ALWAYS; else vertscroll = GTK_POLICY_NEVER;
 gtk_scrolled_window_set_policy( (GtkScrolledWindow *)swin, horzscroll, vertscroll );
 gtk_scrolled_window_add_with_viewport( (GtkScrolledWindow *)swin, winframe );
 gtk_container_add( GTK_CONTAINER( *winptr ), swin );
 return winframe;
}


	 /* Initial call to set up outermost window. */
GtkWidget *init_top_outer_window( int *argc, char ***argv, int winwidth, int winhght, const char *title,
				  int horzscroll, int vertscroll )
{ /* Set horz/vert-scroll to 1 or 0, enable or discable respective scroll. */
 GtkWidget *outer_frame, *swin;
 gtk_init( argc, argv );
 outer_window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
 gtk_widget_set_size_request( outer_window, winwidth, winhght );
 gtk_window_set_title( GTK_WINDOW( outer_window ), title );
 gtk_window_set_position( GTK_WINDOW( outer_window ), top_window_position_policy );
 outer_frame = gtk_fixed_new();
 if (horzscroll || vertscroll)
  {
   if (horzscroll) horzscroll = GTK_POLICY_ALWAYS; else horzscroll = GTK_POLICY_NEVER;
   if (vertscroll) vertscroll = GTK_POLICY_ALWAYS; else vertscroll = GTK_POLICY_NEVER;
   swin = gtk_scrolled_window_new( 0, 0 );
   gtk_scrolled_window_set_policy( (GtkScrolledWindow *)swin, horzscroll, vertscroll );
   gtk_scrolled_window_add_with_viewport( (GtkScrolledWindow *)swin, outer_frame );
   gtk_container_add( GTK_CONTAINER( outer_window ), swin );
  }
 else
  gtk_container_add( GTK_CONTAINER( outer_window ), outer_frame ); 
 return outer_frame;
}


void show_wind( GtkWidget *wind )
{
 gtk_widget_show_all( wind );
}


     /****	---- Example window making function. ----
	GtkWidget *win1=0;

	void respond_to_some_event( GtkWidget *widget, gpointer data )
	{
	 GtkWidget *panel;

	 panel = make_window( 400, 150, "Test Win", &win1 );

	 ---- Place all window items here .... ----
	 make_button( panel, 20, 20, " OK ", close_any_window, &win1 );

	 gtk_widget_show_all( win1 );
	}

    ****/

/* ------------- Sub-Window or (Detachable) Pop-up Window Making Routines ----------------- */




/* ------------- GTK Separator Routines ----------------- */

GtkWidget *make_vseparator( GtkWidget *panel, int xpos, int y1, int y2 )
{
 GtkWidget *sep;
 sep = gtk_vseparator_new();
 gtk_fixed_put( GTK_FIXED( panel ), sep, xpos, y1 );
 gtk_widget_set_size_request( sep, 5, y2 - y1 );
 return sep;
}


GtkWidget *make_hseparator( GtkWidget *panel, int x1, int x2, int ypos )
{
 GtkWidget *sep;
 sep = gtk_hseparator_new();
 gtk_fixed_put( GTK_FIXED( panel ), sep, x1, ypos );
 gtk_widget_set_size_request( sep, x2 - x1, 5 );
 return sep;
}


GtkWidget *make_rectangular_separator( GtkWidget *panel, int x1, int y1, int x2, int y2 )
{
 GtkWidget *sep;
 sep = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), sep, 0, 0 );
 make_hseparator( sep, x1, x2, y1 );
 make_vseparator( sep, x1, y1, y2 );
 make_vseparator( sep, x2, y1, y2 );
 make_hseparator( sep, x1, x2, y2 );
 return sep;
}

/* ------------- End GTK Separator Routines ----------------- */


/*------------------------------------------------------------------------------
  attach_image_from_file - Reads image from jpg/gif file, and places on panel
   at specified position and size.  Returns 0 on error, else returns widget.
 ------------------------------------------------------------------------------*/
GtkWidget *attach_image_from_file( GtkWidget *panel, char *imgfname, int xpos, int ypos, int width, int height )
{
 GtkWidget *pic, *wdg;
 GdkPixbuf *pixbuf;
 pixbuf = gdk_pixbuf_new_from_file_at_scale( imgfname, width, height, 0, 0 );
 if (pixbuf == 0) { printf("Error reading image-file '%s'\n", imgfname );  return 0; }
 pic = gtk_image_new_from_pixbuf( pixbuf );
 wdg = gtk_fixed_new();					/* Position a widget. */
 gtk_fixed_put( GTK_FIXED( panel ), wdg, xpos, ypos );
 gtk_container_add( GTK_CONTAINER( wdg ), pic );	/* Attach image. */
 return wdg;
}


/*------------------------------------------------------------------------------*/
/* place_image - This routine displays an image on the named panel, at the 	*/
/*	specified (x,y) position.  Image_pixels =  image_width x image_height	*/
/*	Allocate and initialize image as:					*/
/*		imagedata = (unsigned char *)malloc( 4 * image_pixels );	*/
/*		imagedata[ 4 * (image_width * row + col) ]     = Blue;		*/
/*		imagedata[ 4 * (image_width * row + col) + 1 ] = Green;		*/
/*		imagedata[ 4 * (image_width * row + col) + 2 ] = Red;		*/
/*	Where values are 0=dark to 255=bright. 					*/
/*	You can de-allocate or re-use image array after calling this function.	*/
/*------------------------------------------------------------------------------*/
void place_image( GtkWidget *window, int image_width, int image_height, int pos_x, int pos_y, unsigned char *imagedata )
{
 int  stride;
 cairo_surface_t  *image;
 cairo_t *cr;

 cr = gdk_cairo_create( window->window );
 stride = cairo_format_stride_for_width( CAIRO_FORMAT_RGB24, image_width );
 /* stride should now equal (4 * image_width). */
  image = cairo_image_surface_create_for_data( imagedata, CAIRO_FORMAT_RGB24, image_width, image_height, stride );
  cairo_set_source_surface( cr, image, pos_x, pos_y );
  cairo_paint(cr);
  cairo_destroy(cr);
}


/* --- Convert Data to Image (cdti) Routines --- */
/* --- These routines retreive image-data from pre-initialized arrays stored within a program, for display. --- */
/*     (The pre-stored array text can be created from the "data2code.c" utility.)   */
unsigned char cdti_get_next_byte( char *data, int *indx )
{
 unsigned char ch, ch0, ch1;
 ch0 = data[ *indx ];   *indx = *indx + 1;
 if (ch0 > 60) ch0 = ch0 - 87;  else  ch0 = ch0 - 48;
 ch1 = data[ *indx ];   *indx = *indx + 1;
 if (ch1 > 60) ch1 = ch1 - 87;  else  ch1 = ch1 - 48;
 ch = (ch0 << 4) | ch1;
 return ch;
}

int cdti_read_binary_number( char *data, int *indx )
{
 int k=0;
 unsigned char ch;
 do
  {
   ch = cdti_get_next_byte( data, indx );
   if (ch == '#') while (cdti_get_next_byte( data, indx ) != '\n');
  }
 while (ch == '#'); 
 while ((ch >= '0') && (ch <= '9'))
  {
   k = (ch - '0') + 10 * k;
   ch = cdti_get_next_byte( data, indx );
  }
 return k;
}

unsigned char *cdti_convert_data_to_image( char *data, int dimension, int *imgwd, int *imght )
{ /* Assumes PPM (P6, raw) file. */
 unsigned char r, g, b, *imagedata;
 int indx=0, ht=0, wd=0, mm=0;

 /* P6<cr>#sdsa<cr>width height<cr>255<cr>binary-data   */
 /*    1        2               3      4                */

 if ((cdti_get_next_byte( data, &indx ) != 'P') || (cdti_get_next_byte( data, &indx ) != '6'))
  { printf("Error: Image data does not have expected header.\n");  return 0; }
 while (cdti_get_next_byte( data, &indx ) != '\n');
 wd = cdti_read_binary_number( data, &indx );
 ht = cdti_read_binary_number( data, &indx );
 // printf("Image %d x %d\n", wd, ht );
 cdti_read_binary_number( data, &indx );
 imagedata = (unsigned char *)malloc( 4 * wd * ht );
 while (indx < dimension)
  {
   r = cdti_get_next_byte( data, &indx );
   g = cdti_get_next_byte( data, &indx );
   b = cdti_get_next_byte( data, &indx );
   imagedata[ mm++ ] = b;
   imagedata[ mm++ ] = g;
   imagedata[ mm++ ] = r;
   mm++;
  }
 *imgwd = wd;
 *imght = ht;
 return imagedata;
}
/* --- End of cdti routines --- */


/* ------------- Tool Tips / Hover Notes ----------------- */

GtkTooltips *add_tool_tip( GtkWidget *wdg, const char *text )		/* Adds tool-tip to button, etc. */
{
 GtkTooltips *tt;
 tt = gtk_tooltips_new();
 gtk_tooltips_set_tip( tt, wdg, text, 0 );
 return tt;
}

/* ------------- File Browser ----------------- */

void canceled_file_browser( GtkWidget *wdg, void *fb ) { gtk_widget_destroy( fb ); }

char *file_browser_filter=0;

GtkWidget *file_browser_popup( const char *dir, const char *text, void callback(GtkWidget *, void *) )
{
 GtkWidget *fb;
 fb = gtk_file_selection_new( text );
 if (file_browser_filter != 0)
  gtk_file_selection_complete( (GtkFileSelection *)fb, file_browser_filter );
 gtk_file_selection_set_filename( GTK_FILE_SELECTION(fb), dir );
 gtk_signal_connect( GTK_OBJECT( GTK_FILE_SELECTION( fb )->ok_button ), "clicked", (GtkSignalFunc)callback, fb );
 gtk_signal_connect( GTK_OBJECT( GTK_FILE_SELECTION( fb )->cancel_button ), "clicked", (GtkSignalFunc)canceled_file_browser, fb );
 show_wind( fb );
 return fb;
}


	/* Example usage: 
	   void receive_filename( GtkWidget *wdg, void *fs )
	    {
	     char *yourfilename;
	     yourfilename = strdup( gtk_file_selection_get_filename( (GtkFileSelection *)fs ) );
	     gtk_widget_destroy( fs );
	     printf("Absolute path = '%s'\n", yourfilename );
	     free( yourfilename );
	    }

	   file_browser_popup( ".", "Select File", receive_filename );
	*/


/* ------------- General Stuff ----------------- */
int udchck_flag=0;

void UpdateCheck()   /* Process any pending window events. */
{
 if (udchck_flag) return;
 udchck_flag = 1;
 while (gtk_events_pending())
  {
   gtk_main_iteration_do( 0 );
  }
 udchck_flag = 0;
}


GTimer *mytimer=0;

double Report_Time()	/* Reports time in seconds, accurate to millisecs, for checking time differences. */
{
  if (mytimer==0) 
   {
    mytimer = g_timer_new();
    g_timer_start( mytimer );
    return 0.0;
   }
  else
   return g_timer_elapsed( mytimer, 0 );     // (double)GetTickCount() * 1.0e-3;
}


void Sleep_seconds( float dt_seconds )
{
 g_usleep( (int)(dt_seconds * 1e6) );
}


/* ------------- End General Stuff ----------------- */


/* --------------- Back-compatibility Stuff ------------ */

GtkEntry *new_formbox( GtkWidget *panel, int xpos, int ypos, int nchars_wide, const char *text, int maxlen, 
				 void callback(GtkWidget *, void *), void *data )
 { return make_formbox( panel, xpos, ypos, nchars_wide, text, maxlen, callback, data ); }

GtkEntry *new_formbox_bypix( GtkWidget *panel, int xpos, int ypos, int npix_wide, const char *text, int maxlen, 
                       void callback(GtkWidget *, void *), void *data )
 { return make_formbox_bypix( panel, xpos, ypos, npix_wide, text, maxlen, callback, data ); }

GtkTextView *new_text_edit_box( GtkWidget *panel, int xpos, int ypos, int width, int height, const char *text )
 { return make_text_edit_box( panel, xpos, ypos, width, height, text ); }

GtkTreeStore *new_selection_list( GtkWidget *panel, int xpos, int ypos, int width, int height, const char *column_titles, 
				  void callback(GtkWidget *, void *), void dclick_callback(GtkWidget *, void *), void *data )
 { return make_selection_list( panel, xpos, ypos, width, height, column_titles, callback, dclick_callback, data ); }

GtkWidget *new_window( int width, int height, const char *title, GtkWidget *(*winptr) )
 { return make_window( width, height, title, winptr ); }

GtkWidget *new_window_wkill( int width, int height, const char *title, GtkWidget *(*winptr), int callback(GtkWidget *, void *) )
 { return make_window_wkill( width, height, title, winptr, callback ); }

GtkWidget *new_scrolled_window_wkill( int width, int height, const char *title, GtkWidget *(*winptr), 
                                      int horzscroll, int vertscroll, int callback(GtkWidget *, void *) )
 { return make_scrolled_window_wkill( width, height, title, winptr, horzscroll, vertscroll, callback ); }

/* --------------- End Back-compatibility Stuff ------------ */
