/************************************************************************/
/* TaxSolve_NJ_1040_2021.c - 						*/
/* Copyright (C) 2021 - Aston Roberts					*/
/* 									*/
/* Compile:   gcc taxsolve_NJ_1040_2021.c -o taxsolve_NJ_1040_2021	*/
/* Run:	      ./taxsolve_NJ_1040_2021  NJ_1040_2021.txt 		*/
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
/* Aston Roberts 1-2-2021	aston_roberts@yahoo.com			*/
/************************************************************************/

float thisversion=19.00;

#include <stdio.h>
#include <time.h>

#include "taxsolve_routines.c"

double COJ[MAX_LINES], S[MAX_LINES], F[MAX_LINES];

#define SINGLE 		        1
#define MARRIED_FILLING_JOINTLY 2
#define MARRIED_FILLING_SEPARAT 3
#define HEAD_OF_HOUSEHOLD       4
#define WIDOW		        5


double TaxRateFormula( double x, int status )
{
 if ((status==SINGLE) || (status==MARRIED_FILLING_SEPARAT))	/* Single, Married/sep */
  {
   if (x < 20000.0)   return x * 0.014;               else
   if (x < 35000.0)   return x * 0.0175  -     70.0;  else
   if (x < 40000.0)   return x * 0.035   -    682.5;  else
   if (x < 75000.0)   return x * 0.05525 -   1492.5;  else
   if (x < 500000.0)  return x * 0.0637  -   2126.25; else
   if (x < 1000000.0) return x * 0.0897  -  15126.25;
   else		      return x * 0.1075  -  32926.25;
  }
 else
 if ((status==MARRIED_FILLING_JOINTLY) || (status==HEAD_OF_HOUSEHOLD) || (status==WIDOW))
  {								/* Married/Joint, HouseHead, widower. */
   if (x < 20000.0)   return x * 0.014;              else
   if (x < 50000.0)   return x * 0.0175  -     70.0; else
   if (x < 70000.0)   return x * 0.0245  -    420.0; else
   if (x < 80000.0)   return x * 0.035   -   1154.5; else
   if (x < 150000.0)  return x * 0.05525 -   2775.0; else
   if (x < 500000.0)  return x * 0.0637  -   4042.5; else
   if (x < 1000000.0) return x * 0.0897  -  17042.5;
   else		      return x * 0.1075  -  34842.5;
  }
 else { printf("Status not covered.\n"); exit(1); }
}


void Report_bracket_info( double x, int status )
{
 double tx, rate;
 tx = TaxRateFormula( x, status );
 if ((status==SINGLE) || (status==MARRIED_FILLING_SEPARAT))	/* Single, Married/sep */
  {
   if (x < 20000.0)   rate = 0.014;	else
   if (x < 35000.0)   rate = 0.0175;	else
   if (x < 40000.0)   rate = 0.035;	else
   if (x < 75000.0)   rate = 0.05525;	else
   if (x < 500000.0)  rate = 0.0637;    else
   if (x < 1000000.0) rate = 0.0897;
   else		      rate = 0.1075;
  }
 else
  {								/* Married/Joint, HouseHead, widower. */
   if (x < 20000.0)   rate = 0.014;	else
   if (x < 50000.0)   rate = 0.0175;	else
   if (x < 70000.0)   rate = 0.0245;	else
   if (x < 80000.0)   rate = 0.035;	else
   if (x < 150000.0)  rate = 0.05525;	else
   if (x < 500000.0)  rate = 0.0637;    else
   if (x < 100000.0)  rate = 0.0897;
   else		      rate = 0.1075;
  }
 printf(" You are in the %2.1f%% marginal tax bracket,\n and you are paying an effective %2.1f%% tax on your total income.\n",
	  100.0 * rate, 100.0 * tx / x );
 fprintf(outfile," You are in the %2.1f%% marginal tax bracket,\n and you are paying an effective %2.1f%% tax on your total income.\n",
	  100.0 * rate, 100.0 * tx / x );
}


double TaxRateFunction( double income, int status )     /* Emulates table lookup or function appropriately. */
{
 double x, dx, tx;
 int k;

 if (income < 100000.0)  /* Quantize to match tax-table exactly. */
  {
   x = 50.0;
   dx = 0.5 * x;
   k = (income - 0.000001) / x;
   x = x * (double)k + dx;
   tx = (int)(TaxRateFormula( x, status ) + 0.5);
  }
 else
  tx = TaxRateFormula( income, status );
 return tx;
}


void place_blocked_value( char *phrase, int numpre, int numpost, char *label )
{ /* Pad and quote a string so there are numpre chars before the radix point. */
 int j=0, k=0;
 char *buf;
 while ((phrase[j] != '.') && (phrase[j] != '\0')) j++;
 buf = (char *)malloc( strlen( phrase ) + numpre + numpost + 1 );
 if (j < numpre)
  k = numpre - j;
 for (j = 0; j < k; j++)
  buf[j] = ' ';
 buf[j] = '\0';
 strcat( buf, phrase );
 fprintf(outfile, "%s = \"%s\"\n", label, buf );
 free( buf );
}


/*----------------------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
 int i, j, k, status=0, answer=0;
 char word[1000], *infname=0, outfname[4000];
 time_t now;
 char *Your1stName="", *YourLastName="", *YourInitial="", *Spouse1stName="", *SpouseLastName="", *SpouseInitial="";
 char YourNames[2048]="";
 double L16b=0.0;

 /* Intercept any command-line arguments. */
 printf("NJ 1040 2021 - v%3.1f\n", thisversion);
 i = 1;  k=1;
 while (i < argc)
 {
  if (strcmp(argv[i],"-verbose")==0)  verbose = 1;
  else
  if (strcmp(argv[i],"-round_to_whole_dollars")==0)  { round_to_whole_dollars = 1; }
  else
  if (k==1)
   {
    infname = strdup(argv[i]);
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
   COJ[i] = 0.0;
   S[i] = 0.0;
   F[i] = 0.0;
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

 fprintf(outfile, "L6a = %d\n", (int)(L[6]) );
 L[6] = 1000.0 * L[6];
 shownum(6); 

 GetYesNoSL( "YouOver65", &answer );			/* Exemptions, Over 65. */
 L[7] = answer;
 if (answer) fprintf(outfile," Check_Over65 = X\n");

 GetYesNoSL( "SpouseOver65", &answer );			/* Exemptions, Spouse Over 65. */
 if (status == MARRIED_FILLING_JOINTLY)
  {
   L[7] = L[7] + answer;
   if (answer) fprintf(outfile," Check_SpOver65 = X\n");
  }
 fprintf(outfile, "L7a = %d\n", (int)(L[7]) );
 L[7] = 1000.0 * L[7];
 shownum(7); 

 GetYesNoSL( "YouBlindDisa", &answer );   		/* Exemptions, Blind/disabled. */
 L[8] = answer;
 if (answer) fprintf(outfile," Check_Blind = X\n");

 GetYesNoSL( "SpouseBlindDisa", &answer );		/* Exemptions, Spouse Blind/disabled. */
 if (status == MARRIED_FILLING_JOINTLY)
  {
   L[8] = L[8] + answer;
   if (answer) fprintf(outfile," Check_SpBlind = X\n");
  }
 fprintf(outfile, "L8a = %d\n", (int)(L[8]) );
 L[8] = 1000.0 * L[8];
 shownum(8); 

 GetYesNoSL( "YouVeteran", &answer );			/* Exemptions, Veteran */
 L[9] = answer;
 if (answer) fprintf(outfile," Check_Vet = X\n");

 GetYesNoSL( "SpouseVeteran", &answer );		/* Exemptions, Spouse Veteran */
 if (status == MARRIED_FILLING_JOINTLY)
  {
   L[9] = L[9] + answer;
   if (answer) fprintf(outfile," Check_SpVet = X\n");
  }
 fprintf(outfile, "L9a = %d\n", (int)(L[9]) );
 L[9] = 6000.0 * L[9];
 shownum(9); 

 get_parameter( infile, 's', word, "L10" );	/* Exemptions, children. */
 get_param_single_line( infile, 'i', &j, "L10");
 fprintf(outfile, "L10a = %d\n", j );
 L[10] = 1500.0 * j;
 shownum(10); 

 get_parameter( infile, 's', word, "L11" );	/* Exemptions, other dependents. */
 get_param_single_line( infile, 'i', &j, "L11"); 
 fprintf(outfile, "L11a = %d\n", j );
 L[11] = 1500.0 * j;
 shownum(11); 

 get_parameter( infile, 's', word, "L12" );	/* Exemptions, college kids. */
 get_param_single_line( infile, 'i', &j, "L12"); 
 fprintf(outfile, "L11a = %d\n", j );
 L[12] = 1000.0 * j;
 shownum(12); 

 fprintf(outfile," FillOutForm_wRoundedNumbers_wZerosAfterDecPt\n" );

 L[13] = L[6] + L[7] + L[8] + L[9] + L[10] + L[11] + L[12];
 showline(13); 

 GetLineF( "L15", &L[15] );	/* Wages. */

 GetLineF( "L16a", &L[16] );	/* Taxable Interest. */

 /* Form asks for tax-exempt income, but does not use it. */
 GetLineF( "L16b", &L16b );	/* Tax-exempt Interest. */

 GetLineF( "L17", &L[17] );	/* Dividends. */

 GetLine( "L18", &L[18] );	/* Business profits, Fed Sched C. */
 if (L[18] < 0.0) L[18] = 0.0;
 showline(18);

 
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
 grab_any_pdf_markups( infname, outfile );
 fclose(outfile);
 Display_File( outfname );
 printf("\nResults written to file:  %s\n", outfname);
 return 0;
}
