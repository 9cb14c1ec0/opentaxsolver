Open Tax Solver - Package
--------------------------

March 28, 2019 v16.06 - For 2018 tax-year.

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
	  All updated for 2018 Tax-Year.
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
    * v16.06 (3/28/2019) - Fixed new GUI-bug that was introduced in
	last update, which could cause some line semi-colons to be
	misplaced in the save-file.
    * v16.05 (3/21/2019) - Improved GUI display formatting, and
	save-file formatting.  Fixed NJ-State lines 74 + 75, and PDF
	positioning of line 17.  Fixed GUI issue when loading State 
	template(s) which did not display spouse fields when importing 
	joint Federal return.
    * v16.04 (3/15/2019) - Added optional AMT worksheet line inputs to 
	the Fed 1040 template. The AMT worksheet lines 2a through 2g now
	display in the PDF outputs.  The OTS GUI now catches forced kills
	by the window manager when user clicks the "X" in the window tab,
	which now eliminates hanging zombe processes under MS-Windows.
	The "MarkupPDF" construct now supports quoted string values.
	This enables proper PDF display of multi-word answers, as well as
	explicit/literal character sequences without numeric formating.
    * v16.03 (3/8/2019) - Added Sched-B Part-III lines to the US-1040 
	(ticket #63). Added PDF metadata for dependent and occupation
	fill-in tags, so users can add these to their results output
	file (_out.txt) to fill-out the PDF forms (ticket #64).
	Added ability to add additional PDF mark-ups, or change them,
	from your Tax Input Files, by adding "MarkupPDF" lines --
	documented under OTS's "Data Input Format" web page.
	Added ability to comment-out lines of your result files
	(_out.txt) by placing exclamation mark "!" as first character
	on a line.  Commented lines will be ignored by the PDF generator
	(ticket #65).  Fixed NY State importing of Fed Sched-1 data
	(ticket #66), and warning on line 17 (ticket #67).
    * v16.02 (2/28/2019) - To the Federal form, added Alimony
	recipient info, which is then also imported by some state
	forms.   Fixed some PDF positioning on federal form,
        including fields on the AMT form's second page.
	Added option to print all federal forms, even when not
	required, the "-allforms" option.
	From California template, removed CA540 line 5 which is
	imported from federal form, and broke out line 21 a-f.
	Fixed CA540 handling of SocSec payments, and CA540 line 10.
    * v16.01 (2/27/2019) - A fix to Fed 1040 line 55, as well
	as fixes for NJ and CA state programs.
	Added Health-Coverage field to federal template.
	Improved and added more active fields on several PDF forms.
	No longer requires spouse inputs when not filing jointly.
	Added alimony recipient fields for those who need it.
    * v16.00 (2/2/2019) - Initial Release for Tax-Year 2018.
	New this year, the "examples_and_templates" directory
	has been renamed "tax_form_files".  It still contains
	templates and example form-files, but the new name
	will be more intuitive to new users.

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
template to a new name, such as "fed1040_2018.txt".  After filling-in the
lines, then run the tax solver on it.  From the GUI, this is done by
pressing "Compute Tax" button.
Or solvers can be run from the command-line, for example, as:
  bin/taxsolve_usfed1040_2018  Fed1040_2018.txt
Where "Fed1040_2018.txt" is the name of -your- tax-data file, which
you can edit with your favorite text-editor to fill it in or print
it out.  Output results are saved to "..._out.txt" files
(eg. Fed1040_2018_out.txt), and can be printed out directly too.

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
OpenTaxSolver2018       ......................................       25.246-KB
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
