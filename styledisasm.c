#include <stdio.h>
#include "style_bytecode.h"
#include "strutils.h"
#include "strhash.h"

#define _(v) case v: return #v;

char *bytecode_to_str(int code) {
	switch (code) {
	_(MOV_RC)
	_(MOV_RND) //mov reg, num[2]/denom[2]
	_(MOV_RR)
	_(MOV_R4TEX) //regbase[1], tex[int2], u[2], v[2] //loads texture data into 4 consequtive registers
	_(ADD_RR)
	_(ADD_RC)
	_(SUB_RR)
	_(SUB_RC)
	_(MUL_RR)
	_(MUL_RC)
	_(DIV_RR)
	_(DIV_RC)
	_(LTN_RR)
	_(LTN_RC)
	_(GTN_RR)
	_(GTN_RC)
	_(EQL_RR)
	_(EQL_RC)
	_(TRU)
	_(SIN_RR)
	_(COS_RR)
	_(ASIN_RR)
	_(ACOS_RR)
	_(TAN_RR)
	_(ATAN_RR)
	_(SQRT_RR)
	_(LOG_RR)
	_(LOG2_RR)
	_(POW_RRR)
	_(POW_RRC)
	_(CBT_RR)
	_(FRC_RR) //fract
	_(END)
	_(MAD_RRR) //fused multiply and add
	_(MIN_RR)
	_(MAX_RR)
	_(RCP_RR) //reciprical
	_(ABS_RR)
	default:
		return "(unknown opcode)";
	}
}

//includes initial bytecode
int bytecode_lens[] = {
  4, //MOV_RC,
  6, //MOV_RND, //mov reg, num[2]/denom[2]
  3, //MOV_RR,
  8, //MOV_R4TEX, //regbase[1], tex[int2], u[2], v[2] //loads texture data into 4 consequtive registers
  3, //ADD_RR,
  4, //ADD_RC,
  3, //SUB_RR,
  4, //SUB_RC,
  3, //MUL_RR,
  4, //MUL_RC,
  3, //DIV_RR,
  4, //DIV_RC,
  3, //LTN_RR,
  4, //LTN_RC,
  3, //GTN_RR,
  4, //GTN_RC,
  3, //EQL_RR,
  4, //EQL_RC,
  2, //TRU,
  3, //SIN_RR,
  3, //COS_RR,
  3, //ASIN_RR,
  3, //ACOS_RR,
  3, //TAN_RR,
  3, //ATAN_RR,
  3, //SQRT_RR,
  3, //LOG_RR,
  3, //LOG2_RR,
  3, //POW_RRR,
  4, //POW_RRC,
  3, //CBT_RR,
  3, //FRC_RR, //fract
  1, //END,
  4, //MAD_RRR, //fused multiply and add
  3, //MIN_RR,
  3, //MAX_RR,
  3, //RCP_RR, //reciprical
  3, //ABS_RR
  1000000, //try to induce crash if we forget to add new bytecodes
  1000000,
  1000000,
  1000000,
};

void style_disasm(unsigned char *buf, int len) {
	int i=0, code;

	while (i < len) {
		code = buf[i];
		int codelen = bytecode_lens[code];

		if (codelen <= 0) {
			printf("%d: ", code);
		}

		printf("%s, ", bytecode_to_str(code));
		 
		if (codelen > 0) {
			int j;
			for (j=1; j<codelen; j++) {
				printf("0x%x, ", buf[i+j]);
			}
		}
		printf("\n");

		i += codelen<=0 ? 1 : codelen;
	}
}

int bytecode_get_regs(unsigned char *buf, int len, unsigned char *regs[32]) {
	switch (*buf) {
	case MOV_RC:
	case ADD_RC:
	case SUB_RC:
	case MUL_RC:
	case DIV_RC:
	case LTN_RC:
	case GTN_RC:
	case EQL_RC:
	case TRU:
	case MOV_RND: //mov reg: num[2]/denom[2]
		regs[0] = buf+1;
		return 1;
	case MOV_R4TEX: //regbase[1]: tex[int2]: u[2]: v[2] //loads texture data into 4 consequtive registers
		regs[0] = buf+1;
		regs[1] = buf+2;
		regs[2] = buf+3;
		regs[3] = buf+4;
		return 4;
	case MOV_RR:
	case ADD_RR:
	case SUB_RR:
	case MUL_RR:
	case DIV_RR:
	case LTN_RR:
	case GTN_RR:
	case EQL_RR:
	case SIN_RR:
	case COS_RR:
	case ASIN_RR:
	case ACOS_RR:
	case TAN_RR:
	case ATAN_RR:
	case SQRT_RR:
	case LOG_RR:
	case LOG2_RR:
	case POW_RRC:
	case CBT_RR:
	case FRC_RR: //fract
	case MIN_RR:
	case MAX_RR:
	case RCP_RR: //reciprical
	case ABS_RR:
		regs[0] = buf+1;
		regs[1] = buf+2;
		return 2;
	case MAD_RRR: //fused multiply and add
	case POW_RRR:
		regs[0] = buf+1;
		regs[1] = buf+2;
		regs[2] = buf+3;
		return 3;
	case END:
		return 0;
	}

	return 0;
}