#as:
#objdump: -d
#name: mpg-2

.*: +file format .*

Disassembly of section .vutext:

0+0000 .*:
   0:	00 00 00 00 	vifnop
   4:	80 00 02 4a 	mpg 0x80,2

0+0008 .*label1.*:
   8:	3c 03 00 80 	nop 	nop
   c:	ff 02 00 00 

0+0010 .*label2.*:
  10:	3c 03 00 80 	nop 	nop
  14:	ff 02 00 00 

0+0018 .*:
  18:	80 00 00 14 	mscal 0x80
  1c:	81 00 01 4a 	mpg 0x81,1

0+0020 .*:
  20:	3c 03 00 80 	nop 	nop
  24:	ff 02 00 00 

0+0028 .*:
  28:	80 00 00 14 	mscal 0x80
  2c:	81 00 01 4a 	mpg 0x81,1

0+0030 .*:
  30:	3c 03 00 80 	nop 	nop
  34:	ff 02 00 00 

0+0038 .*:
  38:	80 00 00 14 	mscal 0x80
  3c:	00 00 00 00 	vifnop
