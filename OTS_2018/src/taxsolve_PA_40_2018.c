/************************************************************************/
/* TaxSolve_PA40_2018.c - Pennsylvania 2018 PA-40 State Tax Form.	*/
/* Copyright (C) 2019, - Aston Roberts					*/
/* 									*/
/* Compile:   gcc taxsolve_PA40_2018.c -o taxsolve_PA40_2018		*/
/* Run:	      ./taxsolve_PA40_2018  PA40_2018.txt 			*/
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
/* Aston Roberts 1-12-2019	aston_roberts@yahoo.com			*/
/************************************************************************/

#define thisversion 16.00

#include "taxsolve_routines.c"

#define SINGLE 		        1
#define MARRIED_FILLING_JOINTLY 2
#define MARRIED_FILLING_SEPARAT 3
#define WIDOW		        1
#define Yes 1
#define No  0

double Tax_Rate = 0.0307;		/* Not Verified for 2018 tax-year. */


double pos( double x )
{ /* Return only positive amounts, otherwise return zero. */
 if (x > 0.0)
  return x;
 else
  return 0.0;
}


int main( int argc, char *argv[] )
{
 int i, j, k, status=0;
 char word[2000], outfname[1500];
 time_t now;
 double oneA, oneB;
 char *Your1stName=0, *YourLastName=0, *Spouse1stName=0, *SpouseLastName, *YourNames;

 /* Decode any command-line arguments. */
 printf("PA40 - 2018 - v%3.1f\n", thisversion);
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
 for (i=0; i<MAX_LINES; i++) L[i] = 0.0;

 /* Accept parameters from input file. */
 /* Expect  PA-40 lines, something like:
	Title:  PA 40 1999 Return
	L12	34900.0  {Wages}
 */

 /* Accept Form's "Title" line, and put out with date-stamp for your records. */
 read_line( infile, word );
 now = time(0);
 fprintf(outfile,"\n%s,	 v%2.2f, %s\n", word, thisversion, ctime( &now ));

 /* get_parameter(infile, kind, x, emssg ) */
 get_parameter( infile, 's', word, "Status" );	/* Single, Married/joint, Married/sep, Widow(er) */
 get_parameter( infile, 'l', word, "Status?");
 if (strncasecmp(word,"Single",4)==0) status = SINGLE; else
 if (strncasecmp(word,"Married/Joint",13)==0) status = MARRIED_FILLING_JOINTLY; else
 if (strncasecmp(word,"Married/Sep",11)==0) status = MARRIED_FILLING_SEPARAT; else
 if (strncasecmp(word,"Widow",4)==0) status = WIDOW;
 else
  { 
   printf("Error: unrecognized status '%s'. Exiting.\n", word); 
   fprintf(outfile,"Error: unrecognized status '%s'. Exiting.\n", word); 
   exit(1);
  }
 fprintf(outfile,"Status = %s (%d)\n", word, status);

 GetLineF( "L1a", &oneA );	/* Gross compensation. */

 GetLineF( "L1b", &oneB );	/* Unreimbursed employee business expenses. */

 L[1] = oneA - oneB;
 fprintf(outfile,"L1c = %2.2f\n", L[1] );		/* Net compensation. */
 
 GetLineF( "L2", &L[2] );	/* Interest Income. */

 GetLineF( "L3", &L[3] );	/* Dividend Income. */

 GetLine( "L4", &L[4] );	/* Income or loss for business operations. */
 fprintf(outfile,"L4 = %6.2f\n", absolutev( L[4] ) );
 if (L[4] < 0.0)
  fprintf(outfile," Check_4Loss X\n");

 GetLine( "L5", &L[5] );	/* Net gain or loss from disposition of property. */
 fprintf(outfile,"L5 = %6.2f\n", absolutev( L[5] ) );
 if (L[5] < 0.0)
  fprintf(outfile," Check_5Loss X\n");

 GetLine( "L6", &L[6] );	/* Net gain or loss rents, royalties, patents, or copyrights. */
 fprintf(outfile,"L6 = %6.2f\n", absolutev( L[6] ) );
 if (L[6] < 0.0)
  fprintf(outfile," Check_6Loss X\n");

 GetLineF( "L7", &L[7] );	/* Estate or Trust Income. */

printf("Under development .... exiting.\n");
fprintf(outfile,"Under development .... exiting.\n");
 
 fprintf(outfile,"\n{ --------- }\n");
 Your1stName = GetTextLineF( "Your1stName:" );
 GetTextLineF( "MidInitial:" );
 YourLastName = GetTextLineF( "YourLastName:" );
 GetTextLineF( "YourSocSec#:" );
 Spouse1stName = GetTextLineF( "Spouse1stName:" );
 GetTextLineF( "SpouseMidInit:" );
 SpouseLastName = GetTextLineF( "SpouseLastName:" );
 GetTextLineF( "SpouseSocSec#:" );
 GetTextLineF( "Number&Street:" );
 GetTextLineF( "Town:" );
 fprintf(outfile,"State: PA\n");
 GetTextLineF( "Zipcode:" );
 GetTextLineF( "Phone:" );
 GetTextLineF( "SchoolCode:" );
 GetTextLineF( "SchooldDist:" );
 GetTextLineF( "YourOccupation:" );
 GetTextLineF( "SpouseOccupat:" );
 if (YourLastName[0] != '\0')
  {
   if (status == MARRIED_FILLING_JOINTLY)
    {
     YourNames = (char *)malloc( strlen(YourLastName) + strlen( Your1stName ) + 
				  strlen( SpouseLastName ) + strlen( Spouse1stName ) + 20 );
     strcpy( YourNames, Your1stName );
     if (strcmp( YourLastName, SpouseLastName ) == 0)
      { /* Common last name */
        strcat( YourNames, " & " );
	strcat( YourNames, Spouse1stName );
	strcat( YourNames, ", " );
	strcat( YourNames, YourLastName);
      }
     else
      {
        strcat( YourNames, " " );
	strcat( YourNames, YourLastName);
	strcat( YourNames, ", " );
	strcat( YourNames, Spouse1stName );
        strcat( YourNames, " " );
        strcat( YourNames, SpouseLastName);
      }
    }
   else
    {
     YourNames = (char *)malloc( strlen(YourLastName) + strlen( Your1stName ) + 10 );
     strcpy( YourNames, Your1stName );
     strcat( YourNames, ", " );
     strcat( YourNames, YourLastName );
    }
   fprintf(outfile,"YourNames: %s\n", YourNames );
  }
 fclose(infile);
 fclose(outfile);
 printf("\nListing results from file: %s\n\n", outfname);
 Display_File( outfname );
 return 0;
}
