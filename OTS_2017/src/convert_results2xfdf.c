/*************************************************************************
 convert_results2xfdf.c - This program reads an OTS result output file,
  along with a cross-reference file, and produces the corresponding "xfdf"
  file.  This can then be fed to a PDFTK tool, along with the corresponding
  government supplied PDF fill-in form file.   The PDFTK tool then fills
  out the fields of the PDF file with the OTS result data.

 The cross-reference file pairs the line-labels in the OTS result file
 with their corresponding field-names in the government supplied PDF
 fill-in form file.  The "xfdf" file needs these field names.

 Compile:
	cc -O convert_results2xfdf.c -o convert_results2xfdf

 Usage:
	convert_results2xfdf  xref.data   result_out.txt
   (It then produces "result_out.xfdf".)

 This program implements a method to use XFDF that was originally 
 developed and provided by Daniel Walker.  Therefore, major credit 
 goes to Daniel.	( dwalker at fifo99.com )

 Author: Skeet Moncur (the elder)	Email: skeeter_mon at yahoo.com
 Date:   Feb 9, 2016
**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXLN 10000

float version=0.02;

enum form_flags { DOLLAR_AND_CENTS, DOLLAR_AND_CENTS_ONE, DOLLAR_ONLY, 
		  USE_KEY_IN_FORM, IF_SET, FOUR_DIGITS, KEY_VALUE };

struct xref_rec
 {
  char *linelabel, *fieldname, *centsfield;
  enum form_flags format;
  struct xref_rec *nxt;
 } *xref_list=0;

int verbose=0;


void add_xref( char *linelabel, char *format, char *fieldname, char *centsfield, int linenum )
{
 struct xref_rec *new;
 new = (struct xref_rec *)calloc( 1, sizeof( struct xref_rec ) );
 new->linelabel = strdup( linelabel );
 new->fieldname = strdup( fieldname );
 new->centsfield = strdup( centsfield );
 if (verbose)
  printf(" AddingXref( '%s', '%s', '%s', '%s', %d )\n",
	linelabel, format, fieldname, centsfield, linenum );	
 if (strcmp( format, "DOLLAR_AND_CENTS" ) == 0)
  new->format = DOLLAR_AND_CENTS;
 else
 if (strcmp( format, "DOLLAR_AND_CENTS_ONE" ) == 0)
  new->format = DOLLAR_AND_CENTS_ONE;
 else
 if (strcmp( format, "DOLLAR_ONLY" ) == 0)
  new->format = DOLLAR_ONLY;
 else
 if (strcmp( format, "USE_KEY_IN_FORM" ) == 0)
  new->format = USE_KEY_IN_FORM;
 else
 if (strcmp( format, "IF_SET" ) == 0)
  new->format = IF_SET;
 else
 if (strcmp( format, "FOUR_DIGITS" ) == 0)
  new->format = FOUR_DIGITS;
 else
 if (strcmp( format, "KEY_VALUE" ) == 0)
   new->format = KEY_VALUE;
 else
  {
   printf("Error: Unknown XFDF format '%s' for label '%s' on line %d\n",
	format, linelabel, linenum );
   exit(1);
  }
 new->nxt = xref_list;
 xref_list = new;
}


int m_round( double x )
{
 if (x >= 0.0) return (int)(x + 0.5);
 else          return  (int)(x - 0.5);
}


void set_xfdf( FILE *outfile, char *label, char *value )
{
 double x;
 struct xref_rec *xref_item;

 if (verbose) printf(" SetXFDF( '%s', '%s' )\n",label, value );
 xref_item = xref_list;
 while ((xref_item != 0) && (strcmp( xref_item->linelabel, label ) != 0))
  xref_item = xref_item->nxt;
 if (xref_item == 0) return;
 else
  {
   switch (xref_item->format)
    {
     case DOLLAR_ONLY:
	fprintf(outfile, "\t<field name=\"%s\">\n", xref_item->fieldname );
	if (sscanf( value, "%lf", &x ) != 1)
	 {
	  printf("Error: Value on line %s is not numeric (%s)\n", label, value );
	  exit(1);
	 }
	fprintf(outfile, "\t  <value>%d</value>\n", m_round( x ) );
	fprintf(outfile,"\t</field>\n");
	break;

     case DOLLAR_AND_CENTS:
	fprintf(outfile, "\t<field name=\"%s\">\n", xref_item->fieldname );
	if (sscanf( value, "%lf", &x ) != 1)
	 {
	  printf("Error: Value on line %s is not numeric (%s)\n", label, value );
	  exit(1);
	 }
	fprintf(outfile, "\t <value>%d</value>\n", (int)x );
	fprintf(outfile, "\t</field>\n");
	fprintf(outfile, "\t<field name=\"%s\">\n", xref_item->centsfield );
	fprintf(outfile, "\t  <value>%d</value>\n", abs( (int)(100.0 * (x - (int)x)) ) );
	fprintf(outfile, "\t</field>\n");
	break;

     case DOLLAR_AND_CENTS_ONE:
	fprintf(outfile, "\t<field name=\"%s\">\n", xref_item->fieldname );
	fprintf(outfile, "\t  <value>%s</value>\n", value );
	fprintf(outfile, "\t</field>\n");
	break;

     case FOUR_DIGITS:
	fprintf(outfile, "\t<field name=\"%s\">\n", xref_item->fieldname );
	if (sscanf( value, "%lf", &x ) != 1)
	 {
	  printf("Error: Value on line %s is not numeric (%s)\n", label, value );
	  exit(1);
	 }
	fprintf(outfile, "\t  <value>%4.f</value>\n", 10000.0 * x );
	fprintf(outfile, "\t</field>\n");
	break;

     case USE_KEY_IN_FORM:	/* Base on line lable. */
	fprintf(outfile, "\t<field name=\"%s\">\n", xref_item->fieldname );
	fprintf(outfile, "\t  <value>%s</value>\n", xref_item->centsfield );
	fprintf(outfile, "\t</field>\n");
	break;

     case IF_SET:		/* Base on line value. */
	if (value[0] == xref_item->centsfield[0])
	 {
	  fprintf(outfile, "\t<field name=\"%s\">\n", xref_item->fieldname );
	  fprintf(outfile, "\t  <value>%s</value>\n", xref_item->centsfield );
	  fprintf(outfile, "\t</field>\n");
	 }
	break;

     case KEY_VALUE:		/* Not used. */
	fprintf(outfile, "\t<field name=\"%s\">\n", xref_item->fieldname );
	fprintf(outfile, "\t  <value>%s</value>\n", xref_item->centsfield );
	fprintf(outfile, "\t</field>\n");
	break;
    }
  }
}


void next_word( char *line, char *word, char *delim )
{
 int k=0, j=0, m=0, flag=1;
 while ((line[k] != '\0') && (line[k] != '"') && (flag))  /* Consume any leading delimiters */
  {
   j = 0;
   while ((delim[j] != '\0') && (line[k] != delim[j])) j = j + 1;
   if (line[k] != delim[j]) flag = 0; else { k++; }
  }
 if (line[k] == '"')
  { /*quoted*/
    k++;
    while ((line[k] != '\0') && (line[k] != '"'))
      word[m++] = line[k++];		/* Copy chars until next quote. */
    if (line[k] == '\0') printf("Warning: Missing end-quote\n");
    else k++;
  } /*quoted*/
 else
  { /*normal*/
   while ((line[k] != '\0') && (!flag))	 /* Copy chars until next delimiter. */
    {
     word[m++] = line[k++];
     if (line[k] != '\0')
      {
       j = 0;
       while ((delim[j] != '\0') && (line[k] != delim[j])) j = j + 1;
       if (line[k] == delim[j]) flag = 1;
      }
    }
  } /*normal*/
 j = 0;			/* Shorten line. */
 while (line[k] != '\0') line[j++] = line[k++];
 line[j] = '\0';	/* Terminate the char-strings. */
 word[m] = '\0';
}


void show_help()
{
 printf("\nConvert_Results2Xfdf Version %g\n", version );
 printf(" This program reads an OTS result output file,\n");
 printf(" along with a cross-reference file, and produces the corresponding 'xfdf'\n");
 printf(" file.  This can then be fed to a PDFTK tool, along with the corresponding\n");
 printf(" government supplied PDF fill-in form file.   The PDFTK tool then fills\n");
 printf(" out the fields of the PDF file with the OTS result data.\n");
 printf("\n");
 printf(" The cross-reference file pairs the line-labels in the OTS result file\n");
 printf(" with their corresponding field-names in the government supplied PDF\n");
 printf(" fill-in form file.  The 'xfdf' file needs these field names.\n");
 printf("\n");
 printf(" Usage:\n");
 printf("        convert_results2xfdf  xref.data   result_out.txt\n");
 printf("   (It then produces 'result_out.xfdf'.)\n");
 exit(0);
}


/* ----------------------------------------------------------- */
int main( int argc, char *argv[] )
{
 char *fname[2]={0,0}, line[MAXLN], word[MAXLN], label[MAXLN], outfname[MAXLN],
      format[MAXLN], fieldname[MAXLN], centsfieldname[MAXLN], value[MAXLN];
 FILE *infile, *outfile;
 int k=1, m=0, linenum=0;
 
 while (k < argc)
  {
   if (strcmp( argv[k], "-verbose" ) == 0)
    verbose = 1;
   else
   if (strcmp( argv[k], "-help" ) == 0)
     show_help();
   else
   if (m < 2)
    fname[m++] = strdup( argv[k] );
   else
    {
     printf("Error: Too many file names on command line.\n");
     exit(1);
    }
   k++;
  }
 if (m < 2) { printf("Missing file on command line.\n");  exit(1); }

 /* First read the xref file. */
 infile = fopen( fname[0], "r" );
 if (infile == 0) { printf("Could not open '%s'.\n", fname[0] );  exit(1); }
 fscanf(infile, "%s", word );
 if (strcmp( word, "XFDF_CrossRef:" ) != 0)
  {
   printf("Error: First file does not look like a Cross-Ref file.  Missing magic header word.\n");
   exit(1);
  }
 fgets( line, MAXLN, infile );  linenum++;
 while (!feof(infile))
  {
   next_word( line, label, " \t\n\r," );
   if (label[0] != '\0')
    {
     next_word( line, format, " \t\n\r," );
     next_word( line, fieldname, " \t\n\r," );
     next_word( line, centsfieldname, " \t\n\r," );
     add_xref( label, format, fieldname, centsfieldname, linenum );
    }
   fgets( line, MAXLN, infile );  linenum++;
  }
 fclose( infile );
 
 /* Now read in the results file, and write out the XFDF file. */
 infile = fopen( fname[1], "r" );
 if (infile == 0) { printf("Could not open '%s'.\n", fname[1] );  exit(1); }
 linenum = 0;

 strcpy( outfname, fname[1] );
 k = strlen( outfname ) - 1;
 while ((k >= 0) && (outfname[k] != '.')) k--;
 if (k >= 0) outfname[k] = '\0';
 strcat( outfname, ".xfdf" );
 printf(" Writing: %s\n", outfname );
 outfile = fopen( outfname, "w" );
 fprintf(outfile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
 fprintf(outfile, "<xfdf xmlns=\"http://ns.adobe.com/xfdf/\">\n");
 fprintf(outfile, "  <fields>\n");

 fgets( line, MAXLN, infile );  linenum++;
 while (!feof(infile))
  {
   next_word( line, label, " \t\n\r=" );
   if (label[0] != '\0')
    {
     next_word( line, value, " \t\n\r=" );
     if (strcmp( label, "Status" ) != 0)
      set_xfdf( outfile, label, value );
     else
      {
       set_xfdf( outfile, value, label );
      }
    }
   fgets( line, MAXLN, infile );  linenum++;
  }
 fclose( infile );

 fprintf(outfile, "  </fields>\n");
 fprintf(outfile, "</xfdf>");;
 fclose( outfile );
 return 0;
}
