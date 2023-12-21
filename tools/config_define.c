/****************************************************************************
 * tools/config_define.c
 *
 *   Copyright (C) 2007-2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name Nutt nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>
#include <ctype.h>
#include "config_define.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

char line[LINESIZE+1];

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* These are configuration variable name that are quoted by configuration tool
 * but which must be unquoted when used in C code.
 */

static const char *dequote_list[] =
{
  /* NuttX */

  "CONFIG_USER_ENTRYPOINT",               /* Name of entry point function */
  "CONFIG_EXECFUNCS_SYMTAB_ARRAY",        /* Symbol table array used by exec[l|v] */
  "CONFIG_EXECFUNCS_NSYMBOLS_VAR",        /* Variable holding number of symbols in the table */
  "CONFIG_MODLIB_SYMTAB_ARRAY",           /* Symbol table array used by modlib functions */
  "CONFIG_MODLIB_NSYMBOLS_VAR",           /* Variable holding number of symbols in the table */
  "CONFIG_PASS1_BUILDIR",                 /* Pass1 build directory */
  "CONFIG_PASS1_TARGET",                  /* Pass1 build target */
  "CONFIG_PASS1_OBJECT",                  /* Pass1 build object */
  "CONFIG_DEBUG_OPTLEVEL",                /* Custom debug level */
  "CONFIG_INIT_SYMTAB",                   /* Global symbol table */
  "CONFIG_INIT_NEXPORTS",                 /* Global symbol table size */

  /* NxWidgets/NxWM */

  "CONFIG_NXWM_BACKGROUND_IMAGE",         /* Name of bitmap image class */
  "CONFIG_NXWM_STOP_BITMAP",              /* Name of bitmap image class */
  "CONFIG_NXWM_MINIMIZE_BITMAP",          /* Name of bitmap image class */
  "CONFIG_NXWM_STARTWINDOW_ICON",         /* Name of bitmap image class */
  "CONFIG_NXWM_NXTERM_ICON",              /* Name of bitmap image class */
  "CONFIG_NXWM_CALIBRATION_ICON",         /* Name of bitmap image class */
  "CONFIG_NXWM_HEXCALCULATOR_ICON",       /* Name of bitmap image class */

  /* apps/ definitions */

  "CONFIG_EXAMPLES_HELLO_PROGNAME",       /* Name of installed hello example program */
  "CONFIG_SYSTEM_NSH_PROGNAME",           /* Name of installed NSH example program */
  "CONFIG_SYSTEM_NSH_SYMTAB_ARRAYNAME",   /* Symbol table array name */
  "CONFIG_SYSTEM_NSH_SYMTAB_COUNTNAME",   /* Name of the variable holding the number of symbols */
  "CONFIG_THTTPD_INDEX_NAMES",            /* List of index file names */
  NULL                                    /* Marks the end of the list */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/**
 * @brief Skip over any spaces
 *
 * @param[in] ptr variable string
 * @return the pointer to the begin which is not space
 */
static char *skip_space(char *ptr)
{
  while (*ptr != '\0' && isspace((unsigned char)*ptr)) ptr++;
  return ptr;
}

/**
 * @brief Find the end of a variable string
 *
 * @param[in] ptr variable string
 * @return the pointer to end of name string
 */
static char *find_name_end(char *ptr)
{
  while (*ptr != '\0' && (isalnum((unsigned char)*ptr) || *ptr == '_')) ptr++;
  return ptr;
}

/**
 * @brief Find the end of a value string
 *
 * @param[in] ptr value string
 * @return the pointer to end of value string
 */
static char *find_value_end(char *ptr)
{
  while (*ptr != '\0' && !isspace((unsigned char)*ptr))
    {
      /* begin with '"' */
      if (*ptr == '"')
        {
           /* skip until the char is '\0' or '"' */
           do ptr++; while (*ptr != '\0' && *ptr != '"');
           if (*ptr != '\0') ptr++;
        }
      else
        {
           /* skip until the char is '\0' or '"' */
           do ptr++; while (*ptr != '\0' && !isspace((unsigned char)*ptr) && *ptr != '"');
        }
    }
  return ptr;
}

/**
 * @brief Read the next line from the configuration file
 *
 * @param[in] stream file pointer
 * @return line string
 */
static char *read_line(FILE *stream)
{
  char *ptr = NULL;
  int is_line_valid = 0;

  do {
    for (;;) {
      is_line_valid = 0;
      line[LINESIZE] = '\0';
      /* get line string */
      if (fgets(line, LINESIZE, stream) == NULL) {
        break;
      } else {
        /* skip the space */
        ptr = skip_space(line);

        /* if the string not valid then break */
        if (*ptr != '\0' && *ptr != '#' && *ptr != '\n') {
          is_line_valid = 1;
          break;
        }
      }
    }
    /* fix end line is "#" */
    if (is_line_valid == 0) {
      ptr = NULL;
    }
  } while (FALSE);

  return ptr;
}

/**
 * @brief Parse the line from the configuration file into a variable name
 * string and a value string.
 *
 * @param[in] ptr line string
 * @param[out] varname variable name
 * @param[out] varval  variable value
 * @return void
 */
static void parse_line(char *ptr, char **varname, char **varval)
{
  /* Skip over any leading spaces */

  ptr = skip_space(ptr);

  /* The first no-space is the beginning of the variable name */

  *varname = skip_space(ptr);
  *varval = NULL;

  /* Parse to the end of the variable name */

  ptr = find_name_end(ptr);

  /* An equal sign is expected next, perhaps after some white space */

  if (*ptr != '\0' && *ptr != '=')
    {
      /* Some else follows the variable name.  Terminate the variable
       * name and skip over any spaces.
       */

      *ptr = '\0';
       ptr = skip_space(ptr + 1);
    }

  /* Verify that the equal sign is present */

  if (*ptr == '=')
    {
      /* Make sure that the variable name is terminated (this was already
       * done if the name was followed by white space.
       */

      *ptr = '\0';

      /* The variable value should follow =, perhaps separated by some
       * white space.
       */

      ptr = skip_space(ptr + 1);
      if (*ptr != '\0')
        {
          /* Yes.. a variable follows.  Save the pointer to the start
           * of the variable string.
           */

          *varval = ptr;

          /* Find the end of the variable string and make sure that it
           * is terminated.
           */

          ptr = find_value_end(ptr);
          *ptr = '\0';
        }
    }
}
/**
 * @brief remove the quote char of variable name and value
 *
 * @param[in] varname variable name
 * @param[in] varval  variable value
 * @return unquoted string result
 */
static char *dequote_value(const char *varname, char *varval)
{
  const char **dqnam;
  char *dqval = varval;
  char *ptr;
  int len;
  int i;

  if (dqval != NULL)
    {
      /* Check if the variable name is in the list of strings to be dequoated */

      for (dqnam = dequote_list; (*dqnam) != NULL; dqnam++)
        {
          if (strcmp(*dqnam, varname) == 0)
            {
              break;
            }
        }

      /* Did we find the variable name in the list of configuration variables
       * to be dequoted?
       */

      if ((*dqnam) != NULL)
        {
          /* Yes... Check if there is a trailing quote */

          len = strlen(dqval);
          if (dqval[len-1] == '"')
            {
              /* Yes... replace it with a terminator */

              dqval[len-1] = '\0';
              len--;
            }

          /* Is there a leading quote? */

           if (dqval[0] == '"')
             {
               /* Yes.. skip over the leading quote */

               dqval++;
               len--;
             }

           /* A special case is a quoted list of quoted strings.  In that case
            * we will need to remove the backspaces from the internally quoted
            * strings.  NOTE: this will not handle nested quoted quotes.
            */

           for (ptr = dqval; *ptr != '\0'; ptr++)
             {
               /* Check for a quoted quote */

               if (ptr[0] == '\\' && ptr[1] == '"')
                 {
                   /* Delete the backslash by moving the rest of the string */

                   for (i = 0; ptr[i] != '\0'; i++)
                     {
                       ptr[i] = ptr[i+1];
                     }

                   len--;
                 }
             }

           /* Handle the case where nothing is left after dequoting */

           if (len <= 0)
             {
               dqval = NULL;
             }
        }
    }

  return dqval;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief generate define macro statement in file
 *
 * @param[in] stream file pointer
 * @return void
 */
void generate_definitions(FILE *stream)
{
  char *varname;
  char *varval;
  char *ptr;

  /* Loop until the entire file has been parsed. */

  do
    {
      /* Read the next line from the file */

      ptr = read_line(stream);
      if (ptr != NULL)
        {
          /* Parse the line into a variable and a value field */

          parse_line(ptr, &varname, &varval);

          /* Was a variable name found? */

          if (varname != NULL)
            {
              /* Yes.. dequote the value if necessary */

              varval = dequote_value(varname, varval);

              /* If no value was provided or if the special value 'n' was provided,
               * then undefine the configuration variable.
               */

              if (varval == NULL || strcmp(varval, "n") == 0)
                {
                  (void)printf("#undef %s\n", varname);
                }

              /* Simply define the configuration variable to '1' if it has the
               * special value "y"
               */

              else if (strcmp(varval, "y") == 0)
                {
                  (void)printf("#define %s 1\n", varname);
                }

              /* Or to '2' if it has the special value 'm' */

              else if (strcmp(varval, "m") == 0)
                {
                  (void)printf("#define %s 2\n", varname);
                }

              /* Otherwise, use the value as provided */

              else
                {
                  (void)printf("#define %s %s\n", varname, varval);
                }
            }
        }
    }
  while (ptr != NULL);
}
