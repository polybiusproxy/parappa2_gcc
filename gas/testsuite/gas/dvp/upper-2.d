#as:
#objdump: -dr -m dvp:vu
#name: upper-2

.*: +file format .*

Disassembly of section .vutext:

0* <.*foo>:
   0:	3c 03 00 80[ 	]*abs.xyz vf01xyz,vf00xyz[ 	]*nop
   4:	fd 01 c1 01 
   8:	3c 03 00 80[ 	]*add\[e\].xyz vf02xyz,vf00xyz,vf01xyz[ 	]*nop
   c:	a8 00 c1 41 
  10:	3c 03 00 80[ 	]*addi\[m\].xyz vf02xyz,vf00xyz,i[ 	]*nop
  14:	a2 00 c0 21 
  18:	3c 03 00 80[ 	]*addq\[d\].xyz vf02xyz,vf00xyz,q[ 	]*nop
  1c:	a0 00 c0 11 
  20:	3c 03 00 80[ 	]*addx\[t\].xyz vf02xyz,vf00xyz,vf01x[ 	]*nop
  24:	80 00 c1 09 
  28:	3c 03 00 80[ 	]*adda.xyz accxyz,vf00xyz,vf01xyz[ 	]*nop
  2c:	bc 02 c1 01 
  30:	3c 03 00 80[ 	]*addai\[e\].xyz accxyz,vf00xyz,i[ 	]*nop
  34:	3e 02 c0 41 
  38:	3c 03 00 80[ 	]*addaq\[m\].xyz accxyz,vf00xyz,q[ 	]*nop
  3c:	3c 02 c0 21 
  40:	3c 03 00 80[ 	]*addax\[d\].xyz accxyz,vf00xyz,vf01x[ 	]*nop
  44:	3c 00 c1 11 
  48:	3c 03 00 80[ 	]*clipw\[\t\].xyz vf01xyz,vf00w[ 	]*nop
  4c:	ff 09 c0 09 
  50:	3c 03 00 80[ 	]*ftoi0\[em\].xyz vf01xyz,vf00xyz[ 	]*nop
  54:	7c 01 c1 61 
  58:	3c 03 00 80[ 	]*ftoi4\[mt\].xyz vf01xyz,vf00xyz[ 	]*nop
  5c:	7d 01 c1 29 
  60:	3c 03 00 80[ 	]*ftoi12\[md\].xyz vf01xyz,vf00xyz[ 	]*nop
  64:	7e 01 c1 31 
  68:	3c 03 00 80[ 	]*ftoi15\[dt\].xyz vf01xyz,vf00xyz[ 	]*nop
  6c:	7f 01 c1 19 
  70:	3c 03 00 80[ 	]*itof0\[emd].xyz vf01xyz,vf00xyz[ 	]*nop
  74:	3c 01 c1 71 
  78:	3c 03 00 80[ 	]*itof4\[emt\].xyz vf01xyz,vf00xyz[ 	]*nop
  7c:	3d 01 c1 69 
  80:	3c 03 00 80[ 	]*itof12\[emdt\].xyz vf01xyz,vf00xyz[ 	]*nop
  84:	3e 01 c1 79 
  88:	3c 03 00 80[ 	]*itof15\[emdt\].xyz vf01xyz,vf00xyz[ 	]*nop
  8c:	3f 01 c1 79 
  90:	00 00 c0 3f[ 	]*nop\[i\][ 	]*loi 1[.]5
  94:	ff 02 00 80 
  98:	3c 03 00 80[ 	]*abs.xyzw vf00xyzw,vf01xyzw[ 	]*nop
  9c:	fd 09 e0 01 
