org 0

use32

%define DWSIZE 4
%define fr0 xmm0
%define fr1 xmm1
%define fr2 xmm2
%define fr3 xmm3

%define constload(fc)
  
execstyle:
%define DWSIZE 4
%define fr0 xmm0
%define fr1 xmm1
%define fr2 xmm2
%define fr3 xmm3

push ebp
mov ebp, esp
push ebx
push ecx

sub esp, 16
movaps [esp], xmm0

movaps xmm0, [esp]
add esp, 16

;get base address
call .get_eip
.get_eip:
  mov ecx, [esp]
  sub ecx, $
  ret

movaps fr0, [ecx + f32const0]
movaps [ebx+468], fr0
 movaps fr0, [ecx + f32const1]
movaps [ebx+464], fr0
 movaps fr0, [ecx + f32const2]
movaps [ebx+460], fr0
 movaps fr0, [ecx + f32const3]
movaps [ebx+456], fr0

cvtsq2ps fr0, fr1

pop ecx
 pop ebx
pop ebp
ret


align 16
f32const0: dd 1.0, 1.0, 1.0, 1.0
f32const1: dd 0.5, 0.5, 0.5, 0.5
f32const2: dd 0.0, 0.0, 0.0, 0.0
f32const3: dd 0.79980, 0.79980, 0.79980, 0.79980
