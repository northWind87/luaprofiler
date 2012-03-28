/*
** LuaProfiler
** Copyright Kepler Project 2005.2007 (http://www.keplerproject.org/luaprofiler)
** $Id: core_profiler.c,v 1.10 2009-01-29 12:39:28 jasonsantos Exp $
*/

/*****************************************************************************
core_profiler.c:
   Lua version independent profiler interface.
   Responsible for handling the "enter function" and "leave function" events
   and for writing the log file.

Design (using the Lua callhook mechanism) :
   'lprofP_init_core_profiler' set up the profile service
   'lprofP_callhookIN'         called whenever Lua enters a function
   'lprofP_callhookOUT'        called whenever Lua leaves a function
*****************************************************************************/

/*****************************************************************************
   The profiled program can be viewed as a graph with the following properties:
directed, multigraph, cyclic and connected. The log file generated by a
profiler section corresponds to a path on this graph.
   There are several graphs for which this path fits on. Some times it is
easier to consider this path as being generated by a simpler graph without
properties like cyclic and multigraph.
   The profiler log file can be viewed as a "reversed" depth-first search
(with the depth-first search number for each vertex) vertex listing of a graph
with the following properties: simple, acyclic, directed and connected, for
which each vertex appears as many times as needed to strip the cycles and
each vertex has an indegree of 1.
   "reversed" depth-first search means that instead of being "printed" before
visiting the vertex's descendents (as done in a normal depth-first search),
the vertex is "printed" only after all his descendents have been processed (in
a depth-first search recursive algorithm).
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "function_meter.h"

#include "core_profiler.h"

    /* default log name (%s is used to place a random string) */
#define OUT_FILENAME "lprof_%s.out"

#define MAX_FUNCTION_NAME_LENGTH 256

    /* for faster execution (??) */
static FILE *outf;
static lprofS_STACK_RECORD *info;
static LPFLOAT function_call_time;


/* output a line to the log file, using 'printf()' syntax */
/* assume the timer is off */
static void output(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(outf, format, ap);
  va_end(ap);

  /* write now to avoid delays when the timer is on */
  fflush(outf);
}


/* do not allow a string with '\n' and '|' (log file format reserved chars) */
/* - replace them by ' '                                                    */
static void formats(char *s) {
  int i;
  if (!s)
    return;
  for (i = strlen(s); i>=0; i--) {
    if ((s[i] == '|') || (s[i] == '\n'))
      s[i] = ' ';
  }
}


/* computes new stack and new timer */
void lprofP_callhookIN(lprofP_STATE* S, char *func_name, char *file, int linedefined, int currentline) {	
  S->stack_level++;
  lprofM_enter_function(S, file, func_name, linedefined, currentline);
}


/* pauses all timers to write a log line and computes the new stack */
/* returns if there is another function in the stack */
int lprofP_callhookOUT(lprofP_STATE* S) {
  char* source;
  char* name;

  if (S->stack_level == 0) {
    return 0;
  }

  S->stack_level--;

  /* 0: do not resume the parent function's timer yet... */
  info = lprofM_leave_function(S, 0);
  /* writing a log may take too long to be computed with the function's time ...*/
  lprofM_pause_total_time(S);
  info->local_time += function_call_time;
  info->total_time += function_call_time;
  
  source = info->file_defined;
  if (source[0] != '@') {
     source = "(string)";
  }
  else {
     formats(source);
  }
  name = info->function_name;
  
  if (strlen(name) > MAX_FUNCTION_NAME_LENGTH) {
     name = malloc(MAX_FUNCTION_NAME_LENGTH+10);
     name[0] = '\"';
     strncpy(name+1, info->function_name, MAX_FUNCTION_NAME_LENGTH);
     name[MAX_FUNCTION_NAME_LENGTH] = '"';
     name[MAX_FUNCTION_NAME_LENGTH+1] = '\0';
  }
  formats(name);
  output("%d\t%s\t%s\t%d\t%d\t%f\t%f\n", S->stack_level, source, name, 
	 info->line_defined, info->current_line,
	 info->local_time, info->total_time);
  /* ... now it's ok to resume the timer */
  if (S->stack_level != 0) {
    lprofM_resume_function(S);
  }

  return 1;

}


/* opens the log file */
/* returns true if the file could be opened */
lprofP_STATE* lprofP_init_core_profiler(const char *_out_filename, int isto_printheader, LPFLOAT _function_call_time) {
  lprofP_STATE* S;
  char auxs[256];
  char *s;
  char *randstr;
  const char *out_filename;

  function_call_time = _function_call_time;
  out_filename = (_out_filename) ? (_out_filename):(OUT_FILENAME);
        
  /* the random string to build the logname is extracted */
  /* from 'tmpnam()' (the '/tmp/' part is deleted)     */
  randstr = tmpnam(NULL);
  for (s = strtok(randstr, "/\\"); s; s = strtok(NULL, "/\\")) {
    randstr = s;
  }

  if(randstr[strlen(randstr)-1]=='.')
    randstr[strlen(randstr)-1]='\0';

  sprintf(auxs, out_filename, randstr);
  outf = fopen(auxs, "a");
  if (!outf) {
    return 0;
  }

  if (isto_printheader) {
    output("stack_level\tfile_defined\tfunction_name\tline_defined\tcurrent_line\tlocal_time\ttotal_time\n");
  }

  /* initialize the 'function_meter' */
  S = lprofM_init();
  if(!S) {
    fclose(outf);
    return 0;
  }
    
  return S;
}

void lprofP_close_core_profiler(lprofP_STATE* S) {
  if(outf) fclose(outf);
  if(S) free(S);
}

lprofP_STATE* lprofP_create_profiler(LPFLOAT _function_call_time) {
  lprofP_STATE* S;

  function_call_time = _function_call_time;

  /* initialize the 'function_meter' */
  S = lprofM_init();
  if(!S) {
    return 0;
  }
    
  return S;
}

