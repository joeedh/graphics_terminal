use32
%define DWSIZE 4
%define fr0 xmm0
%define fr1 xmm1
%define fr2 xmm2
%define fr3 xmm3
org 0
evalpixel:
push ebp
mov ebp, esp
push ebx
push ecx

;get register file.  we're using __fastcall here, since win64 doesn't support cdecl
mov ebx, ecx

;get base address for position-independent code
call .get_eip
nop
jmp .next

.get_eip:
  mov ecx, [esp]
  sub ecx, $
  add ecx, 6
  ret

.next:
movaps fr0, [ecx+f32const0]
movaps [ebx+1872], fr0
movaps fr0, [ecx+f32const1]
movaps [ebx+1856], fr0
movaps fr0, [ecx+f32const2]
movaps [ebx+1840], fr0
movaps fr0, [ecx+f32const3]
movaps [ebx+1824], fr0
movaps fr0, [ebx + 1872]
mulps fr0, [ebx + 2000]
movaps [ebx + 1872], fr0
movaps fr0, [ebx + 1872]
mulps fr0, [ebx + 1984]
movaps [ebx + 1872], fr0
movaps fr0, [ebx + 1872]
mulps  fr0, [ecx + f32const4]
movaps [ebx + 1872], fr0
movaps       fr0,        [ebx + 1872]
movaps       fr2,        fr0
cvttps2dq    fr1,        fr0
psrld        fr0,        31
psubd        fr1,        fr0
cvtdq2ps     fr0,        fr1
subps        fr2,        fr0
movaps       [ebx + 1872], fr2
pop ecx
pop ebx
pop ebp
ret

align 16
f32const0: dd 1.00000, 1.00000, 1.00000, 1.00000
f32const1: dd 0.50000, 0.50000, 0.50000, 0.50000
f32const2: dd 0.00000, 0.00000, 0.00000, 0.00000
f32const3: dd 0.79980, 0.79980, 0.79980, 0.79980
f32const4: dd 5.00000, 5.00000, 5.00000, 5.00000
