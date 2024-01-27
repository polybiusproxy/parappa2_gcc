#objdump: -d --disassemble-zeroes
#name: align-2

# Test alignment of labels preceding automagic alignment.

.*: .*

Disassembly of section .*:

0+00 <dmalab-0x10>:
   0:	00000001 	.*
   4:	00000000 	.*
   8:	00000000 	.*
   c:	00000000 	.*

0+10 <dmalab>:
  10:	00 00 00 10 	dmacnt 0
  14:	00 00 00 00 

0+18 <.*>:
  18:	00 00 00 14 	mscal 0
  1c:	00 00 00 00 	vifnop

0+20 <.*>:
  20:	00 00 00 70 	dmaend 0
  24:	00 00 00 00 

0+28 <.*>:
  28:	00 00 00 00 	vifnop
  2c:	00 00 00 00 	vifnop
  30:	01 00 00 00 	[*]unknown[*]
  34:	00 00 00 00 	vifnop
  38:	00 00 00 00 	vifnop
  3c:	00 00 00 00 	vifnop

0+40 <giflab>:
  40:	00 00 00 00 	gifreglist regs={rgbaq},nloop=0
  44:	00 00 00 14 
  48:	01 00 00 00 
  4c:	00 00 00 00 

0+50 <endgif>:
  50:	00000001 	.*
  54:	00000000 	.*

0+58 <vulab>:
  58:	3c 03 00 80 	abs[.]xyz vf01xyz,vf00xyz 	nop
  5c:	fd 01 c1 01 
