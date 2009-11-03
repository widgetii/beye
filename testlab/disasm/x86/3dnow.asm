global prec_14bit
global prec_15bit
global prec_24bit

[bits 32]

prec_14bit:
femms
prefetch [0]
prefetchw [12]
movd mm0, [0]
pfrcp mm0, mm0
movq mm2, [4]
pfmul mm2, mm0
movntq [12], mm2
ret
prec_15bit:
movd mm0, [0]
pfrsqrt mm1, mm0
pfmul mm0, mm1
movq  [8], mm0
ret

prec_24bit:
movd mm0, [0]
pfrsqrt mm1, mm0
movq mm2, mm1
pfmul mm1, mm1
pfrsqit1 mm1, mm0
pfrcpit2 mm1, mm2
pfmul mm0, mm1
movd [8], mm0
nop
pfrcpit1 mm0, mm1
movq mm2, [12]
pfrcpit2 mm0, mm1
pfmul mm2, mm0
movntq [16], mm2
ret
pfadd mm0, [30]
pavgusb mm4, [38]
pswapd mm3, mm4
pf2id mm2, [40]
pfcmpge mm0, mm2
pfmax mm3, [60]
pfsubr mm1, mm3
pi2fd mm2, [0]
movntq [80], mm2
pf2iw mm7, mm0
pfnacc mm6, mm5
pfpnacc mm4, mm6
femms
ret
