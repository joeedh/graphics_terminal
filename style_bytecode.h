#ifndef _STYLE_BYTECODE_H
#define _STYLE_BYTECODE_H

struct StyleMachine;
void stylepixel(struct StyleMachine *m, float out[4], struct Style *style, int flag, char cargs[6],
                float x, float y, float u, float v);

#define MAX_REG 128

enum {
  MOV_RC,
  MOV_RND, //mov reg, num[2]/denom[2]
  MOV_RR,
  MOV_R4TEX, //regbase[1], tex[int2], u[2], v[2] //loads texture data into 4 consequtive registers
  ADD_RR,
  ADD_RC,
  SUB_RR,
  SUB_RC,
  MUL_RR,
  MUL_RC,
  DIV_RR,
  DIV_RC,
  LTN_RR,
  LTN_RC,
  GTN_RR,
  GTN_RC,
  EQL_RR,
  EQL_RC,
  TRU,
  SIN_RR,
  COS_RR,
  ASIN_RR,
  ACOS_RR,
  TAN_RR,
  ATAN_RR,
  SQRT_RR,
  LOG_RR,
  LOG2_RR,
  POW_RRR,
  POW_RRC,
  CBT_RR,
  FRC_RR, //fract
  END,
  MAD_RRR, //fused multiply and add
  MIN_RR,
  MAX_RR,
  RCP_RR, //reciprical
  ABS_RR
}; //bytecode

struct BuiltinBlock;

void style_disasm(unsigned char *buf, int len);

//returns number of registers bytecode at *buf has, as well as pointers to them
int bytecode_get_regs(unsigned char *buf, int len, unsigned char *regs[32]);

void compile_builtins();
int bytecode_lens[]; //size in bytes of each bytecode instruction

struct BuiltinBlock *get_and_link_builtin(int bytecode, int retreg, int argregs[32]);

enum {
	R0=0,
	R1=1,
	R2=2,
	R3=3,
	R4=4,
	INX=MAX_REG-1,
	INY=MAX_REG-2,
	INU=MAX_REG-3,
	INV=MAX_REG-4,
	INARG1=MAX_REG-5,
	INARG2=MAX_REG-6,
	INARG3=MAX_REG-7,
	INARG4=MAX_REG-8,
	INARG5=MAX_REG-9,
	INARG6=MAX_REG-10,
	OUTR=MAX_REG-11,
	OUTG=MAX_REG-12,
	OUTB=MAX_REG-13,
	OUTA=MAX_REG-14,
}; //registers
#endif /* _STYLE_BYTECODE_H */
