/************************************************************************/
/* TaxSolve_Form_2210.c - 2021                                          */
/*  User contributed.							*/
/************************************************************************/

float thisversion=2.00;

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <math.h>

#include "taxsolve_routines.c"

#define SINGLE 		        1
#define MARRIED_FILING_JOINTLY 2
#define MARRIED_FILING_SEPARAT 3
#define HEAD_OF_HOUSEHOLD       4
#define WIDOW		        5
#define Yes 1
#define No  0
#define Neither 0
#define Short 1
#define Regular 2

int Quest1, Quest2, Quest3, Quest4, Quest5, Method, BoxA, BoxB, BoxC, BoxD, BoxE, Num_Days = 0;

/*-----------Tax Routines Copied From taxsolve_US_1040_2021.c ----------------*/

			/* Following values taken from 1040-Instructions pg 110. */	/* Updated for 2021. */
double brkpt[4][9]={
		{ 0.0,   9950.0,  40525.0,  86375.0, 164925.0, 209425.0, 523600.0, 9e19 },  /* Single */
		{ 0.0,  19900.0,  81050.0, 172750.0, 329850.0, 418850.0, 628300.0, 9e19 },  /* Married, filing jointly. */
		{ 0.0,   9950.0,  40525.0,  86375.0, 164925.0, 209425.0, 314150.0, 9e19 },  /* Married, filing separate. */
		{ 0.0,  14200.0,  54200.0,  86350.0, 164900.0, 209400.0, 523600.0, 9e19 },  /* Head of Household. */
		     };
  double txrt[4][9] ={
		{ 0.1, 0.12, 0.22, 0.24, 0.32, 0.35, 0.37 },	/* Single */
		{ 0.1, 0.12, 0.22, 0.24, 0.32, 0.35, 0.37 },	/* Married, filing jointly. */
		{ 0.1, 0.12, 0.22, 0.24, 0.32, 0.35, 0.37 },	/* Married, filing separate. */
		{ 0.1, 0.12, 0.22, 0.24, 0.32, 0.35, 0.37 },	/* Head of Household. */
		     };

double TaxRateFormula( double x, int status )  /* Returns tax due. */
{		
  double sum=0.0;
  int   bracket=0;
  if (status == WIDOW) status = MARRIED_FILING_JOINTLY;  /* Handle case of widow(er). */
  status = status - 1;  /* Arrays start at zero; not one. */
  while (brkpt[status][bracket+1] < x)
   {
    sum = sum + (brkpt[status][bracket+1] - brkpt[status][bracket]) * txrt[status][bracket];
    bracket = bracket + 1;
   }
  return (x - brkpt[status][bracket]) * txrt[status][bracket] + sum;
}

double TaxRateFunction( double income, int status )     /* Emulates table lookup or function appropriately. */
{
 double x, dx, tx;
 int k;
 if (income < 100000.0)	/* Quantize to match tax-table exactly. */
  {
   if (income < 25.0) x = 5.0; else
   if (income < 3000.0) x = 25.0; else x = 50.0;
   dx = 0.5 * x;
   k = income / x;
   x = x * (double)k + dx;
   tx = (int)(TaxRateFormula( x, status ) + 0.5);
  }
 else
  tx = TaxRateFormula( income, status );
 return tx;
}

/*----------------------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
  int i, j, k, status, individual = Yes;
 char word[4000], outfname[4000], *infname=0;
 time_t now;

 /* line entry variables L[n] are declared in taxsolve_routines.c */

 double A[47], B[47], C[47], D[47];	/* cells in grid of Part IV, Section A, comprised of lines 18 through 26 */
					/* e.g., cell 18(a) will be be variable A[18] */

 double a[57], b[57], c[57], d[57];	/* cells in grid of Part IV, Schedule AI comprised of lines 1 through 36 */
					/* e.g., cell 1(a) will be in variable a[1] */

 printf("Form 2210, 2021 - v%3.2f\n", thisversion);

 /* Decode any command-line arguments. */
 i = 1;  k=1;
 while (i < argc)
 {
  if (strcmp(argv[i],"-verbose")==0)  { verbose = 1; }
  else
  if (k==1)
   {
    infname = strdup(argv[i]);
    infile = fopen(infname,"r");
    if (infile==0) {printf("ERROR: Parameter file '%s' could not be opened.\n", infname ); exit(1);}
    k = 2;
    /* Base name of output file on input file. */
    strcpy(outfname,infname);
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

 for(i = 0; i <= 26; i++){
	A[i] = 0.0;
	B[i] = 0.0;
	C[i] = 0.0;
	D[i] = 0.0;
 }

 for(i = 0; i <= 36; i++){
	a[i] = 0.0;
	b[i] = 0.0;
	c[i] = 0.0;
	d[i] = 0.0;
 }

 /* Accept parameters from input file. */
 /* Expect lines, something like:
        Title:  Form XXXX Return
        L2              {Returns and Allowances}
        . . .
 */

 /* Accept Form's "Title" line, and put out with date-stamp for your records. */
 read_line( infile, word );
 now = time(0);
 fprintf(outfile,"\n%s,  v%2.2f, %s\n", word, thisversion, ctime( &now ));


 /* ----- Accept form data and process the numbers.         ------ */
 /* ----- Place all your form-specific code below here .... ------ */

 fprintf(outfile,"\n--- THIS IS PRELIMINARY USER-CONTRIBUTED FORM ---\n");
 fprintf(outfile,"--- NOT YET FULLY UPDATED FOR 2021. ---\n\n");

 // MarkupPDF( 1, 240, 40, 17, 1.0, 0, 0 ) NotReady "This program is NOT updated for 2021."
 add_pdf_markup( "NotReady", 1, 240, 40, 17, 1, 1.0, 0, 0, "\"This program is NOT ready for 2021.\"" );
 add_pdf_markup( "NotReady", 2, 240, 40, 17, 1, 1.0, 0, 0, "\"This program is NOT ready for 2021.\"" );
 add_pdf_markup( "NotReady", 3, 240, 40, 17, 1, 1.0, 0, 0, "\"This program is NOT ready for 2021.\"" );


 // Example:
 //  GetLineF( "L2", &L[2] );
 //  GetLineF( "L3", &L[3] );
 //  L[4] = L[2] - L[3];
 //  showline_wlabel( "L4", L[4] );

 GetTextLineF( "YourName:" );
 GetTextLineF( "YourSocSec#:" );

 get_parameter( infile, 's', word, "Entity" );
 get_parameter( infile, 'w', word, "Entity?");
 if (strncasecmp(word,"Individual",3)==0) individual = Yes;
 fprintf(outfile,"Entity = %s (%d)\n", word, individual);

get_parameter( infile, 's', word, "Status" );	/* Single, Married/joint, Married/sep, Head house, Widow(er) */
 get_parameter( infile, 'l', word, "Status?");
 if (strncasecmp(word,"Single",4)==0) status = SINGLE; else
 if (strncasecmp(word,"Married/Joint",13)==0) status = MARRIED_FILING_JOINTLY; else
 if (strncasecmp(word,"Married/Sep",11)==0) status = MARRIED_FILING_SEPARAT; else
 if (strncasecmp(word,"Head_of_House",4)==0) status = HEAD_OF_HOUSEHOLD; else
 if (strncasecmp(word,"Widow",4)==0) status = WIDOW;
 else
  { 
   printf("Error: unrecognized status '%s'. Exiting.\n", word); 
   fprintf(outfile,"Error: unrecognized status '%s'. Exiting.\n", word); 
   exit(1);
  }
 fprintf(outfile,"Status = %s (%d)\n", word, status);

 GetLineF( "L1", &L[1] );
 GetLineF( "L2", &L[2] );
 GetLineF( "L3", &L[3] );
 L[4] = L[1] + L[2] + L[3];
 showline( 4 );
 L[5] = L[4] * 0.90;
 showline( 5 );
 GetLineF( "L6", &L[6] );
 L[7] = L[4] - L[6];
 showline( 7 );
 GetLineF( "L8", &L[8] );
 L[9] = SmallerOf(L[5], L[8]);
 showline( 9 );
 if(L[9] > L[6])
	fprintf(outfile,  "CkL9Yes X\n");	/* Yes, may owe penalty */
 else
 	fprintf(outfile,  "CkL9No X\n");	/* No, don't owe penalty */

 get_parameter( infile, 's', word, "Quest1" );
 get_parameter( infile, 'w', word, "Quest1?");
 if (strncasecmp(word,"Yes",1)==0){
	Quest1 = Yes;
	fprintf(outfile,"CkQuest1 X\n");
}

 get_parameter( infile, 's', word, "Quest2" );
 get_parameter( infile, 'w', word, "Quest2?");
 if (strncasecmp(word,"Yes",1)==0){
	 Quest2 = Yes;
	fprintf(outfile,"CkQuest2 X\n");
}

  get_parameter( infile, 's', word, "Quest3" );
 get_parameter( infile, 'w', word, "Quest3?");
 if (strncasecmp(word,"Yes",1)==0){
	Quest3 = Yes;
	fprintf(outfile,"CkQuest3 X\n");
}

 get_parameter( infile, 's', word, "Quest4" );
 get_parameter( infile, 'w', word, "Quest4?");
 if (strncasecmp(word,"Yes",1)==0){
	Quest4 = Yes;
	fprintf(outfile,"CkQuest4 X\n");
}

 get_parameter( infile, 's', word, "BoxA" );
 get_parameter( infile, 'w', word, "BoxA?");
 if (strncasecmp(word,"Yes",1)==0){
	BoxA = Yes;
	fprintf(outfile,"CkBoxA X\n");
}

get_parameter( infile, 's', word, "BoxB" );
get_parameter( infile, 'w', word, "BoxB?");
 if (strncasecmp(word,"Yes",1)==0){
	BoxB = Yes;
	fprintf(outfile,"CkBoxB X\n");
}

 get_parameter( infile, 's', word, "BoxC" );
 get_parameter( infile, 'w', word, "BoxC?");
 if (strncasecmp(word,"Yes",1)==0){
	BoxC = Yes;
	fprintf(outfile,"CkBoxC X\n");
}

get_parameter( infile, 's', word, "BoxD" );
 get_parameter( infile, 'w', word, "BoxD?");
 if (strncasecmp(word,"Yes",1)==0){
	BoxD = Yes;
	fprintf(outfile,"CkBoxD X\n");
}

get_parameter( infile, 's', word, "BoxE" );
 get_parameter( infile, 'w', word, "BoxE?");
 if (strncasecmp(word,"Yes",1)==0){
	BoxE = Yes;
	fprintf(outfile,"CkBoxE X\n");
}

if((L[4] < 1000) || (L[7] < 1000))
	fprintf(outfile, "Line 4 or Line 7 less than $1,000.  Don’t file Form 2210. You don’t owe a penalty.\n");

else if(L[6] >= L[9]){
	if(BoxE == Yes)
		fprintf(outfile, "You don’t owe a penalty.  Because Box E in Part II applies, file page 1 of Form 2210.\n");
	else
		fprintf(outfile, "You don’t owe a penalty.  Don’t file Form 2210.\n");
}
else if((BoxA == Yes) || (BoxB == Yes) || (BoxC == Yes) || (BoxD == Yes) || (BoxE == Yes)){
	fprintf(outfile, "You MUST file Form 2210.\n");
	if((BoxB == Yes) || (BoxC == Yes) || (BoxD == Yes))
		fprintf(outfile, "You must figure your penalty.\n");
	else
		fprintf(outfile, "You aren’t required to figure your penalty because the IRS\n \
will figure it and send you a bill for any unpaid amount. If you\n \
want to figure it, you may use Part III or Part IV as a\n \
worksheet and enter your penalty amount on your tax return,\n \
but FILE ONLY PAGE 1 OF FORM 2210.\n");
}
else
	fprintf(outfile, "Don’t file Form 2210. You aren’t required to figure\n \
your penalty because the IRS will figure it and send\n \
you a bill for any unpaid amount. If you want to figure\n \
it, you may use Part III or Part IV as a worksheet and\n \
enter your penalty amount on your tax return, but\n \
don’t file Form 2210.\n");

if((Quest1 == No) || (Quest2 == Yes)){
	fprintf(outfile, "You can use the SHORT METHOD.\n");
	fprintf(outfile, "Note: If any payment was made earlier than the due date,\n \
you can use the short method, but using it may cause you to pay a\n \
larger penalty than the regular method. If the payment was only a few\n \
days early, the difference is likely to be small.\n");
}

else if((Quest3 == Yes) || (Quest4 == Yes) || (BoxC == Yes) || (BoxD == Yes))
	fprintf(outfile, "You must use the REGULAR METHOD (Part IV) instead of the short method.\n");

 get_parameter( infile, 's', word, "Method" );
 get_parameter( infile, 'w', word, "Method?");
 if (strncasecmp(word,"Neither",5)==0){
	Method = Neither;
	fprintf(outfile,"You are not using either the Short Method or the Regular Method\n");
 }
 else if (strncasecmp(word,"Short",5)==0){
	Method = Short;
	fprintf(outfile,"You are using the Short Method\n");
 }
 else if (strncasecmp(word,"Regular",7)==0){
	Method = Regular;
	fprintf(outfile,"You are using the Regular Method\n");
 }

/* Short Method */

	/* Need to read these Short Method inputs even if Short Method not selected */
	/* Otherwise, error message re: unexpected input item is thrown */

	GetLine( "L12", &L[12] );

	 get_parameter( infile, 's', word, "Quest5" );
	 get_parameter( infile, 'w', word, "Quest5?");
	 if (strncasecmp(word,"Yes",1)==0)
		Quest5 = Yes;

	 get_parameter( infile, 's', word, "Num_Days" );
	 get_parameter( infile, 'w', word, "Num_Days?");
	 Num_Days = atoi(word);

	if(Method == Short){

		L[10] = L[9];
		L[11] = L[6];
		L[13] = L[11] + L[12];
		L[14] = L[10] - L[13];
		if(L[14] <= 0){
			fprintf(outfile, "Stop.  You don't owe a penalty.\n");
			fprintf(outfile, " If the amount on line 14 was paid on or before 1/15/21,\n \
			do not use the short method.\n \
			Don’t file Form 2210 unless you checked box E in Part II\n");
		}
		L[15] = L[14] * 0.01744;
		
		if(Quest5 == Yes){
			fprintf(outfile,"Penalty was paid after 1/15/21 and before 4/15/21\n");
			fprintf(outfile, "Penalty was paid %d days before 4/15/21\n", Num_Days);
		}
		
		L[16] = L[14]  * Num_Days * 0.00008;
		L[17] = L[15] - L[16];
	
		for(i = 10; i <= 16; i++)
			showline( i );
	
		showline_wmsg(17, "Enter this amount on Form 1040, 1040-SR, or\n \
		1040-NR, line 38; or Form 1041, line 27.\n");
	}

	/* Regular Method */

	/* Inputs must be read even if Regular Method was not selected; */
	/* otherwise, error message re: unexpected input is thrown */
	
	   GetLine( "SecA_19a", &A[19] );
	   GetLine( "SecA_19b", &B[19] );
	   GetLine( "SecA_19c", &C[19] );
	   GetLine( "SecA_19d", &D[19] );
	
	   GetLine( "SchdAI_1a", &a[1] );
	   GetLine( "SchdAI_1b", &b[1] );
	   GetLine( "SchdAI_1c", &c[1] );
	   GetLine( "SchdAI_1d", &d[1] );  
	
	   GetLine( "SchdAI_4a", &a[4] );
	   GetLine( "SchdAI_4b", &b[4] );
	   GetLine( "SchdAI_4c", &c[4] );
	   GetLine( "SchdAI_4d", &d[4] ); 
	
	    GetLine( "SchdAI_7a", &a[7] );
	    b[7] = a[7];
	    c[7] = a[7];
	    d[7] = a[7];
	
	    GetLine( "SchdAI_9a", &a[9] );
	   GetLine( "SchdAI_9b", &b[9] );
	   GetLine( "SchdAI_9c", &c[9] );
	   GetLine( "SchdAI_9d", &d[9] ); 
	
	    GetLine( "SchdAI_12a", &a[12] );
	    b[12] = a[12];
	    c[12] = a[12];
	    d[12] = a[12];
	
	    GetLine( "SchdAI_14a", &a[14] );
	   GetLine( "SchdAI_14b", &b[14] );
	   GetLine( "SchdAI_14c", &c[14] );
	   GetLine( "SchdAI_14d", &d[14] );
	
	    GetLine( "SchdAI_16a", &a[16] );
	   GetLine( "SchdAI_16b", &b[16] );
	   GetLine( "SchdAI_16c", &c[16] );
	   GetLine( "SchdAI_16d", &d[16] );
	
	    GetLine( "SchdAI_18a", &a[18] );
	   GetLine( "SchdAI_18b", &b[18] );
	   GetLine( "SchdAI_18c", &c[18] );
	   GetLine( "SchdAI_18d", &d[18] );
	
	    GetLine( "SchdAI_28a", &a[28] );
	   GetLine( "SchdAI_28b", &b[28] );
	   GetLine( "SchdAI_28c", &c[28] );
	   GetLine( "SchdAI_28d", &d[28] );
	
	    GetLine( "SchdAI_30a", &a[30] );
	   GetLine( "SchdAI_30b", &b[30] );
	   GetLine( "SchdAI_30c", &c[30] );
	   GetLine( "SchdAI_30d", &d[30] );
	
	    GetLine( "SchdAI_32a", &a[32] );
	   GetLine( "SchdAI_32b", &b[32] );
	   GetLine( "SchdAI_32c", &c[32] );
	   GetLine( "SchdAI_32d", &d[32] );
	
	 GetLine( "L27", &L[27] );
	
	  /* Regular Method - Schedule AI - PART 1 */

	if(individual == Yes){
	
		a[2] = 4.0;
		b[2] = 2.4;
		c[2] = 1.5;
		d[2] = 1.0;
	}
	else{
		a[2] = 6.0;
		b[2] = 3.0;
		c[2] = 1.71429;
		d[2] = 1.09091;
	}
	
		a[3] = a[1] * a[2];
		b[3] = b[1] * b[2];
		c[3] = c[1] * c[2];
		d[3] = d[1] * d[2];
	
		a[5] = a[2];
		b[5] = b[2];
		c[5] = c[2];
		d[5] = d[2];
	
		a[6] = a[4] * a[5];
		b[6] = b[4] * b[5];
		c[6] = c[4] * c[5];
		d[6] = d[4] * d[5];

		a[8] = LargerOf(a[6], a[7]);
		b[8] = LargerOf(b[6], b[7]);
		c[8] = LargerOf(c[6], c[7]);
		d[8] = LargerOf(d[6], d[7]);
	
		if(individual == Yes){
	
			a[10] = a[8] + a[9];		
			b[10] = b[8] + b[9];
			c[10] = c[8] + c[9];
			d[10] = d[8] + d[9];
	
	       	        a[11] = a[3] - a[10];	
			b[11] = b[3] - b[10];
			c[11] = c[3] - c[10];
			d[11] = d[3] - d[10];
		}
		else{
	       	        a[11] = a[3] - a[9];	
			b[11] = b[3] - b[9];
			c[11] = c[3] - c[9];
			d[11] = d[3] - d[9];		
		}
		
		a[13] = NotLessThanZero(a[11] - a[12]);
		b[13] = NotLessThanZero(b[11] - b[12]);
		c[13] = NotLessThanZero(c[11] - c[12]);
		d[13] = NotLessThanZero(d[11] - d[12]);

		a[14] = TaxRateFunction(a[13], status);
		b[14] = TaxRateFunction(b[13], status);
		c[14] = TaxRateFunction(c[13], status);
		d[14] = TaxRateFunction(d[13], status);
	
		/* Interrupt Part I to Calculate Line 15 */
	
		/* Regular Method - Part II - Annualized Self-Employment Tax */
	
		a[29] = 34425;
		b[29] = 57375;
		c[29] = 91800;
		d[29] = 127700;
	
		a[31] = NotLessThanZero(a[29] - a[30]);
		b[31] = NotLessThanZero(b[29] - b[30]);	
		c[31] = NotLessThanZero(c[29] - c[30]);
		d[31] = NotLessThanZero(d[29] - d[30]);
	
		a[33] = a[32] * SmallerOf(a[28], a[31]);
		b[33] = b[32] * SmallerOf(b[28], b[31]);
		c[33] = c[32] * SmallerOf(c[28], c[31]);
		d[33] = d[32] * SmallerOf(d[28], d[31]);
	
		a[34] = 0.116;
		b[34] = 0.0696;
		c[34] = 0.0435;
		d[34] = 0.029;
	
		a[35] = round(a[28] * a[34]);
		b[35] = round(b[28] * b[34]);	
		c[35] = round(c[28] * c[34]);
		d[35] = round(d[28] * d[34]);
	
		a[36] = a[33] + a[35];
		b[36] = b[33] + b[35];	
		c[36] = c[33] + c[35];
		d[36] = d[33] + d[35];
	
		/* End Part II Annualized Self-Employment Tax */
		/* Continue Part I */
	
		a[15] = a[36];
		b[15] = b[36];
		c[15] = c[36];
		d[15] = d[36];	
		
		a[17] = a[14] + a[15] + a[16];
		b[17] = b[14] + b[15] + b[16];
		c[17] = c[14] + c[15] + c[16];
		d[17] = d[14] + d[15] + d[16];
	
		a[19] = NotLessThanZero(a[17] - a[18]);
		b[19] = NotLessThanZero(b[17] - b[18]);
		c[19] = NotLessThanZero(c[17] - c[18]);
		d[19] = NotLessThanZero(d[17] - d[18]);
	
		a[20] = 0.225;
		b[20] = 0.45;
		c[20] = 0.675;
		d[20] = 0.90;
	
		a[21] = round(a[19] * a[20]);
		b[21] = round(b[19] * b[20]);
		c[21] = round(c[19] * c[20]);
		d[21] = round(d[19] * d[20]);
	
		a[23] = NotLessThanZero(a[21]);
		a[24] = round(L[9] * 0.25);
		a[26] = a[24];
		a[27] = SmallerOf(a[23], a[26]);
		A[18] = a[27];
	
		b[22] = a[27];
		b[23] = NotLessThanZero(b[21] - b[22]);
		b[24] = round(L[9] * 0.25);
		b[25] = a[26] - a[27];
		b[26] = b[24] + b[25];
		b[27] = SmallerOf(b[23], b[26]);
		B[18] = b[27];
	
		c[22] = a[27] + b[27];
		c[23] = NotLessThanZero(c[21] - c[22]);
		c[24] = round(L[9] * 0.25);
		c[25] = b[26] - b[27];
		c[26] = c[24] + c[25];
		c[27] = SmallerOf(c[23], c[26]);
		C[18] = c[27];
	
		d[22] = a[27] + b[27] + c[27];
		d[23] = NotLessThanZero(d[21] - d[22]);
		d[24] = round(L[9] * 0.25);
		d[25] = c[26] - c[27];
		d[26] = d[24] + d[25];
		d[27] = SmallerOf(d[23], d[26]);
		D[18] = d[27];
	
		/* REGULAR METHOD - PART IV */
	
		A[23] = A[19];
		if(A[18] >= A[23])
			A[25] = A[18] - A[23];
		else
			A[26] = A[23] - A[18];
	
		B[20] = A[26];
		B[21] = B[19] + B[20];
		B[22] = A[24] + A[25];
		B[23] = NotLessThanZero(B[21] - B[22]);
		if(B[23] == 0)
			B[24] = B[22] - B[21];
		else
			B[24] = 0;
		if(B[18] >= B[23])
			B[25] = B[18] - B[23];
		else
			B[26] = B[23] - B[18];
	
		C[20] = B[26];
		C[21] = C[19] + C[20];
		C[22] = B[24] + B[25];
		C[23] = NotLessThanZero(C[21] - C[22]);
		if(C[23] == 0)
			C[24] = C[22] - C[21];
		else
			C[24] = 0;
		if(C[18] >= C[23])
			C[25] = C[18] - C[23];
		else
			C[26] = C[23] - C[18];
	
		D[20] = C[26];
		D[21] = D[19] + D[20];
		D[22] = C[24] + C[25];
		D[23] = NotLessThanZero(D[21] - D[22]);
		if(D[18] >= D[23])
			D[25] = D[18] - D[23];
		else
			D[26] = D[23] - D[18];

	if(Method == Regular){
	
		for(i = 18; i <= 26; i++){
			fprintf(outfile, "SecA_%d%s %0.2lf\n", i, "a", A[i]);
			fprintf(outfile, "SecA_%d%s %0.2lf\n", i, "b", B[i]);
			fprintf(outfile, "SecA_%d%s %0.2lf\n", i, "c", C[i]);
			fprintf(outfile, "SecA_%d%s %0.2lf\n", i, "d", D[i]);
		}

		for(i = 1; i <= 36; i++){
			fprintf(outfile, "SchdAI_%d%s %0.2lf\n", i, "a", a[i]);
			fprintf(outfile, "SchdAI_%d%s %0.2lf\n", i, "b", b[i]);
			fprintf(outfile, "SchdAI_%d%s %0.2lf\n", i, "c", c[i]);
			fprintf(outfile, "SchdAI_%d%s %0.2lf\n", i, "d", d[i]);
		}
	
		if((A[25] == 0) && (B[25] == 0) && (C[25] == 0) && (D[25] == 0))
			fprintf(outfile, "If line 25 on page 3 is zero for all payment periods, you don't owe a penalty.\n \
			But if you checked box C or D in Part II, you must file Form 2210 with your\n \
			return. If you checked box E, you must file page 1 of Form 2210 with your\n \
			return. In certain circumstances, the IRS will waive all or part of the\n \
			underpayment penalty. See Waiver of Penalty in the instructions.\n");
		else
			fprintf(outfile, "There is an underpayment for one or more periods.  Use the\n \
			Worksheet for Form 2210, Part IV, Section B—Figure the Penalty in the instructions\n \
			and enter the penalty amount in the OTS GUI for Form 2210.\n");
	}

  /*** 
    Summary of useful functions:
	GetLine( "label", &variable )	- Looks for "label" in input file, and places the corresponding sum of 
					  values following that label (until ";") into variable.
	GetLineF( "label", &variable )	- Like GetLine() above, but also writes the result to the output file.
	GetLineFnz(( "label", &variable ) - Like GetLine(), but only writes non-zero values to the output file.
	GetLine1( "label", &variable )  - Like GetLine() above, but expects single value (no sum, no ";" in input file).

	c = SmallerOf( a, b );		- Selects smaller of two values.
	c = LargerOf( a, b );		- Selects larger of two values.
	c = NotLessThanZero( a );	- Selects positive value or zero. Prevents negative values.

	showline( j )			- Writes currency value of L[j] to output file with label in nice format.
	shownum( j )			- Writes integer value of L[j] to output file with label in nice format.
	showline_wmsg( j, "msg" )	- Like showline, but adds the provided message to the output line.
	ShowLineNonZero( j )		- Like showline, but only writes non-zero values.
	ShowLineNonZero_wMsg( j, "msg" ) - Like showline_wmsg, but only writes non-zero values.
	showline_wlabel( "label", value ) - For custom line names and variables not in the default L[] array.
	showline_wlabelnz( "label", value ) - Like showline_wlabel, but only writes non-zero values.
	showline_wlabelmsg( "label", value, "msg" ) - Like showline_wlabel,but adds the provided message to the output line.
	
  ***/

 fclose(infile);
 grab_any_pdf_markups( infname, outfile );
 fclose(outfile);

 printf("\nListing results from file: %s\n\n", outfname);
 Display_File( outfname );

 return 0;
}
