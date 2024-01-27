#as:
#objdump: -dr --disassemble-zeroes
#name: relax-1

.*: +file format .*

Disassembly of section .vutext:

0+0 <foo>:
 *0:	02 00 00 10 	dmacnt 2
 *4:	00 00 00 00 

0+8 .*:
 *8:	00 00 00 00 	vifnop
 *c:	02 00 00 50 	direct 2

0+10 .*:
 *10:	01 80 00 00 	gifpacked regs={a_d},nloop=1,eop
 *14:	00 00 00 10 
 *18:	0e 00 00 00 
 *1c:	00 00 00 00 
 *20:	00 00 00 00 
 *24:	4c 00 00 00 
 *28:	00 00 00 00 
 *2c:	00 00 0a 00 

0+0030 <dot_org_test>:
.*
.*
.*
.*

0+40 .*:
 *40:	00 00 00 00 	vifnop
 *44:	14 00 01 4a 	mpg 0x14,1

0+48 .*:
 *48:	3c 03 00 80 	nop 	nop
 *4c:	ff 02 00 00 
