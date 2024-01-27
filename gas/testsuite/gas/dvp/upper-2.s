; Upper insn flag bit tests, and misc. other tests.

	.vu
foo:
ABS.xyz  	VF01xyz , VF00xyz                                 NOP
ADD[e].xyz  	VF02xyz , VF00xyz , VF01xyz                       NOP
ADDi[m].xyz  	VF02xyz , VF00xyz , I                             NOP
ADDq[d].xyz  	VF02xyz , VF00xyz , Q                             NOP
ADDx[t].xyz  	VF02xyz , VF00xyz , VF01x                         NOP
ADDA.xyz 	ACCxyz ,  VF00xyz ,	VF01xyz                   NOP
ADDAi[E].xyz 	ACCxyz ,  VF00xyz , I                             NOP
ADDAq[M].xyz 	ACCxyz ,  VF00xyz , Q                             NOP
ADDAx[D].xyz 	ACCxyz ,  VF00xyz , VF01x                         NOP
CLIPw[T].xyz	VF01xyz , VF00w                                   NOP
FTOI0[em].xyz 	VF01xyz , VF00xyz                                 NOP
FTOI4[mt].xyz 	VF01xyz , VF00xyz                                 NOP
FTOI12[md].xyz 	VF01xyz , VF00xyz                                 NOP
FTOI15[dt].xyz 	VF01xyz , VF00xyz                                 NOP
ITOF0[emd].xyz 	VF01xyz , VF00xyz                                 NOP
ITOF4[emt].xyz 	VF01xyz , VF00xyz                                 NOP
ITOF12[emdt].xyz 	VF01xyz , VF00xyz                         NOP
ITOF15[TDME].xyz 	VF01xyz , VF00xyz                         NOP

; Test having a floating point immediate value.

NOP LOI 1.5

; Test specifying none of xyzw -> all of xyzw.

	abs vf00, vf01  nop
