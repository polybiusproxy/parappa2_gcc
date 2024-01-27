#as:
#objdump: -dr --disassemble-zeroes
#name: dma-2

.*: +file format .*

Disassembly of section .vutext:

0+0000 <test.cnt.1>:
   0:	02 00 00 10 	dmacnt 2
   4:	00 00 00 00 

0+0008.*
   8:	2a 00 00 03 	base 0x2a
   c:	2b 00 00 02 	offset 0x2b

0+0010 <test.cnt.2>:
  10:	01 00 00 10 	dmacnt 1
  14:	00 00 00 00 

0+0018.*
  18:	00 00 00 31 	stcol 1,2,3,4
  1c:	01 00 00 00 
  20:	02 00 00 00 
  24:	03 00 00 00 
  28:	04 00 00 00 
  2c:	00 00 00 00 	vifnop

0+0030 <test.next.3>:
  30:	02 00 00 20 	dmanext 2,50 <next1>
  34:	50 00 00 00 
			34: R_MIPS_DVP_27_S4	.vutext

0+0038.*
  38:	00 00 00 20 	stmask 0x12345678
  3c:	78 56 34 12 
  40:	00 00 00 00 	vifnop
  44:	00 00 00 00 	vifnop
  48:	00 00 00 00 	vifnop
  4c:	00 00 00 00 	vifnop

0+0050 <next1>:
  50:	01 00 00 20 	dmanext 1,70 <next2>
  54:	70 00 00 00 
			54: R_MIPS_DVP_27_S4	.vutext

0+0058.*
  58:	00 00 00 00 	vifnop
  5c:	00 00 00 30 	strow 1,2,3,4
  60:	01 00 00 00 
  64:	02 00 00 00 
  68:	03 00 00 00 
  6c:	04 00 00 00 

0+0070 <next2>:
  70:	02 00 00 30 	dmaref 2,80 <.*>
  74:	80 00 00 00 
			74: R_MIPS_DVP_27_S4	.vutext

0+0078.*
  78:	00 00 00 00 	vifnop
  7c:	00 00 00 00 	vifnop

0+0080 <test.ref.6>:
  80:	01 00 00 30 	dmaref 1,a0 <ref2>
  84:	a0 00 00 00 
			84: R_MIPS_DVP_27_S4	.vutext

0+0088 <ref1>:
  88:	00 00 00 00 	vifnop
  8c:	00 00 00 00 	vifnop
  90:	00 00 00 00 	vifnop
  94:	00 00 00 00 	vifnop
  98:	00 00 00 00 	vifnop
  9c:	00 00 00 00 	vifnop

0+00a0 <ref2>:
  a0:	00 00 00 00 	vifnop
  a4:	00 00 00 00 	vifnop
  a8:	00 00 00 00 	vifnop
  ac:	00 00 00 00 	vifnop
