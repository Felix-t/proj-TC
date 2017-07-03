/* This program uses the same features as example 3, but has more
   options, and somewhat more structure in the -help output.  It
   also shows how you can ‘steal’ the remainder of the input
   arguments past a certain point, for programs that accept a
   list of items.  It also shows the special argp KEY value
   ARGP_KEY_NO_ARGS, which is only given if no non-option
   arguments were supplied to the program.

   For structuring the help output, two features are used,
   *headers* which are entries in the options vector with the
   first four fields being zero, and a two part documentation
   string (in the variable DOC), which allows documentation both
   before and after the options; the two parts of DOC are
   separated by a vertical-tab character (’\v’, or ’\013’).  By
   convention, the documentation before the options is just a
   short string saying what the program does, and that afterwards
   is longer, describing the behavior in more detail.  All
   documentation strings are automatically filled for output,
   although newlines may be included to force a line break at a
   particular point.  All documentation strings are also passed to
   the ‘gettext’ function, for possible translation into the
   current locale. */

#include <stdlib.h>
#include <error.h>
#include <argp.h>

const char *argp_program_version =
  "argp-ex4 1.0";
const char *argp_program_bug_address =
  "<bug-gnu-utils@prep.ai.mit.edu>";

/* Program documentation. */
static char doc[] =
  "Program to configure and start data acquisition for thermocouples with ADAM"
  " modules\n";

/* A description of the arguments we accept. */
static char args_doc[] = "ARG1 [STRING...]";

/* Keys for options without short-options. */
#define OPT_ABORT  1            /* –abort */

/* The options we understand. */
static struct argp_option options[] = {
  {"output",   'o', "FILE",  0,
   "Output to FILE instead of standard output" },
  {"config",   'c', "FILE",  0,
   "Get the configuration from FILE instead of by scanning" },
  {"freq", 'f', "Frequency (/min)", 0, 
	"Set the acquisition frequency (default is 1/min)"},
  {"duration", 't', "DURATION", 0, "Set the acquisition duration"},
  { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
  char *arg1;                   /* arg1 */
  char **strings;               /* [string…] */
  int interactive;   /* ‘-s’, ‘-v’, ‘--abort’ */
  char *output_file;            /* file arg to ‘--output’ */
  char *config_file;            /* file arg to ‘--config’ */
  double freq;             /* count arg to ‘--repeat’ */
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'o':
      arguments->output_file = arg;
      break;
    case 'p':
      arguments->config_file = arg;
      break;
    case 'r':
      arguments->freq = arg ? atoi (arg) : 1;
      break;

    case ARGP_KEY_NO_ARGS:
      arguments->interactive = 1;

    case ARGP_KEY_ARG:
      /* Here we know that state->arg_num == 0, since we
         force argument parsing to end before any more arguments can
         get here. */
      arguments->arg1 = arg;

      /* Now we consume all the rest of the arguments.
         state->next is the index in state->argv of the
         next argument to be parsed, which is the first string
         we’re interested in, so we can just use
         &state->argv[state->next] as the value for
         arguments->strings.

         In addition, by setting state->next to the end
         of the arguments, we can force argp to stop parsing here and
         return. */
      arguments->strings = &state->argv[state->next];
      state->next = state->argc;

      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

int
main (int argc, char **argv)
{
  int i, j;
  struct arguments arguments;

  /* Default values. */
  arguments.output_file = stdout;

  /* Parse our arguments; every option seen by parse_opt will be
     reflected in arguments. */
  argp_parse (&argp, argc, argv, 0, 0, &arguments);

  if (arguments.abort)
    error (10, 0, "ABORTED");

  for (i = 0; i < 10; i++)
    {
      printf ("ARG1 = %s\n", arguments.arg1);
      printf ("STRINGS = ");
      for (j = 0; arguments.strings[j]; j++)
        printf (j == 0 ? "%s" : ", %s", arguments.strings[j]);
      printf ("\n");
      printf ("OUTPUT_FILE = %s\nVERBOSE = %s\nSILENT = %s\n",
              arguments.output_file,
              arguments.verbose ? "yes" : "no",
              arguments.silent ? "yes" : "no");
    }

  exit (0);
}
