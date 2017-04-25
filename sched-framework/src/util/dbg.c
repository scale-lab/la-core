/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /**
 *  @file originally dbg.c, now dbg.C
 * @author kma, modified by ashuang to work with CC and to
 *         remove some unnecessary details
 *  @desc mcc's debugging spice
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "debug/dbg_modes.h"
#include "debug/dbg.h"

unsigned long long dbg_modes = 0;
short dbg_initiated = 0;

static dbg_mode_color_t dbg_colortab[] = {
  DBG_COLORTAB
};

static dbg_mode_t dbg_nametab[] = {
  DBG_NAMETAB
};

const char* DCOLOR(unsigned long long d_mode) {
  for (dbg_mode_color_t* mode = dbg_colortab; mode->d_mode != 0; mode++) {
    if (mode->d_mode & d_mode)
      return mode->color;
  }

  return _BWHITE_;
}

/**
* searches for <code>name</code> in the list of known
* debugging modes specified in dbg_modes.H and, if it
* finds <code>name</code>, adds the corresponding
* debugging mode to a list
*/
void dbg_add_mode(const char *name) {
  int cancel;
  dbg_mode_t *mode;

  if (*name == '-') {
    cancel = 1;
    name++;
  }
  else {
    cancel = 0;
  }

  for (mode = dbg_nametab; mode->d_name != NULL; mode++)
    if (strcmp(name, mode->d_name) == 0)
      break;
  if (mode->d_name == NULL) {
    fprintf(stderr, "Warning: Unknown debug option: " "\"%s\"\n", name);
    return;
  }

  if (cancel) {
    dbg_modes &= ~mode->d_mode;
  }
  else {
    dbg_modes = dbg_modes | mode->d_mode;
  }
}

/**
* Cycles through each comma-delimited debugging option and
* adds it to the debugging modes by calling dbg_add_mode
*/
void dbg_add_modes(const char *modes) {
  char env[256];
  char *name;

  strncpy(env, modes, sizeof(env));
  for (name = strtok(env, ","); name; name = strtok(NULL, ","))
    dbg_add_mode(name);
}

/**
* Reads the environment variable specified in dbg_modes.H and
* calls dbg_add_modes()
*/
void dbg_init() {
  dbg_initiated = 1;

  dbg_modes = DBG_DEFAULT;
  dbg_add_modes(DBG_START_MODES);
}