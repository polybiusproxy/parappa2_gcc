/* Obscured Header Functions.
   Copyright (C) 1998 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* These functions are used to implement obscured headers in any language
   front end which uses cpplib as it's preprocessor/lexer.  */

#include "config.h"
#include "system.h"
#include "gansidecl.h"
#include "cpplib.h"
#include "cpphash.h"
#include "cppobscure.h"
#include <utime.h>

#ifndef DIR_SEPARATOR
#define DIR_SEPARATOR '/'
#endif

int cpp_obscure_indirect;

static char *get_obscured_header_directory_name
  PARAMS ((char *, struct cpp_obscured_header_option *));

static char *
get_obscured_header_key PARAMS ((char *, struct cpp_obscured_header_option *));

static char *
get_obscured_file_name PARAMS ((char *, cpp_obscured_header_option*, int));

static void write_obscured_header PARAMS ((FILE *, cpp_reader *));
static void write_unobscured_header PARAMS ((FILE *, cpp_buffer *));

static int
obscured_header_up_to_date PARAMS ((char *, struct stat *stat_original));

static void generate_unobscured_header PARAMS ((cpp_reader *));
static int  key_known PARAMS ((cpp_reader *, char *));

static void io_error PARAMS ((cpp_reader *, int, char *, char *));

static void *my_memmove PARAMS ((void *, void *, size_t));

#ifdef DEBUG_OBSCURED
#define DEBUG(args) fprintf args
#else
#define DEBUG(args) ((void)0)
#endif

/*
 * Generate an obscured version of the file represented by the current
 * buffer in pfile.  This function assumes that the file has already
 * been opened and read into the buffer.
 */
void
generate_obscured_header (pfile)
     cpp_reader *pfile;
{
  struct cpp_buffer *buffer = CPP_BUFFER (pfile);
  struct cpp_options *opts  = CPP_OPTIONS (pfile);
  char *filename = buffer->fname;
  char *obscured_filename;
  FILE *obscured_file;
  int rc;
  struct stat stat_original;
  struct utimbuf time_buf;
  DEBUG ((stderr, "Checking generation of obscured header for %s in %s\n",
	  filename, opts->gen_obscured.directory));

  /* We can't generate an obscured header unless we know something about the
     original.  */
  rc = stat (filename, &stat_original);
  if (rc != 0)
    return;

  obscured_filename = get_obscured_file_name (filename, &opts->gen_obscured, 0);

  if (obscured_header_up_to_date (obscured_filename, &stat_original))
    return;

  /* The obscured version either doesn't exist or is out of date.
     Make a new one.  */
  DEBUG ((stderr, "  writing obscured header %s\n", obscured_filename));
  errno = 0;
  obscured_file = fopen (obscured_filename, "wb");
  if (obscured_file == NULL)
    {
      if (errno != ENOENT)
	{
	  io_error (pfile, 0, "Could not create obscured header %s",
		    obscured_filename);
	}
      return;
    }

  write_obscured_header (obscured_file, pfile);
  fclose (obscured_file);

  /* Set the timestamp of the obscured version to be the same as the original.
     This is used to test whether the obscured version is up to date.  */
  time_buf.actime  = stat_original.st_atime;
  time_buf.modtime = stat_original.st_mtime;
  utime (obscured_filename, &time_buf);
  /* TODO: free obscured_filename? */
}

/*
 * Obscure the data in the current buffer in pfile and write it to
 * the given file.
 */
static void
write_obscured_header (file, pfile)
     FILE *file;
     cpp_reader *pfile;
{
  cpp_buffer *buffer;
  char *key;
  
  fwrite (CPP_OBSCURED_MAGIC, 1, sizeof (CPP_OBSCURED_MAGIC), file);
  fwrite (CPP_OBSCURED_VERSION, 1, sizeof (CPP_OBSCURED_VERSION), file);
  key = CPP_OPTIONS (pfile)->gen_obscured.keys->name;
  buffer = CPP_BUFFER (pfile);
#if 0 /* version 1.00 */
  fwrite (key, 1, strlen (key) + 1, file);
  fwrite (buffer->buf, sizeof (*buffer->buf), buffer->rlimit - buffer->buf,
	  file);
#endif
#if 0 /* version 1.01 */
  fwrite (key, 1, strlen (key) + 1, file);
  {
    char *buf = buffer->buf;
    int i;
    for (i = buffer->rlimit - buffer->buf - 1; i >= 0; --i)
      fputc (buf[i], file);
  }
#endif /* version 2.00 */
  {
    char *buf = buffer->buf;
    int key_length = strlen (key);
    int offset = hashf (key, key_length, 0xff);
    int i;
    if (offset == 0)
      offset = 123; /* some arbitrary non-zero number */
    fputc (offset, file);
    for (i = 0; i <= key_length; ++i) /* encode the terminating nul too */
      fputc (key[i] + offset, file);
    for (i = buffer->rlimit - buffer->buf - 1; i >= 0; --i)
      fputc (buf[i] + offset, file);
  }
}

/*
 * Write the data in the current buffer in pfile to the given file.
 */
static void
write_unobscured_header (file, buffer)
     FILE *file;
     cpp_buffer *buffer;
{
  fwrite (buffer->buf, sizeof (*buffer->buf), buffer->rlimit - buffer->buf,
	  file);
}

/*
 * Decode the obscured header data in the current buffer of pfile and
 * store the decoded version in the same buffer.  Generate an unobscured
 * version of the header if necessary.
 */
int
decode_obscured_header (pfile)
     cpp_reader *pfile;
{
  cpp_buffer *buffer = CPP_BUFFER (pfile);
  unsigned char *buf = buffer->buf;
  int length = buffer->rlimit - buf;
  int header_length = sizeof (CPP_OBSCURED_MAGIC)
                      + sizeof (CPP_OBSCURED_VERSION);
  char *obscured_header_key;

  if (length < header_length
      || bcmp (buf, CPP_OBSCURED_MAGIC, sizeof (CPP_OBSCURED_MAGIC)) != 0)
    return 0; /* not obscured */

  if (bcmp (buf + sizeof (CPP_OBSCURED_MAGIC), CPP_OBSCURED_V01_00,
	    sizeof (CPP_OBSCURED_VERSION)) == 0)
    {
      int key_length;
      /* Version 1.00 is simply the text of the file repeated verbatim.
	 Used for debugging.  */
      DEBUG ((stderr, "Obscured header for %s version %s\n",
              buffer->fname, buf + sizeof (CPP_OBSCURED_MAGIC)));

      key_length = strlen (buf + header_length) + 1;
      obscured_header_key = (char *) alloca (key_length);
      bcopy (buf + header_length, obscured_header_key, key_length);
      header_length += key_length;

      my_memmove (buf, buf + header_length, length - header_length);
      buffer->rlimit -= header_length;
    }
  else if (bcmp (buf + sizeof (CPP_OBSCURED_MAGIC), CPP_OBSCURED_V01_01,
		 sizeof (CPP_OBSCURED_VERSION)) == 0)
    {
      /* Version 1.01 is simply the text of the file reversed.
	 Just a toy format used for debugging.  */
      int key_length;
      int i;
      DEBUG ((stderr, "Obscured header for %s version %s\n",
	      buffer->fname, buf + sizeof (CPP_OBSCURED_MAGIC)));

      key_length = strlen (buf + header_length) + 1;
      obscured_header_key = (char *) xmalloc (key_length);
      bcopy (buf + header_length, obscured_header_key, key_length);
      header_length += key_length;

      length -= header_length;
      my_memmove (buf, buf + header_length, length);
      for (i = 0; i < length / 2; ++i)
	{
	  char temp = buf[i];
	  buf[i] = buf[length - i - 1];
	  buf[length - i - 1] = temp;
	}
      buffer->rlimit -= header_length;
    }
  else if (bcmp (buf + sizeof (CPP_OBSCURED_MAGIC), CPP_OBSCURED_V02_00,
		 sizeof (CPP_OBSCURED_VERSION)) == 0)
    {
      int offset;
      int i;
      int key_length;
      DEBUG ((stderr, "Obscured header for %s version %s\n",
              buffer->fname, buf + sizeof (CPP_OBSCURED_MAGIC)));
      offset = buf[header_length];
      ++header_length;
      for (i = header_length; buf[i] != offset; ++i)
	;
      key_length = i - header_length + 1;
      obscured_header_key = (char *) xmalloc (key_length);
      for (i = 0; i < key_length; ++i) /* get the terminator too */
	obscured_header_key[i] = buf[header_length + i] - offset;
      header_length += key_length;
      length -= header_length;
      my_memmove (buf, buf + header_length, length);
      for (i = 0; i < length / 2; ++i)
	{
	  char temp = buf[i];
	  buf[i] = buf[length - i - 1] - offset;
	  buf[length - i - 1] = temp - offset;
	}
      if (length % 2 != 0)
	buf[i] -= offset;
      buffer->rlimit -= header_length;
    }
  else
    {
      cpp_message (pfile, 2, "Unknown obscured header version", "", "", "");
      return 0;
    }

  buffer->obscured_key_known = key_known (pfile, obscured_header_key);
  if (buffer->obscured_key_known)
    {
      if (! cpp_obscure_indirect)
	generate_unobscured_header (pfile);
    }
  else
    {
#if 0 /* doesn't seem to work anyway */
      if (CPP_OPTIONS (pfile)->no_output == 0)
	{
	  int file_len = strlen (buffer->fname);
	  CPP_RESERVE (pfile, 31 + file_len + 12);
	  CPP_PUTS_Q (pfile, "/* Contents of obscured header ", 31);
	  CPP_PUTS_Q (pfile, buffer->fname, file_len);
	  CPP_PUTS_Q (pfile, " omitted */\n", 12);
	  ++pfile->lineno;
	}
#endif
      ++(CPP_OPTIONS (pfile)->no_output);
    }

  return 1;
}

/*
 * Search the list of keys sepcified on the useobscured option for
 * the given key.
 * Return 1 if found, 0 otherwise.
 */
static int
key_known (pfile, key)
     cpp_reader *pfile;
     char *key;
{
  cpp_options *opts = CPP_OPTIONS (pfile);
  cpp_obscured_header_key *opt_key;
  for (opt_key = opts->use_obscured.keys;
       opt_key != NULL;
       opt_key = opt_key->next)
    {
      if (strcmp (key, opt_key->name) == 0)
	return 1; /* found a match */
    }
  return 0; /* not found */
}

/*
 * Generate an unobscured header containing the data in the current buffer
 * of pfile. Do this only if the unobscured header doesn't exist or if
 * it is out of date.
 */
static void
generate_unobscured_header (pfile)
     cpp_reader *pfile;
{
  struct cpp_buffer *buffer = CPP_BUFFER (pfile);
  struct cpp_options *opts  = CPP_OPTIONS (pfile);
  char *filename = buffer->fname;
  char *unobscured_filename;
  FILE *unobscured_file;
  int rc;
  struct stat stat_obscured;
  struct utimbuf time_buf;
  DEBUG ((stderr, "Checking generation of unobscured header for %s in %s~\n",
	  filename, opts->gen_obscured.directory));

  /* We can't generate an unobscured header unless we know something about the
     obscured one.  */
  rc = stat (filename, &stat_obscured);
  if (rc != 0)
    return;

  if (opts->gen_obscured.enabled)
    unobscured_filename = get_obscured_file_name (filename,
						  &opts->gen_obscured, 1);
  else
    unobscured_filename = get_obscured_file_name (filename,
						  &opts->use_obscured, 1);

  if (obscured_header_up_to_date (unobscured_filename, &stat_obscured))
    return;

  /* The unobscured version either doesn't exist or is out of date.
     Make a new one.  */
  DEBUG ((stderr, "  writing unobscured header %s\n", unobscured_filename));
  errno = 0;
  unobscured_file = fopen (unobscured_filename, "wb");
  if (unobscured_file == NULL)
    {
      if (errno != ENOENT)
	{
	  io_error (pfile, 0, "Could not create unobscured header %s",
		    unobscured_filename);
	}
      return;
    }

  write_unobscured_header (unobscured_file, buffer);
  fclose (unobscured_file);

  /* Set the timestamp of the obscured version to be the same as the original.
     This is used to test whether the obscured version is up to date.  */
  time_buf.actime  = stat_obscured.st_atime;
  time_buf.modtime = stat_obscured.st_mtime;
  utime (unobscured_filename, &time_buf);
  /* TODO: free unobscured_filename? */
}

/*
 * Open the obscured version of the given filename if it exists and is
 * up to date.
 * Return the handle of the opened file, if successful.
 * Return 0 if the obscured header is not to be used.
 */
int
use_obscured_header (pfile, filename)
     cpp_reader *pfile;
     char *filename;
{
  struct cpp_options *opts  = CPP_OPTIONS (pfile);
  struct stat stat_original;
  char *obscured_filename;
  int rc;
  DEBUG ((stderr, "Checking for obscured version of %s in %s\n",
	 filename, opts->use_obscured.directory));

  /* We can't use an obscured header unless we know something about the
     original.  */
  rc = stat (filename, &stat_original);
  if (rc != 0)
    return;

  obscured_filename = get_obscured_file_name (filename, &opts->use_obscured, 0);

  if (! obscured_header_up_to_date (obscured_filename, &stat_original))
    return 0;

  DEBUG ((stderr, "  reading obscured header %s\n", obscured_filename));

  /* The obscured header exists and is up to date, so use it.  */
  cpp_obscure_indirect = 1;
  return open (obscured_filename, O_RDONLY, 0666);
}

/*
 * Construct the name of the obscured or unobscured header associated with
 * the given filename using the directory name in opts.  Generate an obscured
 * name if unobscured is 0, generate an unobscured name otherwise.
 */
static char *
get_obscured_file_name (filename, opts, unobscured)
     char *filename;
     cpp_obscured_header_option* opts;
     int unobscured;
{
  char *ep;
  char *obscured_filename;
  int dir_length;
  int sub_dir_length;
  int file_length;
  int total_length;
  /* First construct the name of the obscured header related to this file.  */
#ifndef VMS
  ep = rindex (filename, '/');
#else				/* VMS */
  ep = rindex (filename, ']');
  if (ep == NULL) ep = rindex (filename, '>');
  if (ep == NULL) ep = rindex (filename, ':');
#endif				/* VMS */
  if (ep != NULL)
    {
      ep++;
      dir_length = ep - filename;
    }
  else
    dir_length = 0;
  
  sub_dir_length = strlen (opts->directory);
  file_length = strlen (filename) - dir_length;
  total_length = dir_length + sub_dir_length + unobscured + file_length;

  obscured_filename = (char *) xmalloc (total_length + 1);
  bcopy (filename, obscured_filename, dir_length);
  bcopy (opts->directory, obscured_filename + dir_length, sub_dir_length);
  bcopy (filename + dir_length,
	 obscured_filename + dir_length + sub_dir_length + unobscured,	 
	 file_length + 1);
  if (unobscured)
    {
      obscured_filename[dir_length + sub_dir_length] = DIR_SEPARATOR;
      obscured_filename[dir_length + sub_dir_length - 1] = '~';
    }

  DEBUG ((stderr, "  %sobscured header name is %s\n",
	  unobscured ? "un" : "", obscured_filename));

  return obscured_filename;
}

/*
 * Determine whether the given obscured_filename is up to date relative
 * to the data in stat_original. Return 1 if it is. Return 0 if it does
 * not exist or is out of date.
 */
static int
obscured_header_up_to_date (obscured_filename, stat_original)
     char *obscured_filename;
     struct stat *stat_original;
{
  struct stat stat_obscured;
  int rc;

   rc = stat (obscured_filename, &stat_obscured);
  if (rc != 0)
    return 0;

  /* It exists, now see if it is up to date.  */
  DEBUG ((stderr, "  obscured header %s exists\n", obscured_filename));
  if (stat_original->st_mtime == stat_obscured.st_mtime)
    {
      DEBUG ((stderr, "  obscured header %s is up to date\n",
	      obscured_filename));
      return 1; /* exists and is up to date.  */
    }

  return 0;
}

/*
 * Parse the -fgenobscured option. Return the number of argv strings
 * consumed.
 */
int
handle_genobscured_option (pfile, argc, argv)
     cpp_reader *pfile;
     int argc;
     char **argv;
{
  struct cpp_options *opts = CPP_OPTIONS (pfile);
  char *option = argv[0] + 13; /* get past -fgenobscured */

  opts->gen_obscured.enabled = 1;
  option = get_obscured_header_directory_name (option, &opts->gen_obscured);

  opts->gen_obscured.keys = NULL;
  get_obscured_header_key (option, &opts->gen_obscured);

  return 1;
}

/*
 * Parse the -fuseobscured option. Return the number of argv strings
 * consumed.
 */
int
handle_useobscured_option (pfile, argc, argv)
     cpp_reader *pfile;
     int argc;
     char **argv;
{
  struct cpp_options *opts = CPP_OPTIONS (pfile);
  char *option = argv[0] + 13; /* get past -fuseobscured */

  opts->use_obscured.enabled = 1;
  option = get_obscured_header_directory_name (option, &opts->use_obscured);

  opts->use_obscured.keys = NULL;
  do
    option = get_obscured_header_key (option, &opts->use_obscured);
  while (*option != '\0');

  return 1;
}

/*
 * Extract the directory name argument from the given fgenobscured or
 * fuseobscured option and place it in opts.
 * This function assumes that the option keyword has already been consumed.
 * Return the remaining unconsumed portion of the option.
 */
static char *
get_obscured_header_directory_name (option, opts)
     char *option;
     struct cpp_obscured_header_option *opts;
{
  char *next_option;
  char *dir_name;
  size_t len;
  int extra;

  if (*option == '=')
    {
      dir_name = ++option;
      len = strlen (dir_name);
      next_option = index (dir_name, ':');
      if (next_option != NULL)
	len = next_option - dir_name;
      option += len;

      /* The directory name is relative. Strip off any leading separators.  */
      for ( ; *dir_name == DIR_SEPARATOR; ++dir_name, --len)
	;

      if (len == 0)
	{
	  dir_name = CPP_DEFAULT_OBSCURE_DIRECTORY;
	  len = sizeof (CPP_DEFAULT_OBSCURE_DIRECTORY) - 1;
	}
    }
  else
    {
      dir_name = CPP_DEFAULT_OBSCURE_DIRECTORY;
      len = sizeof (CPP_DEFAULT_OBSCURE_DIRECTORY) - 1;
    }

  extra = (dir_name[len - 1] != DIR_SEPARATOR);
  opts->directory = (char *) xmalloc (len + extra + 1);
  bcopy (dir_name, opts->directory, len);
  if (extra)
    {
      opts->directory[len] = DIR_SEPARATOR;
      ++len;
    }
  opts->directory[len] = '\0';

  return option;
}

/*
 * Extract one obscured header key from the given portion of a fgenobscured or
 * fuseobscured option. This function assumes that the option has
 * already been consumed up to the point where the key may be specified.
 * Return the remaining unconsumed portion of the option.
 */
static char *
get_obscured_header_key (option, opts)
     char *option;
     struct cpp_obscured_header_option *opts;
{
  char *next_option;
  int option_length;
  struct cpp_obscured_header_key* key;

  option_length = strlen (option);
  if (option_length > 5 && bcmp (option, ":key=", 5) == 0)
    {
      option_length -= 5;
      option += 5;
      next_option = index (option, ':');
      if (next_option != NULL)
	option_length = next_option - option;
    }
  else
    {
      option = CPP_DEFAULT_OBSCURE_KEY;
      option_length = sizeof (CPP_DEFAULT_OBSCURE_KEY) - 1;
    }

  key = (struct cpp_obscured_header_key *)
    xmalloc (sizeof (struct cpp_obscured_header_key));
  key->name = (char*) xmalloc (option_length + 1);
  bcopy (option, key->name, option_length);
  key->name[option_length] = '\0';
  key->next = opts->keys;
  opts->keys = key;

  option += option_length;
  return option;
}

static void
io_error (pfile, is_error, message, filename)
     cpp_reader *pfile;
     int is_error;
     char *message;
     char *filename;
{
  char *buffer = (char *) alloca (strlen (message) + strlen (filename) + 1);
  sprintf (buffer, message, filename);
  cpp_message_from_errno (pfile, is_error, buffer);
}

static void *
my_memmove (dest, src, len)
     void *dest;
     void *src;
     size_t len;
{
  size_t i;
  char *source = src;
  char *target = dest;
  if (source > target)
    {
      for (i = 0; i < len; ++i)
	target[i] = source[i];
    }
  else
    {
      for (i = len; i > 0; --i) /* careful -- i may be unsigned */
	target[i - 1] = source[i - 1];
    }
  return dest;
}
