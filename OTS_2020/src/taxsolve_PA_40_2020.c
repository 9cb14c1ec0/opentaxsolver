/************************************************************************/
/* TaxSolve_PA40_2020.c - Pennsylvania 2020 PA-40 State Tax Form.	*/
/* Copyright (C) 2020, - Aston Roberts					*/
/* 									*/
/* Compile:   gcc taxsolve_PA40_2020.c -o taxsolve_PA40_2020		*/
/* Run:	      ./taxsolve_PA40_2020  PA40_2020.txt 			*/
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
/* Aston Roberts 1-2-2020	aston_roberts@yahoo.com			*/
/************************************************************************/

#define thisversion 18.00

#include "taxsolve_routines.c"

#define SINGLE 		        1
#define MARRIED_FILLING_JOINTLY 2
#define MARRIED_FILLING_SEPARAT 3
#define WIDOW		        1
#define Yes 1
#define No  0



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
 char word[2000], *infname=0, outfname[1500];
 time_t now;
 double oneA, oneB;
 char *Your1stName=0, *YourLastName=0, *Spouse1stName=0, *SpouseLastName, *YourNames;

 return 0;
}
