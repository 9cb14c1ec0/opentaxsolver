/***********************************************************************************
 Universal_PDF_File_Modifier.c - Pulls together a multi-page PDF-file based on 
  background image data files, with arbitrary text overlays.

 Provided to OpenTaxSolver project under GPL license by the Behemoth-Software Co..

 Compile:
	cc -g universal_pdf_file_modifier.c -o universal_pdf_file_modifier

 Run:
	universal_pdf_file_modifier  metadata.txt  example_out.txt  formpages.data  

 ***********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MaxPages 100
#define MAXLINE 2048

float version=1.04;
int verbose=0;
int testmode=0;
int no_zero_entries=0;
int FontSz=10, round_to_whole_numbers=0, add_commas=1, rjustify=0;
int txtcolor=0, current_page=-1;
float txtred=0.0, txtgrn=0.0, txtblu=0.0;
int num_defined_pages=0, num_pages_to_print=0, num_main_pages=0;
int ck_sz_w=0, ck_sz_h=0, ckfntsz=16;
char cksymb[99]="*";


void next_word( char *line, char *word, char *delim )
{
 int j=0, k=0, m=0, flag=1;
 while ((line[k]!='\0') && (flag))
  {                     /* Consume any preceding delimiters. */
   j = 0;
   while ((delim[j] != '\0') && (line[k] != delim[j])) j++;
   if (line[k] != delim[j]) flag = 0; else { k++; }
  }
 while ((line[k] != '\0') && (!flag))
  {                     /* Copy word until next delimiter. */
   word[m++] = line[k++];
   if (line[k] != '\0')
    {
     j = 0;
     while ((delim[j] != '\0') && (line[k] != delim[j])) j++;
     if (line[k] == delim[j]) flag = 1;
    }
  }
 j = 0;                 /* Shorten line. */
 while (line[k] != '\0') line[j++] = line[k++];
 line[j] = '\0';        /* Terminate the char-strings. */
 word[m] = '\0';
}


/* ------------------------------------------------------------ */

struct nvpair
 {
  char *label, *value;
  int used, special, pagenumber;
  struct nvpair *nxt;
 } *results_list=0, *special_list=0;


struct showzero_rec
 {
  char *label;
  struct showzero_rec *nxt;
 } *showevenifzerolist=0;


void add_showifzero( char *label )
{
 struct showzero_rec *new;
 new = (struct showzero_rec *)malloc( sizeof( struct showzero_rec ) );
 new->label = strdup( label );
 new->nxt = showevenifzerolist;
 showevenifzerolist = new;
}


int checknzoveride( char *label )
{ /* Return 0 if label is to display even if zero. Else return 1. */
  struct showzero_rec *item;
  item = showevenifzerolist;
  while (item && (strcmp( item->label, label ) != 0))
   item = item->nxt;
 if (item) 
  return 0;
 else
  return 1;
}


struct nvpair *new_item( char *label, char *value )
{
 struct nvpair *item;
 item = (struct nvpair *)calloc( 1, sizeof( struct nvpair ) );
 item->label = strdup( label );
 item->value = strdup( value );
 item->used = 0;
 item->pagenumber = current_page;
 return item;
}


void add_entry( char *label, char *value )      /* Builds list of result items (tax-return line answer values). */
{                                               /* Example:  name="L7" value="23,456.00"  */
 struct nvpair *item;
 if (verbose) printf("Adding entry: label='%s', value='%s'\n", label, value );
 if ((no_zero_entries) && ((strcmp( value, "0.00" ) == 0) || (strcmp( value, "0" ) == 0)
     || (strcmp( value, "0.0" ) == 0)) && (checknzoveride(label))) return;
 item = new_item( label, value );
 item->nxt = results_list;
 results_list = item;
}


void add_special_rule( char *label, char *rule ) /* Builds list of special rules for lines that are to be handled differently. */
{                                                /* Example:  Special_Rule  L1a_A:  full-line */
 struct nvpair *item;
 item = new_item( label, rule );
 if (verbose) printf("Adding Special Rule '%s' = '%s'\n", label, rule );
 item->nxt = special_list;
 special_list = item;
}


void filter_text( char *line )	/* Prevent disallowed characters. */
{
 int j=0;
 while (line[j] != '\0') 
  {
   if (line[j] == '(') line[j] = '[';
   if (line[j] == ')') line[j] = ']';
   j++;
  }
}


void prepad_with_whitespace( char *wrd, int len, int nspc )
{ int j, k=0, p=0;
  char newword[1000]="";
  j = strlen( wrd );
  while (k < len - j) { newword[p++] = ' ';  if (nspc>1) newword[p++] = ' ';   k++; }
  newword[p] = '\0';
  strcat( newword, wrd );
  strcpy( wrd, newword );
}


int m_round( double x )
{
 if (x >= 0.0) return (int)(x + 0.5);
 else          return  (int)(x - 0.5);
}


void consume_leading_trailing_whitespace( char *line )
{ int j, k;
  while (isspace( line[0] ))    /* Consume any leading white-spaces. */
   {
    j = 0;
    do { line[j] = line[j+1];  j++; }
    while (line[j-1] != '\0');
   }
 k = strlen( line ) - 1;        /* Consume any trailing white-spaces. */
 while ((k >= 0) && (isspace( line[k] )))
  {
   line[k] = '\0';
   k--;
  }
}


void right_justify( char *wrd, int k )
{
 int j;
 char twrd[1024];
 twrd[k--] = '\0';
 j = strlen(wrd) - 1;
 while ((j > 0) && (wrd[j] == ' ')) j--;
 while ((j >= 0) && (k >= 0))
  {
   twrd[k--] = wrd[j--];
  }
 while (k >= 0) twrd[k--] = ' ';
 strcpy( wrd, twrd );
}


struct optional_print_rec
 {
   int form_page, priority;	/* Priority value is really an "order".  So 2 will print before 5. */
   struct nvpair *results;
   struct optional_print_rec *nxt;
 } *optional_print_list=0, *optional_print_page;


void queue_optional_page( int form_page, int page_order )
{ /* Queue an optional page for printing. */
  struct optional_print_rec *prv=0, *ptr, *new;

  num_pages_to_print++;
  new = (struct optional_print_rec *)calloc( 1, sizeof(struct optional_print_rec) );
  new->form_page = form_page;
  new->priority = page_order;
  optional_print_page = new;
  if (verbose) printf("Queuing Optional Page %d\n", form_page );

  ptr = optional_print_list;
  while ((ptr != 0) && (ptr->priority <= page_order))
   {
    prv = ptr;
    ptr = ptr->nxt;
   }
  new->nxt = ptr;
  if (prv == 0)
   optional_print_list = new;
  else
   prv->nxt = new;
}


void append_global_results_to_optional_pages()
{
 struct optional_print_rec *optlist;
 struct nvpair *item, *last=0;
 optlist = optional_print_list;
 while (optlist != 0)
  { /*optlist*/
    if (verbose) printf("Adding list ..\n");
    item = optlist->results;
    while (item != 0)	/* Find last item in list. */
     {
      // printf("	item %s = %s\n", item->label, item->value );
      last = item;
      item = item->nxt;
     }
    if (last == 0)
     optlist->results = results_list;
    else
     last->nxt = results_list;

#if (0)
	printf("\nNow list is:\n");
	item = optlist->results;
	while (item != 0)
	  {
	printf("        item %s = %s\n", item->label, item->value );
	   item = item->nxt;
	  }
	printf("\n");
#endif

    optlist = optlist->nxt;
  } /*optlist*/
}


void read_replacement_text( char *fname )
{
 int idinfo, pageorder;
 char line[MAXLINE], word1[MAXLINE], word2[MAXLINE];
 double x;
 struct nvpair *orig_results_list=0;
 FILE *infile;

 infile = fopen( fname, "rb" );
 if (infile == 0) { printf("Cannot open '%s'\n", fname );  exit(1); }
 fgets( line, MAXLINE, infile );
 while (!feof(infile))
  {
   idinfo = 0;
   next_word( line, word1, " \t=\n\r" );
   if (strcmp( word1, "PDFpage:" ) == 0)
    {
      next_word( line, word1, " \t=\n\r" );
      if (sscanf( word1, "%d", &current_page) != 1)
       printf("Error reading PDFpage: current_page '%s'\n", word1 );
      next_word( line, word1, " \t=\n\r" );
      if (sscanf( word1, "%d", &pageorder) != 1)
       printf("Error reading PDFpage: page-order '%s'\n", word1 );
      queue_optional_page( current_page, pageorder );
      orig_results_list = results_list;
      results_list = 0;
    }
   else
   if (strcmp( word1, "EndPDFpage." ) == 0)
    {
      current_page = -1;	/* Set back to global. */
      optional_print_page->results = results_list;
      results_list = orig_results_list;
    }
   else
   if (strstr( word1, ":" ) != 0)
    {
     consume_leading_trailing_whitespace( line );
     strcpy( word2, line );
     idinfo = 1;
    }
   else
    next_word( line, word2, " \t=\n\r" );
   if (word2[0] != '\0')
    {
     if (strcmp( word1, "Status" ) == 0)
      {
	if (strcmp( word2, "Single" ) == 0)
	 add_entry( "Check_single", "X");
	else
	if (strcmp( word2, "Married/Joint" ) == 0)
	 {
	  add_entry( "Check_mfj", "X");
	  add_entry( "Check_Spouse", "X");
	 }
	else
	if (strcmp( word2, "Married/Sep" ) == 0)
	 add_entry( "Check_sep", "X");
	else
	if (strncmp( word2, "Head_of_Household", 12 ) == 0)
	 add_entry( "Check_hh", "X");
	else
	if (strncmp( word2, "Widow(er)", 5 ) == 0)
	 add_entry( "Check_widow", "X");
      }
     else
      {
 	filter_text( word2 );
	if ( (!idinfo) && round_to_whole_numbers && ( isdigit(word2[0]) || ( (word2[0] == '-') && isdigit(word2[1]) ) ) )
	 {
	  if (sscanf( word2, "%lf", &x) != 1)
	   printf("Error reading number '%s'\n", word2 );
	  else
	  if (x >= 0)
	   sprintf( word2, "%d", (int)(x + 0.5) );
	  else
	   sprintf( word2, "%d", (int)(x - 0.5) );
	 }
	add_entry( word1, word2 );
      }
    }
   fgets( line, MAXLINE, infile );
  }
 fclose(infile);
 append_global_results_to_optional_pages();
}


void lookup_label( char *label, char *rplcstr, int len, int nspc )
{
 struct nvpair *item;
 item = results_list;
 // if (verbose) printf("Looking up label: '%s'\n", label );
 while ((item != 0) && (strcmp( item->label, label ) != 0)) item = item->nxt;
 if (item != 0)
  {
   strcpy( rplcstr, item->value );
   if (strlen( rplcstr ) < len ) prepad_with_whitespace( rplcstr, len, nspc );
  }
 else
  rplcstr[0] = '\0';  /* Not found. */
 if (verbose) printf("Replacing '%s' with '%s'\n", label, rplcstr );
}


/* ------------------------------------------------------------ */


struct metadata_rec
 {
   char *label;
   int x, y, fsz, padlen, txtcolor;
   float txtred, txtgrn, txtblu;
   float dx;
   struct metadata_rec *nxt;
 };

struct metapage_rec
 {
   int optional;
   struct metadata_rec *fields;
 } *metadata[MaxPages];


int pixCoords=0, custom_mediabox=0, mediabox_x=612, mediabox_y=792;
float refptX0, refptY0, refpixX0, refpixY0;
float refptX1, refptY1, refpixX1, refpixY1;

void transform_coords( int xpix, int ypix, int *xpt, int *ypt )
{
 *xpt = (int)((float)refptX0 + (float)(xpix - refpixX0) * (float)(refptX1 - refptX0) / (float)(refpixX1 - refpixX0));
 *ypt = (int)((float)refptY0 + (float)(ypix - refpixY0) * (float)(refptY1 - refptY0) / (float)(refpixY1 - refpixY0));
}


/* -----------------
    Metadata entry tags will be of the form:
	xPos  yPos  RightPaddingSpaces  CharSpacing

  ------------------ */
void read_metadata( char *fname )
{
 int pg=-1, k, nparamsrd;
 char line[MAXLINE], wrd[MAXLINE];
 struct metadata_rec *newitem;
 FILE *infile;
 infile = fopen( fname, "rb" );
 if (infile == 0) { printf("Could not open '%s'\n", fname );  exit(1); }
 fgets( line, 1024, infile );
 while (!feof(infile))
  {
   next_word( line, wrd, " \t\n\r" );
   if (wrd[0] != '\0')
    {
     if (strcmp( wrd, "Page" ) == 0)
      {
       pg++;	num_defined_pages++;	num_main_pages++;  num_pages_to_print++;
       // printf("READING INTO metadata[%d] from '%s'\n", pg, fname );
       metadata[pg] = (struct metapage_rec *)calloc( 1, sizeof(struct metapage_rec) );
       next_word( line, wrd, " \t\n\r" );
       nparamsrd = sscanf( wrd, "%d", &k );
       // printf("Reading Form Page %d\n", pg + 1 );
       // printf("	nparamsrd = %d, k = %d, pg = %d\n", nparamsrd, k, pg );
       if ((nparamsrd != 1) || (k != pg + 1))
	printf("Error: Page bad number '%s' in file %s\n", wrd, fname );
      }
     else
     if (strcmp( wrd, "Optional_Page" ) == 0)
      {
       pg++;	num_defined_pages++;
       // printf("READING OPTIONAL INTO metadata[%d] from '%s'\n", pg, fname );
       metadata[pg] = (struct metapage_rec *)calloc( 1, sizeof(struct metapage_rec) );
       metadata[pg]->optional = 1;
       next_word( line, wrd, " \t\n\r" );
       nparamsrd = sscanf( wrd, "%d", &k );
       // printf("Reading Optional Form Page %d\n", pg + 1 );
       // printf("	nparamsrd = %d, k = %d, pg = %d\n", nparamsrd, k, pg );
       if ((nparamsrd != 1) || (k != pg + 1))
	printf("Error: Optional_Page bad number '%s' in file %s\n", wrd, fname );
      }
     else
     if (strcmp( wrd, "FontSz" ) == 0)
      {
       next_word( line, wrd, " \t\n\r" );
       sscanf( wrd, "%d", &FontSz );
      }
     else
     if (strcmp( wrd, "no_zero_entries") == 0)
      {
       no_zero_entries = 1;
      }
     else
     if (strcmp( wrd, "allow_zero_entries") == 0)
      {
       no_zero_entries = 0;
      }
     else
     if (strcmp( wrd, "round_to_whole_numbers") == 0)
      {
       round_to_whole_numbers = 1;
      }
     else
     if (strcmp( wrd, "show_cents") == 0)
      {
       round_to_whole_numbers = 0;
      }
     else
     if (strcmp( wrd, "showevenifzero" ) == 0)
      {
       next_word( line, wrd, " \t\n\r" );
       add_showifzero( wrd );
      }
     else
     if (strcmp( wrd, "no_commas") == 0)
      {
       add_commas = 0;
      }
     else
     if (strcmp( wrd, "right_justify") == 0)
      {
       next_word( line, wrd, " \t\n\r" );
       sscanf( wrd, "%d", &rjustify );
      }
     else
     if (strcmp( wrd, "solid_status_check") == 0)
      {
       next_word( line, wrd, " \t\n\r" );
       sscanf( wrd, "%d", &ck_sz_w );
       next_word( line, wrd, " \t\n\r" );
       sscanf( wrd, "%d", &ck_sz_h );
       next_word( line, wrd, " \t\n\r" );
       sscanf( wrd, "%d", &ckfntsz );
       next_word( line, cksymb, " \t\n\r" );
      }
     else
     if (strcmp( wrd, "TxtColor:") == 0)	/* Set Text-color in R, G, B. */
      {
       next_word( line, wrd, " \t,\n\r" );
       sscanf( wrd, "%f", &txtred );
       next_word( line, wrd, " \t,\n\r" );
       sscanf( wrd, "%f", &txtgrn );
       next_word( line, wrd, " \t,\n\r" );
       sscanf( wrd, "%f", &txtblu );
       txtcolor = 1;
      }
     else
     if (strcmp( wrd, "CoordReference:") == 0)	/* Changes coordinates from 1/72-Pts to Pixels. */
      {
       next_word( line, wrd, " \t,\n\r" );
       sscanf( wrd, "%f", &refptX0 );
       next_word( line, wrd, " \t,\n\r" );
       sscanf( wrd, "%f", &refptY0 );
       next_word( line, wrd, " \t,\n\r" );
       sscanf( wrd, "%f", &refpixX0 );
       next_word( line, wrd, " \t,\n\r" );
       sscanf( wrd, "%f", &refpixY0 );
       fgets( line, 1024, infile );
       next_word( line, wrd, " \t,\n\r" );
       sscanf( wrd, "%f", &refptX1 );
       next_word( line, wrd, " \t,\n\r" );
       sscanf( wrd, "%f", &refptY1 );
       next_word( line, wrd, " \t,\n\r" );
       sscanf( wrd, "%f", &refpixX1 );
       next_word( line, wrd, " \t,\n\r" );
       if (sscanf( wrd, "%f", &refpixY1 ) != 1) printf("Error reading CoordReference: '%s'\n", wrd );
       pixCoords = 1;
      }
     else
     if (strcmp( wrd, "PtCoords") == 0)
      {
	pixCoords = 0;
      }
     else
     if (strcmp( wrd, "MediaBox") == 0)
      { float fval;
       next_word( line, wrd, " \t,\n\r" );
       if (sscanf( wrd, "%f", &fval ) != 1)
	printf("Error reading MediaBox x '%s'\n", wrd );
       mediabox_x = (int)(fval + 0.5);
       next_word( line, wrd, " \t,\n\r" );
       if (sscanf( wrd, "%f", &fval ) != 1)
	printf("Error reading MediaBox y '%s'\n", wrd );
       mediabox_y = (int)(fval + 0.5);
       custom_mediabox = 1;
      }
     else
     if (strcmp( wrd, "END_OF_INPUT") == 0)
      { /* This enables putting junk in file after this tag -- for whatever reason, comments, notes, etc... */
	fclose(infile);
	return;
      }
     else
      {
	if (pg < 0) { printf("Error: Missing 'Page' tag before field tag.\n");  exit(1); }
	newitem = (struct metadata_rec *)calloc( 1, sizeof(struct metadata_rec) );
	newitem->nxt = metadata[pg]->fields;
	metadata[pg]->fields = newitem;
	newitem->label = strdup( wrd );
	newitem->fsz = FontSz;
	newitem->txtcolor = txtcolor;
	newitem->txtred = txtred;
	newitem->txtgrn = txtgrn;
	newitem->txtblu = txtblu;
	next_word( line, wrd, " \t\n\r," );
	sscanf( wrd, "%d", &(newitem->x) );
	next_word( line, wrd, " \t\n\r," );
	sscanf( wrd, "%d", &(newitem->y) );
	if (pixCoords)
	 transform_coords( newitem->x, newitem->y, &(newitem->x), &(newitem->y) );
	next_word( line, wrd, " \t\n\r," );
	if (wrd[0] != '\0')
	 sscanf( wrd, "%d", &(newitem->padlen) );
	next_word( line, wrd, " \t\n\r," );
	if (wrd[0] != '\0')
	 sscanf( wrd, "%f", &(newitem->dx) );
      }
    }
   fgets( line, 1024, infile );
  }
 fclose(infile);
}


void check_color( struct metadata_rec *item )
{
 if (item->txtcolor)
  {
   txtred = item->txtred;
   txtgrn = item->txtgrn;
   txtblu = item->txtblu;
  }
}

/* ------------------------------------------------------------ */

char streambuf[40000];


void comma_format( char *word )    /* For decimal numeric values, add commas at thousandth positions. */
{
 int j=0, k, mm=0, pp=0, nn=0;
 char *twrd;
 while ((word[j] == ' ') || (word[j] == '\t')) j++;
 if (word[j] == '-') j++;
 nn = j;
 while ((word[j] >= '0') && (word[j] <= '9')) j++;     /* Find decimal point, if any. */
 /* j should now at decimal point or end of number (which is where the decimal-point would be). */
 if ((word[j] != '\0') && (word[j] != '.')) { return; }  /* Return if not a normal numeric value. */
 j--;  /* Places j on last whole number. */
 if (j - nn < 3) { return; }
 k = j;
 if (word[k+1] == '.') { k = k + 2; }
 while ((word[k] >= '0') && (word[k] <= '9')) k++; 
 if (word[k] != '\0') { return;  } /* Return if not a normal numeric value. */
 /* k is at the end of the word (at the '\0'), while j is at the last whole-digit in word. */

 /* Count backward by three characters, and add comma(s). */
 twrd = (char *)malloc( k + 100 );
 while (k > j) 
  twrd[mm++] = word[k--]; 
 while (k >= 0)
  {
   twrd[mm++] = word[k--];
   pp++;
   if ((pp == 3) && (k >= 0) && (word[k] != '-') && (word[k] != ' '))
    { twrd[mm++] = ',';  pp = 0; }
  }
 twrd[mm--] = '\0';
 // printf("The reversed string is '%s'.  mm = %d\n", &(twrd[1]), mm );

 /* Now reverse the character order. */ 
 j = 0;
 do { word[j++] = twrd[mm--]; } while (mm >= 0);
 word[j] = '\0';
 free( twrd );
}



void zeroprepad( char *wrd )
{
 int j=0;
 while (wrd[j] == ' ')
  wrd[j++] = '0';
}


void spew_sumline( FILE *outfile, char *line, int *cnt )
{
  fprintf(outfile, "%s", line );
  *cnt = *cnt + strlen(line);
}


void append_buf( char *streambuf, int fontsz, int xpos, int ypos, char *txt )
{
 char tline[2048];
 if (txtcolor)
  {
   sprintf( tline, "BT %g %g %g rg\n/F1 ", txtred, txtgrn, txtblu );
   strcat( streambuf, tline );
  }
 else
  strcat( streambuf, "BT\n/F1 " );
 sprintf(tline,"%d Tf %d %d Td (", fontsz, xpos, ypos );
 strcat( streambuf, tline );
 strcat( streambuf, txt );
 strcat( streambuf, ") Tj\nET\n");
}


void spew_from_file( FILE *outfile, FILE *infile, int n1, int *cnt )
{
 int j;
 for (j=0; j < n1; j++)
  {
   fprintf(outfile,"%c", getc(infile) );
  }
 *cnt = *cnt + n1;
}


void consume_from_file( FILE *infile, int n1 )
{
 int j;
 for (j=0; j < n1; j++)
   getc(infile);
}


int adjust_xpos_for_commas( char *txt )
{
 int j=0, cnt=0;
 while (txt[j]!= '\0')
  {
   if (txt[j] == ',') cnt++;
   j++;
  }
 return 2 * cnt;
}


void place_overlay_text( char *streambuf, int page )
{
 int j, nspc;
 float x; 
 char wrd[100], value[MAXLINE];
 struct metadata_rec *item;
 streambuf[0] = '\0';
printf("WRITING from metadata[ Pg %d ]\n", page - 1 );
 item = metadata[ page - 1 ]->fields;
 while (item)
  {
   check_color( item );
   if (item->dx > 0.0)  nspc = 1;  else  nspc = 2;
   lookup_label( item->label, value, item->padlen, nspc );
   if (value[0] != '\0')
    { /*valid*/
      if (item->dx > 0.0)
       { /* Separated characters. */
           j=0;
           x = item->x;
           while (value[j] != '\0')
            {
             sprintf( wrd, "%c", value[j++] );
             append_buf( streambuf, item->fsz, (int)(x + 0.5), item->y, wrd );
             x = x + item->dx;
            }
       }
      else
       { /*normal*/
	 float xadj=0.0;
	 if (add_commas && (strstr( item->label, "SocSec" ) == 0) && (strstr( item->label, "Zipcode" ) == 0) &&
	     (strstr( item->label, "Street" ) == 0) && (strstr( item->label, "Birth" ) == 0) &&
	     (strstr( item->label, "Check_") == 0))
	  { /*number*/
	   comma_format( value );
	   if (rjustify)
	    right_justify( value, rjustify );
	   xadj = adjust_xpos_for_commas( value );
	  } /*number*/
	 if (verbose) printf("Placing '%s' to '%s'\n", item->label, value );
	 if ((ck_sz_w == 0) || (strstr( item->label, "Check_") == 0))
	  append_buf( streambuf, item->fsz, item->x - xadj, item->y, value );
	 else
	  { int x, y;
	   y = ck_sz_h;
	   while (y != 0)
	    {
	     x = ck_sz_w;
	     while (x != 0)
	      {
		append_buf( streambuf, ckfntsz, item->x - xadj + x, item->y + y, cksymb );
		x = x - 1;
	      }
	    y = y - 1;
	   }
	  }
       }
    } /*valid*/
   item = item->nxt; 
  }
}


void write_test_pattern( char *streambuf, int page )
{ /*testmode*/	/* Writes labels into their spots. */
 struct metadata_rec *item;
 streambuf[0] = '\0';
 item = metadata[ page - 1 ]->fields;;
 while (item)
  {
   append_buf( streambuf, item->fsz, item->x, item->y, item->label );
   item = item->nxt; 
  }
} /*testmode*/


void old_write_test_pattern( char *streambuf )
{ /*oldtestmode*/	/* Write test-calibration patterns. */
  int x, y=50, dx=50, dy=25;
  char wrd1[1024];
  streambuf[0] = '\0';
  while (y < 750)
   {
     x = 50;
     while (x < 600)
      {
	 sprintf( wrd1, "[%d,%d]", x, y );
	 append_buf( streambuf, 10, x, y, wrd1 );
         x = x + dx;
      }
     y = y + dy;
   }
  y = 691;
  x = 60;
  while (x < 600)
   {
	sprintf( wrd1, "%d", x );
	append_buf( streambuf, 8, x, y, wrd1 );
        x = x + 20;
   }
  y = 683;
  x = 70;
  while (x < 600)
   {
	sprintf( wrd1, "%d", x );
	append_buf( streambuf, 8, x, y, wrd1 );
        x = x + 20;
   }
} /*oldtestmode*/




void page_collector( char *rawpdfname, char *outfname )
{ /* Reads raw-pdf file, and overlays text fields, to produce output pdf-file. */
 int npages, page=0, nobjs=0, obj, address[4096], xrefcnt, cnt=0, streamlen;
 int k, n1, n2, form_page, last_form_page=-1;
 char wrd1[4096], line[4096];
 struct nvpair *saved_results=0;
 FILE *infile, *outfile;

 infile = fopen( rawpdfname, "rb" );
 if (infile == 0) { printf("Cannot open '%s'\n", rawpdfname );  exit(1); }
 if ((fscanf( infile, "%d", &npages) != 1) || (npages < 1) || (npages > 999))
  { printf("Error reading npages in '%s'\n", rawpdfname );  exit(1); }
 if (npages != num_defined_pages)
  printf("Assertion Violation: npages (%d in RawPDF file) != num_defined_pages (%d in MetaDate file)\n", 
	  npages, num_defined_pages );
 fscanf( infile, "%s", wrd1 );	/* Consume "Pages" */

 outfile = fopen( outfname, "wb" );
 sprintf(line,"%%PDF-1.5\n%%%c%c%c%c\n", 0xfe, 0xfe, 0xfe, 0xfe );
  spew_sumline( outfile, line, &cnt );
 address[nobjs++] = cnt;
 sprintf(line,"%d 0 obj\n<< /Type /Catalog\n", nobjs );
  spew_sumline( outfile, line, &cnt );
 sprintf(line,"/Pages 2 0 R\n>>\nendobj\n");
  spew_sumline( outfile, line, &cnt );

 address[nobjs++] = cnt;
 sprintf(line,"%d 0 obj\n<< /Type /Pages\n", nobjs );
  spew_sumline( outfile, line, &cnt );
 sprintf(line,"/Kids [");
  spew_sumline( outfile, line, &cnt );
 for (page=1; page <= num_pages_to_print; page++)
  {
   sprintf(line,"%d 0 R", 4 * page );
    spew_sumline( outfile, line, &cnt );
   if (page < num_pages_to_print) { fprintf(outfile," ");  cnt++; }
  }
 sprintf(line,"]\n");
  spew_sumline( outfile, line, &cnt );
 sprintf(line,"/Count %d\n>>\nendobj\n", num_pages_to_print );
  spew_sumline( outfile, line, &cnt );

 address[nobjs++] = cnt;
 sprintf(line,"%d 0 obj\n<< /Type /Outlines /Count 0 >>\n", nobjs );
  spew_sumline( outfile, line, &cnt );
 sprintf(line,"endobj\n");
  spew_sumline( outfile, line, &cnt );

 // printf("num_defined_pages = %d\n", num_defined_pages );
 // printf("num_pages_to_print = %d\n", num_pages_to_print );
 // printf("num_main_pages = %d\n", num_main_pages );

 for (page=1; page <= num_pages_to_print; page++)
  { /*PageOut*/
    // printf("Printing Page %d\n", page );

   if (page <= num_main_pages)
    {
     form_page = page;
    }
   else
    {
     if (optional_print_list == 0) { printf("Unexpected error 7\n");  exit(1); }
     saved_results = results_list;
     results_list = optional_print_list->results;
     form_page = optional_print_list->form_page;
     optional_print_list = optional_print_list->nxt;
     // printf("Printing Optional Form page: %d\n", form_page );
     if (last_form_page != form_page - 1)
      { /*Re-position raw_pdf file read-pt.*/
	// printf(" ... Re-Syncing Form-Data file\n");
	if (last_form_page >= form_page)
	 {
	  fclose( infile );
	  infile = fopen( rawpdfname, "rb" );
	  fscanf( infile, "%d", &npages);
	  fscanf( infile, "%s", wrd1 );  /* Consume "Pages" */
	 }
	do
	 {
	   fscanf( infile, "%s", wrd1 );	/* "Page" */
	   fscanf( infile, "%d", &k );		/* pg_num */
	   // printf("	Scanning past pg %d\n", k );
	   fscanf( infile, "%d", &n1 );
	   fscanf( infile, "%d", &n2 );
	   fgets( wrd1, 1024, infile );
	   fgets( wrd1, 1024, infile );
   	   consume_from_file( infile, n1 - strlen(wrd1) - 1 );
	   fgets( wrd1, 1024, infile );		/* "2 0 obj\n" */
   	   consume_from_file( infile, n2 - strlen(wrd1) - n1 );
	   fgets( wrd1, 1024, infile );		/* "EndPage\n" */
	 }
	while (k < form_page - 1);
      }
    }
   // printf("  ... from Form %d\n", form_page );


   address[nobjs++] = cnt;
   sprintf(line,"%d 0 obj\n", nobjs );
    spew_sumline( outfile, line, &cnt );
   sprintf(line,"<< /Type /Page\n/Parent 2 0 R\n");
    spew_sumline( outfile, line, &cnt );
   if (!custom_mediabox)
    sprintf(line,"/MediaBox [0 0 612 792]\n");
   else
    sprintf(line,"/MediaBox [0 0 %3d %3d]\n", mediabox_x, mediabox_y );
   spew_sumline( outfile, line, &cnt );
   sprintf(line,"/Contents [ %d 0 R %d 0 R ]\n", nobjs + 2, nobjs + 1 );
    spew_sumline( outfile, line, &cnt );
   sprintf(line,"/Resources << /ProcSet [/PDF /Text] /Font << /F1 << /Type /Font /Subtype /Type1 /Name ");
    spew_sumline( outfile, line, &cnt );
   sprintf(line,"/F1 /BaseFont /Helvetica /Encoding /MacRomanEncoding >>\n>>\n");
    spew_sumline( outfile, line, &cnt );
   sprintf(line,"/XObject << /x5 %d 0 R>>\n", nobjs + 3 );
    spew_sumline( outfile, line, &cnt );
   sprintf(line,"/ProcSet[/PDF /Text /ImageB /ImageC /ImageI]\n");
    spew_sumline( outfile, line, &cnt );
   sprintf(line,">>\n>>\nendobj\n");
    spew_sumline( outfile, line, &cnt );

   /* Place all the text items for the present page. */
   address[nobjs++] = cnt;
   sprintf(line,"%d 0 obj\n", nobjs );
    spew_sumline( outfile, line, &cnt );
   streambuf[0] = '\0';

   if (!testmode)
    { /*normalmode*/
      place_overlay_text( streambuf, form_page );
    } /*normalmode*/
   else
    { /*testmode*/
      write_test_pattern( streambuf, form_page );
    } /*testmode*/

   streamlen = strlen( streambuf );
   sprintf(line,"<< /Length %d >>\nstream\n", streamlen - 1 );
    spew_sumline( outfile, line, &cnt );
   spew_sumline( outfile, streambuf, &cnt );
   sprintf(line,"endstream\nendobj\n");
    spew_sumline( outfile, line, &cnt );

   address[nobjs++] = cnt;
   sprintf(line,"%d 0 obj\n", nobjs );
    spew_sumline( outfile, line, &cnt );

   fscanf( infile, "%s", wrd1 );
   if (feof(infile)) 
    { printf("Premature end of infile\n");  exit(1); }
   if (strcmp(wrd1, "Page" ) != 0)
    { printf("Problem reading infile, expected 'Page' but found '%s'\n", wrd1 );  exit(1); }
   fscanf( infile, "%d", &k );
   if (k != form_page)
    { printf("Problem reading infile, expected 'Page %d' but found '%d'\n", form_page, k );  exit(1); }
   fscanf( infile, "%d", &n1 );
   fscanf( infile, "%d", &n2 );
   fgets( wrd1, 1024, infile );
   fgets( wrd1, 1024, infile );
   if (strcmp( wrd1, "1 0 obj\n" ) != 0)
    { printf("Problem reading infile, expected '1 0 obj' but found '%s'\n", wrd1 );  exit(1); }
   spew_from_file( outfile, infile, n1 - strlen(wrd1) - 1, &cnt );

   address[nobjs++] = cnt;
   sprintf(line,"%d 0 obj\n", nobjs );
    spew_sumline( outfile, line, &cnt );
   fgets( wrd1, 1024, infile );
   if (strcmp( wrd1, "2 0 obj\n" ) != 0)
    { printf("Problem reading infile, expected '2 0 obj' but found '%s'\n", wrd1 );  exit(1); }
   spew_from_file( outfile, infile, n2 - strlen(wrd1) - n1, &cnt );
   fgets( wrd1, 1024, infile );
   if (strcmp( wrd1, "EndPage\n" ) != 0)
    { printf("Problem reading infile, expected 'EndPage' but found '%s'\n", wrd1 );  exit(1); }

   if (saved_results != 0)
    {
     results_list = saved_results;
     saved_results = 0;
    }
   last_form_page = form_page;
  } /*PageOut*/
 fclose( infile );
 xrefcnt = cnt;
 fprintf(outfile,"xref\n0 %d\n", nobjs + 1 );
 fprintf(outfile,"0000000000 65535 f\n");
 for (obj=0; obj < nobjs; obj++)
  {
   sprintf(wrd1,"%10d", address[obj] );
   zeroprepad( wrd1 );
   fprintf(outfile,"%s 00000 n\n", wrd1 );
  }
 fprintf(outfile,"trailer\n<< /Size %d\n/Root 1 0 R\n>>\n", nobjs );
 fprintf(outfile,"startxref\n%d\n%%%%EOF\n", xrefcnt );
 fclose( outfile );
}


void show_help()
{
 printf("Options:\n");
 printf(" -testmode     - Place labels in their locations on pages.\n");
 printf(" -v            - Set to verbose mode.\n");
 printf(" -o  outfile   - Name the output file.\n");
 printf(" -help         - List these options.\n\n");
}


/* ----------------------------------------------------------------------------------- */
int main( int argc, char *argv[] )
{
 int k=1, p=0;
 char *outfname="new.pdf";

 printf("Universal_PDF_File_Modifier version %3.2f.\n", version );
 /* Expect:  metadata.txt  example_out.txt  formpages.data  */
 /* First pre-scan command-line to get any options. */
 while (k < argc)
  { /*k-loop*/
   if (argv[k][0] == '-')
    {
     if (strncmp( argv[k], "-testmode", 5 ) == 0)
      testmode = 1;
     else
     if (strncmp( argv[k], "-v", 2 ) == 0)
      verbose = 1;
     else
     if (strcmp( argv[k], "-o" ) == 0)
      {
       k++;
       if (k == argc)
	{ printf("Missing file-name after '-o'\n");  exit(1); }
       else
	outfname = strdup( argv[k] );
      }
     else
     if (strcmp( argv[k], "-h" ) == 0)
      {
	show_help();
	exit(0);
      }
     else
      {
	printf("Unknown option '%s'\n", argv[k] );
	exit(1);
      }
    }
   k++;
  } /*k-loop*/

 /* Now re-scan to get the files. */
 k = 1;
 while (k < argc)
  { /*k-loop*/
   if (argv[k][0] == '-')
    {
     if (strcmp( argv[k], "-o" ) == 0)
      {
       k++;
      }
    }
   else
    {
     switch (p)
      {
       case 0:  read_metadata( argv[k] );		/* metadata.txt */
 		break;
       case 1:  read_replacement_text( argv[k] );	/* example_out.txt */
 		break;
       case 2:  page_collector( argv[k], outfname );	/* formpages.data */
 		break;
       default: printf("Unexpected command line argument to %s of %s\n", argv[0], argv[k] );
	      exit(1);
      }
     p++;
    }
   k++;
  } /*k-loop*/

 printf(" Wrote: '%s'\n", outfname );
 return 0;
}
