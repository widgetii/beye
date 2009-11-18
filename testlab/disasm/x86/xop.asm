global svm
global prec_14bit
global prec_15bit
global prec_24bit
global unsorted
global amd_sse4a
global amd_sse5

[bits 64]
svm:
invlpga
invlpga [rax], ecx
skinit
skinit [eax]
vmload
vmload [rax]
vmload [rax]
vmrun
vmrun [rax]
vmsave
vmsave [eax]
vmsave [rax]


prec_14bit:
femms
prefetch [r8+r12*2]
prefetchw [12]
movd mm0, [rax*8+rdx+12]
pfrcp mm0, mm0
movq mm2, [r13*4+r11+0x12345678]
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
movntsd qword [r14], xmm9
movntss [4], xmm13
movntss dword [rax], xmm8

lzcnt ax, r10w
lzcnt r13w, word [r9]
lzcnt dx, [4]
lzcnt eax, r8d
lzcnt ecx, dword [r10*2+r11]
lzcnt r11d, [rax]
lzcnt eax, ebx
lzcnt ecx, dword [rcx*4+rdx+0x78563412]
lzcnt r15, [4]

popcnt r9w, bx
popcnt r10w, word [4]
popcnt r11w, [rax]
popcnt r8d, ebx
popcnt ecx, dword [r11]
popcnt edx, [r12]
popcnt eax, r14d
popcnt ecx, dword [r13]
popcnt edx, [4]
retn

amd_sse5:

vfrczpd xmm1, xmm2
vfrczpd xmm8, [0]
vfrczpd xmm9, dqword [0]
vfrczpd ymm1, ymm2
vfrczpd ymm1, [0]
vfrczpd ymm1, yword [0]

vfrczps xmm1, xmm9
vfrczps xmm1, [0]
vfrczps xmm8, dqword [0]
vfrczps ymm8, ymm9
vfrczps ymm1, [0]
vfrczps ymm10, yword [0]

vfrczsd xmm1, xmm2
vfrczsd xmm1, [rax*2+rbx+0x12]
vfrczsd xmm11, qword [rsi*4+rdi+0x12345678]

vfrczss xmm1, xmm2
vfrczss xmm1, [0]
vfrczss xmm1, dword [0]

vpblendvb xmm1, xmm2, xmm3, xmm4
vpblendvb xmm8, xmm9, [0], xmm10

vpcmov xmm1, xmm2, xmm3, xmm4
vpcmov xmm5, xmm6, xmm7, [0]
vpcmov xmm8, xmm9, xmm10, dqword [rax]
vpcmov xmm11, xmm12, [0], xmm13
vpcmov xmm14, xmm15, dqword [rax+r14*8+0x12345678], xmm4
vpcmov ymm1, ymm2, ymm3, ymm4
vpcmov ymm5, ymm6, ymm7, [0]
vpcmov ymm8, ymm9, ymm10, yword [rax]
vpcmov ymm11, ymm12, [0], ymm13
vpcmov ymm14, ymm15, yword [r13], ymm4

vpcomb xmm1, xmm4, xmm7, 0
vpcomb xmm2, xmm5, [rax], byte 1
vpcomb xmm3, xmm6, dqword [rcx*8+r11+0x12345678], 2

vpcomd xmm1, xmm4, xmm7, 3
vpcomd xmm2, xmm5, [4], byte 4
vpcomd xmm3, xmm6, dqword [0], 5

vpcomq xmm1, xmm4, xmm7, 6
vpcomq xmm2, xmm5, [0], byte 7
vpcomq xmm3, xmm6, dqword [0], 0

vpcomub xmm1, xmm4, xmm7, 1
vpcomub xmm2, xmm5, [0], byte 2
vpcomub xmm3, xmm6, dqword [0], 3

vpcomud xmm1, xmm4, xmm7, 4
vpcomud xmm2, xmm5, [0], byte 5
vpcomud xmm3, xmm6, dqword [0], 6

vpcomuq xmm1, xmm4, xmm7, 7
vpcomuq xmm2, xmm5, [0], byte 0
vpcomuq xmm3, xmm6, dqword [0], 1

vpcomuw xmm1, xmm4, xmm7, 2
vpcomuw xmm2, xmm5, [0], byte 3
vpcomuw xmm3, xmm6, dqword [0], 4

vpcomw xmm1, xmm4, xmm7, 5
vpcomw xmm2, xmm5, [0], byte 6
vpcomw xmm3, xmm6, dqword [0], 7

vphaddbd xmm1, xmm2
vphaddbd xmm1, [0]
vphaddbd xmm1, dqword [0]

vphaddbq xmm1, xmm2
vphaddbq xmm1, [0]
vphaddbq xmm1, dqword [0]

vphaddbw xmm8, xmm9
vphaddbw xmm8, [0]
vphaddbw xmm8, dqword [0]

vphadddq xmm1, xmm2
vphadddq xmm1, [0]
vphadddq xmm1, dqword [0]

vphaddubd xmm8, xmm9
vphaddubd xmm8, [0]
vphaddubd xmm8, dqword [0]

vphaddubq xmm1, xmm9
vphaddubq xmm1, [0]
vphaddubq xmm1, dqword [0]

vphaddubw xmm1, xmm9
vphaddubw xmm1, [0]
vphaddubw xmm1, dqword [0]

vphaddudq xmm9, xmm2
vphaddudq xmm9, [0]
vphaddudq xmm9, dqword [0]

vphadduwd xmm9, xmm2
vphadduwd xmm9, [0]
vphadduwd xmm9, dqword [0]

vphadduwq xmm1, xmm2
vphadduwq xmm1, [0]
vphadduwq xmm1, dqword [0]

vphaddwd xmm10, xmm2
vphaddwd xmm10, [0]
vphaddwd xmm10, dqword [0]

vphaddwq xmm1, xmm2
vphaddwq xmm1, [0]
vphaddwq xmm1, dqword [0]

vphsubbw xmm12, xmm2
vphsubbw xmm12, [0]
vphsubbw xmm12, dqword [0]

vphsubdq xmm1, xmm2
vphsubdq xmm1, [0]
vphsubdq xmm1, dqword [0]

vphsubwd xmm13, xmm2
vphsubwd xmm13, [0]
vphsubwd xmm13, dqword [0]

vpmacsdd xmm1, xmm2, xmm3, xmm4
vpmacsdd xmm5, xmm6, [0], xmm7
vpmacsdd xmm8, xmm9, dqword [0], xmm10

vpmacsdqh xmm1, xmm2, xmm3, xmm4
vpmacsdqh xmm5, xmm6, [rdx*4+rsi+0x0C], xmm7
vpmacsdqh xmm8, xmm9, dqword [rdx*2+rsi+0x12345678], xmm10

vpmacsdql xmm1, xmm2, xmm3, xmm4
vpmacsdql xmm5, xmm6, [rax+rdx+0x12345678], xmm7
vpmacsdql xmm8, xmm9, dqword [rax*4+rdx+0x12], xmm10

vpmacssdd xmm1, xmm2, xmm3, xmm4
vpmacssdd xmm5, xmm6, [rax+rdx+0x12345678], xmm7
vpmacssdd xmm8, xmm9, dqword [rax*4+rdx+0x12], xmm10

vpmacssdqh xmm1, xmm2, xmm3, xmm4
vpmacssdqh xmm5, xmm6, [rax+rdx+0x12345678], xmm7
vpmacssdqh xmm8, xmm9, dqword [rax*4+rdx+0x12], xmm10

vpmacssdql xmm1, xmm2, xmm3, xmm4
vpmacssdql xmm5, xmm6, [rax+rdx+0x12345678], xmm7
vpmacssdql xmm8, xmm9, dqword [rax*4+rdx+0x12], xmm10

vpmacsswd xmm1, xmm2, xmm3, xmm4
vpmacsswd xmm5, xmm6, [rax+rdx+0x12345678], xmm7
vpmacsswd xmm8, xmm9, dqword [rax*4+rdx+0x12], xmm10

vpmacssww xmm1, xmm2, xmm3, xmm4
vpmacssww xmm5, xmm6, [rax+rdx+0x12345678], xmm7
vpmacssww xmm8, xmm9, dqword [rax*4+rdx+0x12], xmm10

vpmacswd xmm1, xmm2, xmm3, xmm4
vpmacswd xmm5, xmm6, [rax+rdx+0x12345678], xmm7
vpmacswd xmm8, xmm9, dqword [rax*4+rdx+0x12], xmm10

vpmacsww xmm1, xmm2, xmm3, xmm4
vpmacsww xmm5, xmm6, [rax+rdx+0x12345678], xmm7
vpmacsww xmm8, xmm9, dqword [rax*4+rdx+0x12], xmm10

vpmadcsswd xmm1, xmm2, xmm3, xmm4
vpmadcsswd xmm5, xmm6, [rax+rdx+0x12345678], xmm7
vpmadcsswd xmm8, xmm9, dqword [rax*4+rdx+0x12], xmm10

vpmadcswd xmm1, xmm2, xmm3, xmm4
vpmadcswd xmm5, xmm6, [rax+rdx+0x12345678], xmm7
vpmadcswd xmm8, xmm9, dqword [rax*4+rdx+0x12], xmm10

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
