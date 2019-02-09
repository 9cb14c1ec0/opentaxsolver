/************************************************************************/
/* TaxSolve_US1040_Sched_C.c -	 					*/
/* Copyright (C)  2019 - S.Jenkins					*/
/* 									*/
/* Compile:								*/
/*  cc taxsolve_US1040_Sched_C.c -o taxsolve_US1040_Sched_C		*/
/*									*/
/* Documentation & Updates:						*/
/*        http://opentaxsolver.sourceforge.net/				*/
/*									*/
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
/* Updated for 2018 tax year:						*/
/*  S.Jenkins 1-4-2019   						*/
/* Earlier Updates	Robert Heller  heller@deepsoft.com		*/
/************************************************************************/

float thisversion=16.00;

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include "taxsolve_routines.c"
#define Yes 1
#define No  0

/*----------------------------------------------------------------------------*/

int main ( int argc, char *argv[] )
{
 int i, j, k;
 char word[4000], outfname[4000], *EIN=0, *answ;
 time_t now;
 double L16b=0.0, L20b=0.0, L24b=0.0, Mileage=0.0;
 int L32;

 printf("US 1040 Schedule C, 2018 - v%3.2f\n", thisversion);

 /* Decode any command-line arguments. */
 i = 1;  k=1;
 while (i < argc)
 {
  if (strcmp(argv[i],"-verbose")==0)  { verbose = 1; }
  else
  if (k==1)
   {
    infile = fopen(argv[i],"r");
    if (infile==0) {printf("ERROR: Parameter file '%s' could not be opened.\n", argv[i]); exit(1);}
    k = 2;
    /* Base name of output file on input file. */
    strcpy(outfname,argv[i]);
    j = strlen(outfname)-1;
    while ((j>=0) && (outfname[j]!='.')) j--;
    if (j<0) strcat(outfname,"_out.txt"); else strcpy(&(outfname[j]),"_out.txt");
    outfile = fopen(outfname,"w");
    if (outfile==0) {printf("ERROR: Output file '%s' could not be opened.\n", outfname); exit(1);}
    printf("Writing results to file:  %s\n", outfname);
   }
  else
   {printf("Unknown command-line parameter '%s'\n", argv[i]); exit(1);}
  i = i + 1;
 }

 if (infile==0) {printf("Error: No input file on command line.\n"); exit(1);}

 /* Pre-initialize all lines to zeros. */
 for (i=0; i<MAX_LINES; i++) { L[i] = 0.0; }

 /* Accept parameters from input file. */
 /* Expect  Sched C lines, something like:
	Title:  Sched C 1999 Return
	L1		{Gross Receipts}
	L2		{Returns and Allowances}
	. . .
 */

 /* Accept Form's "Title" line, and put out with date-stamp for your records. */
 read_line( infile, word );
 now = time(0);
 fprintf(outfile,"\n%s,	 v%2.2f, %s\n", word, thisversion, ctime( &now ));

 GetTextLineF( "YourName:" );
 GetTextLineF( "YourSocSec#:" );
 GetTextLineF( "PrincipalBus:" );
 GetTextLineF( "BusinessName:" );
 GetTextLineF( "Number&Street:" );
 GetTextLineF( "TownStateZip:" );

 GetTextLineF( "ActivityCode:" );
 writeout_line = 0;	/* Suppress GetLineF's from immediately writing to outfile. */
 EIN = GetTextLineF( "BusinessEIN:" );
 format_socsec( EIN, 1 );
 fprintf(outfile,"BusinessEIN: %s\n", EIN );

 answ = GetTextLineF( "Fmethod:" );
 next_word( answ, word, " \t;" );
 if (strcasecmp( word, "Cash" ) == 0)
  fprintf(outfile,"CkFcash: X\n");
 else
  if (strcasecmp( word, "Accrual" ) == 0)
  fprintf(outfile,"CkFsccrual: X\n");
 else
  if (strcasecmp( word, "Other" ) == 0)
  fprintf(outfile,"CkFother: X\n");
 
 answ = GetTextLineF( "GPartic:" );
 next_word( answ, word, " \t;" );
 if (toupper( word[0] ) == 'Y')
  fprintf(outfile,"CkParticipate: X\n");
 else
 if ((mystrcasestr( word, "N/A" ) == 0) && (toupper( word[0] ) == 'N'))
  fprintf(outfile,"CkNotParticipate: X\n");
 
 answ = GetTextLineF( "Hacquired:" );
 next_word( answ, word, " \t;" );
 if (toupper( word[0] ) == 'Y')
  fprintf(outfile,"CkAcquired: X\n");

 answ = GetTextLineF( "Ireq1099s:" );
 next_word( answ, word, " \t;" );
 if (toupper( word[0] ) == 'Y')
  fprintf(outfile,"CkReq1099: X\n");
 else
 if ((mystrcasestr( word, "N/A" ) == 0) && (toupper( word[0] ) == 'N'))
  fprintf(outfile,"CkNotReq1099: X\n");
 
 answ = GetTextLineF( "Jfile1099s:" );
 next_word( answ, word, " \t;" );
 if (toupper( word[0] ) == 'Y')
  fprintf(outfile,"CkWillFile1099: X\n");
 else
 if ((mystrcasestr( word, "N/A" ) == 0) && (toupper( word[0] ) == 'N'))
  fprintf(outfile,"CkNotFile1099: X\n");
 
 writeout_line = 1;

 GetLine( "L1", &L[1] );	/* Gross Receipts */

 GetLine( "L2", &L[2] );	/* Returns and Allowances */	

 // GetLine( "L4", &L[4] );	/* Cost of Goods Sold */

 GetLine( "L6", &L[6] );	/* Other income, including fed & state fuel credit */

 GetLine( "L8", &L[8] );	/* Advertising */

 GetLine( "L9", &L[9] );	/* Car & truck expenses */

 GetLine( "Miles", &Mileage );	/* Miles for Line 9, not already included in Line 9. */
 L[9] = L[9] + 0.535 * Mileage;				/* Not Updated for 2018. */

 GetLine( "L10", &L[10] );	/* Commissions & fees */

 GetLine( "L11", &L[11] );	/* Contract labor */

 GetLine( "L12", &L[12] );	/* Depletion */

 GetLine( "L13", &L[13] );	/* Depreciation & Sec 179 exp ded */

 GetLine( "L14", &L[14] );	/* Employee benfit programs (other than line 19) */

 GetLine( "L15", &L[15] );	/* Insurance (other than health) */

 GetLine( "L16a", &L[16] );	/* Interest (mortgage paid to banks) */

 GetLine( "L16b", &L16b );	/* Interest (Other) */

 GetLine( "L17", &L[17] );	/* Legal & professional services */

 GetLine( "L18", &L[18] );	/* Office expense */

 GetLine( "L19", &L[19] );	/* Pension & profit sharing plans */

 GetLine( "L20a", &L[20] );	/* Vehicles and equiment Rent or Lease */

 GetLine( "L20b", &L20b );	/* Rent or lease Other business property */

 GetLine( "L21", &L[21] );	/* Repairs & maintenance */

 GetLine( "L22", &L[22] );	/* Supplies (not in Part III) */

 GetLine( "L23", &L[23] );	/* Taxes & licenses */

 GetLine( "L24a", &L[24] );	/* Travel */

 GetLine( "L24b", &L24b );	/* Deductable Meals & entertainment */

 GetLine( "L25", &L[25] );	/* Utilities */

 GetLine( "L26", &L[26] );	/* Wages (less employment credits) */

 GetLine( "L27", &L[27] );	/* Other expenses here from line 48 pg 2 */

 GetLine( "L30", &L[30] );	/* Expenses for business use of home (form 8829) */

 get_parameter( infile, 's', word, "L32a" );  /* Yes or No, All investment is at risk */
 get_parameter( infile, 'b', &L32, "L32a");

printf("Under development .... exiting.\n");
fprintf(outfile,"Under development .... exiting.\n");

 fclose(infile);

 fclose(outfile);

 printf("\nListing results from file: %s\n\n", outfname);
 Display_File( outfname );

 return 0;
}
