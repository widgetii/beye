global svm
global prec_14bit
global prec_15bit
global prec_24bit
global unsorted
global amd_sse4a
global amd_sse5

[bits 32]
svm:
invlpga
invlpga [eax], ecx
invlpga [ax], ecx
skinit
skinit [eax]
vmload
vmload [eax]
vmload [ax]
vmrun
vmrun [eax]
vmrun [ax]
vmsave
vmsave [eax]
vmsave [ax]


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
ret
unsorted:
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

amd_sse4a:
extrq xmm0, 5, 4
extrq xmm6, 0, 7
extrq xmm2, xmm3
insertq xmm0, xmm1, 5, 4
insertq xmm5, xmm6, 0, 7
insertq xmm2, xmm3
movntsd [4], xmm1
movntsd qword [4], xmm5
movntss [4], xmm3
movntss dword [4], xmm7

lzcnt ax, bx
lzcnt cx, word [4]
lzcnt dx, [4]
lzcnt eax, ebx
lzcnt ecx, dword [4]
lzcnt edx, [4]
lzcnt eax, ebx
lzcnt ecx, dword [4]
lzcnt edx, [4]

popcnt ax, bx
popcnt cx, word [4]
popcnt dx, [4]
popcnt eax, ebx
popcnt ecx, dword [4]
popcnt edx, [4]
popcnt eax, ebx
popcnt ecx, dword [4]
popcnt edx, [4]
retn

amd_sse5:

vfrczpd xmm1, xmm2
vfrczpd xmm1, [0]
vfrczpd xmm1, dqword [0]
vfrczpd ymm1, ymm2
vfrczpd ymm1, [0]
vfrczpd ymm1, yword [0]

vfrczps xmm1, xmm2
vfrczps xmm1, [0]
vfrczps xmm1, dqword [0]
vfrczps ymm1, ymm2
vfrczps ymm1, [0]
vfrczps ymm1, yword [0]

vfrczsd xmm1, xmm2
vfrczsd xmm1, [0]
vfrczsd xmm1, qword [0]

vfrczss xmm1, xmm2
vfrczss xmm1, [0]
vfrczss xmm1, dword [0]

vpcmov xmm1, xmm2, xmm3, xmm4
vpcmov xmm1, xmm2, xmm3, [0]
vpcmov xmm1, xmm2, xmm3, dqword [0]
vpcmov xmm1, xmm2, [0], xmm4
vpcmov xmm1, xmm2, dqword [0], xmm4
vpcmov ymm1, ymm2, ymm3, ymm4
vpcmov ymm1, ymm2, ymm3, [0]
vpcmov ymm1, ymm2, ymm3, yword [0]
vpcmov ymm1, ymm2, [0], ymm4
vpcmov ymm1, ymm2, yword [0], ymm4

vpcomb xmm1, xmm4, xmm7, 5
vpcomb xmm2, xmm5, [0], byte 5
vpcomb xmm3, xmm6, dqword [0], 5

vpcomd xmm1, xmm4, xmm7, 5
vpcomd xmm2, xmm5, [0], byte 5
vpcomd xmm3, xmm6, dqword [0], 5

vpcomq xmm1, xmm4, xmm7, 5
vpcomq xmm2, xmm5, [0], byte 5
vpcomq xmm3, xmm6, dqword [0], 5

vpcomub xmm1, xmm4, xmm7, 5
vpcomub xmm2, xmm5, [0], byte 5
vpcomub xmm3, xmm6, dqword [0], 5

vpcomud xmm1, xmm4, xmm7, 5
vpcomud xmm2, xmm5, [0], byte 5
vpcomud xmm3, xmm6, dqword [0], 5

vpcomuq xmm1, xmm4, xmm7, 5
vpcomuq xmm2, xmm5, [0], byte 5
vpcomuq xmm3, xmm6, dqword [0], 5

vpcomuw xmm1, xmm4, xmm7, 5
vpcomuw xmm2, xmm5, [0], byte 5
vpcomuw xmm3, xmm6, dqword [0], 5

vpcomw xmm1, xmm4, xmm7, 5
vpcomw xmm2, xmm5, [0], byte 5
vpcomw xmm3, xmm6, dqword [0], 5

vphaddbd xmm1, xmm2
vphaddbd xmm1, [0]
vphaddbd xmm1, dqword [0]

vphaddbq xmm1, xmm2
vphaddbq xmm1, [0]
vphaddbq xmm1, dqword [0]

vphaddbw xmm1, xmm2
vphaddbw xmm1, [0]
vphaddbw xmm1, dqword [0]

vphadddq xmm1, xmm2
vphadddq xmm1, [0]
vphadddq xmm1, dqword [0]

vphaddubd xmm1, xmm2
vphaddubd xmm1, [0]
vphaddubd xmm1, dqword [0]

vphaddubq xmm1, xmm2
vphaddubq xmm1, [0]
vphaddubq xmm1, dqword [0]

vphaddubw xmm1, xmm2
vphaddubw xmm1, [0]
vphaddubw xmm1, dqword [0]

vphaddudq xmm1, xmm2
vphaddudq xmm1, [0]
vphaddudq xmm1, dqword [0]

vphadduwd xmm1, xmm2
vphadduwd xmm1, [0]
vphadduwd xmm1, dqword [0]

vphadduwq xmm1, xmm2
vphadduwq xmm1, [0]
vphadduwq xmm1, dqword [0]

vphaddwd xmm1, xmm2
vphaddwd xmm1, [0]
vphaddwd xmm1, dqword [0]

vphaddwq xmm1, xmm2
vphaddwq xmm1, [0]
vphaddwq xmm1, dqword [0]

vphsubbw xmm1, xmm2
vphsubbw xmm1, [0]
vphsubbw xmm1, dqword [0]

vphsubdq xmm1, xmm2
vphsubdq xmm1, [0]
vphsubdq xmm1, dqword [0]

vphsubwd xmm1, xmm2
vphsubwd xmm1, [0]
vphsubwd xmm1, dqword [0]

vpmacsdd xmm1, xmm4, xmm7, xmm3
vpmacsdd xmm2, xmm5, [0], xmm0
vpmacsdd xmm3, xmm6, dqword [0], xmm2

vpmacsdqh xmm1, xmm4, xmm7, xmm3
vpmacsdqh xmm2, xmm5, [0], xmm0
vpmacsdqh xmm3, xmm6, dqword [0], xmm2

vpmacsdql xmm1, xmm4, xmm7, xmm3
vpmacsdql xmm2, xmm5, [0], xmm0
vpmacsdql xmm3, xmm6, dqword [0], xmm2

vpmacssdd xmm1, xmm4, xmm7, xmm3
vpmacssdd xmm2, xmm5, [0], xmm0
vpmacssdd xmm3, xmm6, dqword [0], xmm2

vpmacssdqh xmm1, xmm4, xmm7, xmm3
vpmacssdqh xmm2, xmm5, [0], xmm0
vpmacssdqh xmm3, xmm6, dqword [0], xmm2

vpmacssdql xmm1, xmm4, xmm7, xmm3
vpmacssdql xmm2, xmm5, [0], xmm0
vpmacssdql xmm3, xmm6, dqword [0], xmm2

vpmacsswd xmm1, xmm4, xmm7, xmm3
vpmacsswd xmm2, xmm5, [0], xmm0
vpmacsswd xmm3, xmm6, dqword [0], xmm2

vpmacssww xmm1, xmm4, xmm7, xmm3
vpmacssww xmm2, xmm5, [0], xmm0
vpmacssww xmm3, xmm6, dqword [0], xmm2

vpmacswd xmm1, xmm4, xmm7, xmm3
vpmacswd xmm2, xmm5, [0], xmm0
vpmacswd xmm3, xmm6, dqword [0], xmm2

vpmacsww xmm1, xmm4, xmm7, xmm3
vpmacsww xmm2, xmm5, [0], xmm0
vpmacsww xmm3, xmm6, dqword [0], xmm2

vpmadcsswd xmm1, xmm4, xmm7, xmm3
vpmadcsswd xmm2, xmm5, [0], xmm0
vpmadcsswd xmm3, xmm6, dqword [0], xmm2

vpmadcswd xmm1, xmm4, xmm7, xmm3
vpmadcswd xmm2, xmm5, [0], xmm0
vpmadcswd xmm3, xmm6, dqword [0], xmm2

vpperm xmm1, xmm2, xmm3, xmm4
vpperm xmm1, xmm2, xmm3, [0]
vpperm xmm1, xmm2, xmm3, dqword [0]
vpperm xmm1, xmm2, [0], xmm4
vpperm xmm1, xmm2, dqword [0], xmm4

vprotb xmm1, xmm2, xmm3
vprotb xmm1, xmm2, [0]
vprotb xmm1, xmm2, dqword [0]
vprotb xmm1, [0], xmm3
vprotb xmm1, dqword [0], xmm3
vprotb xmm1, xmm2, byte 5
vprotb xmm1, [0], byte 5
vprotb xmm1, dqword [0], 5

vprotd xmm1, xmm2, xmm3
vprotd xmm1, xmm2, [0]
vprotd xmm1, xmm2, dqword [0]
vprotd xmm1, [0], xmm3
vprotd xmm1, dqword [0], xmm3
vprotd xmm1, xmm2, byte 5
vprotd xmm1, [0], byte 5
vprotd xmm1, dqword [0], 5

vprotq xmm1, xmm2, xmm3
vprotq xmm1, xmm2, [0]
vprotq xmm1, xmm2, dqword [0]
vprotq xmm1, [0], xmm3
vprotq xmm1, dqword [0], xmm3
vprotq xmm1, xmm2, byte 5
vprotq xmm1, [0], byte 5
vprotq xmm1, dqword [0], 5

vprotw xmm1, xmm2, xmm3
vprotw xmm1, xmm2, [0]
vprotw xmm1, xmm2, dqword [0]
vprotw xmm1, [0], xmm3
vprotw xmm1, dqword [0], xmm3
vprotw xmm1, xmm2, byte 5
vprotw xmm1, [0], byte 5
vprotw xmm1, dqword [0], 5

vpshab xmm1, xmm2, xmm3
vpshab xmm1, xmm2, [0]
vpshab xmm1, xmm2, dqword [0]
vpshab xmm1, [0], xmm3
vpshab xmm1, dqword [0], xmm3

vpshad xmm1, xmm2, xmm3
vpshad xmm1, xmm2, [0]
vpshad xmm1, xmm2, dqword [0]
vpshad xmm1, [0], xmm3
vpshad xmm1, dqword [0], xmm3

vpshaq xmm1, xmm2, xmm3
vpshaq xmm1, xmm2, [0]
vpshaq xmm1, xmm2, dqword [0]
vpshaq xmm1, [0], xmm3
vpshaq xmm1, dqword [0], xmm3

vpshaw xmm1, xmm2, xmm3
vpshaw xmm1, xmm2, [0]
vpshaw xmm1, xmm2, dqword [0]
vpshaw xmm1, [0], xmm3
vpshaw xmm1, dqword [0], xmm3

vpshlb xmm1, xmm2, xmm3
vpshlb xmm1, xmm2, [0]
vpshlb xmm1, xmm2, dqword [0]
vpshlb xmm1, [0], xmm3
vpshlb xmm1, dqword [0], xmm3

vpshld xmm1, xmm2, xmm3
vpshld xmm1, xmm2, [0]
vpshld xmm1, xmm2, dqword [0]
vpshld xmm1, [0], xmm3
vpshld xmm1, dqword [0], xmm3

vpshlq xmm1, xmm2, xmm3
vpshlq xmm1, xmm2, [0]
vpshlq xmm1, xmm2, dqword [0]
vpshlq xmm1, [0], xmm3
vpshlq xmm1, dqword [0], xmm3

vpshlw xmm1, xmm2, xmm3
vpshlw xmm1, xmm2, [0]
vpshlw xmm1, xmm2, dqword [0]
vpshlw xmm1, [0], xmm3
vpshlw xmm1, dqword [0], xmm3

vpcomltb xmm1, xmm2, xmm3
vpcomleb xmm1, xmm2, xmm3
vpcomgtb xmm1, xmm2, xmm3
vpcomgeb xmm1, xmm2, xmm3
vpcomeqb xmm1, xmm2, xmm3
vpcomneqb xmm1, xmm2, xmm3
vpcomneb xmm1, xmm2, xmm3
vpcomfalseb xmm1, xmm2, xmm3
vpcomtrueb xmm1, xmm2, xmm3

vpcomltuw xmm1, xmm2, xmm3
vpcomleuw xmm1, xmm2, xmm3
vpcomgtuw xmm1, xmm2, xmm3
vpcomgeuw xmm1, xmm2, xmm3
vpcomequw xmm1, xmm2, xmm3
vpcomnequw xmm1, xmm2, xmm3
vpcomneuw xmm1, xmm2, xmm3
vpcomfalseuw xmm1, xmm2, xmm3
vpcomtrueuw xmm1, xmm2, xmm3

vfmaddpd xmm1, xmm2, xmm3, xmm4
vfmaddpd xmm0, xmm1, xmm2, xmm3
vfmaddpd xmm0, xmm1, [eax], xmm3
vfmaddpd xmm0, xmm1, dqword [eax], xmm3
vfmaddpd xmm0, xmm1, xmm2, [eax]
vfmaddpd xmm0, xmm1, xmm2, dqword [eax]
vfmaddpd ymm0, ymm1, ymm2, ymm3
vfmaddpd ymm0, ymm1, [eax], ymm3
vfmaddpd ymm0, ymm1, yword [eax], ymm3
vfmaddpd ymm0, ymm1, ymm2, [eax]
vfmaddpd ymm0, ymm1, ymm2, yword [eax]

vfmaddps xmm0, xmm1, xmm2, xmm3
vfmaddps xmm0, xmm1, dqword [eax], xmm3
vfmaddps xmm0, xmm1, xmm2, dqword [eax]
vfmaddps ymm0, ymm1, ymm2, ymm3
vfmaddps ymm0, ymm1, yword [eax], ymm3
vfmaddps ymm0, ymm1, ymm2, yword [eax]

vfmaddsd xmm0, xmm1, xmm2, xmm3
vfmaddsd xmm0, xmm1, [eax], xmm3
vfmaddsd xmm0, xmm1, qword [eax], xmm3
vfmaddsd xmm0, xmm1, xmm2, [eax]
vfmaddsd xmm0, xmm1, xmm2, qword [eax]

vfmaddss xmm0, xmm1, xmm2, xmm3
vfmaddss xmm0, xmm1, dword [eax], xmm3
vfmaddss xmm0, xmm1, xmm2, dword [eax]

vfmaddsubpd xmm0, xmm1, xmm2, xmm3
vfmaddsubpd xmm0, xmm1, dqword [eax], xmm3
vfmaddsubpd xmm0, xmm1, xmm2, dqword [eax]
vfmaddsubpd ymm0, ymm1, ymm2, ymm3
vfmaddsubpd ymm0, ymm1, yword [eax], ymm3
vfmaddsubpd ymm0, ymm1, ymm2, yword [eax]

vfmaddsubps xmm0, xmm1, xmm2, xmm3
vfmaddsubps xmm0, xmm1, dqword [eax], xmm3
vfmaddsubps xmm0, xmm1, xmm2, dqword [eax]
vfmaddsubps ymm0, ymm1, ymm2, ymm3
vfmaddsubps ymm0, ymm1, yword [eax], ymm3
vfmaddsubps ymm0, ymm1, ymm2, yword [eax]

vfmsubpd xmm0, xmm1, xmm2, xmm3
vfmsubpd xmm0, xmm1, dqword [eax], xmm3
vfmsubpd xmm0, xmm1, xmm2, dqword [eax]
vfmsubpd ymm0, ymm1, ymm2, ymm3
vfmsubpd ymm0, ymm1, yword [eax], ymm3
vfmsubpd ymm0, ymm1, ymm2, yword [eax]

vfmsubps xmm0, xmm1, xmm2, xmm3
vfmsubps xmm0, xmm1, dqword [eax], xmm3
vfmsubps xmm0, xmm1, xmm2, dqword [eax]
vfmsubps ymm0, ymm1, ymm2, ymm3
vfmsubps ymm0, ymm1, yword [eax], ymm3
vfmsubps ymm0, ymm1, ymm2, yword [eax]

vfmsubsd xmm0, xmm1, xmm2, xmm3
vfmsubsd xmm0, xmm1, qword [eax], xmm3
vfmsubsd xmm0, xmm1, xmm2, qword [eax]

vfmsubss xmm0, xmm1, xmm2, xmm3
vfmsubss xmm0, xmm1, dword [eax], xmm3
vfmsubss xmm0, xmm1, xmm2, dword [eax]

vfnmaddpd xmm0, xmm1, xmm2, xmm3
vfnmaddpd xmm0, xmm1, dqword [eax], xmm3
vfnmaddpd xmm0, xmm1, xmm2, dqword [eax]
vfnmaddpd ymm0, ymm1, ymm2, ymm3
vfnmaddpd ymm0, ymm1, yword [eax], ymm3
vfnmaddpd ymm0, ymm1, ymm2, yword [eax]

vfnmaddps xmm0, xmm1, xmm2, xmm3
vfnmaddps xmm0, xmm1, dqword [eax], xmm3
vfnmaddps xmm0, xmm1, xmm2, dqword [eax]
vfnmaddps ymm0, ymm1, ymm2, ymm3
vfnmaddps ymm0, ymm1, yword [eax], ymm3
vfnmaddps ymm0, ymm1, ymm2, yword [eax]

vfnmaddsd xmm0, xmm1, xmm2, xmm3
vfnmaddsd xmm0, xmm1, qword [eax], xmm3
vfnmaddsd xmm0, xmm1, xmm2, qword [eax]

vfnmaddss xmm0, xmm1, xmm2, xmm3
vfnmaddss xmm0, xmm1, dword [eax], xmm3
vfnmaddss xmm0, xmm1, xmm2, dword [eax]

vfnmsubpd xmm0, xmm1, xmm2, xmm3
vfnmsubpd xmm0, xmm1, dqword [eax], xmm3
vfnmsubpd xmm0, xmm1, xmm2, dqword [eax]
vfnmsubpd ymm0, ymm1, ymm2, ymm3
vfnmsubpd ymm0, ymm1, yword [eax], ymm3
vfnmsubpd ymm0, ymm1, ymm2, yword [eax]

vfnmsubps xmm0, xmm1, xmm2, xmm3
vfnmsubps xmm0, xmm1, dqword [eax], xmm3
vfnmsubps xmm0, xmm1, xmm2, dqword [eax]
vfnmsubps ymm0, ymm1, ymm2, ymm3
vfnmsubps ymm0, ymm1, yword [eax], ymm3
vfnmsubps ymm0, ymm1, ymm2, yword [eax]

vfnmsubsd xmm0, xmm1, xmm2, xmm3
vfnmsubsd xmm0, xmm1, qword [eax], xmm3
vfnmsubsd xmm0, xmm1, xmm2, qword [eax]

vfnmsubss xmm0, xmm1, xmm2, xmm3
vfnmsubss xmm0, xmm1, dword [eax], xmm3
vfnmsubss xmm0, xmm1, xmm2, dword [eax]

retn
