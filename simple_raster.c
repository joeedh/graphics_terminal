#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "alloc.h"
#include "list.h"
#include "hash.h"
#include "floatutil.h"
#include "strhash.h"

#include "stylecompiler/compiler.h"
#include "raster_types.h"
#include "style_bytecode.h"

#define MIN2(a, b) ((a) < (b) ? (a) : (b))
#define MAX2(a, b) ((a) > (b) ? (a) : (b))

#define CLAMP(a, min, max) MIN2(MAX2(a, min), max)

char fa(float f) {
	short f16 = f32_to_f16(f);
	char *c = (char*)&f16;

	return c[0];
}

char fb(float f) {
	short f16 = f32_to_f16(f);
	char *c = (char*)&f16;

	return c[1];
}

#define FCONST(f) fa(f), fb(f)

static Style *_redstyle=NULL;

void init_default_styles() {
	/*
	unsigned char code[] = {
		MOV_RC, OUTR, FCONST(1.0f),
		MOV_RC, OUTG, FCONST(0.5f),
		MOV_RC, OUTB, FCONST(0.0f),
		MOV_RC, OUTA, FCONST(0.8f),

		//let's make a little triangle wave corner gradient
		MUL_RR, OUTR, INU,
		MUL_RR, OUTR, INV,
		MUL_RC, OUTR, FCONST(5.0f),
		FRC_RR, OUTR, OUTR,
		END
	};
	//*/

	unsigned char fallback_code[] = {
		MOV_RR, 0x5, 0x7f,
		MOV_RR, 0xf, 0x7d,
		MUL_RR, 0x5, 0xf,
		MOV_RR, 0x75, 0x5,
		MOV_RR, 0x5, 0x7d,
		MOV_RR, 0xf, 0x7d,
		MOV_RR, 0x10, 0x7c,
		MOV_RR, 0x11, 0x7c,
		MUL_RR, 0x10, 0x11,
		ADD_RR, 0xf, 0x10,
		MUL_RR, 0x5, 0xf,
		MOV_RC, 0xf, 0x99, 0x49,
		MUL_RR, 0x5, 0xf,
		FRC_RR, 0x5, 0x5,
		MOV_RR, 0x73, 0x5,
		MOV_RR, 0x5, 0x7e,
		MOV_RR, 0xf, 0x7c,
		MUL_RR, 0x5, 0xf,
		MOV_RR, 0x74, 0x5,
	};

#if 0
	char *script = 
		"float tent(float f) {\n"
		"	return 1.0 - abs(fract(f)-0.5)*2.0;\n"
		"}\n"

		"\n"

		"r = u;\n"
		"g = v;\n"
		"b = tent((u*u + v*v)*11.2);\n";
#elif 1
	char *script = 
		"r = 0.0;\n"
		"g = 0.1;\n"
		//"b = 1.0 - abs(fract(v * 10.159149 + 1.570796) - 0.500000)*2.0;\n"
		"b = sin((u*u + v*v)*11.2);\n"
		//"b = 0.0;\n"
	;
#endif

	int codelen=0;
	unsigned char *code = compilestyle(script, strlen(script), &codelen);

	if (!code) {
		code = fallback_code;
		codelen = sizeof(fallback_code);
	}

	Style *style = MEM_calloc(sizeof(*style));

	style->codelen = codelen;
	style->code = MEM_malloc(codelen);
	style->ccode = NULL;

	memcpy(style->code, code, codelen);
	_redstyle = style;
}

#define DEFAULT_STYLE _redstyle;

static __forceinline int winding(int x1, int y1, int x2, int y2, int x3, int y3) {
	int dx1 = x1-x2, dy1 = y1-y2;
	int dx2 = x3-x2, dy2 = y3-y2;

	int det = dx1*dy2 - dy1*dx2;
	return det >= 0;
}

/*
on factor;
off period;

procedure winding(x1, y1, x2, y2, x3, y3);
BEGIN scalar dx1, dy1, dx2, dy2, det;
	dx1 := x1-x2; dy1 := y1-y2;
	dx2 := x3-x2; dy2 := y3-y2;
	det := dx1*dy2 - dy1*dx2;
	return det;
END;

w1 := winding(ax, ay, bx, by, cx, cy);
w2 := winding(ax, ay, bx, by, dx, dy);
w3 := winding(cx, cy, dx, dy, ax, ay);
w4 := winding(cx, cy, dx, dy, bx, by);

comment: dxab1 := ax-bx;
comment: sub(dxab1=dxab, w1);

let ax-bx = dxab;
let ay-by = dyab;
let bx-cx = dxbc;
let by-cy = dybc;
let cx-dx = dxcd;
let cy-dy = dycd;
let dx-ax = dxda;
let dy-ay = dyda;
*/
static __forceinline int isect_line(int ax, int ay, int bx, int by, int cx, int cy, int dx, int dy) {
	int w1 = winding(ax, ay, bx, by, cx, cy);
	int w2 = winding(ax, ay, bx, by, dx, dy);

	if (w1 == w2) 
		return 0;

	int w3 = winding(cx, cy, dx, dy, ax, ay);
	int w4 = winding(cx, cy, dx, dy, bx, by);

	if (w3 == w4) 
		return 0;

	return 1;
}

void render_path(StyleMachine *m, Raster *raster, Path *p) {
	LinkNode *node;
	int min[2], max[2];
	int x, y, found=0;
	float rgba[4] = {0, 0, 0, 0};
	Style *style = (Style*) hashtable_get(&raster->styles, p->style);

	if (!style) {
		style = DEFAULT_STYLE;
	}

	m->style = style;
	m->code = style->code;
	m->codelen = style->codelen;

	min[0] = min[1] = INT_MAX;
	max[0] = max[1] = INT_MIN;

	for (node=p->segments.first; node; node=node->next) {
		PathSegment *s = node->data;

		min[0] = MIN2(min[0], s->v1->x);
		min[0] = MIN2(min[0], s->h1->x);
		min[0] = MIN2(min[0], s->h2->x);
		min[0] = MIN2(min[0], s->v2->x);
		min[1] = MIN2(min[1], s->v1->y);
		min[1] = MIN2(min[1], s->h1->y);
		min[1] = MIN2(min[1], s->h2->y);
		min[1] = MIN2(min[1], s->v2->y);

		max[0] = MAX2(max[0], s->v1->x);
		max[0] = MAX2(max[0], s->h1->x);
		max[0] = MAX2(max[0], s->h2->x);
		max[0] = MAX2(max[0], s->v2->x);
		max[1] = MAX2(max[1], s->v1->y);
		max[1] = MAX2(max[1], s->h1->y);
		max[1] = MAX2(max[1], s->h2->y);
		max[1] = MAX2(max[1], s->v2->y);

		found = 1;
	}

	if (!found || (min[0] == max[0] && min[1] == max[1])) {
		return; //path has no area
	}

	int w = raster->size[0], h = raster->size[1];
	int outside[2] = {min[0]-100, min[1]-101};

	//precache some calculations
	for (node=p->segments.first; node; node=node->next) {
		PathSegment *s = node->data;

		s->tmps.dxab = s->v1->x - s->v2->x;
		s->tmps.dyab = s->v1->y - s->v2->y;
		s->tmps.dxbc = s->v2->x - outside[0];
		s->tmps.dybc = s->v2->y - outside[1];
	}

	int miny=min[1], maxy=max[1];
	int minx=min[0], maxx=max[0];

	for (y = miny; y <= maxy; y++) {
		for (x = minx; x <= maxx; x++) {
			if (x < 0 || y < 0 || x >= w || y >= h) {
				continue; //XXX todo: better clipping
			}

			int ok = 1;
			int totisect = 0;

			for (node = p->segments.first; node; node=node->next) {
				PathSegment *s = (PathSegment*) node->data;
				
				s->tmps.dxcd = outside[0] - x;
				s->tmps.dycd = outside[1] - y;
				s->tmps.dxda = x - s->v1->x;
				s->tmps.dyda = y - s->v1->y;

				int w1 = -(s->tmps.dxbc*s->tmps.dycd+s->tmps.dxbc*s->tmps.dyda-s->tmps.dxcd*s->tmps.dybc-s->tmps.dxda*s->tmps.dybc) > 0.0;
				int w2 = -(s->tmps.dxbc*s->tmps.dyda+s->tmps.dxcd*s->tmps.dyda-s->tmps.dxda*s->tmps.dybc-s->tmps.dxda*s->tmps.dycd) > 0.0;
				int w3 = -(s->tmps.dxcd*s->tmps.dyda-s->tmps.dxda*s->tmps.dycd) > 0.0;
				int w4 = -(s->tmps.dxbc*s->tmps.dycd-s->tmps.dxcd*s->tmps.dybc) > 0.0;

				//do winding test
				totisect += !!(w1 != w2 && w3 != w4); ///isect_line(s->v1->x, s->v1->y, s->v2->x, s->v2->y, x, y, outside[0], outside[1]);
			}

			ok = (totisect & 1);

			if (!ok) {
				continue;
			}

			stylepixel(m, rgba, style, p->styleflag, p->styleargs, (float)x, (float)y, (float)x/(float)w, (float)y/(float)h);
			//rgba[0] = 1.0;
			//rgba[1] = 0.0;
			//rgba[1] = 0.0;
			//rgba[3] = 1.0;

			int idx = (y*raster->size[0] + x)*4;

			CLAMP(rgba[0], 0.0f, 1.0f);
			CLAMP(rgba[1], 0.0f, 1.0f);
			CLAMP(rgba[2], 0.0f, 1.0f);
			CLAMP(rgba[3], 0.0f, 1.0f);
			
			raster->buffer[idx] = (unsigned char)(rgba[0]*255.0f);
			raster->buffer[idx+1] = (unsigned char)(rgba[1]*255.0f);
			raster->buffer[idx+2] = (unsigned char)(rgba[2]*255.0f);
			raster->buffer[idx+3] = (unsigned char)(rgba[3]*255.0f);
		}
	}
}

void render_segment(Raster *raster, PathSegment *s) {
}

void raster_raster(Raster *raster) {
	StyleMachine _m, *m = &_m;
	PathElement *e;

	memset(m, 0, sizeof(*m));
	memset(raster->buffer, 255, raster->size[0]*raster->size[1]*4);

	for (e=raster->renderlist.first; e; e=e->next) {
		if (e->type == PATH_PATH) {
			render_path(m, raster, (Path*)e);
		} else {
			render_segment(raster, (PathSegment*)e);
		}
	}
}
