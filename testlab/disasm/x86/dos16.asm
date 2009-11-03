[bits 16]
enter 10, 12
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
cmpxchg [bx+si],dx
cmpxchg8b qword [bx+si]
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
