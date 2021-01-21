/************************************************************************/
/* taxsolve_ma_1_2020.c - OpenTaxSolver for Mass Form 1 		*/
/* Copyright (C) 2020 							*/
/* 									*/
/* OTS Project Home Page and Updates:  					*/
/*		http://opentaxsolver.sourceforge.com/			*/
/* 									*/
/* Compile:   cc taxsolve_ma_1_2020.c -o taxsolve_ma_1_2020		*/
/* Run:       ./taxsolve_ma_1_2020  Mass1_2020.txt			*/
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
/* Robert Heller 2-10-2004	heller@deepsoft.com			*/
/* Updated 1-26-05	Aston Roberts & Robert Heller			*/
/*  ...									*/
/* Updated 1-2-2020	Aston Roberts 					*/
/************************************************************************/

#include <stdio.h>
#include <time.h>

float thisversion=18.00;

#include "taxsolve_routines.c"

#define SINGLE 		        1
#define MARRIED_FILLING_JOINTLY 2
#define MARRIED_FILLING_SEPARAT 3
#define HEAD_OF_HOUSEHOLD       4
#define WIDOW		        5
#define Yes 1
#define No  0


double Sum( double *v, int start_slot, int end_slot )
{
 int j;
 double result = 0.0;
 for (j=start_slot; j <= end_slot; j++) result += v[j];
 return result;
}


/*----------------------------------------------------------------------------*/
/* ---				Main					  --- */
/*----------------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{

 return 0;
}
