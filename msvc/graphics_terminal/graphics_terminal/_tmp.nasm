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
movaps [ebx+80], fr0
movaps fr0, [ebx+80]
movaps [ebx+1856], fr0
movaps fr0, [ecx+f32const1]
movaps [ebx+80], fr0
movaps fr0, [ebx+80]
movaps [ebx+1872], fr0
movaps fr0, [ebx+2000]
movaps [ebx+80], fr0
movaps fr0, [ebx+2000]
movaps [ebx+240], fr0
movaps fr0, [ebx + 80]
mulps fr0, [ebx + 240]
movaps [ebx + 80], fr0
movaps fr0, [ebx+1984]
movaps [ebx+240], fr0
movaps fr0, [ebx+1984]
movaps [ebx+256], fr0
movaps fr0, [ebx + 240]
mulps fr0, [ebx + 256]
movaps [ebx + 240], fr0
movaps fr0, [ebx + 80]
addps fr0, [ebx + 240]
movaps [ebx + 80], fr0
movaps fr0, [ecx+f32const2]
movaps [ebx+240], fr0
movaps fr0, [ebx + 80]
mulps fr0, [ebx + 240]
movaps [ebx + 80], fr0
movaps fr0, [ecx+f32const3]
movaps [ebx+480], fr0
movaps fr0, [ebx+80]
movaps [ebx+640], fr0
movaps fr0, [ecx+f32const4]
movaps [ebx+656], fr0
movaps fr0, [ebx + 640]
mulps fr0, [ebx + 656]
movaps [ebx + 640], fr0
movaps fr0, [ecx+f32const5]
movaps [ebx+656], fr0
movaps fr0, [ebx + 640]
addps fr0, [ebx + 656]
movaps [ebx + 640], fr0
movaps       fr0,        [ebx + 640]
movaps       fr2,        fr0
cvttps2dq    fr1,        fr0
psrld        fr0,        31
psubd        fr1,        fr0
cvtdq2ps     fr0,        fr1
subps        fr2,        fr0
movaps       [ebx + 640], fr2
movaps fr0, [ecx+f32const6]
movaps [ebx+656], fr0
movaps fr0, [ebx + 640]
subps fr0, [ebx + 656]
movaps [ebx + 640], fr0
movaps       fr0,         [ebx + 640]
andps        fr0,         [ecx + unsign_mask]
movaps       [ebx + 640],  fr0
movaps fr0, [ecx+f32const7]
movaps [ebx+656], fr0
movaps fr0, [ebx + 640]
mulps fr0, [ebx + 656]
movaps [ebx + 640], fr0
movaps fr0, [ebx + 480]
subps fr0, [ebx + 640]
movaps [ebx + 480], fr0
movaps       fr0,        [ebx + 480]
movaps       fr2,        fr0
cvttps2dq    fr1,        fr0
psrld        fr0,        31
psubd        fr1,        fr0
cvtdq2ps     fr0,        fr1
subps        fr2,        fr0
movaps       [ebx + 480], fr2
movaps fr0, [ebx+480]
movaps [ebx+80], fr0
movaps fr0, [ebx+80]
movaps [ebx+1840], fr0
pop ecx
pop ebx
pop ebp
ret

align 16
unsign_mask: dd 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff
f32const0: dd 0.09998, 0.09998, 0.09998, 0.09998
f32const1: dd 0.00000, 0.00000, 0.00000, 0.00000
f32const2: dd 11.19531, 11.19531, 11.19531, 11.19531
f32const3: dd 1.00000, 1.00000, 1.00000, 1.00000
f32const4: dd 3.15820, 3.15820, 3.15820, 3.15820
f32const5: dd 1.57031, 1.57031, 1.57031, 1.57031
f32const6: dd 0.50000, 0.50000, 0.50000, 0.50000
f32const7: dd 2.00000, 2.00000, 2.00000, 2.00000
