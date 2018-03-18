Open Tax Solver - Package
--------------------------

March 16, 2018 v15.04 - For 2017 tax-year.

See project homepage:	http://opentaxsolver.sourceforge.net/

OpenTaxSolver (OTS) is a set of programs and templates for helping you
fill out your income tax forms.  It performs the tedious arithmetic.
OTS is intended to assist those who normally prepare their tax forms
themselves, and who generally know on which lines to enter their numbers.
It is meant to be used in combination with the instruction booklet 
corresponding to a given form.

This package contains programs and templates for:
	- US-1040 - which also does the Schedules A, B, D, and forms 8949. 
	- Schedule C for US-1040.
	- State Income Taxes for California, Ohio, New Jersey, 
	  Virginia, Pennsylvania, New York, Massachusetts, and North Carolina.
	  All updated for 2017 Tax-Year.
Also contains a new Automatic PDF Form-Fillout function:
	- Supports all Federal Forms and State Forms.
	  Saves time by filling out many of the numbers.  
	  You may still need to enter some information or check boxes
	  that are not handled by OTS.
	  Tested to work properly with many viewers.

-----------------------------------------------------------
--- This package contains executables for RPLCSTRNG_01 ---
-----------------------------------------------------------
  RPLCSTRNG_02

History:
    * v15.04 (3/16/2018) - Further automated California CA540 Adjustments form.
	Provides more complete form fill-out.
	In US-1040, now blanks unneeded zero entries, and properly resets "various"
	date in Cap-Gains form.  Added optional ability to accept AMT Form 6251
	entries for lines 8-27.
    * v15.03 (2/14/2018) - Fixed fed1040 collectible gains used before input,
	missing fill-in of 1st SchedB Int+Divs, comma issue in Sched-B PDF,
	proper medical percentage in Sched-A line 3. Began addressing OH form
	size issues.  More line-label improvements.  Apt + Middle initials in
	PDF forms.
    * v15.02 (2/6/2018) - Added NY, and MA State Tax forms.
	Several fixes & improvements.  Added page-overflow 
	capability for Fed-1040 Schedules B+D. 
    * v15.01 (2/2/2018) - Added VA, and PA State Tax forms.
    * v15.00 (1/15/2018) - Initial Release for Tax Year 2017.

Usage:
 RPLCSTRNG_03
  (Located in the top directory.)
 RPLCSTRNG_04
 For Auto-Fillout of PDF Forms, click the "Print" button, and select
  "Automatically Fill-out PDF Tax-Form", then click "Print".
 You can set your preferred PDF viewer by the (Set PDF Viewer) button,
  or by setting the environment variable:  PDF_VIEWER
 The Auto-Fillout feature is tested to work properly with the PDF
 viewers "Google-Chrome", "Firefox", "LibreOffice", "Atril", "Xpdf", 
 "Safari", and "IE".

General:
Example tax-data files and blank starting templates are included under
the examples_and_templates directory.  For each filer, save filled-out 
template to a new name, such as "fed1040_2017.txt".  After filling-in the
lines, then run the tax solver on it.  From the GUI, this is done by
pressing "Compute Tax" button.
Or solvers can be run from the command-line, for example, as:
  bin/taxsolve_usfed1040_2017  Fed1040_2017.txt
Where "Fed1040_2017.txt" is the name of -your- tax-data file, which
you can edit with your favorite text-editor to fill it in or print
it out.  Output results are saved to "..._out.txt" files
(eg. Fed1040_2017_out.txt), and can be printed out directly too.

For updates and further information, see:
        http://sourceforge.net/projects/opentaxsolver/
Documentation:
        http://opentaxsolver.sourceforge.net/

Re-compiling:
 Unix/Linux/Mac:
  Pre-compiled executables for Unix/Linux are normally in bin directory.
  However to build the binaries in the bin/ directory:
     rm ./bin/* Run_taxsolve_GUI    ( Clears executables. )
     src/Build_taxsolve_packages.sh   ( Creates executables. )

 MS-Windows:
   Pre-compiled executables are normally in bin directory.
   For compiling OTS on MSwindows, MinGW with Msys is recommended.
   From Msys terminal window:
      rm ./bin/* Run_taxsolve_GUI.exe	 ( Clears executables. )
      src/Build_taxsolve_packages.sh   ( Creates executables. )

Directory Structure:
OpenTaxSolver2017       ......................................       25.246-KB
   |-- src   .................................................      217.555-KB
   |   |-- Gui_gtk   .........................................      141.807-KB
   |
   |-- examples_and_templates   ..............................       49.152-KB
   |   |-- VA_760   ..........................................       11.038-KB
   |   |-- US_1040_Sched_C   .................................       12.803-KB
   |   |-- US_1040   .........................................       21.351-KB
   |   |-- PA_40   ...........................................       11.562-KB
   |   |-- OH_1040   .........................................       14.860-KB
   |   |-- NY_IT201   ........................................       14.686-KB
   |   |-- NJ_1040   .........................................       14.124-KB
   |   |-- NC_400   ..........................................       12.251-KB
   |   |-- MA_1   ............................................       13.844-KB
   |   |-- CA_540   ..........................................       13.166-KB
   |
   |-- bin   .................................................      351.033-KB
16 Directories.

---------------------------------------------------------------------------------
Aston Roberts (aston_roberts@yahoo.com)
File Organization and Makefiles by: Krish Krothapalli, David Masterson, & Jesse Becker
Programs contain contributions by many others.  See OTS credits webpage.
