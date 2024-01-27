#as:
#objdump: -dr
#name: br-2

.*: +file format .*

Disassembly of section .vutext:

0* <.*foo>:
   0:	00 00 00 00 	vifnop
   4:	00 00 02 4a 	mpg 0x0,2

0+0008 <.*foo1>:
   8:	ff 00 00 40 	nop 	b 808 <.*>
   c:	ff 02 00 00 
  10:	3c 03 00 80 	nop 	nop
  14:	ff 02 00 00 

0+0018 .*:
  18:	00 00 00 00 	vifnop
  1c:	00 01 02 4a 	mpg 0x100,2

0+0020 <.*foo2>:
  20:	ff 06 00 40 	nop 	b f+f820 <.*>
  24:	ff 02 00 00 
  28:	3c 03 00 80 	nop 	nop
  2c:	ff 02 00 00 
