/* This program is intended to be started outside of gdb, and then
   attached to by gdb.  Thus, it simply spins in a loop.  The loop
   is exited when & if the variable 'should_exit' is non-zero.  (It
   is initialized to zero in this program, so the loop will never
   exit unless/until gdb sets the variable to non-zero.)
   */
#include <stdio.h>
#include <stdlib.h>

int  should_exit = 0;

main ()
{
  int  local_i = 0;

  sleep( 10 ); /* System call causes register fetch to fail */
               /* This is a known HPUX "feature"            */
  while (! should_exit)
    {
      local_i++;
    }
  return (0);
}
