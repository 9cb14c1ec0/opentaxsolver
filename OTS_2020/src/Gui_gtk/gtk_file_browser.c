/* ------------------------ Begin File Browser --------------------------*/
/* (The Default Gtk file browser widget was confusing for users, and did not 
    offer the file filtering and date-display flexibility we need.  So we built
    a better one here, which does exactly what we need it to.)
*/

#define FB_DEBUG if (0)
#define MXLEN 8192
#define SHOWFILESIZE 0

#if (SHOWFILESIZE == 1)
 #define FB_COLUMNS 4
#else
 #define FB_COLUMNS 3
#endif

GtkWidget *fbwindow=0;
GtkEntry  *fb_filename_formbox, *fb_wildcard_formbox;
int	  fb_maxlen, fb_dirsonly, fb_allowfiles=1, fb_showdotfiles=0, fb_selection_count;
char	  fb_filename[MXLEN], fb_dirname[MXLEN], fb_wildcard[MXLEN], fb_wildcard_default[MXLEN]="", *fb_prompt;
char	  *fb_fnptr, *fb_dnptr, *fb_wcptr, fb_Selected[2][MXLEN]={"",""};
void (*fb_callback)(char *fname);           /* Callback function for filebrowser. */

struct fb_word_list_item
 {
  char *word;
  struct fb_word_list_item *nxt;
 } *fb_altdirs=0, *fb_banned=0;	/* List of alternate directories or files to suggest. */


void fb_clear_word_list( struct fb_word_list_item **list )
{ struct fb_word_list_item *item;
  while (*list != 0)
   {
    item = *list;
    *list = item->nxt;
    free( item->word );
    free( item );
   }
}


char *strdup_padded( char *instr )
{
 int j;
 char *outstr;
 j = strlen( instr );
 outstr = (char *)malloc( j + 64 );
 strcpy( outstr, instr );
 return outstr;
}


void fb_add_word_to_end_of_list( struct fb_word_list_item **list, char *word )
{
 struct fb_word_list_item *newitem, *lastitem;
 newitem = (struct fb_word_list_item *)malloc( sizeof(struct fb_word_list_item) );
 newitem->word = strdup(word);
 newitem->nxt = 0;
 if (*list == 0)
  *list = newitem;
 else
  {
   lastitem = *list;
   while (lastitem->nxt != 0) lastitem = lastitem->nxt;
   lastitem->nxt = newitem;
  }
}


void fb_add_optional_dirfile( char *dir_or_file )   /* Provides method to suggest alternate directory(s) via pull-down. */
{
 struct fb_word_list_item *item;
 item = fb_altdirs;	/* First make sure directory is not already on the list. */
 while ((item != 0) && (strcmp( item->word, dir_or_file ) != 0))
  item = item->nxt;
 if (item == 0)
  fb_add_word_to_end_of_list( &fb_altdirs, dir_or_file );
}

void fb_clear_optional_dirfiles()                   /* Clears all previous list entries. */
{ fb_clear_word_list( &fb_altdirs ); }

void fb_clear_last_selected() { fb_Selected[0][0] = '\0'; }

void cancel_fbwindow( GtkWidget *widget, gpointer data )
{
 if (fbwindow != 0) gtk_widget_destroy( fbwindow );
 fbwindow = 0;
 fb_Selected[0][0] = '\0';
}


void fb_ban_files(char *banstr )
{
  fb_add_word_to_end_of_list( &fb_banned, banstr );
}


void fb_clear_banned_files()
{
 struct fb_word_list_item *olditem;
 while (fb_banned)
  {
   olditem = fb_banned;
   fb_banned = fb_banned->nxt;
   free( olditem->word );
   free( olditem );
  }
}


int not_banned( char *name )
{ /* Returns 1 if filename is not on the banned list, else returns 0. */
 struct fb_word_list_item *item;
 item = fb_banned;
 while (item && (strstr( name, item->word ) == 0))
  item = item->nxt;
 if (item)
  return 0;
 else
  return 1;
}


void BrowseFiles0( char *prompt, int maxlength, char *directory, char *wildcards, char *filename, void (*callback)(char *fname) ); /* Prototype. */

void fb_wildcard_accept( GtkWidget *widget, gpointer data )
{
 get_formbox_text( fb_wildcard_formbox, fb_wildcard, 500 );
 FB_DEBUG printf("accpted wildcard '%s'\n", fb_wildcard);
 BrowseFiles0( fb_prompt, fb_maxlen, fb_dirname, fb_wildcard, fb_filename, fb_callback );
}


void fb_wildcard_clear( GtkWidget *widget, gpointer data )
{
 fb_wildcard[0] = '\0';
 modify_formbox( fb_wildcard_formbox, fb_wildcard );
 FB_DEBUG printf("cleared wildcard '%s'\n", fb_wildcard );
 BrowseFiles0( fb_prompt, fb_maxlen, fb_dirname, fb_wildcard, fb_filename, fb_callback );
}


void fb_wildcard_restore( GtkWidget *widget, gpointer data )
{
 strcpy( fb_wildcard, fb_wildcard_default );
 modify_formbox( fb_wildcard_formbox, fb_wildcard );
 FB_DEBUG printf("cleared wildcard '%s'\n", fb_wildcard );
 BrowseFiles0( fb_prompt, fb_maxlen, fb_dirname, fb_wildcard, fb_filename, fb_callback );
}



void fb_next_word( char *line, char *word, char *delim )
{
 int i=0, j=0, m=0, nodelim=1;
 while ((line[i]!='\0') && (nodelim))  /* Consume any preceding white-space. */
  {
   j = 0;
   while ((delim[j]!='\0') && (line[i]!=delim[j])) j = j + 1;
   if (line[i]==delim[j]) { i = i + 1; } else  nodelim = 0;
  }
 while ((line[i]!='\0') && (!nodelim)) /* Copy the word until the next delimiter. */
  {
   word[m++] = line[i++];
   if (line[i]!='\0')
    {
     j = 0;
     while ((delim[j]!='\0') && (line[i]!=delim[j])) j = j + 1;
     if (line[i]==delim[j]) nodelim = 1;
    }
  }
 j = 0;	 /* Shorten line. */
 while (line[i]!='\0') { line[j++] = line[i++]; }
 /* Terminate the char-strings. */
 line[j] = '\0';
 word[m] = '\0';
}


#ifndef __MINGW32__
 #define FB_DIRSEP_CHR '/'	/* POSIX File directory delimiter. */
 #define FB_DIRSEP_STR "/"
 #define FB_RTN_CHR '\n'	/* Character returned by carriage-return key. */
#else
 #define FB_DIRSEP_CHR '\\'	/* MS-win File directory delimiter. */
 #define FB_DIRSEP_STR "\\"
 #define FB_RTN_CHR '\n'	/* Character returned by carriage-return key. */
#endif

 
void fb_shorten_string_at_front( char *string, int n )
{ int k=0;
  do { string[k] = string[k+n];  k++; }
  while (string[k-1] != '\0');
}

void fb_eliminate_leading_dot( char *fname )	/* Remove leading dot-slash (./), not needed. */
{
 while ((fname[0] == '.') && (fname[1] == FB_DIRSEP_CHR))
  fb_shorten_string_at_front( &(fname[0]), 2 );
}


#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>

void fb_unify_slashes( char *name, char oldslash, char newslash )
{
 int j=0;
 while (name[j] != '\0')
  { if (name[j] == oldslash) name[j] = newslash;  j++; }
}


void fb_reduce_pathname( char *fname )
{
 int j, k=0, nsegs=0;
 char *twrd, *dseg[400];

 if (fname[0] == '\0') return;
 FB_DEBUG printf("REDUCING '%s'\n", fname);
 fb_unify_slashes( fname, '\\', '/' );	/* Unify to a single type of slash (dir-separator) for sanity. */

 if (strcmp(fname,".")==0) fname[0] = '\0';	/* Do not need dot (.) explicitly for current directory. */

 while ( strstr(fname, "//") )			/* Remove any double-slashes. */
  { fb_shorten_string_at_front( strstr(fname, "//"), 1 ); }

 while ((fname[0] == '.') && (fname[1] == '/'))
  fb_shorten_string_at_front( &(fname[0]), 2 );

 k = strlen( fname ) - 1;	/* Check for and remove any trailing slash. */
 #ifdef __MINGW32__
  if ((k == 1) && (fname[k] == ':')) { strcat(fname, "\\");   return; }
  if ((k == 2) && (fname[k-1] == ':') && (fname[k] == '/'))
   {  fname[k] = '\\';  return; }
 #endif
 if ((k > 0) && (fname[k] == '/')) fname[k] = '\0';
 if (fname[0] == '\0') return;

 /* Separate path into segments. */
 twrd = (char *)malloc( MXLEN );
 k = 0;
 while ((fname[k] != '\0') && (nsegs < 400))
  {
   j = 0;
   while ((fname[k] != '\0') && (fname[k] != '/') && (j < MXLEN-1))
    twrd[j++] = fname[k++];
   if ((nsegs == 0) && (j == 0) && (fname[k] == '/')) twrd[j++] = fname[k];
   twrd[j] = '\0';
   dseg[nsegs++] = strdup( twrd );
   if (fname[k] == '/') k++;
  }
 free( twrd );

 /* Reconstruct path name from the segments, but skip any 'aa/..' pairs, where 'aa' is not '..'. */
 k = 0;
 strcpy( fname, dseg[k++] );
 if ((fname[0] != '\0') && (fname[1] == ':') && (fname[2] == '\0')) strcat( fname, "/" );
 while (k < nsegs)
  {
   if ((k < nsegs - 1) && ((strcmp( dseg[k+1], "..") == 0) && (strcmp( dseg[k], "..") != 0)))
    k = k + 2;
   else
    {
     j = strlen(fname) - 1;
     if ((j >= 0) && (fname[j] != '/')) strcat( fname, "/" );
     strcat( fname, dseg[k++] );
    }
  }
 twrd = &(fname[0]);
 #ifdef __MINGW32__
  if ((fname[0] != '\0') && (fname[1] == ':')) twrd = &(fname[2]);
 #endif
 if (strcmp( twrd, "/.." ) == 0) strcpy( twrd, "/");

 #ifdef __MINGW32__
  fb_unify_slashes( fname, '/', '\\' );		/* Make slashes be appropriate to platform. */
  if ((fname[0] != '\0') && (fname[1] == ':') && (fname[2] == '\0'))
   strcat( fname, "\\" );
 #endif
}


void fb_EnvironVarFilter( char *fname )
{ /* Intercept and replace any defined environment variables in file-names. */
 int k=0, j, m, n, p, seppt=1; 
 char twrd[2048], *evar;

 if (strchr(fname,'$') == 0) return;
 while ((k < 2048-1) && (fname[k] != '\0'))
  { /*lookfor$*/
    if ((seppt) && (fname[k] == '$'))
     { /*replace_envvar*/
      k++;	/* Grab just the env-var name. */
      j = 0;  m = k;
      while ((m < 2048-1) && (fname[m] != '/') && (fname[m] != '\\') && (fname[m] != '\0'))
	{ twrd[j++] = fname[m++]; }
      twrd[j] = '\0';
      evar = getenv(twrd);
      if (evar)
       {
	k--;
	n = strlen(evar);
	p = n - m + k;
	if (p > 0)
	 { /*expand string*/
	   n = strlen(fname);
	   while (n >= m) { fname[n+p] = fname[n];  n--; }
	 }
	else
	if (p < 0)
	 { /*shrink string.*/
	   n = m + p;
	   do { fname[n] = fname[n-p];  n++; } while (fname[n-1] != '\0');
	 }
	p = 0;  /* Copy evar into position. */
	while (evar[p] != '\0') fname[k++] = evar[p++];
       }
      else 
       k = m;
     } /*replace_envvar*/
    else
     {
      if ((fname[k] == '/') || (fname[k] == '\\')) seppt = 1; else seppt = 0;
      k++;
     }
  } /*lookfor$*/
}


void fb_accept( GtkWidget *widget, gpointer data ) /* Determine if selected file is a directory or regular file. */
{		/* If it is a directory, then open browser to that directory, otherwise return file's-name. */
 char *pathname;
 struct stat buf;
 int k, erno;

 get_formbox_text( fb_filename_formbox, fb_filename, MXLEN );
 FB_DEBUG printf("accepted file '%s/%s'\n", fb_dirname, fb_filename);

 if (strcmp(fb_filename,".") == 0) { fb_dirname[0] = '\0'; }
 fb_EnvironVarFilter( fb_filename );
 fb_reduce_pathname( fb_filename );

 if ((fb_filename[0] == FB_DIRSEP_CHR) || ((fb_filename[0] != '\0') && (fb_filename[1] == ':')))
  fb_dirname[0] = '\0';	/* If filename contains full absolute path, then erase old path from dirname. */
 else				/* Otherwise, a relative path was given. */
  fb_eliminate_leading_dot( fb_filename );

 /* Construct the new full-path name of the file or directory that was selected. */
 pathname = (char *)malloc(MXLEN);
 strcpy_safe( pathname, fb_dirname, MXLEN);
 k = strlen(pathname) - 1;  /* Remove the trailing slash from the directory name, if any. */
 if ((k > 0) && (pathname[k] == FB_DIRSEP_CHR)) pathname[k] = '\0';
 if (pathname[0] != '\0') strcat(pathname, FB_DIRSEP_STR);  /* Add trailing slash to directory name. */
 strcat( pathname, fb_filename );	/* Append the file-name portion to create full path name. */
 fb_eliminate_leading_dot( pathname );
 fb_reduce_pathname( pathname );
 FB_DEBUG printf("Returning0 filename '%s'\n", pathname );

 erno = stat( pathname, &buf );
 FB_DEBUG printf(" Erno = %d, Regular? = %d\n", erno, S_ISREG(buf.st_mode) );
 if ((erno == 0) && (S_ISDIR(buf.st_mode)) && ((fb_dirsonly == 0) || (data != 0)))
  { /*Traverse directories*/
    // printf("Traversing directory\n");
    FB_DEBUG printf("Traversing directory\n");
    cancel_fbwindow(0,0);	/* Close the fbrowser window. */
    strcpy_safe(fb_dirname, pathname, fb_maxlen);
    strcpy_safe(fb_filename,"", fb_maxlen);
    BrowseFiles0( fb_prompt, fb_maxlen, fb_dirname, fb_wildcard, fb_filename, fb_callback );
  }
 else
  { /*Regular_file*/
   if (erno != 0) printf("File does not exist: '%s'\n", pathname );
   FB_DEBUG printf("Returning1 filename '%s'\n", pathname );
   if ((erno == 0) && (fb_dirsonly) && (!S_ISDIR(buf.st_mode)) && (! fb_allowfiles)) 
    { free(pathname);  return; }
   FB_DEBUG printf("Returning2 filename '%s'\n", pathname );
   cancel_fbwindow(0,0);	/* Close the fbrowser window. */
   FB_DEBUG printf("Returning3 filename '%s'\n", pathname );
   /* Separate directory path-name and 'potential' file-name, into appropriate parts. */
   k = strlen( pathname ) - 1;	/* Find last slash, if any. */
   while ((k >= 0) && (pathname[k] != FB_DIRSEP_CHR)) k--;
   if (k < 1)	/* If no slash(es) after leading position. */
    { fb_dirname[0] = '\0';  strcpy( fb_filename, pathname ); }  /* Place all in filename. */
   else
    { strcpy( fb_dirname, pathname );  fb_dirname[k] = '\0';  strcpy( fb_filename, &(pathname[k+1]) ); }
   // printf("fb_dirname='%s', fb_filename='%s', pathname='%s'\n", fb_dirname, fb_filename, fb_filename );
   strcpy_safe( fb_fnptr, fb_filename, fb_maxlen );
   strcpy_safe( fb_dnptr, fb_dirname,  fb_maxlen );
   strcpy_safe( fb_wcptr, fb_wildcard, fb_maxlen );
   FB_DEBUG printf("Returning filename '%s'\n", pathname );
   // printf("Returning filename '%s'\n", pathname );

   fb_callback( pathname );
   free(pathname);
  }
}



void fb_select( GtkWidget *wdg, void *data )
{ char *fname;
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (wdg == 0)
   {
    fname = (char *)data;
    modify_formbox( fb_filename_formbox, fname );
   }
  else
   {
    if (gtk_tree_selection_get_selected( GTK_TREE_SELECTION(wdg), &model, &iter ))
     {
      fb_selection_count++;
      gtk_tree_model_get( model, &iter, FB_COLUMNS - 1, &fname, -1 );
      FB_DEBUG printf("selected file item '%s'\n", fname );
      if (fname[0] == '\0') return;
      modify_formbox( fb_filename_formbox, fname );
     }
   }
}


void fb_select_and_go( GtkWidget *widget, gpointer fname )
{
 char *name=(char *)fname;
 modify_formbox( fb_filename_formbox, name );
 strcpy( fb_Selected[1], fb_Selected[0] );
 strcpy( fb_Selected[0], name );
 fb_dirname[0] = '\0'; 
 fb_select( 0, fname );
 fb_accept( 0, "x" );	  /* The "x" give non-null argument allowing immediate fb-redraw. */
}


void fb_truncate_fname( char *truncname, char *origfname, int n )	/* Limit file names to no longer than n characters. */
{
 if (strlen(origfname) > n)
  {
    strcpy_safe( truncname, origfname, n - 2 );		/* Truncate and add ".." indication that is was truncated. */
    strcat(truncname, ".." );
  }
 else strcpy( truncname, origfname );
}



struct fb_directory_item
 {
  char *filename;
  time_t file_date;
  int sz;
  struct fb_directory_item *nxt;
 };

void fb_format_minutes( int value, char *buf )
{
 if (value < 10)
  { char buf2[30];
    sprintf(buf2,"%d", value);
    strcpy(buf,"0"); strcat(buf,buf2);
  } else sprintf(buf,"%2d", value);
}

int fb_current_day=0;


struct fb_directory_item *fb_new_dirlist_item( char *fname, int sz, time_t filedate )
{
  struct fb_directory_item *newitem;

  newitem = (struct fb_directory_item *)malloc( sizeof(struct fb_directory_item) );
  newitem->filename = strdup_padded( fname );
  newitem->sz = sz;
  newitem->file_date = filedate;
  return newitem;
}


int fb_wildcard_match( char *fname, char *wildcards[] )
{
 int j=0; 
 while ((wildcards[j] != 0) && (strstr(fname, wildcards[j]) == 0)) j++;
 if (wildcards[j] != 0) return 1; else return 0;
}



void spread2rightjustify( char *wrd )
{
 int j=0, k=0, stop=0;
 char twrd[100];
 strcpy( twrd, wrd );
 while (!stop)
  {
   if (twrd[j] == ' ') { wrd[k++] = ' ';  wrd[k++] = ' '; }
   else
    {
     wrd[k++] = twrd[j];
     if (twrd[j] == '\0') stop = 1;
    }
   j++;
  }
}



GtkWidget *fbrowser_frame;
int fbwinwidth, fbwinheight;
char *fb_prior_directory=0;



void renderBrowseFiles0( char *prompt, int maxlength, char *directory, char *wildcards, char *filename, void (*callback)(char *fname) )
{
 int frmbx_wdth_pix, j=0;
 GtkWidget *pulldown, *btn;
 GtkTreeStore *mylist;
 int ypos=20, erno;
 float posx1;
 char *line, *word, *pathname, *wildcard_array[200], *twrd1, *dir_row[4];
 #if (SHOWFILESIZE == 1)
   const char *headings[4]={ " ", "    Size    ", "      Date", "       File Name      " };
 #else
   const char *headings[4]={ " ", "       Date       ", "       File Name      " };
 #endif

 line = (char *)malloc(MXLEN);
 word = (char *)malloc(MXLEN);
 twrd1 = (char *)malloc(MXLEN);
 pathname = (char *)malloc(MXLEN);
 fb_reduce_pathname( directory );
 if (directory[0] == '\0') strcpy( directory, "." );

 sprintf(line,"Directory:  %s", directory );
 make_label( fbrowser_frame, 5, 5, line );

 strcpy_safe( fb_dirname, directory, MXLEN );

 /* Filter any '*' from wildcards, and parse into an array. */
 strcpy_safe( line, wildcards, 500 );
 fb_next_word( line, word, " \t*" );
 while ((word[0] != '\0') && (j < 18))
  {
     wildcard_array[j++] = strdup( word );
     fb_next_word( line, word, " \t*" );
  }
 wildcard_array[j] = 0;	/* Terminate wild-card list. */

 if (fb_current_day == 0)
  { /*Get current date.*/
    const struct tm *tm;
    #ifdef __MINGW32__
     time_t T;
     T = time(0);
     tm = localtime( &T );
    #else
     struct timeval tp;
     gettimeofday(&tp,0);
     tm = localtime( &(tp.tv_sec) );
    #endif
   /* Compute days since 1900, assuming all months have 31 days. */
   fb_current_day = tm->tm_year * (12 * 31) + tm->tm_mon * 31 + tm->tm_mday;
  }

 fb_selection_count = 0;
 mylist = make_multicolumn_selection_list( fbrowser_frame, 5, 26, fbwinwidth - 10, fbwinheight - 145, 
				FB_COLUMNS, headings, fb_select, fb_accept, 0 );
 for (j=0; j < FB_COLUMNS; j++) dir_row[j] = (char *)calloc( 1, 200 );

 { /*listdirectory*/
      DIR *dirpt;
      struct stat buf;
      struct dirent *dir_entry;
      struct tm *time_struct;
      char month_name[13][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","BAD"};
      int month_value, day, n_entries=0, column;
      struct fb_directory_item *dir_sublist_hd, *dir_filelist_hd, *newitem, *flstptr, *olditem, *previtem;
      char minfrmt[30], yeartime[50];

      // printf("Listing dir: %s\n", directory );
      dir_sublist_hd = 0;
      dir_filelist_hd = 0;
      dirpt = opendir( directory );
      if (dirpt == 0) {printf("Error: Could not open directory '%s'.\n", directory );  /* exit(1); */ }
      else
       { /*ok*/
         dir_entry = readdir(dirpt);
	 if (dir_entry == 0)
	  {
	    strcpy_safe( directory, fb_prior_directory, MXLEN );
	    dirpt = opendir( directory );
	    dir_entry = readdir(dirpt);
	  }
         else
	  {
	    if (fb_prior_directory) free(fb_prior_directory);
	    fb_prior_directory = strdup_padded(directory);
	  }
         while (dir_entry != 0)
          { /*direntry*/
	    if ((fb_showdotfiles) || ((dir_entry->d_name[0] != '.') || (strcmp(dir_entry->d_name,"..") == 0)))
            if (strcmp(dir_entry->d_name,".") != 0)
             { /*fileorsubdir*/
	       strcpy_safe(pathname, directory, MXLEN-100);  strcat(pathname,"/");
	       strcat( pathname, dir_entry->d_name );
	       erno = stat( pathname, &buf );
	       if (erno)
		printf("Bad file name %s\n", pathname );
	       else
               if (S_ISDIR(buf.st_mode))	/* If subdirectory, then */
 	        {				/*  Enqueue onto subdirectory list. */
		 newitem = fb_new_dirlist_item( dir_entry->d_name, buf.st_size, buf.st_mtime );
		 previtem = 0;			/* Insert in alphabetic sorted order. */
		 flstptr = dir_sublist_hd;
		 while ((flstptr != 0) && (strcmp(flstptr->filename, dir_entry->d_name) < 0 ))
		  { previtem = flstptr;  flstptr = flstptr->nxt; }
		 if (previtem == 0) { dir_sublist_hd = newitem; }
		 else { previtem->nxt = newitem; }
		 newitem->nxt = flstptr;
                }
               else
	       if (not_banned(dir_entry->d_name) &&  ((wildcard_array[0] == 0) || (fb_wildcard_match( dir_entry->d_name, wildcard_array ))))
 	        {				/*  Otherwise, Enqueue on regular files list. */
		 newitem = fb_new_dirlist_item( dir_entry->d_name, buf.st_size, buf.st_mtime );
		 previtem = 0;			/* Insert in alphabetic sorted order. */
		 flstptr = dir_filelist_hd;
		 while ((flstptr != 0) && (strcmp(flstptr->filename, dir_entry->d_name) < 0 ))
		  { previtem = flstptr;  flstptr = flstptr->nxt; }
		 if (previtem == 0) { dir_filelist_hd = newitem; }
		 else { previtem->nxt = newitem; }
		 newitem->nxt = flstptr;
                }

             } /*fileorsubdir*/
            dir_entry = readdir(dirpt);
          } /*direntry*/
         closedir( dirpt );
       } /*ok*/

      /* First add an empty item at top of list, because it is always pre-selected. */
      for (j=0; j < FB_COLUMNS; j++)  dir_row[j][0] = '\0';
      add_multicolumn_selection_item( mylist, FB_COLUMNS, dir_row );

      /* First list the directories at this level. */
      flstptr = dir_sublist_hd;
      while (flstptr != 0)
       {
	// fb_truncate_fname( word, flstptr->filename, 46 );	/* Limit file names to no longer than 45 characters. */
	/* If file is older than six months, list year, else list time of day. */
	time_struct = localtime( &(flstptr->file_date) );
	day = time_struct->tm_year * (12 * 31) + time_struct->tm_mon * 31 + time_struct->tm_mday;
	if (fb_current_day - day > 6 * 31)
	 sprintf(yeartime,"%d", time_struct->tm_year + 1900 );
	else
	 {
	  fb_format_minutes( time_struct->tm_min, minfrmt );
	  sprintf(yeartime,"%2d:%s", time_struct->tm_hour, minfrmt );
	 }
	month_value = time_struct->tm_mon;
	if ((month_value < 0) || (month_value > 11)) { printf("ERROR: BAD MONTH %d for file %s\n", month_value, pathname );  month_value = 12; }
	column = 0;
	strcpy( dir_row[column++],"d");
	#if (SHOWFILESIZE == 1)
	 // sprintf( dir_row[1], "%7d", (int)((double)(flstptr->sz) / 1000.0 + 0.5) );
	 sprintf( dir_row[column], "%7d", flstptr->sz );
	 spread2rightjustify( dir_row[column++] );
	#endif
	strcpy( dir_row[column], month_name[month_value] );
	sprintf(twrd1," %2d ", time_struct->tm_mday );
	strcat( dir_row[column], twrd1 );
        sprintf(twrd1,"%5s", yeartime );
	strcat( dir_row[column++], twrd1 );
	free( dir_row[column] );
	dir_row[column] = strdup( flstptr->filename );
        // sprintf(line, "d %8d  %s %2d %5s  %s", flstptr->sz, month_name[month_value], time_struct->tm_mday, yeartime, flstptr->filename );
	add_multicolumn_selection_item( mylist, FB_COLUMNS, dir_row );
	n_entries++;
	olditem = flstptr;
        flstptr = flstptr->nxt;		/* Free file entries as we list them. */
	free( olditem );
       }

      /* Second list the regular files at this level. */
      flstptr = dir_filelist_hd;
      while (flstptr != 0)
       {
	// fb_truncate_fname( word, flstptr->filename, 46.0 );	/* Limit file names to no longer than 45 characters. */
	/* If file is older than six months, list year, else list time of day. */
	time_struct = localtime( &(flstptr->file_date) );
	day = time_struct->tm_year * (12 * 31) + time_struct->tm_mon * 31 + time_struct->tm_mday;
	if (fb_current_day - day > 6 * 31)
	 sprintf(yeartime,"%d", time_struct->tm_year + 1900 );
	else
	 {
	  fb_format_minutes( time_struct->tm_min, minfrmt );
	  sprintf(yeartime,"%2d:%s", time_struct->tm_hour, minfrmt );
	 }
	month_value = time_struct->tm_mon;
	if ((month_value < 0) || (month_value > 11)) { printf("ERROR: BAD MONTH %d for file %s\n", month_value, pathname );  month_value = 12; }
	if ((!fb_dirsonly) || (fb_allowfiles))
	 { 
	  column = 0;
	  strcpy( dir_row[column++]," ");
	  #if (SHOWFILESIZE == 1)
	   // sprintf( dir_row[column], "%7d", (int)((double)(flstptr->sz) / 1000.0 + 0.5) );
	   sprintf( dir_row[column], "%7d", flstptr->sz );
	   spread2rightjustify( dir_row[column++] );
	  #endif
	  strcpy( dir_row[column], month_name[month_value] );
	  sprintf(twrd1," %2d ", time_struct->tm_mday );
	  strcat( dir_row[column], twrd1 );
          sprintf(twrd1,"%5s", yeartime );
	  strcat( dir_row[column++], twrd1 );
	  free( dir_row[column] );
	  dir_row[column] = strdup( flstptr->filename );
          add_multicolumn_selection_item( mylist, FB_COLUMNS, dir_row ); 
	  n_entries++;
	 }
	olditem = flstptr;
        flstptr = flstptr->nxt;		/* Free file entries as we list them. */
	free( olditem );
       }
 } /*listdirectory*/

 /* Cleanup any temporary wildcards. */
 j = 0;
 while (wildcard_array[j] != 0) { free(wildcard_array[j]);  j++; }

 ypos = fbwinheight - 95;
 make_label( fbrowser_frame, 2, ypos, prompt );
 ypos = ypos + 20;
 if (fb_altdirs)	/* If alternate-directories list is populated, then place selector button. */
  { struct fb_word_list_item *altdir=fb_altdirs;
   pulldown = make_menu_button( fbrowser_frame, fbwinwidth - 30, ypos, "v" );
   add_tool_tip( most_recent_menu, "Jump to alternate or recent directories." );
   while (altdir != 0)
    {
     add_menu_item( pulldown, altdir->word, fb_select_and_go, altdir->word );
     altdir = altdir->nxt;
    }
   frmbx_wdth_pix = fbwinwidth - 30 - 25;
  }
 else 
   frmbx_wdth_pix = fbwinwidth - 30;
 fb_filename_formbox = make_formbox_bypix( fbrowser_frame, 25, ypos, frmbx_wdth_pix, filename, maxlength, fb_accept, 0 );

 make_button( fbrowser_frame, 10, fbwinheight - 38, "    OK    ", fb_accept, 0 );

 posx1 = 0.38;
 make_label( fbrowser_frame, posx1 * fbwinwidth - 72, fbwinheight - 36, "File Types:" );
 frmbx_wdth_pix = 100;
 fb_wildcard_formbox = make_formbox_bypix( fbrowser_frame, posx1 * fbwinwidth, fbwinheight - 39, frmbx_wdth_pix, wildcards, 500, fb_wildcard_accept, 0 );
 btn = make_button( fbrowser_frame, posx1 * fbwinwidth + 102, fbwinheight - 40, "Filter", fb_wildcard_accept, 0 );
 add_tool_tip( btn, "Refresh file listing showing only files matching the current filter strings." );
 btn = make_button( fbrowser_frame, posx1 * fbwinwidth + 152, fbwinheight - 40, "Clr", fb_wildcard_clear, 0 );
 add_tool_tip( btn, "Clear filter wildcards to Show All File Types." );
 btn = make_button( fbrowser_frame, posx1 * fbwinwidth + 188, fbwinheight - 40, "Res", fb_wildcard_restore, 0 );
 add_tool_tip( btn, "Restore the normal file filter types." );

 make_button( fbrowser_frame, fbwinwidth - 70, fbwinheight - 38, "Cancel ", cancel_fbwindow, 0 );
 for (j=0; j < FB_COLUMNS; j++) free(dir_row[j]);
 free(pathname);
 free(twrd1);
 free(word);
 free(line);
 show_wind( fbwindow );
}



void fb_extract_path_fname( char *fullname, char *path, char *fname )
{
 int j, k;
 j = strlen( fullname ) - 1;
 while ((j >= 0) && (fullname[j] != '/') && (fullname[j] != '\\'))
  j--;
 if (j < 0)
  { /*No slashes in fullname. Assume it is a filename only.*/
   path[0] = '\0';
   strcpy( fname, fullname );
  }
 else
  {
   k = 0;
   while (k <= j)
    { path[k] = fullname[k];  k++; }
   path[k] = '\0';
   k = 0;  j++;
   while (fullname[j] != '\0')
    fname[k++] = fullname[j++];
   fname[k] = '\0';
  }
}


struct fb_data_rec
 {
   char *prompt, *directory, *wildcards, *filename;
   int maxlength;
   void (*callback)(char *fname);
 } fbdata;


GtkWidget *fbwinframe;


static gboolean fb_expose_event( GtkWidget *widget, GdkEventExpose *event, gpointer data )
{
 int new_width, new_height;
 gtk_window_get_size( GTK_WINDOW( fbwindow ), &new_width, &new_height );
 if ((new_width != fbwinwidth) || (new_height != fbwinheight))
  {
   fbwinwidth = new_width;
   fbwinheight = new_height;
   gtk_widget_destroy( fbrowser_frame );
   fbrowser_frame = gtk_fixed_new();
   gtk_container_add( GTK_CONTAINER( fbwinframe ), fbrowser_frame );
   renderBrowseFiles0( fbdata.prompt, fbdata.maxlength, fbdata.directory, fbdata.wildcards, fbdata.filename, fbdata.callback );
  }
 return 0;
}


void BrowseFiles0( char *prompt, int maxlength, char *directory, char *wildcards, char *filename, void (*callback)(char *fname) )
{
 fbwinwidth = 600;
 if (SHOWFILESIZE)
  fbwinheight = 500;
 else
  fbwinheight = 450;
 fbdata.prompt = strdup( prompt );
 fbdata.maxlength = maxlength;
 fbdata.directory = strdup_padded( directory );
 fbdata.wildcards = strdup( wildcards );
 fbdata.filename = strdup( filename );
 fbdata.callback = callback;
 fbwinframe = make_window( fbwinwidth, fbwinheight, "File Browser", &fbwindow );
 gtk_window_set_resizable( GTK_WINDOW( fbwindow ), 1 );
 g_signal_connect( fbwindow, "expose-event", G_CALLBACK(fb_expose_event), NULL);
 fbrowser_frame = gtk_fixed_new();
 gtk_container_add( GTK_CONTAINER( fbwinframe ), fbrowser_frame );
 renderBrowseFiles0( prompt, maxlength, directory, wildcards, filename, callback );
}


void Browse_Files( char *prompt, int maxlength, char *directory, char *wildcards, char *filename, void (*callback)(char *fname) )
{
 fb_callback = callback;
 fb_prompt = strdup( prompt );
 fb_maxlen = maxlength;
 fb_dnptr = directory;
 strcpy_safe( fb_wildcard_default, wildcards, 500 );
 strcpy_safe( fb_wildcard, wildcards, 500 );
 fb_wcptr = wildcards;
 fb_fnptr = filename;
 fb_dirsonly = 0;
 BrowseFiles0( fb_prompt, maxlength, directory, wildcards, filename, callback );
}


void Browse_Dirs( char *prompt, int maxlength, char *directory, char *wildcards, char *dirname, void (*callback)(char *fdname) )
{
 fb_callback = callback;
 fb_prompt = strdup( prompt );
 fb_maxlen = maxlength;
 fb_dnptr = directory;
 strcpy_safe( fb_wildcard_default, wildcards, 500 );
 strcpy_safe( fb_wildcard, wildcards, 500 );
 fb_wcptr = wildcards;
 fb_fnptr = dirname;
 fb_dirsonly = 1;
 BrowseFiles0( fb_prompt, maxlength, directory, wildcards, dirname, callback );
}

void fb_allow_files( int state )	/* Sets whether Browse_Dirs can return path */
{					/* to a regular file, or only directories.	   */
 fb_allowfiles = state;			/* Zero=false (no-files, dirs-only), 1=true (allow files or directories). */
}

void fb_show_dotfiles( int state )	/* Sets whether file-browser shows files beginning with dot. */
{ fb_showdotfiles = state; }		/* 0=no.  1=yes. */

/* ------------------------ End File Browser --------------------------*/
