Open Tax Solver - Package
--------------------------

February 19, 2021 v18.03 - For 2020 tax-year.

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
	- State Income Taxes for Ohio, New Jersey, 
	  Virginia, Pennsylvania, Massachusetts, and North Carolina.
	  A beta-version version of the updated California taxes was added.
	  All updated for 2020 Tax-Year.
Also contains an Automatic PDF Form-Fillout function:
	- Supports all Federal Forms and State Forms.
	  Saves time by filling out many of the numbers.  
	  You may still need to enter some information or check boxes
	  that are not handled by OTS.
	  Tested to work properly with many viewers.
	- You can edit your forms with Libre-Office.

-----------------------------------------------------------
--- This package contains executables for RPLCSTRNG_01 ---
-----------------------------------------------------------
  RPLCSTRNG_02

History:
    * 18.03 (2/19/2021) - Added option to calculate taxes in whole dollars.
		 (Select under the GUI's 'Options' button,
		   or use run-time option '-round_to_whole_dollars'.)
		Added updated NY State taxes.
		Fixed California CA540 lines A5 through A7, SocSec payments.
		Now overly long comments are truncated on form 8849 PDF output.
		Tax-bracket information reporting was fixed on Fed-1040.
    * 18.02 (2/11/2021) - Additional validation was added to test that numeric 
		entries are valid numbers for all forms.
		For the Fed 1040:
		  1. The Social Security worksheet lines 3+6 calculations were fixed.
		  2. The circular dependency introduced in this year's taxes on Schedule-A
		     and 1040 Lines 10b, 10c, and 11 -- which affects the itemization 
		     decision and amounts -- was addressed and cleaned up.
		  3. A check-box option was added on Fed 1040 Schedule-A (line-18) to
		     elect to itemize, even when less than standard deduction.
		Fixed comment on Massachusetts template line 15.
		A beta-version version of the updated California taxes was added.
    * v18.01 (2/5/2021) - On Fed 1040, fixed line 10c sign, and Updates to the
		new separated Town/State/zip entries.
    * v18.00 (1/25/2021) - Initial Release for Tax-Year 2020.
		CA and NY state taxes are not included in this version because
		the states have not yet released their form instructions
		information.  Please check back for updates.

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
 "Safari", "IE", "Edge", and "Acrobat Reader".
 You can edit your filled-out PDF forms with LibreOffice.

General:
Example tax-data files and blank starting templates are included under
the "tax_form_files" directory.  For each filer, save filled-out 
template to a new name, such as "fed1040_2020.txt".  After filling-in the
lines, then run the tax solver on it.  From the GUI, this is done by
pressing "Compute Tax" button.
Or solvers can be run from the command-line, for example, as:
  bin/taxsolve_usfed1040_2020  Fed1040_2020.txt
Where "Fed1040_2020.txt" is the name of -your- tax-data file, which
you can edit with your favorite text-editor to fill it in or print
it out.  Output results are saved to "..._out.txt" files
(eg. Fed1040_2020_out.txt), and can be printed out directly too.

For updates and further information, see:
        http://sourceforge.net/projects/opentaxsolver/
Documentation:
        http://opentaxsolver.sourceforge.net/

Re-compiling:
 Unix/Linux/Mac:
  Pre-compiled executables for Unix/Linux are normally in bin directory.
  However to build the binaries in the bin/ directory:
     cd OpenTaxSolver2020_18.xx	      ( cd into top directory. )
     rm ./bin/* Run_taxsolve_GUI      ( Clears executables. )
     src/Build_taxsolve_packages.sh   ( Creates executables. )

 MS-Windows:
   Pre-compiled executables are normally in bin directory.
   For compiling OTS on MSwindows, MinGW with Msys is recommended.
   From Msys terminal window:
      cd OpenTaxSolver2020_18.xx       ( cd into top directory. )
      rm ./bin/* Run_taxsolve_GUI.exe  ( Clears executables. )
      src/Build_taxsolve_packages.sh   ( Creates executables. )

Directory Structure:
OpenTaxSolver2020       ......................................       25.246-KB
   |-- src   .................................................      217.555-KB
   |   |-- Gui_gtk   .........................................      141.807-KB
   |
   |-- tax_form_files   ......................................       49.152-KB
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
