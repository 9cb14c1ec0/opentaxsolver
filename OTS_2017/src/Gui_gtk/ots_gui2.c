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

float version=2.23;
char package_date[]="Feb. 5, 2018";
char ots_release_package[]="15.02";

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
char directory_dat[MaxFname]=".", wildcards_dat[MaxFname]="*.txt", filename_incl[MaxFname]="";
char directory_incl[MaxFname]="examples_and_templates", wildcards_incl[MaxFname]="*_out.txt";
char directory_fb[MaxFname]="", wildcards_fb[MaxFname]="", filename_fb[MaxFname]="";
char *title_line="Tax File", *current_working_filename=0, *invocation_path, *include_file_name=0;
char wildcards_out[MaxFname]="*_out.txt";
int fronty1, fronty2, computed=0, ok_slcttxprog=1;
char *yourfilename=0;
char toolpath[MaxFname]="";
int pending_compute=0;

void pick_file( GtkWidget *wdg, void *data );	/* Prototype */
void consume_leading_trailing_whitespace( char *line );
void Run_TaxSolver( GtkWidget *wdg, void *x );
void helpabout2( GtkWidget *wdg, void *data );
void dump_taxinfo();
int warn_release=0;

int selected_form=11;
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

char program_names[30][100] = 
	{
	 "taxsolve_US_1040_2017",		/* 0 */
	 "taxsolve_US_1040_Sched_C_2017",	/* 1 */
	 "taxsolve_US_8829",			/* 2 */
	 "taxsolve_CA_540_2017",		/* 3 */
	 "taxsolve_NC_D400_2017",		/* 4 */
	 "taxsolve_NJ_1040_2017",		/* 5 */
	 "taxsolve_OH_IT1040_2017",		/* 6 */
	 "taxsolve_PA_40_2017",			/* 7 */
	 "taxsolve_VA_760_2017",		/* 8 */
	 "taxsolve_NY_IT201_2017",		/* 9 */
	 "taxsolve_MA_1_2017",			/* 10 */
	 "Other",				/* 11 */
	};

enum form_names { form_US_1040, form_US_1040_Sched_C, form_US_8829, form_CA_540, 
		  form_NC_D400, form_NJ_1040, form_OH_IT1040, form_PA_40,
		  form_VA_760, form_NY_IT201, form_MA_1, form_other
		};


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


void GeneralPopup( char *title, char *mesg )       /* Used for multi-line informational messages. */
{
 int xpos=20, ypos=20, winwdth, winhght, j=0, k=0, maxcols=0, nlines=0;
 GtkWidget *winframe;
 printf("%s\n", mesg);
 while (mesg[k] != '\0')	/* Determine max line width and number of rows. */
  {
   if ((mesg[k] == '\n') || (mesg[k] == '\0'))
    {
     if (j > maxcols) maxcols = j;
     j = 0;
     nlines++;
    }
   else
    j++;
   k++;
  }
 winwdth = 40 + maxcols * 7;
 winhght = 60 + 18 * nlines;
 winframe = new_window( winwdth, winhght, title, &warnwin );
 make_label( winframe, xpos, ypos, mesg );
 make_button( winframe, winwdth/2 - 30, winhght - 30, "  Ok  ", dismiss_general_warning, &warnwin );
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
  int linenum, format_offset;
  struct value_list *values_hd, *values_tl;	/* Head and tail list pointers for a tax-line-entry. */
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
    if (word[k-1]==';')
     { 
      if (k==1) { word[1] = '\0';  return SEMICOLON; }
      else { ungetc(word[k-1], infile); word[k-1] = '\0';  return VALUE_LABEL; }
     }
    else { word[k-1] = '\0';  return VALUE_LABEL; }
  } /*get_value_or_linelabel*/
}


/*--------------------------------------------------------------*/
/* Get_Line_Entry - Reads remainder of line from input file.	*/
/*--------------------------------------------------------------*/
void get_line_entry( char *word, int maxn, FILE *infile )
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
 word[k] = '\0';
 // printf("	k = %d, word[%d] = %d\n", k, k, word[k] );
 // printf("	word = '%s'\n", word );
 consume_leading_trailing_whitespace( word );
}


void DisplayTaxInfo();		/* This is a prototype statement only. */
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



/*--------------------------------------------------------------*/
/* Add_New_Boxes - Callback for "+" button on form-boxes.	*/
/*  Adds new form-box(s) to the line item.			*/
/*--------------------------------------------------------------*/
void add_new_boxes( void *data, int num )
{
 struct taxline_record *txline;
 struct value_list *item, *lineitem, *newitem1, *newitem2, *oldtail;
 double vpos;
 GtkAdjustment *adj;

 Update_box_info();

 item = (struct value_list *)data;
 oldtail = item->parent->values_tl;

 newitem1 = new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 1 );
 if (num>1)
  {
   new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 1 );
   new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 2 );
   newitem2 = new_list_item_value( VKIND_TEXT, item->parent, "", 0, item->linenum + 2 );
  }
 else newitem2 = newitem1;

 lineitem = item;	 /* Skip the items on the original line. */
 while ((lineitem->nxt!=newitem1) && (lineitem->nxt->linenum == item->linenum)) lineitem = lineitem->nxt;

 /* Insert in list and remove from tail, if not on end of list. */
 if (lineitem->nxt != newitem1)
  {
   newitem2->nxt = lineitem->nxt;
   lineitem->nxt = newitem1;
   oldtail->nxt = 0;  
   item->parent->values_tl = oldtail;   
  }

 /* Increment the effective file-line-number of all subsequent line entries. */
 lineitem = newitem2->nxt;
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
 add_new_boxes( data, 2 ); 
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



void quit_wcheck( GtkWidget *wdg, void *x );		/* Protoyypes */
void print_outfile_directly( GtkWidget *wdg, void *data );
void create_pdf_file_directly( GtkWidget *wdg, void *data );



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


#if (PLATFORM_KIND != Posix_Platform) 
 char slashchr='\\';
 char slashstr[]="\\";
#else
 char slashchr='/';
 char slashstr[]="/";
#endif


char *taxform_name;


/***********************/
/* Read Tax Data File. */
/***********************/
void Read_Tax_File( char *fname )
{
 int j, k, kind, state=0, column=0, linenum=0, linecnt=0, lastline=0, newentry=0, entrycnt=0;
 char word[10000], *tmpstr, tmpstr2[100], tmpstr3[100];
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
	  {
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
	      get_line_entry( word, 10000, infile );
	     else
	      word[0] = '\0';
	     tmppt = new_list_item_value( VKIND_TEXT, txline, word, column, linecnt );
	     tmppt->formtype = ID_INFO;	/* Special ID-only info lines. */
	     state = 0;
	    }
	  }
	 else
	  {
	   if (verbose) printf(" Value:	%s\n", word);
	   if (strcasecmp(txline->linename, "Status") == 0)
	    {
	     new_list_item_value( VKIND_TEXT, txline, word, column, linecnt );
	     state = 0;
	    }
	   else
	    {
	     tmppt = new_list_item_value( VKIND_TEXT, txline, word, column, linecnt );
	     if (strstr( txline->linename, ":" ) != 0)
		 tmppt->formtype = LITERAL_INFO;
	     entrycnt++;
	    }
	  }
	newentry++;
	break;
     case COMMENT: if (verbose) printf(" Comment:	%s\n", word);
	if (txline==0) txline = new_taxline("", linecnt);
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
	  new_list_item_value( VKIND_TEXT, txline, "", column, linecnt );
	  new_list_item_value( VKIND_TEXT, txline, "", column, linecnt++ );
	  new_list_item_value( VKIND_TEXT, txline, "", column, linecnt );
	  new_list_item_value( VKIND_TEXT, txline, "", column, linecnt++ );
	 }
	state = 0;
	new_list_item_value( VKIND_COLON, txline, word, column, linecnt );
	break;
    }
   column = column + strlen(word);
   kind = get_next_entry( word, 10000, &column, &linenum, infile );
   if (linenum > lastline) { lastline = linenum;  if (newentry) linecnt++;  newentry = 0; }
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
}



void Setup_Tax_Form_Page()	/* This is called whenever the form window needs to be redisplayed for any reason. */
{
 GtkWidget *button;
 GtkRequisition actual;
 int x1=1, xpos;
 char *twrd;
 float fontsz;

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

 xpos = (int)(0.2 * (float)winwidth + 0.5);
 // printf("Compute = %1.3g, ", (float)xpos / (float)winwidth );
 button = make_button( mpanel, xpos, winht - 35, "Compute Tax", Run_TaxSolver, 0 );
 add_tool_tip( button, "Run TaxSolver." );

 xpos = (int)(0.46 * (float)winwidth + 0.5);
 // printf("Print = %1.2g, ", (float)xpos / (float)winwidth );
 button = make_button( mpanel, xpos, winht - 35, "  Print  ", printout, 0 );
 add_tool_tip( button, "Print results." );

 xpos = (int)(0.72 * (float)winwidth + 0.5);
 // printf("Help = %1.2g, ", (float)xpos / (float)winwidth );
 button = make_button( mpanel, xpos, winht - 35, "Help", helpabout2, 0 );
 add_tool_tip( button, "Get information about this program,\n Help, and Updates." );

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

 DisplayTaxInfo();
 gtk_widget_show_all( outer_window );
}


void Get_Tax_Form_Page( char *fname )		/* This is only called once, to bring up the initial form. */
{
 Read_Tax_File( fname );
 Setup_Tax_Form_Page();
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


struct choice_rec
 {
  char *word;
  GtkEntry *box;
 };


void status_choice_S( GtkWidget *wdg, void *x )
{ struct value_list *tmppt=(struct value_list *)x;
  modify_formbox( tmppt->box, "Single" );
}

void status_choice_MJ( GtkWidget *wdg, void *x )
{ struct value_list *tmppt=(struct value_list *)x;
  modify_formbox( tmppt->box, "Married/Joint" );
}

void status_choice_MS( GtkWidget *wdg, void *x )
{ struct value_list *tmppt=(struct value_list *)x;
  modify_formbox( tmppt->box, "Married/Sep" );
}

void status_choice_HH( GtkWidget *wdg, void *x )
{ struct value_list *tmppt=(struct value_list *)x;
  modify_formbox( tmppt->box, "Head_of_Household" );
}

void status_choice_W( GtkWidget *wdg, void *x )
{ struct value_list *tmppt=(struct value_list *)x;
  modify_formbox( tmppt->box, "Widow(er)" );
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
 char messg[2048];
 int linenum, iscapgains, noplus=0;
 int offset=0, capgtoggle=0, firstbox_on_line_x=0;
 int y1, y1a, yoffset=4, y2, y3, dy;
 int entry_box_height=1, extra_dy;

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
   offset = offset + txline->format_offset;

   /* Place the line label. */
   // printf("\nAdding LineLabel %d (%3d, %d): '%s'\n", txline->linenum, 2, y1a, txline->linename );
   label = make_label( mpanel2, 2, y1a, txline->linename );
   gtk_widget_size_request( label, &req );	/* First find the label's size. */
   gtk_widget_destroy( label );			/* Remove it, then re-place it at best position. */
   label_width = req.width;
   label_x0 = norm_label_x1 - label_width - 4;
   if (label_x0 < 0) label_x0 = 0;
   if (debug) printf("%d: LineLabel '%s' at (%d, %d)\n",txline->linenum, txline->linename, label_x0, y1a );
   label = make_label( mpanel2, label_x0, y1a, txline->linename );
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
		 }
		else
		if (iscapgains)
		 {
		  switch (capgtoggle)
		   {
		    case 0:
			make_label( mpanel2, box_x0 + 15, y1 - 14, "Buy Cost" );
			firstbox_on_line_x = box_x0;
			box_x0 = comment_x0;
			capgtoggle++;
			break;
		    case 1:
			make_label( mpanel2, box_x0 + 15, y1 - 14, "Date Bought" );
			box_x0 = firstbox_on_line_x;
			capgtoggle++;
			break;
		    case 2:
			make_label( mpanel2, box_x0 + 15, y1 - 14, "Sold For" );
			box_x0 = comment_x0;
			capgtoggle++;
			break;
		    case 3:
			make_label( mpanel2, box_x0 + 15, y1 - 14, "Date Sold" );
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
		label = make_label( mpanel2, comment_x0, y1a, entry->comment );
		entry->comment_label = label;

		/* Add edit_line_comment button */
		if (entry_box_height != 0)
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
		break;
      }
     noplus = 0;
     entry = entry->nxt;
    } /*entry*/

   y1 = y1 + dy;
   y1a = y1 + yoffset;
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



void Save_Tax_File( char *fname )
{
 struct taxline_record *txline;
 struct value_list *tmppt;
 int lastline=-1, semicolon, j;
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
   j = 0;
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
 fprintf(outfile,"%s\n", title_line);
 txline = taxlines_hd;
 while (txline!=0)
  {
   fprintf(outfile,"\n%s", txline->linename );
   semicolon = 0;
   lastline = txline->linenum;
   tmppt = txline->values_hd;
   while (tmppt!=0)
    {
     if (tmppt->linenum != lastline)
      { fprintf(outfile,"\n");  lastline = tmppt->linenum; }
     switch (tmppt->kind)
      {
       case VKIND_FLOAT:   fprintf(outfile,"	%6.2f	", tmppt->value ); break;
       case VKIND_INT:     fprintf(outfile,"	%d	", (int)(tmppt->value) ); break;
       case VKIND_TEXT:    fprintf(outfile,"	%s	", tmppt->text ); break;
       case VKIND_COMMENT: if (strlen(tmppt->comment)>0) fprintf(outfile," {%s}", tmppt->comment ); break;
       case VKIND_COLON:   semicolon = 1; break;
      }
     tmppt = tmppt->nxt;
    }
   if (semicolon) fprintf(outfile,"\n		;");
   txline = txline->nxt;
  }

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
 char cmd[MaxFname], outfname[MaxFname];
 GtkWidget *panel, *label;
 GtkTreeStore *mylist;
 GtkTreeIter iter;
 FILE *viewfile;
 char vline[5000];
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

 #if (PLATFORM_KIND == Posix_Platform)
  sprintf(cmd,"'%s' '%s' &", taxsolvecmd, current_working_filename );
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
  sprintf(cmd,"%s \"%s\"", taxsolvecmd, current_working_filename );
 #endif

 printf("Invoking '%s'\n", cmd );
 system(cmd);		/* Run the TaxSolver. */

 /* Make a popup window telling where the results are, and showing them. */
 predict_output_filename( current_working_filename, outfname );
 wd = 620;  ht = 550;
 panel = new_window( wd, ht, "Results", &resultswindow );
 make_sized_label( panel, 1, 1, "Results written to file:", 12 );
 label = make_sized_label( panel, 30, 25, outfname, 8 );
 set_widget_color( label, "#0000ff" );
 // make_button( panel, wd/2 - 15, ht - 35, "  OK  ", canceltxslvr, 0 ); 
 make_button( panel, 60, ht - 35, "Print Result File", print_outfile_directly, 0 ); 
 make_button( panel, wd - 180, ht - 35, " Close ", canceltxslvr, 0 ); 
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
     fgets( vline, 256, viewfile );
     if (strstr( vline, "Identity-Information:" ) != 0) 
      valid = 0;
     if (strstr( vline, "Error" ) != 0) 
      valid_results = 0;
    }
   fclose(viewfile);
  }
 if ((valid_results) && (linesread > 10))
  make_button( panel, wd/2 - 80, ht - 35, "Fill-out PDF Forms", create_pdf_file_directly, 0 ); 
 show_wind( resultswindow );
 computed = 1;
 compute_needed = 0;
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
 taxsolve();
}





GtkWidget *printpopup=0, *print_label, *print_button;
GtkEntry *printerformbox;
char printer_command[MaxFname], wrkingfname[MaxFname];
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


void call_pdfviewer( char *pdfname )
{
 char cmd[4096], tmppdfname[4096];
 strcpy( tmppdfname, pdfname );
 quote_file_name( tmppdfname );
 #if (PLATFORM_KIND==Posix_Platform)
  #ifdef __APPLE__
   strcpy( cmd, "open -a ");
   strcat( cmd, pdfviewer );
  #else
   strcpy( cmd, pdfviewer );  
  #endif
  strcat( cmd, " ");  strcat( cmd, tmppdfname );  strcat( cmd, " &" );
 #else
  strcpy( cmd, "start ");  strcat( cmd, pdfviewer );  strcat( cmd, " ");  
	strcat( cmd, tmppdfname );
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
 #if (PLATFORM_KIND!=Posix_Platform)
  struct stat buf;
 #endif

 // printf("get_pdf_viewer:\n");
 pdfviewer = getenv( "PDF_VIEWER" );
 if (pdfviewer != 0) return;

 if (verbose) printf(" Checking config file\n");
 set_invocation_path( toolpath );
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
 if (pdfviewer == 0)
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
   #else
     /* --- The following are methods for opening PDF document(s) on Microsoft. --- */

     if ((stat( "c:/Program Files (x86)/Google/Chrome/Application/chrome.exe", &buf ) != 0) ||
	 (stat( "c:/Program Files/Google/Chrome/Application/chrome.exe", &buf ) != 0))
      pdfviewer = strdup( "chrome" );
     else
      {
       if (selected_form == form_US_1040)
        GeneralWarning( "Chrome Browser is required for viewing these forms properly." );

       // if (stat( "c:/Program Files (x86)/Mozilla Firefox/firefox.exe", &buf ) != 0)
       // pdfviewer = strdup( "firefox" );
       // else
        pdfviewer = strdup( "" );		/* Calls default pdf viewer properly, like Acroread32.exe or whatever. */
        // pdfviewer = strdup( "iexplore" );
      }
   #endif
   if (pdfviewer == 0)
    {
     printf("Could not find a PDF viewer on your system.\n");
     // pdfviewer = strdup( "google-chrome" );
    }
   else
    printf("Found PDF viewer '%s'\n", pdfviewer );
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
    append_selection_list( spdfvlist, &iter, "chrome" );
    append_selection_list( spdfvlist, &iter, "firefox" );
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


/* --- This routine is deprecated. To be removed from future versions. --- */
/*  (Is replaced by "prepare_universal_pdf_cmd()".)  */
void prepare_pdf_cmd( char *options, char *metadata, char *contxt, char *wrkingfname, char *markedpdf, char *outputname )
{ char *tmpmetadata, *tmpcntxt, *tmpwrkingfname, *tmpmarkedpdf, *tmpout;

 tmpmetadata = (char *)malloc(4096);
 tmpcntxt = (char *)malloc(4096);
 tmpwrkingfname = (char *)malloc(4096);
 tmpmarkedpdf = (char *)malloc(4096);
 tmpout = (char *)malloc(4096);

 strcpy( fillout_pdf_command, fillout_pdf_tool );
 strcat( fillout_pdf_command, " " );
 if (strlen(options) > 0) 
  {
   strcat( fillout_pdf_command, options );
   strcat( fillout_pdf_command, " " );
  }

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
 if (strlen(contxt) > 0)
  {
   strcat( fillout_pdf_command, "-readSchedinput " );
   strcpy( tmpcntxt, contxt );
   quote_file_name( tmpcntxt );
   strcat( fillout_pdf_command, tmpcntxt );
   strcat( fillout_pdf_command, " " );
  }
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
 free( tmpcntxt );
 free( tmpwrkingfname );
 free( tmpmarkedpdf );
 free( tmpout );
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
     case 12:
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
     case 13:
	statusw.nfiles = 0;
	setpdfoutputname( wrkingfname, ".pdf", outputname );
	prepare_universal_pdf_cmd( "", "f4562_meta.dat", wrkingfname, "f4562_pdf.dat", outputname );
	printf("Issuing: %s\n", fillout_pdf_command );
	add_status_line( outputname );
	execute_cmd( fillout_pdf_command );
	statusw.fnames[ statusw.nfiles ] = strdup( outputname );	statusw.nfiles = statusw.nfiles + 1;
	add_view_pdf_button();
	break;
     case 14:
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
     default: printf("Form type not supported.\n");
	make_button( status_panel, 30, statusw.ht - 50, " Ok ", dismiss_status_win, &status_win );
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

 if ((selected_form == form_US_1040) || (selected_form == form_US_1040_Sched_C) || (selected_form == form_PA_40)
	|| (selected_form == form_CA_540) || (selected_form == form_OH_IT1040) || (selected_form == form_VA_760)
	|| (selected_form == form_NJ_1040) || (selected_form == form_NY_IT201)
	|| (selected_form == form_MA_1) || (selected_form == form_NC_D400)
	|| (selected_form == 12) || (selected_form == 13) || (selected_form == 14))
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
   if (verbose) printf("invocation_path = '%s'\n", invocation_path );
   set_invocation_path( toolpath );
   fb_clear_banned_files();
   fb_ban_files( ".txt" );
   fb_ban_files( ".pdf" );
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
   sprintf(directory_dat, "%sexamples_and_templates%c", tmpstr, slashchr);

   return;
  }
 else
  {
   selected_other = 0;
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
   sprintf(directory_dat, "%sexamples_and_templates%c", tmpstr, slashchr);

   sel = strstr( strg, "_2017" );
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
 if (j < 0) { ots_path = strdup( "../" );  ots_path[2] = slashchr; }
 else
 if ((j > 1) && (ots_path[j-1] == 's')) ots_path[j-2] = '\0';
 else ots_path[j] = '\0';
}


void helpabout1( GtkWidget *wdg, void *data )
{
 char msg[4096];
 sprintf( msg, "OpenTaxSolver (OTS) GUI - Version %1.2f,  %s\n", version, package_date );
 strcat( msg, "                For the 2017 Tax Year.    OTS release ");
 strcat( msg, ots_release_package );   strcat( msg, "\n\n" );
 strcat( msg, "Use this GUI to open tax-forms and calculate taxes.\n");
 strcat( msg, " 1. First select a tax-form to do from the available programs listed.\n" ); 
 strcat( msg, " 2. Then to start a new blank return click 'Start New Return'.\n");
 strcat( msg, "     Or to open a file you previously saved, or a working example,\n");
 strcat( msg, "      click 'Open Saved Form' button, select your file or example,\n" ); 
 strcat( msg, "      and click 'Ok'\n");
 strcat( msg, " 3. Fill out the form that pops up.\n");
 strcat( msg, " 4. Save your filled-out form to a name of your choice.\n");
 strcat( msg, " 5. Click 'Compute Tax' to see your results.\n");
 strcat( msg, " 6. Click 'Print' to fill-out or print-out your forms.\n\n");
 strcat( msg, "For help, additional information, and updates:\n" );
 strcat( msg, " Surf to:   http://opentaxsolver.sourceforge.net/\n" );
 GeneralPopup( "OTS Information", msg );
}


void helpabout2( GtkWidget *wdg, void *data )
{
 char msg[4096];
 sprintf( msg, "OpenTaxSolver (OTS) GUI - Version %1.2f, %s\n", version, package_date );
 strcat( msg, "                For the 2017 Tax Year.    OTS release ");
 strcat( msg, ots_release_package );   strcat( msg, "\n\n" );
 strcat( msg, "Use this GUI to fill-out tax forms and calculate taxes.\n");
 strcat( msg, "  1. Fill-out the line entries that apply to you.\n");
 strcat( msg, "  2. Save your filled-out form by clicking 'Save' button.\n");
 strcat( msg, "      (If you started a new form ('_template'), then save your\n");
 strcat( msg, "       version with a unique name that is meaningful to you.)\n");
 strcat( msg, "  3. Click 'Compute Tax' to see results.\n");
 strcat( msg, "  4. Click 'Print' to print-out the results or to automatically\n");
 strcat( msg, "       fill-out the final forms.\n\n"); 
 strcat( msg, "For help, additional information, and updates:\n" );
 strcat( msg, " Surf to:   http://opentaxsolver.sourceforge.net/\n" );
 GeneralPopup( "OTS Information", msg );
}



/*----------------------------------------------------------------------------*/
/* Main -								      */
/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[] )
{
 int argn, k, grayed_out=0;
 char vrsnmssg[256], ots_pkg_mssg[256], tmpstr[MaxFname];
 float x, y, dy, y1, y2;
 GtkWidget *txprogstog, *button, *tmpwdg;

 sprintf(ots_pkg_mssg, "OTS Release %s", ots_release_package );  printf("%s\n\n", ots_pkg_mssg );
 sprintf(vrsnmssg, "GUI v%1.2f", version );  printf("%s\n", vrsnmssg );
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
    printf("  -verbose          - Show status messages.\n");
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
    sprintf(directory_dat, "%sexamples_and_templates%c", tmpstr, slashchr);
    selected_form = 11;
    ok_slcttxprog = 0;
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
 gtk_window_set_resizable( GTK_WINDOW( outer_window ), 0 );
 // make_sized_label( mpanel, 180, 10, "Open-Tax-Solver", 20.0 );

 gtk_widget_set_app_paintable( outer_window, TRUE );
 g_signal_connect( outer_window, "expose-event", G_CALLBACK(on_expose_event), NULL);

 make_rectangular_separator( mpanel, 59, 6, 387, 102 );

 y = 105;
 make_sized_label( mpanel, winwidth / 2 - 60, y, "2017 Tax Year", 11.0 );
 y = y + 35;
 make_sized_label( mpanel, 10, 135, "Select Tax Program:", 12.0 );

 x = 30;
 y = y + 25;
 y1 = y;
 dy = ((winht - 120) - y) / 6;
 txprogstog = make_radio_button( mpanel, 0, x, y, "US 1040 (w/Scheds A,B,D)", slcttxprog, "0" );
 add_tool_tip( txprogstog, "Also does the 8949 forms." );
 y = y + dy;
 make_radio_button( mpanel, txprogstog, x, y, "US 1040 Sched C", slcttxprog, "1" );
 y = y + dy;
 make_radio_button( mpanel, txprogstog, x, y, "CA State 540", slcttxprog, "3" );
 y = y + dy;
 make_radio_button( mpanel, txprogstog, x, y, "NC State DC400", slcttxprog, "4" );
 y = y + dy;
 make_radio_button( mpanel, txprogstog, x, y, "NJ State 1040", slcttxprog, "5" );
 y = y + dy;
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "GA State 500", slcttxprog, "500" );
 gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */

 y = y1;
 x = winwidth/2 + 40;
 make_radio_button( mpanel, txprogstog, x, y, "OH State IT1040", slcttxprog, "6" );
 y = y + dy;
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "PA State 40", slcttxprog, "7" );
 // gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "VA State 760", slcttxprog, "8" );
 // gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "NY State IT201", slcttxprog, "9" );
 // gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 tmpwdg = make_radio_button( mpanel, txprogstog, x, y, "MA State 1", slcttxprog, "10" );
 // gtk_widget_set_sensitive( tmpwdg, grayed_out );  /* Gray-out for this version - Not Ready. */
 y = y + dy;
 txprogstog = make_radio_button( mpanel, txprogstog, x, y, "Other", slcttxprog, "11" );
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

   button = make_button_wsizedcolor_text( mpanel, 235, winht - 100, "Open Saved Form", 14.0, "#000000", pick_file, 0 );
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
	Setup_Tax_Form_Page();
	need_to_resize = 0;
    }
   if (schedule_PDF_conversion) do_pdf_conversion();
  }
 return 0;
}
