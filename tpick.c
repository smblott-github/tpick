#include <ncurses.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Try to pick up GNU extensions for <fnmatch.h>.
#if defined(__linux__)
#define _GNU_SOURCE
#endif

#include <fnmatch.h>

// FNM_CASEFOLD is a GNU extension.  We get smartcase if we have FNM_CASEFOLD,
// otherwise matching is case sensitive.
#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD 0
#endif

/* **********************************************************************
 * Usage.
 */

char *usage_message[] =
   {
      "tpick [OPTIONS...] [THINGS...]",
      "OPTIONS:",
      "   -i      : read things from standard input, instead of from the command line",
      "   -p TEXT : prepend TEXT to fnmatch pattern (default is \"*\")",
      "   -s TEXT : append TEXT to fnmatch pattern (default is \"*\")",
      "   -P      : equivalent to -p \"\"",
      "   -S      : equivalent to -s \"\"",
      "   -Q      : disable exit (and fail) on two consecutive q characters",
      "   -h      : output this help message",
      0
   };

void usage(int status)
{
   int i;
   FILE *file = status ? stderr : stdout;
   for (i=0; usage_message[i]; i+=1)
      fprintf(file,"%s\n", usage_message[i]);
   exit(status);
}

/* **********************************************************************
 * Start/end curses.
 */

void curses_start()
{
   FILE *fd;

   // Open /dev/tty directly, so that standard input and/or standard output can
   // be redirected.
   if ( !(fd = fopen("/dev/tty", "r+")) )
      { fprintf(stderr, "failed to open /dev/tty\n"); exit(1); }

   set_term(newterm(NULL, fd, fd));
   cbreak(); 
   noecho();
   clear();
   raw();
   keypad(stdscr, TRUE);
   meta(stdscr, TRUE);
}

void curses_end()
{
   clrtoeol();
   refresh();
   endwin();
}

/* **********************************************************************
 * Utilities.
 */

void die(int err)
{
   curses_end();
   exit(err);
}

void fail(char *msg)
{
   curses_end();
   fprintf(stderr, "%s\n", msg);
   exit(1);
}

void *non_null(void *ptr)
{
   if ( !ptr )
   {
      curses_end();
      fprintf(stderr, "malloc failed\n");
      exit(1);
   }
   return ptr;
}

/* **********************************************************************
 * Globals.
 */

static const char *prompt = "pattern: ";
static int prompt_len = 0;

static char *prefix = "*";
static char *suffix = "*";
static int qq_quits = 1;
static int standard_input = 0;

static int argc;
static char **argv;

static WINDOW *prompt_win = 0;
static WINDOW *search_win = 0;
static WINDOW *main_win = 0;

/* **********************************************************************
 * Exit keys.
 * Exit on <ESCAPE>, <CONTROL-C> or, normally, two consecutive 'q's.
 */

void quit(const int c, const char *kn)
{
   static int qcnt = 0;

   if ( qq_quits ) {
      if ( c == 'q' ) { if ( qcnt ) die(1); else qcnt += 1; }
      else qcnt = 0;
   }

   if ( c == 27 /* escape */ || strcmp(kn,"^C") == 0 )
      die(1);
}

/* **********************************************************************
 * Slurp standard input into argv/argc.
 */

#define MAX_LINE 4096

void read_standard_input()
{
   char buf[MAX_LINE];

   argc = 0; argv = 0;
   while ( fgets(buf,MAX_LINE,stdin) )
   {
      char *newline = strchr(buf,'\n');
      if ( newline ) newline[0] = 0;
      argv = (char **) non_null(realloc(argv,(argc+1)*sizeof(char *)));
      argv[argc] = (char *) non_null(strdup(buf));
      argc += 1;
   }
}

/* **********************************************************************
 * Main.
 */

void display(int c, char *kn);
void re_display();

int main(int original_argc, char *original_argv[])
{
   prompt_len = strlen(prompt);
   argc = original_argc;
   argv = original_argv;

   int opt;
   while ( (opt = getopt(argc, argv, "Qp:s:PSi")) != -1 )
   {
      switch ( opt )
      {
         case 'Q': qq_quits = 0; break;
         case 'p': prefix = optarg; break;
         case 's': suffix = optarg; break;
         case 'P': prefix = ""; break;
         case 'S': suffix = ""; break;
         case 'i': standard_input = 1; break;
         default: usage(1);
      }
   }

   argv += optind;
   argc -= optind;

   if ( standard_input && argc )
      { fprintf(stderr, "reading from standard input, but extra arguments provided\n"); exit(1); }

   if ( standard_input )
      read_standard_input();

   if ( argc == 0 )
      { fprintf(stderr, "nothing from which to pick\n"); exit(1); }

   signal(SIGINT,  die);
   signal(SIGQUIT, die);
   signal(SIGTERM, die);
   signal(SIGWINCH, re_display);

   curses_start();
   main_win = newwin(LINES-2,COLS,2,0);
   prompt_win = newwin(1,prompt_len,0,0);
   search_win = newwin(1,COLS-prompt_len,0,prompt_len);
   waddstr(prompt_win,prompt);

   re_display();
   while ( 1 ) {
      int c = getch();
      char *kn = (char *) keyname(c);
      quit(c,kn);
      display(c,kn);
   }
}

/* **********************************************************************
 * Pattern matching.
 */

static char *fn_pattern = 0;
static int fn_flag = 0;

void fn_match_init(char *needle)
{
   int i, len = strlen(needle);

   // Smartcase.
   fn_flag = FNM_CASEFOLD;
   for (i=0; i<len && fn_flag; i+=1)
      if ( isupper(needle[i]) )
         fn_flag = 0;

   fn_pattern = (char *) non_null(realloc(fn_pattern,strlen(prefix)+len+strlen(suffix)+1));
   sprintf(fn_pattern, "%s%s%s",prefix,needle,suffix);
}

int fn_match(char *haystack)
   { return !fnmatch(fn_pattern,haystack,fn_flag); }

/* **********************************************************************
 * Display.
 */

void re_display()
   { display(0,0); }

void display(int c, char *kn)
{
   static char *selection = 0;
   static char *search = 0;
   static int search_len = 0;
   static int offset = 0;
   int i, j = 0, change = 0;

   if ( c == '\n' ) {
      curses_end();
      if ( ! selection )
         exit(1);
      printf("%s\n", selection);
      exit(0);
   }

   if ( !search )
      search = (char *) non_null(strdup(""));

   if ( kn && strcmp(kn,"KEY_DOWN") == 0 )
      { offset += 1; c = 0; }

   if ( kn && strcmp(kn,"KEY_UP") == 0 )
      { offset -= 1; c = 0; }

   if ( kn && strcmp(kn,"KEY_NPAGE") == 0 )
      { offset += LINES / 2; c = 0; }

   if ( kn && strcmp(kn,"KEY_PPAGE") == 0 )
      { offset -= LINES / 2; c = 0; }

   if ( offset < 0 )
      offset = 0;

   if ( c == ' ' )
      { c = '*'; kn = (char *) keyname(c); }

   if ( isalnum(c) || ispunct(c) )
      { change = 1; search[search_len] = c; }

   if ( c == KEY_BACKSPACE && 0 < search_len )
      change = -1;

   if ( change ) {
      search_len += change;
      search = (char *) non_null(realloc(search,search_len+1));
      search[search_len] = 0;
      wclear(search_win);
      waddstr(search_win,search);
      offset = 0;
   }

   wclear(main_win);
   selection = 0;
   fn_match_init(search);
   for (i=0; i<argc && j<LINES-2; i+=1)
      if ( fn_match(argv[i]) )
      {
         j += 1;
         if ( j <= offset )
            continue;
         if ( selection == 0 )
            selection = argv[i];
         if ( j-offset == 1 ) wattron(main_win, A_REVERSE);
         waddstr(main_win,argv[i]);
         if ( j-offset == 1 ) wattroff(main_win, A_REVERSE);
         wclrtoeol(main_win);
         wmove(main_win,j-offset,0);
      }

   // Ensure we don't scroll off the bottom of the list.
   if ( offset && j <= offset )
   {
      offset = j ? j-1 : 0;
      re_display();
      return;
   }

   refresh();
   wrefresh(main_win);
   wrefresh(prompt_win);
   wrefresh(search_win);
   wmove(search_win,0,search_len);
}

