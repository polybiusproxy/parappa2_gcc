#as:
#objdump: -dr -m dvp:vu
#name: lower-2

.*: +file format .*

Disassembly of section .vutext:

0+0000.*:
   0:	3c 03 00 80 	nop 	nop
   4:	ff 02 00 00 

0+0008 <iaddiu_label>:
   8:	01 00 01 10 	nop 	iaddiu vi01,vi00,1
   c:	ff 02 00 00 
			8: R_MIPS_DVP_U15_S3	.vutext

0+0010 <ilw_label>:
  10:	10 00 01 09 	nop 	ilw.x vi01,16\(vi00\)x
  14:	ff 02 00 00 
  18:	10 00 81 0a 	nop 	isw.y vi01,16\(vi00\)y
  1c:	ff 02 00 00 
