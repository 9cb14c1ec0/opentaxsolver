/************************************************************************/
/* TaxSolve_VA760_2018.c - 						*/
/* Copyright (C) 2019 - Aston Roberts					*/
/* 									*/
/* Compile:   gcc taxsolve_VA760_2018.c -o taxsolve_VA760_2018		*/
/* Run:	      ./taxsolve_VA760_2018  VA_760_2018.txt 			*/
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
/* Aston Roberts 1-2-2019	aston_roberts@yahoo.com			*/
/************************************************************************/

#include <stdio.h>
#include <time.h>

#include "taxsolve_routines.c"

float thisversion=16.00;

#define SINGLE 		        1
#define MARRIED_FILLING_JOINTLY 2
#define MARRIED_FILLING_SEPARAT 3
#define HEAD_OF_HOUSEHOLD       4
#define WIDOW		        5



struct date_record yourDOB, spouseDOB, DL;


/*----------------------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
 int i, j, k;
 char word[1000], outfname[4000], *lnameptr, lastname[1024], *socsec, *datestr, *twrd;
 int status=0, exemptionsA=0, exemptionsB=0, youBlind=0, spouseBlind=0;
 time_t now;
 double L20b=0.0, std_ded=0.0, min2file;

 /* Intercept any command-line arguments. */
 printf("VA-760 2018 - v%3.1f\n", thisversion);
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
  }

 /* Accept parameters from input file. */
 /* Expect  VA-760 lines, something like:
	Title:  VA-760 1999 Return
	L1		{Wages}
*/


 /* Accept Form's "Title" line, and put out with date-stamp for records. */
 read_line( infile, word );
 now = time(0);
 fprintf(outfile,"\n%s,	 v%2.2f, %s\n", word, thisversion, ctime( &now ));

 /* get_parameter(infile, kind, x, emssg ) */
 get_parameter( infile, 's', word, "Status" );
 get_parameter( infile, 'l', word, "Status ?");
 if (strncasecmp(word,"Single",4)==0) status = SINGLE; else
 if (strncasecmp(word,"Married/Joint",13)==0) status = MARRIED_FILLING_JOINTLY; else
 if (strncasecmp(word,"Married/Sep",11)==0) status = MARRIED_FILLING_SEPARAT; else
 if (strncasecmp(word,"Head_of_House",4)==0) status = HEAD_OF_HOUSEHOLD;
 else
  { 
   printf("Error: unrecognized status '%s'. Must be: Single, Married/joint, Married/sep, Head_of_house, Widow(er)\nExiting.\n", word); 
   fprintf(outfile,"Error: unrecognized status '%s'. Must be: Single, Married/joint, Married/sep, Head_of_house, Widow(er)\nExiting.\n", word); 
   exit(1); 
  }
 fprintf(outfile,"Status = %s\n", word );
 fprintf(outfile," FilingStatus: %d\n", status );

 GetTextLineF( "Your1stName:" );
 GetTextLineF( "YourMI:" );
 lnameptr = GetTextLineF( "YourLastName:" );
 strcpy( lastname, lnameptr );
 lastname[4] = '\0';
 fprintf(outfile,"Y1st4: %s\n", lastname );
 writeout_line = 0;
 socsec = GetTextLineF( "YourSocSec#:" );
 format_socsec( socsec, 1 );
 fprintf(outfile,"YourSocSec#: %s\n", socsec );
 datestr = GetTextLineF( "YourDOB:" );
 if (datestr[0] == '\0')
  {
   show_errmsg("\nMissing 'YourDOB' -- needed to check age exemptions.\n");
   datestr = strdup( "1 / 1 / 2000");	/* Pressume under 65. */
  }
 if (interpret_date( datestr, &(yourDOB.month), &(yourDOB.day), &(yourDOB.year), "Bad YourDOB" ) != 1)
   exit(1);
 twrd = format_mmddyyyy( yourDOB.month, yourDOB.day, yourDOB.year );
 fprintf(outfile,"YourDOB: %s\n", twrd );
 writeout_line = 1;
 GetTextLineF( "YourDrivLic:" );
 writeout_line = 0;
 datestr = GetTextLineF( "YourDLdate:" );
 if ((datestr[0] != '\0') && (interpret_date( datestr, &(DL.month), &(DL.day), &(DL.year), "Bad YourDL" )))
  {
   twrd = format_mmddyyyy( DL.month, DL.day, DL.year );
   fprintf(outfile,"YourDLdate: %s\n", twrd );
  }
 writeout_line = 1;

 GetTextLineF( "Spouse1stName:" );
 GetTextLineF( "SpouseMI:" );
 lnameptr = GetTextLineF( "SpouseLastName:" );
 strcpy( lastname, lnameptr );
 lastname[4] = '\0';
 fprintf(outfile,"S1st4: %s\n", lastname );
 writeout_line = 0;
 socsec = GetTextLineF( "SpouseSocSec#:" );
 format_socsec( socsec, 1 );
 fprintf(outfile,"SpouseSocSec#: %s\n", socsec );
 datestr = GetTextLineF( "SpouseDOB:" );
 if (status == MARRIED_FILLING_JOINTLY)
  {
   if (datestr[0] == '\0')
    {
     show_errmsg("\nMissing 'SpouseDOB' -- needed to check age exemptions.\n");
     datestr = strdup( "1 / 1 / 2000");   /* Pressume under 65. */
    }
   if (interpret_date( datestr, &(spouseDOB.month), &(spouseDOB.day), &(spouseDOB.year), "Bad SpouseDOB" ) != 1)
     exit(1);
  twrd = format_mmddyyyy( spouseDOB.month, spouseDOB.day, spouseDOB.year );
  fprintf(outfile,"SpouseDOB: %s\n", twrd );
 }
 writeout_line = 1;
 GetTextLineF( "SpouseDrivLic:" );
 writeout_line = 0;
 datestr = GetTextLineF( "SpouseDLdate:" );
 if ((datestr[0] != '\0') && (interpret_date( datestr, &(DL.month), &(DL.day), &(DL.year), "Bad YourDL" )))
  {
   twrd = format_mmddyyyy( DL.month, DL.day, DL.year );
   fprintf(outfile,"SpouseDLdate: %s\n", twrd );
  }
 writeout_line = 1;

 GetTextLineF( "Number&Street:" );
 GetTextLineF( "Town:" );
 fprintf(outfile,"State: VA\n");
 GetTextLineF( "Zipcode:" );
 fprintf(outfile,"\n{ --------- }\n");

 get_parameter( infile, 's', word, "OtherDependents" );	/* Dependents not including you or spouse. */
 get_parameters( infile, 'i', &exemptionsA, "OtherDependents");
 if (exemptionsA > 0) 
  fprintf(outfile,"ExmpDeps: %d\n", exemptionsA );
 if (status == MARRIED_FILLING_JOINTLY)
  exemptionsA = 2 + exemptionsA;
 else
  exemptionsA = 1 + exemptionsA; 
 fprintf(outfile,"NExemptionsA = %d\n", exemptionsA );
 fprintf(outfile,"ExemptionsA = %d\n", 930 * exemptionsA );

 if (yourDOB.year < 1953)
  {
   fprintf(outfile,"YouOver65 = 1\n" );
   exemptionsB = 1;
  }
 else
  exemptionsB = 0;

 /* Handle Exemptions for:  YouOver65:      SpOver65:       YouBlind:     SpBlind:       */

 get_parameter( infile, 's', word, "YouBlind" );   
 get_parameter( infile, 'b', &youBlind, "YouBlind"); 
 if (youBlind != 0)
  {
   fprintf(outfile,"YouBlind = 1\n");
   exemptionsB++;
  }

 get_parameter( infile, 's', word, "SpouseBlind" );
 get_parameter( infile, 'b', &spouseBlind, "SpouseBlind"); 
 if (status == MARRIED_FILLING_JOINTLY)
  {
   if (spouseDOB.year < 1953)
    {
     fprintf(outfile,"SpouseOver65 = 1\n" );
     exemptionsB++;
    }
   if (spouseBlind != 0)
    {
     fprintf(outfile,"SpouseBlind = 1\n");
     exemptionsB++;
    }
  }

 fprintf(outfile,"NExemptionsB = %d\n", exemptionsB );
 fprintf(outfile,"ExemptionsB = %d\n", 800 * exemptionsB );

 if (status == MARRIED_FILLING_JOINTLY)
  {
   fprintf(outfile,"ExmpSpouse:  1\n");
   if (exemptionsA > 2)
    fprintf(outfile,"ExmpDeps: %d\n", exemptionsA - 2 );
  }
 else
 if (exemptionsA > 1)
  fprintf(outfile,"ExmpDeps: %d\n", exemptionsA - 1 );

 GetLineF( "L1", &L[1] );	/* Federal Adjusted Gross Income */

 GetLineF( "L2", &L[2] );	/* Additions from attached Schedule ADJ, line 3 */

 L[3] = L[1] + L[2];
 showline(3);

 GetLineF( "L4", &L[4] );	/* Deduction for age on Jan 1, 2018. */

 GetLineF( "L5", &L[5] );	/* Social Security Act, Tier 1 Railroad Retirement Act benef. */

 GetLineF( "L6", &L[6] );	/* State Income Tax refund or overpayment credit */

 GetLineF( "L7", &L[7] );	/* Subtractions from Schedule ADJ, line 7 */

 L[8] = L[4] + L[5] + L[6] + L[7];
 showline(8);

 L[9] = L[3] - L[8]; 
 showline(9);			/* Virginia Adjusted Gross Income (VAGI) */

 GetLineF( "L10", &L[10] );	/* Deductions - Std or Itemized minus income taxes */

 GetLineF( "L11", &L[11] );	/* State and Local Income Taxes claimed on federal Schedule A. */

printf("Under development .... exiting.\n");
fprintf(outfile,"Under development .... exiting.\n");

 fclose(infile);
 fclose(outfile);
 Display_File( outfname );
 printf("\nResults written to file:  %s\n", outfname);
 return 0;
}
