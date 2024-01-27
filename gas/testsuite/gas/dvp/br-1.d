#as:
#objdump: -dr -m dvp:vu
#name: br-1

.*: +file format .*

Disassembly of section .vutext:

0* <.*foo>:
 *0:	01 00 00 40[ 	]*nop[ 	]*b 10 <foo1>
 *4:	ff 02 00 00[ 	]*
 *8:	3c 03 00 80[ 	]*nop[ 	]*nop
 *c:	ff 02 00 00[ 	]*

0*10 <.*foo1>:
  10:	fd 07 01 42[ 	]*nop[ 	]*bal vi01,0 <foo>
  14:	ff 02 00 00[ 	]*
  18:	3c 03 00 80[ 	]*nop[ 	]*nop
  1c:	ff 02 00 00[ 	]*
  20:	ff 07 00 40[ 	]*nop[ 	]*b 20 <foo1\+0x10>
  24:	ff 02 00 00[ 	]*
[ 	]*20: R_MIPS_DVP_11_PCREL[ 	]*bar1
  28:	3c 03 00 80[ 	]*nop[ 	]*nop
  2c:	ff 02 00 00[ 	]*
