/************************************************************************************************
  Notify_Popup - Stand alone pop-up notifier utility.
   Accepts a message on command-line, and pops up a small window with the message.
   Useful for alerting users of warnings or errors, especially for non-graphical programs or
   scripts.

   Amazing there is not already a universal command like this.  It should be the same command
   natively available within *all* operating systems and platforms.  But could not be found.
    (Other than "xmessage" in Linux, but is often not installed. Or "osascript" in MacOS, but indirect.)
   So here it is.

   Multi-word messages should be quoted.
   Example:
	notify_popup "Warning: Bad file detected."

   You can enter multiple explicit lines with separate arguments, like:
   Example:
	notify_popup "Please re-run the program." "Because of bad data." " -bye"

   Options:
	-delay n	- Delays popping for n-seconds.
	-expire nn	- Automatically close after nn seconds.

   Planned future features:
    - Command-line options to specify:
	- Message text color, size, & boldness.
		-textcolor r g b,		(RGB values in 0.0 - 1.0 range)
		-textsize  fs			(Default Font size = 10)
		-textbold
		-textnormal
	- Ability to specify line-breaks in message with a symbol.  (In addition to the method above.)
		\n
	- Window name (ex. Note, Warning, Error, etc.)
		-title "Warning"
	- Optional button(s), with command(s) to execute when pressed.
		-button "label" command
	- Ability to specify a file to provide the text.
		-file  notice.txt
	- Scrollable panel when text exceeds window size.

  Compile:
   gcc -O notify_popup.c `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0` -o notify_popup

  Public Domain.
 ***********************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gtk_utils.c"


void OK_Button( GtkWidget *win, void *data )
{
 exit(0);
}


int main( int argc, char *argv[] )
{
 GtkWidget *winpanel;
 int winwidth, winhght, ypos=10, minht=80, minwd=250, nsecs=0;
 int j, k=1, col=0, maxcols=0, nlines=0, num_in=0, cnt=0, expire=0;
 char *text_in[999], *mesg[999], line[4096];
 double endtime=0.0;

 /* Accept command-line arguments. */
 k = 1;
 while (k < argc)
  {
   if (strcmp( argv[k], "-delay" ) == 0)
    {
     k++;
     if (k == argc)
      { printf("Missing value after option '%s'\n", argv[k-1] );  exit(1); }
     if (sscanf(argv[k], "%d", &nsecs ) != 1)
      { printf("Bad integer after option '%s'\n", argv[k-1] );  exit(1); }
    }
   else
   if (strcmp( argv[k], "-expire" ) == 0)
    {
     k++;
     if (k == argc)
      { printf("Missing value after option '%s'\n", argv[k-1] );  exit(1); }
     if (sscanf(argv[k], "%d", &expire ) != 1)
      { printf("Bad integer after option '%s'\n", argv[k-1] );  exit(1); }
     endtime = Report_Time() + (double)expire;
    }
   else
    text_in[num_in++] = strdup( argv[k] );	/* Accept text to display. */
   k++;
  }
 if (num_in == 0)
  {
   printf("Error: No text to display on command-line.\n");
   exit(1);
  }

 /* Scan through the text, breaking it into lines. */
 for (cnt = 0; cnt < num_in; cnt++)
  { /*cnt*/
    col = 0;
    j = 0;
    while (text_in[cnt][j] != '\0')	/* Determine max line width and number of rows. */
     { /*loop*/
      if ((text_in[cnt][j] == '\n') || ((text_in[cnt][j] == ' ') && (col >= 90)) || (col > 180))
       {
        if ((text_in[cnt][j] != '\n') && (text_in[cnt][j] != ' '))
         line[col++] = text_in[cnt][j++];
        line[col] = '\0';
        mesg[nlines++] = strdup( line );
        if (nlines > 900) { printf("Error: Too many lines.\n");  exit(1); }
        if (col > maxcols)
         maxcols = col;
        col = 0;
       }
      else
       line[col++] = text_in[cnt][j];
      j++;
      if (j >= 1024) { printf("Error: line too long.\n");  exit(1); }
     } /*loop*/
    if (col > 0)
     {
      line[col] = '\0';
      mesg[nlines++] = strdup( line );
      if (col > maxcols)
       maxcols = col;
     }
  } /*cnt*/

 /* Determine the needed window size. */
 winwidth = 30 + maxcols * 7;
 winhght = 65 + 18 * nlines;
 if (winwidth < minwd)
  winwidth = minwd;
 if (winhght < minht)
  winhght = minht;

 if (nsecs > 0)
  Sleep_seconds( (float)nsecs );

 /* Initialize and generate top-outer window. */
 winpanel = init_top_outer_window( &argc, &argv, winwidth, winhght, "Notice", 0, 0 );

 for (j=0; j < nlines; j++)
  {
   make_label( winpanel, 20, ypos, mesg[j] );
   ypos = ypos + 18;
  }
 make_button( winpanel, winwidth/2 - 30, winhght - 38, "  OK  ", OK_Button, 0 );

 /* When the window is given the 'forced delete_event' signal by the window manager, exit the program. */
 gtk_signal_connect( GTK_OBJECT(outer_window), "delete_event", GTK_SIGNAL_FUNC(exit), NULL );

 gtk_widget_show_all( outer_window );
 while (1)       // gtk_main();
  {
   UpdateCheck();               /* Check for, and serve, any pending GTK window/interaction events. */
   Sleep_seconds( 0.1 );       /* No need to spin faster than ~10 Hz update rate. */
   if (expire > 0)
    {
     if (Report_Time() > endtime + (double)nsecs)
      exit(0);
    }
  }
 return 0;
}

