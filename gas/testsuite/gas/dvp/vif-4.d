#as:
#objdump: -dr
#name: vif-4

.*: +file format .*

Disassembly of section .vutext:

0+0 <foo>:
   0:	00 00 00 20 	stmask 0x55555555
   4:	55 55 55 55 
   8:	00 00 00 30 	strow 0xffff,0xffff,0xffff,0xffff
   c:	ff ff 00 00 
  10:	ff ff 00 00 
  14:	ff ff 00 00 
  18:	ff ff 00 00 
  1c:	00 00 00 01 	stcycl 0,0
  20:	00 00 00 7a 	unpack\[m\] v3_8,0x0,0x100
  24:	00 00 00 7a 	unpack\[m\] v3_8,0x0,0x100
  28:	00 40 00 01 	stcycl 0x40,0
  2c:	00 00 c0 7a 	unpack\[m\] v3_8,0x0,0xc0
