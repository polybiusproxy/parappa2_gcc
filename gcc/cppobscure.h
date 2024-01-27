/* obscure.h - Various declarations for functions found in obscure.c
   Copyright (C) 1998 Free Software Foundation, Inc.
 */

#ifndef __GCC_CPPOBSCURE_H__
#define __GCC_CPPOBSCURE_H__

#include "cpplib.h"

#define CPP_OBSCURED_MAGIC "\x0\xb\x5\xc\x0\x7\xe"
#define CPP_OBSCURED_V01_00 "0100"
#define CPP_OBSCURED_V01_01 "0101"
#define CPP_OBSCURED_V02_00 "0200"

#define CPP_OBSCURED_VERSION CPP_OBSCURED_V02_00

#define CPP_DEFAULT_OBSCURE_DIRECTORY "obscured"
#define CPP_DEFAULT_OBSCURE_KEY "generated key"

#define CPP_OBSCURE_DIRECTORY_IS_DEFAULT(dir) \
  (bcmp ((dir), CPP_DEFAULT_OBSCURE_DIRECTORY, \
	 sizeof (CPP_DEFAULT_OBSCURE_DIRECTORY) - 1) == 0)

int handle_genobscured_option PROTO ((cpp_reader *, int, char **));
int handle_useobscured_option PROTO ((cpp_reader *, int, char **));

void generate_obscured_header PROTO ((cpp_reader *));
int  use_obscured_header      PROTO ((cpp_reader *, char *));
int  decode_obscured_header   PROTO ((cpp_reader *));
static int  key_known PARAMS ((cpp_reader *, char *));

/* non-zero if obscure header found relative to the original. */
extern int cpp_obscure_indirect;

#endif /* __GCC_CPPOBSCURE_H__ */
