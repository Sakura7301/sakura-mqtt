/****************************************************************************
 * tools/mock_config.c
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
 * 3. Neither the name NuttX nor the names of its contributors may be
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
#include <stdlib.h>
#include <errno.h>

#include "config_define.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFCONFIG ".config"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/**
 * @brief get file path
 *
 * @param[in] name file name
 * @return file path
 */
static inline char *getfilepath(const char *name)
{
  (void)snprintf(line, PATH_MAX, "%s/" DEFCONFIG, name);
  line[PATH_MAX] = '\0';
  return strdup(line);
}
/**
 * @brief show usage
 *
 * @param[in] progname program name
 * @return void
 */
static void show_usage(const char *progname)
{
  (void)fprintf(stderr, "USAGE: %s <abs path to .config>\n", progname);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief main for mock_config
 *
 * @param[in] argc argument number
 * @param[in] argv argument list
 * @param[in] envp environment list
 * @return void
 */
int main(int argc, char **argv, char **envp)
{
  int ret = 0;
  char *filepath;
  FILE *stream;

  do {
    /* check argument number */
    if (argc != 2)
      {
        (void)fprintf(stderr, "Unexpected number of arguments\n");
        /* show usage info */
        show_usage(argv[0]);
        ret = 1;
        break;
      }

    /* get file path by armgument 1 */
    filepath = getfilepath(argv[1]);
    if (filepath == NULL)
      {
        /* output error info */
        (void)fprintf(stderr, "getfilepath failed\n");
        ret = 2;
        break;
      }

    /* open file */
    stream = fopen(filepath, "r");
    if (stream == NULL)
      {
        /* output error info */
        (void)fprintf(stderr, "open %s failed: %s\n", filepath, strerror(errno));
        free(filepath);
        ret = 3;
        break;
      }

    /* write autoconfig header info */
    (void)printf("/*\n");
    (void)printf(" * Copyright (c) 2021-2023 SAKURA. All rights reserved.\n");
    (void)printf(" * sakura_autoconfig.h -- Autogenerated! Do not edit.\n");
    (void)printf(" */\n\n");
    (void)printf("#ifndef SAKURA_AUTOCONFIG_H__\n");
    (void)printf("#define SAKURA_AUTOCONFIG_H__\n\n");

    /* write definition info */
    generate_definitions(stream);

    /* write end info */
    (void)printf("\n#endif /* SAKURA_AUTOCONFIG_H__ */\n\n");
    /* close file */
    (void)fclose(stream);

    /* Exit (without bothering to clean up allocations) */

    /* release resource */
    free(filepath);

  } while (FALSE);

  return ret;
}