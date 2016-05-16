#include "raster_types.h"
#include "string.h"

Raster *raster_new(int width, int height) {
	Raster *raster = MEM_calloc(sizeof(*raster));

	hashtable_init(&raster->verts);
	hashtable_init(&raster->segments);
	hashtable_init(&raster->paths);
	hashtable_init(&raster->styles);
	hashtable_init(&raster->textures);
	hashtable_init(&raster->master);

	raster->size[0] = width;
	raster->size[1] = height;
	raster->buffer = MEM_malloc(width*height*4);
	memset(raster->buffer, 255, width*height*4); //initialize to white

	return raster;
}

void raster_free(Raster *raster) {
	hashtable_releaseitems(&raster->verts);
	hashtable_releaseitems(&raster->segments);
	hashtable_releaseitems(&raster->paths);
	hashtable_releaseitems(&raster->styles);
	hashtable_releaseitems(&raster->textures);
	hashtable_release(&raster->master);

	MEM_free(raster->buffer);
	MEM_free(raster);
}

PathVertex *raster_make_vertex(Raster *raster, short eid, short x, short y) {
	PathVertex *v = MEM_calloc(sizeof(*v));
	
	v->head.eid = eid;
	v->head.type = PATH_VERTEX;

	v->x = x;
	v->y = y;

	hashtable_set(&raster->verts, eid, (intptr_t)v);
	hashtable_set(&raster->master, eid, (intptr_t)v);

	return v;
}

PathSegment *raster_make_segment(Raster *raster, short eid, short v1, short h1, short h2, short v2) {
	PathSegment *s = MEM_calloc(sizeof(*s));

	s->head.eid = eid;
	s->head.type = PATH_SEGMENT;

	s->v1 = (void*)hashtable_get(&raster->verts, v1);
	s->h1 = (void*)hashtable_get(&raster->verts, h1);
	s->h2 = (void*)hashtable_get(&raster->verts, h2);
	s->v2 = (void*)hashtable_get(&raster->verts, v2);
	
	if (!s->v1) {
		fprintf(stderr, "failed to find vert with id %d\n", v1);
		return NULL;
	}
	if (!s->h1) {
		fprintf(stderr, "failed to find vert with id %d\n", h1);
		return NULL;
	}
	if (!s->h2) {
		fprintf(stderr, "failed to find vert with id %d\n", h2);
		return NULL;
	}
	if (!s->v2) {
		fprintf(stderr, "failed to find vert with id %d\n", v2);
		return NULL;
	}

	hashtable_set(&raster->segments, eid, (intptr_t)s);
	hashtable_set(&raster->master, eid, (intptr_t)s);

	return s;
}

Path *raster_make_path(Raster *raster, short eid, short style) {
	Path *p = MEM_calloc(sizeof(*p));

	p->head.eid = eid;
	p->head.type = PATH_PATH;
	p->style = style;

	hashtable_set(&raster->paths, eid, (intptr_t)p);
	hashtable_set(&raster->master, eid, (intptr_t)p);

	list_append(&raster->renderlist, p);

	return p;
}

void raster_path_append(Raster *raster, Path *p, PathSegment *s) {
	if (!s) {
		fprintf(stderr, "error: NULL segment passed to render_path_append()\n");
		return;
	}

	LinkNode *node = MEM_malloc(sizeof(*node));
	node->data = s;

	list_append(&p->segments, node);
}
