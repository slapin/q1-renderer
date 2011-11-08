#define MAXWIDTH 1280
#define MAXHEIGHT 1024
#define DPS_MAXSPANS MAXHEIGHT+1
#define CACHE_SIZE 32
struct r_view {
	unsigned char *viewbuffer;
	short *zbuffer;
	unsigned int zwidth;
	int screenwidth;
	short * zspantable[MAXHEIGHT];
	int d_scantable[MAXHEIGHT];
};
struct finalvert_s;
typedef struct mtriangle_s {
	int facesfront;
	int vertindex[3];
} mtriangle_t;
typedef struct finalvert_s {
	int v[6];		// u, v, s, t, l, 1/z
	int flags;
	float reserved;
} finalvert_t;
struct r_finalmesh {
	mtriangle_t *triangles;
	finalvert_t *finalverts;
	int ntriangles;
	void *pskin;
	int seamfixupX16;
};

#define ALIAS_ONSEAM 0x0020

void D_PolysetDraw(const struct r_view *v,
			  unsigned char *acolormap,
			  unsigned char **skintable,
			  int skinwidth, struct r_finalmesh *fm, int drawtype);

