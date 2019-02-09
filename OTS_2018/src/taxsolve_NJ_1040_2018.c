/************************************************************************/
/* TaxSolve_NJ_1040_2018.c - 						*/
/* Copyright (C) 2019 - Aston Roberts					*/
/* 									*/
/* Compile:   gcc taxsolve_NJ_1040_2018.c -o taxsolve_NJ_1040_2018	*/
/* Run:	      ./taxsolve_NJ_1040_2018  NJ_1040_2018.txt 		*/
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
/* Aston Roberts 1-25-2019	aston_roberts@yahoo.com			*/
/*    2-14-05   BWB							*/
/*      2-24-06 Further updates BWB					*/
/************************************************************************/

float thisversion=16.00;

#include <stdio.h>
#include <time.h>

#include "taxsolve_routines.c"

double A[MAX_LINES], S[MAX_LINES], E[MAX_LINES];

#define SINGLE 		        1
#define MARRIED_FILLING_JOINTLY 2
#define MARRIED_FILLING_SEPARAT 3
#define HEAD_OF_HOUSEHOLD       4
#define WIDOW		        5





/*----------------------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
 int i, j, k;
 char word[1000], outfname[4000];
 int status=0;
 time_t now;
 int L12a=0, L12b=0;
 double L27a=0.0, L27b=0.0, L29a=0.0, L29b=0.0, L37a=0.0;
 double Ab[10], A9a=0.0, proptxcredit;
 double F[10], Fb[10];	/* Schedule F, added by BWB. */
 double I[10], Ib[10];	/* Schedule I. */
 char *Your1stName="", *YourLastName="", *YourInitial="", *Spouse1stName="", *SpouseLastName="", *SpouseInitial="";
 char YourNames[2048]="";

 /* Intercept any command-line arguments. */
 printf("NJ 1040 2018 - v%3.1f\n", thisversion);
 i = 1;  k=1;
 while (i < argc)
 {
  if (strcmp(argv[i],"-verbose")==0)  verbose = 1;
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
  else {printf("Unknown command-line parameter '%s'\n", argv[i]); exit(1);}
  i = i + 1;
 }

 if (infile==0) {printf("Error: No input file on command line.\n"); exit(1);}

 /* Pre-initialize all lines to zeros. */
 for (i=0; i<MAX_LINES; i++) 
  {
   L[i] = 0.0;
   A[i] = 0.0;
   S[i] = 0.0;
   E[i] = 0.0;
  }

 /* Accept parameters from input file. */
 /* Expect  NJ-1040 lines, something like:
	Title:  NJ 1040 1999 Return
	L14		{Wages}
	L15a		{Interest}
	L16		{Dividends}
	L18		{Capital Gains}
	S1		{Property Tax}
	L42		{Witheld tax, from W-2}
*/


 /* Accept Form's "Title" line, and put out with date-stamp for records. */
 read_line( infile, word );
 now = time(0);
 fprintf(outfile,"\n%s,	 v%2.2f, %s\n", word, thisversion, ctime( &now ) );

 /* get_parameter(infile, kind, x, mesage ) */
 get_parameter( infile, 's', word, "Status" );
 get_parameter( infile, 'l', word, "Status ?");
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
 switch (status)
 {
  case SINGLE: 			fprintf(outfile,"Status = Single (%d)\n", status); 
				L[6] = 1;
				break;
  case MARRIED_FILLING_JOINTLY: fprintf(outfile,"Status = Married/Joint (%d)\n", status); 
				fprintf(outfile," Check_Spouse = X\n"); 
				L[6] = 2;
				break;
  case MARRIED_FILLING_SEPARAT: fprintf(outfile,"Status = Married/Sep (%d)\n", status);
				L[6] = 1;
				break;
  case HEAD_OF_HOUSEHOLD: 	fprintf(outfile,"Status = Head_of_Household (%d)\n", status); 
				L[6] = 1;
				break;
  case WIDOW: 		  	fprintf(outfile,"Status = Widow(er) (%d)\n", status); 
				L[6] = 1;
				break;
 }

 // get_parameter( infile, 's', word, "L6" );	/* Exemptions, self/spouse. */
 // get_parameter( infile, 'i', &j, "L6"); 
 // L[6] = j;
 shownum(6); 

 get_parameter( infile, 's', word, "YouOver65" );	/* Exemptions, Over 65. */
 get_parameter( infile, 'b', &j, "YouOver65"); 
 L[7] = j;
 if (j) fprintf(outfile," Check_Over65 = X\n");

 get_parameter( infile, 's', word, "SpouseOver65" );	/* Exemptions, Over 65. */
 get_parameter( infile, 'b', &j, "SpouseOver65"); 
 if (status == MARRIED_FILLING_JOINTLY)
  {
   L[7] = L[7] + j;
   if (j) fprintf(outfile," Check_Spover65 = X\n");
  }
 shownum(7); 

 get_parameter( infile, 's', word, "YouBlindDisa" );	/* Exemptions, Blind/disabled. */
 get_parameter( infile, 'b', &j, "YouBlindDisa"); 
 L[8] = j;
 if (j) fprintf(outfile," Check_Blind = X\n");

 get_parameter( infile, 's', word, "SpouseBlindDisa" );    /* Exemptions, Blind/disabled. */
 get_parameter( infile, 'b', &j, "SpouseBlindDisa"); 
 if (status == MARRIED_FILLING_JOINTLY)
  {
   L[8] = L[8] + j;
   if (j) fprintf(outfile," Check_SpBlind = X\n");
  }
 shownum(8); 

 get_parameter( infile, 's', word, "L9" );	/* Exemptions, children. */
 get_parameter( infile, 'i', &j, "L9"); 
 L[9] = j;
 shownum(9); 

 get_parameter( infile, 's', word, "L10" );	/* Exemptions, other dependents. */
 get_parameter( infile, 'i', &j, "L10"); 
 L[10] = j;
 shownum(10); 

 get_parameter( infile, 's', word, "L11" );	/* Exemptions, college kids. */
 get_parameter( infile, 'i', &j, "L11"); 
 L[11] = j;
 shownum(11); 

 L12a = L[6] + L[7] + L[8] + L[11];
 fprintf(outfile,"L12a = %d\n", L12a);
 L12b = L[9] + L[10];
 fprintf(outfile,"L12b = %d\n", L12b);

 GetLineF( "L14", &L[14] );	/* Wages. */

 GetLineF( "L15a", &L[15] );	/* Taxable Interest. */

 /* Form asks for tax-exempt income, but does not use it. */

 GetLineF( "L16", &L[16] );	/* Dividends. */

 GetLine( "L17", &L[17] );	/* Business profits, Fed Sched C. */
 if (L[17] < 0.0) L[17] = 0.0;
 showline(17);

printf("Under development .... exiting.\n");
fprintf(outfile,"Under development .... exiting.\n");
 
 fprintf(outfile,"\n{ --------- }\n");
 Your1stName    = GetTextLineF( "Your1stName:" );
 YourInitial    = GetTextLineF( "YourInitial:" );
 YourLastName   = GetTextLineF( "YourLastName:" );
 GetTextLineF( "YourSocSec#:" );
 Spouse1stName  = GetTextLineF( "Spouse1stName:" );
 SpouseInitial  = GetTextLineF( "SpouseInitial:" );
 SpouseLastName = GetTextLineF( "SpouseLastName:" );
 GetTextLineF( "SpouseSocSec#:" );
 if (strlen( YourLastName ) > 0)
  {
   strcpy( YourNames, YourLastName );
   strcat( YourNames, ", " );
   strcat( YourNames, Your1stName );
   if (YourInitial[0] != '\0')
    {
     strcat( YourNames, ", " );
     strcat( YourNames, YourInitial );
    }
   if (Spouse1stName[0] != '\0')
    {
     strcat( YourNames, ", " );
     if ((SpouseLastName[0] != '\0') && (strcmp( YourLastName, SpouseLastName ) != 0))
      {
       strcat( YourNames, SpouseLastName );
       strcat( YourNames, ", " );
      }
     strcat( YourNames, Spouse1stName );
     if (SpouseInitial[0] != '\0')
      {
       strcat( YourNames, ", " );
       strcat( YourNames, SpouseInitial );
      }
    }
   fprintf(outfile,"YourNames: %s\n", YourNames );
  }
 GetTextLineF( "Number&Street:" );
 GetTextLineF( "Town:" );
 GetTextLineF( "State:" );
 GetTextLineF( "Zipcode:" );

 fclose(infile);
 fclose(outfile);
 Display_File( outfname );
 printf("\nResults written to file:  %s\n", outfname);
 return 0;
}
