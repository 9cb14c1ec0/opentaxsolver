/************************************************************************/
/* TaxSolve_NY_IT-201_2018.c - NY State Tax form IT-201 for 2018.	*/
/* Copyright (C) 2003-2019 - Aston Roberts, Skeet Monker		*/
/* 									*/
/* Compile:   gcc taxsolve_NY_IT201_2018.c -o taxsolve_NY_IT201_2018	*/
/* Run:	      ./taxsolve_NY_IT201_2018  NY_IT201_2018.txt 		*/
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
/* Aston Roberts 4-8-2003	aston_roberts@yahoo.com			*/
/* Modified for NY 2003 taxes - Skeet Monker				*/
/* Modified for NY 2004 taxes - A Roberts				*/
/* Modified for NY 2005-2019 taxes - Skeet Monker			*/
/************************************************************************/

float thisversion=16.00;

#include "taxsolve_routines.c"

double A[10], S[10];

#define SINGLE                  1
#define MARRIED_FILLING_JOINTLY 2
#define MARRIED_FILLING_SEPARAT 3
#define HEAD_OF_HOUSEHOLD       4
#define WIDOW                   5
#define Yes 1
#define No  0

int 	status=0;

char 	statusnames[10][20]={"0","Single","Married/Joint","Married/Sep","Head_of_House","Widow"};
char 	*Your1stName="", *YourLastName="", *YourInitial="", 
	*Spouse1stName="", *SpouseLastName="", *SpouseInitial="";
char	*YourSocSec=0, *SpouseSocSec=0, *MailAddress=0, *AptNumber=0,
	Town[2048]="", StateName[1024]="", Zipcode[1024]="";

struct FedReturnData
 {
  double fedline[MAX_LINES], schedA[MAX_LINES], schedD[MAX_LINES];
  int Exception, Itemized;
 } PrelimFedReturn;


void convert_slashes( char *fname )
{ /* Convert slashes in file name based on machine type. */
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


char *pull_initial( char *name )
{ /* Expect names like:  "John, D.", and pull initial out. */
  int j=0;
  char midinitial[10];
  while ((name[j] != '\0') && (name[j] != ','))
   j++;
  if (name[j] == ',')
   {
    name[j++] = '\0';
    while ((name[j] != '\0') && (isspace( name[j] )))
     j++;
    midinitial[0] = name[j];
    midinitial[1] = '\0';
   }
  else
   strcpy( midinitial, "" );
  return strdup( midinitial );
}



int ImportFederalReturnData( char *fedlogfile, struct FedReturnData *fed_data )
{
 FILE *infile;
 char fline[2000], word[2000];
 int linenum;

 for (linenum=0; linenum<MAX_LINES; linenum++) 
  { 
   fed_data->fedline[linenum] = 0.0;
   fed_data->schedA[linenum] = 0.0;
   fed_data->schedD[linenum] = 0.0;
  }
 convert_slashes( fedlogfile );
 infile = fopen(fedlogfile, "r");
 if (infile==0)
  {
   printf("Error: Could not open Federal return '%s'\n", fedlogfile);
   fprintf(outfile,"Error: Could not open Federal return '%s'\n", fedlogfile);
   return 0; 
  }
 fed_data->Itemized = 1; /* Set initial default values. */
 read_line(infile,fline);  linenum = 0;
 while (!feof(infile))
  {
   if (strstr(fline,"Use standard deduction.")!=0) fed_data->Itemized = 0;
   next_word(fline, word, " \t=");
   if ((strstr(word,"L")==word) && (strstr(fline," = ")!=0))
    {
     if (strcmp(word,"L9b") != 0)
      {
       if (sscanf(&word[1],"%d",&linenum)!=1)
	{
	 printf("Error: Reading Fed line number '%s%s'\n",word,fline);
	 fprintf(outfile,"Error: Reading Fed line number '%s%s'\n",word,fline);
	}
       next_word(fline, word, " \t=");
       if (sscanf(word,"%lf", &fed_data->fedline[linenum])!=1)
	{
	 printf("Error: Reading Fed line %d '%s%s'\n",linenum,word,fline);
	 fprintf(outfile,"Error: Reading Fed line %d '%s%s'\n",linenum,word,fline);
	}
       if (verbose) printf("FedLin[%d] = %2.2f\n", linenum, fed_data->fedline[linenum]);
      }
    }
   else
   if ((strstr(word,"A")==word) && (strstr(word,"AMT")!=word) && (strstr(fline," = ")!=0))
    {
     if (sscanf(&word[1],"%d",&linenum)!=1)
      {
	printf("Error: Reading Fed line number '%s%s'\n",word,fline);
	fprintf(outfile,"Error: Reading Fed line number '%s%s'\n",word,fline);
      }
     next_word(fline, word, " \t=");
     if (sscanf(word,"%lf", &fed_data->schedA[linenum])!=1) 
       {
	printf("Error: Reading Fed schedA %d '%s%s'\n",linenum,word,fline);
	fprintf(outfile, "Error: Reading Fed schedA %d '%s%s'\n",linenum,word,fline);
       }
     if (verbose) printf("FedLin[%d] = %2.2f\n", linenum, fed_data->schedA[linenum]);
    }
   else
   if ((strstr(word,"D")==word) && (strstr(fline," = ")!=0))
    {
     if (sscanf(&word[1],"%d",&linenum)!=1)
      {
	printf("Error: Reading Fed line number '%s%s'\n",word,fline);
	fprintf(outfile,"Error: Reading Fed line number '%s%s'\n",word,fline);
      }
     next_word(fline, word, " \t=");
     if (strcmp(word,"d") == 0)
      { /*Basis,Sale,Gain line. Capture last value.*/
	next_word(fline, word, " \teh=" );
	while (word[0] != '\0')
	 {
	  if (sscanf(word,"%lf", &fed_data->schedD[linenum]) != 1) 
	   fprintf(outfile,"Error: Reading Fed schedD %d '%s %s'\n", linenum, word, fline);
	  next_word(fline, word, " \teh=" );
	 }
      }
     else
     if (sscanf(word,"%lf", &fed_data->schedD[linenum]) != 1) 
      {
       if (strncasecmp(word,"yes",1) == 0) fed_data->schedD[linenum] = 1;
       else
       if (strncasecmp(word,"no",1) == 0) fed_data->schedD[linenum] = 0;
       else
        {
         printf("Error: Reading fed schedD %d '%s%s'\n", linenum, word, fline);
	 fprintf(outfile,"Error: Reading Fed schedD %d '%s%s'\n", linenum, word, fline);
        }
      }
     if (verbose) printf("FedLin[%d] = %2.2f\n", linenum, fed_data->schedD[linenum]);
    }
   else
   if (strcmp(word,"Status") == 0)
    {
     next_word(fline, word, " \t=");
     fprintf(outfile," Status %s\n", word );
     if (strncasecmp(word,"Single",4)==0)
	status = SINGLE;
     else
     if (strncasecmp(word,"Married/Joint",13)==0)
	status = MARRIED_FILLING_JOINTLY; 
     else
     if (strncasecmp(word,"Married/Sep",11)==0)
	status = MARRIED_FILLING_SEPARAT;
     else
     if (strncasecmp(word,"Head_of_House",4)==0)
	status = HEAD_OF_HOUSEHOLD;
     else
     if (strncasecmp(word,"Widow",4)==0)
	status = WIDOW;
     else 
      { 
       printf("Error: unrecognized status '%s'. Exiting.\n", word); 
       fprintf(outfile,"Error: unrecognized status '%s'. Exiting.\n", word);
       return 0; 
      }
    }
   else
   if (strcmp(word,"Your1stName:") == 0)
    {
	Your1stName = strdup( fline );
	YourInitial = pull_initial( Your1stName );
    }
   else
   if (strcmp(word,"YourLastName:") == 0)
    {
	YourLastName = strdup( fline );
    }
   else
   if (strcmp(word,"YourSocSec#:") == 0)
    {
	YourSocSec = strdup( fline );
    }
   else
   if (strcmp(word,"Spouse1stName:") == 0)
    {
	Spouse1stName = strdup( fline );
	SpouseInitial = pull_initial( Spouse1stName );
    }
   else
   if (strcmp(word,"SpouseLastName:") == 0)
    {
	SpouseLastName = strdup( fline );
    }
   else
   if (strcmp(word,"SpouseSocSec#:") == 0)
    {
	SpouseSocSec = strdup( fline );
    }
   else
   if (strcmp(word,"Number&Street:") == 0)
    {
	MailAddress = strdup( fline );
    }
   else
   if (strcmp(word,"Apt#:") == 0)
    {
	AptNumber = strdup( fline );
    }
   else
   if (strcmp(word,"TownStateZip:") == 0)
    { /* Expect:  town name, NY, 10033	*/
     next_word( fline, Town, "," );
     next_word( fline, StateName, " \t," );
     next_word( fline, Zipcode, " \t," );
    }

   read_line(infile,fline);
  }
 fclose(infile);
 return 1;
}



double TaxRateFunction( double income, int status )
{
 double tax;
 switch (status)
  {
   case MARRIED_FILLING_JOINTLY: case WIDOW:					/* Not Updated for 2018. */
	if (income <=   17150.0) tax = 	         0.04 * income; else		/* Data from pg 57. */
	if (income <=   23600.0) tax =   686.0 + 0.045  * (income - 17150.0); else
	if (income <=   27900.0) tax =   976.0 + 0.0525 * (income - 23600.0); else
	if (income <=   43000.0) tax =  1202.0 + 0.059  * (income - 27900.0); else
	if (income <=  161550.0) tax =  2093.0 + 0.0645 * (income - 43000.0); else
	if (income <=  323200.0) tax =  9739.0 + 0.0665 * (income - 161550.0); else
	if (income <= 2155350.0) tax = 20498.0 + 0.0685 * (income - 323200.0); 
	else			tax = 145991.0 + 0.0882 * (income - 2155350.0);
      break;
   case SINGLE: case MARRIED_FILLING_SEPARAT:
	if (income <=    8500.0) tax =  	  0.04   * income; else
	if (income <=   11700.0) tax =    340.0 + 0.045  * (income - 8500.0); else
	if (income <=   13900.0) tax =    484.0 + 0.0525 * (income - 11700.0); else
	if (income <=   21400.0) tax =    600.0 + 0.059  * (income - 13900.0); else
	if (income <=   80650.0) tax =   1042.0 + 0.0645 * (income - 21400.0); else
	if (income <=  215400.0) tax =   4864.0 + 0.0665 * (income - 80650.0); else
	if (income <= 1077550.0) tax =  13825.0 + 0.0685 * (income - 215400.0); 
	else 		 	 tax =  72882.0 + 0.0882 * (income - 1077550.0);
      break;
   case HEAD_OF_HOUSEHOLD:
	if (income <=   12080.0) tax = 	         0.04 * income; else
	if (income <=   17650.0) tax =   512.0 + 0.045  * (income - 12800.0); else
	if (income <=   20900.0) tax =   730.0 + 0.0525 * (income - 17650.0); else
	if (income <=   32200.0) tax =   901.0 + 0.059  * (income - 20900.0); else
	if (income <=  107650.0) tax =  1568.0 + 0.0645 * (income - 32200.0); else
	if (income <=  269300.0) tax =  6434.0 + 0.0665 * (income - 107650.0); else
	if (income <= 1616450.0) tax = 17184.0 + 0.0685 * (income - 269300.0);
	else			tax = 109464.0 + 0.0882 * (income - 1616450.0);
      break;
   default: printf("Error: Unhandled status\n"); exit(0); break;
  }
 return tax;
}


void Report_bracket_info( double income, double tx, int status )
{
 double rate;
 switch (status)
  {
   case MARRIED_FILLING_JOINTLY: case WIDOW:				/* Not Updated for 2018. */
	if (income <=   17150.0) rate = 0.04;  else
	if (income <=   23600.0) rate = 0.045;  else
	if (income <=   27900.0) rate = 0.0525;  else
	if (income <=   43000.0) rate = 0.059;  else
	if (income <=  161550.0) rate = 0.0645;  else
	if (income <=  323200.0) rate = 0.0665;  else
	if (income <= 2155350.0) rate = 0.0685;  else  rate = 0.0882;
      break;
   case SINGLE: case MARRIED_FILLING_SEPARAT:
	if (income <=    8500.0) rate = 0.04;  else
	if (income <=   11700.0) rate = 0.045;  else
	if (income <=   13900.0) rate = 0.0525;  else
	if (income <=   21400.0) rate = 0.059;  else
	if (income <=   80650.0) rate = 0.0645;  else
	if (income <=  215400.0) rate = 0.0665;  else
	if (income <= 1077550.0) rate = 0.0685;  else  rate = 0.0882;
      break;
   case HEAD_OF_HOUSEHOLD:
	if (income <=   12800.0) rate = 0.04;  else
	if (income <=   17650.0) rate = 0.045;  else
	if (income <=   20900.0) rate = 0.0525;  else
	if (income <=   32200.0) rate = 0.059;  else
	if (income <=  107650.0) rate = 0.0645;  else
	if (income <=  269300.0) rate = 0.0665;  else
	if (income <= 1616450.0) rate = 0.0685;  else  rate = 0.0882;
      break;
   default: printf("Error: Unhandled status\n"); exit(0); break;
  }
 printf(" You are in the %2.1f%% marginal tax bracket,\n and you are paying an effective %2.1f%% tax on your total income.\n",
	  100.0 * rate, 100.0 * tx / income );
 fprintf(outfile," You are in the %2.1f%% marginal tax bracket,\n and you are paying an effective %2.1f%% tax on your total income.\n",
	  100.0 * rate, 100.0 * tx / income );
}


double TaxRateLookup( double income, int status )
{
 double tax, dx;
 int m;

 if (income < 25.0)  dx = 12.5;  else
 if (income < 50.0)  dx = 25.0;  else  dx = 50.0;

 /* Round and truncate results from tax-function to approximate table lookup. */
 m = income / dx;             /* Round income to nearest $50. */
 income = (double)m * dx + 0.5 * dx;      /* Place into center of a $50 bracket. */
 tax = TaxRateFunction( income, status );

 return (int)(tax + 0.5);
}


double NYcityTaxRateFunction( double income, int status )	/* From page 68. */
{
 double tax, dx;
 int m;

 if (income < 25.0) dx = 12.5; else
 if (income < 50.0) dx = 25.0; else dx = 50.0;

 m = income / dx;             /* Round income to nearest $50. */
 if (income < 65000.0)
  income = m * dx + 0.5 * dx;      /* Place into center of a $50 bracket. */

 if ((status==MARRIED_FILLING_JOINTLY) || (status==WIDOW))		/* Not Updated for 2018. */
  {
   if (income < 21600.0)  tax = income * 0.03078; else
   if (income < 45000.0)  tax = (income - 21600.00) * 0.03762 + 665.00; else
   if (income < 90000.0)  tax = (income - 45000.00) * 0.03819 + 1545.0; else
			  tax = (income - 90000.00) * 0.03876 + 3264.0;
  }
 else
 if ((status==SINGLE) || (status==MARRIED_FILLING_SEPARAT))
  {
   if (income < 12000.0)  tax = income * 0.03078; else
   if (income < 25000.0)  tax = (income - 12000.00) * 0.03762 + 369.0;  else
   if (income < 50000.0)  tax = (income - 25000.00) * 0.03819 + 858.0;  else
			  tax = (income - 50000.00) * 0.03876 + 1813.00;
  }
 else
 if (status==HEAD_OF_HOUSEHOLD) 
  {
   if (income < 14400.00) tax = income * 0.03078; else
   if (income < 30000.00) tax = (income - 14400.00) * 0.03762 +  443.0;  else
   if (income < 60000.00) tax = (income - 30000.00) * 0.03819 + 1030.0;  else
			  tax = (income - 60000.00) * 0.03876 + 2176.0;
 }
 else {printf("Status not covered.\n");  exit(1);}

 if (income < 65000.0) tax = (int)(tax + 0.5);   /* Round result to whole dollar. */
 return tax;
}


void worksheet1()	/*Tax Computation Worksheet 1 (pg 58) */		/* Not Updated for 2018. */
{ double ws[100];
  ws[1] = L[33];
  ws[2] = L[38];
  ws[3] = 0.0645 * ws[2];
  if (ws[1] >= 157650.0)
    ws[9] = ws[3];
  else
   {
    ws[4] = TaxRateFunction( ws[2], status );
    ws[5] = ws[3] - ws[4];
    ws[6] = ws[1] - 107650.0;
    /* Divide by 50k and round to forth decimal place. */
    ws[7] = 0.0001 * (double)Round( 10000.0 * (ws[6] / 50000.0) );
    ws[8] = ws[5] * ws[7];
    ws[9] = ws[4] + ws[8];
   }
  L[39] = ws[9];
}


void worksheet2()	/*Tax Computation Worksheet 2 (pg 58) */
{ double ws[100];
  ws[1] = L[33];
  ws[2] = L[38];
  ws[3] = 0.0665 * ws[2];
  if (ws[1] >= 211550.0)
    ws[11] = ws[3];
  else
   {
    ws[4] = TaxRateFunction( ws[2], status );
    ws[5] = ws[3] - ws[4];
    ws[6] = 681.0;
    ws[7] = ws[5] - ws[6];
    ws[8] = ws[1] - 160500.0;
    /* Divide by 50k and round to forth decimal place. */
    ws[9] = 0.0001 * (double)Round( 10000.0 * (ws[8] / 50000.0) );
    ws[10] = ws[7] * ws[9];
    ws[11] = ws[4] + ws[6] + ws[10];
   }
  L[39] = ws[11];
}


void worksheet3()	/*Tax Computation Worksheet 3 (pg 58) */
{ double ws[100];
  ws[1] = L[33];
  ws[2] = L[38];
  ws[3] = 0.0685 * ws[2];
  if (ws[1] >= 373200.0)
   ws[11] = ws[3];
  else
   {
    ws[4] = TaxRateFunction( ws[2], status );
    ws[5] = ws[3] - ws[4];
    ws[6] = 1004.0;
    ws[7] = ws[5] - ws[6];
    ws[8] = ws[1] - 323200.0;
    /* Divide by 50k and round to forth decimal place. */
    ws[9] = 0.0001 * (double)Round( 10000.0 * (ws[8] / 50000.0) );
    ws[10] = ws[7] * ws[9];
    ws[11] = ws[4] + ws[6] + ws[10];
    }
   L[39] = ws[11];
  }


void worksheet4()	/*Tax Computation Worksheet 4 (pg 58) */
{ double ws[100];
  ws[1] = L[33];
  ws[2] = L[38];
  ws[3] = 0.0882 * ws[2];
  if (ws[1] >= 2205350.0)
   ws[11] = ws[3];
  else
   {
    ws[4] = TaxRateFunction( ws[2], status );
    ws[5] = ws[3] - ws[4];
    if (ws[2] <= 161550.0)
     ws[6] = 681.0;
    else
    if (ws[2] <= 323200.0)
     ws[6] = 1004.0;
    else
     ws[6] = 1650.0;
    ws[7] = ws[5] - ws[6];
    ws[8] = ws[1] - 2155350.0;
    /* Divide by 50k and round to forth decimal place. */
    ws[9] = 0.0001 * (double)Round( 10000.0 * (ws[8] / 50000.0) );
    ws[10] = ws[7] * ws[9];
    ws[11] = ws[4] + ws[6] + ws[10];
    }
   L[39] = ws[11];
  }


void worksheet5()	/*Tax Computation Worksheet 5 (pg 59) */
{ double ws[100];
  ws[1] = L[33];
  ws[2] = L[38];
  ws[3] = 0.0665 * ws[2];
  if (ws[1] >= 157650.0)
    ws[9] = ws[3];
  else
   {
    ws[4] = TaxRateFunction( ws[2], status );
    ws[5] = ws[3] - ws[4];
    ws[6] = ws[1] - 107650.0;
    /* Divide by 50k and round to forth decimal place. */
    ws[7] = 0.0001 * (double)Round( 10000.0 * (ws[6] / 50000.0) );
    ws[8] = ws[5] * ws[7];
    ws[9] = ws[4] + ws[8];
   }
  L[39] = ws[9];
}


void worksheet6()	/*Tax Computation Worksheet 6 (pg 59) */
{ double ws[100];
  ws[1] = L[33];
  ws[2] = L[38];
  ws[3] = 0.0685 * ws[2];
  if (ws[1] >= 265400.0)
    ws[11] = ws[3];
  else
   {
    ws[4] = TaxRateFunction( ws[2], status );
    ws[5] = ws[3] - ws[4];
    ws[6] = 500.0;
    ws[7] = ws[5] - ws[6];
    ws[8] = ws[1] - 214000.0;
    /* Divide by 50k and round to forth decimal place. */
    ws[9] = 0.0001 * (double)Round( 10000.0 * (ws[8] / 50000.0) );
    ws[10] = ws[7] * ws[9];
    ws[11] = ws[4] + ws[6] + ws[10];
   }
  L[39] = ws[11];
}


void worksheet7()	/*Tax Computation Worksheet 7 (pg 59) */
{ double ws[100];
  ws[1] = L[33];
  ws[2] = L[38];
  ws[3] = 0.0882 * ws[2];
  if (ws[1] >= 1127550.0)
   ws[11] = ws[3];
  else
   {
    ws[4] = TaxRateFunction( ws[2], status );
    ws[5] = ws[3] - ws[4];
    if (ws[2] <= 215400.0)
     ws[6] = 500.0;
    else
     ws[6] = 930.0;
    ws[7] = ws[5] - ws[6];
    ws[8] = ws[1] - 1077550.0;
    /* Divide by 50k and round to forth decimal place. */
    ws[9] = 0.0001 * (double)Round( 10000.0 * (ws[8] / 50000.0) );
    ws[10] = ws[7] * ws[9];
    ws[11] = ws[4] + ws[6] + ws[10];
    }
   L[39] = ws[11];
  }


void worksheet8()	/*Tax Computation Worksheet 8 (pg 60) */
{ double ws[100];
  ws[1] = L[33];
  ws[2] = L[38];
  ws[3] = 0.0665 * ws[2];
  if (ws[1] >= 157650.0)
    ws[9] = ws[3];
  else
   {
    ws[4] = TaxRateFunction( ws[2], status );
    ws[5] = ws[3] - ws[4];
    ws[6] = ws[1] - 107650.0;
    /* Divide by 50k and round to forth decimal place. */
    ws[7] = 0.0001 * (double)Round( 10000.0 * (ws[6] / 50000.0) );
    ws[8] = ws[5] * ws[7];
    ws[9] = ws[4] + ws[8];
   }
  L[39] = ws[9];
}


void worksheet9()	/*Tax Computation Worksheet 9 (pg 60) */
{ double ws[100];
  ws[1] = L[33];
  ws[2] = L[38];
  ws[3] = 0.0685 * ws[2];
  if (ws[1] >= 319300.0)
    ws[11] = ws[3];
  else
   {
    ws[4] = TaxRateFunction( ws[2], status );
    ws[5] = ws[3] - ws[4];
    ws[6] = 725.0;
    ws[7] = ws[5] - ws[6];
    ws[8] = ws[1] - 269300.0;
    /* Divide by 50k and round to forth decimal place. */
    ws[9] = 0.0001 * (double)Round( 10000.0 * (ws[8] / 50000.0) );
    ws[10] = ws[7] * ws[9];
    ws[11] = ws[4] + ws[6] + ws[10];
   }
  L[39] = ws[11];
}


void worksheet10()	/*Tax Computation Worksheet 10 (pg 60) */
{ double ws[100];
  ws[1] = L[33];
  ws[2] = L[38];
  ws[3] = 0.0882 * ws[2];
  if (ws[1] >= 1666450.0)
   ws[11] = ws[3];
  else
   {
    ws[4] = TaxRateFunction( ws[2], status );
    ws[5] = ws[3] - ws[4];
    if (ws[2] <= 269300.0)
     ws[6] = 725.0;
    else
     ws[6] = 1263.0;
    ws[7] = ws[5] - ws[6];
    ws[8] = ws[1] - 1616550.0;
    /* Divide by 50k and round to forth decimal place. */
    ws[9] = 0.0001 * (double)Round( 10000.0 * (ws[8] / 50000.0) );
    ws[10] = ws[7] * ws[9];
    ws[11] = ws[4] + ws[6] + ws[10];
    }
   L[39] = ws[11];
  }


void tax_computation_worksheet( int status )
{ /* Worksheets from pages 58-60. Come here when AGI L[33] > $106,950. */
 switch (status)								/* Not Updated for 2018. */
  {
     case MARRIED_FILLING_JOINTLY:  case WIDOW:
	if (L[33] <= 2155350.0)
	 {
	   if (L[38] <= 161550.0)
	    worksheet1();
	   else
	   if ((L[33] > 161550.0) && (L[38] <= 323200.0))
	    worksheet2();
	   else
	   if ((L[33] > 323200.0) && (L[38] > 323200.0))
	    worksheet3();
	   else
	    worksheet4();
	 }
	else
	 worksheet4();
	break;
     case SINGLE:  case MARRIED_FILLING_SEPARAT:
	if (L[33] <= 1077550.0)
	 {
	   if (L[38] <= 215400.0)
	    worksheet5();
	   else
	    worksheet6();
	 }
	else
	 worksheet7();
	break;
     case HEAD_OF_HOUSEHOLD:
	if (L[33] <= 1616450.0)
	 {
	   if (L[38] <= 269300.0)
	    worksheet8();
	   else
	    worksheet9();
	 }
	else
	 worksheet10();
	break;
     default: printf("Case not handled.\n");  fprintf(outfile,"Case not handled.\n"); exit(1);
  }
}


/*----------------------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
 int j, k, argk, day, month, yyyy;
 char word[1000], outfname[1000], *answ;
 time_t now;
 int Dependent, Exemptions, nyc_resident;
 double itemized_ded, std_ded=0.0, LTC=0, AddAdj=0.0, CollegeDed=0.0;
 double ded_sched[MAX_LINES];
 char prelim_1040_outfilename[5000];
 char YourNames[2048]="";

 /* Intercept any command-line arguments. */
 printf("NY-IT201 - 2011 - v%3.1f\n", thisversion);
 argk = 1;  k=1;
 while (argk < argc)
 {
  if (strcmp(argv[argk],"-verbose")==0)  verbose = 1;
  else
  if (k==1)
   {
    infile = fopen(argv[argk],"r");
    if (infile==0) {printf("ERROR: Parameter file '%s' could not be opened.\n", argv[argk]);  exit(1);}
    k = 2;
    /* Base name of output file on input file. */
    strcpy(outfname,argv[argk]);
    j = strlen(outfname)-1;
    while ((j>=0) && (outfname[j]!='.')) j--;
    if (j<0) strcat(outfname,"_out.txt"); else strcpy(&(outfname[j]),"_out.txt");
    outfile = fopen(outfname,"w");
    if (outfile==0) {printf("ERROR: Output file '%s' could not be opened.\n", outfname);  exit(1);}
    printf("Writing results to file:  %s\n", outfname);
   }
  else {printf("Unknown command-line parameter '%s'\n", argv[argk]);  exit(1);}
  argk = argk + 1;
 }

 if (infile==0) {printf("Error: No input file on command line.\n"); exit(1);}

 /* Pre-initialize all lines to zeros. */
 for (j=0; j<MAX_LINES; j++) 
  {
   L[j] = 0.0;
   ded_sched[j] = 0.0;
  }

 /* Accept parameters from input file. */
 /* Expect  NY IT200 form lines, something like:
	Title:  NY IT-200 1999 Return
	L1	;	{Wages}
	L2	;	{Interest}
	L3	;	{Dividends}
	...
*/

 /* Accept Form's "Title" line, and put out with date-stamp for records. */
 read_line( infile, word );
 now = time(0);
 fprintf(outfile,"\n%s,	 v%2.2f, %s\n", word, thisversion, ctime( &now ));

 get_parameter( infile, 's', word, "FileName" );      /* Preliminary Return Output File-name. */
 get_word(infile, prelim_1040_outfilename );
 if (ImportFederalReturnData( prelim_1040_outfilename, &PrelimFedReturn ) == 0)
  {
   fclose(infile);
   fclose(outfile);
   Display_File( outfname );
   exit(1);
  }

 answ = GetTextLine( "YourDOB" );
 if (interpret_date( answ, &day, &month, &yyyy, "reading 'YourDOB'" ))
   fprintf(outfile,"YourDOB %s\n", format_mmddyyyy( month, day, yyyy ) );
 else
  fprintf(outfile,"YourDOB %s\n", answ );

 answ = GetTextLine( "SpouseDOB" );
 if (interpret_date( answ, &day, &month, &yyyy, "reading 'SpouseDOB'" ))
   fprintf(outfile,"SpouseDOB %s\n", format_mmddyyyy( month, day, yyyy ) );
 else
  fprintf(outfile,"SpouseDOB %s\n", answ );

 GetTextLineF( "County" );
 GetTextLineF( "SchooldDist" );
 GetTextLineF( "SchoolCode" );

 answ = GetTextLineF( "D1_ForeignAcct" );
 next_word( answ, word, " \t;" );
 if (toupper( word[0] ) == 'Y')
  fprintf(outfile,"CkD1y: X\n");
 else
  fprintf(outfile,"CkD1n: X\n");

 answ = GetTextLineF( "D2_1-YonkRelCred" );
 next_word( answ, word, " \t;" );
 if (toupper( word[0] ) == 'Y')
  fprintf(outfile,"CkD2_1y: X\n");
 else
 if ((mystrcasestr( word, "N/A" ) == 0) && (toupper( word[0] ) == 'N'))
  fprintf(outfile,"CkD2_1n: X\n");

 GetTextLineF( "D2_2-YRCamount" );
 
 answ = GetTextLineF( "D3-NonQualComp" );
 next_word( answ, word, " \t;" );
 if (toupper( word[0] ) == 'Y')
  fprintf(outfile,"CkD3y: X\n");
 else
  fprintf(outfile,"CkD3n: X\n");

 answ = GetTextLineF( "E1_LivedNYC" );
 next_word( answ, word, " \t;" );
 if (toupper( word[0] ) == 'Y')
  fprintf(outfile,"CkE1y: X\n");
 else
  fprintf(outfile,"CkE1n: X\n");

 GetTextLineF( "E2_DaysNYC" );
 GetTextLineF( "F1_MonthsYouNYC" );
 GetTextLineF( "F2_MonthsSpNYC" );
 GetTextLineF( "G_SpecCondCode" );

 get_parameter( infile, 's', word, "Dependent" );
 get_parameter( infile, 'b', &Dependent, "Dependent?"); 

 // GetLineF( "L1", &L[1] );	/* Wages. */
 L[1] = PrelimFedReturn.fedline[7];

 if (PrelimFedReturn.Itemized)
  {
   fprintf(outfile," Check box B = Yes\n");
   fprintf(outfile,"  Check_Itemized = X\n");
  }
 else
  {
   fprintf(outfile," Check box B = No\n");
   fprintf(outfile,"  Check_NoItemiz = X\n");
  }

 if (Dependent==1)
  {
   fprintf(outfile," Check box C = Yes\n");
   fprintf(outfile,"  Check_Depend = X\n");
  }
 else
  {
   fprintf(outfile," Check box C = No\n");
   fprintf(outfile,"  Check_NotDep = X\n");
  }

 showline(1);

 // GetLineF( "L2", &L[2] );	/* Taxable Interest. */
 L[2] = PrelimFedReturn.fedline[8];
 showline(2);

 // GetLineF( "L3", &L[3] );	/* Ordinary Dividends. */
 L[3] = PrelimFedReturn.fedline[9];
 showline(3);

 // GetLineF( "L4", &L[4] );	/* Taxable refunds, credits, offsets */
 L[4] = PrelimFedReturn.fedline[10];
 showline(4);

 // GetLineF( "L5", &L[5] );	/* Alimony received */
 L[5] = PrelimFedReturn.fedline[11];
 showline(5);

 // GetLineF( "L6", &L[6] );	/* Business income/loss (fed sched C) */
 L[6] = PrelimFedReturn.fedline[12];
 showline(6);

 // GetLineF( "L7", &L[7] );	/* Capital gains/losses (fed sched D) */
 L[7] = PrelimFedReturn.fedline[13];
 showline(7);

 // GetLineF( "L8", &L[8] );	/* Other gains/losses (fed form 4794) */
 L[8] = PrelimFedReturn.fedline[14];
 showline(8);

 // GetLineF( "L9", &L[9] );	/* Taxable IRA distributions */
 L[9] = PrelimFedReturn.fedline[15];
 showline(9);

 // GetLineF( "L10", &L[10] );	/* Taxable pension/annuity amounts  */
 L[10] = PrelimFedReturn.fedline[16];
 showline(10);

 // GetLineF( "L11", &L[11] );	/* Rental, royalties, partnership, S corp, (fed sched E) */
 L[11] = PrelimFedReturn.fedline[17];
 showline(11);

 // GetLineF( "L13", &L[13] );	/* Farm income (fed sched F) */
 L[13] = PrelimFedReturn.fedline[18];
 showline(13);

 // GetLineF( "L14", &L[14] );	/* Unemployment compensation */
 L[14] = PrelimFedReturn.fedline[19];
 showline(14);

 // GetLineF( "L15", &L[15] );	/* Taxable Social Sec. benefits */
 L[15] = PrelimFedReturn.fedline[20];
 showline(15);
 L[27] = L[15];

 // GetLineF( "L16", &L[16] );	/* Other income (pg. 14) */
 L[16] = PrelimFedReturn.fedline[21];
 showline(16);

 for (j = 1; j <= 11; j++)
  L[17] = L[17] + L[j];
 for (j=13; j <= 16; j++)
  L[17] = L[17] + L[j];
 showline(17);
 if (absolutev( L[17] - PrelimFedReturn.fedline[22]) > 0.1)
  {
   printf(" Warning: L[17] = %6.2f, while Fed-line[22] = %6.2f\n", L[17], PrelimFedReturn.fedline[22] );
   fprintf(outfile," Warning: L[17] = %6.2f, while Fed-line[22] = %6.2f\n", L[17], PrelimFedReturn.fedline[22] );
  }

 // GetLineF( "L18", &L[18] );	/* Total federal adjustments to income (pg 14) */
 L[18] = PrelimFedReturn.fedline[36];
 showline(18);

 L[19] = L[17] - L[18];
 showline_wmsg( 19, "Federal adjusted gross income" );
 if (absolutev(L[19] - PrelimFedReturn.fedline[38]) > 0.1)
  {
   printf(" Warning: L[19] = %6.2f, while Fed-line[38] = %6.2f\n", L[19], PrelimFedReturn.fedline[38] );
   fprintf(outfile," Warning: L[19] = %6.2f, while Fed-line[38] = %6.2f\n", L[19], PrelimFedReturn.fedline[38] );
  } 

 GetLineF( "L20", &L[20] );	/* Interest income from non-NY state or local bonds */

 GetLineF( "L21", &L[21] );	/* Public employee retirement contributions (pg 15) */

 GetLineF( "L22", &L[22] );	/* College choice tuition saving distributions */

 GetLineF( "L23", &L[23] );	/* Other (pg 16) */

 for (j = 19; j <= 23; j++) L[24] = L[24] + L[j];
 showline(24);

 L[25] = L[4];			/* Taxable refunds, credits, offsets */
 showline(25);

 GetLineF( "L26", &L[26] );	/* Pensions of NYS and local governments and the federal government (see page 16)  */

 L[27] = L[15];			/* Taxable amount of social security benefits */
 showline(27);

 GetLineF( "L28", &L[28] );	/* Interest income on U.S. government bonds */

 GetLine( "L29", &L[29] );	/* Pension and annuity income exclusion  */
 if (L[29] > 20000.0)
  {
   L[29] = 20000.0;
   showline_wmsg( 29, "(Limited to 20,000.)" );
  }
 else
  showline(29);

 GetLine( "L30", &L[30] );	/* College choice tuition savings deduction / earnings distributions */
 if (status == MARRIED_FILLING_JOINTLY)
  L[30] = smallerof( L[30], 10000.0 );
 else
  L[30] = smallerof( L[30], 5000.0 );
 showline(30);

 GetLineF( "L31", &L[31] );	/* Other (see page 21) */

 for (j=25; j <= 31; j++)
  L[32] = L[32] + L[j];
 showline(32);

 L[33] = L[24] - L[32];
 showline_wmsg(33,"New York adjusted gross income (AGI)");

 /* NYS Itemized Deductions Schedule, pg. 37-38 */
 GetLine( "LTcare%", &LTC );
 GetLine( "AddAdj", &AddAdj );
 GetLine( "CollegeDed", &CollegeDed );
 ded_sched[1] = PrelimFedReturn.schedA[4];
 ded_sched[2] = PrelimFedReturn.schedA[9];
 ded_sched[3] = PrelimFedReturn.schedA[15];
 ded_sched[4] = PrelimFedReturn.schedA[19];
 ded_sched[5] = PrelimFedReturn.schedA[20];
 ded_sched[6] = PrelimFedReturn.schedA[27];
 ded_sched[7] = PrelimFedReturn.schedA[28];
 ded_sched[8] = PrelimFedReturn.schedA[29];
 ded_sched[9] = PrelimFedReturn.schedA[5] + PrelimFedReturn.schedA[8] 
		+ LTC * PrelimFedReturn.schedA[4];
 ded_sched[10] = ded_sched[8] - ded_sched[9];	
 ded_sched[11] = AddAdj;
 ded_sched[12] = ded_sched[10] + ded_sched[11];
 if (L[33] <= 100000.0)
  ded_sched[13] = 0.0;
 else
  { /*L33_morethan_100000*/
   double ws[50];
   if (L[33] <= 475000.0)
    { /* Worksheet 3 pg 39 */
      ws[1] = L[33];
      switch (status)
       {
        case SINGLE:  case MARRIED_FILLING_SEPARAT: ws[2] = 100000.0; break;
        case HEAD_OF_HOUSEHOLD:                     ws[2] = 150000.0; break;
        case MARRIED_FILLING_JOINTLY:  case WIDOW:  ws[2] = 200000.0; break;
	default: ws[2] = 0.0;
       }
      ws[3] = ws[1] - ws[2];
      if (ws[3] < 0.0)
	ded_sched[13] = 0.0;
      else
       {
	ws[4] = smallerof( ws[3], 50000.0 );
	ws[5] = 0.0001 * Round( 10000.0 * (ws[4] / 50000) );
	ws[6] = 0.25 * ded_sched[1];
	ws[7] = ws[5] * ws[6];
	ded_sched[13] = ws[7];
       }
    }
   else
   if (L[33] <= 525000.0)
    { /* Worksheet 4 pg 39 */
      ws[1] = L[33] - 475000.0;
      ws[2] = 0.0001 * Round( 10000.0 * (ws[1] / 50000) );
      ws[3] = 0.25 * ded_sched[1];
      ws[4] = ws[2] * ws[3];
      ws[5] = ws[3] + ws[4];
      ded_sched[13] = ws[5];  
    }
   else
   if (L[33] <= 1000000.0)
    ded_sched[13] = 0.5 * ded_sched[12];
   else
   if (L[33] <= 10000000.0)
    { /* Worksheet 5 pg 39 */
      ws[1] = L[33];
      ws[2] = 0.5 * ded_sched[4];
      ws[3] = ws[1] - ws[2];
      ded_sched[13] = ws[3];  
    }
   else
    { /* Worksheet 6 pg 39 */
      ws[1] = L[33];
      ws[2] = 0.25 * ded_sched[4];
      ws[3] = ws[1] - ws[2];
      ded_sched[13] = ws[3];  
    }
   } /*L33_morethan_100000*/
 ded_sched[14] = ded_sched[12] - ded_sched[13];
 ded_sched[15] = CollegeDed;
 ded_sched[16] = ded_sched[14] + ded_sched[15];
 itemized_ded = ded_sched[16];
 // GetLine( "Itemized_Deduction", &itemized_ded );     /* Form IT-201-ATT or 0.0 if std ded. */

 switch (status)	/* Determine the Std. Deduction. Pg. 20. */
  {
   case SINGLE: if (Dependent)   std_ded = 3100.0; 
		else 		 std_ded = 8000.0;			/* Not Updated for 2018. */
	break;
   case MARRIED_FILLING_JOINTLY: std_ded = 16050.0; break;
   case MARRIED_FILLING_SEPARAT: std_ded =  8000.0; break;
   case HEAD_OF_HOUSEHOLD: 	 std_ded = 11200.0; break;
   case WIDOW: 			 std_ded = 16050.0; break;
  }

 if (std_ded > itemized_ded) 
  {
   L[34] = std_ded;
   fprintf(outfile,"Check_Std = X\n");
   showline_wmsg(34,"(Mark Std-deduction)");
  }
 else
  {
   L[34] = itemized_ded;
   fprintf(outfile,"Check_Item = X\n");
   showline_wmsg(34,"(Mark Itemized-deduction)");
  }

 L[35] = L[33] - L[34];
 if (L[35] < 0.0) L[35] = 0.0;
 else showline(35);

 get_parameter( infile, 's', word, "L36" );	/* Number of Dependent Exemptions (Pg 76, line e) */
 get_parameters( infile, 'i', &k, "L36" );
 L[36] = 1000.0 * (double)k;
 showline(36);
 if (k > 0)
  fprintf(outfile, "L36_enter %d\n", k ); 

 L[37] = L[35] - L[36];
 if (L[37] < 0.0)
   L[37] = 0.0;
 showline_wmsg(37,"taxable income");
 L[38] = L[37];
 showline(38);

 if (L[33] <= 106950.0)
   L[39] = TaxRateLookup( L[38], status );
 else
   tax_computation_worksheet( status );
 showline(39);
 Report_bracket_info( L[38], L[39], status );

printf("Under development .... exiting.\n");
fprintf(outfile,"Under development .... exiting.\n");




 if (Your1stName)
  fprintf(outfile,"Your1stName: %s\n", Your1stName );
 if (YourInitial)
  fprintf(outfile,"YourInitial: %s\n", YourInitial );
 if (YourLastName)
  fprintf(outfile,"YourLastName: %s\n", YourLastName );
 if (YourSocSec)
  fprintf(outfile,"YourSocSec#: %s\n", YourSocSec );
 if (Spouse1stName)
  fprintf(outfile,"Spouse1stName: %s\n", Spouse1stName );
 if (SpouseInitial)
  fprintf(outfile,"SpouseInitial: %s\n", SpouseInitial );
 if (SpouseLastName)
  fprintf(outfile,"SpouseLastName: %s\n", SpouseLastName );
 if (SpouseSocSec)
  fprintf(outfile,"SpouseSocSec#: %s\n", SpouseSocSec );
 if (MailAddress)
  fprintf(outfile,"Number&Street: %s\n", MailAddress );
 if (AptNumber)
  fprintf(outfile,"Apt#: %s\n", AptNumber );
 if (Town[0] != '\0')
  fprintf(outfile,"Town: %s\n", Town );
 if (StateName[0] != '\0')
  fprintf(outfile,"StateName: %s\n", StateName );
 if (Zipcode[0] != '\0')
  fprintf(outfile,"Zipcode: %s\n", Zipcode );

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

 fclose(infile);
 fclose(outfile);
 printf("\nListing results from file: %s\n\n", outfname);
 Display_File( outfname );
 return 0;
}
