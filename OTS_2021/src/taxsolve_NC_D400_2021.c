/************************************************************************/
/* TaxSolve_NC_D400_2021.c - North Carolina 2021 NC-DC400 State Taxes.	*/
/* Copyright (C) 2021 - S.Jenkins					*/
/* 									*/
/* Compile:   gcc taxsolve_NC_D400_2021.c -o taxsolve_NC_D400_2021	*/
/* Run:  ./taxsolve_NC_D400_2021		  			*/
/*  Uses log from TaxSolve Federal 1040.				*/
/* 									*/
/* GNU Public License - GPL:						*/
/* This program is free software; you can redistribute it and/or	*/
/* modify it under the terms of the GNU General Public License as	*/
/* published by the Free Software Foundation; either version 2 of the	*/
/* License, or (at your option) any later version.			*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU	*/
/* General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA		*/
/* 02111-1307 USA							*/
/* 									*/
/* 1-2-2021	http://opentaxsolver.sourceforge.com/			*/
/*	Earlier versions - Lincoln Baxter (lab@lincolnbaxter.com)	*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>

#include "taxsolve_routines.c"

float thisversion=19.00;

#define SINGLE                  1
#define MARRIED_FILLING_JOINTLY 2
#define MARRIED_FILLING_SEPARAT 3
#define HEAD_OF_HOUSEHOLD       4
#define WIDOW                   5
#define Yes 1
#define No  0

double flat_tax_rate = 0.0525;		/* Not updated for 2021. */


struct FedReturnData
 {
  double fedline[MAX_LINES];
  int Itemized, Limited, Limited_L6;
  double Sched_A[MAX_LINES];
 };


void convert_slashes( char *fname )
{ /* Convert slashes in file name based on OS type. */
  char *ptr;
 #ifdef __MINGW32__
  char slash_sreach='/', slash_replace='\\';
 #else
  char slash_sreach='\\', slash_replace='/';
 #endif

  ptr = strchr( fname, slash_sreach );
  while (ptr)
   {
    ptr[0] = slash_replace;
    ptr = strchr( fname, slash_sreach );
   }
}


void ImportFederalReturnData( char *fedlogfile, struct FedReturnData *fed_data )
{
 FILE *infile;
 char fline[1000], word[1000];
 int linenum;

 convert_slashes( fedlogfile );
 infile = fopen(fedlogfile, "r");
 if (infile==0)
  {
   printf("Error: Could not open federal return '%s'\n", fedlogfile);
   fprintf(outfile,"\nError: Could not open federal return '%s'\n", fedlogfile);
   #ifdef __MINGW32__
    system("dir");
   #else
    system("pwd");
    system("ls -l");
   #endif
   exit(1);
  }
 printf(" Reading file: %s\n", fedlogfile );

 /* Set initial default values. */
 fed_data->Itemized = 1; 
 fed_data->Limited = 1; 
 fed_data->Limited_L6 = 1; 
 for (linenum=0; linenum<MAX_LINES; linenum++) fed_data->fedline[linenum] = 0.0;
 for (linenum=0; linenum<MAX_LINES; linenum++) fed_data->Sched_A[linenum] = 0.0;

 read_line(infile,fline);  linenum = 0;
 while (!feof(infile))
  {
   if ( verbose ) printf( "Read Line: %s" ,fline );
   if (strstr(fline,"Use standard deduction.")!=0) fed_data->Itemized = 0;
   if (strstr(fline,"Deductions not limited")!=0) 
    {
     fed_data->Limited = 0;
     if ( strstr(fline,"line 6") ) fed_data->Limited_L6 = 0;
    }
   next_word(fline, word, " \t=");
   if ((strstr(word,"A")==word) && (isdigit(word[1])) && (strstr(fline," = ")!=0))
    {
     if (sscanf(&word[1],"%d",&linenum)!=1) printf("Error: Reading fed sched-A line number '%s%s'\n",word,fline);
     next_word(fline, word, " 	=");
     if (sscanf(word,"%lf", &fed_data->Sched_A[linenum])!=1) printf("Error: Reading fed sched-A line %d '%s%s'\n",linenum,word,fline);
     if (verbose) printf("Sched_A[%d] = %2.2f\n", linenum, fed_data->Sched_A[linenum]);
    }
   if ((strstr(word,"L")==word) && (strstr(fline," = ")!=0))
    {
     if (sscanf(&word[1],"%d",&linenum)!=1) printf("Error: Reading fed line number '%s%s'\n",word,fline);
     next_word(fline, word, " 	=");
     if (sscanf(word,"%lf", &fed_data->fedline[linenum])!=1) printf("Error: Reading fed line %d '%s%s'\n",linenum,word,fline);
     if (verbose) printf("FedLin[%d] = %2.2f\n", linenum, fed_data->fedline[linenum]);
    }
   read_line(infile,fline);
  }
 fclose(infile);
}


/* ------------------------------------------------------------------------------------ */
/* ---				Main						    --- */
/* ------------------------------------------------------------------------------------ */
int main( int argc, char *argv[] )
{
 int j, jj, k, status;
 char word[1000], *infname=0, outfname[1000];
 time_t now;
 struct FedReturnData fed_data;
 int L10a=0;
 double L20a=0.0, L20b=0.0;

 /*-----------------------------------------*/
 /* --- Decode any command line options. -- */
 /*-----------------------------------------*/
 printf("NC D400 2021 - v%3.2f\n", thisversion);
 jj = 1;  k=1;
 while (jj < argc)
 {
  if (strcmp(argv[jj],"-verbose")==0)  { verbose = 1; }
  else
  if (strcmp(argv[jj],"-round_to_whole_dollars")==0)  { round_to_whole_dollars = 1; }
  else
  if (k==1)
   {
    infname = strdup(argv[jj]);
    infile = fopen(argv[jj],"r");
    if (infile==0)
     {
	printf("ERROR: Parameter file '%s' could not be opened.\n", argv[jj]); 
	fprintf(outfile,"ERROR: Parameter file '%s' could not be opened.\n", argv[jj]); 
	exit(1);
     }
    k = 2;
    /* Base name of output file on input file. */
    strcpy(outfname,argv[jj]);
    j = strlen(outfname)-1;
    while ((j>=0) && (outfname[j]!='.')) j--;
    if (j<0) strcat(outfname,"_out.txt"); else strcpy(&(outfname[j]),"_out.txt");
    outfile = fopen(outfname,"w");
    if (outfile==0)
     {
	printf("ERROR: Output file '%s' could not be opened.\n", outfname); 
	fprintf(outfile,"ERROR: Output file '%s' could not be opened.\n", outfname); 
	exit(1);
     }
    printf("Writing results to file:  %s\n", outfname);
   }
  else
   {
	printf("Unknown command-line parameter '%s'\n", argv[jj]); 
	fprintf(outfile,"Unknown command-line parameter '%s'\n", argv[jj]); 
	exit(1);
   }
  jj++;
 }

 if (infile==0)
  {
	printf("Error: No input file on command line.\n"); 
	fprintf(outfile,"Error: No input file on command line.\n"); 
	exit(1);
  }
 

 /*--------------------------*/
 /* ---- Get Input Data ---- */
 /*--------------------------*/

 /* Pre-initialize all lines to zeros. */
 for (j = 0; j < MAX_LINES; j++) { L[j] = 0.0; }

 /* Accept parameters from input file. */
 /* Expect  D-400 lines, something like:
	Title:  NC-400 1999 Return
	L6	34900.0  {Wages}
 */

 /* Accept Form's "Title" line, and put out with date-stamp for your records. */
 read_line( infile, word );
 now = time(0);
 fprintf(outfile,"\n%s,	 v%2.2f, %s\n", word, thisversion, ctime( &now ));


 /* get_parameter(infile, kind, x, emssg ) */
 get_parameter( infile, 's', word, "FedReturn" );	/* File name of Federal Return log file. */
 if ( verbose ) printf( "word: %s\n", word );
 get_word( infile, word ); 
 ImportFederalReturnData( word, &fed_data);

 get_parameter( infile, 's', word, "Status"); /* 1=single, 2=married/joint, 3=married/separate, 4=house-head, 5=widow */
 get_parameter( infile, 'l', word, "Status ?");
 if ((word[0]>'0') && (word[0]<'6')) status = word[0]-48; else
 if (strncasecmp(word,"Single",4)==0) status = SINGLE; else
 if (strncasecmp(word,"Married/Joint",13)==0) status = MARRIED_FILLING_JOINTLY; else
 if (strncasecmp(word,"Married/Sep",11)==0) status = MARRIED_FILLING_SEPARAT; else
 if (strncasecmp(word,"Head_of_House",4)==0) status = HEAD_OF_HOUSEHOLD; else
 if (strncasecmp(word,"Widow",4)==0) status = WIDOW;
 else
  { 
   printf("Error: unrecognized status '%s'. Must be: Single, Married/joint, Married/sep, Head_of_house, Widow(er)\nExiting.\n", word); 
   fprintf(outfile,"Error: unrecognized status '%s'. Must be: Single, Married/joint, Married/sep, Head_of_house, Widow(er)\nExiting.\n", word); 
   exit(1); 
  }
 fprintf(outfile,"Status = %s (%d)\n", word, status);

 GetLine( "L7", &L[7] );	/* Additions to Fed AGI - From Sched-S Part A, Line 6. */  
 GetLine( "L9", &L[9] );	/* Deductions from Fed AGI - From Sched-S Part B, Line 14. */  

 GetInteger( "L10a", &L10a );	/* Number of Dependents. */

 GetLine( "L11", &L[11] );	/* Enter 0 to use Std Deduction; Otherwise Itemized Ded. from Sched-S part C, Line 23. */
 GetLine( "L13", &L[13] );	/* Enter 1.0 for full-year residents. Otherwise amount from Sched-S part D, Line 26. */  
 GetLine( "L16", &L[16] );	/* Tax credits. (D-400TC part 3 line 20) */  
 GetLine( "L18", &L[18] );	/* Consumer Use Tax. (pgs 9+10) */
 GetLine( "L20a", &L20a );	/* North Carolina Income Tax Withheld (yours) */
 GetLine( "L20b", &L20b );	/* North Carolina Income Tax Withheld (spouses) */
 L[20] = L20a + L20b;

 fclose(infile);
 grab_any_pdf_markups( infname, outfile );
 fclose(outfile);
 Display_File( outfname );
 printf("\nResults written to file '%s'\n", outfname);
 return 0;
}
