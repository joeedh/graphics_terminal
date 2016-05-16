#include <xmmintrin.h>

#include "windows.h"
#include "raster_types.h"
#include "style_bytecode.h"
#include "simple_raster.h"
#include <math.h>
#include <string.h>
#include <float.h>
#include <stdarg.h>
#include <stdio.h>

#include "floatutil.h"

#define READ() m->code[m->ip++]
#define LOADFC() c16[0] = READ(); c16[1] = READ(); fc = f16_to_f32(i16)
#define LOADI2() c16[0] = READ(); c16[1] = READ()

typedef struct CompiledCode {
	char *code;
	void *entry;
	int codelen, codesize;
	int bad;
	void (__fastcall *execpixel)(__m128 *regs);
} CompiledCode;

static int code_out(CompiledCode *code, const char *str, ...);

CompiledCode *statemachine_compile(StyleMachine *m) {
	short i16;
	unsigned char *c16 = (unsigned char*) &i16;
	float fc = 0;
	char buf[256];
	CompiledCode *code = MEM_calloc(sizeof(*code));
	CompiledCode *consts = MEM_calloc(sizeof(*consts));
	int totconst=0;

	//generates function that takes one argument, float regs[4][128];

	int ptrsize = sizeof(void*);

	if (sizeof(void*) == 4) {
		code_out(code, "use32\n");
		code_out(code, "%%define DWSIZE 4\n");
		code_out(code, "%%define fr0 xmm0\n");
		code_out(code, "%%define fr1 xmm1\n");
		code_out(code, "%%define fr2 xmm2\n");
		code_out(code, "%%define fr3 xmm3\n");
	} else if (sizeof(void*) == 8) {
		code_out(code, "use64\n");
		code_out(code, "%%define DWSIZE 8\n");
		code_out(code, "%%define fr0 xmm0\n");
		code_out(code, "%%define fr1 xmm1\n");
		code_out(code, "%%define fr2 xmm2\n");
		code_out(code, "%%define fr3 xmm3\n");
		code_out(code, "%define ebp rbp\n");
		code_out(code, "%define ecx rcx\n");
		code_out(code, "%define ebx rbx\n");
		code_out(code, "%define eax rax\n");
		code_out(code, "%define esp rsp\n");
	}

	//allocate space for registers
	
	code_out(code, "org 0\n");
	code_out(code, "evalpixel:\n");

	code_out(code, "push ebp\n");
	code_out(code, "mov ebp, esp\n");
	code_out(code, "push ebx\n");
	code_out(code, "push ecx\n");
	
	//code_out(code, "sub esp, 16\n");
	//code_out(code, "movaps [esp], xmm0\n");

	code_out(code, "\n;get register file.  we're using __fastcall here, since win64 doesn't support cdecl\n");
	code_out(code, "mov ebx, ecx\n");

	code_out(code, "\n;get base address for position-independent code\n");
	code_out(code, "call .get_eip\n");
	code_out(code, "nop\n");
	code_out(code, "jmp .next\n");

	code_out(code, "\n.get_eip:\n");
	code_out(code, "  mov ecx, [esp]\n");
	code_out(code, "  sub ecx, $\n");
	code_out(code, "  add ecx, 6\n");
	code_out(code, "  ret\n");

	code_out(code, "\n.next:\n");

	while (m->ip < m->codelen) {
		unsigned char c = READ();

		switch (c)  {
			  case MOV_RC: {
				  char r = READ();
				  LOADFC();


				  code_out(consts, "f32const%d: dd %.5f, %.5f, %.5f, %.5f\n", totconst, fc, fc, fc, fc);
				  code_out(code, "movaps fr0, [ecx+f32const%d]\n", totconst);
				  code_out(code, "movaps [ebx+%d], fr0\n", r*16);
				  totconst++;

				  break;
			  } 

			  case MOV_RND: {  //mov reg, num/denom
				  char r = READ();
				  float num, denom;

				  LOADI2();
				  num = i16;

				  LOADI2();
				  denom = i16;

				  code_out(consts, "f32const%d: dd %.5f, %.5f, %.5f, %.5f\n", totconst, fc, fc, fc, fc);
				  code_out(code, "movaps fr0, [ecx+f32const%d]\n", totconst);
				  code_out(code, "movaps [ebx+%d], fr0\n", r*16);
				  totconst++;

				  break;
			  }
			  case MOV_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] = m->regs[r2];
				  
				  code_out(code, "movaps fr0, [ebx+%d]\n", r2*16);
				  code_out(code, "movaps [ebx+%d], fr0\n", r1*16);

				  break;
			  }
			  case MOV_R4TEX: {  //regbase[1], tex[int2], u[2], v[2] //loads texture data into 4 consequtive registers
				  //XXX implement me!
				  break;
			  }
			  case ADD_RR: { 
				  char r1 = READ(), r2 = READ();
				  //m->regs[r1] += m->regs[r2];
				  break;
			  }
			  case ADD_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] += fc;
				  break;
			  }
			  case SUB_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] -= m->regs[r2];
				  break;
			  }
			  case SUB_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] -= fc;
				  break;
			  }
			  case MUL_RR: { 
				  char r1 = READ(), r2 = READ();

				  code_out(code, "movaps fr0, [ebx + %d]\n", r1*16);
				  code_out(code, "mulps fr0, [ebx + %d]\n", r2*16);
				  code_out(code, "movaps [ebx + %d], fr0\n", r1*16);
				  break;
			  }
			  case MUL_RC: { 
				  char r1 = READ();
				  LOADFC();

   				  code_out(code, "movaps fr0, [ebx + %d]\n", r1*16);
				  code_out(code, "mulps  fr0, [ecx + f32const%d]\n", totconst);
				  code_out(code, "movaps [ebx + %d], fr0\n", r1*16);

				  code_out(consts, "f32const%d: dd %.5f, %.5f, %.5f, %.5f\n", totconst, fc, fc, fc, fc);
				  totconst++;

				  break;
			  }
			  case DIV_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] /= m->regs[r2];
				  break;
			  }
			  case DIV_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] /= fc;
				  break;
			  }
			  case LTN_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] = m->regs[r1] < m->regs[r2] ? 1.0f :  0.0f;
				  break;
			  }
			  case LTN_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] = m->regs[r1] < fc ? 1.0f :  0.0f;
				  break;
			  }
			  case GTN_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] = m->regs[r1] > m->regs[r2] ? 1.0f : 0.0f;
				  break;
			  }
			  case GTN_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] = m->regs[r1] > fc ? 1.0f : 0.0f;
				  break;
			  }
			  case EQL_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] = m->regs[r1] == m->regs[r2] ? 1.0f : 0.0f;
				  break;
			  }
			  case EQL_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] = m->regs[r1] == fc ? 1.0f : 0.0f;
				  break;
			  }
			  case TRU: { 
				  char r = READ();
				  m->regs[r] = (m->regs[r] > 0.0 || m->regs[r] < 0.0) ? 1.0f : 0.0f;
				  break;
			  }
			  case SIN_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] = (float) sin((double)m->regs[r2]);
				  break;
			  }
			  case COS_RR: { 
				  break;
			  }
			  case ASIN_RR: { 
				  break;
			  }
			  case ACOS_RR: { 
				  break;
			  }
			  case TAN_RR: { 
				  break;
			  }
			  case ATAN_RR: { 
				  break;
			  }
			  case SQRT_RR: { 
				  break;
			  }
			  case LOG_RR: { 
				  break;
			  }
			  case LOG2_RR: { 
				  break;
			  }
			  case POW_RRR: { 
				  break;
			  }
			  case POW_RRC: { 
				  break;
			  }
			  case CBT_RR: { 
				  break;
			  }
			  case FRC_RR: { 
				  char r1 = READ(), r2 = READ();

				  code_out(code, "movaps       fr0,        [ebx + %d]\n", r2*16);
				  code_out(code, "movaps       fr2,        fr0\n");
				  code_out(code, "cvttps2dq    fr1,        fr0\n");
				  code_out(code, "psrld        fr0,        31\n");
	  			  code_out(code, "psubd        fr1,        fr0\n");
  				  code_out(code, "cvtdq2ps     fr0,        fr1\n");

				  code_out(code, "subps        fr2,        fr0\n", r1*16);
				  code_out(code, "movaps       [ebx + %d], fr2\n", r1*16);

				  m->regs[r1] = m->regs[r2] - floor(m->regs[r2]);
				  break;
			  }
			  case END: { 
				  m->ip = m->codelen;
				  break;
			  }
			  default: { 
				  fprintf(stderr, "unknown opcode 0x%x\n", (int)code);
				  return 0;
				  break;
			  }
		}
	}

	//code_out(code, "movaps xmm0, [esp]\n");
	//code_out(code, "add esp, 16\n");

	code_out(code, "pop ecx\n");
	code_out(code, "pop ebx\n");
	code_out(code, "pop ebp\n");

	if (sizeof(void*) == 8) {
		code_out(code, "fst xmm0;\n");
	}

	code_out(code, "ret\n\n");
	code_out(code, "align 16\n");

	char *code2 = MEM_malloc(code->codelen+consts->codelen+1);
	code2[code->codelen+consts->codelen] = 0;

	memcpy(code2, code->code, code->codelen);
	memcpy(code2+code->codelen, consts->code, consts->codelen);

	code->codelen += consts->codelen;

	MEM_free(consts->code);
	MEM_free(code->code);
	MEM_free(consts);

	code->code = code2;

	{
		extern int nasm_main(int argc, char **argv);
		FILE *tmp = NULL;
		fopen_s(&tmp, "_tmp.nasm", "wb");

		fwrite(code->code, code->codelen, 1, tmp);
		fclose(tmp);

		char *argv[] = {
			"nasm",
			"_tmp.nasm",
			"-fbin",
			"-o",
			"_tmp.bin",
			NULL
		};

		int ret = nasm_main(5, argv);
		int line = 0;

		if (ret != 0) {
			char *c = (char*)code->code;

			while (*c) {
				if (*c == '\n') {
					fprintf(stderr, "\n%d: ", ++line);
				} else {
					fputc(*c, stderr);
				}
				c++;
			}

			//fprintf(stderr, "%s\n\n", code->code);
			fprintf(stderr, "nasm returned an error! code: %d\n", ret);
			code->bad = 1;
		} else {
			size_t size;

			code->bad = 0;
			FILE *file=NULL;

			fopen_s(&file, "_tmp.bin", "rb");
			if (!file) {
				fprintf(stderr, "nasm failed to write output file!\n");
				code->bad = 1;
				return code;
			}

			fseek(file, 0, SEEK_END);
			size = ftell(file);
			fseek(file, 0, SEEK_SET);

			//apparently windows does this for us
			int align = 4*1024; //align to 4k page boundary
			int size2 = size + align - (size % align);

			char *bin = (char*) VirtualAlloc(NULL, size2, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			if (!bin) {
				fprintf(stderr, "failed to allocated executable memory for compiled style shader\n");
				code->bad = 1;
				fclose(file);

				return code;
			}

			fread(bin, size, 1, file);
			fclose(file);

			void (__fastcall *execpixel)(__m128 *regs) = (void (__fastcall *)(__m128 *regs)) bin;
			code->execpixel = execpixel;

			//int ret = VirtualProtect(bin, size2, PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE);
			//if (!ret) {
			//	fprintf(stderr, "warning: failed to set security parameters on compiled style shader code. %d\n", GetLastError());
			//}
			/*
			{

				__declspec(align(16)) __m128 regs[128];
				uintptr_t ubin = (uintptr_t)bin;
				intptr_t ibin = 0;

				__asm {
					call _geteip_label
				_geteip_label:
					add esp, 4
					
					mov eax, ubin
					sub eax, [esp-4]
					mov ibin, eax
				}

				void (__fastcall *execpixel)(__m128 *regs) = (void (__fastcall *)(__m128 *regs)) bin;
				execpixel(regs);
			}
			*/
		}
	}

	return code;
}

int statemachine_exec(StyleMachine *m) {
	short i16;
	unsigned char *c16 = (unsigned char*) &i16;
	float fc = 0;

	if (!m->style->ccode) {
		CompiledCode *code = statemachine_compile(m);

		m->style->ccode = code;
	}

	//m->style->ccode->bad = 1; //XXX

	if (!m->style->ccode->bad) {
		m->style->ccode->execpixel(m->mregs);
		return 1;
	}

	while (m->ip < m->codelen) {
		unsigned char code = READ();

		switch (code)  {
			  case MOV_RC: {
				  char r = READ();
				  LOADFC();
				  m->regs[r] = fc;
				  break;
			  } 

			  case MOV_RND: {  //mov reg, num/denom
				  char r = READ();
				  float num, denom;

				  LOADI2();
				  num = i16;

				  LOADI2();
				  denom = i16;

				  m->regs[r] = num / denom;
				  break;
			  }
			  case MOV_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] = m->regs[r2];
				  break;
			  }
			  case MOV_R4TEX: {  //regbase[1], tex[int2], u[2], v[2] //loads texture data into 4 consequtive registers
				  //XXX implement me!
				  break;
			  }
			  case ADD_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] += m->regs[r2];
				  break;
			  }
			  case ADD_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] += fc;
				  break;
			  }
			  case SUB_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] -= m->regs[r2];
				  break;
			  }
			  case SUB_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] -= fc;
				  break;
			  }
			  case MUL_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] *= m->regs[r2];
				  break;
			  }
			  case MUL_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] *= fc;
				  break;
			  }
			  case DIV_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] /= m->regs[r2];
				  break;
			  }
			  case DIV_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] /= fc;
				  break;
			  }
			  case LTN_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] = m->regs[r1] < m->regs[r2] ? 1.0f :  0.0f;
				  break;
			  }
			  case LTN_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] = m->regs[r1] < fc ? 1.0f :  0.0f;
				  break;
			  }
			  case GTN_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] = m->regs[r1] > m->regs[r2] ? 1.0f : 0.0f;
				  break;
			  }
			  case GTN_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] = m->regs[r1] > fc ? 1.0f : 0.0f;
				  break;
			  }
			  case EQL_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] = m->regs[r1] == m->regs[r2] ? 1.0f : 0.0f;
				  break;
			  }
			  case EQL_RC: { 
				  char r1 = READ();
				  LOADFC();

				  m->regs[r1] = m->regs[r1] == fc ? 1.0f : 0.0f;
				  break;
			  }
			  case TRU: { 
				  char r = READ();
				  m->regs[r] = (m->regs[r] > 0.0 || m->regs[r] < 0.0) ? 1.0f : 0.0f;
				  break;
			  }
			  case SIN_RR: { 
				  char r1 = READ(), r2 = READ();
				  m->regs[r1] = (float) sin((double)m->regs[r2]);
				  break;
			  }
			  case COS_RR: { 
				  break;
			  }
			  case ASIN_RR: { 
				  break;
			  }
			  case ACOS_RR: { 
				  break;
			  }
			  case TAN_RR: { 
				  break;
			  }
			  case ATAN_RR: { 
				  break;
			  }
			  case SQRT_RR: { 
				  break;
			  }
			  case LOG_RR: { 
				  break;
			  }
			  case LOG2_RR: { 
				  break;
			  }
			  case POW_RRR: { 
				  break;
			  }
			  case POW_RRC: { 
				  break;
			  }
			  case CBT_RR: { 
				  break;
			  }
			  case FRC_RR: { 
				  char r1 = READ(), r2 = READ();

				  m->regs[r1] = m->regs[r2] - floor(m->regs[r2]);
				  break;
			  }
			  case END: { 
				  m->ip = m->codelen;
				  break;
			  }
			  default: { 
				  fprintf(stderr, "unknown opcode 0x%x\n", (int)code);
				  return 0;
				  break;
			  }
		}
	}

	return 1;
}

void stylepixel(StyleMachine *m, float out[4], struct Style *style, int flag, char cargs[6], float x, float y, float u, float v) {
	/*
	m->regs[INARG1] = (float)cargs[0] / 255.0f;
	m->regs[INARG2] = (float)cargs[1] / 255.0f;
	m->regs[INARG3] = (float)cargs[2] / 255.0f;
	m->regs[INARG4] = (float)cargs[3] / 255.0f;
	m->regs[INARG5] = (float)cargs[4] / 255.0f;
	m->regs[INARG6] = (float)cargs[5] / 255.0f;
	*/

	m->regs[INX] = x;
	m->regs[INY] = y;
	m->regs[INU] = u;
	m->regs[INV] = v;

	//load xmms registers
	/*
	m.mregs[INARG1] = _mm_load_ps1(&m.regs[INARG1]);
	m.mregs[INARG2] = _mm_load_ps1(&m.regs[INARG2]);
	m.mregs[INARG3] = _mm_load_ps1(&m.regs[INARG3]);
	m.mregs[INARG4] = _mm_load_ps1(&m.regs[INARG4]);
	m.mregs[INARG5] = _mm_load_ps1(&m.regs[INARG5]);
	m.mregs[INARG6] = _mm_load_ps1(&m.regs[INARG6]);
	*/

	m->mregs[INX] = _mm_load_ps1(&m->regs[INX]);
	m->mregs[INY] = _mm_load_ps1(&m->regs[INY]);

	m->mregs[INU] = _mm_load_ps1(&m->regs[INU]);
	m->mregs[INV] = _mm_load_ps1(&m->regs[INV]);

	statemachine_exec(m);

	if (!m->style->ccode->bad) {
		float _four[16], *four = _four; //*four = MEM_calloc(sizeof(float)*16);
		uintptr_t f = (uintptr_t)four;

		f += 16 - (f & 15);
		four = (float*) f;

		out[0] = ((float*)&m->mregs[OUTR])[0];
		out[1] = ((float*)&m->mregs[OUTG])[1];
		out[2] = ((float*)&m->mregs[OUTB])[2];
		out[3] = ((float*)&m->mregs[OUTA])[3];

		//_mm_store_ps1(four, m->mregs[OUTR]); out[0] = four[0];
		//_mm_store_ps1(four, m->mregs[OUTG]); out[1] = four[0];
		//_mm_store_ps1(four, m->mregs[OUTB]); out[2] = four[0];
		//_mm_store_ps1(four, m->mregs[OUTA]); out[3] = four[0];
	} else {
		out[0] = m->regs[OUTR];
		out[1] = m->regs[OUTG];
		out[2] = m->regs[OUTB];
		out[3] = m->regs[OUTA];
	}
}

static int code_out(CompiledCode *code, const char *str, ...) {
	char buf[512];
	va_list va;

	va_start(va, str);

//#define _CRT_SECURE_NO_WARNINGS
	vsnprintf_s(buf, sizeof(buf), _TRUNCATE, str, va);
//#undef _CRT_SECURE_NO_WARNINGS

	buf[sizeof(buf)-1] = 0;

	va_end(va);

	int len = strlen(buf);

	if (code->codelen+len+1 >= code->codesize) {
		code->codesize = (code->codesize+len+1)*2;

		char *newcode = MEM_malloc(code->codesize);

		if (code->code) {
			memcpy(newcode, code->code, code->codelen+1);
			MEM_free(code->code);
		} else {
			code->codelen = 0;
		}

		code->code = newcode;
	}

	memcpy(code->code+code->codelen, buf, len);
	
	code->codelen += len;
	code->code[code->codelen] = 0;

	return 1;
}
