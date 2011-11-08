typedef struct {
	void *pdest;
	short *pz;
	int count;
	unsigned char *ptex;
	int sfrac, tfrac, light, zi;
} spanpackage_t;

typedef struct {
	int isflattop;
	int numleftedges;
	int *pleftedgevert0;
	int *pleftedgevert1;
	int *pleftedgevert2;
	int numrightedges;
	int *prightedgevert0;
	int *prightedgevert1;
	int *prightedgevert2;
} edgetable;

struct r_state {
	int ubasestep;
	int d_aspancount;
	int erroradjustup;
	int erroradjustdown;
	int errorterm;
	int d_countextrastep;
	int a_ststepxwhole;
	int a_sstepxfrac;
	int a_tstepxfrac;
	int d_light;
	int d_lightbasestep;
	int d_lightextrastep;
	int d_pdestbasestep;
	int d_pdestextrastep;
	short *d_pz;
	int d_pzbasestep;
	int d_pzextrastep;
	int d_sfrac;
	int d_sfracbasestep;
	int d_sfracextrastep;
	int d_tfrac;
	int d_tfracbasestep;
	int d_tfracextrastep;
	int d_zi;
	int d_zibasestep;
	int d_ziextrastep;
	int r_lstepx;
	int r_lstepy;
	int r_sstepx, r_sstepy;
	int r_tstepx, r_tstepy;
	int r_zistepx, r_zistepy;
	spanpackage_t *d_pedgespanpackage;
	spanpackage_t *a_spans;
	edgetable *pedgetable;
};

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
			  unsigned char *acolormap, struct r_state *r,
			  unsigned char **skintable,
			  int skinwidth, struct r_finalmesh *fm, int drawtype);
