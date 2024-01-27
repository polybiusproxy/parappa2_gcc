#as:
#objdump: -dr --disassemble-zeroes
#name: dma-1

.*: +file format .*

Disassembly of section .vutext:

0+0000 <test.cnt.1>:
   0:	02 00 00 10 	dmacnt 2
   4:	00 00 00 00 

0+0008 .*:
   8:	00 00 00 00 	vifnop
   c:	00 00 00 00 	vifnop
  10:	00 00 00 00 	vifnop
  14:	00 00 00 00 	vifnop
  18:	00 00 00 00 	vifnop
  1c:	00 00 00 00 	vifnop

0+0020 <test.cnt.2>:
  20:	01 00 00 10 	dmacnt 1
  24:	00 00 00 00 

0+0028 .*:
  28:	00 00 00 00 	vifnop
  2c:	00 00 00 00 	vifnop
  30:	00 00 00 00 	vifnop
  34:	00 00 00 00 	vifnop
  38:	00 00 00 00 	vifnop
  3c:	00 00 00 00 	vifnop

0+0040 <test.next.3>:
  40:	02 00 00 20 	dmanext 2,60 <next1>
  44:	60 00 00 00 
			44: R_MIPS_DVP_27_S4	.vutext

0+0048 .*:
  48:	00 00 00 00 	vifnop
  4c:	00 00 00 00 	vifnop
  50:	00 00 00 00 	vifnop
  54:	00 00 00 00 	vifnop
  58:	00 00 00 00 	vifnop
  5c:	00 00 00 00 	vifnop

0+0060 <next1>:
  60:	01 00 00 20 	dmanext 1,80 <next2>
  64:	80 00 00 00 
			64: R_MIPS_DVP_27_S4	.vutext

0+0068 .*:
  68:	00 00 00 00 	vifnop
  6c:	00 00 00 00 	vifnop
  70:	00 00 00 00 	vifnop
  74:	00 00 00 00 	vifnop
  78:	00 00 00 00 	vifnop
  7c:	00 00 00 00 	vifnop

0+0080 <next2>:
  80:	02 00 00 30 	dmaref 2,a0 <ref1>
  84:	a0 00 00 00 
			84: R_MIPS_DVP_27_S4	.vutext

0+0088 .*:
  88:	00 00 00 00 	vifnop
  8c:	00 00 00 00 	vifnop

0+0090 <test.ref.6>:
  90:	01 00 00 30 	dmaref 1,b0 <ref2>
  94:	b0 00 00 00 
			94: R_MIPS_DVP_27_S4	.vutext

0+0098 .*:
  98:	00 00 00 00 	vifnop
  9c:	00 00 00 00 	vifnop

0+00a0 <ref1>:
  a0:	00 00 00 00 	vifnop
  a4:	00 00 00 00 	vifnop
  a8:	00 00 00 00 	vifnop
  ac:	00 00 00 00 	vifnop

0+00b0 <ref2>:
  b0:	00 00 00 00 	vifnop
  b4:	00 00 00 00 	vifnop
  b8:	00 00 00 00 	vifnop
  bc:	00 00 00 00 	vifnop

0+00c0 <test.refs.7>:
  c0:	02 00 00 40 	dmarefs 2,a0 <ref1>
  c4:	a0 00 00 00 
			c4: R_MIPS_DVP_27_S4	.vutext

0+00c8 .*:
  c8:	00 00 00 00 	vifnop
  cc:	00 00 00 00 	vifnop

0+00d0 <test.refs.8>:
  d0:	01 00 00 40 	dmarefs 1,b0 <ref2>
  d4:	b0 00 00 00 
			d4: R_MIPS_DVP_27_S4	.vutext

0+00d8 .*:
  d8:	00 00 00 00 	vifnop
  dc:	00 00 00 00 	vifnop

0+00e0 <test.refe.9>:
  e0:	02 00 00 00 	dmarefe 2,a0 <ref1>
  e4:	a0 00 00 00 
			e4: R_MIPS_DVP_27_S4	.vutext

0+00e8 .*:
  e8:	00 00 00 00 	vifnop
  ec:	00 00 00 00 	vifnop

0+00f0 <test.refe.10>:
  f0:	01 00 00 00 	dmarefe 1,b0 <ref2>
  f4:	b0 00 00 00 
			f4: R_MIPS_DVP_27_S4	.vutext

0+00f8 .*:
  f8:	00 00 00 00 	vifnop
  fc:	00 00 00 00 	vifnop

0+0100 <test.call.11>:
 100:	02 00 00 50 	dmacall 2,80 <next2>
 104:	80 00 00 00 
			104: R_MIPS_DVP_27_S4	.vutext

0+0108 .*:
 108:	00 00 00 00 	vifnop
 10c:	00 00 00 00 	vifnop
 110:	00 00 00 00 	vifnop
 114:	00 00 00 00 	vifnop
 118:	00 00 00 00 	vifnop
 11c:	00 00 00 00 	vifnop

0+0120 <test.call.12>:
 120:	01 00 00 50 	dmacall 1,80 <next2>
 124:	80 00 00 00 
			124: R_MIPS_DVP_27_S4	.vutext

0+0128 .*:
 128:	00 00 00 00 	vifnop
 12c:	00 00 00 00 	vifnop
 130:	00 00 00 00 	vifnop
 134:	00 00 00 00 	vifnop
 138:	00 00 00 00 	vifnop
 13c:	00 00 00 00 	vifnop

0+0140 <test.ret.13>:
 140:	02 00 00 60 	dmaret 2
 144:	00 00 00 00 

0+0148 .*:
 148:	00 00 00 00 	vifnop
 14c:	00 00 00 00 	vifnop
 150:	00 00 00 00 	vifnop
 154:	00 00 00 00 	vifnop
 158:	00 00 00 00 	vifnop
 15c:	00 00 00 00 	vifnop

0+0160 <test.ret.14>:
 160:	01 00 00 60 	dmaret 1
 164:	00 00 00 00 

0+0168 .*:
 168:	00 00 00 00 	vifnop
 16c:	00 00 00 00 	vifnop
 170:	00 00 00 00 	vifnop
 174:	00 00 00 00 	vifnop
 178:	00 00 00 00 	vifnop
 17c:	00 00 00 00 	vifnop

0+0180 <test.end.15>:
 180:	02 00 00 70 	dmaend 2
 184:	00 00 00 00 

0+0188 .*:
 188:	00 00 00 00 	vifnop
 18c:	00 00 00 00 	vifnop
 190:	00 00 00 00 	vifnop
 194:	00 00 00 00 	vifnop
 198:	00 00 00 00 	vifnop
 19c:	00 00 00 00 	vifnop

0+01a0 <test.end.16>:
 1a0:	01 00 00 70 	dmaend 1
 1a4:	00 00 00 00 

0+01a8 .*:
 1a8:	00 00 00 00 	vifnop
 1ac:	00 00 00 00 	vifnop
 1b0:	00 00 00 00 	vifnop
 1b4:	00 00 00 00 	vifnop
 1b8:	00 00 00 00 	vifnop
 1bc:	00 00 00 00 	vifnop
