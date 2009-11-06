global cpu_asm
global vmx
global smx
global simd1
global simd2
global sse_width
global sse3
global ssse3
global sse4
global aes
global avx
global fma

[bits 32]
cpu_asm:
enter 10, 12
pause
jp near label
js near label
salc
sahf
pusha
push 128
push byte 127
push word 128
push dword 128
pushad
popaw
popad

mov ax,[eax+ebx+ecx-eax]
mov ax,[eax+ecx+ebx-eax]
lea edi,[edi*8+eax+label]
lea edi,[eax+edi*8+label]
mov eax,[eax*2]
mov eax,[nosplit eax*2]
mov eax,[esi+ebp]
mov eax,[ebp+esi]
mov eax,[esi*1+ebp]
mov eax,[ebp*1+esi]
mov eax,[byte eax]
mov eax,[dword eax]
xor eax, [ebp+4*ecx   ]
xor ebx, [ebp+4*ecx+ 4]
xor esi, [ebp+4*ecx+ 8]
xor edi, [ebp+4*ecx+12]

mov al, 0
mov byte cl, 0
mov bl, byte 0
mov byte dh, byte 0

mov byte [0], 1
mov [1], word 2
mov dword [2], dword 3

mov eax, 1
mov dword eax, 2
mov eax, dword 4

mov dword eax, dword 8
mov bx, 1h
mov cr0, eax
mov cr2, ebx
mov cr4, edx
db 0x0F, 0x24, 0x00 ; mov eax, tr0
mov dr1, edx
mov eax, dr7
mov [0], ds
mov word [8], es
mov ax, cs
mov eax, ss
mov gs, ax
mov fs, eax
mov es, [cs:0]
mov ds, word [4]
mov word ds, [ss:16]

mov [0], al
mov [0], bl
mov [1], al
mov [1], bl
mov ecx, edx
movsx ax, [ecx]

movzx ebx, word [eax]
movzx ecx, byte [ebx]
fnstenv [es:ecx+5]
nop

push cs
push word cs
push dword cs ; NASM unsupported
push ds
push es
push fs
push gs
pop ds
pop es
pop fs
pop gs
xchg al, bl
xchg al, [0]
xchg [0], al
xchg ax, bx
xchg cx, ax
xchg [0], ax
xchg [0], cx
xchg cx, [0]
xchg eax, edx
xchg ebx, eax
xchg ecx, ebx
xchg [0], ecx
xchg eax, [0]
in al, 55
in ax, 99
in eax, 100
in al, dx
in ax, dx
in eax, dx
insb
insw
outsb
outsw
out 55, al
out 66, ax
out 77, eax
out dx, al
out dx, ax
out dx, eax
lea bx, [5]
lea ebx, [32]
lds si, [0]
lds ax, [1]
lds ax,[1]
lds ax,word [1]
lds ax,dword [1]
lds eax,[1]
lds eax,word [1]
lds eax,dword [1]


les di, [5]
lds eax, [7]
les ebx, [9]
lss esp, [11]
lfs ecx, [13]
lgs edx, [15]

;; TODO: add arith stuff
add ax,bx
mul cx
mul bl
mul bx
mul ebx
imul si,bp,2
sub bx,di
div ah
idiv al
idiv byte [bx]
idiv word [bx+si]
idiv dword [bp]
neg cx
nop
idiv al
idiv ax
idiv eax
nop
idiv byte [word 0]
idiv byte [dword 0xFFFFFFFF]
idiv byte [0]
idiv dword [es:dword 5]
idiv dword [byte cs:5]
idiv word [ss:dword edi+5]
nop
not eax
and eax, 3584
or  ebx, 35
xor ecx, strict dword 3584
or  edx, strict dword 35
xor si,  strict word 3584
and edi, strict byte 35
or  ebp, 3584
xor esp, 35
and eax, strict dword 3584
or  ebx, strict dword 35
xor cx,  strict word 3584
and edx, strict byte 35

imul eax, 4
aad
aam
aad 5
aam 10
shl al, 5
shl bl, 1
shl cl, cl
shr ax, 5
shr bx, 1
shr cx, cl
shld ax, bx, 5
shrd cx, dx, cl
shld ecx, edx, 10
shld eax, ebx, cl
retn
retf
retn 8
retf 16
setc al
setc [0]
;; TODO: add bit manip
rol  ax,cl
ror  bx,3
rcl  cx,2
rcr  dx,1
sar  si,cl
shl  ah,4
shr  dh,cl
bsf  ax,dx
bsr  cx,bx
bt   si,di
btr  sp,bp
bts  ax,dx
bsf  dx,[bx+si]
bsr  bx,[bx+si]
bt   [bx+si],di
btr  [bx+si],bp
bts  [bx+si],dx
bswap ecx
lock cmpxchg [bx+si],dx
lock cmpxchg8b qword [bx+si]
xadd [bx+si],di
cpuid
rdpmc
pushf
int 10
popfd
iret
bound sp,word [4]
arpl [bx+si],ax
clts
lar  ax,[bx+si]
lgdt [bx+si]
lidt [bx+si]
lldt [bx+si]
lsl  ax,[bx+si]
ltr  [bx+si]
sgdt [bx+si]
sidt [bx+si]
sldt [bx+si]
smsw [bx+si]
str  [bx+si]
verr [bx+si]
verw [bx+si]
invd
invlpg [bx+si]
wbinvd
rsm
rdmsr
rdtsc
wrmsr
;; TODO: add bound
;; TODO: add protection control
fxsave [0]
fsetpm
fld dword [0]
fld qword [4]
fld tword [16]
fld st2
fstp dword [0]
fstp st4
fild word [0]
fild dword [4]
fild qword [8]
fbld [100]
fbld tword [10]
fst dword [1]
fst qword [8]
fst st1
fxch
fxch st1
fxch st0, st2
fxch st2, st0
fcom dword [0]
fcom qword [8]
fcom st1
fucom st2
fucomp st3
fucompp
fcos
fprem1
fsin
fsincos
fcom st0, st0
fucom st7
fucomp st0, st5
fadd dword [10]
fadd qword [5]
fadd st0
fadd st0, st5
fadd to st7
fadd st6, st0
faddp ;NASM unsupported
faddp st2
faddp st5, st0
fiadd word [10]
fucomi st2
fcmove st0, st6
fcmovu st0, st5
fisub dword [4]
fldcw [0]
fnstcw [4]
fstcw word [4]
fnstsw [8]
fnstsw ax
fstsw word [0]
fstsw ax
ffree st1
ffreep st0 ;NASM unsupported
        fsub st1
        fsubp st1
        fsubr st1
        fsubrp st1
        fdiv st1
        fdivp st1
        fdivr st1
        fdivrp st1
fxrstor [0]
jc short label
jc label
label:
jz label
jz near label
loop label
jcxz label
jecxz label
call label
call [label]
call dword [label]
;jmp label
jmp short label
jmp near label
jmp far [label]
jmp far dword 0x1234:0x56789ABC
call far word [label]
loop short label
jcxz short label
jecxz short label
repne lodsw
repnz lodsd
rep stosb
repe cmpsb
repz movsb

push si
push esi

pop edi
leave
ret

foo equ 1:2
jmp far[foo]
mov ax,[foo]
push dword [foo]
mov ax,foo

mov ax, 'abcd'
mov ax, 0x1ffff
mov eax, 0x111111111
syscall
sysret
jmp 0x1234:0x56789ABC
call dword 0x1234:0x56789ABC
call far [ss:0]

rep gs movsd
fs rep movsd
leave
retn


vmx:
db 0x66, 0x0F, 0x38, 0x80, 0x3E ;invept edi, qword [esi]
db 0x66, 0x0F, 0x38, 0x81, 0xA  ;invvpid ecx, qword [edx]
vmcall
vmclear [12]
vmlaunch
vmresume
vmptrld [8]
vmptrst [eax]
vmread [ebx], ecx
vmwrite ebp, [ebp]
vmxoff
vmxon [esi*4+edi+400]

smx:
enter 10,0
mov eax, 0
db 0x0F, 0x37 ; getsec
leave
retn

simd1:
enter 10,0
prefetcht0 [32]
prefetcht1 [16]
prefetcht2 [8]
prefetchnta [0]

fxsave [eax+ebx]

movntps [0], xmm4
movntps dqword [0], xmm5
movntq [8], mm6
movntq qword [8], mm7
movss xmm0, [0]
movss xmm1, dword [8]
movss [0], xmm2
movss dword [8], xmm3
pcmpeqb xmm3, xmm4
pcmpgtw mm0, mm2

fxrstor [eax*2+edi]

lfence
mfence
sfence
leave
retn

simd2:
enter 20,0
pextrw ebx, mm5, 0
pextrw ecx, xmm0, 1

pinsrw mm3, esi, 5
pinsrw mm3, [0], 4

pinsrw xmm1, eax, 3
pinsrw xmm1, [0], 2

movmskpd edx, xmm7
movmskps eax, xmm1

pmovmskb edi, mm0
pmovmskb esi, xmm1

cvtdq2pd xmm5, xmm4
cvtdq2pd xmm3, [4]
cvtdq2pd xmm2, qword [8]

cvtdq2ps xmm1, xmm2
cvtdq2ps xmm4, [4]
cvtdq2ps xmm5, dqword [8]

cvtpd2dq xmm0, xmm1
cvtpd2dq xmm2, [4]
cvtpd2dq xmm3, dqword [8]

cvtpd2pi mm4, xmm5
cvtpd2pi mm6, [4]
cvtpd2pi mm7, dqword [8]

cvtpd2ps xmm1, xmm2
cvtpd2ps xmm3, [4]
cvtpd2ps xmm4, dqword [8]

cvtpi2pd xmm5, mm6
cvtpi2pd xmm7, [4]
cvtpi2pd xmm0, qword [0]

cvtpi2ps xmm2, mm3
cvtpi2ps xmm4, [4]
cvtpi2ps xmm5, qword [0]

cvtps2dq xmm6, xmm7
cvtps2dq xmm0, [4]
cvtps2dq xmm1, dqword [8]

cvtps2pd xmm2, xmm3
cvtps2pd xmm4, [4]
cvtps2pd xmm5, qword [0]

cvtps2pi mm6, xmm7
cvtps2pi mm0, [4]
cvtps2pi mm1, qword [0]

cvtsd2si edx, xmm0
cvtsd2si eax, [4]
cvtsd2si ebx, qword [0]

cvtsd2ss xmm1, xmm2
cvtsd2ss xmm3, [4]
cvtsd2ss xmm4, qword [0]

cvtsi2sd xmm5, eax
cvtsi2sd xmm6, [4]
cvtsi2sd xmm7, dword [0]

cvtsi2ss xmm0, edx
cvtsi2ss xmm1, [4]
cvtsi2ss xmm2, dword [0]

cvtss2sd xmm3, xmm4
cvtss2sd xmm5, [4]
cvtss2sd xmm6, dword [0]

cvtss2si ebx, xmm7
cvtss2si ecx, [4]
cvtss2si eax, dword [0]

cvttpd2pi mm0, xmm1
cvttpd2pi mm2, [4]
cvttpd2pi mm3, dqword [8]

cvttpd2dq xmm4, xmm5
cvttpd2dq xmm6, [4]
cvttpd2dq xmm7, dqword [8]

cvttps2dq xmm0, xmm1
cvttps2dq xmm2, [4]
cvttps2dq xmm3, dqword [8]

cvttps2pi mm4, xmm5
cvttps2pi mm6, [4]
cvttps2pi mm7, qword [8]

cvttsd2si ecx, xmm0
cvttsd2si ebx, [4]
cvttsd2si edi, qword [8]

cvttss2si esi, xmm3
cvttss2si ebp, [4]
cvttss2si eax, dword [8]

cmpss xmm0, [eax], 0
cmpss xmm0, [es:eax], 0

cmpsd xmm0, [eax], 0
cmpsd xmm0, [es:eax], 0
leave
retn
sse_width:
enter 40,2
cpu sse2

xsave [4]
xrstor [8]
xgetbv
xsetbv


addpd xmm1, xmm2
addpd xmm1, dqword [ebx]

addps xmm1, xmm2
addps xmm1, dqword [ebx]

addsd xmm1, xmm2
addsd xmm1, qword [ebx]

addss xmm1, xmm2
addss xmm1, dword [ebx]

addsubpd xmm1, xmm2
addsubpd xmm1, dqword [ebx]

addsubps xmm1, xmm2
addsubps xmm1, dqword [ebx]

andnpd xmm1, xmm2
andnpd xmm1, dqword [ebx]

andnps xmm1, xmm2
andnps xmm1, dqword [ebx]

andpd xmm1, xmm2
andpd xmm1, dqword [ebx]

andps xmm1, xmm2
andps xmm1, dqword [ebx]

cmppd xmm1, xmm2, 0
cmppd xmm1, dqword [ebx], 0
cmpeqpd xmm1, xmm2
cmpeqpd xmm1, dqword [ebx]

cmpps xmm1, xmm2, 0
cmpps xmm1, dqword [ebx], 0
cmpeqps xmm1, xmm2
cmpeqps xmm1, dqword [ebx]

cmpsd xmm1, xmm2, 0
cmpsd xmm1, qword [ebx], 0
cmpeqsd xmm1, xmm2
cmpeqsd xmm1, qword [ebx]

cmpss xmm1, xmm2, 0
cmpss xmm1, dword [ebx], 0
cmpeqss xmm1, xmm2
cmpeqss xmm1, dword [ebx]
cmpeqss xmm1, dword [8]

comisd xmm1, xmm2
comisd xmm1, qword [ebx]

comiss xmm1, xmm2
comiss xmm1, dword [ebx]

cvtdq2pd xmm1, xmm2
cvtdq2pd xmm1, qword [ebx]

cvtdq2ps xmm1, xmm2
cvtdq2ps xmm1, dqword [ebx]

cvtpd2dq xmm1, xmm2
cvtpd2dq xmm1, dqword [ebx]

cvtpd2pi mm1, xmm2		; mmx
cvtpd2pi mm1, dqword [ebx]

cvtpd2ps xmm1, xmm2
cvtpd2ps xmm1, dqword [ebx]

cvtpi2pd xmm1, mm2		; mmx
cvtpi2pd xmm1, qword [ebx]

cvtpi2ps xmm1, mm2		; mmx
cvtpi2ps xmm1, qword [ebx]

cvtps2dq xmm1, xmm2
cvtps2dq xmm1, dqword [ebx]

cvtps2pd xmm1, xmm2
cvtps2pd xmm1, qword [ebx]

cvtps2pi mm1, xmm2
cvtps2pi mm1, qword [ebx]

cvtsd2si ebx, xmm2
cvtsd2si ebx, qword [ebx]

cvtsd2ss xmm1, xmm2
cvtsd2ss xmm1, qword [ebx]

cvtsi2sd xmm1, ebx
cvtsi2sd xmm1, dword [ebx]
cvtsi2sd xmm1, ebx
cvtsi2sd xmm1, dword [ebx]

cvtsi2ss xmm1, ebx
cvtsi2ss xmm1, dword [ebx]
cvtsi2ss xmm1, ebx
cvtsi2ss xmm1, dword [ebx]

cvtss2sd xmm1, xmm2
cvtss2sd xmm1, dword [ebx]

cvtss2si ebx, xmm2
cvtss2si ebx, dword [ebx]
cvtss2si ecx, xmm2
cvtss2si ecx, dword [ebx]

cvttpd2dq xmm1, xmm2
cvttpd2dq xmm1, dqword [ebx]

cvttpd2pi mm1, xmm2
cvttpd2pi mm1, dqword [ebx]

cvttps2dq xmm1, xmm2
cvttps2dq xmm1, dqword [ebx]

cvttps2pi mm1, xmm2
cvttps2pi mm1, qword [ebx]

cvttsd2si eax, xmm1
cvttsd2si eax, qword [ebx]
cvttsd2si eax, xmm1
cvttsd2si eax, qword [ebx]

cvttss2si eax, xmm1
cvttss2si eax, dword [ebx]
cvttss2si eax, xmm1
cvttss2si eax, dword [ebx]

divpd xmm1, xmm2
divpd xmm1, dqword [ebx]

divps xmm1, xmm2
divps xmm1, dqword [ebx]

divsd xmm1, xmm2
divsd xmm1, qword [ebx]

divss xmm1, xmm2
divss xmm1, dword [ebx]

extrq xmm1, 0, 1
extrq xmm1, byte 0, byte 1
extrq xmm1, xmm2

haddpd xmm1, xmm2
haddpd xmm1, dqword [ebx]

haddps xmm1, xmm2
haddps xmm1, dqword [ebx]

hsubpd xmm1, xmm2
hsubpd xmm1, dqword [ebx]

hsubps xmm1, xmm2
hsubps xmm1, dqword [ebx]

insertq xmm1, xmm2, 0, 1
insertq xmm1, xmm2, byte 0, byte 1
insertq xmm1, xmm2

lddqu xmm1, dqword [ebx]

ldmxcsr dword [ebx]

maskmovdqu xmm1, xmm2

maxpd xmm1, xmm2
maxpd xmm1, dqword [ebx]

maxps xmm1, xmm2
maxps xmm1, dqword [ebx]

maxsd xmm1, xmm2
maxsd xmm1, qword [ebx]

maxss xmm1, xmm2
maxss xmm1, dword [ebx]

minpd xmm1, xmm2
minpd xmm1, dqword [ebx]

minps xmm1, xmm2
minps xmm1, dqword [ebx]

minsd xmm1, xmm2
minsd xmm1, qword [ebx]

minss xmm1, xmm2
minss xmm1, dword [ebx]

movapd xmm1, xmm2
movapd xmm1, dqword [ebx]
movapd dqword [ebx], xmm2

movaps xmm1, xmm2
movaps xmm1, dqword [ebx]
movaps dqword [ebx], xmm2

movd xmm1, ebx
movd xmm1, dword [ebx]
movd xmm1, ecx
movd xmm1, dword [ebx]
movd dword [ebx], xmm2
movd dword [ebx], xmm2

movddup xmm1, xmm2
movddup xmm1, qword [ebx]

movdq2q mm1, xmm2

movdqa xmm1, xmm2
movdqa xmm1, dqword [ebx]
movdqa dqword [ebx], xmm2

movdqu xmm1, xmm2
movdqu xmm1, dqword [ebx]
movdqu dqword [ebx], xmm2

movhlps xmm1, xmm2

movhpd xmm1, qword [ebx]
movhpd qword [ebx], xmm2

movhps xmm1, qword [ebx]
movhps qword [ebx], xmm2

movlhps xmm1, xmm2

movlpd xmm1, qword [ebx]
movlpd qword [ebx], xmm2

movlps xmm1, qword [ebx]
movlps qword [ebx], xmm2

movmskpd ebx, xmm2

movmskps ebx, xmm2

movntdq dqword [ebx], xmm2

movntpd dqword [ebx], xmm2

movntps dqword [ebx], xmm2

movntsd qword [ebx], xmm2

movntss dword [ebx], xmm2

movq xmm1, xmm2
movq xmm1, qword [ebx]
movq qword [ebx], xmm2

movq2dq xmm1, mm2

movsd xmm1, xmm2
movsd xmm1, qword [ebx]
movsd qword [ebx], xmm2

movshdup xmm1, xmm2
movshdup xmm1, dqword [ebx]

movsldup xmm1, xmm2
movsldup xmm1, dqword [ebx]

movss xmm1, xmm2
movss xmm1, dword [ebx]
movss dword [ebx], xmm2

movupd xmm1, xmm2
movupd xmm1, dqword [ebx]
movupd dqword [ebx], xmm2

movups xmm1, xmm2
movups xmm1, dqword [ebx]
movups dqword [ebx], xmm2

mulpd xmm1, xmm2
mulpd xmm1, dqword [ebx]

mulps xmm1, xmm2
mulps xmm1, dqword [ebx]

mulsd xmm1, xmm2
mulsd xmm1, qword [ebx]

mulss xmm1, xmm2
mulss xmm1, dword [ebx]

orpd xmm1, xmm2
orpd xmm1, dqword [ebx]

orps xmm1, xmm2
orps xmm1, dqword [ebx]

packssdw xmm1, xmm2
packssdw xmm1, dqword [ebx]

packsswb xmm1, xmm2
packsswb xmm1, dqword [ebx]

packuswb xmm1, xmm2
packuswb xmm1, dqword [ebx]

paddb xmm1, xmm2
paddb xmm1, dqword [ebx]

paddd xmm1, xmm2
paddd xmm1, dqword [ebx]

paddq xmm1, xmm2
paddq xmm1, dqword [ebx]

paddsb xmm1, xmm2
paddsb xmm1, dqword [ebx]

paddsw xmm1, xmm2
paddsw xmm1, dqword [ebx]

paddusb xmm1, xmm2
paddusb xmm1, dqword [ebx]

paddusw xmm1, xmm2
paddusw xmm1, dqword [ebx]

paddw xmm1, xmm2
paddw xmm1, dqword [ebx]

pand xmm1, xmm2
pand xmm1, dqword [ebx]

pandn xmm1, xmm2
pandn xmm1, dqword [ebx]

pavgb xmm1, xmm2
pavgb xmm1, dqword [ebx]

pavgw xmm1, xmm2
pavgw xmm1, dqword [ebx]

pcmpeqb xmm1, xmm2
pcmpeqb xmm1, dqword [ebx]

pcmpeqd xmm1, xmm2
pcmpeqd xmm1, dqword [ebx]

pcmpeqw xmm1, xmm2
pcmpeqw xmm1, dqword [ebx]

pcmpgtb xmm1, xmm2
pcmpgtb xmm1, dqword [ebx]

pcmpgtd xmm1, xmm2
pcmpgtd xmm1, dqword [ebx]

pcmpgtw xmm1, xmm2
pcmpgtw xmm1, dqword [ebx]

pextrw ebx, xmm2, byte 0

pinsrw xmm1, ebx, byte 0
pinsrw xmm1, word [ebx], byte 0

pmaddwd xmm1, xmm2
pmaddwd xmm1, dqword [ebx]

pmaxsw xmm1, xmm2
pmaxsw xmm1, dqword [ebx]

pmaxub xmm1, xmm2
pmaxub xmm1, dqword [ebx]

pminsw xmm1, xmm2
pminsw xmm1, dqword [ebx]

pminub xmm1, xmm2
pminub xmm1, dqword [ebx]

pmovmskb eax, xmm2

pmulhuw xmm1, xmm2
pmulhuw xmm1, dqword [ebx]

pmulhw xmm1, xmm2
pmulhw xmm1, dqword [ebx]

pmullw xmm1, xmm2
pmullw xmm1, dqword [ebx]

pmuludq xmm1, xmm2
pmuludq xmm1, dqword [ebx]

por xmm1, xmm2
por xmm1, dqword [ebx]

psadbw xmm1, xmm2
psadbw xmm1, dqword [ebx]

pshufd xmm1, xmm2, byte 0
pshufd xmm1, dqword [ebx], byte 0

pshufhw xmm1, xmm2, byte 0
pshufhw xmm1, dqword [ebx], byte 0

pshuflw xmm1, xmm2, byte 0
pshuflw xmm1, dqword [ebx], byte 0

pslld xmm1, xmm2
pslld xmm1, dqword [ebx]
pslld xmm1, byte 5

pslldq xmm1, byte 5

psllq xmm1, xmm2
psllq xmm1, dqword [ebx]
psllq xmm1, byte 5

psllw xmm1, xmm2
psllw xmm1, dqword [ebx]
psllw xmm1, byte 5

psrad xmm1, xmm2
psrad xmm1, dqword [ebx]
psrad xmm1, byte 5

psraw xmm1, xmm2
psraw xmm1, dqword [ebx]
psraw xmm1, byte 5

psrld xmm1, xmm2
psrld xmm1, dqword [ebx]
psrld xmm1, byte 5

psrldq xmm1, byte 5

psrlq xmm1, xmm2
psrlq xmm1, dqword [ebx]
psrlq xmm1, byte 5

psrlw xmm1, xmm2
psrlw xmm1, dqword [ebx]
psrlw xmm1, byte 5

psubb xmm1, xmm2
psubb xmm1, dqword [ebx]

psubd xmm1, xmm2
psubd xmm1, dqword [ebx]

psubq xmm1, xmm2
psubq xmm1, dqword [ebx]

psubsb xmm1, xmm2
psubsb xmm1, dqword [ebx]

psubsw xmm1, xmm2
psubsw xmm1, dqword [ebx]

psubusb xmm1, xmm2
psubusb xmm1, dqword [ebx]

psubusw xmm1, xmm2
psubusw xmm1, dqword [ebx]

psubw xmm1, xmm2
psubw xmm1, dqword [ebx]

punpckhbw xmm1, xmm2
punpckhbw xmm1, dqword [ebx]

punpckhdq xmm1, xmm2
punpckhdq xmm1, dqword [ebx]

punpckhqdq xmm1, xmm2
punpckhqdq xmm1, dqword [ebx]

punpckhwd xmm1, xmm2
punpckhwd xmm1, dqword [ebx]

punpcklbw xmm1, xmm2
punpcklbw xmm1, dqword [ebx]

punpckldq xmm1, xmm2
punpckldq xmm1, dqword [ebx]

punpcklqdq xmm1, xmm2
punpcklqdq xmm1, dqword [ebx]

punpcklwd xmm1, xmm2
punpcklwd xmm1, dqword [ebx]

pxor xmm1, xmm2
pxor xmm1, dqword [ebx]

rcpps xmm1, xmm2
rcpps xmm1, dqword [ebx]

rcpss xmm1, xmm2
rcpss xmm1, dword [ebx]

rsqrtps xmm1, xmm2
rsqrtps xmm1, dqword [ebx]

rsqrtss xmm1, xmm2
rsqrtss xmm1, dword [ebx]

shufpd xmm1, xmm2, 0
shufpd xmm1, dqword [ebx], byte 0

shufps xmm1, xmm2, 0
shufps xmm1, dqword [ebx], byte 0

sqrtpd xmm1, xmm2
sqrtpd xmm1, dqword [ebx]

sqrtps xmm1, xmm2
sqrtps xmm1, dqword [ebx]

sqrtsd xmm1, xmm2
sqrtsd xmm1, qword [ebx]

sqrtss xmm1, xmm2
sqrtss xmm1, dword [ebx]

stmxcsr dword [ebx]

subpd xmm1, xmm2
subpd xmm1, dqword [ebx]

subps xmm1, xmm2
subps xmm1, dqword [ebx]

subsd xmm1, xmm2
subsd xmm1, qword [ebx]

subss xmm1, xmm2
subss xmm1, dword [ebx]

ucomisd xmm1, xmm2
ucomisd xmm1, qword [ebx]

ucomiss xmm1, xmm2
ucomiss xmm1, dword [ebx]

unpckhpd xmm1, xmm2
unpckhpd xmm1, dqword [ebx]

unpckhps xmm1, xmm2
unpckhps xmm1, dqword [ebx]

unpcklpd xmm1, xmm2
unpcklpd xmm1, dqword [ebx]

unpcklps xmm1, xmm2
unpcklps xmm1, dqword [ebx]

xorpd xmm1, xmm2
xorpd xmm1, dqword [ebx]

xorps xmm1, xmm2
xorps xmm1, dqword [ebx]
leave
retn

sse3:
enter 800,4
addsubpd xmm5, xmm7
addsubpd xmm0, [eax]
addsubps xmm1, xmm5
addsubps xmm3, dqword [edx]
fisttp word [0]
fisttp dword [4]
fisttp qword [8]
haddpd xmm2, xmm4
haddpd xmm7, [ecx+4]
haddps xmm6, xmm1
haddps xmm0, dqword [0]
hsubpd xmm5, xmm3
hsubpd xmm1, [ebp]
hsubps xmm4, xmm1
hsubps xmm2, [esp]
lddqu xmm3, [ecx+edx*4+8]
monitor
movddup xmm7, xmm6
movddup xmm1, qword [4]
movshdup xmm3, xmm4
movshdup xmm2, [0]
movsldup xmm0, xmm7
movsldup xmm5, dqword [eax+ebx]
mwait
leave
retn

ssse3:
enter 1000,1
%MACRO TEST_GENERIC 5
;global _test_ %+ %1 %+ _ %+ %4
;global test_ %+ %1 %+ _ %+ %4
_test_ %+ %1 %+ _ %+ %4:
test_ %+ %1 %+ _ %+ %4:
   mov         edx, [ esp + 4 ]
   mov         eax, [ esp + 8 ]
   %2          %3, [ edx ]
   %2          %5, [ eax ]
   %1          %3, %5
   %2          [ edx ], %3
   ret
%ENDMACRO

TEST_GENERIC pabsb, movq, mm0, mmx, mm1
TEST_GENERIC pabsw, movq, mm0, mmx, mm1
TEST_GENERIC pabsd, movq, mm0, mmx, mm1

TEST_GENERIC pabsb, movdqu, xmm0, xmm, xmm1
TEST_GENERIC pabsw, movdqu, xmm0, xmm, xmm1
TEST_GENERIC pabsd, movdqu, xmm0, xmm, xmm1

TEST_GENERIC psignb, movq, mm0, mmx, mm1
TEST_GENERIC psignw, movq, mm0, mmx, mm1
TEST_GENERIC psignd, movq, mm0, mmx, mm1
          
TEST_GENERIC psignb, movdqu, xmm0, xmm, xmm1
TEST_GENERIC psignw, movdqu, xmm0, xmm, xmm1
TEST_GENERIC psignd, movdqu, xmm0, xmm, xmm1

TEST_GENERIC phaddw, movq, mm0, mmx, mm1
TEST_GENERIC phaddsw, movq, mm0, mmx, mm1
TEST_GENERIC phaddd, movq, mm0, mmx, mm1

TEST_GENERIC phaddw, movdqu, xmm0, xmm, xmm1
TEST_GENERIC phaddsw, movdqu, xmm0, xmm, xmm1
TEST_GENERIC phaddd, movdqu, xmm0, xmm, xmm1

TEST_GENERIC phsubw, movq, mm0, mmx, mm1
TEST_GENERIC phsubsw, movq, mm0, mmx, mm1
TEST_GENERIC phsubd, movq, mm0, mmx, mm1

TEST_GENERIC phsubw, movdqu, xmm0, xmm, xmm1
TEST_GENERIC phsubsw, movdqu, xmm0, xmm, xmm1
TEST_GENERIC phsubd, movdqu, xmm0, xmm, xmm1

TEST_GENERIC pmulhrsw, movq, mm0, mmx, mm1
TEST_GENERIC pmulhrsw, movdqu, xmm0, xmm, xmm1

TEST_GENERIC pmaddubsw, movq, mm0, mmx, mm1
TEST_GENERIC pmaddubsw, movdqu, xmm0, xmm, xmm1

TEST_GENERIC pshufb, movq, mm0, mmx, mm1
TEST_GENERIC pshufb, movdqu, xmm0, xmm, xmm1

%MACRO TEST_ALIGNR 5
;global _test_ %+ %1 %+ _ %+ %4
;global test_ %+ %1 %+ _ %+ %4
_test_ %+ %1 %+ _ %+ %4:
test_ %+ %1 %+ _ %+ %4:
   mov         edx, [ esp + 4 ]
   mov         eax, [ esp + 8 ]
   %2          %3, [ edx ]
   %2          %5, [ eax ]
   %1          %3, %5, 3
   %2          [ edx ], %3
   ret
%ENDMACRO

TEST_ALIGNR palignr, movq, mm0, mmx, mm1
TEST_ALIGNR palignr, movdqu, xmm0, xmm, xmm1
leave
retn

sse4:
enter 23,8
blendpd xmm1, xmm2, 5
blendpd xmm1, [4], 5

blendps xmm1, xmm2, 5
blendps xmm1, [4], 5

blendvpd xmm1, xmm2
blendvpd xmm1, xmm2, xmm0
blendvpd xmm1, [4]
blendvpd xmm1, [4], xmm0

blendvps xmm1, xmm2
blendvps xmm1, xmm2, xmm0
blendvps xmm1, [4]
blendvps xmm1, [4], xmm0

crc32 eax, bl
crc32 eax, bh
crc32 eax, ecx
crc32 eax, byte [4]
crc32 eax, bx
crc32 eax, word [4]
crc32 eax, ebx
crc32 eax, dword [4]

crc32 ecx, bl
;crc32 r8d, bh			; error
crc32 ecx, edx
crc32 ecx, byte [4]
crc32 ecx, bx
crc32 ecx, word [4]
crc32 ecx, ebx
crc32 ecx, dword [4]

crc32 eax, bl
;crc32 eax, bh			; error
crc32 eax, edx
crc32 eax, byte [4]
crc32 eax, ebx
crc32 eax, dword [4]

dppd xmm1, xmm2, 5
dppd xmm1, [4], 5

dpps xmm1, xmm2, 5
dpps xmm1, [4], 5

extractps eax, xmm1, 5
extractps [4], xmm1, 5
extractps dword [4], xmm1, 5
extractps edi, xmm1, 5
extractps eax, xmm1, 5

insertps xmm1, xmm2, 5
insertps xmm1, [4], 5
insertps xmm1, dword [4], 5

movntdqa xmm1, [4]
movntdqa xmm1, dqword [4]

mpsadbw xmm1, xmm2, 5
mpsadbw xmm1, [4], 5

packusdw xmm1, xmm2
packusdw xmm1, [4]

pblendvb xmm1, xmm2, xmm0
pblendvb xmm1, [4], xmm0
pblendvb xmm1, xmm2
pblendvb xmm1, [4]

pblendw xmm1, xmm2, 5
pblendw xmm1, [4], 5

pcmpeqq xmm1, xmm2
pcmpeqq xmm1, [4]

pcmpestri xmm1, xmm2, 5
pcmpestri xmm1, [4], 5

pcmpestrm xmm1, xmm2, 5
pcmpestrm xmm1, [4], 5

pcmpistri xmm1, xmm2, 5
pcmpistri xmm1, [4], 5

pcmpistrm xmm1, xmm2, 5
pcmpistrm xmm1, [4], 5

pcmpgtq xmm1, xmm2
pcmpgtq xmm1, [4]

pextrb eax, xmm1, 5
pextrb ecx, xmm1, 5
pextrb [4], xmm1, 5
pextrb byte [4], xmm1, 5

pextrd eax, xmm1, 5
pextrd [4], xmm1, 5
pextrd dword [4], xmm1, 5
;pextrq edx, xmm1, 5
;pextrq qword [4], xmm1, 5

; To get the SSE4 versions we need to disable the SSE2 versions
cpu nosse2
pextrw eax, xmm1, 5
pextrw [4], xmm1, 5
pextrw word [4], xmm1, 5
pextrw esi, xmm1, 5

phminposuw xmm1, xmm2
phminposuw xmm1, [4]

pinsrb xmm1, eax, 5
pinsrb xmm1, [4], 5
pinsrb xmm1, byte [4], 5

pinsrd xmm1, eax, 5
pinsrd xmm1, [4], 5
pinsrd xmm1, dword [4], 5

;pinsrq xmm1, ebp, 5
;pinsrq xmm1, [4], 5
;pinsrq xmm1, qword [4], 5

pmaxsb xmm1, xmm2
pmaxsb xmm1, [4]

pmaxsd xmm1, xmm2
pmaxsd xmm1, [4]

pmaxud xmm1, xmm2
pmaxud xmm1, [4]

pmaxuw xmm1, xmm2
pmaxuw xmm1, [4]

pminsb xmm1, xmm2
pminsb xmm1, [4]

pminsd xmm1, xmm2
pminsd xmm1, [4]

pminud xmm1, xmm2
pminud xmm1, [4]

pminuw xmm1, xmm2
pminuw xmm1, [4]

pmovsxbw xmm1, xmm2
pmovsxbw xmm1, [4]
pmovsxbw xmm1, qword [4]

pmovsxbd xmm1, xmm2
pmovsxbd xmm1, [4]
pmovsxbd xmm1, dword [4]

pmovsxbq xmm1, xmm2
pmovsxbq xmm1, [4]
pmovsxbq xmm1, word [4]

pmovsxwd xmm1, xmm2
pmovsxwd xmm1, [4]
pmovsxwd xmm1, qword [4]

;pmovsxwq xmm1, xmm2
;pmovsxwq xmm1, [4]
;pmovsxwq xmm1, dword [4]

;pmovsxdq xmm1, xmm2
;pmovsxdq xmm1, [4]
;pmovsxdq xmm1, qword [4]

pmovzxbw xmm1, xmm2
pmovzxbw xmm1, [4]
pmovzxbw xmm1, qword [4]

pmovzxbd xmm1, xmm2
pmovzxbd xmm1, [4]
pmovzxbd xmm1, dword [4]

;pmovzxbq xmm1, xmm2
;pmovzxbq xmm1, [4]
;pmovzxbq xmm1, word [4]

pmovzxwd xmm1, xmm2
pmovzxwd xmm1, [4]
pmovzxwd xmm1, qword [4]

pmovzxwq xmm1, xmm2
pmovzxwq xmm1, [4]
pmovzxwq xmm1, dword [4]

;pmovzxdq xmm1, xmm2
;pmovzxdq xmm1, [4]
;pmovzxdq xmm1, qword [4]

pmuldq xmm1, xmm2
pmuldq xmm1, [4]

pmulld xmm1, xmm2
pmulld xmm1, [4]

popcnt ax, bx
popcnt ax, [4]
popcnt ebx, ecx
popcnt ebx, [4]
popcnt ecx, edx
popcnt ecx, [4]

ptest xmm1, xmm2
ptest xmm1, [4]

roundpd xmm1, xmm2, 5
roundpd xmm1, [4], 5

roundps xmm1, xmm2, 5
roundps xmm1, [4], 5

roundsd xmm1, xmm2, 5
roundsd xmm1, [4], 5

roundss xmm1, xmm2, 5
roundss xmm1, [4], 5
leave
retn

aes:
enter 26,3
aesenc xmm1, xmm2
aesenc xmm1, [eax]
aesenc xmm1, dqword [eax]
aesenc xmm5, xmm6
aesenc xmm5, [eax+edi*4]
aesenc xmm5, [edx+ecx*4]
vaesenc xmm1, xmm2
vaesenc xmm1, [eax]
vaesenc xmm1, dqword [eax]
vaesenc xmm1, xmm2, xmm3
vaesenc xmm1, xmm2, [eax]
vaesenc xmm1, xmm2, dqword [eax]

aesenclast xmm1, xmm2
aesenclast xmm1, [eax]
aesenclast xmm1, dqword [eax]
vaesenclast xmm1, xmm2
vaesenclast xmm1, [eax]
vaesenclast xmm1, dqword [eax]
vaesenclast xmm1, xmm2, xmm3
vaesenclast xmm1, xmm2, [eax]
vaesenclast xmm1, xmm2, dqword [eax]

aesdec xmm1, xmm2
aesdec xmm1, [eax]
aesdec xmm1, dqword [eax]
vaesdec xmm1, xmm2
vaesdec xmm1, [eax]
vaesdec xmm1, dqword [eax]
vaesdec xmm1, xmm2, xmm3
vaesdec xmm1, xmm2, [eax]
vaesdec xmm1, xmm2, dqword [eax]

aesdeclast xmm1, xmm2
aesdeclast xmm1, [eax]
aesdeclast xmm1, dqword [eax]
vaesdeclast xmm1, xmm2
vaesdeclast xmm1, [eax]
vaesdeclast xmm1, dqword [eax]
vaesdeclast xmm1, xmm2, xmm3
vaesdeclast xmm1, xmm2, [eax]
vaesdeclast xmm1, xmm2, dqword [eax]

aesimc xmm1, xmm2
aesimc xmm1, [eax]
aesimc xmm1, dqword [eax]
vaesimc xmm1, xmm2
vaesimc xmm1, [eax]
vaesimc xmm1, dqword [eax]
; no 3-operand form

aeskeygenassist xmm1, xmm2, 5
aeskeygenassist xmm1, [eax], byte 5
aeskeygenassist xmm1, dqword [eax], 5
vaeskeygenassist xmm1, xmm2, 5
vaeskeygenassist xmm1, [eax], byte 5
vaeskeygenassist xmm1, dqword [eax], 5

pclmulqdq xmm1, xmm2, 5
pclmulqdq xmm1, [eax], byte 5
pclmulqdq xmm1, dqword [eax], 5

; pclmulqdq variants
pclmullqlqdq xmm1, xmm2
pclmullqlqdq xmm1, [eax]
pclmullqlqdq xmm1, dqword [eax]

pclmulhqlqdq xmm1, xmm2
pclmulhqlqdq xmm1, [eax]
pclmulhqlqdq xmm1, dqword [eax]

pclmullqhqdq xmm1, xmm2
pclmullqhqdq xmm1, [eax]
pclmullqhqdq xmm1, dqword [eax]

pclmulhqhqdq xmm1, xmm2
pclmulhqhqdq xmm1, [eax]
pclmulhqhqdq xmm1, dqword [eax]
leave
retn

avx:
enter 200,0
cpu sse2
addpd xmm1, xmm2
addpd xmm1, [eax]
addpd xmm1, dqword [eax]
addpd xmm5, xmm6
addpd xmm5, [eax+edi*4]
addpd xmm5, [esi+edi*4]

vaddpd xmm1, xmm2
vaddpd xmm1, [eax]
vaddpd xmm1, dqword [eax]
vaddpd xmm5, xmm6
vaddpd xmm5, [eax+edi*4]
vaddpd xmm5, [esi+edi*4]

vaddpd xmm1, xmm2, xmm3
vaddpd xmm1, xmm2, [eax]
vaddpd xmm1, xmm2, dqword [eax]
vaddpd xmm5, xmm6, xmm7
vaddpd xmm5, xmm6, [eax+edi*4]
vaddpd xmm5, xmm6, [esi+edi*4]

vaddpd ymm1, ymm2, ymm3
vaddpd ymm1, ymm2, [eax]
vaddpd ymm1, ymm2, yword [eax]
vaddpd ymm5, ymm6, ymm7
vaddpd ymm5, ymm6, [eax+edi*4]
vaddpd ymm5, ymm6, [esi+edi*4]

; Further instructions won't test high 8 registers (validated above)
addps xmm1, xmm2
addps xmm1, [eax]
addps xmm1, dqword [eax]
vaddps xmm1, xmm2
vaddps xmm1, [eax]
vaddps xmm1, dqword [eax]
vaddps xmm1, xmm2, xmm3
vaddps xmm1, xmm2, [eax]
vaddps xmm1, xmm2, dqword [eax]
vaddps ymm1, ymm2, ymm3
vaddps ymm1, ymm2, [eax]
vaddps ymm1, ymm2, yword [eax]

addsd xmm1, xmm2
addsd xmm1, [eax]
addsd xmm1, qword [eax]
vaddsd xmm1, xmm2
vaddsd xmm1, [eax]
vaddsd xmm1, qword [eax]
vaddsd xmm1, xmm2, xmm3
vaddsd xmm1, xmm2, [eax]
vaddsd xmm1, xmm2, qword [eax]

addss xmm1, xmm2
addss xmm1, [eax]
addss xmm1, dword [eax]
vaddss xmm1, xmm2
vaddss xmm1, [eax]
vaddss xmm1, dword [eax]
vaddss xmm1, xmm2, xmm3
vaddss xmm1, xmm2, [eax]
vaddss xmm1, xmm2, dword [eax]

addsubpd xmm1, xmm2
addsubpd xmm1, [eax]
addsubpd xmm1, dqword [eax]
vaddsubpd xmm1, xmm2
vaddsubpd xmm1, [eax]
vaddsubpd xmm1, dqword [eax]
vaddsubpd xmm1, xmm2, xmm3
vaddsubpd xmm1, xmm2, [eax]
vaddsubpd xmm1, xmm2, dqword [eax]
vaddsubpd ymm1, ymm2, ymm3
vaddsubpd ymm1, ymm2, [eax]
vaddsubpd ymm1, ymm2, yword [eax]

addsubps xmm1, xmm2
addsubps xmm1, [eax]
addsubps xmm1, dqword [eax]
vaddsubps xmm1, xmm2
vaddsubps xmm1, [eax]
vaddsubps xmm1, dqword [eax]
vaddsubps xmm1, xmm2, xmm3
vaddsubps xmm1, xmm2, [eax]
vaddsubps xmm1, xmm2, dqword [eax]
vaddsubps ymm1, ymm2, ymm3
vaddsubps ymm1, ymm2, [eax]
vaddsubps ymm1, ymm2, yword [eax]

andpd xmm1, xmm2
andpd xmm1, [eax]
andpd xmm1, dqword [eax]
vandpd xmm1, xmm2
vandpd xmm1, [eax]
vandpd xmm1, dqword [eax]
vandpd xmm1, xmm2, xmm3
vandpd xmm1, xmm2, [eax]
vandpd xmm1, xmm2, dqword [eax]
vandpd ymm1, ymm2, ymm3
vandpd ymm1, ymm2, [eax]
vandpd ymm1, ymm2, yword [eax]

andps xmm1, xmm2
andps xmm1, [eax]
andps xmm1, dqword [eax]
vandps xmm1, xmm2
vandps xmm1, [eax]
vandps xmm1, dqword [eax]
vandps xmm1, xmm2, xmm3
vandps xmm1, xmm2, [eax]
vandps xmm1, xmm2, dqword [eax]
vandps ymm1, ymm2, ymm3
vandps ymm1, ymm2, [eax]
vandps ymm1, ymm2, yword [eax]

andnpd xmm1, xmm2
andnpd xmm1, [eax]
andnpd xmm1, dqword [eax]
vandnpd xmm1, xmm2
vandnpd xmm1, [eax]
vandnpd xmm1, dqword [eax]
vandnpd xmm1, xmm2, xmm3
vandnpd xmm1, xmm2, [eax]
vandnpd xmm1, xmm2, dqword [eax]
vandnpd ymm1, ymm2, ymm3
vandnpd ymm1, ymm2, [eax]
vandnpd ymm1, ymm2, yword [eax]

andnps xmm1, xmm2
andnps xmm1, [eax]
andnps xmm1, dqword [eax]
vandnps xmm1, xmm2
vandnps xmm1, [eax]
vandnps xmm1, dqword [eax]
vandnps xmm1, xmm2, xmm3
vandnps xmm1, xmm2, [eax]
vandnps xmm1, xmm2, dqword [eax]
vandnps ymm1, ymm2, ymm3
vandnps ymm1, ymm2, [eax]
vandnps ymm1, ymm2, yword [eax]

blendpd xmm1, xmm2, 5
blendpd xmm1, [eax], byte 5
blendpd xmm1, dqword [eax], 5
vblendpd xmm1, xmm2, 5
vblendpd xmm1, [eax], byte 5
vblendpd xmm1, dqword [eax], 5
vblendpd xmm1, xmm2, xmm3, 5
vblendpd xmm1, xmm2, [eax], byte 5
vblendpd xmm1, xmm2, dqword [eax], 5
vblendpd ymm1, ymm2, ymm3, 5
vblendpd ymm1, ymm2, [eax], byte 5
vblendpd ymm1, ymm2, yword [eax], 5

blendps xmm1, xmm2, 5
blendps xmm1, [eax], byte 5
blendps xmm1, dqword [eax], 5
vblendps xmm1, xmm2, 5
vblendps xmm1, [eax], byte 5
vblendps xmm1, dqword [eax], 5
vblendps xmm1, xmm2, xmm3, 5
vblendps xmm1, xmm2, [eax], byte 5
vblendps xmm1, xmm2, dqword [eax], 5
vblendps ymm1, ymm2, ymm3, 5
vblendps ymm1, ymm2, [eax], byte 5
vblendps ymm1, ymm2, yword [eax], 5

; blendvpd doesn't have vex-encoded version of implicit xmm0
blendvpd xmm1, xmm3
blendvpd xmm1, [eax]
blendvpd xmm1, dqword [eax]
blendvpd xmm1, xmm3, xmm0
blendvpd xmm1, [eax], xmm0
blendvpd xmm1, dqword [eax], xmm0
vblendvpd xmm1, xmm2, xmm3, xmm4
vblendvpd xmm1, xmm2, [eax], xmm4
vblendvpd xmm1, xmm2, dqword [eax], xmm4
vblendvpd ymm1, ymm2, ymm3, ymm4
vblendvpd ymm1, ymm2, [eax], ymm4
vblendvpd ymm1, ymm2, yword [eax], ymm4

; blendvps doesn't have vex-encoded version of implicit xmm0
blendvps xmm1, xmm3
blendvps xmm1, [eax]
blendvps xmm1, dqword [eax]
blendvps xmm1, xmm3, xmm0
blendvps xmm1, [eax], xmm0
blendvps xmm1, dqword [eax], xmm0
vblendvps xmm1, xmm2, xmm3, xmm4
vblendvps xmm1, xmm2, [eax], xmm4
vblendvps xmm1, xmm2, dqword [eax], xmm4
vblendvps ymm1, ymm2, ymm3, ymm4
vblendvps ymm1, ymm2, [eax], ymm4
vblendvps ymm1, ymm2, yword [eax], ymm4

vbroadcastss xmm1, [eax]
vbroadcastss xmm1, dword [eax]
vbroadcastss ymm1, [eax]
vbroadcastss ymm1, dword [eax]

vbroadcastsd ymm1, [eax]
vbroadcastsd ymm1, qword [eax]

vbroadcastf128 ymm1, [eax]
vbroadcastf128 ymm1, dqword [eax]

cmppd xmm1, xmm2, 5
cmppd xmm1, [eax], byte 5
cmppd xmm1, dqword [eax], 5
vcmppd xmm1, xmm2, 5
vcmppd xmm1, [eax], byte 5
vcmppd xmm1, dqword [eax], 5
vcmppd xmm1, xmm2, xmm3, 5
vcmppd xmm1, xmm2, [eax], byte 5
vcmppd xmm1, xmm2, dqword [eax], 5
vcmppd ymm1, ymm2, ymm3, 5
vcmppd ymm1, ymm2, [eax], byte 5
vcmppd ymm1, ymm2, yword [eax], 5

cmpps xmm1, xmm2, 5
cmpps xmm1, [eax], byte 5
cmpps xmm1, dqword [eax], 5
vcmpps xmm1, xmm2, 5
vcmpps xmm1, [eax], byte 5
vcmpps xmm1, dqword [eax], 5
vcmpps xmm1, xmm2, xmm3, 5
vcmpps xmm1, xmm2, [eax], byte 5
vcmpps xmm1, xmm2, dqword [eax], 5
vcmpps ymm1, ymm2, ymm3, 5
vcmpps ymm1, ymm2, [eax], byte 5
vcmpps ymm1, ymm2, yword [eax], 5

cmpsd xmm1, xmm2, 5
cmpsd xmm1, [eax], byte 5
cmpsd xmm1, qword [eax], 5
vcmpsd xmm1, xmm2, 5
vcmpsd xmm1, [eax], byte 5
vcmpsd xmm1, qword [eax], 5
vcmpsd xmm1, xmm2, xmm3, 5
vcmpsd xmm1, xmm2, [eax], byte 5
vcmpsd xmm1, xmm2, qword [eax], 5

cmpss xmm1, xmm2, 5
cmpss xmm1, [eax], byte 5
cmpss xmm1, dword [eax], 5
vcmpss xmm1, xmm2, 5
vcmpss xmm1, [eax], byte 5
vcmpss xmm1, dword [eax], 5
vcmpss xmm1, xmm2, xmm3, 5
vcmpss xmm1, xmm2, [eax], byte 5
vcmpss xmm1, xmm2, dword [eax], 5

comisd xmm1, xmm2
comisd xmm1, [eax]
comisd xmm1, qword [eax]
vcomisd xmm1, xmm2
vcomisd xmm1, [eax]
vcomisd xmm1, qword [eax]

comiss xmm1, xmm2
comiss xmm1, [eax]
comiss xmm1, dword [eax]
vcomiss xmm1, xmm2
vcomiss xmm1, [eax]
vcomiss xmm1, dword [eax]

cvtdq2pd xmm1, xmm2
cvtdq2pd xmm1, [eax]
cvtdq2pd xmm1, qword [eax]
vcvtdq2pd xmm1, xmm2
vcvtdq2pd xmm1, [eax]
vcvtdq2pd xmm1, qword [eax]
vcvtdq2pd ymm1, xmm2
vcvtdq2pd ymm1, [eax]
vcvtdq2pd ymm1, dqword [eax]

cvtdq2ps xmm1, xmm2
cvtdq2ps xmm1, [eax]
cvtdq2ps xmm1, dqword [eax]
vcvtdq2ps xmm1, xmm2
vcvtdq2ps xmm1, [eax]
vcvtdq2ps xmm1, dqword [eax]
vcvtdq2ps ymm1, ymm2
vcvtdq2ps ymm1, [eax]
vcvtdq2ps ymm1, yword [eax]

; These require memory operand size to be specified (in AVX version)
cvtpd2dq xmm1, xmm2
cvtpd2dq xmm1, [eax]
cvtpd2dq xmm1, dqword [eax]
vcvtpd2dq xmm1, xmm2
vcvtpd2dq xmm1, dqword [eax]
vcvtpd2dq xmm1, ymm2
vcvtpd2dq xmm1, yword [eax]

cvtpd2ps xmm1, xmm2
cvtpd2ps xmm1, [eax]
cvtpd2ps xmm1, dqword [eax]
vcvtpd2ps xmm1, xmm2
vcvtpd2ps xmm1, dqword [eax]
vcvtpd2ps xmm1, ymm2
vcvtpd2ps xmm1, yword [eax]

cvtps2dq xmm1, xmm2
cvtps2dq xmm1, [eax]
cvtps2dq xmm1, dqword [eax]
vcvtps2dq xmm1, xmm2
vcvtps2dq xmm1, [eax]
vcvtps2dq xmm1, dqword [eax]
vcvtps2dq ymm1, ymm2
vcvtps2dq ymm1, [eax]
vcvtps2dq ymm1, yword [eax]

cvtps2pd xmm1, xmm2
cvtps2pd xmm1, [eax]
cvtps2pd xmm1, qword [eax]
vcvtps2pd xmm1, xmm2
vcvtps2pd xmm1, [eax]
vcvtps2pd xmm1, qword [eax]
vcvtps2pd ymm1, xmm2
vcvtps2pd ymm1, [eax]
vcvtps2pd ymm1, dqword [eax]

cvtsd2si eax, xmm2
cvtsd2si eax, [eax]
cvtsd2si eax, qword [eax]
vcvtsd2si eax, xmm2
vcvtsd2si eax, [eax]
vcvtsd2si eax, qword [eax]
cvtsd2si eax, xmm2
cvtsd2si eax, [eax]
cvtsd2si eax, qword [eax]
vcvtsd2si eax, xmm2
vcvtsd2si eax, [eax]
vcvtsd2si eax, qword [eax]

cvtsd2ss xmm1, xmm2
cvtsd2ss xmm1, [eax]
cvtsd2ss xmm1, qword [eax]
vcvtsd2ss xmm1, xmm2
vcvtsd2ss xmm1, [eax]
vcvtsd2ss xmm1, qword [eax]
vcvtsd2ss xmm1, xmm2, xmm3
vcvtsd2ss xmm1, xmm2, [eax]
vcvtsd2ss xmm1, xmm2, qword [eax]

; unsized not valid
cvtsi2sd xmm1, eax
cvtsi2sd xmm1, dword [eax]
vcvtsi2sd xmm1, eax
vcvtsi2sd xmm1, dword [eax]
vcvtsi2sd xmm1, xmm2, eax
vcvtsi2sd xmm1, xmm2, dword [eax]
cvtsi2sd xmm1, eax
cvtsi2sd xmm1, dword [eax]
vcvtsi2sd xmm1, eax
vcvtsi2sd xmm1, dword [eax]
vcvtsi2sd xmm1, xmm2, eax
vcvtsi2sd xmm1, xmm2, dword [eax]

cvtsi2ss xmm1, eax
cvtsi2ss xmm1, dword [eax]
vcvtsi2ss xmm1, eax
vcvtsi2ss xmm1, dword [eax]
vcvtsi2ss xmm1, xmm2, eax
vcvtsi2ss xmm1, xmm2, dword [eax]
cvtsi2ss xmm1, eax
cvtsi2ss xmm1, dword [eax]
vcvtsi2ss xmm1, eax
vcvtsi2ss xmm1, dword [eax]
vcvtsi2ss xmm1, xmm2, eax
vcvtsi2ss xmm1, xmm2, dword [eax]

cvtss2sd xmm1, xmm2
cvtss2sd xmm1, [eax]
cvtss2sd xmm1, dword [eax]
vcvtss2sd xmm1, xmm2
vcvtss2sd xmm1, [eax]
vcvtss2sd xmm1, dword [eax]
vcvtss2sd xmm1, xmm2, xmm3
vcvtss2sd xmm1, xmm2, [eax]
vcvtss2sd xmm1, xmm2, dword [eax]

cvtss2si eax, xmm2
cvtss2si eax, [eax]
cvtss2si eax, dword [eax]
vcvtss2si eax, xmm2
vcvtss2si eax, [eax]
vcvtss2si eax, dword [eax]
cvtss2si eax, xmm2
cvtss2si eax, [eax]
cvtss2si eax, dword [eax]
vcvtss2si eax, xmm2
vcvtss2si eax, [eax]
vcvtss2si eax, dword [eax]

; These require memory operand size to be specified (in AVX version)
cvttpd2dq xmm1, xmm2
cvttpd2dq xmm1, [eax]
cvttpd2dq xmm1, dqword [eax]
vcvttpd2dq xmm1, xmm2
vcvttpd2dq xmm1, dqword [eax]
vcvttpd2dq xmm1, ymm2
vcvttpd2dq xmm1, yword [eax]

cvttps2dq xmm1, xmm2
cvttps2dq xmm1, [eax]
cvttps2dq xmm1, dqword [eax]
vcvttps2dq xmm1, xmm2
vcvttps2dq xmm1, [eax]
vcvttps2dq xmm1, dqword [eax]
vcvttps2dq ymm1, ymm2
vcvttps2dq ymm1, [eax]
vcvttps2dq ymm1, yword [eax]

cvttsd2si eax, xmm2
cvttsd2si eax, [eax]
cvttsd2si eax, qword [eax]
vcvttsd2si eax, xmm2
vcvttsd2si eax, [eax]
vcvttsd2si eax, qword [eax]
cvttsd2si eax, xmm2
cvttsd2si eax, [eax]
cvttsd2si eax, qword [eax]
vcvttsd2si eax, xmm2
vcvttsd2si eax, [eax]
vcvttsd2si eax, qword [eax]

cvttss2si eax, xmm2
cvttss2si eax, [eax]
cvttss2si eax, dword [eax]
vcvttss2si eax, xmm2
vcvttss2si eax, [eax]
vcvttss2si eax, dword [eax]
cvttss2si eax, xmm2
cvttss2si eax, [eax]
cvttss2si eax, dword [eax]
vcvttss2si eax, xmm2
vcvttss2si eax, [eax]
vcvttss2si eax, dword [eax]

divpd xmm1, xmm2
divpd xmm1, [eax]
divpd xmm1, dqword [eax]
vdivpd xmm1, xmm2
vdivpd xmm1, [eax]
vdivpd xmm1, dqword [eax]
vdivpd xmm1, xmm2, xmm3
vdivpd xmm1, xmm2, [eax]
vdivpd xmm1, xmm2, dqword [eax]
vdivpd ymm1, ymm2, ymm3
vdivpd ymm1, ymm2, [eax]
vdivpd ymm1, ymm2, yword [eax]

divps xmm1, xmm2
divps xmm1, [eax]
divps xmm1, dqword [eax]
vdivps xmm1, xmm2
vdivps xmm1, [eax]
vdivps xmm1, dqword [eax]
vdivps xmm1, xmm2, xmm3
vdivps xmm1, xmm2, [eax]
vdivps xmm1, xmm2, dqword [eax]
vdivps ymm1, ymm2, ymm3
vdivps ymm1, ymm2, [eax]
vdivps ymm1, ymm2, yword [eax]

divsd xmm1, xmm2
divsd xmm1, [eax]
divsd xmm1, qword [eax]
vdivsd xmm1, xmm2
vdivsd xmm1, [eax]
vdivsd xmm1, qword [eax]
vdivsd xmm1, xmm2, xmm3
vdivsd xmm1, xmm2, [eax]
vdivsd xmm1, xmm2, qword [eax]

divss xmm1, xmm2
divss xmm1, [eax]
divss xmm1, dword [eax]
vdivss xmm1, xmm2
vdivss xmm1, [eax]
vdivss xmm1, dword [eax]
vdivss xmm1, xmm2, xmm3
vdivss xmm1, xmm2, [eax]
vdivss xmm1, xmm2, dword [eax]

dppd xmm1, xmm2, 5
dppd xmm1, [eax], byte 5
dppd xmm1, dqword [eax], 5
vdppd xmm1, xmm2, 5
vdppd xmm1, [eax], byte 5
vdppd xmm1, dqword [eax], 5
vdppd xmm1, xmm2, xmm3, 5
vdppd xmm1, xmm2, [eax], byte 5
vdppd xmm1, xmm2, dqword [eax], 5
; no ymm version

dpps xmm1, xmm2, 5
dpps xmm1, [eax], byte 5
dpps xmm1, dqword [eax], 5
vdpps xmm1, xmm2, 5
vdpps xmm1, [eax], byte 5
vdpps xmm1, dqword [eax], 5
vdpps xmm1, xmm2, xmm3, 5
vdpps xmm1, xmm2, [eax], byte 5
vdpps xmm1, xmm2, dqword [eax], 5
vdpps ymm1, ymm2, ymm3, 5
vdpps ymm1, ymm2, [eax], byte 5
vdpps ymm1, ymm2, yword [eax], 5

vextractf128 xmm1, ymm2, 5
vextractf128 [eax], ymm2, byte 5
vextractf128 dqword [eax], ymm2, 5

extractps eax, xmm1, 5
extractps eax, xmm1, 5
extractps [eax], xmm1, byte 5
extractps dword [eax], xmm1, 5
vextractps eax, xmm1, 5
vextractps eax, xmm1, 5
vextractps [eax], xmm1, byte 5
vextractps dword [eax], xmm1, 5

haddpd xmm1, xmm2
haddpd xmm1, [eax]
haddpd xmm1, dqword [eax]
vhaddpd xmm1, xmm2
vhaddpd xmm1, [eax]
vhaddpd xmm1, dqword [eax]
vhaddpd xmm1, xmm2, xmm3
vhaddpd xmm1, xmm2, [eax]
vhaddpd xmm1, xmm2, dqword [eax]
vhaddpd ymm1, ymm2, ymm3
vhaddpd ymm1, ymm2, [eax]
vhaddpd ymm1, ymm2, yword [eax]

haddps xmm1, xmm2
haddps xmm1, [eax]
haddps xmm1, dqword [eax]
vhaddps xmm1, xmm2
vhaddps xmm1, [eax]
vhaddps xmm1, dqword [eax]
vhaddps xmm1, xmm2, xmm3
vhaddps xmm1, xmm2, [eax]
vhaddps xmm1, xmm2, dqword [eax]
vhaddps ymm1, ymm2, ymm3
vhaddps ymm1, ymm2, [eax]
vhaddps ymm1, ymm2, yword [eax]

hsubpd xmm1, xmm2
hsubpd xmm1, [eax]
hsubpd xmm1, dqword [eax]
vhsubpd xmm1, xmm2
vhsubpd xmm1, [eax]
vhsubpd xmm1, dqword [eax]
vhsubpd xmm1, xmm2, xmm3
vhsubpd xmm1, xmm2, [eax]
vhsubpd xmm1, xmm2, dqword [eax]
vhsubpd ymm1, ymm2, ymm3
vhsubpd ymm1, ymm2, [eax]
vhsubpd ymm1, ymm2, yword [eax]

hsubps xmm1, xmm2
hsubps xmm1, [eax]
hsubps xmm1, dqword [eax]
vhsubps xmm1, xmm2
vhsubps xmm1, [eax]
vhsubps xmm1, dqword [eax]
vhsubps xmm1, xmm2, xmm3
vhsubps xmm1, xmm2, [eax]
vhsubps xmm1, xmm2, dqword [eax]
vhsubps ymm1, ymm2, ymm3
vhsubps ymm1, ymm2, [eax]
vhsubps ymm1, ymm2, yword [eax]

vinsertf128 ymm1, ymm2, xmm3, 5
vinsertf128 ymm1, ymm2, [eax], byte 5
vinsertf128 ymm1, ymm2, dqword [eax], 5

insertps xmm1, xmm2, 5
insertps xmm1, [eax], byte 5
insertps xmm1, dword [eax], 5
vinsertps xmm1, xmm2, 5
vinsertps xmm1, [eax], byte 5
vinsertps xmm1, dword [eax], 5
vinsertps xmm1, xmm2, xmm3, 5
vinsertps xmm1, xmm2, [eax], byte 5
vinsertps xmm1, xmm2, dword [eax], 5

lddqu xmm1, [eax]
lddqu xmm1, dqword [eax]
vlddqu xmm1, [eax]
vlddqu xmm1, dqword [eax]
vlddqu ymm1, [eax]
vlddqu ymm1, yword [eax]

ldmxcsr [eax]
ldmxcsr dword [eax]
vldmxcsr [eax]
vldmxcsr dword [eax]

maskmovdqu xmm1, xmm2
vmaskmovdqu xmm1, xmm2

vmaskmovps xmm1, xmm2, [eax]
vmaskmovps xmm1, xmm2, dqword [eax]
vmaskmovps ymm1, ymm2, [eax]
vmaskmovps ymm1, ymm2, yword [eax]
vmaskmovps [eax], xmm2, xmm3
vmaskmovps dqword [eax], xmm2, xmm3
vmaskmovps [eax], ymm2, ymm3
vmaskmovps yword [eax], ymm2, ymm3

vmaskmovpd xmm1, xmm2, [eax]
vmaskmovpd xmm1, xmm2, dqword [eax]
vmaskmovpd ymm1, ymm2, [eax]
vmaskmovpd ymm1, ymm2, yword [eax]
vmaskmovpd [eax], xmm2, xmm3
vmaskmovpd dqword [eax], xmm2, xmm3
vmaskmovpd [eax], ymm2, ymm3
vmaskmovpd yword [eax], ymm2, ymm3

maxpd xmm1, xmm2
maxpd xmm1, [eax]
maxpd xmm1, dqword [eax]
vmaxpd xmm1, xmm2
vmaxpd xmm1, [eax]
vmaxpd xmm1, dqword [eax]
vmaxpd xmm1, xmm2, xmm3
vmaxpd xmm1, xmm2, [eax]
vmaxpd xmm1, xmm2, dqword [eax]
vmaxpd ymm1, ymm2, ymm3
vmaxpd ymm1, ymm2, [eax]
vmaxpd ymm1, ymm2, yword [eax]

maxps xmm1, xmm2
maxps xmm1, [eax]
maxps xmm1, dqword [eax]
vmaxps xmm1, xmm2
vmaxps xmm1, [eax]
vmaxps xmm1, dqword [eax]
vmaxps xmm1, xmm2, xmm3
vmaxps xmm1, xmm2, [eax]
vmaxps xmm1, xmm2, dqword [eax]
vmaxps ymm1, ymm2, ymm3
vmaxps ymm1, ymm2, [eax]
vmaxps ymm1, ymm2, yword [eax]

maxsd xmm1, xmm2
maxsd xmm1, [eax]
maxsd xmm1, qword [eax]
vmaxsd xmm1, xmm2
vmaxsd xmm1, [eax]
vmaxsd xmm1, qword [eax]
vmaxsd xmm1, xmm2, xmm3
vmaxsd xmm1, xmm2, [eax]
vmaxsd xmm1, xmm2, qword [eax]

maxss xmm1, xmm2
maxss xmm1, [eax]
maxss xmm1, dword [eax]
vmaxss xmm1, xmm2
vmaxss xmm1, [eax]
vmaxss xmm1, dword [eax]
vmaxss xmm1, xmm2, xmm3
vmaxss xmm1, xmm2, [eax]
vmaxss xmm1, xmm2, dword [eax]

minpd xmm1, xmm2
minpd xmm1, [eax]
minpd xmm1, dqword [eax]
vminpd xmm1, xmm2
vminpd xmm1, [eax]
vminpd xmm1, dqword [eax]
vminpd xmm1, xmm2, xmm3
vminpd xmm1, xmm2, [eax]
vminpd xmm1, xmm2, dqword [eax]
vminpd ymm1, ymm2, ymm3
vminpd ymm1, ymm2, [eax]
vminpd ymm1, ymm2, yword [eax]

minps xmm1, xmm2
minps xmm1, [eax]
minps xmm1, dqword [eax]
vminps xmm1, xmm2
vminps xmm1, [eax]
vminps xmm1, dqword [eax]
vminps xmm1, xmm2, xmm3
vminps xmm1, xmm2, [eax]
vminps xmm1, xmm2, dqword [eax]
vminps ymm1, ymm2, ymm3
vminps ymm1, ymm2, [eax]
vminps ymm1, ymm2, yword [eax]

minsd xmm1, xmm2
minsd xmm1, [eax]
minsd xmm1, qword [eax]
vminsd xmm1, xmm2
vminsd xmm1, [eax]
vminsd xmm1, qword [eax]
vminsd xmm1, xmm2, xmm3
vminsd xmm1, xmm2, [eax]
vminsd xmm1, xmm2, qword [eax]

minss xmm1, xmm2
minss xmm1, [eax]
minss xmm1, dword [eax]
vminss xmm1, xmm2
vminss xmm1, [eax]
vminss xmm1, dword [eax]
vminss xmm1, xmm2, xmm3
vminss xmm1, xmm2, [eax]
vminss xmm1, xmm2, dword [eax]

movapd xmm1, xmm2
movapd xmm1, [eax]
movapd xmm1, dqword [eax]
vmovapd xmm1, xmm2
vmovapd xmm1, [eax]
vmovapd xmm1, dqword [eax]
movapd [eax], xmm2
movapd dqword [eax], xmm2
vmovapd [eax], xmm2
vmovapd dqword [eax], xmm2
vmovapd ymm1, ymm2
vmovapd ymm1, [eax]
vmovapd ymm1, yword [eax]
vmovapd [eax], ymm2
vmovapd yword [eax], ymm2

movaps xmm1, xmm2
movaps xmm1, [eax]
movaps xmm1, dqword [eax]
vmovaps xmm1, xmm2
vmovaps xmm1, [eax]
vmovaps xmm1, dqword [eax]
movaps [eax], xmm2
movaps dqword [eax], xmm2
vmovaps [eax], xmm2
vmovaps dqword [eax], xmm2
vmovaps ymm1, ymm2
vmovaps ymm1, [eax]
vmovaps ymm1, yword [eax]
vmovaps [eax], ymm2
vmovaps yword [eax], ymm2

movd xmm1, eax
movd xmm1, [eax]
movd xmm1, dword [eax]
vmovd xmm1, eax
vmovd xmm1, [eax]
vmovd xmm1, dword [eax]
movd eax, xmm2
movd [eax], xmm2
movd dword [eax], xmm2
vmovd eax, xmm2
vmovd [eax], xmm2
vmovd dword [eax], xmm2

movq xmm1, [eax]
movq xmm1, qword [eax]
vmovq xmm1, [eax]
vmovq xmm1, qword [eax]
movd eax, xmm2
movq [eax], xmm2
movq qword [eax], xmm2
vmovd eax, xmm2
vmovq [eax], xmm2
vmovq qword [eax], xmm2

movq xmm1, xmm2
movq xmm1, [eax]
movq xmm1, qword [eax]
vmovq xmm1, xmm2
vmovq xmm1, [eax]
vmovq xmm1, qword [eax]
movq [eax], xmm1
movq qword [eax], xmm1
vmovq [eax], xmm1
vmovq qword [eax], xmm1

movddup xmm1, xmm2
movddup xmm1, [eax]
movddup xmm1, qword [eax]
vmovddup xmm1, xmm2
vmovddup xmm1, [eax]
vmovddup xmm1, qword [eax]
vmovddup ymm1, ymm2
vmovddup ymm1, [eax]
vmovddup ymm1, yword [eax]

movdqa xmm1, xmm2
movdqa xmm1, [eax]
movdqa xmm1, dqword [eax]
movdqa [eax], xmm2
movdqa dqword [eax], xmm2
vmovdqa xmm1, xmm2
vmovdqa xmm1, [eax]
vmovdqa xmm1, dqword [eax]
vmovdqa [eax], xmm2
vmovdqa dqword [eax], xmm2
vmovdqa ymm1, ymm2
vmovdqa ymm1, [eax]
vmovdqa ymm1, yword [eax]
vmovdqa [eax], ymm2
vmovdqa yword [eax], ymm2

movdqu xmm1, xmm2
movdqu xmm1, [eax]
movdqu xmm1, dqword [eax]
movdqu [eax], xmm2
movdqu dqword [eax], xmm2
vmovdqu xmm1, xmm2
vmovdqu xmm1, [eax]
vmovdqu xmm1, dqword [eax]
vmovdqu [eax], xmm2
vmovdqu dqword [eax], xmm2
vmovdqu ymm1, ymm2
vmovdqu ymm1, [eax]
vmovdqu ymm1, yword [eax]
vmovdqu [eax], ymm2
vmovdqu yword [eax], ymm2

movhlps xmm1, xmm2
vmovhlps xmm1, xmm2
vmovhlps xmm1, xmm2, xmm3

movhpd xmm1, [eax]
movhpd xmm1, qword [eax]
vmovhpd xmm1, [eax]
vmovhpd xmm1, qword [eax]
vmovhpd xmm1, xmm2, [eax]
vmovhpd xmm1, xmm2, qword [eax]
movhpd [eax], xmm2
movhpd qword [eax], xmm2
vmovhpd [eax], xmm2
vmovhpd qword [eax], xmm2

movhps xmm1, [eax]
movhps xmm1, qword [eax]
vmovhps xmm1, [eax]
vmovhps xmm1, qword [eax]
vmovhps xmm1, xmm2, [eax]
vmovhps xmm1, xmm2, qword [eax]
movhps [eax], xmm2
movhps qword [eax], xmm2
vmovhps [eax], xmm2
vmovhps qword [eax], xmm2

movhlps xmm1, xmm2
vmovhlps xmm1, xmm2
vmovhlps xmm1, xmm2, xmm3

movlpd xmm1, [eax]
movlpd xmm1, qword [eax]
vmovlpd xmm1, [eax]
vmovlpd xmm1, qword [eax]
vmovlpd xmm1, xmm2, [eax]
vmovlpd xmm1, xmm2, qword [eax]
movlpd [eax], xmm2
movlpd qword [eax], xmm2
vmovlpd [eax], xmm2
vmovlpd qword [eax], xmm2

movlps xmm1, [eax]
movlps xmm1, qword [eax]
vmovlps xmm1, [eax]
vmovlps xmm1, qword [eax]
vmovlps xmm1, xmm2, [eax]
vmovlps xmm1, xmm2, qword [eax]
movlps [eax], xmm2
movlps qword [eax], xmm2
vmovlps [eax], xmm2
vmovlps qword [eax], xmm2

movmskpd eax, xmm2
movmskpd eax, xmm2
vmovmskpd eax, xmm2
vmovmskpd eax, xmm2
vmovmskpd eax, ymm2
vmovmskpd eax, ymm2

movmskps eax, xmm2
movmskps eax, xmm2
vmovmskps eax, xmm2
vmovmskps eax, xmm2
vmovmskps eax, ymm2
vmovmskps eax, ymm2

movntdq [eax], xmm1
movntdq dqword [eax], xmm1
vmovntdq [eax], xmm1
vmovntdq dqword [eax], xmm1
vmovntdq [eax], ymm1
vmovntdq yword [eax], ymm1

movntdqa xmm1, [eax]
movntdqa xmm1, dqword [eax]
vmovntdqa xmm1, [eax]
vmovntdqa xmm1, dqword [eax]

movntpd [eax], xmm1
movntpd dqword [eax], xmm1
vmovntpd [eax], xmm1
vmovntpd dqword [eax], xmm1
vmovntpd [eax], ymm1
vmovntpd yword [eax], ymm1

movntps [eax], xmm1
movntps dqword [eax], xmm1
vmovntps [eax], xmm1
vmovntps dqword [eax], xmm1
vmovntps [eax], ymm1
vmovntps yword [eax], ymm1

movsd xmm1, xmm2
vmovsd xmm1, xmm2
vmovsd xmm1, xmm2, xmm3
movsd xmm1, [eax]
movsd xmm1, qword [eax]
vmovsd xmm1, [eax]
vmovsd xmm1, qword [eax]
movsd [eax], xmm2
movsd qword [eax], xmm2
vmovsd [eax], xmm2
vmovsd qword [eax], xmm2

movshdup xmm1, xmm2
movshdup xmm1, [eax]
movshdup xmm1, dqword [eax]
vmovshdup xmm1, xmm2
vmovshdup xmm1, [eax]
vmovshdup xmm1, dqword [eax]
vmovshdup ymm1, ymm2
vmovshdup ymm1, [eax]
vmovshdup ymm1, yword [eax]

movsldup xmm1, xmm2
movsldup xmm1, [eax]
movsldup xmm1, dqword [eax]
vmovsldup xmm1, xmm2
vmovsldup xmm1, [eax]
vmovsldup xmm1, dqword [eax]
vmovsldup ymm1, ymm2
vmovsldup ymm1, [eax]
vmovsldup ymm1, yword [eax]

movss xmm1, xmm2
vmovss xmm1, xmm2
vmovss xmm1, xmm2, xmm3
movss xmm1, [eax]
movss xmm1, dword [eax]
vmovss xmm1, [eax]
vmovss xmm1, dword [eax]
movss [eax], xmm2
movss dword [eax], xmm2
vmovss [eax], xmm2
vmovss dword [eax], xmm2

movupd xmm1, xmm2
movupd xmm1, [eax]
movupd xmm1, dqword [eax]
vmovupd xmm1, xmm2
vmovupd xmm1, [eax]
vmovupd xmm1, dqword [eax]
movupd [eax], xmm2
movupd dqword [eax], xmm2
vmovupd [eax], xmm2
vmovupd dqword [eax], xmm2
vmovupd ymm1, ymm2
vmovupd ymm1, [eax]
vmovupd ymm1, yword [eax]
vmovupd [eax], ymm2
vmovupd yword [eax], ymm2

movups xmm1, xmm2
movups xmm1, [eax]
movups xmm1, dqword [eax]
vmovups xmm1, xmm2
vmovups xmm1, [eax]
vmovups xmm1, dqword [eax]
movups [eax], xmm2
movups dqword [eax], xmm2
vmovups [eax], xmm2
vmovups dqword [eax], xmm2
vmovups ymm1, ymm2
vmovups ymm1, [eax]
vmovups ymm1, yword [eax]
vmovups [eax], ymm2
vmovups yword [eax], ymm2

mpsadbw xmm1, xmm2, 5
mpsadbw xmm1, [eax], byte 5
mpsadbw xmm1, dqword [eax], 5
vmpsadbw xmm1, xmm2, 5
vmpsadbw xmm1, [eax], byte 5
vmpsadbw xmm1, dqword [eax], 5
vmpsadbw xmm1, xmm2, xmm3, 5
vmpsadbw xmm1, xmm2, [eax], byte 5
vmpsadbw xmm1, xmm2, dqword [eax], 5

mulpd xmm1, xmm2
mulpd xmm1, [eax]
mulpd xmm1, dqword [eax]
vmulpd xmm1, xmm2
vmulpd xmm1, [eax]
vmulpd xmm1, dqword [eax]
vmulpd xmm1, xmm2, xmm3
vmulpd xmm1, xmm2, [eax]
vmulpd xmm1, xmm2, dqword [eax]
vmulpd ymm1, ymm2, ymm3
vmulpd ymm1, ymm2, [eax]
vmulpd ymm1, ymm2, yword [eax]

mulps xmm1, xmm2
mulps xmm1, [eax]
mulps xmm1, dqword [eax]
vmulps xmm1, xmm2
vmulps xmm1, [eax]
vmulps xmm1, dqword [eax]
vmulps xmm1, xmm2, xmm3
vmulps xmm1, xmm2, [eax]
vmulps xmm1, xmm2, dqword [eax]
vmulps ymm1, ymm2, ymm3
vmulps ymm1, ymm2, [eax]
vmulps ymm1, ymm2, yword [eax]

mulsd xmm1, xmm2
mulsd xmm1, [eax]
mulsd xmm1, qword [eax]
vmulsd xmm1, xmm2
vmulsd xmm1, [eax]
vmulsd xmm1, qword [eax]
vmulsd xmm1, xmm2, xmm3
vmulsd xmm1, xmm2, [eax]
vmulsd xmm1, xmm2, qword [eax]

mulss xmm1, xmm2
mulss xmm1, [eax]
mulss xmm1, dword [eax]
vmulss xmm1, xmm2
vmulss xmm1, [eax]
vmulss xmm1, dword [eax]
vmulss xmm1, xmm2, xmm3
vmulss xmm1, xmm2, [eax]
vmulss xmm1, xmm2, dword [eax]

orpd xmm1, xmm2
orpd xmm1, [eax]
orpd xmm1, dqword [eax]
vorpd xmm1, xmm2
vorpd xmm1, [eax]
vorpd xmm1, dqword [eax]
vorpd xmm1, xmm2, xmm3
vorpd xmm1, xmm2, [eax]
vorpd xmm1, xmm2, dqword [eax]
vorpd ymm1, ymm2, ymm3
vorpd ymm1, ymm2, [eax]
vorpd ymm1, ymm2, yword [eax]

orps xmm1, xmm2
orps xmm1, [eax]
orps xmm1, dqword [eax]
vorps xmm1, xmm2
vorps xmm1, [eax]
vorps xmm1, dqword [eax]
vorps xmm1, xmm2, xmm3
vorps xmm1, xmm2, [eax]
vorps xmm1, xmm2, dqword [eax]
vorps ymm1, ymm2, ymm3
vorps ymm1, ymm2, [eax]
vorps ymm1, ymm2, yword [eax]

pabsb xmm1, xmm2
pabsb xmm1, [eax]
pabsb xmm1, dqword [eax]
vpabsb xmm1, xmm2
vpabsb xmm1, [eax]
vpabsb xmm1, dqword [eax]

pabsw xmm1, xmm2
pabsw xmm1, [eax]
pabsw xmm1, dqword [eax]
vpabsw xmm1, xmm2
vpabsw xmm1, [eax]
vpabsw xmm1, dqword [eax]

pabsd xmm1, xmm2
pabsd xmm1, [eax]
pabsd xmm1, dqword [eax]
vpabsd xmm1, xmm2
vpabsd xmm1, [eax]
vpabsd xmm1, dqword [eax]

packsswb xmm1, xmm2
packsswb xmm1, [eax]
packsswb xmm1, dqword [eax]
vpacksswb xmm1, xmm2
vpacksswb xmm1, [eax]
vpacksswb xmm1, dqword [eax]
vpacksswb xmm1, xmm2, xmm3
vpacksswb xmm1, xmm2, [eax]
vpacksswb xmm1, xmm2, dqword [eax]

packssdw xmm1, xmm2
packssdw xmm1, [eax]
packssdw xmm1, dqword [eax]
vpackssdw xmm1, xmm2
vpackssdw xmm1, [eax]
vpackssdw xmm1, dqword [eax]
vpackssdw xmm1, xmm2, xmm3
vpackssdw xmm1, xmm2, [eax]
vpackssdw xmm1, xmm2, dqword [eax]

packuswb xmm1, xmm2
packuswb xmm1, [eax]
packuswb xmm1, dqword [eax]
vpackuswb xmm1, xmm2
vpackuswb xmm1, [eax]
vpackuswb xmm1, dqword [eax]
vpackuswb xmm1, xmm2, xmm3
vpackuswb xmm1, xmm2, [eax]
vpackuswb xmm1, xmm2, dqword [eax]

packusdw xmm1, xmm2
packusdw xmm1, [eax]
packusdw xmm1, dqword [eax]
vpackusdw xmm1, xmm2
vpackusdw xmm1, [eax]
vpackusdw xmm1, dqword [eax]
vpackusdw xmm1, xmm2, xmm3
vpackusdw xmm1, xmm2, [eax]
vpackusdw xmm1, xmm2, dqword [eax]

paddb xmm1, xmm2
paddb xmm1, [eax]
paddb xmm1, dqword [eax]
vpaddb xmm1, xmm2
vpaddb xmm1, [eax]
vpaddb xmm1, dqword [eax]
vpaddb xmm1, xmm2, xmm3
vpaddb xmm1, xmm2, [eax]
vpaddb xmm1, xmm2, dqword [eax]

paddw xmm1, xmm2
paddw xmm1, [eax]
paddw xmm1, dqword [eax]
vpaddw xmm1, xmm2
vpaddw xmm1, [eax]
vpaddw xmm1, dqword [eax]
vpaddw xmm1, xmm2, xmm3
vpaddw xmm1, xmm2, [eax]
vpaddw xmm1, xmm2, dqword [eax]

paddd xmm1, xmm2
paddd xmm1, [eax]
paddd xmm1, dqword [eax]
vpaddd xmm1, xmm2
vpaddd xmm1, [eax]
vpaddd xmm1, dqword [eax]
vpaddd xmm1, xmm2, xmm3
vpaddd xmm1, xmm2, [eax]
vpaddd xmm1, xmm2, dqword [eax]

paddq xmm1, xmm2
paddq xmm1, [eax]
paddq xmm1, dqword [eax]
vpaddq xmm1, xmm2
vpaddq xmm1, [eax]
vpaddq xmm1, dqword [eax]
vpaddq xmm1, xmm2, xmm3
vpaddq xmm1, xmm2, [eax]
vpaddq xmm1, xmm2, dqword [eax]

paddsb xmm1, xmm2
paddsb xmm1, [eax]
paddsb xmm1, dqword [eax]
vpaddsb xmm1, xmm2
vpaddsb xmm1, [eax]
vpaddsb xmm1, dqword [eax]
vpaddsb xmm1, xmm2, xmm3
vpaddsb xmm1, xmm2, [eax]
vpaddsb xmm1, xmm2, dqword [eax]

paddsw xmm1, xmm2
paddsw xmm1, [eax]
paddsw xmm1, dqword [eax]
vpaddsw xmm1, xmm2
vpaddsw xmm1, [eax]
vpaddsw xmm1, dqword [eax]
vpaddsw xmm1, xmm2, xmm3
vpaddsw xmm1, xmm2, [eax]
vpaddsw xmm1, xmm2, dqword [eax]

paddusb xmm1, xmm2
paddusb xmm1, [eax]
paddusb xmm1, dqword [eax]
vpaddusb xmm1, xmm2
vpaddusb xmm1, [eax]
vpaddusb xmm1, dqword [eax]
vpaddusb xmm1, xmm2, xmm3
vpaddusb xmm1, xmm2, [eax]
vpaddusb xmm1, xmm2, dqword [eax]

paddusw xmm1, xmm2
paddusw xmm1, [eax]
paddusw xmm1, dqword [eax]
vpaddusw xmm1, xmm2
vpaddusw xmm1, [eax]
vpaddusw xmm1, dqword [eax]
vpaddusw xmm1, xmm2, xmm3
vpaddusw xmm1, xmm2, [eax]
vpaddusw xmm1, xmm2, dqword [eax]

palignr xmm1, xmm2, 5
palignr xmm1, [eax], byte 5
palignr xmm1, dqword [eax], 5
vpalignr xmm1, xmm2, 5
vpalignr xmm1, [eax], byte 5
vpalignr xmm1, dqword [eax], 5
vpalignr xmm1, xmm2, xmm3, 5
vpalignr xmm1, xmm2, [eax], byte 5
vpalignr xmm1, xmm2, dqword [eax], 5

pand xmm1, xmm2
pand xmm1, [eax]
pand xmm1, dqword [eax]
vpand xmm1, xmm2
vpand xmm1, [eax]
vpand xmm1, dqword [eax]
vpand xmm1, xmm2, xmm3
vpand xmm1, xmm2, [eax]
vpand xmm1, xmm2, dqword [eax]

pandn xmm1, xmm2
pandn xmm1, [eax]
pandn xmm1, dqword [eax]
vpandn xmm1, xmm2
vpandn xmm1, [eax]
vpandn xmm1, dqword [eax]
vpandn xmm1, xmm2, xmm3
vpandn xmm1, xmm2, [eax]
vpandn xmm1, xmm2, dqword [eax]

pavgb xmm1, xmm2
pavgb xmm1, [eax]
pavgb xmm1, dqword [eax]
vpavgb xmm1, xmm2
vpavgb xmm1, [eax]
vpavgb xmm1, dqword [eax]
vpavgb xmm1, xmm2, xmm3
vpavgb xmm1, xmm2, [eax]
vpavgb xmm1, xmm2, dqword [eax]

pavgw xmm1, xmm2
pavgw xmm1, [eax]
pavgw xmm1, dqword [eax]
vpavgw xmm1, xmm2
vpavgw xmm1, [eax]
vpavgw xmm1, dqword [eax]
vpavgw xmm1, xmm2, xmm3
vpavgw xmm1, xmm2, [eax]
vpavgw xmm1, xmm2, dqword [eax]

; implicit XMM0 cannot be VEX encoded
pblendvb xmm1, xmm2
pblendvb xmm1, [eax]
pblendvb xmm1, dqword [eax]
pblendvb xmm1, xmm2, xmm0
pblendvb xmm1, [eax], xmm0
pblendvb xmm1, dqword [eax], xmm0
vpblendvb xmm1, xmm2, xmm3, xmm4
vpblendvb xmm1, xmm2, [eax], xmm4
vpblendvb xmm1, xmm2, dqword [eax], xmm4

pblendw xmm1, xmm2, 5
pblendw xmm1, [eax], byte 5
pblendw xmm1, dqword [eax], 5
vpblendw xmm1, xmm2, 5
vpblendw xmm1, [eax], byte 5
vpblendw xmm1, dqword [eax], 5
vpblendw xmm1, xmm2, xmm3, 5
vpblendw xmm1, xmm2, [eax], byte 5
vpblendw xmm1, xmm2, dqword [eax], 5

pcmpestri xmm1, xmm2, 5
pcmpestri xmm1, [eax], byte 5
pcmpestri xmm1, dqword [eax], 5
vpcmpestri xmm1, xmm2, 5
vpcmpestri xmm1, [eax], byte 5
vpcmpestri xmm1, dqword [eax], 5

pcmpestrm xmm1, xmm2, 5
pcmpestrm xmm1, [eax], byte 5
pcmpestrm xmm1, dqword [eax], 5
vpcmpestrm xmm1, xmm2, 5
vpcmpestrm xmm1, [eax], byte 5
vpcmpestrm xmm1, dqword [eax], 5

pcmpistri xmm1, xmm2, 5
pcmpistri xmm1, [eax], byte 5
pcmpistri xmm1, dqword [eax], 5
vpcmpistri xmm1, xmm2, 5
vpcmpistri xmm1, [eax], byte 5
vpcmpistri xmm1, dqword [eax], 5

pcmpistrm xmm1, xmm2, 5
pcmpistrm xmm1, [eax], byte 5
pcmpistrm xmm1, dqword [eax], 5
vpcmpistrm xmm1, xmm2, 5
vpcmpistrm xmm1, [eax], byte 5
vpcmpistrm xmm1, dqword [eax], 5

pcmpeqb xmm1, xmm2
pcmpeqb xmm1, [eax]
pcmpeqb xmm1, dqword [eax]
vpcmpeqb xmm1, xmm2
vpcmpeqb xmm1, [eax]
vpcmpeqb xmm1, dqword [eax]
vpcmpeqb xmm1, xmm2, xmm3
vpcmpeqb xmm1, xmm2, [eax]
vpcmpeqb xmm1, xmm2, dqword [eax]

pcmpeqw xmm1, xmm2
pcmpeqw xmm1, [eax]
pcmpeqw xmm1, dqword [eax]
vpcmpeqw xmm1, xmm2
vpcmpeqw xmm1, [eax]
vpcmpeqw xmm1, dqword [eax]
vpcmpeqw xmm1, xmm2, xmm3
vpcmpeqw xmm1, xmm2, [eax]
vpcmpeqw xmm1, xmm2, dqword [eax]

pcmpeqd xmm1, xmm2
pcmpeqd xmm1, [eax]
pcmpeqd xmm1, dqword [eax]
vpcmpeqd xmm1, xmm2
vpcmpeqd xmm1, [eax]
vpcmpeqd xmm1, dqword [eax]
vpcmpeqd xmm1, xmm2, xmm3
vpcmpeqd xmm1, xmm2, [eax]
vpcmpeqd xmm1, xmm2, dqword [eax]

pcmpeqq xmm1, xmm2
pcmpeqq xmm1, [eax]
pcmpeqq xmm1, dqword [eax]
vpcmpeqq xmm1, xmm2
vpcmpeqq xmm1, [eax]
vpcmpeqq xmm1, dqword [eax]
vpcmpeqq xmm1, xmm2, xmm3
vpcmpeqq xmm1, xmm2, [eax]
vpcmpeqq xmm1, xmm2, dqword [eax]

pcmpgtb xmm1, xmm2
pcmpgtb xmm1, [eax]
pcmpgtb xmm1, dqword [eax]
vpcmpgtb xmm1, xmm2
vpcmpgtb xmm1, [eax]
vpcmpgtb xmm1, dqword [eax]
vpcmpgtb xmm1, xmm2, xmm3
vpcmpgtb xmm1, xmm2, [eax]
vpcmpgtb xmm1, xmm2, dqword [eax]

pcmpgtw xmm1, xmm2
pcmpgtw xmm1, [eax]
pcmpgtw xmm1, dqword [eax]
vpcmpgtw xmm1, xmm2
vpcmpgtw xmm1, [eax]
vpcmpgtw xmm1, dqword [eax]
vpcmpgtw xmm1, xmm2, xmm3
vpcmpgtw xmm1, xmm2, [eax]
vpcmpgtw xmm1, xmm2, dqword [eax]

pcmpgtd xmm1, xmm2
pcmpgtd xmm1, [eax]
pcmpgtd xmm1, dqword [eax]
vpcmpgtd xmm1, xmm2
vpcmpgtd xmm1, [eax]
vpcmpgtd xmm1, dqword [eax]
vpcmpgtd xmm1, xmm2, xmm3
vpcmpgtd xmm1, xmm2, [eax]
vpcmpgtd xmm1, xmm2, dqword [eax]

pcmpgtq xmm1, xmm2
pcmpgtq xmm1, [eax]
pcmpgtq xmm1, dqword [eax]
vpcmpgtq xmm1, xmm2
vpcmpgtq xmm1, [eax]
vpcmpgtq xmm1, dqword [eax]
vpcmpgtq xmm1, xmm2, xmm3
vpcmpgtq xmm1, xmm2, [eax]
vpcmpgtq xmm1, xmm2, dqword [eax]

vpermilpd xmm1, xmm2, xmm3
vpermilpd xmm1, xmm2, [eax]
vpermilpd xmm1, xmm2, dqword [eax]
vpermilpd ymm1, ymm2, ymm3
vpermilpd ymm1, ymm2, [eax]
vpermilpd ymm1, ymm2, yword [eax]
vpermilpd xmm1, [eax], byte 5
vpermilpd xmm1, dqword [eax], 5
vpermilpd ymm1, [eax], byte 5
vpermilpd ymm1, yword [eax], 5

vpermilps xmm1, xmm2, xmm3
vpermilps xmm1, xmm2, [eax]
vpermilps xmm1, xmm2, dqword [eax]
vpermilps ymm1, ymm2, ymm3
vpermilps ymm1, ymm2, [eax]
vpermilps ymm1, ymm2, yword [eax]
vpermilps xmm1, [eax], byte 5
vpermilps xmm1, dqword [eax], 5
vpermilps ymm1, [eax], byte 5
vpermilps ymm1, yword [eax], 5

vperm2f128 ymm1, ymm2, ymm3, 5
vperm2f128 ymm1, ymm2, [eax], byte 5
vperm2f128 ymm1, ymm2, yword [eax], 5

pextrb eax, xmm2, 5
pextrb eax, xmm2, byte 5
pextrb eax, xmm2, 5
pextrb eax, xmm2, byte 5
pextrb byte [eax], xmm2, 5
pextrb [eax], xmm2, byte 5
vpextrb eax, xmm2, 5
vpextrb eax, xmm2, byte 5
vpextrb eax, xmm2, 5
vpextrb eax, xmm2, byte 5
vpextrb byte [eax], xmm2, 5
vpextrb [eax], xmm2, byte 5

pextrw eax, xmm2, 5
pextrw eax, xmm2, byte 5
pextrw eax, xmm2, 5
pextrw eax, xmm2, byte 5
pextrw word [eax], xmm2, 5
pextrw [eax], xmm2, byte 5
vpextrw eax, xmm2, 5
vpextrw eax, xmm2, byte 5
vpextrw eax, xmm2, 5
vpextrw eax, xmm2, byte 5
vpextrw word [eax], xmm2, 5
vpextrw [eax], xmm2, byte 5

pextrd eax, xmm2, 5
pextrd eax, xmm2, byte 5
pextrd dword [eax], xmm2, 5
pextrd [eax], xmm2, byte 5
vpextrd eax, xmm2, 5
vpextrd eax, xmm2, byte 5
vpextrd dword [eax], xmm2, 5
vpextrd [eax], xmm2, byte 5

pextrd eax, xmm2, 5
pextrd eax, xmm2, byte 5
pextrd dword [eax], xmm2, 5
pextrd [eax], xmm2, byte 5
vpextrd eax, xmm2, 5
vpextrd dword [eax], xmm2, 5
vpextrd [eax], xmm2, byte 5

phaddw xmm1, xmm2
phaddw xmm1, [eax]
phaddw xmm1, dqword [eax]
vphaddw xmm1, xmm2
vphaddw xmm1, [eax]
vphaddw xmm1, dqword [eax]
vphaddw xmm1, xmm2, xmm3
vphaddw xmm1, xmm2, [eax]
vphaddw xmm1, xmm2, dqword [eax]

phaddd xmm1, xmm2
phaddd xmm1, [eax]
phaddd xmm1, dqword [eax]
vphaddd xmm1, xmm2
vphaddd xmm1, [eax]
vphaddd xmm1, dqword [eax]
vphaddd xmm1, xmm2, xmm3
vphaddd xmm1, xmm2, [eax]
vphaddd xmm1, xmm2, dqword [eax]

phaddsw xmm1, xmm2
phaddsw xmm1, [eax]
phaddsw xmm1, dqword [eax]
vphaddsw xmm1, xmm2
vphaddsw xmm1, [eax]
vphaddsw xmm1, dqword [eax]
vphaddsw xmm1, xmm2, xmm3
vphaddsw xmm1, xmm2, [eax]
vphaddsw xmm1, xmm2, dqword [eax]

phminposuw xmm1, xmm2
phminposuw xmm1, [eax]
phminposuw xmm1, dqword [eax]
vphminposuw xmm1, xmm2
vphminposuw xmm1, [eax]
vphminposuw xmm1, dqword [eax]

phsubw xmm1, xmm2
phsubw xmm1, [eax]
phsubw xmm1, dqword [eax]
vphsubw xmm1, xmm2
vphsubw xmm1, [eax]
vphsubw xmm1, dqword [eax]
vphsubw xmm1, xmm2, xmm3
vphsubw xmm1, xmm2, [eax]
vphsubw xmm1, xmm2, dqword [eax]

phsubd xmm1, xmm2
phsubd xmm1, [eax]
phsubd xmm1, dqword [eax]
vphsubd xmm1, xmm2
vphsubd xmm1, [eax]
vphsubd xmm1, dqword [eax]
vphsubd xmm1, xmm2, xmm3
vphsubd xmm1, xmm2, [eax]
vphsubd xmm1, xmm2, dqword [eax]

phsubsw xmm1, xmm2
phsubsw xmm1, [eax]
phsubsw xmm1, dqword [eax]
vphsubsw xmm1, xmm2
vphsubsw xmm1, [eax]
vphsubsw xmm1, dqword [eax]
vphsubsw xmm1, xmm2, xmm3
vphsubsw xmm1, xmm2, [eax]
vphsubsw xmm1, xmm2, dqword [eax]

pinsrb xmm1, eax, 5
pinsrb xmm1, byte [eax], 5
pinsrb xmm1, [eax], byte 5
vpinsrb xmm1, eax, 5
vpinsrb xmm1, byte [eax], 5
vpinsrb xmm1, [eax], byte 5
vpinsrb xmm1, xmm2, eax, 5
vpinsrb xmm1, xmm2, byte [eax], 5
vpinsrb xmm1, xmm2, [eax], byte 5

pinsrw xmm1, eax, 5
pinsrw xmm1, word [eax], 5
pinsrw xmm1, [eax], byte 5
vpinsrw xmm1, eax, 5
vpinsrw xmm1, word [eax], 5
vpinsrw xmm1, [eax], byte 5
vpinsrw xmm1, xmm2, eax, 5
vpinsrw xmm1, xmm2, word [eax], 5
vpinsrw xmm1, xmm2, [eax], byte 5

pinsrd xmm1, eax, 5
pinsrd xmm1, dword [eax], 5
pinsrd xmm1, [eax], byte 5
vpinsrd xmm1, eax, 5
vpinsrd xmm1, dword [eax], 5
vpinsrd xmm1, [eax], byte 5
vpinsrd xmm1, xmm2, eax, 5
vpinsrd xmm1, xmm2, dword [eax], 5
vpinsrd xmm1, xmm2, [eax], byte 5

pinsrd xmm1, eax, 5
pinsrd xmm1, dword [eax], 5
pinsrd xmm1, [eax], byte 5
vpinsrd xmm1, eax, 5
vpinsrd xmm1, dword [eax], 5
vpinsrd xmm1, [eax], byte 5
vpinsrd xmm1, xmm2, eax, 5
vpinsrd xmm1, xmm2, dword [eax], 5
vpinsrd xmm1, xmm2, [eax], byte 5

pmaddwd xmm1, xmm2
pmaddwd xmm1, [eax]
pmaddwd xmm1, dqword [eax]
vpmaddwd xmm1, xmm2
vpmaddwd xmm1, [eax]
vpmaddwd xmm1, dqword [eax]
vpmaddwd xmm1, xmm2, xmm3
vpmaddwd xmm1, xmm2, [eax]
vpmaddwd xmm1, xmm2, dqword [eax]

pmaddubsw xmm1, xmm2
pmaddubsw xmm1, [eax]
pmaddubsw xmm1, dqword [eax]
vpmaddubsw xmm1, xmm2
vpmaddubsw xmm1, [eax]
vpmaddubsw xmm1, dqword [eax]
vpmaddubsw xmm1, xmm2, xmm3
vpmaddubsw xmm1, xmm2, [eax]
vpmaddubsw xmm1, xmm2, dqword [eax]

pmaxsb xmm1, xmm2
pmaxsb xmm1, [eax]
pmaxsb xmm1, dqword [eax]
vpmaxsb xmm1, xmm2
vpmaxsb xmm1, [eax]
vpmaxsb xmm1, dqword [eax]
vpmaxsb xmm1, xmm2, xmm3
vpmaxsb xmm1, xmm2, [eax]
vpmaxsb xmm1, xmm2, dqword [eax]

pmaxsw xmm1, xmm2
pmaxsw xmm1, [eax]
pmaxsw xmm1, dqword [eax]
vpmaxsw xmm1, xmm2
vpmaxsw xmm1, [eax]
vpmaxsw xmm1, dqword [eax]
vpmaxsw xmm1, xmm2, xmm3
vpmaxsw xmm1, xmm2, [eax]
vpmaxsw xmm1, xmm2, dqword [eax]

pmaxsd xmm1, xmm2
pmaxsd xmm1, [eax]
pmaxsd xmm1, dqword [eax]
vpmaxsd xmm1, xmm2
vpmaxsd xmm1, [eax]
vpmaxsd xmm1, dqword [eax]
vpmaxsd xmm1, xmm2, xmm3
vpmaxsd xmm1, xmm2, [eax]
vpmaxsd xmm1, xmm2, dqword [eax]

pmaxub xmm1, xmm2
pmaxub xmm1, [eax]
pmaxub xmm1, dqword [eax]
vpmaxub xmm1, xmm2
vpmaxub xmm1, [eax]
vpmaxub xmm1, dqword [eax]
vpmaxub xmm1, xmm2, xmm3
vpmaxub xmm1, xmm2, [eax]
vpmaxub xmm1, xmm2, dqword [eax]

pmaxuw xmm1, xmm2
pmaxuw xmm1, [eax]
pmaxuw xmm1, dqword [eax]
vpmaxuw xmm1, xmm2
vpmaxuw xmm1, [eax]
vpmaxuw xmm1, dqword [eax]
vpmaxuw xmm1, xmm2, xmm3
vpmaxuw xmm1, xmm2, [eax]
vpmaxuw xmm1, xmm2, dqword [eax]

pmaxud xmm1, xmm2
pmaxud xmm1, [eax]
pmaxud xmm1, dqword [eax]
vpmaxud xmm1, xmm2
vpmaxud xmm1, [eax]
vpmaxud xmm1, dqword [eax]
vpmaxud xmm1, xmm2, xmm3
vpmaxud xmm1, xmm2, [eax]
vpmaxud xmm1, xmm2, dqword [eax]

pminsb xmm1, xmm2
pminsb xmm1, [eax]
pminsb xmm1, dqword [eax]
vpminsb xmm1, xmm2
vpminsb xmm1, [eax]
vpminsb xmm1, dqword [eax]
vpminsb xmm1, xmm2, xmm3
vpminsb xmm1, xmm2, [eax]
vpminsb xmm1, xmm2, dqword [eax]

pminsw xmm1, xmm2
pminsw xmm1, [eax]
pminsw xmm1, dqword [eax]
vpminsw xmm1, xmm2
vpminsw xmm1, [eax]
vpminsw xmm1, dqword [eax]
vpminsw xmm1, xmm2, xmm3
vpminsw xmm1, xmm2, [eax]
vpminsw xmm1, xmm2, dqword [eax]

pminsd xmm1, xmm2
pminsd xmm1, [eax]
pminsd xmm1, dqword [eax]
vpminsd xmm1, xmm2
vpminsd xmm1, [eax]
vpminsd xmm1, dqword [eax]
vpminsd xmm1, xmm2, xmm3
vpminsd xmm1, xmm2, [eax]
vpminsd xmm1, xmm2, dqword [eax]

pminub xmm1, xmm2
pminub xmm1, [eax]
pminub xmm1, dqword [eax]
vpminub xmm1, xmm2
vpminub xmm1, [eax]
vpminub xmm1, dqword [eax]
vpminub xmm1, xmm2, xmm3
vpminub xmm1, xmm2, [eax]
vpminub xmm1, xmm2, dqword [eax]

pminuw xmm1, xmm2
pminuw xmm1, [eax]
pminuw xmm1, dqword [eax]
vpminuw xmm1, xmm2
vpminuw xmm1, [eax]
vpminuw xmm1, dqword [eax]
vpminuw xmm1, xmm2, xmm3
vpminuw xmm1, xmm2, [eax]
vpminuw xmm1, xmm2, dqword [eax]

pminud xmm1, xmm2
pminud xmm1, [eax]
pminud xmm1, dqword [eax]
vpminud xmm1, xmm2
vpminud xmm1, [eax]
vpminud xmm1, dqword [eax]
vpminud xmm1, xmm2, xmm3
vpminud xmm1, xmm2, [eax]
vpminud xmm1, xmm2, dqword [eax]

pmovmskb eax, xmm1
pmovmskb eax, xmm1
vpmovmskb eax, xmm1
vpmovmskb eax, xmm1

pmovsxbw xmm1, xmm2
pmovsxbw xmm1, [eax]
pmovsxbw xmm1, qword [eax]
vpmovsxbw xmm1, xmm2
vpmovsxbw xmm1, [eax]
vpmovsxbw xmm1, qword [eax]

pmovsxbd xmm1, xmm2
pmovsxbd xmm1, [eax]
pmovsxbd xmm1, dword [eax]
vpmovsxbd xmm1, xmm2
vpmovsxbd xmm1, [eax]
vpmovsxbd xmm1, dword [eax]

pmovsxbq xmm1, xmm2
pmovsxbq xmm1, [eax]
pmovsxbq xmm1, word [eax]
vpmovsxbq xmm1, xmm2
vpmovsxbq xmm1, [eax]
vpmovsxbq xmm1, word [eax]

pmovsxwd xmm1, xmm2
pmovsxwd xmm1, [eax]
pmovsxwd xmm1, qword [eax]
vpmovsxwd xmm1, xmm2
vpmovsxwd xmm1, [eax]
vpmovsxwd xmm1, qword [eax]

pmovsxwq xmm1, xmm2
pmovsxwq xmm1, [eax]
pmovsxwq xmm1, dword [eax]
vpmovsxwq xmm1, xmm2
vpmovsxwq xmm1, [eax]
vpmovsxwq xmm1, dword [eax]

pmovsxdq xmm1, xmm2
pmovsxdq xmm1, [eax]
pmovsxdq xmm1, qword [eax]
vpmovsxdq xmm1, xmm2
vpmovsxdq xmm1, [eax]
vpmovsxdq xmm1, qword [eax]

pmovzxbw xmm1, xmm2
pmovzxbw xmm1, [eax]
pmovzxbw xmm1, qword [eax]
vpmovzxbw xmm1, xmm2
vpmovzxbw xmm1, [eax]
vpmovzxbw xmm1, qword [eax]

pmovzxbd xmm1, xmm2
pmovzxbd xmm1, [eax]
pmovzxbd xmm1, dword [eax]
vpmovzxbd xmm1, xmm2
vpmovzxbd xmm1, [eax]
vpmovzxbd xmm1, dword [eax]

pmovzxbq xmm1, xmm2
pmovzxbq xmm1, [eax]
pmovzxbq xmm1, word [eax]
vpmovzxbq xmm1, xmm2
vpmovzxbq xmm1, [eax]
vpmovzxbq xmm1, word [eax]

pmovzxwd xmm1, xmm2
pmovzxwd xmm1, [eax]
pmovzxwd xmm1, qword [eax]
vpmovzxwd xmm1, xmm2
vpmovzxwd xmm1, [eax]
vpmovzxwd xmm1, qword [eax]

pmovzxwq xmm1, xmm2
pmovzxwq xmm1, [eax]
pmovzxwq xmm1, dword [eax]
vpmovzxwq xmm1, xmm2
vpmovzxwq xmm1, [eax]
vpmovzxwq xmm1, dword [eax]

pmovzxdq xmm1, xmm2
pmovzxdq xmm1, [eax]
pmovzxdq xmm1, qword [eax]
vpmovzxdq xmm1, xmm2
vpmovzxdq xmm1, [eax]
vpmovzxdq xmm1, qword [eax]

pmulhuw xmm1, xmm2
pmulhuw xmm1, [eax]
pmulhuw xmm1, dqword [eax]
vpmulhuw xmm1, xmm2
vpmulhuw xmm1, [eax]
vpmulhuw xmm1, dqword [eax]
vpmulhuw xmm1, xmm2, xmm3
vpmulhuw xmm1, xmm2, [eax]
vpmulhuw xmm1, xmm2, dqword [eax]

pmulhrsw xmm1, xmm2
pmulhrsw xmm1, [eax]
pmulhrsw xmm1, dqword [eax]
vpmulhrsw xmm1, xmm2
vpmulhrsw xmm1, [eax]
vpmulhrsw xmm1, dqword [eax]
vpmulhrsw xmm1, xmm2, xmm3
vpmulhrsw xmm1, xmm2, [eax]
vpmulhrsw xmm1, xmm2, dqword [eax]

pmulhw xmm1, xmm2
pmulhw xmm1, [eax]
pmulhw xmm1, dqword [eax]
vpmulhw xmm1, xmm2
vpmulhw xmm1, [eax]
vpmulhw xmm1, dqword [eax]
vpmulhw xmm1, xmm2, xmm3
vpmulhw xmm1, xmm2, [eax]
vpmulhw xmm1, xmm2, dqword [eax]

pmullw xmm1, xmm2
pmullw xmm1, [eax]
pmullw xmm1, dqword [eax]
vpmullw xmm1, xmm2
vpmullw xmm1, [eax]
vpmullw xmm1, dqword [eax]
vpmullw xmm1, xmm2, xmm3
vpmullw xmm1, xmm2, [eax]
vpmullw xmm1, xmm2, dqword [eax]

pmulld xmm1, xmm2
pmulld xmm1, [eax]
pmulld xmm1, dqword [eax]
vpmulld xmm1, xmm2
vpmulld xmm1, [eax]
vpmulld xmm1, dqword [eax]
vpmulld xmm1, xmm2, xmm3
vpmulld xmm1, xmm2, [eax]
vpmulld xmm1, xmm2, dqword [eax]

pmuludq xmm1, xmm2
pmuludq xmm1, [eax]
pmuludq xmm1, dqword [eax]
vpmuludq xmm1, xmm2
vpmuludq xmm1, [eax]
vpmuludq xmm1, dqword [eax]
vpmuludq xmm1, xmm2, xmm3
vpmuludq xmm1, xmm2, [eax]
vpmuludq xmm1, xmm2, dqword [eax]

pmuldq xmm1, xmm2
pmuldq xmm1, [eax]
pmuldq xmm1, dqword [eax]
vpmuldq xmm1, xmm2
vpmuldq xmm1, [eax]
vpmuldq xmm1, dqword [eax]
vpmuldq xmm1, xmm2, xmm3
vpmuldq xmm1, xmm2, [eax]
vpmuldq xmm1, xmm2, dqword [eax]

por xmm1, xmm2
por xmm1, [eax]
por xmm1, dqword [eax]
vpor xmm1, xmm2
vpor xmm1, [eax]
vpor xmm1, dqword [eax]
vpor xmm1, xmm2, xmm3
vpor xmm1, xmm2, [eax]
vpor xmm1, xmm2, dqword [eax]

psadbw xmm1, xmm2
psadbw xmm1, [eax]
psadbw xmm1, dqword [eax]
vpsadbw xmm1, xmm2
vpsadbw xmm1, [eax]
vpsadbw xmm1, dqword [eax]
vpsadbw xmm1, xmm2, xmm3
vpsadbw xmm1, xmm2, [eax]
vpsadbw xmm1, xmm2, dqword [eax]

pshufb xmm1, xmm2
pshufb xmm1, [eax]
pshufb xmm1, dqword [eax]
vpshufb xmm1, xmm2
vpshufb xmm1, [eax]
vpshufb xmm1, dqword [eax]
vpshufb xmm1, xmm2, xmm3
vpshufb xmm1, xmm2, [eax]
vpshufb xmm1, xmm2, dqword [eax]

pshufd xmm1, xmm2, 5
pshufd xmm1, [eax], byte 5
pshufd xmm1, dqword [eax], 5
vpshufd xmm1, xmm2, 5
vpshufd xmm1, [eax], byte 5
vpshufd xmm1, dqword [eax], 5

pshufhw xmm1, xmm2, 5
pshufhw xmm1, [eax], byte 5
pshufhw xmm1, dqword [eax], 5
vpshufhw xmm1, xmm2, 5
vpshufhw xmm1, [eax], byte 5
vpshufhw xmm1, dqword [eax], 5

pshuflw xmm1, xmm2, 5
pshuflw xmm1, [eax], byte 5
pshuflw xmm1, dqword [eax], 5
vpshuflw xmm1, xmm2, 5
vpshuflw xmm1, [eax], byte 5
vpshuflw xmm1, dqword [eax], 5

psignb xmm1, xmm2
psignb xmm1, [eax]
psignb xmm1, dqword [eax]
vpsignb xmm1, xmm2
vpsignb xmm1, [eax]
vpsignb xmm1, dqword [eax]
vpsignb xmm1, xmm2, xmm3
vpsignb xmm1, xmm2, [eax]
vpsignb xmm1, xmm2, dqword [eax]

psignw xmm1, xmm2
psignw xmm1, [eax]
psignw xmm1, dqword [eax]
vpsignw xmm1, xmm2
vpsignw xmm1, [eax]
vpsignw xmm1, dqword [eax]
vpsignw xmm1, xmm2, xmm3
vpsignw xmm1, xmm2, [eax]
vpsignw xmm1, xmm2, dqword [eax]

psignd xmm1, xmm2
psignd xmm1, [eax]
psignd xmm1, dqword [eax]
vpsignd xmm1, xmm2
vpsignd xmm1, [eax]
vpsignd xmm1, dqword [eax]
vpsignd xmm1, xmm2, xmm3
vpsignd xmm1, xmm2, [eax]
vpsignd xmm1, xmm2, dqword [eax]

; Test these with high regs as it goes into VEX.B (REX.B)
pslldq xmm7, 5
pslldq xmm7, byte 5
vpslldq xmm7, 5
vpslldq xmm7, byte 5
vpslldq xmm7, xmm6, 5
vpslldq xmm7, xmm6, byte 5

pslldq xmm1, 5
pslldq xmm1, byte 5
vpslldq xmm1, 5
vpslldq xmm1, byte 5
vpslldq xmm1, xmm2, 5
vpslldq xmm1, xmm2, byte 5

psrldq xmm1, 5
psrldq xmm1, byte 5
vpsrldq xmm1, 5
vpsrldq xmm1, byte 5
vpsrldq xmm1, xmm2, 5
vpsrldq xmm1, xmm2, byte 5

psllw xmm1, xmm2
psllw xmm1, [eax]
psllw xmm1, dqword [eax]
vpsllw xmm1, xmm2
vpsllw xmm1, [eax]
vpsllw xmm1, dqword [eax]
vpsllw xmm1, xmm2, xmm3
vpsllw xmm1, xmm2, [eax]
vpsllw xmm1, xmm2, dqword [eax]
psllw xmm1, 5
psllw xmm1, byte 5
vpsllw xmm1, 5
vpsllw xmm1, byte 5
vpsllw xmm1, xmm2, 5
vpsllw xmm1, xmm2, byte 5

pslld xmm1, xmm2
pslld xmm1, [eax]
pslld xmm1, dqword [eax]
vpslld xmm1, xmm2
vpslld xmm1, [eax]
vpslld xmm1, dqword [eax]
vpslld xmm1, xmm2, xmm3
vpslld xmm1, xmm2, [eax]
vpslld xmm1, xmm2, dqword [eax]
pslld xmm1, 5
pslld xmm1, byte 5
vpslld xmm1, 5
vpslld xmm1, byte 5
vpslld xmm1, xmm2, 5
vpslld xmm1, xmm2, byte 5

psllq xmm1, xmm2
psllq xmm1, [eax]
psllq xmm1, dqword [eax]
vpsllq xmm1, xmm2
vpsllq xmm1, [eax]
vpsllq xmm1, dqword [eax]
vpsllq xmm1, xmm2, xmm3
vpsllq xmm1, xmm2, [eax]
vpsllq xmm1, xmm2, dqword [eax]
psllq xmm1, 5
psllq xmm1, byte 5
vpsllq xmm1, 5
vpsllq xmm1, byte 5
vpsllq xmm1, xmm2, 5
vpsllq xmm1, xmm2, byte 5

psraw xmm1, xmm2
psraw xmm1, [eax]
psraw xmm1, dqword [eax]
vpsraw xmm1, xmm2
vpsraw xmm1, [eax]
vpsraw xmm1, dqword [eax]
vpsraw xmm1, xmm2, xmm3
vpsraw xmm1, xmm2, [eax]
vpsraw xmm1, xmm2, dqword [eax]
psraw xmm1, 5
psraw xmm1, byte 5
vpsraw xmm1, 5
vpsraw xmm1, byte 5
vpsraw xmm1, xmm2, 5
vpsraw xmm1, xmm2, byte 5

psrad xmm1, xmm2
psrad xmm1, [eax]
psrad xmm1, dqword [eax]
vpsrad xmm1, xmm2
vpsrad xmm1, [eax]
vpsrad xmm1, dqword [eax]
vpsrad xmm1, xmm2, xmm3
vpsrad xmm1, xmm2, [eax]
vpsrad xmm1, xmm2, dqword [eax]
psrad xmm1, 5
psrad xmm1, byte 5
vpsrad xmm1, 5
vpsrad xmm1, byte 5
vpsrad xmm1, xmm2, 5
vpsrad xmm1, xmm2, byte 5

psrlw xmm1, xmm2
psrlw xmm1, [eax]
psrlw xmm1, dqword [eax]
vpsrlw xmm1, xmm2
vpsrlw xmm1, [eax]
vpsrlw xmm1, dqword [eax]
vpsrlw xmm1, xmm2, xmm3
vpsrlw xmm1, xmm2, [eax]
vpsrlw xmm1, xmm2, dqword [eax]
psrlw xmm1, 5
psrlw xmm1, byte 5
vpsrlw xmm1, 5
vpsrlw xmm1, byte 5
vpsrlw xmm1, xmm2, 5
vpsrlw xmm1, xmm2, byte 5

psrld xmm1, xmm2
psrld xmm1, [eax]
psrld xmm1, dqword [eax]
vpsrld xmm1, xmm2
vpsrld xmm1, [eax]
vpsrld xmm1, dqword [eax]
vpsrld xmm1, xmm2, xmm3
vpsrld xmm1, xmm2, [eax]
vpsrld xmm1, xmm2, dqword [eax]
psrld xmm1, 5
psrld xmm1, byte 5
vpsrld xmm1, 5
vpsrld xmm1, byte 5
vpsrld xmm1, xmm2, 5
vpsrld xmm1, xmm2, byte 5

psrlq xmm1, xmm2
psrlq xmm1, [eax]
psrlq xmm1, dqword [eax]
vpsrlq xmm1, xmm2
vpsrlq xmm1, [eax]
vpsrlq xmm1, dqword [eax]
vpsrlq xmm1, xmm2, xmm3
vpsrlq xmm1, xmm2, [eax]
vpsrlq xmm1, xmm2, dqword [eax]
psrlq xmm1, 5
psrlq xmm1, byte 5
vpsrlq xmm1, 5
vpsrlq xmm1, byte 5
vpsrlq xmm1, xmm2, 5
vpsrlq xmm1, xmm2, byte 5

ptest xmm1, xmm2
ptest xmm1, [eax]
ptest xmm1, dqword [eax]
vptest xmm1, xmm2
vptest xmm1, [eax]
vptest xmm1, dqword [eax]
vptest ymm1, ymm2
vptest ymm1, [eax]
vptest ymm1, yword [eax]

vtestps xmm1, xmm2
vtestps xmm1, [eax]
vtestps xmm1, dqword [eax]
vtestps ymm1, ymm2
vtestps ymm1, [eax]
vtestps ymm1, yword [eax]

vtestpd xmm1, xmm2
vtestpd xmm1, [eax]
vtestpd xmm1, dqword [eax]
vtestpd ymm1, ymm2
vtestpd ymm1, [eax]
vtestpd ymm1, yword [eax]

psubb xmm1, xmm2
psubb xmm1, [eax]
psubb xmm1, dqword [eax]
vpsubb xmm1, xmm2
vpsubb xmm1, [eax]
vpsubb xmm1, dqword [eax]
vpsubb xmm1, xmm2, xmm3
vpsubb xmm1, xmm2, [eax]
vpsubb xmm1, xmm2, dqword [eax]

psubw xmm1, xmm2
psubw xmm1, [eax]
psubw xmm1, dqword [eax]
vpsubw xmm1, xmm2
vpsubw xmm1, [eax]
vpsubw xmm1, dqword [eax]
vpsubw xmm1, xmm2, xmm3
vpsubw xmm1, xmm2, [eax]
vpsubw xmm1, xmm2, dqword [eax]

psubd xmm1, xmm2
psubd xmm1, [eax]
psubd xmm1, dqword [eax]
vpsubd xmm1, xmm2
vpsubd xmm1, [eax]
vpsubd xmm1, dqword [eax]
vpsubd xmm1, xmm2, xmm3
vpsubd xmm1, xmm2, [eax]
vpsubd xmm1, xmm2, dqword [eax]

psubq xmm1, xmm2
psubq xmm1, [eax]
psubq xmm1, dqword [eax]
vpsubq xmm1, xmm2
vpsubq xmm1, [eax]
vpsubq xmm1, dqword [eax]
vpsubq xmm1, xmm2, xmm3
vpsubq xmm1, xmm2, [eax]
vpsubq xmm1, xmm2, dqword [eax]

psubsb xmm1, xmm2
psubsb xmm1, [eax]
psubsb xmm1, dqword [eax]
vpsubsb xmm1, xmm2
vpsubsb xmm1, [eax]
vpsubsb xmm1, dqword [eax]
vpsubsb xmm1, xmm2, xmm3
vpsubsb xmm1, xmm2, [eax]
vpsubsb xmm1, xmm2, dqword [eax]

psubsw xmm1, xmm2
psubsw xmm1, [eax]
psubsw xmm1, dqword [eax]
vpsubsw xmm1, xmm2
vpsubsw xmm1, [eax]
vpsubsw xmm1, dqword [eax]
vpsubsw xmm1, xmm2, xmm3
vpsubsw xmm1, xmm2, [eax]
vpsubsw xmm1, xmm2, dqword [eax]

psubusb xmm1, xmm2
psubusb xmm1, [eax]
psubusb xmm1, dqword [eax]
vpsubusb xmm1, xmm2
vpsubusb xmm1, [eax]
vpsubusb xmm1, dqword [eax]
vpsubusb xmm1, xmm2, xmm3
vpsubusb xmm1, xmm2, [eax]
vpsubusb xmm1, xmm2, dqword [eax]

psubusw xmm1, xmm2
psubusw xmm1, [eax]
psubusw xmm1, dqword [eax]
vpsubusw xmm1, xmm2
vpsubusw xmm1, [eax]
vpsubusw xmm1, dqword [eax]
vpsubusw xmm1, xmm2, xmm3
vpsubusw xmm1, xmm2, [eax]
vpsubusw xmm1, xmm2, dqword [eax]

punpckhbw xmm1, xmm2
punpckhbw xmm1, [eax]
punpckhbw xmm1, dqword [eax]
vpunpckhbw xmm1, xmm2
vpunpckhbw xmm1, [eax]
vpunpckhbw xmm1, dqword [eax]
vpunpckhbw xmm1, xmm2, xmm3
vpunpckhbw xmm1, xmm2, [eax]
vpunpckhbw xmm1, xmm2, dqword [eax]

punpckhwd xmm1, xmm2
punpckhwd xmm1, [eax]
punpckhwd xmm1, dqword [eax]
vpunpckhwd xmm1, xmm2
vpunpckhwd xmm1, [eax]
vpunpckhwd xmm1, dqword [eax]
vpunpckhwd xmm1, xmm2, xmm3
vpunpckhwd xmm1, xmm2, [eax]
vpunpckhwd xmm1, xmm2, dqword [eax]

punpckhdq xmm1, xmm2
punpckhdq xmm1, [eax]
punpckhdq xmm1, dqword [eax]
vpunpckhdq xmm1, xmm2
vpunpckhdq xmm1, [eax]
vpunpckhdq xmm1, dqword [eax]
vpunpckhdq xmm1, xmm2, xmm3
vpunpckhdq xmm1, xmm2, [eax]
vpunpckhdq xmm1, xmm2, dqword [eax]

punpckhqdq xmm1, xmm2
punpckhqdq xmm1, [eax]
punpckhqdq xmm1, dqword [eax]
vpunpckhqdq xmm1, xmm2
vpunpckhqdq xmm1, [eax]
vpunpckhqdq xmm1, dqword [eax]
vpunpckhqdq xmm1, xmm2, xmm3
vpunpckhqdq xmm1, xmm2, [eax]
vpunpckhqdq xmm1, xmm2, dqword [eax]

punpcklbw xmm1, xmm2
punpcklbw xmm1, [eax]
punpcklbw xmm1, dqword [eax]
vpunpcklbw xmm1, xmm2
vpunpcklbw xmm1, [eax]
vpunpcklbw xmm1, dqword [eax]
vpunpcklbw xmm1, xmm2, xmm3
vpunpcklbw xmm1, xmm2, [eax]
vpunpcklbw xmm1, xmm2, dqword [eax]

punpcklwd xmm1, xmm2
punpcklwd xmm1, [eax]
punpcklwd xmm1, dqword [eax]
vpunpcklwd xmm1, xmm2
vpunpcklwd xmm1, [eax]
vpunpcklwd xmm1, dqword [eax]
vpunpcklwd xmm1, xmm2, xmm3
vpunpcklwd xmm1, xmm2, [eax]
vpunpcklwd xmm1, xmm2, dqword [eax]

punpckldq xmm1, xmm2
punpckldq xmm1, [eax]
punpckldq xmm1, dqword [eax]
vpunpckldq xmm1, xmm2
vpunpckldq xmm1, [eax]
vpunpckldq xmm1, dqword [eax]
vpunpckldq xmm1, xmm2, xmm3
vpunpckldq xmm1, xmm2, [eax]
vpunpckldq xmm1, xmm2, dqword [eax]

punpcklqdq xmm1, xmm2
punpcklqdq xmm1, [eax]
punpcklqdq xmm1, dqword [eax]
vpunpcklqdq xmm1, xmm2
vpunpcklqdq xmm1, [eax]
vpunpcklqdq xmm1, dqword [eax]
vpunpcklqdq xmm1, xmm2, xmm3
vpunpcklqdq xmm1, xmm2, [eax]
vpunpcklqdq xmm1, xmm2, dqword [eax]

pxor xmm1, xmm2
pxor xmm1, [eax]
pxor xmm1, dqword [eax]
vpxor xmm1, xmm2
vpxor xmm1, [eax]
vpxor xmm1, dqword [eax]
vpxor xmm1, xmm2, xmm3
vpxor xmm1, xmm2, [eax]
vpxor xmm1, xmm2, dqword [eax]

rcpps xmm1, xmm2
rcpps xmm1, [eax]
rcpps xmm1, dqword [eax]
vrcpps xmm1, xmm2
vrcpps xmm1, [eax]
vrcpps xmm1, dqword [eax]
vrcpps ymm1, ymm2
vrcpps ymm1, [eax]
vrcpps ymm1, yword [eax]

rcpss xmm1, xmm2
rcpss xmm1, [eax]
rcpss xmm1, dword [eax]
vrcpss xmm1, xmm2
vrcpss xmm1, [eax]
vrcpss xmm1, dword [eax]
vrcpss xmm1, xmm2, xmm3
vrcpss xmm1, xmm2, [eax]
vrcpss xmm1, xmm2, dword [eax]

rsqrtps xmm1, xmm2
rsqrtps xmm1, [eax]
rsqrtps xmm1, dqword [eax]
vrsqrtps xmm1, xmm2
vrsqrtps xmm1, [eax]
vrsqrtps xmm1, dqword [eax]
vrsqrtps ymm1, ymm2
vrsqrtps ymm1, [eax]
vrsqrtps ymm1, yword [eax]

rsqrtss xmm1, xmm2
rsqrtss xmm1, [eax]
rsqrtss xmm1, dword [eax]
vrsqrtss xmm1, xmm2
vrsqrtss xmm1, [eax]
vrsqrtss xmm1, dword [eax]
vrsqrtss xmm1, xmm2, xmm3
vrsqrtss xmm1, xmm2, [eax]
vrsqrtss xmm1, xmm2, dword [eax]

roundpd xmm1, xmm2, 5
roundpd xmm1, [eax], byte 5
roundpd xmm1, dqword [eax], 5
vroundpd xmm1, xmm2, 5
vroundpd xmm1, [eax], byte 5
vroundpd xmm1, dqword [eax], 5
vroundpd ymm1, ymm2, 5
vroundpd ymm1, [eax], byte 5
vroundpd ymm1, yword [eax], 5

roundps xmm1, xmm2, 5
roundps xmm1, [eax], byte 5
roundps xmm1, dqword [eax], 5
vroundps xmm1, xmm2, 5
vroundps xmm1, [eax], byte 5
vroundps xmm1, dqword [eax], 5
vroundps ymm1, ymm2, 5
vroundps ymm1, [eax], byte 5
vroundps ymm1, yword [eax], 5

roundsd xmm1, xmm2, 5
roundsd xmm1, [eax], byte 5
roundsd xmm1, qword [eax], 5
vroundsd xmm1, xmm2, 5
vroundsd xmm1, [eax], byte 5
vroundsd xmm1, qword [eax], 5
vroundsd xmm1, xmm2, xmm3, 5
vroundsd xmm1, xmm2, [eax], byte 5
vroundsd xmm1, xmm2, qword [eax], 5

roundss xmm1, xmm2, 5
roundss xmm1, [eax], byte 5
roundss xmm1, dword [eax], 5
vroundss xmm1, xmm2, 5
vroundss xmm1, [eax], byte 5
vroundss xmm1, dword [eax], 5
vroundss xmm1, xmm2, xmm3, 5
vroundss xmm1, xmm2, [eax], byte 5
vroundss xmm1, xmm2, dword [eax], 5

shufpd xmm1, xmm2, 5
shufpd xmm1, [eax], byte 5
shufpd xmm1, dqword [eax], 5
vshufpd xmm1, xmm2, 5
vshufpd xmm1, [eax], byte 5
vshufpd xmm1, dqword [eax], 5
vshufpd xmm1, xmm2, xmm3, 5
vshufpd xmm1, xmm2, [eax], byte 5
vshufpd xmm1, xmm2, dqword [eax], 5
vshufpd ymm1, ymm2, ymm3, 5
vshufpd ymm1, ymm2, [eax], byte 5
vshufpd ymm1, ymm2, yword [eax], 5

shufps xmm1, xmm2, 5
shufps xmm1, [eax], byte 5
shufps xmm1, dqword [eax], 5
vshufps xmm1, xmm2, 5
vshufps xmm1, [eax], byte 5
vshufps xmm1, dqword [eax], 5
vshufps xmm1, xmm2, xmm3, 5
vshufps xmm1, xmm2, [eax], byte 5
vshufps xmm1, xmm2, dqword [eax], 5
vshufps ymm1, ymm2, ymm3, 5
vshufps ymm1, ymm2, [eax], byte 5
vshufps ymm1, ymm2, yword [eax], 5

sqrtpd xmm1, xmm2
sqrtpd xmm1, [eax]
sqrtpd xmm1, dqword [eax]
vsqrtpd xmm1, xmm2
vsqrtpd xmm1, [eax]
vsqrtpd xmm1, dqword [eax]
vsqrtpd ymm1, ymm2
vsqrtpd ymm1, [eax]
vsqrtpd ymm1, yword [eax]

sqrtps xmm1, xmm2
sqrtps xmm1, [eax]
sqrtps xmm1, dqword [eax]
vsqrtps xmm1, xmm2
vsqrtps xmm1, [eax]
vsqrtps xmm1, dqword [eax]
vsqrtps ymm1, ymm2
vsqrtps ymm1, [eax]
vsqrtps ymm1, yword [eax]

sqrtsd xmm1, xmm2
sqrtsd xmm1, [eax]
sqrtsd xmm1, qword [eax]
vsqrtsd xmm1, xmm2
vsqrtsd xmm1, [eax]
vsqrtsd xmm1, qword [eax]
vsqrtsd xmm1, xmm2, xmm3
vsqrtsd xmm1, xmm2, [eax]
vsqrtsd xmm1, xmm2, qword [eax]

sqrtss xmm1, xmm2
sqrtss xmm1, [eax]
sqrtss xmm1, dword [eax]
vsqrtss xmm1, xmm2
vsqrtss xmm1, [eax]
vsqrtss xmm1, dword [eax]
vsqrtss xmm1, xmm2, xmm3
vsqrtss xmm1, xmm2, [eax]
vsqrtss xmm1, xmm2, dword [eax]

stmxcsr [eax]
stmxcsr dword [eax]
vstmxcsr [eax]
vstmxcsr dword [eax]

subpd xmm1, xmm2
subpd xmm1, [eax]
subpd xmm1, dqword [eax]
vsubpd xmm1, xmm2
vsubpd xmm1, [eax]
vsubpd xmm1, dqword [eax]
vsubpd xmm1, xmm2, xmm3
vsubpd xmm1, xmm2, [eax]
vsubpd xmm1, xmm2, dqword [eax]
vsubpd ymm1, ymm2, ymm3
vsubpd ymm1, ymm2, [eax]
vsubpd ymm1, ymm2, yword [eax]

subps xmm1, xmm2
subps xmm1, [eax]
subps xmm1, dqword [eax]
vsubps xmm1, xmm2
vsubps xmm1, [eax]
vsubps xmm1, dqword [eax]
vsubps xmm1, xmm2, xmm3
vsubps xmm1, xmm2, [eax]
vsubps xmm1, xmm2, dqword [eax]
vsubps ymm1, ymm2, ymm3
vsubps ymm1, ymm2, [eax]
vsubps ymm1, ymm2, yword [eax]

subsd xmm1, xmm2
subsd xmm1, [eax]
subsd xmm1, qword [eax]
vsubsd xmm1, xmm2
vsubsd xmm1, [eax]
vsubsd xmm1, qword [eax]
vsubsd xmm1, xmm2, xmm3
vsubsd xmm1, xmm2, [eax]
vsubsd xmm1, xmm2, qword [eax]

subss xmm1, xmm2
subss xmm1, [eax]
subss xmm1, dword [eax]
vsubss xmm1, xmm2
vsubss xmm1, [eax]
vsubss xmm1, dword [eax]
vsubss xmm1, xmm2, xmm3
vsubss xmm1, xmm2, [eax]
vsubss xmm1, xmm2, dword [eax]

ucomisd xmm1, xmm2
ucomisd xmm1, [eax]
ucomisd xmm1, qword [eax]
vucomisd xmm1, xmm2
vucomisd xmm1, [eax]
vucomisd xmm1, qword [eax]

ucomiss xmm1, xmm2
ucomiss xmm1, [eax]
ucomiss xmm1, dword [eax]
vucomiss xmm1, xmm2
vucomiss xmm1, [eax]
vucomiss xmm1, dword [eax]

unpckhpd xmm1, xmm2
unpckhpd xmm1, [eax]
unpckhpd xmm1, dqword [eax]
vunpckhpd xmm1, xmm2
vunpckhpd xmm1, [eax]
vunpckhpd xmm1, dqword [eax]
vunpckhpd xmm1, xmm2, xmm3
vunpckhpd xmm1, xmm2, [eax]
vunpckhpd xmm1, xmm2, dqword [eax]
vunpckhpd ymm1, ymm2, ymm3
vunpckhpd ymm1, ymm2, [eax]
vunpckhpd ymm1, ymm2, yword [eax]

unpckhps xmm1, xmm2
unpckhps xmm1, [eax]
unpckhps xmm1, dqword [eax]
vunpckhps xmm1, xmm2
vunpckhps xmm1, [eax]
vunpckhps xmm1, dqword [eax]
vunpckhps xmm1, xmm2, xmm3
vunpckhps xmm1, xmm2, [eax]
vunpckhps xmm1, xmm2, dqword [eax]
vunpckhps ymm1, ymm2, ymm3
vunpckhps ymm1, ymm2, [eax]
vunpckhps ymm1, ymm2, yword [eax]

unpcklpd xmm1, xmm2
unpcklpd xmm1, [eax]
unpcklpd xmm1, dqword [eax]
vunpcklpd xmm1, xmm2
vunpcklpd xmm1, [eax]
vunpcklpd xmm1, dqword [eax]
vunpcklpd xmm1, xmm2, xmm3
vunpcklpd xmm1, xmm2, [eax]
vunpcklpd xmm1, xmm2, dqword [eax]
vunpcklpd ymm1, ymm2, ymm3
vunpcklpd ymm1, ymm2, [eax]
vunpcklpd ymm1, ymm2, yword [eax]

unpcklps xmm1, xmm2
unpcklps xmm1, [eax]
unpcklps xmm1, dqword [eax]
vunpcklps xmm1, xmm2
vunpcklps xmm1, [eax]
vunpcklps xmm1, dqword [eax]
vunpcklps xmm1, xmm2, xmm3
vunpcklps xmm1, xmm2, [eax]
vunpcklps xmm1, xmm2, dqword [eax]
vunpcklps ymm1, ymm2, ymm3
vunpcklps ymm1, ymm2, [eax]
vunpcklps ymm1, ymm2, yword [eax]

xorpd xmm1, xmm2
xorpd xmm1, [eax]
xorpd xmm1, dqword [eax]
vxorpd xmm1, xmm2
vxorpd xmm1, [eax]
vxorpd xmm1, dqword [eax]
vxorpd xmm1, xmm2, xmm3
vxorpd xmm1, xmm2, [eax]
vxorpd xmm1, xmm2, dqword [eax]
vxorpd ymm1, ymm2, ymm3
vxorpd ymm1, ymm2, [eax]
vxorpd ymm1, ymm2, yword [eax]

xorps xmm1, xmm2
xorps xmm1, [eax]
xorps xmm1, dqword [eax]
vxorps xmm1, xmm2
vxorps xmm1, [eax]
vxorps xmm1, dqword [eax]
vxorps xmm1, xmm2, xmm3
vxorps xmm1, xmm2, [eax]
vxorps xmm1, xmm2, dqword [eax]
vxorps ymm1, ymm2, ymm3
vxorps ymm1, ymm2, [eax]
vxorps ymm1, ymm2, yword [eax]

vzeroall
vzeroupper
emms
leave
retn

fma:
enter 100,0
vfmadd132ss xmm1, xmm2, xmm3
vfmadd132ss xmm1, xmm2, dword [eax]
vfmadd132ss xmm1, xmm3, [ebx]
vfmadd231ss xmm1, xmm2, xmm3
vfmadd231ss xmm1, xmm2, dword [ecx]
vfmadd231ss xmm1, xmm2, [edx]
vfmadd213ss xmm1, xmm2, xmm3
vfmadd213ss xmm1, xmm2, dword [edi]
vfmadd213ss xmm1, xmm2, [esi]
vfmadd132sd xmm1, xmm2, xmm3
vfmadd132sd xmm1, xmm2, qword [esp]
vfmadd132sd xmm1, xmm2, [ebp]
vfmadd231sd xmm1, xmm2, xmm3
vfmadd231sd xmm1, xmm2, qword [eax]
vfmadd231sd xmm1, xmm2, [eax+257]
vfmadd213sd xmm1, xmm2, xmm3
vfmadd213sd xmm1, xmm2, qword [ebx+257]
vfmadd213sd xmm1, xmm2, [ecx+257]
vfmadd132ps xmm1, xmm2, xmm3
vfmadd132ps xmm1, xmm2, xmm3
vfmadd132ps xmm1, xmm2, [edx+257]
vfmadd231ps xmm1, xmm2, xmm3
vfmadd231ps xmm1, xmm2, xmm3
vfmadd231ps xmm1, xmm2, [esi+257]
vfmadd213ps xmm1, xmm2, xmm3
vfmadd213ps xmm1, xmm2, xmm3
vfmadd213ps xmm1, xmm2, [edi+257]
vfmadd132ps ymm1, ymm2, ymm3
vfmadd132ps ymm1, ymm2, yword [ebp+257]
vfmadd132ps ymm1, ymm2, [esp+257]
vfmadd231ps ymm1, ymm2, ymm3
vfmadd231ps ymm1, ymm2, yword [eax*2+ebx+300]
vfmadd231ps ymm1, ymm2, [ebx*2+ecx+300]
vfmadd213ps ymm1, ymm2, ymm3
vfmadd213ps ymm1, ymm2, yword [edx*2+esi+300]
vfmadd213ps ymm1, ymm2, [esi*2+edi+300]
vfmadd132pd xmm1, xmm2, xmm3
vfmadd132pd xmm1, xmm2, dqword [edi*2+ebp+300]
vfmadd132pd xmm1, xmm2, [esi*2+ebp+300]
vfmadd231pd xmm1, xmm2, xmm3
vfmadd231pd xmm1, xmm2, dqword [ebp*2+esp+300]
vfmadd231pd xmm1, xmm2, [esp+eax+300]
vfmadd213pd xmm1, xmm2, xmm3
vfmadd213pd xmm1, xmm2, dqword [eax*4+ebx+600]
vfmadd213pd xmm1, xmm2, [ebx*4+ecx+600]
vfmadd132pd ymm1, ymm2, ymm3
vfmadd132pd ymm1, ymm2, yword [edx*4+esi+600]
vfmadd132pd ymm1, ymm2, [esi*4+edi+600]
vfmadd231pd ymm1, ymm2, ymm3
vfmadd231pd ymm1, ymm2, yword [edi*4+esp+600]
vfmadd231pd ymm1, ymm2, [ebp*4+esp+600]
vfmadd213pd ymm1, ymm2, ymm3
vfmadd213pd ymm1, ymm2, yword [esp+eax*4+600]
vfmadd213pd ymm1, ymm2, [eax*8]
vfmsub132ss xmm1, xmm2, xmm3
vfmsub132ss xmm1, xmm2, dword [ebx*8]
vfmsub132ss xmm1, xmm2, [ecx*8]
vfmsub231ss xmm1, xmm2, xmm3
vfmsub231ss xmm1, xmm2, dword [edx*8]
vfmsub231ss xmm1, xmm2, [edi*8]
vfmsub213ss xmm1, xmm2, xmm3
vfmsub213ss xmm1, xmm2, dword [esi*8]
vfmsub213ss xmm1, xmm2, [ebp*8]
vfmsub132sd xmm1, xmm2, xmm3
vfmsub132sd xmm1, xmm2, qword [esp]
vfmsub132sd xmm1, xmm2, [eax+8]
vfmsub231sd xmm1, xmm2, xmm3
vfmsub231sd xmm1, xmm2, qword [ebx+8]
vfmsub231sd xmm1, xmm2, [ecx+8]
vfmsub213sd xmm1, xmm2, xmm3
vfmsub213sd xmm1, xmm2, qword [edx+8]
vfmsub213sd xmm1, xmm2, [edi+8]
vfmsub132ps xmm1, xmm2, xmm3
vfmsub132ps xmm1, xmm2, xmm3
vfmsub132ps xmm1, xmm2, [esi+8]
vfmsub231ps xmm1, xmm2, xmm3
vfmsub231ps xmm1, xmm2, xmm3
vfmsub231ps xmm1, xmm2, [ebp+8]
vfmsub213ps xmm1, xmm2, xmm3
vfmsub213ps xmm1, xmm2, xmm3
vfmsub213ps xmm1, xmm2, [esp+8]
vfmsub132ps ymm1, ymm2, ymm3
vfmsub132ps ymm1, ymm2, yword [8]
vfmsub132ps ymm1, ymm2, [8]
vfmsub231ps ymm1, ymm2, ymm3
vfmsub231ps ymm1, ymm2, yword [8]
vfmsub231ps ymm1, ymm2, [8]
vfmsub213ps ymm1, ymm2, ymm3
vfmsub213ps ymm1, ymm2, yword [8]
vfmsub213ps ymm1, ymm2, [8]
vfmsub132pd xmm1, xmm2, xmm3
vfmsub132pd xmm1, xmm2, dqword [8]
vfmsub132pd xmm1, xmm2, [8]
vfmsub231pd xmm1, xmm2, xmm3
vfmsub231pd xmm1, xmm2, dqword [8]
vfmsub231pd xmm1, xmm2, [8]
vfmsub213pd xmm1, xmm2, xmm3
vfmsub213pd xmm1, xmm2, dqword [8]
vfmsub213pd xmm1, xmm2, [8]
vfmsub132pd ymm1, ymm2, ymm3
vfmsub132pd ymm1, ymm2, yword [eax]
vfmsub132pd ymm1, ymm2, [eax]
vfmsub231pd ymm1, ymm2, ymm3
vfmsub231pd ymm1, ymm2, yword [eax]
vfmsub231pd ymm1, ymm2, [eax]
vfmsub213pd ymm1, ymm2, ymm3
vfmsub213pd ymm1, ymm2, yword [eax]
vfmsub213pd ymm1, ymm2, [eax]
vfnmadd132ss xmm1, xmm2, xmm3
vfnmadd132ss xmm1, xmm2, dword [eax]
vfnmadd132ss xmm1, xmm2, [eax]
vfnmadd231ss xmm1, xmm2, xmm3
vfnmadd231ss xmm1, xmm2, dword [eax]
vfnmadd231ss xmm1, xmm2, [eax]
vfnmadd213ss xmm1, xmm2, xmm3
vfnmadd213ss xmm1, xmm2, dword [eax]
vfnmadd213ss xmm1, xmm2, [eax]
vfnmadd132sd xmm1, xmm2, xmm3
vfnmadd132sd xmm1, xmm2, qword [eax]
vfnmadd132sd xmm1, xmm2, [eax]
vfnmadd231sd xmm1, xmm2, xmm3
vfnmadd231sd xmm1, xmm2, qword [eax]
vfnmadd231sd xmm1, xmm2, [eax]
vfnmadd213sd xmm1, xmm2, xmm3
vfnmadd213sd xmm1, xmm2, qword [eax]
vfnmadd213sd xmm1, xmm2, [eax]
vfnmadd132ps xmm1, xmm2, xmm3
vfnmadd132ps xmm1, xmm2, xmm3
vfnmadd132ps xmm1, xmm2, [eax]
vfnmadd231ps xmm1, xmm2, xmm3
vfnmadd231ps xmm1, xmm2, xmm3
vfnmadd231ps xmm1, xmm2, [eax]
vfnmadd213ps xmm1, xmm2, xmm3
vfnmadd213ps xmm1, xmm2, xmm3
vfnmadd213ps xmm1, xmm2, [eax]
vfnmadd132ps ymm1, ymm2, ymm3
vfnmadd132ps ymm1, ymm2, yword [eax]
vfnmadd132ps ymm1, ymm2, [eax]
vfnmadd231ps ymm1, ymm2, ymm3
vfnmadd231ps ymm1, ymm2, yword [eax]
vfnmadd231ps ymm1, ymm2, [eax]
vfnmadd213ps ymm1, ymm2, ymm3
vfnmadd213ps ymm1, ymm2, yword [eax]
vfnmadd213ps ymm1, ymm2, [eax]
vfnmadd132pd xmm1, xmm2, xmm3
vfnmadd132pd xmm1, xmm2, dqword [eax]
vfnmadd132pd xmm1, xmm2, [eax]
vfnmadd231pd xmm1, xmm2, xmm3
vfnmadd231pd xmm1, xmm2, dqword [eax]
vfnmadd231pd xmm1, xmm2, [eax]
vfnmadd213pd xmm1, xmm2, xmm3
vfnmadd213pd xmm1, xmm2, dqword [eax]
vfnmadd213pd xmm1, xmm2, [eax]
vfnmadd132pd ymm1, ymm2, ymm3
vfnmadd132pd ymm1, ymm2, yword [eax]
vfnmadd132pd ymm1, ymm2, [eax]
vfnmadd231pd ymm1, ymm2, ymm3
vfnmadd231pd ymm1, ymm2, yword [eax]
vfnmadd231pd ymm1, ymm2, [eax]
vfnmadd213pd ymm1, ymm2, ymm3
vfnmadd213pd ymm1, ymm2, yword [eax]
vfnmadd213pd ymm1, ymm2, [eax]
vfnmsub132ss xmm1, xmm2, xmm3
vfnmsub132ss xmm1, xmm2, dword [eax]
vfnmsub132ss xmm1, xmm2, [eax]
vfnmsub231ss xmm1, xmm2, xmm3
vfnmsub231ss xmm1, xmm2, dword [eax]
vfnmsub231ss xmm1, xmm2, [eax]
vfnmsub213ss xmm1, xmm2, xmm3
vfnmsub213ss xmm1, xmm2, dword [eax]
vfnmsub213ss xmm1, xmm2, [eax]
vfnmsub132sd xmm1, xmm2, xmm3
vfnmsub132sd xmm1, xmm2, qword [eax]
vfnmsub132sd xmm1, xmm2, [eax]
vfnmsub231sd xmm1, xmm2, xmm3
vfnmsub231sd xmm1, xmm2, qword [eax]
vfnmsub231sd xmm1, xmm2, [eax]
vfnmsub213sd xmm1, xmm2, xmm3
vfnmsub213sd xmm1, xmm2, qword [eax]
vfnmsub213sd xmm1, xmm2, [eax]
vfnmsub132ps xmm1, xmm2, xmm3
vfnmsub132ps xmm1, xmm2, xmm3
vfnmsub132ps xmm1, xmm2, [eax]
vfnmsub231ps xmm1, xmm2, xmm3
vfnmsub231ps xmm1, xmm2, xmm3
vfnmsub231ps xmm1, xmm2, [eax]
vfnmsub213ps xmm1, xmm2, xmm3
vfnmsub213ps xmm1, xmm2, xmm3
vfnmsub213ps xmm1, xmm2, [eax]
vfnmsub132ps ymm1, ymm2, ymm3
vfnmsub132ps ymm1, ymm2, yword [eax]
vfnmsub132ps ymm1, ymm2, [eax]
vfnmsub231ps ymm1, ymm2, ymm3
vfnmsub231ps ymm1, ymm2, yword [eax]
vfnmsub231ps ymm1, ymm2, [eax]
vfnmsub213ps ymm1, ymm2, ymm3
vfnmsub213ps ymm1, ymm2, yword [eax]
vfnmsub213ps ymm1, ymm2, [eax]
vfnmsub132pd xmm1, xmm2, xmm3
vfnmsub132pd xmm1, xmm2, dqword [eax]
vfnmsub132pd xmm1, xmm2, [eax]
vfnmsub231pd xmm1, xmm2, xmm3
vfnmsub231pd xmm1, xmm2, dqword [eax]
vfnmsub231pd xmm1, xmm2, [eax]
vfnmsub213pd xmm1, xmm2, xmm3
vfnmsub213pd xmm1, xmm2, dqword [eax]
vfnmsub213pd xmm1, xmm2, [eax]
vfnmsub132pd ymm1, ymm2, ymm3
vfnmsub132pd ymm1, ymm2, yword [eax]
vfnmsub132pd ymm1, ymm2, [eax]
vfnmsub231pd ymm1, ymm2, ymm3
vfnmsub231pd ymm1, ymm2, yword [eax]
vfnmsub231pd ymm1, ymm2, [eax]
vfnmsub213pd ymm1, ymm2, ymm3
vfnmsub213pd ymm1, ymm2, yword [eax]
vfnmsub213pd ymm1, ymm2, [eax]
vfmaddsub132ps xmm1, xmm2, xmm3
vfmaddsub132ps xmm1, xmm2, xmm3
vfmaddsub132ps xmm1, xmm2, [eax]
vfmaddsub231ps xmm1, xmm2, xmm3
vfmaddsub231ps xmm1, xmm2, xmm3
vfmaddsub231ps xmm1, xmm2, [eax]
vfmaddsub213ps xmm1, xmm2, xmm3
vfmaddsub213ps xmm1, xmm2, xmm3
vfmaddsub213ps xmm1, xmm2, [eax]
vfmaddsub132ps ymm1, ymm2, ymm3
vfmaddsub132ps ymm1, ymm2, yword [eax]
vfmaddsub132ps ymm1, ymm2, [eax]
vfmaddsub231ps ymm1, ymm2, ymm3
vfmaddsub231ps ymm1, ymm2, yword [eax]
vfmaddsub231ps ymm1, ymm2, [eax]
vfmaddsub213ps ymm1, ymm2, ymm3
vfmaddsub213ps ymm1, ymm2, yword [eax]
vfmaddsub213ps ymm1, ymm2, [eax]
vfmaddsub132pd xmm1, xmm2, xmm3
vfmaddsub132pd xmm1, xmm2, dqword [eax]
vfmaddsub132pd xmm1, xmm2, [eax]
vfmaddsub231pd xmm1, xmm2, xmm3
vfmaddsub231pd xmm1, xmm2, dqword [eax]
vfmaddsub231pd xmm1, xmm2, [eax]
vfmaddsub213pd xmm1, xmm2, xmm3
vfmaddsub213pd xmm1, xmm2, dqword [eax]
vfmaddsub213pd xmm1, xmm2, [eax]
vfmaddsub132pd ymm1, ymm2, ymm3
vfmaddsub132pd ymm1, ymm2, yword [eax]
vfmaddsub132pd ymm1, ymm2, [eax]
vfmaddsub231pd ymm1, ymm2, ymm3
vfmaddsub231pd ymm1, ymm2, yword [eax]
vfmaddsub231pd ymm1, ymm2, [eax]
vfmaddsub213pd ymm1, ymm2, ymm3
vfmaddsub213pd ymm1, ymm2, yword [eax]
vfmaddsub213pd ymm1, ymm2, [eax]
vfmsubadd132ps xmm1, xmm2, xmm3
vfmsubadd132ps xmm1, xmm2, xmm3
vfmsubadd132ps xmm1, xmm2, [eax]
vfmsubadd231ps xmm1, xmm2, xmm3
vfmsubadd231ps xmm1, xmm2, xmm3
vfmsubadd231ps xmm1, xmm2, [eax]
vfmsubadd213ps xmm1, xmm2, xmm3
vfmsubadd213ps xmm1, xmm2, xmm3
vfmsubadd213ps xmm1, xmm2, [eax]
vfmsubadd132ps ymm1, ymm2, ymm3
vfmsubadd132ps ymm1, ymm2, yword [eax]
vfmsubadd132ps ymm1, ymm2, [eax]
vfmsubadd231ps ymm1, ymm2, ymm3
vfmsubadd231ps ymm1, ymm2, yword [eax]
vfmsubadd231ps ymm1, ymm2, [eax]
vfmsubadd213ps ymm1, ymm2, ymm3
vfmsubadd213ps ymm1, ymm2, yword [eax]
vfmsubadd213ps ymm1, ymm2, [eax]
vfmsubadd132pd xmm1, xmm2, xmm3
vfmsubadd132pd xmm1, xmm2, dqword [eax]
vfmsubadd132pd xmm1, xmm2, [eax]
vfmsubadd231pd xmm1, xmm2, xmm3
vfmsubadd231pd xmm1, xmm2, dqword [eax]
vfmsubadd231pd xmm1, xmm2, [eax]
vfmsubadd213pd xmm1, xmm2, xmm3
vfmsubadd213pd xmm1, xmm2, dqword [eax]
vfmsubadd213pd xmm1, xmm2, [eax]
vfmsubadd132pd ymm1, ymm2, ymm3
vfmsubadd132pd ymm1, ymm2, yword [eax]
vfmsubadd132pd ymm1, ymm2, [eax]
vfmsubadd231pd ymm1, ymm2, ymm3
vfmsubadd231pd ymm1, ymm2, yword [eax]
vfmsubadd231pd ymm1, ymm2, [eax]
vfmsubadd213pd ymm1, ymm2, ymm3
vfmsubadd213pd ymm1, ymm2, yword [eax]
vfmsubadd213pd ymm1, ymm2, [eax]
leave
retn
