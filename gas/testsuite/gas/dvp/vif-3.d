#as:
#objdump: -dr
#name: vif-3

.*: +file format .*

Disassembly of section .vutext:

0+0 <foo>:
   0:	01 01 00 01 	stcycl 1,1
   4:	00 00 01 60 	unpack s_32,0x0,1
   8:	00 00 00 00 
   c:	02 02 00 01 	stcycl 2,2
  10:	01 00 01 60 	unpack s_32,0x1,1
  14:	01 00 00 00 
  18:	01 01 00 01 	stcycl 1,1
  1c:	02 00 01 60 	unpack s_32,0x2,1
  20:	02 00 00 00 
  24:	01 01 00 01 	stcycl 1,1
  28:	03 80 01 70 	unpack\[mr\] s_32,0x3,1
  2c:	03 00 00 00 
  30:	01 02 00 01 	stcycl 2,1
  34:	04 80 02 70 	unpack\[mr\] s_32,0x4,2
  38:	04 00 00 00 
  3c:	02 01 00 01 	stcycl 1,2
  40:	05 80 01 70 	unpack\[mr\] s_32,0x5,1
  44:	05 00 00 00 
