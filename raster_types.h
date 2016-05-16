#ifndef _RASTER_TYPES
#define _RASTER_TYPES

#include <xmmintrin.h>

#include <stddef.h>
#include <stdint.h>
#include <float.h>

#include "alloc.h"
#include "list.h"
#include "hash.h"

//PathElement->type
enum {
	PATH_VERTEX   = 1,
	PATH_SEGMENT  = 2,
	PATH_PATH     = 4,
	PATH_STYLE    = 8,
	PATH_TEXTURE  = 16
};

typedef struct PathElement {
	struct PathElement *next, *prev;
	short type, eid, flag, pad;
} PathElement;

struct PathSegment;

typedef struct PathVertex {
	PathElement head;
	short x, y;
} PathVertex;

typedef struct PathSegment {
	PathElement head;
	short segtype, strokestyle;

	PathVertex *v1, *v2, *h1, *h2;

	struct {
		int dxab, dyab, dxbc, dybc, dxcd, dycd, dxda, dyda;
	} tmps;
} PathSegment;

//PathSegment->segtype
enum {
	SEG_LINEAR=0,
	SEG_CUBIC=1,
	SEG_QUADRATIC=2,
	SEG_ARC=3 //XXX need to figure out how to encode arcs
};

typedef struct Path {
	PathElement head;
	List segments;
	short style, styleflag;
	char styleargs[6];
} Path;

struct Style;
typedef struct StyleMachine {
	unsigned char *code;
	int ip, codelen;

	float regs[128];
	__m128 mregs[128];
	struct Style *style;
} StyleMachine;

struct CompiledCode;

typedef struct Style {
	PathElement head;

	short id, flag, codelen;
	unsigned char *code;

	struct CompiledCode *ccode;
} Style;

#define MAX_REG	128

typedef struct Texture { //RGBA only
	PathElement head;

	char *buffer;
	int bufferlen, size[2];
} Texture;

typedef struct Raster {
	HashTable verts, segments, paths, styles, textures, master;

	int size[2];
	unsigned char *buffer; //rgba output

	List renderlist;
} Raster;

Raster *raster_new(int width, int height);
void raster_free(Raster *raster);
PathVertex *raster_make_vertex(Raster *raster, short eid, short x, short y);
PathSegment *raster_make_segment(Raster *raster, short eid, short v1, short h1, short h2, short v2);
Path *raster_make_path(Raster *raster, short eid, short style);
void raster_path_append(Raster *raster, Path *p, PathSegment *s);

#endif /* _RASTER_TYPES */
