/********************************************************/
/* OTS_GUI.c - OpenTaxSolver Graphical User Interface.	*/
/* Based on GimpToolKit (Gtk) widgets.			*/
/*							*/
/* This version is based on Gtk widgets.		*/
/*							*/
/* The OTS_GUI is designed to read-in standard OTS data	*/
/* files, and present the tax information on your 	*/
/* screen.  It allows entering the values, saving 	*/
/* return data, editing previously saved return sheets, */
/* and running the OTS tax-solver to compute your 	*/
/* taxes.						*/
/*							*/
/* The OTS tax-solver is a text program which can be	*/
/* used by itself. This GUI front-end simplifies using  */
/* OTS.  It walks you through the steps and invokes the	*/
/* regular OTS solver when you are ready.		*/
/*							*/
/* OTS data files, for example "tax_xx.txt", contain 	*/
/* the line numbers (or names) of the entries for a 	*/
/* given tax form, as well as any previously entered 	*/
/* values for each line.  Additional comments may 	*/
/* follow on any line.  				*/
/* A few example lines follow:				*/
/*							*/
/*	L15    	      ;  { Rental income }		*/
/*	L17    234.00 ;  { Savings interest }		*/
/*	L18     	 { Dividends }			*/
/*		23.00    {  Bank1 }			*/
/*		14.50 ;  {  Work loan }			*/
/*							*/
/* OTS_GUI reads these lines and places a label for 	*/
/* each line number/name, a text-box for filling in the	*/
/* value(s), and additional labels for the comments.	*/
/* At the bottom are placed buttons to save and 	*/
/* calculate-taxes.					*/
/* 							*/
/* To compile this graphical program, you will need the */
/* Gtk library.						*/
/*							*/
/* Compile:						*/
/*  cc -O `pkg-config --cflags gtk+-2.0` ots_gui2.c  \
 	`pkg-config --libs gtk+-2.0`  -o ots_gui2	*/
/*							*/
/********************************************************/

float version=2.50;
char package_date[]="January 15, 2022";
char ots_release_package[]="19.00";

/************************************************************/
/* Design Notes - 					    */
/*  Unlike the individual tax programs, which can know	    */
/*  what to expect on each line, this GUI does not know     */
/*  about the format of any particular tax form file.	    */
/*  Therefore, it applies some simple rules to parse	    */
/*  the fields:						    */
/*   1. The next non-comment / non-white-space character    */
/*	after a ";" is interpreted as a line-label.	    */
/*	(eg. L51)					    */
/*	However, some lines expecting single values were    */
/*	not terminated with ";", so other rules are needed. */
/*   2. Any line with a non-comment / non-white-space char  */
/*	in column 1 (first char on line), is assumed to be  */
/*	a line-label.  So all field values should be        */
/*	indented to use the GUI with them.		    */
/* Internal Architecture:
     Main
      -pick_file
       -receive_filename
	-open_taxfile
	 -Get_New_Tax_Form_Page
          -Get_Tax_Form_Page
	   -Read_Tax_File
	   -Setup_Tax_Form_Page - Creates the window for the form-page.
	     -DisplayTaxInfo - Renders the interactive form-page.
	      -check_comments
*/
/************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
// #include "backcompat.c"
#include "gtk_utils.c"		/* Include the graphics library. */
#include "gtk_file_browser.c"

 GtkWidget *mpanel, *mpanel2, *warnwin=0, *popupwin=0, *resultswindow=0, *scrolledpane, *title_label;
 int operating_mode=1, need_to_resize=0, debug=0;
 double last_resize_time;

int verbose=0;
int winwidth=450, winht=480;
FILE *infile;
int ots_column=0, ots_line=0;	/* Input file position. */
#define MaxFname 4096
char wildcards_bin[MaxFname]="", filename_exe[MaxFname]="", *ots_path;
char directory_dat[MaxFname+512]=".", wildcards_dat[MaxFname]="*.txt", filename_incl[MaxFname]="";
char directory_incl[MaxFname]="tax_form_files", wildcards_incl[MaxFname]="*_out.txt";
char directory_fb[MaxFname]="", wildcards_fb[MaxFname]="", filename_fb[MaxFname]="";
char *title_line="Tax File", *current_working_filename=0, *invocation_path, *include_file_name=0;
char wildcards_out[MaxFname]="*_out.txt";
char run_options[MaxFname]="";
int fronty1, fronty2, computed=0, ok_slcttxprog=1;
char *yourfilename=0;
char toolpath[MaxFname]="", *start_cmd;
int pending_compute=0, supported_pdf_form=1;
int filingstatus_mfj=1;
int round_to_whole_nums=0;

void pick_file( GtkWidget *wdg, void *data );	/* Prototype */
void consume_leading_trailing_whitespace( char *line );
void get_line_entry( char *word, int maxn, int *linenum, FILE *infile );
void Run_TaxSolver( GtkWidget *wdg, void *x );
void helpabout2( GtkWidget *wdg, void *data );
void read_instructions( int init );
void dump_taxinfo();
int warn_release=0;

int selected_other=0;
void save_taxfile( GtkWidget *wdg, void *data );
void printout( GtkWidget *wdg, void *data );
void taxsolve();
char *taxsolvecmd=0, taxsolvestrng[MaxFname]="";

#define VKIND_FLOAT   0
#define VKIND_INT     1
#define VKIND_TEXT    2
#define VKIND_COMMENT 3
#define VKIND_COLON   4

#define VALUE_LABEL   0
#define COMMENT       1
#define SEMICOLON     2
#define NOTHING	     10
#define ENABLED       1
#define DISABLED      0

#define LITERAL_INFO 1
#define ID_INFO 2

#define CAPGAIN_READY  ""		// "Ready"

char program_names[30][100] = 
	{
	 "taxsolve_US_1040_2021",		/* 0 */
	 "taxsolve_US_1040_Sched_C_2021",	/* 1 */
	 "taxsolve_US_8829",			/* 2 */
	 "taxsolve_CA_540_2021",		/* 3 */
	 "taxsolve_NC_D400_2021",		/* 4 */
	 "taxsolve_NJ_1040_2021",		/* 5 */
	 "taxsolve_OH_IT1040_2021",		/* 6 */
	 "taxsolve_PA_40_2021",			/* 7 */
	 "taxsolve_VA_760_2021",		/* 8 */
	 "taxsolve_NY_IT201_2021",		/* 9 */
	 "taxsolve_MA_1_2021",			/* 10 */
	 "taxsolve_GA_500",			/* 11 */
	 "Other",				/* xx */
	};

enum form_names { form_US_1040, form_US_1040_Sched_C, form_US_8829, form_CA_540, 
		  form_NC_D400, form_NJ_1040, form_OH_IT1040, form_PA_40,
		  form_VA_760, form_NY_IT201, form_MA_1, form_GA_500, 
		  form_other,
		  form_1040e, form_4562, form_8582
		};
int selected_form=form_other;


char *setform( int formnum )
{
 char twrd[500];
 sprintf( twrd, "%d", formnum );
 return strdup( twrd );
}


#if (PLATFORM_KIND != Posix_Platform) 
 char slashchr='\\';
 char slashstr[]="\\";
#else
 char slashchr='/';
 char slashstr[]="/";
#endif


void dismiss_general_warning( GtkWidget *wdg, void *data )
{
 switch (warn_release)
  {
   case 1:  pick_file( 0, 0 );
	    warn_release = 0;
	    close_any_window( 0, data );
	break;
   case 2:  save_taxfile(0,"1");	/* Re-open file-browser. */
	    warn_release = 0;
	    close_any_window( 0, data );
	    if (pending_compute == 1) pending_compute++;
	break;
   default:  close_any_window( 0, data );
  }
}


int dismiss_general_warning_ret0( GtkWidget *wdg, void *data )
{
 dismiss_general_warning( wdg, data );
 warnwin = 0;
 return 0;
}


char wmsg[4096], *pending_message=0;

void GeneralWarning( char *mesg )       /* Used for one-line warning messages. */
{
 int xpos=20, ypos=20, winwdth, winhght=100;
 GtkWidget *winframe;
 printf("%s\n", mesg);
 winwdth = 90 + strlen( mesg ) * 8;
 winframe = new_window( winwdth, winhght, "Warning Message", &warnwin );
 make_label( winframe, xpos, ypos, mesg );
 make_button( winframe, winwdth/2 - 30, winhght - 40, "  Ok  ", dismiss_general_warning, &warnwin );
 gtk_window_set_keep_above( (GtkWindow *)warnwin, 1 );
 show_wind( warnwin );
}


void GeneralPopup( char *title, char *mesg, int to_text_win )       /* Used for multi-line informational messages. */
{
 int xpos=20, ypos=10, winwdth, winhght, orig_winhght, j=0, k=0, maxcols=0, nlines=0;
 GtkWidget *winframe;
 if (to_text_win)
  printf("%s\n", mesg);
 while (mesg[k] != '\0')	/* Determine max line width and number of rows. */
  {
   if (j > maxcols) maxcols = j;
   if ((mesg[k] == '\n') || (mesg[k] == '\0'))
    {
     j = 0;
     nlines++;
    }
   else
    j++;
   k++;
  }
 winwdth = 50 + maxcols * 8;
 winhght = 70 + 18 * nlines + 5;
 orig_winhght = winhght;
 if (winhght < 500) 
  {
   if (winwdth <= 600)
    winframe = new_window( winwdth, winhght, title, &warnwin );
   else
    {
     winwdth = 600;
     winframe = new_scrolled_window_wkill( winwdth, winhght, title, &warnwin, 1, 0, dismiss_general_warning_ret0 );
    }
  }
 else
  {
   winhght = 500;
   if (winwdth <= 600)
    winframe = new_scrolled_window_wkill( winwdth, winhght, title, &warnwin, 0, 1, dismiss_general_warning_ret0 );
   else
    {
     winwdth = 600;
     winframe = new_scrolled_window_wkill( winwdth, winhght, title, &warnwin, 1, 1, dismiss_general_warning_ret0 );
    }
  }
 make_label( winframe, xpos, ypos, mesg );
 make_button( winframe, winwdth/2 - 30, orig_winhght - 38, "  Ok  ", dismiss_general_warning, &warnwin );
 gtk_window_set_keep_above( (GtkWindow *)warnwin, 1 );
 show_wind( warnwin );
}



struct value_list
 {
  int	    kind;	/* 0=float, 1=integer, 2=text, 3=comment. */
  float     value;
  char      *comment, *text;
  int       column, linenum, formtype;
  struct taxline_record *parent;
  GtkEntry  *box;
  GtkWidget *comment_label;
  struct value_list *nxt;
 };

struct taxline_record
 {
  char *linename;
  int linenum, vpos;
  struct value_list *values_hd, *values_tl;	/* Head and tail list pointers for a tax-line-entry. */
  struct instruct_rec *instructions;
  struct taxline_record *nxt;
 } *taxlines_hd=0, *taxlines_tl=0;		/* Head and tail list pointers for tax-form. */


 struct taxline_record * 
new_taxline( char *linename, int linenum )
{
 struct taxline_record *tmppt;

 tmppt = (struct taxline_record *)calloc( 1, sizeof(struct taxline_record) );
 tmppt->linename = strdup(linename);
 tmppt->linenum = linenum;
 if (taxlines_hd==0) taxlines_hd = tmppt;
 else taxlines_tl->nxt = tmppt;
 taxlines_tl = tmppt;
 return tmppt;
}


 struct value_list * 
new_list_item_value( int kind, struct taxline_record *txline, void *x, int column, int linenum )
{ 
 struct value_list *tmppt;

 tmppt = (struct value_list *)calloc( 1, sizeof(struct value_list) );
 tmppt->kind = kind;
 tmppt->text = 0;
 tmppt->comment = 0;
 tmppt->column = column;
 tmppt->linenum = linenum;
 tmppt->box = 0;
 switch (kind)
  {
   case VKIND_FLOAT:   tmppt->value = *(float *)x; break;
   case VKIND_INT:     tmppt->value = *(int *)x; break;
   case VKIND_TEXT:    tmppt->text = strdup( (char *)x ); break;
   case VKIND_COMMENT: tmppt->comment = strdup( (char *)x ); break;
  }
 tmppt->parent = txline;
 tmppt->nxt = 0;
 if (txline==0) {printf("ERROR1:  called add_value %d before any line.\n",kind); return tmppt;}
 if (txline->values_hd==0) txline->values_hd = tmppt;  else  txline->values_tl->nxt = tmppt;
 txline->values_tl = tmppt;
 return tmppt;
}


void pasteurize_entry( char *text )	/* Filter disallowed characters from user input. */
{
 int j=0;
 while (text[j] != '\0')
  {
   if (text[j] == ';') text[j] = ' ';
   j++;
  }
}


struct line_record
 {
  char *line;
  struct line_record *next;
 } *markup_commands_hd=0, *markup_commands_tl=0;

void add_markup_command( char *markup )
{
 struct line_record *new;
 new = (struct line_record *)calloc( 1, sizeof( struct line_record ) );
 new->line = strdup( markup );
 if (markup_commands_hd == 0)
  markup_commands_hd = new;
 else
  markup_commands_tl->next = new;
 markup_commands_tl = new;
}

void dump_any_markup_commands( FILE *outfile )
{
 struct line_record *old;
 while (markup_commands_hd)
  {
   if (verbose) printf("MARKup: %s\n", markup_commands_hd->line );
   fprintf(outfile,"%s\n", markup_commands_hd->line );
   old = markup_commands_hd;
   markup_commands_hd = markup_commands_hd->next;
   free( old->line );
   free( old );
  }
}


int intercept_any_pragmas( char *word )  /* Intercept any special command pragmas. */
{
 if (strncmp( word, "Round_to_Whole_Dollars", 21 ) == 0)     /* Intercept any mode-setting commands. */
  {
   printf("Setting Round_to_Whole_Dollars mode.\n");
   round_to_whole_nums = 1;
   return 1;
  }
 else
  return 0;
}



/*--------------------------------------------------------------*/
/* Get_Next_Entry - Reads next item from input file.		*/
/* Returns 0=VALUE_LABEL if reads data value or line-label. 	*/
/* Returns 1=COMMENT     if reads comment.			*/
/* Returns 2=SEMICOLON   if reads ';' entry-end character.	*/
/*								*/
/* Passes back the column and line number where the current     */
/* entry begins on the line in the input file.  		*/
/*--------------------------------------------------------------*/
int get_next_entry( char *word, int maxn, int *column, int *linenum, FILE *infile )
{
 int k=0;

 /* Get up to the next non-white-space character. */
 ots_line = *linenum;
 do 
  { 
   word[k] = getc(infile);
   if (word[k] == '\n') { ots_column = 0;  ots_line++; } else ots_column++;
  }
 while ((!feof(infile)) && ((word[k]==' ') || (word[k]=='\t') || (word[k]=='\n') || (word[k]=='\r')));
 *column = ots_column;
 *linenum = ots_line;

 if (feof(infile)) {word[0] = '\0'; return NOTHING;}
 if (word[k]=='{')
  { /*get_comment*/
    do 
     {
      word[k++] = getc(infile);
      if (word[k-1] == '\n') { ots_column = 0;  ots_line++; } else ots_column++;
     }
    while ((!feof(infile)) && (word[k-1]!='}') && (k<maxn));
    word[k-1] = '\0';
    if (k>=maxn) {printf("Error: Character buffer overflow detected.\n"); exit(1);}
    return COMMENT;
  } /*get_comment*/
 else
 if (word[k]=='"')
  { /*get_quoted_value*/
    k++;
    do 
     {
      word[k++] = getc(infile);
      if (word[k-1] == '\n') { ots_column = 0;  ots_line++; } else ots_column++;
     }
    while ((!feof(infile)) && (word[k-1]!='"') && (k<maxn));
    if (k>=maxn) {printf("Error: Character buffer overflow detected.\n"); exit(1);}
    word[k] = '\0';
    return VALUE_LABEL;
  } /*get_quoted_value*/
 else
  { /*get_value_or_linelabel*/
    k++;
    while ((!feof(infile)) && (word[k-1]!=' ') && (word[k-1]!='\t') && 
	   (word[k-1]!='\n') && (word[k-1]!='\r') && (word[k-1]!=';') && (k<maxn))
      { 
	word[k++] = getc(infile);
	if (word[k-1] == '\n') { ots_column = 0;  ots_line++; } else ots_column++;
      }
    if (k>=maxn) {printf("Error: Character buffer overflow detected.\n"); exit(1);}
    word[k] = '\0';
    if (strncasecmp( word, "MarkupPDF", 9 ) == 0)
     { /* Store any custom markup commands. */
       if (word[k-1] != '\n')
	{ /* Get the remainder of the line. */
	 do word[k++] = getc(infile); while ((!feof(infile)) && (word[k-1] != '\n'));
	}
       word[k-1] = '\0';
       ots_column = 0;
       ots_line++;
       add_markup_command( word );
       return NOTHING;
     }
    if (intercept_any_pragmas( word ))
     {
	return NOTHING;
     }
    if (word[k-1]==';')
     { 
      if (k==1) { word[1] = '\0';  return SEMICOLON; }
      else { ungetc(word[k-1], infile); word[k-1] = '\0';  return VALUE_LABEL; }
     }
    else { ungetc(word[k-1], infile);  word[k-1] = '\0';  return VALUE_LABEL; }
  } /*get_value_or_linelabel*/
}


/*--------------------------------------------------------------*/
/* Get_Line_Entry - Reads remainder of line from input file.	*/
/*--------------------------------------------------------------*/
void get_line_entry( char *word, int maxn, int *linenum, FILE *infile )
{
 int k=0;
 word[k] = getc(infile);
 while ((!feof(infile)) && (word[k] != '\n') && (word[k] != '{'))
  {
   if (word[k] == '{')
    {
     do word[k] = getc(infile); while ((!feof(infile)) && (word[k] != '}'));
     if (word[k] == '}') word[k] = getc(infile);
    }
   else
    {
     k++;
     if (k > maxn)
      { 
	word[k-1] = '\0';  
	while ((!feof(infile)) && (getc(infile) != '\n'));  
	consume_leading_trailing_whitespace( word );
	return;
      }
     // printf("	get_line_entry = '%c'\n", word[k-1] );
     word[k] = getc(infile);
    }
  }
 if (word[k] == '{')
  ungetc( word[k], infile );
 else
  *linenum = *linenum + 1;
 word[k] = '\0';
 // printf("	k = %d, word[%d] = %d\n", k, k, word[k] );
 // printf("	word = '%s'\n", word );
 consume_leading_trailing_whitespace( word );
}


void DisplayTaxInfo();		/* This is a prototype statement only. */
void warn_about_save_needed_switch();
int save_needed=0;
int compute_needed=0;



int Update_box_info()	/* Capture entries from form-boxes. */
{
 struct taxline_record *txline;
 struct value_list *tmppt;
 char text[1024];

 txline = taxlines_hd;
 while (txline!=0)
  {
   tmppt = txline->values_hd;
   while (tmppt!=0)
    {
     if (tmppt->box != 0)
      {
       get_formbox_text( tmppt->box, text, 1024 );
       pasteurize_entry( text );
       tmppt->kind = VKIND_TEXT;
       if (strcmp( tmppt->text, text ) != 0) { save_needed++;  compute_needed = 1; }
       tmppt->text = strdup( text );
      }
     tmppt = tmppt->nxt;
    }
   txline = txline->nxt;
  }
 return save_needed;
}


void refresh()
{
 gtk_widget_destroy( mpanel2 );		/* Clear out panel. */
 /* Set up new panel with scrollbars for tax form data. */
 mpanel2 = gtk_fixed_new();
 scrolledpane = gtk_scrolled_window_new( 0, 0 );
 gtk_scrolled_window_set_policy( (GtkScrolledWindow *)scrolledpane, GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS );
 gtk_scrolled_window_add_with_viewport( (GtkScrolledWindow *)scrolledpane, mpanel2 );
 //   gtk_container_add( GTK_CONTAINER( mpanel ), scrolledpane );
 gtk_fixed_put( GTK_FIXED( mpanel ), scrolledpane, 0, 35 );
 gtk_widget_set_size_request( scrolledpane, winwidth, winht - 80 );
 DisplayTaxInfo();
 gtk_widget_show_all( outer_window );
}



void re_display_form()
{
 double vpos;
 GtkAdjustment *adj;

 Update_box_info();	/* Grab any new entries. */

 /* Prestore the scrolling position. */
 adj = gtk_scrolled_window_get_vadjustment( (GtkScrolledWindow *)scrolledpane );
 vpos = gtk_adjustment_get_value( adj );

 refresh();

 /* Restore the scrolling position. */
 adj = gtk_scrolled_window_get_vadjustment( (GtkScrolledWindow *)scrolledpane );
 adj->value = vpos;
}



/*--------------------------------------------------------------*/
/* Add_New_Boxes - Callback for "+" button on form-boxes.	*/
/*  Adds new form-box(s) to the line item.			*/
/*--------------------------------------------------------------*/
void add_new_boxes( void *data, int num )
{
 struct taxline_record *txline;
 struct value_list *item, *lineitem, *newitem1, *newitemlast, *oldtail;
 double vpos;
 GtkAdjustment *adj;

 Update_box_info();

 item = (struct value_list *)data;
 oldtail = item->parent->values_tl;

  if (num==2)
  {
   newitem1 = new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 1 );
   new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 1 );
   new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 2 );
   newitemlast = new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 2 );
  }
 else if (num==3)  /* CapGain only */
  {
   newitem1 = new_list_item_value( VKIND_TEXT, item->parent, CAPGAIN_READY, 0, item->linenum + 1 );
   new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 1 );
   new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 2 );
   new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 2 );
   new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 3 );
   newitemlast = new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 3 );
  } 
 else 
 {
  newitem1 = new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 1 );
  newitemlast = newitem1; 
 }

 lineitem = item;	 /* Skip the items on the original line. */
 while ((lineitem->nxt!=newitem1) && (lineitem->nxt->linenum == item->linenum)) lineitem = lineitem->nxt;

 /* Insert in list and remove from tail, if not on end of list. */
 if (lineitem->nxt != newitem1)
  {
   newitemlast->nxt = lineitem->nxt;
   lineitem->nxt = newitem1;
   oldtail->nxt = 0;  
   item->parent->values_tl = oldtail;   
  }

 /* Increment the effective file-line-number of all subsequent line entries. */
 lineitem = newitemlast->nxt;
 while (lineitem!=0)		/* Now do remaining lines of this item. */
  {
   lineitem->linenum = lineitem->linenum + num;
   lineitem = lineitem->nxt;
  }
 txline = item->parent->nxt;	/* Next do remaining line items. */
 while (txline!=0)
  {
   txline->linenum = txline->linenum + num;
   lineitem = txline->values_hd;
   while (lineitem!=0)
    {
     lineitem->linenum = lineitem->linenum + num;
     lineitem = lineitem->nxt;
    }
   txline = txline->nxt;
  }

 /* Prestore the scrolling position. */
 adj = gtk_scrolled_window_get_vadjustment( (GtkScrolledWindow *)scrolledpane );
 // printf("Adj = (%x), %g, (%g, %g), (%g, %g), %g\n", adj, adj->value, adj->lower, adj->upper, adj->step_increment, adj->page_increment, adj->page_size );
 vpos = gtk_adjustment_get_value( adj );
 // vpos = adj->value;

 refresh();

 /* Restore the scrolling position. */
 adj = gtk_scrolled_window_get_vadjustment( (GtkScrolledWindow *)scrolledpane );
 // gtk_adjustment_set_value( adj, vpos );	/* Isn't working because at this time upper and lower are 0 and 1. */
 adj->value = vpos;
 // gtk_scrolled_window_set_vadjustment( (GtkScrolledWindow *)scrolledpane, adj );  	/* Not needed. */
}


/*-------------------------------------------------------------------------*/
/* Add_new_capgain_boxes - Callback for "+" button on cap-gain form-boxes. */
/*  Adds a new form-boxes to the line item.				   */
/*-------------------------------------------------------------------------*/
void add_new_capgain_boxes( GtkWidget *wdg, void *data )
{
 add_new_boxes( data, 3 ); /* 3 lines of two boxes per line */
}

void add_new_box_item( GtkWidget *wdg, void *data )
{
 add_new_boxes( data, 1 );
}


GtkEntry *commentbox;

void cancelpopup( GtkWidget *wdg, void *data )
{ gtk_widget_destroy( popupwin );  popupwin = 0; }


void acceptcomment( GtkWidget *wdg, void *data )
{
 char *comment;
 struct value_list *tmppt;

 tmppt = (struct value_list *)data;
 comment = get_formbox( commentbox );
 // printf("Prior comment was '%s', new comment is '%s'\n", tmppt->comment, comment );
 if (tmppt->comment != 0) 
  {
   if (strcmp( tmppt->comment, comment ) != 0) { save_needed++;  compute_needed = 1; }
   free( tmppt->comment );
  } else { save_needed++;  compute_needed = 1; }
 tmppt->comment = strdup( comment );
 modify_label( tmppt->comment_label, comment );
 cancelpopup(0,0);
 // refresh();
}


void edit_line_comment( GtkWidget *wdg, void *data )	/* Edit_comment. */
{
 struct value_list *tmppt;
 int winwidth=510, winht=110;
 GtkWidget *panel;

 tmppt = (struct value_list *)data;
 if (popupwin) gtk_widget_destroy( popupwin );
 panel = new_window( winwidth, winht, "Edit Comment", &popupwin );
 make_label( panel, 2, 2, "Edit Line Comment:" );
 commentbox = new_formbox_bypix( panel, 10, 25, winwidth - 40, tmppt->comment, 500, acceptcomment, tmppt );
 make_button( panel, 20, winht - 30, " Ok ", acceptcomment, tmppt );
 make_button( panel, winwidth - 60, winht - 30, "Cancel", cancelpopup, 0 );
 gtk_widget_show_all( popupwin );
}


void accept_capgain_reset(GtkWidget *wdg, void *data)
{
 struct value_list *tmppt;

 tmppt = (struct value_list *)data;
 int valflg = 0, comcnt = 0;  
 char tmpstr[256];

  while (comcnt < 3)
 {
  switch (tmppt->kind)
   {
    case VKIND_TEXT:  
         if (valflg == 0)  /* Set 1st box  'Buy Cost' to "Ready" */
         {
          strcpy (tmpstr, CAPGAIN_READY);
          valflg = 1;
         }
         else strcpy (tmpstr, "");
         modify_formbox( tmppt->box, tmpstr);
         break;
    case VKIND_COMMENT:
         if (tmppt->comment != 0)free( tmppt->comment );
         tmppt->comment = strdup("");
         comcnt++;
         break;  
    default:
         break; 
   }
  tmppt = tmppt->nxt; 
 }
 cancelpopup(0,0);
 re_display_form();
}


void verify_capgain_reset(GtkWidget *wdg, void *data)  /* Code follows edit_line_comment as a framework */
{
 struct value_list *tmppt;
 int winwidth=220, winht=60;
 GtkWidget *panel; 

 tmppt = (struct value_list *)data; 
 if (popupwin) gtk_widget_destroy( popupwin );
 panel = new_window( winwidth, winht, "Clear Gain/Loss values ?", &popupwin );
 make_label( panel, 5, 1, "Clear gain/loss values ?" );
 make_button( panel, 20, winht - 35, " Ok ", accept_capgain_reset, tmppt );
 make_button( panel, winwidth - 60, winht - 35, "Cancel", cancelpopup, 0 );
 gtk_widget_show_all( popupwin );
}


#ifndef PLATFORM_KIND
 #define Posix_Platform  0 
 #define Mingw_Platform  1
 #define MsVisC_Platform 2
 #ifdef __CYGWIN32__
  #ifndef __CYGWIN__
   #define __CYGWIN__ __CYGWIN32__
  #endif
 #endif
 #if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MSYS__)
  #define PLATFORM_KIND Mingw_Platform /* MinGW or like platform */
 #elif defined(__WIN32) || defined(WIN32)
  #define PLATFORM_KIND MsVisC_Platform /* microsoft visual C */
 #else
  #define PLATFORM_KIND Posix_Platform    /* Posix/Linux/Unix */
 #endif
#endif


void switch_form( GtkWidget *wdg, void *data )
{ char *cmd;
 Update_box_info();
 if (save_needed)
  {
   warn_about_save_needed_switch();
   return;
  }
 cmd = (char *)malloc( strlen(start_cmd) + 100 );
 #if (PLATFORM_KIND==Posix_Platform)
   strcpy( cmd, start_cmd );
   strcat( cmd, " &" );
 #else
   strcpy( cmd, "start " );
   strcat( cmd, start_cmd  );
 #endif
  printf("Issuing: '%s'\n", cmd );
  system( cmd );
  exit(0);
 }


void switch_anyway( GtkWidget *wdg, void *data )
{
 save_needed = 0;
 switch_form( 0, 0 );
}

void warn_about_save_needed_switch()
{
 int xpos=20, ypos=20, winwdth, winhght=100;
 GtkWidget *winframe, *label;
 winwdth = 300;
 winframe = new_window( winwdth, winhght, "Warning Message", &warnwin );
 label = make_sized_label( winframe, xpos, ypos, "<b>Change(s) not saved !!</b>", 12 );
 set_widget_color( label, "#ff0000" );
 make_button( winframe, 10, winhght - 40, "Switch anyway, without saving", switch_anyway, &warnwin );
 make_button( winframe, winwdth - 60, winhght - 40, "Cancel", dismiss_general_warning, &warnwin );
 gtk_window_set_keep_above( (GtkWindow *)warnwin, 1 );
 show_wind( warnwin );
}


void quit_wcheck( GtkWidget *wdg, void *x );		/* Prototypes */
void print_outfile_directly( GtkWidget *wdg, void *data );
void create_pdf_file_directly( GtkWidget *wdg, void *data );
void set_pdfviewer( GtkWidget *wdg, void *data );


/* ----------------- Tax Instructions Helper -------------------- */

struct instruct_rec
 {
   char *instr_label, *instr_text;
   int buflen;
   struct instruct_rec *nxt;
 } *instruct_lst=0;

char *instructions_filename=0;


struct instruct_rec *new_instruction( char *line_label, char *buf, int *buflen )
{
 struct instruct_rec *new;
 //if (instruct_lst != 0)
 //printf("	Instr: [%s] '%s'\n", instruct_lst->instr_label, buf );
 if (instruct_lst != 0)
  instruct_lst->instr_text = strdup( buf );	/* Store prior instruction text. */
 buf[0] = '\0';					/* Clear buffer for new text. */
 *buflen = 0;
 new = (struct instruct_rec *)calloc( 1, sizeof( struct instruct_rec ) );
 new->instr_label = strdup( line_label );
 new->nxt = instruct_lst;
 instruct_lst = new;
 return new;
}


void strcat_safe( char *dst, const char *src, int maxlen, int *buflen )
{ 
  int j, k=0, oneless;
//printf("Adding: '%s' + '%s' = ", dst, src );
  oneless = maxlen - 1;
  j = *buflen;
  while ((j < oneless) && (src[k] != '\0')) { dst[j++] = src[k++]; }
  dst[j] = '\0';
  *buflen = j;
//printf("'%s'\n", dst );
}


int found_char( char ch, char *srch )
{
 int j=0;
 while ((ch != srch[j]) && (srch[j] != '\0'))
  j++;
 if (srch[j] != '\0')
  return 1;
 else
  return 0;
}


void advance_word( char *tstr, char delim, char *srch )
{
 int j=0, k=0;
 while ((tstr[j] != '\0') && (tstr[j] != delim))
  j++;
 if (tstr[j] != '\0')
  j++;
 while ((tstr[j] != '\0') && (found_char( tstr[j], srch )))
  j++;
 do
  tstr[k++] = tstr[j++];
 while (tstr[k-1] != '\0');
}


void dispose_instuctions()
{
 struct instruct_rec *old;
 while (instruct_lst != 0)
  {
   old = instruct_lst;
   instruct_lst = instruct_lst->nxt;
   free( old->instr_label );
   free( old->instr_text );
   free( old );
  }
}


void attach_instruction2line( struct instruct_rec *newinstr )
{
 struct taxline_record *txline;

 txline = taxlines_hd;
 while (txline!=0)
  {
   if (strcmp( txline->linename, newinstr->instr_label ) == 0)
    {
     txline->instructions = newinstr;
     return;
    }
   txline = txline->nxt;
  }
}


void read_instructions( int init )
{ char *tmpinstrfname, *tline, *tstr, *twrd, *buf;
  int maxstr1=16384, maxstr2=32768, buflen=0;
  struct instruct_rec *newinstr;
  FILE *instrfile;
  /**
    Instruction files must have the following form for any tax-lines with instructions:
	[line-lable] instruction text
    Example:
	[L17] Report here your interest income.
	      More instruction text ...
	[L20] This should come from your 1099-G box 2a.
	      Etc., etc..
    The instructions can span multiple lines, as shown, until the next bracket line.
    The label bracket, "[", must be the first character on the line.
    Do not have tabs or line-feeds in text.  Only printable ascii characters!!!
   **/
  if (init == 1)
   {
    dispose_instuctions();
    switch (selected_form)
     {
      case form_US_1040:
	instructions_filename = strdup( "f1040_instructions.dat" );	break;
      case form_US_1040_Sched_C:
	instructions_filename = strdup( "f1040sc_instructions.dat" );	break;
      case form_PA_40:
	instructions_filename = strdup( "PA_instructions.dat" );	break;
      case form_CA_540:
	instructions_filename = strdup( "CA_540_instructions.dat" );	break;
      case form_OH_IT1040:
	instructions_filename = strdup( "OH_PIT_IT1040_instructions.dat" );	break;
      case form_VA_760:
	instructions_filename = strdup( "VA_760_instructions.dat" );	break;
      case form_NJ_1040:
	instructions_filename = strdup( "NJ_1040_instructions.dat" );	break;
      case form_NY_IT201:
	instructions_filename = strdup( "NY_it201_instructions.dat" );	break;
      case form_MA_1:
	instructions_filename = strdup( "MA_1_instructions.dat" );	break;
      case form_NC_D400:
	instructions_filename = strdup( "NC_instructions.dat" );	break;
      default:
	if (strstr( taxsolvestrng, "taxsolve_HSA_f8889" ) != 0)
	 instructions_filename = strdup( "f8889_instructions.dat" );
	else
	if (strstr( taxsolvestrng, "taxsolve_US_1040_Sched_SE" ) != 0)
	 instructions_filename = strdup( "f1040sse_instructions.dat" );
	else
	if (strstr( taxsolvestrng, "taxsolve_f8959" ) != 0)
	 instructions_filename = strdup( "f8959_instructions.dat" );
	else
	if (strstr( taxsolvestrng, "taxsolve_f8960" ) != 0)
	 instructions_filename = strdup( "f8961_instructions.dat" );
	else
	if (strstr( taxsolvestrng, "taxsolve_f2210" ) != 0)
	 instructions_filename = strdup( "f2210_instructions.dat" );
	else
	if (strstr( taxsolvestrng, "taxsolve_CA_5805" ) != 0)
	 instructions_filename = strdup( "CA_5805_instructions.dat" );	 
	else
	 return;	
     }
    if (verbose) printf("Instruction file = '%s'\n", instructions_filename );
   }
  tmpinstrfname = (char *)malloc(8192);
  tstr = (char *)malloc( 1024 );
  twrd = (char *)malloc( 1024 );
  tline = (char *)malloc( maxstr1 + 10 );
  buf = (char *)calloc( 1, maxstr2 + 10 );
  strcpy( tmpinstrfname, ots_path );
  strcat( tmpinstrfname, "src" );  strcat( tmpinstrfname, slashstr );  
  strcat( tmpinstrfname, "formdata" ); strcat( tmpinstrfname, slashstr );
  strcat( tmpinstrfname, instructions_filename );
  if (verbose) printf("Opening: '%s'\n", tmpinstrfname );

  instrfile = fopen( tmpinstrfname, "rb" );
  if (instrfile == 0)
   {
    if (verbose) printf("Could not open instructions file: '%s'\n", tmpinstrfname );
    return;
   }

  fgets( tline, maxstr1, instrfile );
  // printf("%s", tline );
  while (!feof(instrfile))
   {
    // printf("%s", tline );
    if (tline[0] == '[')
     {
      strcpy_safe( tstr, tline, 256 );
      fb_next_word( tstr, twrd, "[] \t\n\r" );
      // printf("Label: '%s'\n", twrd );
      newinstr = new_instruction( twrd, buf, &buflen );
      attach_instruction2line( newinstr );
      advance_word( tline, ']', " \t" );
     }
    strcat_safe( buf, tline, maxstr2, &buflen );
    fgets( tline, maxstr1, instrfile );
   }
  new_instruction( "_END_", buf, &buflen );
  fclose( instrfile );
  free( tmpinstrfname );
  free( tstr );
  free( twrd );
}



int mouse_clicked( GtkWidget *widget, GdkEventButton *event, gpointer data )
{
 int xpos, vpos, mindist=9999999;
 GtkAdjustment *adj;
 struct taxline_record *txline, *closest_line=0;

 if (warnwin)
  { /* Remove any previously open text window. */
   gtk_widget_destroy( warnwin );
   warnwin = 0;
  }
 xpos = (int)(event->x);
 if (((xpos > 100) && (xpos < 245)) || (xpos > 710))
  return 1;	/* Too close to buttons or text-forms of a tax-line, so return from here. */

 /* Find the closest tax-line to where clicked. */
 // printf("Mouse button %d Pressed at: %d, %d, %g\n", (int)(event->button), (int)(event->x), (int)(event->y), (double)(event->time) );
 adj = gtk_scrolled_window_get_vadjustment( (GtkScrolledWindow *)scrolledpane );
 vpos = (int)(event->y + gtk_adjustment_get_value( adj ));
 txline = taxlines_hd;
 while (txline!=0)
  {
   if (abs( vpos - txline->vpos ) < mindist)
    {
	mindist = abs( vpos - txline->vpos );
	closest_line = txline;
    } 
   txline = txline->nxt;
  }
 if ((closest_line != 0) && (vpos - closest_line->vpos > -30))
  { /* Pop up instruction-text window, if clicked near enough, and/or below, a tax-line with instructions. */
   // printf("Picked '%s', dist = %d\n", closest_line->linename, mindist );
   if (closest_line->instructions)
    GeneralPopup( closest_line->instructions->instr_label, closest_line->instructions->instr_text, 0 );
  }
 return 1;      /* Stops other handlers from being invoked. */
}


int mouse_unclicked( GtkWidget *widget, GdkEventButton *event, gpointer data )
{
 printf("Mouse button %d Released at: %d, %d\n", (int)(event->button), (int)(event->x), (int)(event->y) );
 return 1;      /* Stops other handlers from being invoked. */
}


/* ----------------- End Tax Instructions Helper -------------------- */



/* Check if entry looks like a date. */
/* If so, return 1, else return 0.  */
int datecheck( char *word )
{
 int j, k=0;

 j = strlen(word) - 1;
 while (j>0)
  {
   if ((word[j]=='-') || (word[j]=='/')) k++;
   else if (word[j]>'9') return 0;
   j--;
  }
 if (k==2) return 1; else return 0;
}


char *taxform_name;


/***********************/
/* Read Tax Data File. */
/***********************/
void Read_Tax_File( char *fname )
{
 int 	j, k, kind, state=0, column=0, 
	linenum=0, 	/* Line number in input file. */
	linecnt=0, 	/* Line number of gui display. */
	lastline=0, newentry=0, entrycnt=0;
 int lastlinenum=-1;
 char word[15000], *tmpstr, tmpstr2[900], tmpstr3[900];
 struct taxline_record *txline=0;
 struct value_list *tmppt, *newitem, *oldtail;

 /* Read the Tax Data Form File. */
 current_working_filename = strdup(fname);
 taxlines_hd = 0;
 /* Accept the form's Title line.  (Must be first line!) */
 fgets(word, 200, infile);
 title_line = strdup( word );
 j = strlen(word);
 if (j>0) word[j-1] = '\0';
 // printf("Title: '%s'\n", word);
 if (strstr(word,"Title:")==word) tmpstr = &(word[6]); else tmpstr = &(word[0]);
 k = strlen(tmpstr);	/* Pad to center if title is too short. */
 if (k < 20)
  { for (j=0; j<(20-k)/2; j++) tmpstr2[k]=' '; tmpstr2[(20-k)/2] = '\0'; 
    strcpy(tmpstr3,tmpstr2); strcat(tmpstr3,tmpstr); strcpy(tmpstr,tmpstr3); strcat(tmpstr,tmpstr2);
  }
 taxform_name = strdup( tmpstr );

 kind = get_next_entry( word, 10000, &column, &linenum, infile );
 if (linenum > lastline) { lastline = linenum;  if (newentry) linecnt++;  newentry = 0; }
 while (!feof(infile))
  { /*Loop1*/
   if (column == 1) state = 0;
   if (verbose) printf("Kind=%d: state=%d: col=%d: lnum=%d:  '%s'\n", kind, state, column, linenum, word);
   switch (kind)
    {
     case VALUE_LABEL: 
	 if (state==0) 
	  { /*statezero*/
	   if (verbose) printf(" LineLabel:	'%s'\n", word);
	   state = 1;
	   entrycnt = 0;
	   txline = new_taxline( word, linecnt );
	   if ((strcasecmp(txline->linename, "Your1stName:") == 0) || (strcasecmp(txline->linename, "YourName:") == 0) ||
               (strcasecmp(txline->linename, "YourLastName:") == 0) || (strcasecmp(txline->linename, "YourSocSec#:") == 0) ||
               (strcasecmp(txline->linename, "Spouse1stName:") == 0) || (strcasecmp(txline->linename, "SpouseLastName:") == 0) ||
	       (strcasecmp(txline->linename, "YourInitial:") == 0) || (strcasecmp(txline->linename, "SpouseInitial:") == 0) ||
               (strcasecmp(txline->linename, "SpouseSocSec#:") == 0) || (strcasecmp(txline->linename, "Number&Street:") == 0) ||
               (strcmp(txline->linename, "YourBirthDate:") == 0) || (strcmp(txline->linename, "SpouseBirthDate:") == 0) ||
               (strcmp(txline->linename, "Apt#:") == 0) || (strcmp(txline->linename, "TownStateZip:") == 0) ||
		(strcmp(txline->linename, "Town:") == 0) || (strcmp(txline->linename, "State:") == 0) ||
		(strcmp(txline->linename, "Zipcode:") == 0) || (strstr( txline->linename, ":" ) != 0) ||
		(strcmp(txline->linename, "PrincipalBus:") == 0) || (strcmp(txline->linename, "BusinessName:") == 0))
	    {
	     if (ots_column > 0)
	      { get_line_entry( word, 10000, &linenum, infile ); }
	     else
	      word[0] = '\0';
	     tmppt = new_list_item_value( VKIND_TEXT, txline, word, column, linecnt );
	     tmppt->formtype = ID_INFO;	/* Special ID-only info lines. */
	     state = 0;
	    }
	  } /*statezero*/
	 else
	  { /*stateNotzero*/
	   if (verbose) printf(" Value:	%s\n", word);
	   if (strcasecmp(txline->linename, "Status") == 0)
	    {
	     new_list_item_value( VKIND_TEXT, txline, word, column, linecnt );
	     state = 0;
	    }
	   else
	    { /*Accept normal value. */
	     tmppt = new_list_item_value( VKIND_TEXT, txline, word, column, linecnt );
	     if (strstr( txline->linename, ":" ) != 0)
		 tmppt->formtype = LITERAL_INFO;
	     entrycnt++;
	    }
	  } /*stateNotzero*/
	newentry++;
	break;
     case COMMENT: if (verbose) printf(" Comment:	%s\n", word);
	if ((txline==0) || ((strncasecmp(txline->linename, "CapGains",7) != 0) && (lastlinenum > 0) && (linenum > lastlinenum)))
	 txline = new_taxline("", linecnt);
	new_list_item_value( VKIND_COMMENT, txline, word, column, linecnt );
	newentry++;
	break;
     case SEMICOLON: if (verbose) printf(" End:	%s\n", word);
	  /* When line is labeled "CapGains", and there are no entries,	  */
	  /* then produce extra boxes for date bought or sold.	  */
	  /* So far, this is only known to be needed on US-Fed form. */
	if ((txline != 0) && ((strncasecmp(txline->linename, "Cap-Gains",8) == 0) ||
	    (strncasecmp(txline->linename, "CapGains",7) == 0)) && (entrycnt < 2))
	 {
	  new_list_item_value( VKIND_TEXT, txline, CAPGAIN_READY, column, linecnt );
	  new_list_item_value( VKIND_TEXT, txline, "", column, linecnt++ );
	  new_list_item_value( VKIND_TEXT, txline, "", column, linecnt );
	  new_list_item_value( VKIND_TEXT, txline, "", column, linecnt++ );
	  new_list_item_value( VKIND_TEXT, txline, "", column, linecnt );
	  new_list_item_value( VKIND_TEXT, txline, "", column, linecnt++ );
	 }
	state = 0;
	new_list_item_value( VKIND_COLON, txline, word, column, linecnt );
	lastlinenum = linenum;
	break;
    }
   column = column + strlen(word);
   lastlinenum = linenum;
   kind = get_next_entry( word, 10000, &column, &linenum, infile );
   if (linenum > lastline) 
    { 
     if ((txline!=0) && (strncasecmp(txline->linename, "CapGains",7) == 0))
      {
	if ((entrycnt % 6) == 0)
        linecnt++;
      }
     lastline = linenum;
     linecnt++;  
     newentry = 0;
    }
  } /*Loop1*/
 fclose(infile);

 /* Check for missing entries. */
 txline = taxlines_hd;
 while (txline!=0)
  {
   tmppt = txline->values_hd;  state = 0;
   while (tmppt!=0)
    {
     if ((tmppt->kind==VKIND_FLOAT) || (tmppt->kind==VKIND_TEXT) || (tmppt->kind==VKIND_INT)) state = 1;
     tmppt = tmppt->nxt;
    }
   if ((state==0) && (strlen(txline->linename)>0))	/* Place empty formbox on any line having no entries. */
    {
      oldtail = txline->values_tl;
      newitem = new_list_item_value( VKIND_TEXT, txline, "", 0, txline->linenum );
      if (newitem!=txline->values_hd)
       {
	newitem->nxt = txline->values_hd;
	txline->values_hd = newitem;
	txline->values_tl = oldtail;
	oldtail->nxt = 0;
       }
    }
   txline = txline->nxt;
  }
  //dump_taxinfo();
}



GtkWidget *options_window=0, *allforms_button;
int allforms_toggle=0;
double winopentime;


void set_pdf_option( GtkWidget *wdg, void *data )
{
 if (Report_Time() - winopentime < 0.2) return;
 allforms_toggle = !allforms_toggle;
 printf("Allforms = %d\n", allforms_toggle );
}

void set_r2wn_option( GtkWidget *wdg, void *data )
{
 // printf("dT = %g\n", Report_Time() - winopentime );
 if (Report_Time() - winopentime < 0.2) return;
 round_to_whole_nums = !round_to_whole_nums;
 printf("Round_to_Whole_Nums = %d\n", round_to_whole_nums );
 save_needed++;  compute_needed = 1; 
}

void options_pdf_diaglog( GtkWidget *wdg, void *data )
{
 GtkWidget *panel;
 int wd=400, ht=170, xpos=10, ypos=30;
 panel = new_window( wd, ht, "Options Menu", &options_window );
 make_sized_label( panel, 5, 1, "Options Menu:", 12 );
 winopentime = Report_Time();
 allforms_button = make_toggle_button( panel, xpos, ypos, "Force production of All PDF Form Pages", allforms_toggle, set_pdf_option, "allforms" );
 ypos = ypos + 30;
 make_toggle_button( panel, xpos, ypos, "Round calculations to Whole Numbers", round_to_whole_nums, set_r2wn_option, 0 );
 ypos = ypos + 30;
 make_button( panel, xpos, ypos, "Set PDF-Viewer", set_pdfviewer, 0 ); 
 make_button( panel, wd/2 - 30, ht - 35, " Close ", close_any_window, &options_window ); 
 show_wind( options_window );
}



void Setup_Tax_Form_Page( int init )	/* This is called whenever the form window needs to be redisplayed for any reason. */
{
 GtkWidget *button;
 GtkRequisition actual;
 int x1=1, xpos;
 char *twrd;
 float fontsz;
 double T1;

 gtk_widget_destroy( mpanel );	/* Clear out panel. */
 mpanel = gtk_fixed_new();
 gtk_container_add( GTK_CONTAINER( outer_window ), mpanel );
 /* Set up new panel with scrollbars for tax form data. */
 mpanel2 = gtk_fixed_new();
 scrolledpane = gtk_scrolled_window_new( 0, 0 );
 gtk_scrolled_window_set_policy( (GtkScrolledWindow *)scrolledpane, GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS );
 gtk_scrolled_window_add_with_viewport( (GtkScrolledWindow *)scrolledpane, mpanel2 );
 //   gtk_container_add( GTK_CONTAINER( mpanel ), scrolledpane );
 gtk_fixed_put( GTK_FIXED( mpanel ), scrolledpane, 0, 35 );
 gtk_widget_set_size_request( scrolledpane, winwidth, winht - 80 );
 operating_mode = 2;

 xpos = (int)(0.037 * (float)winwidth + 0.5);
 // printf("\nwinwidth = %d, Save = %g, ", winwidth, (float)xpos / (float)winwidth );
 button = make_button( mpanel, xpos, winht - 35, "  Save  ", save_taxfile, "0" );	/* The "Save" button. */
 add_tool_tip( button, "Save your changes." );

 xpos = (int)(0.17 * (float)winwidth + 0.5);
 button = make_button( mpanel, xpos, winht - 35, "Compute Tax", Run_TaxSolver, 0 );
 add_tool_tip( button, "Run TaxSolver." );

 xpos = (int)(0.36 * (float)winwidth + 0.5);
 button = make_button( mpanel, xpos, winht - 35, "  Print  ", printout, 0 );
 add_tool_tip( button, "Print results." );

 xpos = (int)(0.555 * (float)winwidth + 0.5);
 button = make_button( mpanel, xpos, winht - 35, "Options", options_pdf_diaglog, 0 ); 
 add_tool_tip( button, "Review and set options." );

 xpos = (int)(0.66 * (float)winwidth + 0.5);
 button = make_button( mpanel, xpos, winht - 35, "Help", helpabout2, 0 );
 add_tool_tip( button, "Get information about this program,\n Help, and Updates." );

 xpos = (int)(0.76 * (float)winwidth + 0.5);
 button = make_button( mpanel, xpos, winht - 35, "Switch Form", switch_form, 0 );
 add_tool_tip( button, "Switch to-, or Open-, another form." );

 xpos = (int)(0.94 * (float)winwidth + 0.5) - 20;
 // printf("Exit = %1.2g\n", (float)xpos / (float)winwidth );
 button = make_button( mpanel, xpos, winht - 35, " Exit ", quit_wcheck, 0 );
 add_tool_tip( button, "Leave this program." );

 twrd = (char *)malloc( strlen( taxform_name ) + 100 );
 strcpy( twrd, "<b>" );
 strcat( twrd, taxform_name );
 strcat( twrd, "</b>" );
 fontsz = 12.0;
 title_label = make_sized_label( mpanel, x1, 10, twrd, fontsz );   /* Temporarily make label to get its size. */
 gtk_widget_size_request( (GtkWidget *)title_label, &actual );
 if (actual.width > winwidth)
  fontsz = fontsz * (float)winwidth / (float)actual.width;
 else
  x1 = (winwidth - (actual.width + 20)) / 2;
 if (x1 < 0) x1 = 0;
 gtk_widget_destroy( title_label );
 title_label = make_sized_label( mpanel, x1, 10, twrd, fontsz );    /* Remake label in centered position. */
 set_widget_color( title_label, "#0000ff" );
 free( twrd );

 if (init)
  {
T1 = Report_Time();
   read_instructions( init );
printf("\nRead_Instructions took %g Seconds.\n\n", Report_Time() - T1 );
  }

 g_signal_connect( outer_window, "button-press-event", G_CALLBACK( mouse_clicked ), NULL ); 
 // gtk_widget_add_events( outer_window, GDK_BUTTON_RELEASE_MASK );
 // g_signal_connect( outer_window, "button-release-event", G_CALLBACK( mouse_unclicked ), NULL ); 

 DisplayTaxInfo();
 gtk_widget_show_all( outer_window );
}


void Get_Tax_Form_Page( char *fname )		/* This is only called once, to bring up the initial form. */
{
 Read_Tax_File( fname );
 Setup_Tax_Form_Page(1);
}


void Get_New_Tax_Form_Page( char *fname )	/* Transition from initial opening window to tax-form window. */
{
 winwidth = 780;
 winht = 700;
 gtk_window_set_resizable( GTK_WINDOW( outer_window ), 1 );
 gtk_window_resize( (GtkWindow *)outer_window, winwidth, winht );
 Get_Tax_Form_Page( fname );
}



void check_comments()	/* Make sure every line has a comment field. */
{
 struct taxline_record *txline;
 struct value_list *tmppt, *npt, *tail;
 int ncomments;

 txline = taxlines_hd;
 while (txline!=0)
  {
   ncomments = 0;
   tmppt = txline->values_hd;
   while (tmppt!=0)
    {
     if (tmppt->kind==VKIND_COMMENT) ncomments++;
     if ((tmppt->nxt==0) || (tmppt->linenum != tmppt->nxt->linenum))
      {
       if (ncomments==0)
        {
	 if (debug) printf(" Adding empty missing comment to line %d\n", tmppt->linenum );
         new_list_item_value( VKIND_COMMENT, txline, "", 50, tmppt->linenum);
	 if (tmppt->nxt != txline->values_tl)
	  {
	   npt = txline->values_tl;
	   tail = tmppt->nxt;
	   while (tail->nxt != npt) tail = tail->nxt;
	   tail->nxt = 0;
 	   txline->values_tl = tail;
	   npt->nxt = tmppt->nxt;
	   tmppt->nxt = npt;
	 }
        }
       ncomments = 0;
      }
     tmppt = tmppt->nxt;
    }
   txline = txline->nxt;
  }
}


int startswith( char *line, char *phrase )
{ /* Return true if first non-whitespace characters of line begin with pharse. */
  int j=0, k=0;
  while ((line[j] != '\0') && (isspace(line[j])))
   j++;
  while ((line[j] != '\0') && (line[j] == phrase[k]) && (phrase[k] != '\0'))
   { j++; k++; }
  if (phrase[k] == '\0')
   return 1;
  else
   return 0;
}


struct choice_rec
 {
  char *word;
  GtkEntry *box;
 };


void status_choice_S( GtkWidget *wdg, void *x )
{ struct value_list *tmppt=(struct value_list *)x;
  modify_formbox( tmppt->box, "Single" );
  if (filingstatus_mfj != 0)
   re_display_form();
}

void status_choice_MJ( GtkWidget *wdg, void *x )
{ struct value_list *tmppt=(struct value_list *)x;
  modify_formbox( tmppt->box, "Married/Joint" );
  if (filingstatus_mfj != 1)
   re_display_form();
}

void status_choice_MS( GtkWidget *wdg, void *x )
{ struct value_list *tmppt=(struct value_list *)x;
  modify_formbox( tmppt->box, "Married/Sep" );
  if (filingstatus_mfj != 0)
   re_display_form();
}

void status_choice_HH( GtkWidget *wdg, void *x )
{ struct value_list *tmppt=(struct value_list *)x;
  modify_formbox( tmppt->box, "Head_of_Household" );
  if (filingstatus_mfj != 0)
   re_display_form();
}

void status_choice_W( GtkWidget *wdg, void *x )
{ struct value_list *tmppt=(struct value_list *)x;
  modify_formbox( tmppt->box, "Widow(er)" );
  if (filingstatus_mfj != 0)
   re_display_form();
}

void spinner_choice( GtkWidget *wdg, void *x )
{ struct choice_rec *tmppt=(struct choice_rec *)x;
  modify_formbox( tmppt->box, tmppt->word );
}





GtkEntry *active_entry;

void set_included_file( char *fname )
{
 // printf("OTS_RET_open_include_file: f='%s', dir='%s', wc='%s', fname='%s'\n", fname, directory_dat, wildcards_fb, filename_fb );
 if (include_file_name != 0) free( include_file_name );
 include_file_name = strdup( fname );
 if (strstr( include_file_name, " "))
  { /* If filename contains white-space, then add quotes around it. */
   char tmpfname[MaxFname]="\"";
   strcat( tmpfname, include_file_name );
   strcat( tmpfname, "\"" );
   free( include_file_name );
   include_file_name = strdup( tmpfname );
  }
 modify_formbox( active_entry, include_file_name );
}


void open_include_file( GtkWidget *wdg, gpointer data )
{ char *filename;
  struct value_list *eb=(struct value_list *)data;
  active_entry = eb->box;
  filename = get_formbox( active_entry );
  if (filename[0] == '?') filename[0] = '\0';  /* Erase place-holder. */
  fb_clear_banned_files();
  strcpy( wildcards_incl, "_out.txt" );
  fb_extract_path_fname( filename, directory_incl, filename_incl );
  // printf("OTS_open_include_file: dir='%s', wc='%s', fname='%s'\n", directory_incl, wildcards_fb, filename_fb );
  Browse_Files( "File to Include:", 2048, directory_incl, wildcards_incl, filename_incl, set_included_file );
}


void escape_special_symbols( char *phrase, int maxlen )
{ /* Replace any ampersand (&), quotes ("), or brackets (<,>), with XML escapes. */
  int j=0, k, m, n;
  n = strlen(phrase);
  do
   {
    if (phrase[j]=='&') 
     {
      k = n + 4;  m = n;  n = n + 4;
      if (n > maxlen) {printf("xml_Parse: MaxStrLen %d exceeded.\n",maxlen); return;}
      do phrase[k--] = phrase[m--]; while (m > j);
      j++;  phrase[j++] = 'a';  phrase[j++] = 'm';  phrase[j++] = 'p';  phrase[j++] = ';';
     } else
    if (phrase[j]=='"') 
     {
      k = n + 5;  m = n;  n = n + 5;
      if (n > maxlen) {printf("xml_Parse: MaxStrLen %d exceeded.\n",maxlen); return;}
      do phrase[k--] = phrase[m--]; while (m > j);
      phrase[j++] = '&';  phrase[j++] = 'q';  phrase[j++] = 'u';  phrase[j++] = 'o';  phrase[j++] = 't';  phrase[j++] = ';';
     } else
    if (phrase[j]=='<') 
     {
      k = n + 3;  m = n;  n = n + 3;
      if (n > maxlen) {printf("xml_Parse: MaxStrLen %d exceeded.\n",maxlen); return;}
      do phrase[k--] = phrase[m--]; while (m > j);
      phrase[j++] = '&';  phrase[j++] = 'l';  phrase[j++] = 't';  phrase[j++] = ';';
     } else
    if (phrase[j]=='>') 
     {
      k = n + 3;  m = n;  n = n + 3;
      if (n > maxlen) {printf("xml_Parse: MaxStrLen %d exceeded.\n",maxlen); return;}
      do phrase[k--] = phrase[m--]; while (m > j);
      phrase[j++] = '&';  phrase[j++] = 'g';  phrase[j++] = 't';  phrase[j++] = ';';
     } else j++;
   }
  while (phrase[j] != '\0');
}


GtkWidget *make_bold_label( GtkWidget *panel, int xpos, int ypos, char *text )
{
 GtkWidget *bpanel, *label;
 char *tmptxt1, *tmptxt2;
 bpanel = gtk_fixed_new();
 gtk_fixed_put( GTK_FIXED( panel ), bpanel, xpos, ypos );
 label = gtk_label_new( text );
 tmptxt1 = (char *)malloc( strlen(text) + 1000 );
 tmptxt2 = (char *)malloc( strlen(text) + 1000 );
 strcpy( tmptxt1, text );
 escape_special_symbols( tmptxt1, 1000 );
 sprintf( tmptxt2, "<b>%s</b>", tmptxt1 );
 gtk_label_set_markup( (GtkLabel *)label, tmptxt2 );
 gtk_container_add( GTK_CONTAINER( bpanel ), label );
 free( tmptxt1 );
 free( tmptxt2 );
 return label;
}


char *mystrcasestr( char *haystack, char *needle )
{
 int j=0;
 char *hs, *ndl, *pt;
 hs = strdup( haystack );
 while (hs[j] != '\0') { hs[j] = toupper( hs[j] );  j++; }
 ndl = strdup( needle );
 j = 0;
 while (ndl[j] != '\0') { ndl[j] = toupper( ndl[j] );  j++; }
 pt = strstr( hs, ndl );
 if (pt != 0)
  {
   j = 0;
   while (pt != &(hs[j])) j++;
   pt = &(haystack[j]);
  }
 free( ndl );
 free( hs );
 return pt;
}


/*************************************************************************/
/* Display the Tax Info - This routine constructs, lays-out and populates */
/*  the panels.  Called after initial read-in and on updates.		 */
/*************************************************************************/
void DisplayTaxInfo()
{
 struct taxline_record *txline;
 struct value_list *entry, *previous_entry=0;
 GtkWidget *label, *button, *cbutton, *menu;
 GtkRequisition req;
 GtkEntry *lastbox=0;
 char messg[4096];
 int linenum, iscapgains, noplus=0;
 int capgtoggle=0, firstbox_on_line_x=0;
 int y1, y1a, yoffset=4, y2, y3, dy;
 int entry_box_height=1, extra_dy, sectionheader=1;

 int label_x0=2, label_width, label_x1, box_x0, box_width, box_x1=100, comment_x0;
 int norm_label_x1=100, min_box_x0 = 110, min_comment_x0 = 100;
 int horzpad=10;

 check_comments();
 y1 = 5;  y1a = y1 + yoffset;  dy = 50;
 if (debug) dump_taxinfo();

 /* Now place the form-data onto the pages. */
 if (debug) printf("\n--------- Now rendering interactive form-page ------------\n");
 txline = taxlines_hd;
 while (txline != 0)
  {
   if ((filingstatus_mfj == 1) || (strstr( txline->linename, "Spouse" ) == 0))
    { /*DisplayLine*/
     /* Place the line label. */
     // printf("\nAdding LineLabel %d (%3d, %d): '%s'\n", txline->linenum, 2, y1a, txline->linename );
     label = make_label( mpanel2, 2, y1a, txline->linename );
     txline->vpos = y1a;
     gtk_widget_size_request( label, &req );	/* First find the label's size. */
     gtk_widget_destroy( label );			/* Remove it, then re-place it at best position. */
     label_width = req.width;
     label_x0 = norm_label_x1 - label_width - 4;
     if (label_x0 < 0) label_x0 = 0;
     if (debug) printf("%d: LineLabel '%s' at (%d, %d)\n",txline->linenum, txline->linename, label_x0, y1a );
     label = make_label( mpanel2, label_x0, y1a, txline->linename );
     if (txline->instructions)
      set_widget_color( label, "#0000a0" );
     label_x1 = label_x0 + label_width;
     box_x0 = label_x1 + horzpad;
     if (box_x0 < min_box_x0) box_x0 = min_box_x0;
     comment_x0 = label_x1 + horzpad + 10;
     button = 0;
     lastbox = 0;

     if ((strncmp(txline->linename,"Cap-Gains",9) == 0) || (strncmp(txline->linename,"CapGains",8) == 0))
      iscapgains = 1;
     else
      iscapgains = 0;

     linenum = txline->linenum;
     entry = txline->values_hd;
     button = 0;  capgtoggle = 0;  extra_dy = 0;
     while (entry != 0)
      { /*entry*/
       if (linenum != entry->linenum)
        {
	  y1 = y1 + dy + extra_dy;
	  y1a = y1 + yoffset;
	  linenum = entry->linenum;
	  extra_dy = 0;
	  comment_x0 = min_comment_x0;
	  if (debug) printf("\tLineNum now = %d\n", linenum );
	  button = 0;
        }
       if (strstr( txline->linename, ":" ) != 0) noplus = 1;
       switch (entry->kind)
        {
         case VKIND_FLOAT:	/* This kind is presently not used at all. (or anymore?) */
		sprintf(messg, "%12.2f", entry->value ); 
		entry->box = new_formbox( mpanel2, box_x0, y1, 12, messg, 500, 0, 0 );
		lastbox = entry->box;
		gtk_widget_size_request( (GtkWidget *)(entry->box), &req );
		box_width = req.width;
		entry_box_height = req.height;
		box_x1 = box_x0 + box_width;
		comment_x0 = box_x1 + horzpad;
		if (debug) printf("\tFloat-FormBox(%d-%d, %d) = '%s'\n", box_x0, box_x1, y1, messg );
		y2 = y1 + entry_box_height - 1;
		button = make_button_wsizedcolor_text( mpanel2, box_x1 - 15, y2, "+", 6.0, "#000000", add_new_box_item, entry );   /* Add another box - button */
		add_tool_tip( button, "Add another entry box\nfor this line." );
		break;

         case VKIND_INT:  
		if (debug) printf("\tUnhandled VKIND_INT happened (%d) ??\n", (int)(entry->value));
		break;

         case VKIND_TEXT:  
		if (debug) printf("\tText-FormBox: '%s' formtype = %d\n", entry->text, entry->formtype );

		if (entry->formtype == 0)
		 { /*normal*/
		  entry->box = new_formbox( mpanel2, box_x0, y1, 12, entry->text, 500, 0, 0 );
		  if (debug) printf("\t\tPlaced type0 at (%d, %d) 12-wide\n", box_x0, y1 );
		 }
		else
		 {
		  if (entry->formtype == ID_INFO)
		   {
		    entry->box = new_formbox( mpanel2, box_x0, y1, 24, entry->text, 500, 0, 0 );
		    if (debug) printf("\t\tPlaced type2 at (%d, %d) 24-wide\n", box_x0 + 20, y1 );
		   }
		  else
		   { /*Literal_Info*/
		    entry->box = new_formbox( mpanel2, box_x0, y1, 10, entry->text, 500, 0, 0 );
		    if (debug) printf("\t\tPlaced type1 at (%d, %d) 10-wide\n", box_x0 + 20, y1 );
		   }
		  noplus = 1;
		 }
		lastbox = entry->box;
		gtk_widget_size_request( (GtkWidget *)(entry->box), &req );
		box_width = req.width;
		entry_box_height = req.height;
		box_x1 = box_x0 + box_width;
		comment_x0 = box_x1 + horzpad;

		previous_entry = entry;
		if (strcmp(txline->linename,"Status") == 0)
		 {
		  menu = make_menu_button( mpanel2, box_x1 + 3, y1a-2, "*" );
		  add_tool_tip( most_recent_menu, "Click to select filing status\nfrom available choices." );
		  add_menu_item( menu, "Single", status_choice_S, entry );
		  add_menu_item( menu, "Married/Joint", status_choice_MJ, entry );
		  add_menu_item( menu, "Married/Sep", status_choice_MS, entry );
		  add_menu_item( menu, "Head_of_Household", status_choice_HH, entry );
		  add_menu_item( menu, "Widow(er)", status_choice_W, entry );
		  comment_x0 = comment_x0 + 20;
		  if (strlen( entry->text ) > 2)
		   {
		    if (mystrcasestr( entry->text, "Married/Joint" ) != 0)
		     filingstatus_mfj = 1;
		    else
		     filingstatus_mfj = 0;
		   }
		 }
		else
		if (iscapgains)
		 {
		  switch (capgtoggle)
		   {
		    case 0:
			make_label( mpanel2, box_x0 + 15, y1 - 16, "Buy Cost" );
			firstbox_on_line_x = box_x0;
			box_x0 = comment_x0;
			capgtoggle++;

                        y2 = y1 + 15;
                        button = make_button_wsizedcolor_text( mpanel2, 60, y2 - 15, "Clear", 8.0, "#000000", verify_capgain_reset, entry );
                        add_tool_tip( button, "Clear all data for this CapGain\nSet Buy Cost box to Ready" );

			break;
		    case 1:
			make_label( mpanel2, box_x0 + 15, y1 - 16, "Date Bought" );
			box_x0 = firstbox_on_line_x;
			capgtoggle++;
			break;

		    case 2:
			make_label( mpanel2, box_x0 + 15, y1 - 16, "Sold For" );
			box_x0 = comment_x0;
			capgtoggle++;
			break;
		    case 3:
			make_label( mpanel2, box_x0 + 15, y1 - 16, "Date Sold" );
			box_x0 = firstbox_on_line_x;
			capgtoggle++;
			break;


		    case 4:
			make_label( mpanel2, box_x0 + 15, y1 - 16, "Adj Code" );
			box_x0 = comment_x0;
			capgtoggle++;
			break;

		    case 5:
			make_label( mpanel2, box_x0 + 15, y1 - 16, "Adj Amnt" );
			capgtoggle = 0;
		        y2 = y1 + entry_box_height - 1;
		        button = make_button_wsizedcolor_text( mpanel2, box_x1 - 15, y2, "+", 6.0, "#000000", add_new_capgain_boxes, entry );   /* Add more boxes - button */
		        add_tool_tip( button, "Add another set of entry\nboxes for another\ncap-gains entry." );
		        extra_dy = 23;
			box_x0 = firstbox_on_line_x;
			break;
		    default: capgtoggle = 0;
		   }
		 }
		else
		if (!noplus)
		 {
		   y2 = y1 + entry_box_height - 1;
		   button = make_button_wsizedcolor_text( mpanel2, box_x1 - 15, y2, "+", 6.0, "#000000", add_new_box_item, entry );   /* Add another box - button */
		   add_tool_tip( button, "Add another entry box\nfor this line." );
		 }
		break;

         case VKIND_COMMENT: 
		if (debug) printf("\tComment {%s} at (%d, %d)\n", entry->comment, comment_x0, y1a );

		if (startswith( entry->comment, "--" ))
		 { /*Section_header*/
		     sectionheader = 1;
		     y1 = y1 + 10;
		     y1a = y1a + 10;
		     if (startswith( entry->comment, "---" ))
			{
			 comment_x0 = 20;
			 sectionheader = 2;
			}
		 }
		if ((lastbox != 0) && (strstr( entry->comment, "(answer: " ) != 0))
		 { char tmpline[1024], tmpword[512];		/* Add choices-spinner. */
		  struct choice_rec *choice_item;
		  int j=0;
		  if (button != 0) { gtk_widget_destroy( button );  button = 0; }
		  strcpy( tmpline, strstr( entry->comment, "(answer: " ) );
		  while ((tmpline[j] != '\0') && (tmpline[j] != ')')) j++;
		  if (tmpline[j] == ')') tmpline[j] = '\0';
		  fb_next_word( tmpline, tmpword, " \t," );
		  fb_next_word( tmpline, tmpword, " \t," );
		  menu = make_menu_button( mpanel2, box_x1 + 3, y1a-2, "*" );
		  add_tool_tip( most_recent_menu, "Click to select available choices." );
		  while (tmpword[0] != '\0')
		   {
		    if (strcmp( tmpword, "...") != 0)
		     {
		      choice_item = (struct choice_rec *)malloc( sizeof(struct choice_rec) );
		      choice_item->box = lastbox;
		      choice_item->word = strdup( tmpword );
		      add_menu_item( menu, tmpword, spinner_choice, choice_item );
		     }
		    fb_next_word( tmpline, tmpword, " \t," );
		   }
		  comment_x0 = comment_x0 + 20;
		 }

		// printf(" Comment %d (%3d, %3d): '%s'\n", txline->linenum, comment_x0, y1a, entry->comment );
		if (sectionheader < 2)
		 label = make_label( mpanel2, comment_x0, y1a, entry->comment );
		else
		 label = make_bold_label( mpanel2, comment_x0, y1a, entry->comment );
		entry->comment_label = label;

		/* Add edit_line_comment button */
		if ((!sectionheader) && (entry_box_height != 0))
		 { GtkRequisition sz;
		   gtk_widget_size_request( entry->comment_label, &sz );
		   y3 = y1 + entry_box_height - 1;
		   if (comment_x0 + sz.width < winwidth - 65)
		    y2 = y1 + 0.25 * entry_box_height - 1;
		   else
		    y2 = y1 + entry_box_height - 1;

		   cbutton = make_button_wsizedcolor_text( mpanel2, winwidth - 40, y2 - 4, "*", 7.0, "#000000", edit_line_comment, entry );
		   add_tool_tip( cbutton, "Edit the comment for\nthis line." );
		   if ((strstr( entry->comment, "File-name") != 0) && (previous_entry != 0))
		    {
		     if (button != 0) { gtk_widget_destroy( button );  button = 0; }
		     cbutton = make_button_wsizedcolor_text( mpanel2, comment_x0 + 40, y3 - 4, "Browse", 7.0, "#0000ff", open_include_file, previous_entry );
		     add_tool_tip( cbutton, "Browse for tax return\noutput file to reference." );
		    }
		 }

		if (strstr( entry->comment, "\n" ))	/* Add extra line spacing for multi-line comments. */
		 { int j=0;
		   while ( entry->comment[j] != 0) { if (entry->comment[j] == '\n') y1 = y1 + 0.2 * dy;  j++; }
		 }
		sectionheader = 0;
		break;
        }
       noplus = 0;
       entry = entry->nxt;
      } /*entry*/

     y1 = y1 + dy;
     y1a = y1 + yoffset;
    } /*DisplayLine*/
   else
    txline->values_hd->box = 0;

   txline = txline->nxt;
  }
 if (debug) printf("\n--------- Done rendering interactive form-page ------------\n");
}




void dump_taxinfo()
{
 struct taxline_record *txline;
 struct value_list *tmppt;

 printf("\n======================================\n");
 printf("Line#, Kind, formtype: Value\n--------------------\n"); 
 txline = taxlines_hd;
 while (txline != 0)
  {
   printf("\n%d: %s\n", txline->linenum, txline->linename );
   tmppt = txline->values_hd;
   while (tmppt != 0)
    {
     switch (tmppt->kind)
      {
       case VKIND_FLOAT:   printf("\t%d,F,%d: %6.2f\n", tmppt->linenum, tmppt->formtype, tmppt->value ); break;
       case VKIND_INT:     printf("\t%d,I,%d: %d\n", tmppt->linenum, tmppt->formtype, (int)(tmppt->value) ); break;
       case VKIND_TEXT:    printf("\t%d,T,%d: %s\n", tmppt->linenum, tmppt->formtype, tmppt->text ); break;
       case VKIND_COMMENT: printf("\t%d,C: {%s}\n", tmppt->linenum, tmppt->comment ); break;
       case VKIND_COLON:   printf("\t%d,s:\n", tmppt->linenum ); break;
       default:		   printf("\t%d,U: \n", tmppt->linenum ); break;
      }
     tmppt = tmppt->nxt;
    }
   txline = txline->nxt;
  }
 printf("\n");
}



char *my_strcasestr( char *line, char *srchstr )
{ /* Define portable version of "non-std" C-extension function that is not (yet?) available on all platforms. */
  int j=0, k;
  while (line[j] != '\0')
   {
    k = 0;
    while ((line[j+k] != '\0') && (srchstr[k] != '\0') && (toupper( line[j+k] ) == toupper( srchstr[k])))
	k++;
    if (srchstr[k] == '\0') { return &(line[j]); }
    j++;
   }
 return 0; 
}


int lookaheadvals( struct value_list *linept )
{ /* Look ahead and count the number of any remaining values in the list of items for the given line pointer. */
 int nn=0;
 while (linept != 0)
  {
   if ((linept->kind==VKIND_FLOAT) || (linept->kind==VKIND_TEXT) || (linept->kind==VKIND_INT))
    nn++;
   linept = linept->nxt;
  }
 return nn;
}


void capgain_ready_warning(char ready_val[5][1024], char ready_comment[2][1024])  /* Used in Save_TaxFile, VKIND_TEXT, iscapgains true */
{
 char msg[4096];
 sprintf(msg, "CapGain * Ready *  Entries are  NOT  Written to the Save File\n");
 sprintf(msg + strlen(msg), "However, there are non-empty boxes present.\n\n");
 sprintf(msg + strlen(msg), "If you wish to preserve non-empty boxes, fill in the Buy Cost box.\n\n");
 sprintf(msg + strlen(msg), "%-12s%-12s%-12s\n\n", ready_val[0], ready_val[1], ready_comment[0]);
 sprintf(msg + strlen(msg), "%-12s%-12s%-12s\n\n", ready_val[2], ready_val[3], ready_comment[1]);
 sprintf(msg + strlen(msg), "%-12s%-12s%-12s\n", ready_val[4], ready_val[5], ready_comment[2]);
 GeneralPopup( "Caution Advisory:", msg, 1);
}


void Save_Tax_File( char *fname )
{
 struct taxline_record *txline;
 struct value_list *tmppt;
 int lastline=-1, semicolon, j, newline;
 char *suffix, *tmpstr;
 FILE *outfile;

 // printf("OTS_save_taxfile RET: f='%s' dir='%s', wc='%s', fname='%s'\n", fname, directory_dat, wildcards_fb, filename_fb );
 if (current_working_filename != 0) free( current_working_filename );
 current_working_filename = strdup( fname );

 /* Update the data structure(s) by getting the form fields. */
 Update_box_info();

 /* Prevent weird characters in the save-name. */
 if (1)	  /* 1 = Protect users from creating bad filenames.  0 = Let them do whatever. */
  {
   j = strlen( current_working_filename ) - 1;	/* Find leaf-name, to skip over path name. */
   while ((j >= 0) && (current_working_filename[j] != '/') && (current_working_filename[j] != '\\'))
    j--;
   j++;	 /* Will be at last slash or first character in file name. */
   while (current_working_filename[j] != '\0')
    {
     #ifdef __MINGW32__
      if ((current_working_filename[j] == ':') && (j == 1))
  	;	/* Allow ':' as second character - only. */
      else
     #endif
     if ((current_working_filename[j] < '+') || (current_working_filename[j] > 'z') ||
         (current_working_filename[j] == ','))
      {
       if (current_working_filename[j] != ' ')
        current_working_filename[j] = '_';
      }
     else
     if ((current_working_filename[j] > '9') && (current_working_filename[j] < 'A'))
       current_working_filename[j] = '_';
     else
     if ((current_working_filename[j] > ']') && (current_working_filename[j] < 'a'))
      current_working_filename[j] = '_';
     j++;
    }
  }

 suffix = my_strcasestr( current_working_filename, ".txt" );
 if ((suffix == 0) || (strcasecmp( suffix, ".txt" ) != 0))
  {
   tmpstr = (char *)malloc( strlen( current_working_filename ) + 10 );
   strcpy( tmpstr, current_working_filename );
   strcat( tmpstr, ".txt" );
   current_working_filename = tmpstr;
  }

 suffix = my_strcasestr( current_working_filename, "_out.txt" );
 if ((suffix != 0) && (strcasecmp( suffix, "_out.txt" ) == 0))
  {
   warn_release = 2;
   GeneralWarning( "Your are saving an 'input-file', but the file name you picked looks like an output file." );
   return;
  }

 if ((my_strcasestr( current_working_filename, "_template.txt" ) != 0))
  {
   warn_release = 2;
   GeneralWarning( "Your are saving over the 'template' file. Please choose a new unique name." );
   return;
  }

 outfile = fopen(current_working_filename, "w");
 if (outfile==0) 
  {
   sprintf(wmsg,"ERROR: Output file '%s' could not be opened for writing.", current_working_filename );
   warn_release = 2;
   GeneralWarning( wmsg );
   return;
  }
 if (yourfilename != 0) free( yourfilename );
 yourfilename = strdup( current_working_filename );
 fprintf(outfile,"%s", title_line);

 if (round_to_whole_nums)
  fprintf(outfile,"\nRound_to_Whole_Dollars\n");

 txline = taxlines_hd;
 while (txline!=0)
  { /*txline*/
   int numvals=0, numcoms=0, iscapgains, capgain_ready_flg=0, ReadyErrFlg=0, tilde_flg=0;
   char ready_val[6][1024], ready_comment[3][1024]={{0}}; /* Used by capgain_ready_warning */
   int valcnt=0, comcnt=0; /* Counters for an individual gain */
   // printf("\nNewTaxLine: '%s' (linenum = %d)\n", txline->linename, txline->linenum );
   fprintf(outfile,"\n%s", txline->linename );		/* Write line-label, if any. */
   if (strncasecmp(txline->linename, "CapGains",7) == 0)
    iscapgains = 1;
   else
    iscapgains = 0;
   semicolon = 0;
   newline = 0;
   lastline = txline->linenum;
   tmppt = txline->values_hd;
   while (tmppt!=0)					/* Now write the line-value(s), comment(s), and ";", if any. */
    { /*line_item*/
     if (valcnt == 6)    /* Counters and flags to enable Capgain Reset */
     {
       valcnt = 0;
       comcnt = 0;
       capgain_ready_flg = 0;
       ReadyErrFlg = 0;
       tilde_flg = 0;
     }

     switch (tmppt->kind)
      {
       case VKIND_FLOAT:
		if (newline) fprintf(outfile,"\n");
		fprintf(outfile,"	%6.2f	", tmppt->value );
		numvals++;
		// printf("	FloatValue: '%6.2f' (linenum = %d)\n", tmppt->value, tmppt->linenum );
		break;
       case VKIND_INT:
		if (newline) fprintf(outfile,"\n");
		fprintf(outfile,"	%d	", (int)(tmppt->value) );
		numvals++;
		// printf("	IntValue: '%d' (linenum = %d)\n", (int)(tmppt->value), tmppt->linenum );
		break;
       case VKIND_TEXT:
		if (iscapgains)
		{
		   /* Handle  capgain_ready_flg */
                   if ( valcnt == 0 && (strcmp(tmppt->text, CAPGAIN_READY) == 0 ) ) capgain_ready_flg = 1; 
                   if (capgain_ready_flg == 0)
                   {
                    tilde_flg=0;
                    if ( (strlen(tmppt->text) == 0 ) ) /* Check for empty capgains box */
                    {
                     //GeneralWarning ("Detected Empty CapGains Box, Inserting   '~'   into save file.");
		     tilde_flg=1;
                    }
                    if (lastline != tmppt->linenum)
	             { int j;  for (j=0; j < tmppt->linenum - lastline; j++) fprintf(outfile,"\n");  lastline++; }  
                    if (tilde_flg==1) fprintf(outfile,"	~	"); 
 		    else 
                    {
                     fprintf(outfile,"	%s	", tmppt->text );
		    }
                    if (valcnt==5 && ReadyErrFlg==1) capgain_ready_warning(ready_val, ready_comment);  /* Display Warning Message */
                   } // capgain_ready_flg == 0
		   else  
		   {   /* capgain_ready_flg  = 1,  "Ready" is in Buy Cost box, Do Error trapping.  Do NOT save to input.txt file. */                    
		    strcpy(ready_val[valcnt], tmppt->text) ; /* Collect the capgain data values in array ready_val */ 
                    /* A "Ready" gain/loss should only contain empty or "~" values */
	            if ( (valcnt > 0) && (strlen(tmppt->text) != 0) && (strcmp(tmppt->text, "~") != 0 ) )  ReadyErrFlg = 1;
                    if (valcnt==5 && ReadyErrFlg==1) capgain_ready_warning(ready_val, ready_comment);  /* Display Warning Message */
		   }
                } /* iscapgains */
                else
                {
                 if (newline) fprintf(outfile,"\n");
                 fprintf(outfile,"	%s	", tmppt->text );
                }
                 valcnt++;
                 numvals++;
		// printf("	TextValue: '%s' (linenum = %d)\n", tmppt->text, tmppt->linenum );
		break;		 
       case VKIND_COMMENT:
		if (tmppt->linenum != lastline) fprintf(outfile,"\n\t");
		if ( (strlen(tmppt->comment)>0))
                {
                 if (capgain_ready_flg == 0) fprintf(outfile," {%s}", tmppt->comment );
                 else
                 {
                  strcpy(ready_comment[comcnt], tmppt->comment);  /* CapGain Ready gain/loss */
                  comcnt++;
                 }
                }
		// printf("	Comment: '%s' (linenum = %d)\n", tmppt->comment, tmppt->linenum );
		numcoms++;
		break;
       case VKIND_COLON:   
		if ((numvals < 2) && (numcoms == 0) && (lookaheadvals(tmppt->nxt) == 0))
		 fprintf(outfile,"\t;");
		else
		 semicolon = 1;
		// printf("	SemiColon: numvals = %d (linenum = %d)\n", numvals, tmppt->linenum );
		break;
      }
     newline = 1;
     lastline = tmppt->linenum;
     tmppt = tmppt->nxt;
    } /*line_item*/

   if (semicolon)
    {
     fprintf(outfile,"\n");
     fprintf(outfile,"		;");
    }
   txline = txline->nxt;
  } /*txline*/


 fprintf(outfile,"\n");
 dump_any_markup_commands( outfile );
 fclose(outfile);
 save_needed = 0;
 printf("\nWrote form-data to file %s\n.", yourfilename );
 if (pending_compute == 2)
  taxsolve();
}



void open_taxfile( char *filename )
{
 infile = fopen(filename,"r");
 if (infile==0) 
  {
   printf("ERROR: Input file '%s' could not be opened.\n", filename);
   GeneralWarning("Error: Tax file could not be opened.");
   return;
  }
 else
  {
   Get_New_Tax_Form_Page( filename );
  }
} 



void save_taxfile( GtkWidget *wdg, void *data )
{ 
 char *cpt, *param;
 if (verbose) printf("File-Save Dialog at: '%s'\n", directory_dat );
 param = (char *)data;
 if (param[0] == '0') pending_compute = 0;
 fb_clear_banned_files();
 fb_ban_files( "_out.txt" );
 fb_ban_files( "_template.txt" );
 fb_ban_files( "README" );
 strcpy( wildcards_fb, ".txt" );
 fb_extract_path_fname( yourfilename, directory_dat, filename_fb );
 cpt = strstr( filename_fb, "_template.txt" );
 if (cpt != 0)
  strcpy( cpt, "_xxxx.txt" );
 // printf("OTS_save_taxfile: dir='%s', wc='%s', fname='%s'\n", directory_dat, wildcards_fb, filename_fb );
 Browse_Files( "File to Save As:", 2048, directory_dat, wildcards_fb, filename_fb, Save_Tax_File );
}


void predict_output_filename(char *indatafile, char *outfname)
{
 int j;
 /* Base name of output file on input file. */
 strcpy( outfname, indatafile );
 j = strlen(outfname) - 1;
 while ((j >= 0) && (outfname[j] != '.')) j--;
 if (j < 0) strcat( outfname, "_out.txt" ); else strcpy( &(outfname[j]), "_out.txt" );
}



void set_tax_solver( char *fname )
{
 // printf("OTS_set_tax_solver RET: f='%s', dir='%s', wc='%s', fname='%s'\n", fname, toolpath, wildcards_fb, filename_fb );
 taxsolvecmd = strdup( fname );
 strcpy( taxsolvestrng, taxsolvecmd );

 if (strstr( taxsolvestrng, "taxsolve_HSA_f8889" ) != 0)
  {
   supported_pdf_form = 1;
   strcat( directory_dat, slashstr );		/* Set the directory name for the form template & example files. */
   strcat( directory_dat, "HSA_Form_8889" );
  }
 else
 if (strstr( taxsolvestrng, "taxsolve_f8606" ) != 0)
  {
   supported_pdf_form = 1;
   strcat( directory_dat, slashstr );		/* Set the directory name for the form template & example files. */
   strcat( directory_dat, "Form_8606" );
  }
 else
 if (strstr( taxsolvestrng, "taxsolve_US_1040_Sched_SE" ) != 0)
  {
   supported_pdf_form = 1;
   strcat( directory_dat, slashstr );		/* Set the directory name for the form template & example files. */
   strcat( directory_dat, "US_1040_Sched_SE" );
  }
 else
 if (strstr( taxsolvestrng, "taxsolve_f8959" ) != 0)
  {
   supported_pdf_form = 1;
   strcat( directory_dat, slashstr );		/* Set the directory name for the form template & example files. */
   strcat( directory_dat, "Form_8959" );
  }
 else
 if (strstr( taxsolvestrng, "taxsolve_f8960" ) != 0)
  {
   supported_pdf_form = 1;
   strcat( directory_dat, slashstr );		/* Set the directory name for the form template & example files. */
   strcat( directory_dat, "Form_8960" );
  }
 else
 if (strstr( taxsolvestrng, "taxsolve_f2210" ) != 0)
  {
   supported_pdf_form = 1;
   strcat( directory_dat, slashstr );		/* Set the directory name for the form template & example files. */
   strcat( directory_dat, "Form_2210" );
  }
 else
 if (strstr( taxsolvestrng, "taxsolve_CA_5805" ) != 0)
  {
   supported_pdf_form = 1;
   strcat( directory_dat, slashstr );		/* Set the directory name for the form template & example files. */
   strcat( directory_dat, "Form_CA_5805" );
  }
}


void canceltxslvr( GtkWidget *wdg, void *data )
{
 gtk_widget_destroy( resultswindow );
 resultswindow = 0;
}



int missingfile=0;


void filter_tabs( char *line )
{ /* Replace tabs and <cr> with spaces for clean printouts. */
 char *ptr;

 ptr = strchr( line, '\r' );
 if (ptr != 0)
  ptr[0] ='\0';

 ptr = strchr( line, '\n' );
 if (ptr != 0)
  ptr[0] ='\0';

 ptr = strchr( line, '\t' );
 while (ptr != 0)
  {
   ptr[0] = ' ';
   ptr = strchr( line, '\t' );
  }
}


void set_invocation_path( char *toolpath )
{
 char tmpstr[MaxFname];
 int k;
 strcpy(tmpstr, invocation_path);
 k = strlen(tmpstr)-1;
 while ((k > 0) && (tmpstr[k] != slashchr)) k--;
 if (k > 0) k--;
 while ((k > 0) && (tmpstr[k] != slashchr)) k--;
 if (tmpstr[k] == slashchr)
  tmpstr[k+1] = '\0';
 else 
#if (PLATFORM_KIND==Posix_Platform)
   {sprintf(tmpstr,".%c", slashchr);}
  if (strstr( invocation_path, "bin" ) != 0)
   sprintf( toolpath, "%sbin%c", tmpstr, slashchr);
  else
   strcpy( toolpath, "./" );   
 #else
   tmpstr[k] = '\0';
  sprintf( toolpath, "%sbin%c", tmpstr, slashchr);
 #endif
}


void quote_file_name( char *fname )	/* Place quotes around a file name.  With special care on Microsoft systems. */
{					/* Enables proper operation when files or pathnames have spaces in them. */
 char *tmpstr;
 if ((fname[0] == '\0') || (strstr( fname, "\"" ) != 0)) return;
 tmpstr = (char *)malloc( strlen(fname) + 10 );
 #if (PLATFORM_KIND == Posix_Platform)
   strcpy( tmpstr, "\"" );
   strcat( tmpstr, fname );
 #else
   if (fname[1] == ':')
    { /* Leading quote must be inserted after drive letter for Microsoft OS's. */
     int j;
     tmpstr[0] = fname[0];
     tmpstr[1] = fname[1];
     tmpstr[2] = '"';
     j = 2;
     do { tmpstr[j+1] = fname[j];  j++; } while (tmpstr[j] != '\0');
    }
   else
    {
     strcpy( tmpstr, "\"" );
     strcat( tmpstr, fname );
    }
 #endif
 strcpy( fname, tmpstr );
 strcat( fname, "\"" );
 free(tmpstr);
}



void taxsolve()				/* "Compute" the taxes. Run_TaxSolver. */
{
 char cmd[MaxFname+512], outfname[MaxFname];
 GtkWidget *panel, *label;
 GtkTreeStore *mylist;
 GtkTreeIter iter;
 FILE *viewfile;
 char vline[9000], *errmsg=0;
 int wd, ht, valid_results=1, linesread=0;

 if (current_working_filename == 0) 
  {
   GeneralWarning( "No tax file selected." );
   return;
  }
 if (strlen(taxsolvestrng) > 0) 
  taxsolvecmd = taxsolvestrng;
 if (taxsolvecmd == 0) 
  taxsolvecmd = getenv("taxsolvecmd");
 if (taxsolvecmd == 0)
  {
   set_invocation_path( toolpath );
   fb_clear_banned_files();
   fb_ban_files( ".txt" );
   fb_ban_files( ".pdf" );
   strcpy( wildcards_fb, "" );
   strcpy( filename_fb, "" );
   // printf("OTS_taxsolve: dir='%s', wc='%s', fname='%s'\n", toolpath, wildcards_fb, filename_fb );
   Browse_Files( "Select Tax Program to Use:", 2048, toolpath, wildcards_fb, filename_fb, set_tax_solver );
   place_window_atmouse();	/* Temporarily change the new window position policy. */
   GeneralWarning( "No tax solver selected.  Re-try after selecting." );
   place_window_center();	/* Restore the normal window position policy. */
   return;
  }

 if ((allforms_toggle) && (selected_form == form_US_1040))
  strcpy( run_options, "-allforms" );
 else
  strcpy( run_options, "" );

 if (round_to_whole_nums)
  strcat( run_options, " -round_to_whole_dollars" );

 #if (PLATFORM_KIND == Posix_Platform)
  sprintf(cmd,"'%s' %s '%s' &", taxsolvecmd, run_options, current_working_filename );
 #else
   if ((strlen(taxsolvecmd) > 2 ) && (taxsolvecmd[1] == ':') && (taxsolvecmd[2] != '"')) 
    { /*Insert quotes around file name for Microsoft, in case pathname has spaces in it.*/
     int j;
     char *tstr;
     j = strlen( taxsolvecmd ) + 10;
     tstr = (char *)malloc(j);
     for (j = 0; j < 2; j++) tstr[j] = taxsolvecmd[j];
     j = 2;	/* Leading quote must be inserted after drive letter for Microsoft OS's. */
     tstr[2] = '"';  j++;
     do { tstr[j] = taxsolvecmd[j-1];  j++; } while (tstr[j-1] != '\0');
     strcat( tstr, "\"" );
     taxsolvecmd = tstr;
    }
  sprintf(cmd,"%s %s \"%s\"", taxsolvecmd, run_options, current_working_filename );
 #endif

 printf("Invoking '%s'\n", cmd );
 system(cmd);		/* Run the TaxSolver. */
 Sleep_seconds( 0.1 );

 /* Make a popup window telling where the results are, and showing them. */
 predict_output_filename( current_working_filename, outfname );
 wd = 620;  ht = 550;
 panel = new_window( wd, ht, "Results", &resultswindow );
 make_sized_label( panel, 1, 1, "Results written to file:", 12 );
 label = make_sized_label( panel, 30, 25, outfname, 8 );
 set_widget_color( label, "#0000ff" );
 // make_button( panel, wd/2 - 15, ht - 35, "  OK  ", canceltxslvr, 0 ); 
 make_button( panel, 40, ht - 35, "Print Result File", print_outfile_directly, 0 ); 
 make_button( panel, wd - 85, ht - 35, " Close ", canceltxslvr, 0 ); 
 show_wind( resultswindow );
 UpdateCheck();
 Sleep_seconds( 0.25 );
 UpdateCheck();
 Sleep_seconds( 0.25 );
 mylist = new_selection_list( panel, 5, 50, wd - 10, ht - 50 - 50, "Results Preview:", 0, 0, 0 );
 viewfile = fopen( outfname, "rb" );
 if (viewfile == 0)
  {
   sprintf(vline,"Cannot open: %s", outfname);
   printf("%s\n", vline );
   append_selection_list( mylist, &iter, vline );
   valid_results = 0;
  }
 else
  {
   int valid=1;
   fgets( vline, 256, viewfile );
   while ((!feof(viewfile)) && valid)
    {
     filter_tabs( vline );
     append_selection_list( mylist, &iter, vline );
     linesread++;

     if (my_strcasestr( vline, "Error" ) != 0) 
      {
       if ((errmsg == 0) && (strncasecmp( vline, "Error", 5 ) == 0))
	{ /* Any line starting with "Error" is considered to indicate an error-problem. */
	 valid_results = 0;
	 errmsg = strdup( vline );
	}
      }

     fgets( vline, 256, viewfile );
     if (strstr( vline, "Identity-Information:" ) != 0) 
      valid = 0;
    }
   fclose(viewfile);
  }
 if ((valid_results) && (linesread > 10) && (supported_pdf_form))
   make_button( panel, wd/2 - 80, ht - 35, "Fill-out PDF Forms", create_pdf_file_directly, 0 ); 
 show_wind( resultswindow );
 computed = 1;
 compute_needed = 0;
 if (errmsg)
  GeneralPopup( "Error", errmsg, 1 );
}



void Run_TaxSolver( GtkWidget *wdg, void *x )
{
 Update_box_info();
 if (save_needed)
  {
   warn_release = 2;
   pending_compute = 1;
   GeneralWarning( "Change(s) not saved.  You must save before computing." );
   return;
  }
 taxsolve();	/* Compute Taxes. */
}





GtkWidget *printpopup=0, *print_label, *print_button;
GtkEntry *printerformbox;
char printer_command[MaxFname+256], wrkingfname[MaxFname];
char fillout_pdf_command[8192], fillout_pdf_tool[2048]="", modify_pdf_tool[2048]="";
int printdialogsetup;
int print_mode=0;
#if (PLATFORM_KIND==Posix_Platform)
 char base_printer_command[]="lpr ";
#else
 char base_printer_command[]="print ";
#endif
void acceptprinter_command( GtkWidget *wdg, void *data );



void set_pdf_tool_path()
{
 set_invocation_path( fillout_pdf_tool );
 strcat( fillout_pdf_tool, "fillout_PDF_forms" );
 quote_file_name( fillout_pdf_tool );

 set_invocation_path( modify_pdf_tool );
 strcat( modify_pdf_tool, "universal_pdf_file_modifier" );
 quote_file_name( modify_pdf_tool );
}


void cancelprintpopup( GtkWidget *wdg, void *data )
{ gtk_widget_destroy( printpopup );  printpopup = 0; }


void togprntcmd_in( GtkWidget *wdg, void *data )
{ 
 if (!printdialogsetup) return;
 sprintf(printer_command,"%s \"%s\"", base_printer_command, current_working_filename );
 modify_formbox( printerformbox, printer_command );
 if (print_mode == 2)
  {
   modify_label( print_label, "Print Command" );
   gtk_button_set_label( GTK_BUTTON( print_button ), "  Print It  " );
  }
 print_mode = 0;
}


void togprntcmd_out( GtkWidget *wdg, void *data )
{
 if (!printdialogsetup) return;
 predict_output_filename( current_working_filename, wrkingfname );
 sprintf(printer_command,"%s \"%s\"", base_printer_command, wrkingfname );
 modify_formbox( printerformbox, printer_command );
 if (print_mode == 2) 
  {
   modify_label( print_label, "Print Command" );
   gtk_button_set_label( GTK_BUTTON( print_button ), "  Print It  " );
  }
 print_mode = 1;
}


GtkWidget *status_win=0, *status_panel, *status_label;
struct 
 { int wd, ht, y_val, nfiles;
   char *fnames[10];
 } statusw;
char *pdfviewer=0;


void dismiss_status_win( GtkWidget *wdg, void *data )
{ 
 if (status_win != 0)
  { gtk_widget_destroy( status_win );  status_win = 0; }
}

int killed_status_win( GtkWidget *wdg, void *data )
{ 
 status_win = 0;
 return 0;	/* Returning "0" causes window to be destroyed. */
}

void add_status_line( char *msg )
{
 if (status_win == 0) return;
 make_label( status_panel, 10, statusw.y_val, msg );
 statusw.y_val = statusw.y_val + 20;
 UpdateCheck();
 show_wind( status_win );
 UpdateCheck();
}

void update_status_label( char *msg )
{
 if (status_win == 0) return;
 modify_label( status_label, msg );
 UpdateCheck();
}

void create_status_popup_window( int width, int height )
{
 status_panel = new_scrolled_window_wkill( width, height, "Filling-out PDF Form(s) ...", &status_win, 1, 0, killed_status_win );
 status_label = make_label( status_panel, 1, 1, "Filling-out PDF Form(s):" );
 gtk_window_set_keep_above( (GtkWindow *)status_win, 1 );
 statusw.y_val = 28;
 statusw.wd = width;  statusw.ht = height;
 show_wind( status_win );
}


void get_cwd( char *pwd )
{
 int j=0;
 FILE *fp;
 fp = popen( "echo %cd%", "rb" );
 if (fp != 0)
  {
   do pwd[j++] = getc( fp );
   while ((!feof(fp)) && (pwd[j-1] != '\n') && (pwd[j-1] != '\r'));
   if (j > 0) j--;
   pwd[j] = '\0';
   pclose(fp);
  }
 else
  pwd[0] = '\0';
}


void call_pdfviewer( char *pdfname )
{
 char cmd[4096], tmppdfname[4096];
 strcpy( tmppdfname, pdfname );
 #if (PLATFORM_KIND==Posix_Platform)
  quote_file_name( tmppdfname );
  #ifdef __APPLE__
   strcpy( cmd, "open -a ");
   strcat( cmd, pdfviewer );
  #else
   if (strcmp( pdfviewer, "default-pdf-viewer" ) == 0)
    strcpy( cmd, "xdg-open" );
   else
    strcpy( cmd, pdfviewer );
  #endif
  strcat( cmd, " ");  strcat( cmd, tmppdfname );  strcat( cmd, " &" );
 #else
  strcpy( cmd, "start ");
  if (strcmp( pdfviewer, "default-pdf-viewer" ) != 0)
   { char pwd[4096];
    strcat( cmd, pdfviewer );
    strcat( cmd, " ");  
    if ((strstr( pdfviewer, "chrome") != 0) || (strstr( pdfviewer, "firefox") != 0))
     { /* Prepend 'file:///' + Path, to relative file name. */
       strcat( cmd, "\"file://" );
       get_cwd( pwd );
       strcat( cmd, pwd );
       strcat( cmd, "\\" );
       strcat( cmd, tmppdfname );
       strcat( cmd, "\"" );
     }
    else
    if ((strstr( pdfviewer, "iexplore" ) != 0) || (strstr( pdfviewer, "edge") != 0))
     { /* As above, but without quotes. */
       strcat( cmd, "file://" );
       get_cwd( pwd );
       strcat( cmd, pwd );
       strcat( cmd, "\\" );
       strcat( cmd, tmppdfname );
     }
    else
     strcat( cmd, tmppdfname );
   }
  else
   { /* Default PDF-Viewer. */
    strcat( cmd, " ");  
    strcat( cmd, tmppdfname );
   }
 #endif
 printf("Issuing: %s\n", cmd );
 system( cmd );
}


void consume_leading_trailing_whitespace( char *line )
{ int j, k;
  while (isspace( line[0] ))
   {
    j = 0;
    do { line[j] = line[j+1];  j++; }
    while (line[j-1] != '\0');
   }
 k = strlen( line ) - 1;
 while ((k >= 0) && (isspace( line[k] )))
  {
   line[k] = '\0';
   k--;
  }
}


char *check4tool( char *toolname )
{
 char line[4096]="which ";
 FILE *cmdresp;
 strcat( line, toolname );
 cmdresp = popen( line, "r" );
 line[0] = '\0';
 if (cmdresp != 0)
  {
   fscanf( cmdresp, "%s", line );
   pclose( cmdresp );
  }
 if (line[0] != '\0')
   return strdup( line );
 else
   return 0;
}



void get_pdf_viewer()
{
 FILE *configfile;
 char fname[4096], line[4096];

 // printf("get_pdf_viewer:\n");
 pdfviewer = getenv( "PDF_VIEWER" );	/* First check if user has set PDF_Viewer environment variable. */
 if (pdfviewer != 0) return;

 if (verbose) printf(" Checking config file\n");
 set_invocation_path( toolpath );	/* Next check if user has set preferences in config-file. */
 strcpy( fname, toolpath );
 strcat( fname, "gui_settings.conf" );
 configfile = fopen( fname, "r" );
 if (configfile == 0)
  printf("Could not open 'gui_settings.conf'.\n");
 else
  {
   /* Expect config file to have options like:  PDF_VIEWER: acroread	*/
   fgets( line, 2048, configfile );
   while (!feof(configfile))
    {
     consume_leading_trailing_whitespace( line );
     if ((line[0] != '#') && (line[0] != '\0'))
      {
	if (strncmp( line, "PDF_VIEWER:", 11 ) == 0)
	 {
	  pdfviewer = strdup( &(line[12]) );
	  consume_leading_trailing_whitespace( pdfviewer );
	  if (verbose) printf("Setting pdfviewer = '%s'\n", pdfviewer );
	 }
	else
	 printf("Unknown config-file option '%s'\n", line );
      }
     fgets( line, 2048, configfile );
    }
   fclose( configfile );
  }
 if (pdfviewer == 0)	/* If the above fails, then set viewer to a default. */
  {
   printf(" (Did not see preferred PDF_VIEWER in gui_settings.conf - Using default viewer.)\n");
   #if (PLATFORM_KIND==Posix_Platform)
     /* --- The following are methods for opening PDF document(s) on Apple or Linux. --- */
     // pdfviewer = strdup( "xpdf" );
     // pdfviewer = strdup( "acroread" );
     // pdfviewer = strdup( "atril" );
     #ifdef __APPLE__
       { // struct stat buf;
	 // if (stat( "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome", &buf ) == 0)
	 //  pdfviewer = strdup( "\"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome\"" );
         // else
          pdfviewer = strdup( "Preview" );
         // pdfviewer = strdup( "Safari" );
       }
     #else
       pdfviewer = check4tool( "atril" );
     #endif
       if (pdfviewer == 0)
        pdfviewer = check4tool( "google-chrome" );
       if (pdfviewer == 0)
        pdfviewer = check4tool( "firefox" );
       if (pdfviewer == 0)
        pdfviewer = check4tool( "evince" );
       if (pdfviewer == 0)
        pdfviewer = check4tool( "xpdf" );
       if (pdfviewer == 0)
        pdfviewer = check4tool( "libreoffice" );
       if (pdfviewer == 0)
        pdfviewer = check4tool( "ooffice" );
       if (pdfviewer == 0)
        pdfviewer = check4tool( "acroread" );
       if (pdfviewer == 0)
	pdfviewer = strdup( "xdg-open" );
   #else
     /* --- The following is(are) method(s) for opening PDF document(s) on Microsoft. --- */
     pdfviewer = strdup( "default-pdf-viewer" );	/* Calls default pdf viewer properly. */
   #endif
   if (pdfviewer == 0)
    {
     printf("Could not find a PDF viewer on your system.\n");
     // pdfviewer = strdup( "google-chrome" );
    }
   else
    printf("Using PDF viewer: '%s'\n", pdfviewer );
  }
}




GtkWidget *spdfpopup=0;
GtkEntry *spdffrmbx;


void cancelspdfpopup( GtkWidget *wdg, void *data )
{ gtk_widget_destroy( spdfpopup );  spdfpopup = 0; }


void accept_pdfviewer( GtkWidget *wdg, void *data )
{ char *selection;
 selection = get_formbox( spdffrmbx );
 if (pdfviewer != 0) free( pdfviewer );
 #ifdef __APPLE__
   if (strcmp(selection, "google-chrome" ) == 0)
     pdfviewer = strdup( "\"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome\"" );
   else
   if (strcmp(selection, "firefox" ) == 0)
     pdfviewer = strdup( "\"/Applications/Firefox.app/Contents/MacOS/firefox\"" );
   else
 #endif
 pdfviewer = strdup( selection );
 printf("Accepted PDF-Viewer as '%s'\n", pdfviewer );
 cancelspdfpopup( 0, 0 );
}


void saveconfig( GtkWidget *wdg, void *data )
{
 FILE *configfile, *infile;
 char fname[4096], bckfile[4096], line[4096];

 accept_pdfviewer( 0, 0 );
 if (pdfviewer == 0) return;
 printf("Saving config file ...\n");
 set_invocation_path( toolpath );
 strcpy( fname, toolpath );
 strcat( fname, "gui_settings.conf" );

 /* First make a backup config-file. */
 infile = fopen( fname, "r" );
 if (infile == 0)
  printf("Could not open 'gui_settings.conf' for reading.\n");
 else
  {
   strcpy( bckfile, toolpath );
   strcat( bckfile, "gui_settings.bak" );
   configfile = fopen( bckfile, "w" );
   if (configfile == 0)
    printf("Could not open 'gui_settings.bak' for writing.\n");
   else
    {
     fgets( line, 2048, infile );
     while (!feof(infile))
      {
	fprintf(configfile,"%s", line );
	fgets( line, 2048, infile );
      }
     fclose( configfile );
    }
   fclose( infile );
  }

 configfile = fopen( fname, "w" );
 if (configfile == 0)
  printf("Could not open 'gui_settings.conf' for writing.\n");
 else
  {
   fprintf( configfile, "PDF_VIEWER: %s\n", pdfviewer );
   fclose( configfile );
  }
}



void catch_pdfviewer_selection( GtkWidget *wdg, void *data )
{
 char *text;
 text = get_selection_from_list( wdg );
 if (strlen( text ) > 0)
  modify_formbox( spdffrmbx, text ); 
}


void set_pdfviewer( GtkWidget *wdg, void *data )
{
 int wd=400, ht=300;
 GtkWidget *panel;

 window_position_policy = GTK_WIN_POS_NONE;
 panel = new_window( wd, ht, "Select Your PDF-Viewer", &spdfpopup );
 place_window_center();
 make_sized_label( panel, 15, 15, "Pick and Set PDF Viewer to Use:", 12 );
 { GtkTreeStore *spdfvlist;  GtkTreeIter iter;
   spdfvlist = new_selection_list( panel, 40, 40, 333, 119, "Some popular viewers ...", catch_pdfviewer_selection, 0, 0 );
   append_selection_list( spdfvlist, &iter, "" );
   #if (PLATFORM_KIND==Posix_Platform)
    /* --- For Apple's and Linux --- */
    #ifdef __APPLE__
     append_selection_list( spdfvlist, &iter, "Google Chrome" );
     append_selection_list( spdfvlist, &iter, "Preview" );
     append_selection_list( spdfvlist, &iter, "Safari" );
    #else
     append_selection_list( spdfvlist, &iter, "default-pdf-viewer" );
     append_selection_list( spdfvlist, &iter, "google-chrome" );
    #endif
    append_selection_list( spdfvlist, &iter, "firefox" );
    append_selection_list( spdfvlist, &iter, "atril" );
    append_selection_list( spdfvlist, &iter, "xpdf" );
    append_selection_list( spdfvlist, &iter, "evince" );
    append_selection_list( spdfvlist, &iter, "libreoffice" );
    append_selection_list( spdfvlist, &iter, "ooffice" );
    #ifdef __APPLE__
    #endif
    append_selection_list( spdfvlist, &iter, "acroread" );
   #else
    /* --- For Microsoft --- */
    append_selection_list( spdfvlist, &iter, "default-pdf-viewer" );
    append_selection_list( spdfvlist, &iter, "chrome" );
    append_selection_list( spdfvlist, &iter, "firefox" );
    // append_selection_list( spdfvlist, &iter, "microsoft-edge:" );	/* Edge can't open local files. :-( */
    append_selection_list( spdfvlist, &iter, "iexplore" );
    append_selection_list( spdfvlist, &iter, "evince" );
    append_selection_list( spdfvlist, &iter, "xpdf" );
    append_selection_list( spdfvlist, &iter, "libreoffice" );
    append_selection_list( spdfvlist, &iter, "acroread" );
   #endif
 }
 make_label( panel, 20, 185, "PDF Viewer Executable:" );
 if (pdfviewer == 0)
   get_pdf_viewer();
 if (pdfviewer == 0)
  spdffrmbx = new_formbox( panel, 35, 200, 42, "", 500, accept_pdfviewer, 0 );
 else
  spdffrmbx = new_formbox( panel, 35, 200, 42, pdfviewer, 500, accept_pdfviewer, 0 );

 make_button( panel, 30, 260, "      OK      ", accept_pdfviewer, 0 );
 make_button( panel, 180, 260, "Save", saveconfig, 0 );
 make_button( panel, 320, 260, "Cancel", cancelspdfpopup, 0 );
 // gtk_window_activate_focus( GTK_WINDOW( spdfpopup ) );
 // gtk_window_set_transient_for( GTK_WINDOW( spdfpopup  ), GTK_WINDOW( status_win ) );
 // gtk_window_set_transient_for( GTK_WINDOW( spdfpopup  ), GTK_WINDOW( outer_window ) );
 // gtk_window_set_keep_above( GTK_WINDOW( spdfpopup ), 1 );
 show_wind( spdfpopup );
}


void callup_pdfviewer( GtkWidget *wdg, void *data )
{
 int k=0;
 dismiss_status_win( 0, 0 );
 UpdateCheck();
 printf("\n");
 if (pdfviewer == 0)
  {
   get_pdf_viewer();
  }
  if (pdfviewer == 0)
  {
   GeneralWarning( "Could not find a PDF viewer on your system.\n");
   return;
  }
 while (k < statusw.nfiles)
   call_pdfviewer( statusw.fnames[k++] );
}


void add_view_pdf_button()
{
 make_button( status_panel, 30, statusw.ht - 50, "(Set PDF Viewer)", set_pdfviewer, 0 );
 make_button( status_panel, statusw.wd/2 - 90, statusw.ht - 50, "   Open in PDF Viewer   ", callup_pdfviewer, 0 );
 make_button( status_panel, statusw.wd - 90, statusw.ht - 50, "Close", dismiss_status_win, &status_win );
 show_wind( status_win );
}



void togprntcmd_pdf( GtkWidget *wdg, void *data )
{
 if (!printdialogsetup) return;
 predict_output_filename( current_working_filename, wrkingfname );
 modify_formbox( printerformbox, wrkingfname );
 if (print_mode != 2)
  {
   modify_label( print_label, "File:" );
   gtk_button_set_label( GTK_BUTTON( print_button ), "    OK     " );
  }
 print_mode = 2;
}


int schedule_PDF_conversion=0, pdf_conversion_step;

void acceptprinter_command( GtkWidget *wdg, void *data )
{
 if ((print_mode > 0) && (compute_needed))
  {
   cancelprintpopup( 0, 0 );
   GeneralWarning( "Change(s) not Re-Computed.  You should Compute-Tax before printing results." );
   return;
  }
 if (print_mode < 2)
  {
   get_formbox_text( printerformbox, printer_command, MaxFname );
   printf("Issuing: %s\n", printer_command);
   system( printer_command );
   cancelprintpopup( 0, 0 );
  }
 else
  { /*Fill-out_PDF*/
   cancelprintpopup( 0, 0 );
   pdf_conversion_step = 1;
   schedule_PDF_conversion = 1;
   create_status_popup_window( 800, 125 );
  }
}


void setpdfoutputname( char *origname, char *suffix, char *newname )
{
 int j;
 strcpy( newname, origname );
 j = strlen( newname ) - 1;
 while ((j >= 0) && (newname[j] != '.')) j--;
 if (j >= 0) 
  newname[j] = '\0';
 strcat( newname, suffix );  
}


void prepare_universal_pdf_cmd( char *options, char *metadata, char *wrkingfname, char *markedpdf, char *outputname )
{ char *tmpmetadata, *tmpwrkingfname, *tmpmarkedpdf, *tmpout;

 tmpmetadata = (char *)malloc(4096);
 tmpwrkingfname = (char *)malloc(4096);
 tmpmarkedpdf = (char *)malloc(4096);
 tmpout = (char *)malloc(4096);

 strcpy( fillout_pdf_command, modify_pdf_tool );
 strcat( fillout_pdf_command, " " );
 strcat( fillout_pdf_command, options );
 strcat( fillout_pdf_command, " " );
 strcpy( tmpmetadata, ots_path );
 strcat( tmpmetadata, "src" );  strcat( tmpmetadata, slashstr );  
 strcat( tmpmetadata, "formdata" ); strcat( tmpmetadata, slashstr );
 strcat( tmpmetadata, metadata );
 quote_file_name( tmpmetadata );
 strcpy( tmpwrkingfname, wrkingfname );
 quote_file_name( tmpwrkingfname );
 strcpy( tmpmarkedpdf, ots_path );
 strcat( tmpmarkedpdf, "src" );  strcat( tmpmarkedpdf, slashstr );  
 strcat( tmpmarkedpdf, "formdata" ); strcat( tmpmarkedpdf, slashstr );
 strcat( tmpmarkedpdf, markedpdf );
 quote_file_name( tmpmarkedpdf );
 strcat( fillout_pdf_command, tmpmetadata );
 strcat( fillout_pdf_command, " " );
 strcat( fillout_pdf_command, tmpwrkingfname );
 strcat( fillout_pdf_command, " " );
 strcat( fillout_pdf_command, tmpmarkedpdf );
 if (strlen(outputname) > 0)
  {
   strcat( fillout_pdf_command, " -o " );
   strcpy( tmpout, outputname );
   quote_file_name( tmpout );
   strcat( fillout_pdf_command, tmpout );
  }
 free( tmpmetadata );
 free( tmpwrkingfname );
 free( tmpmarkedpdf );
 free( tmpout );
}


#ifndef MY_TMPNAM_DEFINED
#define MY_TMPNAM_DEFINED 1
char *my_tmp_dir=0, *my_tmp_suffix=".bat";

int my_tmpnam( char *fname )    /* Get a temporary file name for writing.  Return 1 on success, 0 on failure. */
{
 int k=0;
 struct stat buf;

 if (my_tmp_dir == 0)
  {
   #ifdef __MINGW32__
    char *sptr;  int j;
    sptr = getenv( "TEMP" );
    if ((sptr == 0) || (sptr[0] == '\0'))
     my_tmp_dir = strdup("");
    else
     {
      my_tmp_dir = (char *)malloc( strlen( sptr ) + 2 );
      strcpy( my_tmp_dir, sptr );
      j = strlen( my_tmp_dir );
      if ((my_tmp_dir[j-1] != '/') && (my_tmp_dir[j-1] != '\\'))
        strcat( my_tmp_dir, "\\" );
     }
   #else
    my_tmp_dir = strdup( "/tmp/" );
   #endif
  }
 do
  { char cnt[20];
   strcpy( fname, my_tmp_dir );
   strcat( fname, "tmpfile" );
   sprintf(cnt,"%d", k++ );
   strcat( fname, cnt );
   if (my_tmp_suffix == 0)
    strcat( fname, ".dat" );
   else
    strcat( fname, my_tmp_suffix );
  }
 while ((stat( fname, &buf ) == 0) && (k < 10000));
 if (k == 10000) return 0;
 else return 1;
}
#endif



void execute_cmd( char *cmd )
{
 #if (PLATFORM_KIND==Posix_Platform)
   system( fillout_pdf_command );
 #else
	char tmpfname[1024];
	FILE *T;	/* Due to limitation of microsoft, we must use a temp-cmd file on MSwin. */
	my_tmpnam( tmpfname );
	T = fopen(tmpfname,"w");
	fprintf(T,"%s\n", fillout_pdf_command );
	fclose(T);
	system( tmpfname );
	remove( tmpfname );
	// system( "erase tmpcmd.bat" );	/* remove does not work for some reason. */
 #endif
}


void do_pdf_conversion()
{  char outputname[4096];
   schedule_PDF_conversion = 0;
   predict_output_filename( current_working_filename, wrkingfname );
   if (strlen(fillout_pdf_tool) < 1) set_pdf_tool_path();
   switch (selected_form)
    {
     case form_US_1040:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "f1040_meta.dat", wrkingfname, "f1040_pdf.dat", outputname );
	printf("\nIssuing: %s\n", fillout_pdf_command );
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	pdf_conversion_step = 0;
	update_status_label( "Completed Filling-out PDF Forms:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_US_1040_Sched_C:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "f1040sc_meta.dat", wrkingfname, "f1040sc_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	update_status_label( "Completed Filling-out PDF Form:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_PA_40:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "PA_40_meta.dat", wrkingfname, "PA_40_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	update_status_label( "Completed Filling-out PDF Form:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_CA_540:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "CA_540_meta.dat", wrkingfname, "CA_540_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	pdf_conversion_step = 0;
	update_status_label( "Completed Filling-out PDF Forms:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );        statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_OH_IT1040:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "OH_PIT_IT1040_meta.dat", wrkingfname, "OH_PIT_IT1040_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	update_status_label( "Completed Filling-out PDF Form:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_VA_760:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "VA_760_meta.dat", wrkingfname, "VA_760_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	update_status_label( "Completed Filling-out PDF Form:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_NJ_1040:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "NJ_1040_meta.dat", wrkingfname, "NJ_1040_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	update_status_label( "Completed Filling-out PDF Form:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_NY_IT201:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "NY_it201_meta.dat", wrkingfname, "NY_it201_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	update_status_label( "Completed Filling-out PDF Form:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_MA_1:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "MA_1_meta.dat", wrkingfname, "MA_1_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	update_status_label( "Completed Filling-out PDF Form:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_NC_D400:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "NC_meta.dat", wrkingfname, "NC_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	update_status_label( "Completed Filling-out PDF Form:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_1040e:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "f1040e_meta.dat", wrkingfname, "f1040e_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	update_status_label( "Completed Filling-out PDF Form:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_4562:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "f4562_meta.dat", wrkingfname, "f4562_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_8582:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "f8582_meta.dat", wrkingfname, "f8582_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	update_status_label( "Completed Filling-out PDF Form:" );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case form_other:
     default: 
	if (strstr( taxsolvestrng, "taxsolve_HSA_f8889" ) != 0)
	 {
	  statusw.nfiles = 0;
	  setpdfoutputname( wrkingfname, ".pdf", outputname );
	  prepare_universal_pdf_cmd( "", "f8889_meta.dat", wrkingfname, "f8889_pdf.dat", outputname );
	  printf("Issuing: %s\n", fillout_pdf_command );
	  add_status_line( outputname );
	  execute_cmd( fillout_pdf_command );
	  update_status_label( "Completed Filling-out PDF Form:" );
	  statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	  add_view_pdf_button();
	 }
	else
	if (strstr( taxsolvestrng, "taxsolve_f8606" ) != 0)
	 {
	  statusw.nfiles = 0;
	  setpdfoutputname( wrkingfname, ".pdf", outputname );
	  prepare_universal_pdf_cmd( "", "f8606_meta.dat", wrkingfname, "f8606_pdf.dat", outputname );
	  printf("Issuing: %s\n", fillout_pdf_command );
	  add_status_line( outputname );
	  execute_cmd( fillout_pdf_command );
	  update_status_label( "Completed Filling-out PDF Form:" );
	  statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	  add_view_pdf_button();
	 }
	else
	if (strstr( taxsolvestrng, "taxsolve_US_1040_Sched_SE" ) != 0)
	 {
	  statusw.nfiles = 0;
	  setpdfoutputname( wrkingfname, ".pdf", outputname );
	  prepare_universal_pdf_cmd( "", "f1040sse_meta.dat", wrkingfname, "f1040sse_pdf.dat", outputname );
	  printf("Issuing: %s\n", fillout_pdf_command );
	  add_status_line( outputname );
	  execute_cmd( fillout_pdf_command );
	  update_status_label( "Completed Filling-out PDF Form:" );
	  statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	  add_view_pdf_button();
	 }
	else
	if (strstr( taxsolvestrng, "taxsolve_f8959" ) != 0)
	 {
	  statusw.nfiles = 0;
	  setpdfoutputname( wrkingfname, ".pdf", outputname );
	  prepare_universal_pdf_cmd( "", "f8959_meta.dat", wrkingfname, "f8959_pdf.dat", outputname );
	  printf("Issuing: %s\n", fillout_pdf_command );
	  add_status_line( outputname );
	  execute_cmd( fillout_pdf_command );
	  update_status_label( "Completed Filling-out PDF Form:" );
	  statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	  add_view_pdf_button();
	  }
	else
	if (strstr( taxsolvestrng, "taxsolve_f8960" ) != 0)
	 {
	  statusw.nfiles = 0;
	  setpdfoutputname( wrkingfname, ".pdf", outputname );
	  prepare_universal_pdf_cmd( "", "f8960_meta.dat", wrkingfname, "f8960_pdf.dat", outputname );
	  printf("Issuing: %s\n", fillout_pdf_command );
	  add_status_line( outputname );
	  execute_cmd( fillout_pdf_command );
	  update_status_label( "Completed Filling-out PDF Form:" );
	  statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	  add_view_pdf_button();
	  }
	else
	if (strstr( taxsolvestrng, "taxsolve_f2210" ) != 0)
	 {
	  statusw.nfiles = 0;
	  setpdfoutputname( wrkingfname, ".pdf", outputname );
	  prepare_universal_pdf_cmd( "", "f2210_meta.dat", wrkingfname, "f2210_pdf.dat", outputname );
	  printf("Issuing: %s\n", fillout_pdf_command );
	  add_status_line( outputname );
	  execute_cmd( fillout_pdf_command );
	  update_status_label( "Completed Filling-out PDF Form:" );
	  statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	  add_view_pdf_button();
	  }
	else
	if (strstr( taxsolvestrng, "taxsolve_CA_5805" ) != 0)
	 {
	  statusw.nfiles = 0;
	  setpdfoutputname( wrkingfname, ".pdf", outputname );
	  prepare_universal_pdf_cmd( "", "CA_5805_meta.dat", wrkingfname, "CA_5805_pdf.dat", outputname );
	  printf("Issuing: %s\n", fillout_pdf_command );
	  add_status_line( outputname );
	  execute_cmd( fillout_pdf_command );
	  update_status_label( "Completed Filling-out PDF Form:" );
	  statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	  add_view_pdf_button();
	  }
	else
	 {
	  printf("Form type not supported.\n");
	  make_button( status_panel, 30, statusw.ht - 50, " Ok ", dismiss_status_win, &status_win );
	 }
    }
}


void print_outfile_directly( GtkWidget *wdg, void *data )
{
 // predict_output_filename( current_working_filename, wrkingfname );
 // sprintf(printer_command,"%s \"%s\"", base_printer_command, wrkingfname );
 // printf("Issuing: %s\n", printer_command);
 // system( printer_command );
 printout( wdg, data );
}

void create_pdf_file_directly( GtkWidget *wdg, void *data )
{
 predict_output_filename( current_working_filename, wrkingfname );
 pdf_conversion_step = 1;
 schedule_PDF_conversion = 1;
 create_status_popup_window( 800, 125 );
}


void printout( GtkWidget *wdg, void *data )
{
 GtkWidget *rad, *panel; 
 int x1=70, x2, y1=30, dy=20, wd=750, ht=190, printht;

 Update_box_info();
 if (save_needed)
  {
   if (!computed)
    GeneralWarning( "Change(s) not saved.  You should Save input file before printing it." );
   else
    GeneralWarning( "Change(s) not saved.  You should Save + Re-Compute before printing." );
   return;
  }
 dismiss_status_win( 0, 0 );
 print_mode = 0;
 panel = new_window( wd, ht, "Print Return Data", &printpopup );
 make_sized_label( panel, 1, 1, "Print Return Data:", 12 );
 rad = make_radio_button( panel, 0, x1, y1, "Print Input Data", togprntcmd_in, 0 );

 if (supported_pdf_form)
  {
    make_radio_button( panel, rad, x1 + 170, y1, "Automatically Fill-out PDF Tax-Form", togprntcmd_pdf, 0 );
    x2 = x1 + 460;
  }
 else x2 = x1 + 160;

 y1 = y1 + dy;
 rad = make_radio_button( panel, rad, x1, y1, "Print Output Data", togprntcmd_out, 0 );
 make_rectangular_separator( panel, x1 - 10, y1 - dy - 10, x2, y1 + dy );
 y1 = y1 + dy + 15;
 print_label = make_label( panel, 10, y1 + 4, "Print Command:" );
 y1 = y1 + dy;
 printdialogsetup = 0;
 strcpy( printer_command, base_printer_command );
 strcat( printer_command, " " );
 if (computed)
  {
   set_radio_button( rad );
   predict_output_filename( current_working_filename, wrkingfname );
   strcat( printer_command, wrkingfname );
   print_mode = 1;
  }
 else
  strcat( printer_command, current_working_filename );
 x1 = 20;
 printerformbox = new_formbox_bypix( panel, x1, y1, wd - x1 - 10, printer_command, MaxFname, acceptprinter_command, 0 ); 
 printht = ht - 35;
 print_button = make_button( panel, 50, printht, "  Print It  ", acceptprinter_command, 0 );
 make_button( panel, wd - 95, printht, "Cancel", cancelprintpopup, 0 );
 show_wind( printpopup );
 printdialogsetup = 1;
}





void slcttxprog( GtkWidget *wdg, void *data )
{
 char *sel=(char *)data;
 char strg[MaxFname], tmpstr[MaxFname];
 int k, prev;

 if (!ok_slcttxprog) return;
 prev = selected_form;
 if (sscanf(sel,"%d", &selected_form) != 1) printf("Internal error '%s'\n", sel );
 if (selected_form == prev) return;
 strcpy( strg, program_names[selected_form] );
 if (strcmp(strg,"Other")==0)
  {
   selected_other = 1;
   supported_pdf_form = 0;
   if (verbose) printf("invocation_path = '%s'\n", invocation_path );
   set_invocation_path( toolpath );
   fb_clear_banned_files();
   fb_ban_files( ".txt" );
   fb_ban_files( ".pdf" );
   fb_ban_files( ".dll" );
   fb_ban_files( ".dylib" );
   fb_ban_files( "convert_results2xfdf" );
   fb_ban_files( "ots_gui" );
   fb_ban_files( "universal_pdf_file_modifier" );

   fb_ban_files( "taxsolve_CA_540" );
   fb_ban_files( "taxsolve_MA_1" );
   fb_ban_files( "taxsolve_NC_D400" );
   fb_ban_files( "taxsolve_NJ_1040" );
   fb_ban_files( "taxsolve_NY_IT201" );
   fb_ban_files( "taxsolve_OH_IT1040" );
   fb_ban_files( "taxsolve_PA_40_" );
   fb_ban_files( "taxsolve_US_1040_2021");
   fb_ban_files( "taxsolve_US_1040_Sched_C_" );
   fb_ban_files( "taxsolve_VA_760_" );

   strcpy( wildcards_fb, "" );
   strcpy( filename_fb, "" );
   // printf("OTS_taxsolve: dir='%s', wc='%s', fname='%s'\n", toolpath, wildcards_fb, filename_fb );
   Browse_Files( "Select Tax Program to Use:", 2048, toolpath, wildcards_fb, filename_fb, set_tax_solver );

   strcpy(tmpstr, invocation_path);
   k = strlen(tmpstr)-1;
   while ((k > 0) && (tmpstr[k] != slashchr)) k--;
   if (k > 0) k--;
   while ((k > 0) && (tmpstr[k] != slashchr)) k--;
   if (tmpstr[k] == slashchr)  tmpstr[k+1] = '\0';
   else  {sprintf(tmpstr,".%c", slashchr);}
   sprintf(directory_dat, "%stax_form_files%c", tmpstr, slashchr);

   return;
  }
 else
  {
   selected_other = 0;
   supported_pdf_form = 1;
   sprintf(tmpstr,"%s%s", invocation_path, strg);
   printf("Setting Tax Program to be: '%s'\n", tmpstr);
   taxsolvecmd = strdup(tmpstr);
   strcpy(taxsolvestrng, tmpstr);

   strcpy(tmpstr, invocation_path);
   k = strlen(tmpstr)-1;
   while ((k > 0) && (tmpstr[k] != slashchr)) k--;
   if (k > 0) k--;
   while ((k > 0) && (tmpstr[k] != slashchr)) k--;
   if (tmpstr[k] == slashchr)  tmpstr[k+1] = '\0';
   else  {sprintf(tmpstr,".%c", slashchr);}
   sprintf(directory_dat, "%stax_form_files%c", tmpstr, slashchr);

   sel = strstr( strg, "_2021" );
   if (sel != 0)
     sel[0] = '\0';
   strcpy( tmpstr, strg );
   if (strlen(tmpstr) < 10) {printf("Internal error, name too short '%s'\n", tmpstr); return;}
   strcpy( strg, &(tmpstr[9]) );	/* Remove the prefix "taxsolve_". */
   strcat( directory_dat, strg );
   printf("Setting Tax Data Directory to be: '%s'\n", directory_dat);
  }
}



#include "logo_image.c"   /* <--- The image-data is included here, produced by "data2code". */


void quit( GtkWidget *wdg, void *x )
{
 printf("OTS Exiting.\n");
 exit(0);
}


void warn_about_save_needed()
{
 int xpos=20, ypos=20, winwdth, winhght=100;
 GtkWidget *winframe, *label;
 winwdth = 300;
 winframe = new_window( winwdth, winhght, "Warning Message", &warnwin );
 label = make_sized_label( winframe, xpos, ypos, "<b>Change(s) not saved !!</b>", 12 );
 set_widget_color( label, "#ff0000" );
 make_button( winframe, 10, winhght - 40, "Exit anyway, without saving", quit, &warnwin );
 make_button( winframe, winwdth - 60, winhght - 40, "Cancel", dismiss_general_warning, &warnwin );
 gtk_window_set_keep_above( (GtkWindow *)warnwin, 1 );
 show_wind( warnwin );
}


void quit_wcheck( GtkWidget *wdg, void *x )
{
 Update_box_info();
 if (save_needed)
  {
   warn_about_save_needed();
   return;
  }
 quit( 0, 0 );
}



static gboolean on_expose_event( GtkWidget *widget, GdkEventExpose *event, gpointer data )
{
 unsigned char *logodata;
 int imgwd, imght, x1, y1, x2, y2, new_width, new_height;

 if (operating_mode == 1)
  { /*mode1*/
   logodata = cdti_convert_data_to_image( data_0, data_0_size, &imgwd, &imght );
   x1 = (winwidth - imgwd) / 2;
   y1 = 10;
   x2 = x1 + imgwd;
   y2 = y1 + imght;
   place_image( mpanel, imgwd, imght, x1, y1, logodata );
   free( logodata );
   // make_rectangular_separator( mpanel, x1 - 2, y1 - 2, x2 - 2, y2 + 2 );
   { /* Place a border around the logo image. */
    cairo_t *canvas=0;
    canvas = gdk_cairo_create( mpanel->window );
    cairo_set_line_width( canvas, 2.0 );
    cairo_set_source_rgb( canvas, 0.1, 0.1, 0.2 );
    x1 = x1 - 2;  y1 = y1 - 2;
    x2 = x2 + 2;  y2 = y2 + 2;
    cairo_move_to( canvas, x1, y1 );
    cairo_line_to( canvas, x2, y1 );
    cairo_line_to( canvas, x2, y2 );
    cairo_line_to( canvas, x1, y2 );
    cairo_line_to( canvas, x1, y1 );
    cairo_stroke( canvas );
    x1 = 20;   	   	x2 = winwidth - 20;
    y1 = fronty1 - 5;	y2 = fronty2;
    cairo_set_line_width( canvas, 1.0 );
    cairo_set_source_rgb( canvas, 0.1, 0.1, 0.5 );
    cairo_move_to( canvas, x1, y1 );
    cairo_line_to( canvas, x2, y1 );
    cairo_line_to( canvas, x2, y2 );
    cairo_line_to( canvas, x1, y2 );
    cairo_line_to( canvas, x1, y1 );
    cairo_stroke( canvas );
    cairo_destroy(canvas);
   }
  } /*mode1*/
 else
  { /*mode2*/
   gtk_window_get_size( GTK_WINDOW( outer_window ), &new_width, &new_height );
   if ((new_width != winwidth) || (new_height != winht))
    {
     winwidth = new_width;
     winht = new_height;
     need_to_resize = 1;
     last_resize_time = Report_Time();
    }
  } /*mode2*/
 return FALSE;
}



int myfilterfunc( char *word )
{
 if (my_strcasestr( word, ".txt") != 0)
  {
   if (my_strcasestr( word, "_out.txt" ) != 0) return 0;
   if (my_strcasestr( word, "README_" ) != 0) return 0;
   return 1; 
  }
 else return 0;
}


//gboolean filterfunc( const GtkFileFilterInfo *finfo, gpointer data )
//{
 // printf("Got '%s'\n", finfo->filename );
// return myfilterfunc( finfo->filename );
//}


void receive_filename( char *fname )
{
 // printf("OTS_pick_file RET: f='%s', dir='%s', wc='%s', fname='%s'\n", fname, directory_dat, wildcards_fb, filename_fb );
 if (yourfilename != 0) free( yourfilename );
 yourfilename = strdup( fname );
 printf("You picked '%s'\n", yourfilename );
 if (myfilterfunc( yourfilename ) == 0)
  {
   warn_release = 1;
   GeneralWarning("File name does not look like a tax input file.");
  }
 else
  {
   open_taxfile( yourfilename );
  }
}


void pick_file( GtkWidget *wdg, void *data )
{ int j;
  GtkFileFilter *rule;

  printf("Selected_Form = %d\n", selected_form );
  rule = gtk_file_filter_new();
  gtk_file_filter_add_pattern( rule, "*.txt" );
  // gtk_file_filter_add_custom( rule, 15, filterfunc, 0, 0 );

  j = strlen( directory_dat );
  if ((j > 0) && (directory_dat[j-1] != slashchr))
   strcat( directory_dat, slashstr );
  if (verbose) printf("BrowsePath = '%s'\n", directory_dat );
  fb_clear_banned_files();
  fb_ban_files( "_out.txt" );
  fb_ban_files( "_template.txt" );
  fb_ban_files( "README" );
  strcpy( wildcards_fb, ".txt" );
  strcpy( filename_fb, "" );
  // printf("OTS_pick_file: dir='%s', wc='%s', fname='%s'\n", directory_dat, wildcards_fb, filename_fb );
  Browse_Files( "Select File", 2048, directory_dat, wildcards_fb, filename_fb, receive_filename );
}


void pick_template( GtkWidget *wdg, void *data )
{ int j;
  GtkFileFilter *rule;
  DIR *dirpt;
  struct dirent *dir_entry;
  int match=0;
  char *templatefile="", *fileanswer;

  printf("Selected_Form = %d\n", selected_form );
  rule = gtk_file_filter_new();
  gtk_file_filter_add_pattern( rule, "*.txt" );
  // gtk_file_filter_add_custom( rule, 15, filterfunc, 0, 0 );

  j = strlen( directory_dat );
  if ((j > 0) && (directory_dat[j-1] != slashchr))
   strcat( directory_dat, slashstr );
  if (verbose) printf("BrowsePath = '%s'\n", directory_dat );
  fb_clear_banned_files();
  fb_ban_files( "_out.txt" );
  fb_ban_files( "README" );
  strcpy( wildcards_fb, "_template.txt" );
  strcpy( filename_fb, "" );
  // printf("OTS_pick_template: dir='%s', wc='%s', fname='%s'\n", directory_dat, wildcards_fb, filename_fb );

  /* Expect only a single file in the named directory to match. */
  /* If so, then call "receive_filename" on that file. Otherwise drop through to the File-Browser. */
  dirpt = opendir( directory_dat );
  if (dirpt != 0)
   {
    dir_entry = readdir(dirpt);
    while (dir_entry != 0)
     {
      if (strstr( dir_entry->d_name, "_template.txt" ) != 0)
	{
	 match++;
	 templatefile = strdup( dir_entry->d_name );
	}
      dir_entry = readdir(dirpt);
     }
    closedir( dirpt );
    if (match == 1)
     {
	printf(" Dir = '%s', File = '%s'\n", directory_dat, templatefile );
	fileanswer = (char *)malloc( strlen( directory_dat ) + strlen( templatefile ) + 100 );
	strcpy( fileanswer, directory_dat );
	strcat( fileanswer, "/" );
	strcat( fileanswer, templatefile );
	fb_reduce_pathname( fileanswer );
	printf("Directly opening: '%s'\n", fileanswer );
	receive_filename( fileanswer );
	return;
     }
   }

  Browse_Files( "Select File", 2048, directory_dat, wildcards_fb, filename_fb, receive_filename );
}


void set_ots_path()
{ /* Expect invocation path to end with "bin" or "msbin", and remove that part. */
 int j;
 ots_path = strdup( invocation_path );
 j = strlen( ots_path ) - 1;
 while ((j >= 0) && (strstr( &(ots_path[j]), "bin" ) != &(ots_path[j]))) j--;
 if (j < 0) { ots_path = strdup( "./" );  ots_path[1] = slashchr; }
 else
 if ((j > 1) && (ots_path[j-1] == 's')) ots_path[j-2] = '\0';
 else ots_path[j] = '\0';
}


void helpabout1( GtkWidget *wdg, void *data )
{
 char msg[4096];
 sprintf( msg, "OpenTaxSolver (OTS) GUI - Version %1.2f,  %s\n", version, package_date );
 strcat( msg, "                For the 2021 Tax Year.    OTS release ");
 strcat( msg, ots_release_package );   strcat( msg, "\n\n" );
 strcat( msg, "Use this GUI to open tax-forms and calculate taxes.\n");
 strcat( msg, " 1. First select a tax-form to do from the available programs listed.\n" ); 
 strcat( msg, " 2. Then to start a new blank return click 'Start New Return'.\n");
 strcat( msg, "     Or to open a file you previously saved, or a working example,\n");
 strcat( msg, "      click 'Open Saved Form' button, select your file or example,\n" ); 
 strcat( msg, "      and click 'Ok'\n");
 strcat( msg, " 3. Fill out the form that pops up.\n");
 strcat( msg, "     (Click the label on any line to see tax-instructions, if provided.)\n" ); 
 strcat( msg, " 4. Save your filled-out form to a name of your choice.\n");
 strcat( msg, " 5. Click 'Compute Tax' to see your results.\n");
 strcat( msg, " 6. Click 'Print' to fill-out or print-out your forms.\n\n");
 strcat( msg, "For help, additional information, and updates:\n" );
 strcat( msg, " Surf to:   http://opentaxsolver.sourceforge.net/\n" );
 GeneralPopup( "OTS Information", msg, 1 );
}


void helpabout2( GtkWidget *wdg, void *data )
{
 char msg[4096];
 sprintf( msg, "OpenTaxSolver (OTS) GUI - Version %1.2f, %s\n", version, package_date );
 strcat( msg, "                For the 2021 Tax Year.    OTS release ");
 strcat( msg, ots_release_package );   strcat( msg, "\n\n" );
 strcat( msg, "Use this GUI to fill-out tax forms and calculate taxes.\n");
 strcat( msg, "  1. Fill-out the line entries that apply to you.\n");
 strcat( msg, "     (Click the label on any line to see tax-instructions, if provided.)\n" ); 
 strcat( msg, "  2. Save your filled-out form by clicking 'Save' button.\n");
 strcat( msg, "      (If you started a new form ('_template'), then save your\n");
 strcat( msg, "       version with a unique name that is meaningful to you.)\n");
 strcat( msg, "  3. Click 'Compute Tax' to see results.\n");
 strcat( msg, "  4. Click 'Print' to print-out the results or to automatically\n");
 strcat( msg, "       fill-out the final forms.\n\n"); 
 strcat( msg, "For help, additional information, and updates:\n" );
 strcat( msg, " Surf to:   http://opentaxsolver.sourceforge.net/\n" );
 GeneralPopup( "OTS Information", msg, 1 );
}



/*----------------------------------------------------------------------------*/
/* Main -								      */
/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[] )
{
 int argn, k, grayed_out=0, setwinsz=0;
 char vrsnmssg[256], ots_pkg_mssg[256], tmpstr[MaxFname], *formid;
 float x, y, dy, y1, y2;
 GtkWidget *txprogstog, *button, *tmpwdg;

 sprintf(ots_pkg_mssg, "OTS Release %s", ots_release_package );  printf("%s\n\n", ots_pkg_mssg );
 sprintf(vrsnmssg, "GUI v%1.2f", version );  printf("%s\n", vrsnmssg );
 start_cmd = strdup(argv[0]);
 invocation_path = strdup(argv[0]);
 k = strlen(invocation_path)-1;
 while ((k>0) && (invocation_path[k]!=slashchr)) k--;
 if (invocation_path[k]==slashchr) k++;
 invocation_path[k] = '\0';
 // printf("Invocation path = '%s'\n", invocation_path);
 set_ots_path();

 /* Decode any command-line arguments. */
 argn = 1;  k=1;
 while (argn < argc)
 {
  if (strcmp(argv[argn],"-verbose")==0)
   { 
    verbose = 1;
   }
  else
  if (strcmp(argv[argn],"-help")==0)
   { 
    printf("OTS GUI v%1.2f, %s:\n", version, package_date );
    printf(" Command-line Options:\n");
    printf("  -verbose          - Show debugging messages.\n");
    printf("  -winsz wd ht      - Set the window size to wd x ht.\n");
    printf("  -debug            - Set debug mode.\n");
    printf("  -taxsolver xx     - Set path and name of the tax-solver executable.\n");
    printf("  {file-name}.txt   - Set path and name of the tax data input file.\n\n");
    exit(0);
   }
  else
  if (strcmp(argv[argn],"-taxsolver")==0)
   { int kx;
    argn++;
    if (argn == argc) { printf("Missing entry after '-taxsolver'.\n");  exit(1); }
    taxsolvecmd = strdup( argv[argn] );
    strcpy( taxsolvestrng, taxsolvecmd );
    selected_other = 1;
    if (verbose) printf("invocation_path = '%s'\n", invocation_path );

    strcpy(tmpstr, invocation_path);
    kx = strlen(tmpstr)-1;
    while ((kx > 0) && (tmpstr[kx] != slashchr)) kx--;
    if (kx > 0) kx--;
    while ((kx > 0) && (tmpstr[kx] != slashchr)) kx--;
    if (tmpstr[kx] == slashchr)  tmpstr[kx+1] = '\0';
    else  {sprintf(tmpstr,".%c", slashchr);}
    sprintf(directory_dat, "%stax_form_files%c", tmpstr, slashchr);
    selected_form = form_other;
    ok_slcttxprog = 0;
   }
  else
  if (strcmp(argv[argn],"-winsz")==0)
   { 
    argn++;
    if (argn == argc) { printf("Missing entry after '%s'.\n", argv[argn-1] );  exit(1); }
    if (sscanf(argv[argn],"%d",&winwidth) != 1)
     { printf("Bad integer window-width after '%s'.\n", argv[argn-1] );  exit(1); }
    argn++;
    if (argn == argc) { printf("Missing entry after '%s'.\n", argv[argn-2] );  exit(1); }
    if (sscanf(argv[argn],"%d",&winht) != 1)
     { printf("Bad integer window-height after '%s'.\n", argv[argn-2] );  exit(1); }
    printf(" Setting window size = %d x %d\n", winwidth, winht );
    setwinsz = 1;
   }
  else
  if (strcmp( argv[argn], "-debug" ) == 0)
   { 
    debug = 1;
    #if (PLATFORM_KIND==Posix_Platform)
	system( "pwd" );
    #else
	system( "dir" );
    #endif
   }
  else
  if (k==1)
   {
    current_working_filename = strdup(argv[argn]);
    infile = fopen(current_working_filename,"r");
    if (infile==0) {printf("ERROR: Parameter file '%s' could not be opened.\n", argv[argn]); exit(1);}
    k = 2;
    ok_slcttxprog = 0;
   }
  else
   {
    printf("Unknown command-line parameter '%s'\n", argv[argn]); 
    /* exit(1); */ 
   }
  argn = argn + 1;
 }

 mpanel = init_top_outer_window( &argc, &argv, winwidth, winht, "OpenTaxSolver-GUI", 0, 0 );
 if (!setwinsz)
  {
   int swd, sht;
   GdkScreen *scrn;
   scrn = gtk_window_get_screen( (GtkWindow *)outer_window );
   swd = gdk_screen_get_width( scrn );
   if (swd > 2500)
    { /*Hi-DPI Screen*/
     sht = gdk_screen_get_height( scrn );
     printf("Screen size = %d x %d\n", swd, sht);
     if (swd > 3840) swd = 3840;	/* Limit reported screen size to realistic values. */
     if (sht > 2160) sht = 2160;
     winwidth = (int)((float)winwidth * (float)swd / 1920.0);
     winht = (int)((float)winht * (float)sht / 1080.0);
     printf("Detected HiDPI Screen.  Setting window size = %d x %d\n", winwidth, winht );
     gtk_widget_set_size_request( outer_window, winwidth, winht );
    }
  }

 gtk_window_set_resizable( GTK_WINDOW( outer_window ), 0 );
 // make_sized_label( mpanel, 180, 10, "Open-Tax-Solver", 20.0 );

 /* When the window is given the "delete_event" signal by the window manager, exit the program. */
 gtk_signal_connect( GTK_OBJECT(outer_window), "delete_event", GTK_SIGNAL_FUNC(quit), NULL );
 // g_signal_connect( GTK_WINDOW(outer_window), "destroy", quit, NULL);

 gtk_widget_set_app_paintable( outer_window, TRUE );
 g_signal_connect( outer_window, "expose-event", G_CALLBACK(on_expose_event), NULL);

 make_rectangular_separator( mpanel, 59, 6, 387, 102 );

 y = 105;
 make_sized_label( mpanel, winwidth / 2 - 60, y, "2021 Tax Year", 11.0 );
 y = y + 35;
 make_sized_label( mpanel, 10, 135, "Select Tax Program:", 12.0 );

 x = 30;
 y = y + 25;
 y1 = y;
 dy = ((winht - 120) - y) / 6;
 formid = setform( form_US_1040 );
 txprogstog = make_radio_button( mpanel, 0, x, y, "US 1040 (w/Scheds A,B,D)", slcttxprog, formid );
 add_tool_tip( txprogstog, "Also does the 8949 forms." );
 // gtk_widget_set_sensitive( txprogstog, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 formid = setform( form_US_1040_Sched_C );
 make_radio_button( mpanel, txprogstog, x, y, "US 1040 Sched C", slcttxprog, formid );
 y = y + dy;
 formid = setform( form_CA_540 );
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "CA State 540", slcttxprog, formid );
 // gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 formid = setform( form_NC_D400 );
 make_radio_button( mpanel, txprogstog, x, y, "NC State DC400", slcttxprog, formid );
 y = y + dy;
 formid = setform( form_NJ_1040 );
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "NJ State 1040", slcttxprog, formid );
 // gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 formid = setform( form_GA_500 );
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "GA State 500", slcttxprog, formid );
 gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */

 y = y1;
 x = winwidth/2 + 40;
 formid = setform( form_OH_IT1040 );
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "OH State IT1040", slcttxprog, formid );
 // gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 formid = setform( form_PA_40 );
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "PA State 40", slcttxprog, formid );
 // gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 formid = setform( form_VA_760 );
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "VA State 760", slcttxprog, formid );
 // gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 formid = setform( form_NY_IT201 );
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "NY State IT201", slcttxprog, formid );
 // gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 formid = setform( form_MA_1 );
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "MA State 1", slcttxprog, formid );
 // gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 formid = setform( form_other );
 txprogstog = make_radio_button( mpanel, txprogstog, x, y, "Other", slcttxprog, formid );
 y2 = y + dy;
 if (selected_other) set_radio_button( txprogstog );

 fronty1 = y1;
 fronty2 = y2;
 // make_rectangular_separator( mpanel, 20, y1-5, winwidth - 20, y2 );

 slcttxprog( 0, "0" );	/* Set default tax program. */

 if (infile == 0) 
  {
   button = make_button_wsizedcolor_text( mpanel, 30, winht - 100, "Start New Return", 14.0, "#000000", pick_template, 0 );
   add_tool_tip( button, "Start a fresh new blank return of the selected type." );

   button = make_button_wsizedcolor_text( mpanel, (int)(0.5222 * (float)winwidth), winht - 100, "Open Saved Form", 14.0, "#000000", pick_file, 0 );
   add_tool_tip( button, "Open a previously saved or existing, tax form or example." );
   make_sized_label( mpanel, winwidth / 2 - 25, winht - 30, vrsnmssg, 8 );
   make_sized_label( mpanel, winwidth / 2 - 45, winht - 14, ots_pkg_mssg, 7 );
  }
 else
   Get_New_Tax_Form_Page( current_working_filename );

 button = make_button( mpanel, 20, winht - 35, "Help", helpabout1, 0 );
 add_tool_tip( button, "Get information about this program, Help, and Updates." );
 button = make_button( mpanel, winwidth - 60, winht - 35, " Quit ", quit, 0 );
 add_tool_tip( button, "Leave this program." );

 ok_slcttxprog = 1;
 gtk_widget_show_all( outer_window );
 while (1)	 // gtk_main();
  {
   UpdateCheck();       	/* Check for, and serve, any pending GTK window/interaction events. */
   Sleep_seconds( 0.05 );       /* No need to spin faster than ~20 Hz update rate. */
   if ((need_to_resize) && (Report_Time() - last_resize_time > 0.35))
    {
	if (verbose) printf("	Resizing to (%d, %d)\n", winwidth, winht );
	Update_box_info();
	Setup_Tax_Form_Page(0);
	need_to_resize = 0;
    }
   if (schedule_PDF_conversion) do_pdf_conversion();
  }
 return 0;
}
