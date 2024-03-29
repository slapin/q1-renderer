#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include "control.h"
#include "triangle.h"
#include "polyset.h"

#define MAXHEIGHT 1024
#define MAXWIDTH 1280
#define MAXROWBYTES (MAXWIDTH * 1)

#define ALIAS_LEFT_CLIP 0x0001
#define ALIAS_TOP_CLIP 0x0002
#define ALIAS_RIGHT_CLIP 0x0004
#define ALIAS_BOTTOM_CLIP 0x0008
#define ALIAS_Z_CLIP 0x0010
#define ALIAS_XY_CLIP_MASK 0x000F
#define MAX_LBM_HEIGHT 480
#define VectorCopy(a,b) \
	((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2])

typedef unsigned char byte;
typedef enum { ALIAS_SKIN_SINGLE = 0, ALIAS_SKIN_GROUP } aliasskintype_t;
typedef struct {
	aliasskintype_t type;
	int skin;
} maliasskindesc_t;
typedef struct {
	int seamfixupX16;
} affinetridesc_t;
typedef byte pixel_t;
typedef struct {
	int onseam;
	int s;
	int t;
} stvert_t;

typedef struct {
	float fv[3];		// viewspace x, y
} auxvert_t;


/* R_Alias symbols */
#define MAXALIASVERTS	2000	// TODO: tune this
#define RF_WEAPONMODEL	1
#define RF_LIMITLERP	4
#define RF_PLAYERMODEL	8
#define MAX_QPATH	64
#define	MAX_MAP_HULLS	4
#define MIPLEVELS	4
#define MAXLIGHTMAPS	4
#define NUMVERTEXNORMALS    162
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#define bound(a,b,c) ((a) >= (c) ? (a) : \
                    (b) < (a) ? (a) : (b) > (c) ? (c) : (b))

#define VectorSubtract(a,b,c)   ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1], (c)[2] = (a)[2] - (b)[2])
#define VectorNegate(a,b)       ((b)[0] = -(a)[0], (b)[1] = -(a)[1], (b)[2] = -(a)[2])
#define VectorSet(v, x, y, z)   ((v)[0] = ((x)), (v)[1] = (y), (v)[2] = (z))
#define R_AliasTransformVector(in, out)                                         \
    (((out)[0] = DotProduct((in), aliastransform[0]) + aliastransform[0][3]),   \
    ((out)[1] = DotProduct((in), aliastransform[1]) + aliastransform[1][3]),    \
    ((out)[2] = DotProduct((in), aliastransform[2]) + aliastransform[2][3]))

#define Sys_Error printf
#define Host_Error printf
#define Com_DPrintf printf
#define COM_FileBase(i, o) strcpy(o, i)
#define IDPOLYHEADER    (('O'<<24)+('P'<<16)+('D'<<8)+'I')
#define	ALIAS_VERSION	6
#define ALIAS_BASE_SIZE_RATIO       (1.0 / 11.0)
		    // normalizing factor so player model works out to about
		    //  1 pixel per triangle
#define PITCH   0		// up / down
#define YAW     1		// left / right
#define ROLL    2		// fall over
#define MAX_INFO_STRING	1024
#define SURF_DRAWTILED 32
#define MAX_DLIGHTS 64
#define LIGHT_MIN 5
#define VID_CBITS 6
#define VID_GRADES (1 << VID_CBITS)

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef enum { ALIAS_SINGLE = 0, ALIAS_GROUP } aliasframetype_t;
typedef struct {
	byte v[3];
	byte lightnormalindex;
} trivertx_t;

typedef struct maliasframedesc_s {
	aliasframetype_t type;
	trivertx_t bboxmin;
	trivertx_t bboxmax;
	int frame;
	char name[16];
} maliasframedesc_t;

typedef struct aliashdr_s {
	int model;
	int stverts;
	int skindesc;
	int triangles;
	maliasframedesc_t frames[1];
} aliashdr_t;
typedef enum { ST_SYNC = 0, ST_RAND } synctype_t;
typedef struct {
	int ident;
	int version;
	vec3_t scale;
	vec3_t scale_origin;
	float boundingradius;
	vec3_t eyeposition;
	int numskins;
	int skinwidth;
	int skinheight;
	int numverts;
	int numtris;
	int numframes;
	synctype_t synctype;
	int flags;
	float size;
} mdl_t;
typedef enum { false, true } qbool;
typedef enum { MOD_NORMAL, MOD_PLAYER, MOD_EYES, MOD_FLAME,
	MOD_THUNDERBOLT, MOD_BACKPACK,
	MOD_RAIL, MOD_BUILDINGGIBS
} modhint_t;
typedef enum { mod_brush, mod_sprite, mod_alias } modtype_t;

typedef struct {
	float mins[3], maxs[3];
	float origin[3];
	int headnode[MAX_MAP_HULLS];
	int visleafs;		// not including the solid leaf 0
	int firstface, numfaces;
} dmodel_t;

typedef struct mplane_s {
	vec3_t normal;
	float dist;
	byte type;		// for texture axis selection and fast side tests
	byte signbits;		// signx + signy<<1 + signz<<1
	byte pad[2];
} mplane_t;

typedef struct texture_s {
	char name[16];
	unsigned width, height;
	int anim_total;		// total tenths in sequence ( 0 = no)
	int anim_min, anim_max;	// time for this frame min <=time< max
	struct texture_s *anim_next;	// in the animation sequence
	struct texture_s *alternate_anims;	// bmodels in frmae 1 use these
	unsigned offsets[MIPLEVELS];	// four mip maps stored
	int flatcolor3ub;	// hetman: r_drawflat for software builds
} texture_t;

typedef struct mtexinfo_s {
	float vecs[2][4];
	float mipadjust;
	texture_t *texture;
	int flags;
} mtexinfo_t;

typedef struct msurface_s {
	int visframe;		// should be drawn when node is crossed

	int dlightframe;
	int dlightbits;

	mplane_t *plane;
	int flags;

	int firstedge;		// look up in model->surfedges[], negative numbers
	int numedges;		// are backwards edges

	// surface generation data
	struct surfcache_s *cachespots[MIPLEVELS];

	short texturemins[2];
	short extents[2];

	mtexinfo_t *texinfo;

	// lighting info
	byte styles[MAXLIGHTMAPS];
	byte *samples;		// [numstyles*surfsize]
} msurface_t;

typedef struct mleaf_s {
	// common with node
	int contents;		// wil be a negative contents number
	int visframe;		// node needs to be traversed if current

	short minmaxs[6];	// for bounding box culling

	struct mnode_s *parent;

	// leaf specific
	byte *compressed_vis;
	struct efrag_s *efrags;

	msurface_t **firstmarksurface;
	int nummarksurfaces;
	int key;		// BSP sequence number for leaf's contents
} mleaf_t;

typedef struct mvertex_s {
	vec3_t position;
} mvertex_t;

typedef struct medge_s {
	unsigned short v[2];
	unsigned int cachededgeoffset;
} medge_t;

typedef struct mnode_s {
	// common with leaf
	int contents;		// 0, to differentiate from leafs
	int visframe;		// node needs to be traversed if current

	short minmaxs[6];	// for bounding box culling

	struct mnode_s *parent;

	// node specific
	mplane_t *plane;
	struct mnode_s *children[2];

	unsigned short firstsurface;
	unsigned short numsurfaces;
} mnode_t;

typedef struct cache_user_s {
	void *data;
} cache_user_t;

typedef struct cache_system_s {
	int size;		// including this header
	cache_user_t *user;
	char name[16];
	struct cache_system_s *prev, *next;
	struct cache_system_s *lru_prev, *lru_next;	// for LRU flushing
} cache_system_t;

typedef struct model_s {
	char name[MAX_QPATH];
	qbool needload;		// bmodels and sprites don't cache normally

	unsigned short crc;

	modhint_t modhint;

	modtype_t type;
	int numframes;
	synctype_t synctype;

	int flags;

	// volume occupied by the model graphics
	vec3_t mins, maxs;
	float radius;

	// brush model
	int firstmodelsurface, nummodelsurfaces;

	// FIXME, don't really need these two
	int numsubmodels;
	dmodel_t *submodels;

	int numplanes;
	mplane_t *planes;

	int numleafs;		// number of visible leafs, not counting 0
	mleaf_t *leafs;

	int numvertexes;
	mvertex_t *vertexes;

	int numedges;
	medge_t *edges;

	int numnodes;
	int firstnode;
	mnode_t *nodes;

	int numtexinfo;
	mtexinfo_t *texinfo;

	int numsurfaces;
	msurface_t *surfaces;

	int numsurfedges;
	int *surfedges;

	int nummarksurfaces;
	msurface_t **marksurfaces;

	int numtextures;
	texture_t **textures;

	byte *visdata;
	byte *lightdata;

	int bspversion;
	qbool isworldmodel;

	// additional model data
	cache_user_t cache;	// only access through Mod_Extradata

} model_t;

#if 0
typedef struct player_info_s {
	int userid;
	char userinfo[MAX_INFO_STRING];

	// Scoreboard information.
	char name[MAX_SCOREBOARDNAME];
	float entertime;
	int frags;
	int ping;
	byte pl;

	// Skin information.
	int topcolor;
	int bottomcolor;

	int _topcolor;
	int _bottomcolor;

	int real_topcolor;
	int real_bottomcolor;
	char team[MAX_INFO_STRING];
	char _team[MAX_INFO_STRING];

	int fps_msec;
	int last_fps;
	int fps;		// > 0 - fps, < 0 - invalid, 0 - collecting
	int fps_frames;
	double fps_measure_time;
	qbool isnear;

	int spectator;
	byte translations[VID_GRADES * 256];
	skin_t *skin;

	int stats[MAX_CL_STATS];

	qbool skin_refresh;
	qbool ignored;		// for ignore
	qbool validated;	// for authentication
	char f_server[16];	// for f_server responses

	// VULT DEATH EFFECT
	// Better putting the dead flag here instead of on the entity so whats dead stays dead
	qbool dead;

} player_info_t;
#endif

typedef struct entity_s {
	vec3_t origin;
	vec3_t angles;
	struct model_s *model;	// NULL = no model
	byte *colormap;
	int skinnum;		// for Alias models
	struct player_info_s *scoreboard;	// identify player
	int renderfx;		// RF_ bit mask
	int effects;		// EF_ flags like EF_BLUE and etc, atm this used for powerup shells

	int oldframe;
	int frame;
	float framelerp;

	struct efrag_s *efrag;	// linked list of efrags (FIXME)
	int visframe;		// last frame this entity was found in an active leaf. only used for static objects

	int dlightframe;	// dynamic lighting
	int dlightbits;

	//VULT MOTION TRAILS
	float alpha;

	//FootStep sounds
	//int   stepframe;//disabled

	// FIXME: could turn these into a union
	int trivial_accept;
	struct mnode_s *topnode;	// for bmodels, first world node that splits bmodel, or NULL if not split
} entity_t;

typedef struct vrect_s {
	int x, y, width, height;
	struct vrect_s *pnext;
} vrect_t;

typedef struct {
	vrect_t vrect;		// subwindow in video for refresh
	// FIXME: not need vrect next field here?
	vrect_t aliasvrect;	// scaled Alias version
	int vrectright, vrectbottom;	// right & bottom screen coords
	int aliasvrectright, aliasvrectbottom;	// scaled Alias versions
	float vrectrightedge;	// rightmost right edge we care about,
	//  for use in edge list
	float fvrectx, fvrecty;	// for floating-point compares
	float fvrectx_adj, fvrecty_adj;	// left and top edges, for clamping
	int vrect_x_adj_shift20;	// (vrect.x + 0.5 - epsilon) << 20
	int vrectright_adj_shift20;	// (vrectright + 0.5 - epsilon) << 20
	float fvrectright_adj, fvrectbottom_adj;
	// right and bottom edges, for clamping
	float fvrectright;	// rightmost edge, for Alias clamping
	float fvrectbottom;	// bottommost edge, for Alias clamping
	float horizontalFieldOfView;	// at Z = 1.0, this many X is visible 
	// 2.0 = 90 degrees
	float xOrigin;		// should probably always be 0.5
	float yOrigin;		// between be around 0.3 to 0.5

	vec3_t vieworg;
	vec3_t viewangles;

	float fov_x, fov_y;

	int ambientlight;
} refdef_t;

typedef struct dtriangle_s {
	int facesfront;
	int vertindex[3];
} dtriangle_t;

typedef struct {
	aliasframetype_t type;
} daliasframetype_t;

typedef struct {
	aliasskintype_t type;
} daliasskintype_t;

typedef struct {
	int numskins;
} daliasskingroup_t;

typedef struct {
	float interval;
} daliasskininterval_t;

typedef struct maliasskingroup_s {
	int numskins;
	int intervals;
	maliasskindesc_t skindescs[1];
} maliasskingroup_t;

typedef struct {
	trivertx_t bboxmin;	// lightnormal isn't used
	trivertx_t bboxmax;	// lightnormal isn't used
	char name[16];		// frame name from grabbing
} daliasframe_t;

typedef struct {
	int numframes;
	trivertx_t bboxmin;	// lightnormal isn't used
	trivertx_t bboxmax;	// lightnormal isn't used
} daliasgroup_t;

typedef struct {
	float interval;
} daliasinterval_t;

typedef struct maliasgroupframedesc_s {
	trivertx_t bboxmin;
	trivertx_t bboxmax;
	int frame;
} maliasgroupframedesc_t;

typedef struct maliasgroup_s {
	int numframes;
	int intervals;
	maliasgroupframedesc_t frames[1];
} maliasgroup_t;

typedef struct {
	int index0;
	int index1;
} aedge_t;

typedef enum { lt_default, lt_muzzleflash, lt_explosion, lt_rocket,
	lt_red, lt_blue, lt_redblue, lt_green, lt_redgreen, lt_bluegreen,
	lt_white, lt_custom, NUM_DLIGHTTYPES
} dlighttype_t;

typedef struct {
	int key;		// so entities can reuse same entry
	vec3_t origin;
	float radius;
	float die;		// stop lighting after this time
	float decay;		// drop this each second
	float minlight;		// don't add when contributing less
	int bubble;		// non zero means no flashblend bubble
	dlighttype_t type;
#ifdef GLQUAKE
	byte color[3];		// use such color if type == lt_custom
#endif
} dlight_t;

struct r_verts {
	trivertx_t *r_oldapverts;
	trivertx_t *r_apverts;
};
struct r_vects {
	vec3_t r_plightvec;
	vec3_t alias_forward, alias_right, alias_up;
	vec3_t vup, vright, vpn /* Forward */ ;
};

static refdef_t r_refdef;
static float aliastransform[3][4];
static const float r_avertexnormals[NUMVERTEXNORMALS][3] = {
	{-0.525731, 0.000000, 0.850651},
	{-0.442863, 0.238856, 0.864188},
	{-0.295242, 0.000000, 0.955423},
	{-0.309017, 0.500000, 0.809017},
	{-0.162460, 0.262866, 0.951056},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.850651, 0.525731},
	{-0.147621, 0.716567, 0.681718},
	{0.147621, 0.716567, 0.681718},
	{0.000000, 0.525731, 0.850651},
	{0.309017, 0.500000, 0.809017},
	{0.525731, 0.000000, 0.850651},
	{0.295242, 0.000000, 0.955423},
	{0.442863, 0.238856, 0.864188},
	{0.162460, 0.262866, 0.951056},
	{-0.681718, 0.147621, 0.716567},
	{-0.809017, 0.309017, 0.500000},
	{-0.587785, 0.425325, 0.688191},
	{-0.850651, 0.525731, 0.000000},
	{-0.864188, 0.442863, 0.238856},
	{-0.716567, 0.681718, 0.147621},
	{-0.688191, 0.587785, 0.425325},
	{-0.500000, 0.809017, 0.309017},
	{-0.238856, 0.864188, 0.442863},
	{-0.425325, 0.688191, 0.587785},
	{-0.716567, 0.681718, -0.147621},
	{-0.500000, 0.809017, -0.309017},
	{-0.525731, 0.850651, 0.000000},
	{0.000000, 0.850651, -0.525731},
	{-0.238856, 0.864188, -0.442863},
	{0.000000, 0.955423, -0.295242},
	{-0.262866, 0.951056, -0.162460},
	{0.000000, 1.000000, 0.000000},
	{0.000000, 0.955423, 0.295242},
	{-0.262866, 0.951056, 0.162460},
	{0.238856, 0.864188, 0.442863},
	{0.262866, 0.951056, 0.162460},
	{0.500000, 0.809017, 0.309017},
	{0.238856, 0.864188, -0.442863},
	{0.262866, 0.951056, -0.162460},
	{0.500000, 0.809017, -0.309017},
	{0.850651, 0.525731, 0.000000},
	{0.716567, 0.681718, 0.147621},
	{0.716567, 0.681718, -0.147621},
	{0.525731, 0.850651, 0.000000},
	{0.425325, 0.688191, 0.587785},
	{0.864188, 0.442863, 0.238856},
	{0.688191, 0.587785, 0.425325},
	{0.809017, 0.309017, 0.500000},
	{0.681718, 0.147621, 0.716567},
	{0.587785, 0.425325, 0.688191},
	{0.955423, 0.295242, 0.000000},
	{1.000000, 0.000000, 0.000000},
	{0.951056, 0.162460, 0.262866},
	{0.850651, -0.525731, 0.000000},
	{0.955423, -0.295242, 0.000000},
	{0.864188, -0.442863, 0.238856},
	{0.951056, -0.162460, 0.262866},
	{0.809017, -0.309017, 0.500000},
	{0.681718, -0.147621, 0.716567},
	{0.850651, 0.000000, 0.525731},
	{0.864188, 0.442863, -0.238856},
	{0.809017, 0.309017, -0.500000},
	{0.951056, 0.162460, -0.262866},
	{0.525731, 0.000000, -0.850651},
	{0.681718, 0.147621, -0.716567},
	{0.681718, -0.147621, -0.716567},
	{0.850651, 0.000000, -0.525731},
	{0.809017, -0.309017, -0.500000},
	{0.864188, -0.442863, -0.238856},
	{0.951056, -0.162460, -0.262866},
	{0.147621, 0.716567, -0.681718},
	{0.309017, 0.500000, -0.809017},
	{0.425325, 0.688191, -0.587785},
	{0.442863, 0.238856, -0.864188},
	{0.587785, 0.425325, -0.688191},
	{0.688191, 0.587785, -0.425325},
	{-0.147621, 0.716567, -0.681718},
	{-0.309017, 0.500000, -0.809017},
	{0.000000, 0.525731, -0.850651},
	{-0.525731, 0.000000, -0.850651},
	{-0.442863, 0.238856, -0.864188},
	{-0.295242, 0.000000, -0.955423},
	{-0.162460, 0.262866, -0.951056},
	{0.000000, 0.000000, -1.000000},
	{0.295242, 0.000000, -0.955423},
	{0.162460, 0.262866, -0.951056},
	{-0.442863, -0.238856, -0.864188},
	{-0.309017, -0.500000, -0.809017},
	{-0.162460, -0.262866, -0.951056},
	{0.000000, -0.850651, -0.525731},
	{-0.147621, -0.716567, -0.681718},
	{0.147621, -0.716567, -0.681718},
	{0.000000, -0.525731, -0.850651},
	{0.309017, -0.500000, -0.809017},
	{0.442863, -0.238856, -0.864188},
	{0.162460, -0.262866, -0.951056},
	{0.238856, -0.864188, -0.442863},
	{0.500000, -0.809017, -0.309017},
	{0.425325, -0.688191, -0.587785},
	{0.716567, -0.681718, -0.147621},
	{0.688191, -0.587785, -0.425325},
	{0.587785, -0.425325, -0.688191},
	{0.000000, -0.955423, -0.295242},
	{0.000000, -1.000000, 0.000000},
	{0.262866, -0.951056, -0.162460},
	{0.000000, -0.850651, 0.525731},
	{0.000000, -0.955423, 0.295242},
	{0.238856, -0.864188, 0.442863},
	{0.262866, -0.951056, 0.162460},
	{0.500000, -0.809017, 0.309017},
	{0.716567, -0.681718, 0.147621},
	{0.525731, -0.850651, 0.000000},
	{-0.238856, -0.864188, -0.442863},
	{-0.500000, -0.809017, -0.309017},
	{-0.262866, -0.951056, -0.162460},
	{-0.850651, -0.525731, 0.000000},
	{-0.716567, -0.681718, -0.147621},
	{-0.716567, -0.681718, 0.147621},
	{-0.525731, -0.850651, 0.000000},
	{-0.500000, -0.809017, 0.309017},
	{-0.238856, -0.864188, 0.442863},
	{-0.262866, -0.951056, 0.162460},
	{-0.864188, -0.442863, 0.238856},
	{-0.809017, -0.309017, 0.500000},
	{-0.688191, -0.587785, 0.425325},
	{-0.681718, -0.147621, 0.716567},
	{-0.442863, -0.238856, 0.864188},
	{-0.587785, -0.425325, 0.688191},
	{-0.309017, -0.500000, 0.809017},
	{-0.147621, -0.716567, 0.681718},
	{-0.425325, -0.688191, 0.587785},
	{-0.162460, -0.262866, 0.951056},
	{0.442863, -0.238856, 0.864188},
	{0.162460, -0.262866, 0.951056},
	{0.309017, -0.500000, 0.809017},
	{0.147621, -0.716567, 0.681718},
	{0.000000, -0.525731, 0.850651},
	{0.425325, -0.688191, 0.587785},
	{0.587785, -0.425325, 0.688191},
	{0.688191, -0.587785, 0.425325},
	{-0.955423, 0.295242, 0.000000},
	{-0.951056, 0.162460, 0.262866},
	{-1.000000, 0.000000, 0.000000},
	{-0.850651, 0.000000, 0.525731},
	{-0.955423, -0.295242, 0.000000},
	{-0.951056, -0.162460, 0.262866},
	{-0.864188, 0.442863, -0.238856},
	{-0.951056, 0.162460, -0.262866},
	{-0.809017, 0.309017, -0.500000},
	{-0.864188, -0.442863, -0.238856},
	{-0.951056, -0.162460, -0.262866},
	{-0.809017, -0.309017, -0.500000},
	{-0.681718, 0.147621, -0.716567},
	{-0.681718, -0.147621, -0.716567},
	{-0.850651, 0.000000, -0.525731},
	{-0.688191, 0.587785, -0.425325},
	{-0.587785, 0.425325, -0.688191},
	{-0.425325, 0.688191, -0.587785},
	{-0.425325, -0.688191, -0.587785},
	{-0.587785, -0.425325, -0.688191},
	{-0.688191, -0.587785, -0.425325},
};

static const aedge_t aedges[12] = {
	{0, 1}, {1, 2}, {2, 3}, {3, 0},
	{4, 5}, {5, 6}, {6, 7}, {7, 4},
	{0, 5}, {1, 4}, {2, 7}, {3, 6}
};

static int cl_minlight = 1;	/* FIXME */
static int d_lightstylevalue[256];
static dlight_t cl_dlights[MAX_DLIGHTS];

/* end of r_alias symbols */

static float _mathlib_temp_float1;
static vec3_t _mathlib_temp_vec1;

#define VectorL2Compare(v, w, m)                        \
    (_mathlib_temp_float1 = (m) * (m),                      \
    _mathlib_temp_vec1[0] = (v)[0] - (w)[0], _mathlib_temp_vec1[1] = (v)[1] - (w)[1], _mathlib_temp_vec1[2] = (v)[2] - (w)[2],  \
    _mathlib_temp_vec1[0] * _mathlib_temp_vec1[0] +     \
    _mathlib_temp_vec1[1] * _mathlib_temp_vec1[1] +     \
    _mathlib_temp_vec1[2] * _mathlib_temp_vec1[2] < _mathlib_temp_float1)

#define VectorInterpolate(v1, _frac, v2, v)                         \
do {                                                                \
    _mathlib_temp_float1 = _frac;                                   \
                                                                    \
    (v)[0] = (v1)[0] + _mathlib_temp_float1 * ((v2)[0] - (v1)[0]);  \
    (v)[1] = (v1)[1] + _mathlib_temp_float1 * ((v2)[1] - (v1)[1]);  \
    (v)[2] = (v1)[2] + _mathlib_temp_float1 * ((v2)[2] - (v1)[2]);  \
} while (0);

#define DotProduct(x,y)         ((x)[0] * (y)[0] + (x)[1] * (y)[1] + (x)[2] * (y)[2])

static void R_AliasTransformFinalVert(entity_t * ent, finalvert_t * fv,
				      auxvert_t * av, trivertx_t * pverts1,
				      trivertx_t * pverts2, stvert_t * pstverts,
				      float r_framelerp, float r_lerpdistance,
				      int r_ambientlight, struct r_vects *rv,
				      float r_shadelight)
/*
Definition at line 373 of file r_alias.c.

References aliastransform, currententity, DotProduct, finalvert_s::flags, auxvert_t::fv, int, trivertx_t::lightnormalindex, max, stvert_t::onseam, r_ambientlight, r_avertexnormals, r_framelerp, r_lerpdistance, r_plightvec, r_shadelight, entity_s::renderfx, RF_LIMITLERP, stvert_t::s, stvert_t::t, finalvert_s::v, trivertx_t::v, VectorInterpolate, and VectorL2Compare.

Referenced by R_AliasPreparePoints().
*/
{
	int temp;
	float lightcos, lerpfrac;
	vec3_t interpolated_verts, interpolated_norm;

	lerpfrac = r_framelerp;
	if ((ent->renderfx & RF_LIMITLERP)) {

		lerpfrac =
		    VectorL2Compare(pverts1->v, pverts2->v,
				    r_lerpdistance) ? r_framelerp : 1;
	}

	VectorInterpolate(pverts1->v, lerpfrac, pverts2->v, interpolated_verts);

	av->fv[0] =
	    DotProduct(interpolated_verts,
		       aliastransform[0]) + aliastransform[0][3];
	av->fv[1] =
	    DotProduct(interpolated_verts,
		       aliastransform[1]) + aliastransform[1][3];
	av->fv[2] =
	    DotProduct(interpolated_verts,
		       aliastransform[2]) + aliastransform[2][3];

	fv->v[2] = pstverts->s;
	fv->v[3] = pstverts->t;

	fv->flags = pstverts->onseam;

	// lighting
	VectorInterpolate(r_avertexnormals[pverts1->lightnormalindex], lerpfrac,
			  r_avertexnormals[pverts1->lightnormalindex],
			  interpolated_norm);
	lightcos = DotProduct(interpolated_norm, rv->r_plightvec);
	temp = r_ambientlight;

	if (lightcos < 0) {
		temp += (int)(r_shadelight * lightcos);
		// clamp; because we limited the minimum ambient and shading light, we don't have to clamp low light, just bright
		temp = max(temp, 0);
	}
	fv->v[4] = temp;
}

static void R_AliasProjectFinalVert(finalvert_t * fv, auxvert_t * av,
				    float ziscale, float aliasxscale,
				    float aliasyscale, float aliasxcenter,
				    float aliasycenter)
/*
Definition at line 456 of file r_alias.c.

References aliasxcenter, aliasxscale, aliasycenter, aliasyscale, auxvert_t::fv, finalvert_s::v, and ziscale.

Referenced by R_Alias_clip_z(), and R_AliasPreparePoints().
*/
{
	float zi;

	// project points
	zi = 1.0 / av->fv[2];
	fv->v[5] = zi * ziscale;
	fv->v[0] = (av->fv[0] * aliasxscale * zi) + aliasxcenter;
	fv->v[1] = (av->fv[1] * aliasyscale * zi) + aliasycenter;
}

struct r_scale {
	float ziscale;
	float aliasxscale;
	float aliasyscale;
	float aliasxcenter;
	float aliasycenter;
	float r_nearclip;
	finalvert_t fv[2][8];
	auxvert_t av[8];
};

static int R_AliasClip(finalvert_t * in, finalvert_t * out, int flag, int count,
		       void (*clip) (finalvert_t * pfv0, finalvert_t * pfv1,
				     finalvert_t * out, struct r_scale * rs),
		       struct r_scale *rs)
/*
Definition at line 200 of file r_aclip.c.

References ALIAS_BOTTOM_CLIP, ALIAS_LEFT_CLIP, ALIAS_RIGHT_CLIP, ALIAS_TOP_CLIP, refdef_t::aliasvrect, refdef_t::aliasvrectbottom, refdef_t::aliasvrectright, count, finalvert_s::flags, r_refdef, vrect_s::x, and vrect_s::y.

Referenced by R_AliasClipTriangle().
*/
{
	int i, j, k;
	int flags, oldflags;

	j = count - 1;
	k = 0;
	for (i = 0; i < count; j = i, i++) {
		oldflags = in[j].flags & flag;
		flags = in[i].flags & flag;

		if (flags && oldflags)
			continue;
		if (oldflags ^ flags) {
			clip(&in[j], &in[i], &out[k], rs);
			out[k].flags = 0;
			if (out[k].v[0] < r_refdef.aliasvrect.x)
				out[k].flags |= ALIAS_LEFT_CLIP;
			if (out[k].v[1] < r_refdef.aliasvrect.y)
				out[k].flags |= ALIAS_TOP_CLIP;
			if (out[k].v[0] > r_refdef.aliasvrectright)
				out[k].flags |= ALIAS_RIGHT_CLIP;
			if (out[k].v[1] > r_refdef.aliasvrectbottom)
				out[k].flags |= ALIAS_BOTTOM_CLIP;
			k++;
		}
		if (!flags) {
			out[k] = in[i];
			k++;
		}
	}

	return k;
}

/*
================
R_Alias_clip_z

pfv0 is the unclipped vertex, pfv1 is the z-clipped vertex
================
*/
static void R_Alias_clip_z(finalvert_t * pfv0, finalvert_t * pfv1,
			   finalvert_t * out, struct r_scale *rs)
/*
Definition at line 56 of file r_aclip.c.

References ALIAS_BOTTOM_CLIP, ALIAS_LEFT_CLIP, ALIAS_RIGHT_CLIP, ALIAS_TOP_CLIP, refdef_t::aliasvrect, refdef_t::aliasvrectbottom, refdef_t::aliasvrectright, finalvert_s::flags, auxvert_t::fv, R_AliasProjectFinalVert(), r_nearclip, r_refdef, finalvert_s::v, cvar_s::value, vrect_s::x, and vrect_s::y.

Referenced by R_AliasClipTriangle().
*/
{
	float scale;
	auxvert_t *pav0, *pav1, avout;

	pav0 = &rs->av[pfv0 - &rs->fv[0][0]];
	pav1 = &rs->av[pfv1 - &rs->fv[0][0]];

	if (pfv0->v[1] >= pfv1->v[1]) {
		scale = (rs->r_nearclip - pav0->fv[2]) /
		    (pav1->fv[2] - pav0->fv[2]);

		avout.fv[0] = pav0->fv[0] + (pav1->fv[0] - pav0->fv[0]) * scale;
		avout.fv[1] = pav0->fv[1] + (pav1->fv[1] - pav0->fv[1]) * scale;
		avout.fv[2] = rs->r_nearclip;

		out->v[2] = pfv0->v[2] + (pfv1->v[2] - pfv0->v[2]) * scale;
		out->v[3] = pfv0->v[3] + (pfv1->v[3] - pfv0->v[3]) * scale;
		out->v[4] = pfv0->v[4] + (pfv1->v[4] - pfv0->v[4]) * scale;
	} else {
		scale = (rs->r_nearclip - pav1->fv[2]) /
		    (pav0->fv[2] - pav1->fv[2]);

		avout.fv[0] = pav1->fv[0] + (pav0->fv[0] - pav1->fv[0]) * scale;
		avout.fv[1] = pav1->fv[1] + (pav0->fv[1] - pav1->fv[1]) * scale;
		avout.fv[2] = rs->r_nearclip;

		out->v[2] = pfv1->v[2] + (pfv0->v[2] - pfv1->v[2]) * scale;
		out->v[3] = pfv1->v[3] + (pfv0->v[3] - pfv1->v[3]) * scale;
		out->v[4] = pfv1->v[4] + (pfv0->v[4] - pfv1->v[4]) * scale;
	}

	R_AliasProjectFinalVert(out, &avout, rs->ziscale,
				rs->aliasxscale,
				rs->aliasyscale,
				rs->aliasxcenter, rs->aliasycenter);

	if (out->v[0] < r_refdef.aliasvrect.x)
		out->flags |= ALIAS_LEFT_CLIP;
	if (out->v[1] < r_refdef.aliasvrect.y)
		out->flags |= ALIAS_TOP_CLIP;
	if (out->v[0] > r_refdef.aliasvrectright)
		out->flags |= ALIAS_RIGHT_CLIP;
	if (out->v[1] > r_refdef.aliasvrectbottom)
		out->flags |= ALIAS_BOTTOM_CLIP;
}

static void R_Alias_clip_left(finalvert_t * pfv0, finalvert_t * pfv1,
			      finalvert_t * out, struct r_scale *rs)
/*
Definition at line 106 of file r_aclip.c.

References refdef_t::aliasvrect, r_refdef, finalvert_s::v, and vrect_s::x.

Referenced by R_AliasClipTriangle().
*/
{
	float scale;
	int i;

	if (pfv0->v[1] >= pfv1->v[1]) {
		scale = (float)(r_refdef.aliasvrect.x - pfv0->v[0]) /
		    (pfv1->v[0] - pfv0->v[0]);
		for (i = 0; i < 6; i++)
			out->v[i] =
			    pfv0->v[i] + (pfv1->v[i] - pfv0->v[i]) * scale +
			    0.5;
	} else {
		scale = (float)(r_refdef.aliasvrect.x - pfv1->v[0]) /
		    (pfv0->v[0] - pfv1->v[0]);
		for (i = 0; i < 6; i++)
			out->v[i] =
			    pfv1->v[i] + (pfv0->v[i] - pfv1->v[i]) * scale +
			    0.5;
	}
}

static void R_Alias_clip_right(finalvert_t * pfv0, finalvert_t * pfv1,
			       finalvert_t * out, struct r_scale *rs)
/*
Definition at line 128 of file r_aclip.c.

References refdef_t::aliasvrectright, r_refdef, and finalvert_s::v.

Referenced by R_AliasClipTriangle().
*/
{
	float scale;
	int i;

	if (pfv0->v[1] >= pfv1->v[1]) {
		scale = (float)(r_refdef.aliasvrectright - pfv0->v[0]) /
		    (pfv1->v[0] - pfv0->v[0]);
		for (i = 0; i < 6; i++)
			out->v[i] =
			    pfv0->v[i] + (pfv1->v[i] - pfv0->v[i]) * scale +
			    0.5;
	} else {
		scale = (float)(r_refdef.aliasvrectright - pfv1->v[0]) /
		    (pfv0->v[0] - pfv1->v[0]);
		for (i = 0; i < 6; i++)
			out->v[i] =
			    pfv1->v[i] + (pfv0->v[i] - pfv1->v[i]) * scale +
			    0.5;
	}
}

static void R_Alias_clip_bottom(finalvert_t * pfv0, finalvert_t * pfv1,
				finalvert_t * out, struct r_scale *rs)
/*
Definition at line 173 of file r_aclip.c.

References refdef_t::aliasvrectbottom, r_refdef, and finalvert_s::v.

Referenced by R_AliasClipTriangle().
*/
{
	float scale;
	int i;

	if (pfv0->v[1] >= pfv1->v[1]) {
		scale = (float)(r_refdef.aliasvrectbottom - pfv0->v[1]) /
		    (pfv1->v[1] - pfv0->v[1]);

		for (i = 0; i < 6; i++)
			out->v[i] =
			    pfv0->v[i] + (pfv1->v[i] - pfv0->v[i]) * scale +
			    0.5;
	} else {
		scale = (float)(r_refdef.aliasvrectbottom - pfv1->v[1]) /
		    (pfv0->v[1] - pfv1->v[1]);

		for (i = 0; i < 6; i++)
			out->v[i] =
			    pfv1->v[i] + (pfv0->v[i] - pfv1->v[i]) * scale +
			    0.5;
	}
}

static void R_Alias_clip_top(finalvert_t * pfv0, finalvert_t * pfv1,
			     finalvert_t * out, struct r_scale *rs)
/*
Definition at line 151 of file r_aclip.c.

References refdef_t::aliasvrect, r_refdef, finalvert_s::v, and vrect_s::y.

Referenced by R_AliasClipTriangle().
*/
{
	float scale;
	int i;

	if (pfv0->v[1] >= pfv1->v[1]) {
		scale = (float)(r_refdef.aliasvrect.y - pfv0->v[1]) /
		    (pfv1->v[1] - pfv0->v[1]);
		for (i = 0; i < 6; i++)
			out->v[i] =
			    pfv0->v[i] + (pfv1->v[i] - pfv0->v[i]) * scale +
			    0.5;
	} else {
		scale = (float)(r_refdef.aliasvrect.y - pfv1->v[1]) /
		    (pfv0->v[1] - pfv1->v[1]);
		for (i = 0; i < 6; i++)
			out->v[i] =
			    pfv1->v[i] + (pfv0->v[i] - pfv1->v[i]) * scale +
			    0.5;
	}
}

static int R_AliasClipTriangle(mtriangle_t * ptri, const struct r_view *v,
				byte * acolormap,
				int skinwidth,
				auxvert_t * pauxverts,
				finalvert_t * pfinalverts, struct r_scale *rs,
				byte **skintable, struct r_finalmesh *fm)
/*
Definition at line 245 of file r_aclip.c.

References ALIAS_BOTTOM_CLIP, ALIAS_LEFT_CLIP, ALIAS_ONSEAM, ALIAS_RIGHT_CLIP, ALIAS_TOP_CLIP, ALIAS_Z_CLIP, refdef_t::aliasvrect, refdef_t::aliasvrectbottom, refdef_t::aliasvrectright, D_PolysetDraw(), mtriangle_s::facesfront, finalvert_s::flags, pauxverts, affinetridesc_t::pfinalverts, pfinalverts, affinetridesc_t::ptriangles, r_affinetridesc, R_Alias_clip_bottom(), R_Alias_clip_left(), R_Alias_clip_right(), R_Alias_clip_top(), R_Alias_clip_z(), R_AliasClip(), r_refdef, affinetridesc_t::seamfixupX16, finalvert_s::v, mtriangle_s::vertindex, vrect_s::x, and vrect_s::y.

Referenced by R_AliasPreparePoints().
*/
{
	int i, k, pingpong;
	mtriangle_t mtri;
	unsigned clipflags;

// copy vertexes and fix seam texture coordinates
	if (ptri->facesfront) {
		rs->fv[0][0] = pfinalverts[ptri->vertindex[0]];
		rs->fv[0][1] = pfinalverts[ptri->vertindex[1]];
		rs->fv[0][2] = pfinalverts[ptri->vertindex[2]];
	} else {
		printf("%s:%d not facesfront\n", __func__, __LINE__);
		rs->fv[0][0] = pfinalverts[ptri->vertindex[0]];
		for (i = 0; i < 3; i++) {
			rs->fv[0][i] = pfinalverts[ptri->vertindex[i]];

			if (!ptri->facesfront
			    && (rs->fv[0][i].flags & ALIAS_ONSEAM))
				rs->fv[0][i].v[2] += fm->seamfixupX16;
		}
	}

// clip
	clipflags = rs->fv[0][0].flags | rs->fv[0][1].flags | rs->fv[0][2].flags;

	if (clipflags & ALIAS_Z_CLIP) {
		for (i = 0; i < 3; i++)
			rs->av[i] = pauxverts[ptri->vertindex[i]];

		k = R_AliasClip(rs->fv[0], rs->fv[1], ALIAS_Z_CLIP, 3, R_Alias_clip_z,
				rs);
		if (k == 0)
			return 0;

		pingpong = 1;
		clipflags = rs->fv[1][0].flags | rs->fv[1][1].flags | rs->fv[1][2].flags;
	} else {
		pingpong = 0;
		k = 3;
	}

	if (clipflags & ALIAS_LEFT_CLIP) {
		k = R_AliasClip(rs->fv[pingpong], rs->fv[pingpong ^ 1],
				ALIAS_LEFT_CLIP, k, R_Alias_clip_left, rs);
		if (k == 0)
			return 0;

		pingpong ^= 1;
	}

	if (clipflags & ALIAS_RIGHT_CLIP) {
		k = R_AliasClip(rs->fv[pingpong], rs->fv[pingpong ^ 1],
				ALIAS_RIGHT_CLIP, k, R_Alias_clip_right, rs);
		if (k == 0)
			return 0;

		pingpong ^= 1;
	}

	if (clipflags & ALIAS_BOTTOM_CLIP) {
		k = R_AliasClip(rs->fv[pingpong], rs->fv[pingpong ^ 1],
				ALIAS_BOTTOM_CLIP, k, R_Alias_clip_bottom, rs);
		if (k == 0)
			return 0;

		pingpong ^= 1;
	}

	if (clipflags & ALIAS_TOP_CLIP) {
		k = R_AliasClip(rs->fv[pingpong], rs->fv[pingpong ^ 1],
				ALIAS_TOP_CLIP, k, R_Alias_clip_top, rs);
		if (k == 0)
			return 0;

		pingpong ^= 1;
	}

	for (i = 0; i < k; i++) {
		if (rs->fv[pingpong][i].v[0] < r_refdef.aliasvrect.x)
			rs->fv[pingpong][i].v[0] = r_refdef.aliasvrect.x;
		else if (rs->fv[pingpong][i].v[0] > r_refdef.aliasvrectright)
			rs->fv[pingpong][i].v[0] = r_refdef.aliasvrectright;

		if (rs->fv[pingpong][i].v[1] < r_refdef.aliasvrect.y)
			rs->fv[pingpong][i].v[1] = r_refdef.aliasvrect.y;
		else if (rs->fv[pingpong][i].v[1] > r_refdef.aliasvrectbottom)
			rs->fv[pingpong][i].v[1] = r_refdef.aliasvrectbottom;

		rs->fv[pingpong][i].flags = 0;
	}

// draw triangles
	fm->finalverts = rs->fv[pingpong];
	return k;
}

static void R_AliasPreparePoints(entity_t * ent, const struct r_view *v,
				 byte * acolormap,
				 int skinwidth, mdl_t * pmdl,
				 aliashdr_t * paliashdr, auxvert_t * pauxverts,
				 struct r_verts *rv, finalvert_t * pfinalverts,
				 float r_framelerp, float ziscale,
				 float r_lerpdistance, int r_ambientlight,
				 struct r_scale *rs, struct r_vects *rve,
				 float r_shadelight, byte **skintable, struct r_finalmesh *fm, int drawtype)
/*
Definition at line 254 of file r_alias.c.

References ALIAS_BOTTOM_CLIP, ALIAS_LEFT_CLIP, ALIAS_RIGHT_CLIP, ALIAS_TOP_CLIP, ALIAS_XY_CLIP_MASK, ALIAS_Z_CLIP, refdef_t::aliasvrect, refdef_t::aliasvrectbottom, refdef_t::aliasvrectright, av, D_PolysetDraw(), fv, affinetridesc_t::numtriangles, mdl_t::numtris, mdl_t::numverts, pauxverts, affinetridesc_t::pfinalverts, pfinalverts, affinetridesc_t::ptriangles, R_AliasClipTriangle(), R_AliasProjectFinalVert(), R_AliasTransformFinalVert(), r_anumverts, r_nearclip, r_refdef, aliashdr_s::stverts, aliashdr_s::triangles, cvar_s::value, mtriangle_s::vertindex, vrect_s::x, and vrect_s::y.

Referenced by R_AliasDrawModel().
*/
{
	int i;
	stvert_t *pstverts;
	finalvert_t *fv;
	auxvert_t *av;
	mtriangle_t *ptri;
	finalvert_t *pfv[3];
	int r_anumverts;

	pstverts = (stvert_t *) ((byte *) paliashdr + paliashdr->stverts);
	r_anumverts = pmdl->numverts;
	fv = pfinalverts;
	av = pauxverts;

	for (i = 0; i < r_anumverts;
	     i++, fv++, av++, rv->r_oldapverts++, rv->r_apverts++, pstverts++) {
		R_AliasTransformFinalVert(ent, fv, av, rv->r_oldapverts,
					  rv->r_apverts, pstverts, r_framelerp,
					  r_lerpdistance, r_ambientlight, rve,
					  r_shadelight);
		if (av->fv[2] < 1.f) {
			fv->flags |= ALIAS_Z_CLIP;
		} else {
			R_AliasProjectFinalVert(fv, av, rs->ziscale,
						rs->aliasxscale,
						rs->aliasyscale,
						rs->aliasxcenter,
						rs->aliasycenter);

			if (fv->v[0] < r_refdef.aliasvrect.x)
				fv->flags |= ALIAS_LEFT_CLIP;
			if (fv->v[1] < r_refdef.aliasvrect.y)
				fv->flags |= ALIAS_TOP_CLIP;
			if (fv->v[0] > r_refdef.aliasvrectright)
				fv->flags |= ALIAS_RIGHT_CLIP;
			if (fv->v[1] > r_refdef.aliasvrectbottom)
				fv->flags |= ALIAS_BOTTOM_CLIP;
		}
	}

	fm->ntriangles = 1;

	ptri = (mtriangle_t *) ((byte *) paliashdr + paliashdr->triangles);
	for (i = 0; i < pmdl->numtris; i++, ptri++) {
		pfv[0] = &pfinalverts[ptri->vertindex[0]];
		pfv[1] = &pfinalverts[ptri->vertindex[1]];
		pfv[2] = &pfinalverts[ptri->vertindex[2]];

		if (pfv[0]->flags & pfv[1]->
		    flags & pfv[2]->flags & (ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP))
			continue;	// completely clipped

		if (!
		    ((pfv[0]->flags | pfv[1]->
		      flags | pfv[2]->flags) & (ALIAS_XY_CLIP_MASK |
						ALIAS_Z_CLIP))) {
			// totally unclipped
			fm->finalverts = pfinalverts;
			fm->triangles = ptri;
			D_PolysetDraw(v, acolormap, skintable, skinwidth, fm, drawtype);
		} else {
			// partially clipped
			int i;
			mtriangle_t mtri;
			int ntris = R_AliasClipTriangle(ptri, v, acolormap,
					    skinwidth, pauxverts, pfinalverts,
					    rs, skintable, fm);
			fm->triangles = &mtri;
			for (i = 1; i < ntris - 1; i++) {
				fm->triangles[0].facesfront = ptri->facesfront;
				fm->triangles[0].vertindex[0] = 0;
				fm->triangles[0].vertindex[1] = i;
				fm->triangles[0].vertindex[2] = i + 1;
				D_PolysetDraw(v, acolormap, skintable, skinwidth, fm, drawtype);
			}
		}
	}
}

static void R_AliasTransformAndProjectFinalVerts(finalvert_t * fv,
						 stvert_t * pstverts,
						 int r_anumverts,
						 struct r_verts *rv,
						 float r_framelerp,
						 int r_ambientlight,
						 struct r_scale *rs,
						 struct r_vects *rve,
						 float r_shadelight)
/*
Definition at line 412 of file r_alias.c.

References aliastransform, aliasxcenter, aliasycenter, DotProduct, int, max, stvert_t::onseam, r_ambientlight, r_anumverts, r_apverts, r_avertexnormals, r_framelerp, r_oldapverts, r_plightvec, r_shadelight, stvert_t::s, stvert_t::t, and VectorInterpolate.

Referenced by R_AliasPrepareUnclippedPoints().
*/
{
	int i, temp;
	float lightcos, zi;
	trivertx_t *pverts1, *pverts2;
	vec3_t interpolated_verts, interpolated_norm;

	pverts1 = rv->r_oldapverts;
	pverts2 = rv->r_apverts;

	for (i = 0; i < r_anumverts;
	     i++, fv++, pverts1++, pverts2++, pstverts++) {

		VectorInterpolate(pverts1->v, r_framelerp, pverts2->v,
				  interpolated_verts);
		// transform and project
		zi = 1.0 / (DotProduct(interpolated_verts, aliastransform[2]) +
			    aliastransform[2][3]);

		// x, y, and z are scaled down by 1/2**31 in the transform, so 1/z is
		// scaled up by 1/2**31, and the scaling cancels out for x and y in the projection
		fv->v[5] = zi;

		fv->v[0] =
		    ((DotProduct(interpolated_verts, aliastransform[0]) +
		      aliastransform[0][3]) * zi) + rs->aliasxcenter;
		fv->v[1] =
		    ((DotProduct(interpolated_verts, aliastransform[1]) +
		      aliastransform[1][3]) * zi) + rs->aliasycenter;
#if 0
		printf("%s:%d fv[0] = %d fv[1] = %d\n", __func__, __LINE__,
		       fv->v[0], fv->v[1]);
#endif

		fv->v[2] = pstverts->s;
		fv->v[3] = pstverts->t;
		fv->flags = pstverts->onseam;

		// lighting
		VectorInterpolate(r_avertexnormals[pverts1->lightnormalindex],
				  r_framelerp,
				  r_avertexnormals[pverts1->lightnormalindex],
				  interpolated_norm);
		lightcos = DotProduct(interpolated_norm, rve->r_plightvec);
		temp = r_ambientlight;

		if (lightcos < 0) {
			temp += (int)(r_shadelight * lightcos);
			// clamp; because we limited the minimum ambient and shading light, we don't have to clamp low light, just bright
			temp = max(temp, 0);
		}

		fv->v[4] = temp;
	}
}

static void D_PolysetDrawFinalVerts(finalvert_t * fv, int numverts, const struct r_view *v,
				    byte * acolormap, byte **skintable)
/*
Definition at line 157 of file d_polyse.c.

References acolormap, d_scantable, d_viewbuffer, r_refdef, skintable, finalvert_s::v, refdef_t::vrectbottom, refdef_t::vrectright, and zspantable.

Referenced by R_AliasPrepareUnclippedPoints().
*/
{
	int i, z;
	short *zbuf;

	for (i = 0; i < numverts; i++, fv++) {
		// valid triangle coordinates for filling can include the bottom and
		// right clip edges, due to the fill rule; these shouldn't be drawn
		if ((fv->v[0] < r_refdef.vrectright) &&
		    (fv->v[1] < r_refdef.vrectbottom)) {
			z = fv->v[5] >> 16;
			zbuf = v->zspantable[fv->v[1]] + fv->v[0];
			if (z >= *zbuf) {
				int pix;

				*zbuf = z;
				pix = skintable[fv->v[3] >> 16][fv->v[2] >> 16];
				pix = acolormap[pix + (fv->v[4] & 0xFF00)];
				v->viewbuffer[v->d_scantable[fv->v[1]] + fv->v[0]] =
				    pix;
#if 0
				printf("plot %d %d %d = %d\n", fv->v[1],
				       v->d_scantable[fv->v[1]], fv->v[0], pix);
#endif
			} else {
#if 0
				printf("zbuffer\n");
#endif
			}
		}
	}
}

static void R_AliasPrepareUnclippedPoints(const struct r_view *v,
					  byte * acolormap,
					  int skinwidth,
					  mdl_t * pmdl, aliashdr_t * paliashdr,
					  struct r_verts *rv,
					  finalvert_t * pfinalverts,
					  float r_framelerp, int r_ambientlight,
					  struct r_scale *rs,
					  struct r_vects *rve,
					  float r_shadelight,
					  byte **skintable, struct r_finalmesh *fm, int drawtype)
/*
Definition at line 466 of file r_alias.c.

References D_PolysetDraw(), D_PolysetDrawFinalVerts(), affinetridesc_t::drawtype, affinetridesc_t::numtriangles, mdl_t::numtris, mdl_t::numverts, pfinalverts, affinetridesc_t::pfinalverts, affinetridesc_t::ptriangles, R_AliasTransformAndProjectFinalVerts(), r_anumverts, aliashdr_s::stverts, and aliashdr_s::triangles.

Referenced by R_AliasDrawModel().
*/
{
	stvert_t *pstverts;
	int r_anumverts;

	pstverts = (stvert_t *) ((byte *) paliashdr + paliashdr->stverts);
	r_anumverts = pmdl->numverts;

	R_AliasTransformAndProjectFinalVerts(pfinalverts, pstverts,
					     r_anumverts, rv, r_framelerp,
					     r_ambientlight, rs, rve,
					     r_shadelight);

	if (drawtype)
		D_PolysetDrawFinalVerts(pfinalverts, r_anumverts, v,
					acolormap, skintable);

	fm->finalverts = pfinalverts;
	fm->triangles =
	    (mtriangle_t *) ((byte *) paliashdr + paliashdr->triangles);
	fm->ntriangles = pmdl->numtris;
}

#if 0
static void Cache_UnlinkLRU(cache_system_t * cs)
/*
Definition at line 312 of file zone.c.

References cache_system_s::lru_next, cache_system_s::lru_prev, NULL, and Sys_Error().

Referenced by Cache_Check(), and Cache_Free().
*/
{
	if (!cs->lru_next || !cs->lru_prev)
		Sys_Error("Cache_UnlinkLRU: NULL link");

	cs->lru_next->lru_prev = cs->lru_prev;
	cs->lru_prev->lru_next = cs->lru_next;

	cs->lru_prev = cs->lru_next = NULL;
}

static void Cache_MakeLRU(cache_system_t * cs)
/*
Definition at line 322 of file zone.c.

References cache_head, cache_system_s::lru_next, cache_system_s::lru_prev, and Sys_Error().

Referenced by Cache_Check(), and Cache_TryAlloc().
*/
{
	if (cs->lru_next || cs->lru_prev)
		Sys_Error("Cache_MakeLRU: active link");

	cache_head.lru_next->lru_prev = cs;
	cs->lru_next = cache_head.lru_next;
	cs->lru_prev = &cache_head;
	cache_head.lru_next = cs;
}

static void *Cache_Check(cache_user_t * c)
/*
Definition at line 449 of file zone.c.

References Cache_MakeLRU(), Cache_UnlinkLRU(), cache_user_s::data, and NULL.

Referenced by Cache_Alloc(), Mod_Extradata(), Mod_LoadModel(), Mod_TouchModel(), S_LoadSound(), S_RawAudio(), S_RawClearStream(), S_SoundList_f(), and Skin_Cache().
*/
{
	cache_system_t *cs;

	if (!c->data)
		return NULL;

	cs = ((cache_system_t *) c->data) - 1;

	// move to head of LRU
	Cache_UnlinkLRU(cs);
	Cache_MakeLRU(cs);

	return c->data;
}
#endif

/* Unimplemented stub */
static byte *FS_LoadTempFile(char *path, int *len)
{
	struct stat s;
	int fd;
	byte *p;
	int ret = stat(path, &s);
	if (ret < 0)
		return NULL;
	p = malloc(s.st_size);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return NULL;
	*len = read(fd, p, s.st_size);
	close(fd);
	return p;
}

static int hunk_low_used = 0;
static int Hunk_LowMark(void)
{
	return hunk_low_used;
}

#if 0
void *Hunk_AllocName(int size, char *name)
{
	hunk_low_used += size;
	return malloc(size);
}
#endif

#define LittleLong(x)	(x)
#define LittleFloat(x)	(x)
static void *Mod_LoadAliasSkin(void *pin, int *pskinindex, int skinsize,
			       aliashdr_t * pheader)
/*
Definition at line 1017 of file r_model.c.

References Hunk_AllocName(), loadname, and pheader.

Referenced by Mod_LoadAliasSkinGroup().
*/
{
	byte *pskin, *pinskin;

	pskin = (byte *) malloc(skinsize);
	pinskin = (byte *) pin;
	*pskinindex = (byte *) pskin - (byte *) pheader;

	memcpy(pskin, pinskin, skinsize);

	pinskin += skinsize;

	return ((void *)pinskin);
}

static void *Mod_LoadAliasSkinGroup(void *pin, int *pskinindex, int skinsize,
				    aliashdr_t * pheader)
{
	daliasskingroup_t *pinskingroup;
	maliasskingroup_t *paliasskingroup;
	int i, numskins;
	daliasskininterval_t *pinskinintervals;
	float *poutskinintervals;
	void *ptemp;

	pinskingroup = (daliasskingroup_t *) pin;

	numskins = LittleLong(pinskingroup->numskins);

	paliasskingroup =
	    (maliasskingroup_t *) malloc(sizeof(maliasskingroup_t) +
					 (numskins -
					  1) *
					 sizeof(paliasskingroup->skindescs[0]));

	paliasskingroup->numskins = numskins;

	*pskinindex = (byte *) paliasskingroup - (byte *) pheader;

	pinskinintervals = (daliasskininterval_t *) (pinskingroup + 1);

	poutskinintervals = (float *)malloc(numskins * sizeof(float));

	paliasskingroup->intervals =
	    (byte *) poutskinintervals - (byte *) pheader;

	for (i = 0; i < numskins; i++) {
		*poutskinintervals = LittleFloat(pinskinintervals->interval);
		if (*poutskinintervals <= 0)
			Host_Error("Mod_LoadAliasSkinGroup: interval<=0");

		poutskinintervals++;
		pinskinintervals++;
	}

	ptemp = (void *)pinskinintervals;

	for (i = 0; i < numskins; i++)
		ptemp =
		    Mod_LoadAliasSkin(ptemp,
				      &paliasskingroup->skindescs[i].skin,
				      skinsize, pheader);

	return ptemp;
}

static void *Mod_LoadAliasFrame(void *pin, int *pframeindex, int numv,
				trivertx_t * pbboxmin, trivertx_t * pbboxmax,
				aliashdr_t * pheader, char *name)
/*
Definition at line 926 of file r_model.c.

References daliasframe_t::bboxmax, daliasframe_t::bboxmin, Hunk_AllocName(), trivertx_t::lightnormalindex, loadname, daliasframe_t::name, pheader, and trivertx_t::v.
*/
{
	trivertx_t *pframe, *pinframe;
	int i, j;
	daliasframe_t *pdaliasframe;

	pdaliasframe = (daliasframe_t *) pin;

	strcpy(name, pdaliasframe->name);

	for (i = 0; i < 3; i++) {
		// these are byte values, so we don't have to worry about endianness
		pbboxmin->v[i] = pdaliasframe->bboxmin.v[i];
		pbboxmax->v[i] = pdaliasframe->bboxmax.v[i];
	}

	pinframe = (trivertx_t *) (pdaliasframe + 1);
	pframe = (trivertx_t *) malloc(numv * sizeof(*pframe));

	*pframeindex = (byte *) pframe - (byte *) pheader;

	for (j = 0; j < numv; j++) {
		int k;

		// these are all byte values, so no need to deal with endianness
		pframe[j].lightnormalindex = pinframe[j].lightnormalindex;

		for (k = 0; k < 3; k++)
			pframe[j].v[k] = pinframe[j].v[k];
	}

	pinframe += numv;
	return pinframe;
}

static void *Mod_LoadAliasGroup(void *pin, int *pframeindex, int numv,
				trivertx_t * pbboxmin, trivertx_t * pbboxmax,
				aliashdr_t * pheader, char *name)
/*
Definition at line 962 of file r_model.c.

References maliasgroupframedesc_s::bboxmax, daliasgroup_t::bboxmax, maliasgroupframedesc_s::bboxmin, daliasgroup_t::bboxmin, maliasgroupframedesc_s::frame, maliasgroup_s::frames, Host_Error(), Hunk_AllocName(), daliasinterval_t::interval, maliasgroup_s::intervals, loadname, Mod_LoadAliasFrame(), maliasgroup_s::numframes, daliasgroup_t::numframes, pheader, and trivertx_t::v.
*/
{
	daliasgroup_t *pingroup;
	maliasgroup_t *paliasgroup;
	int i, numframes;
	daliasinterval_t *pin_intervals;
	float *poutintervals;
	void *ptemp;
	printf("%s\n", __func__);

	pingroup = (daliasgroup_t *) pin;

	numframes = LittleLong(pingroup->numframes);

	paliasgroup = (maliasgroup_t *) malloc(sizeof(maliasgroup_t) +
					       (numframes -
						1) *
					       sizeof(paliasgroup->frames[0]));

	paliasgroup->numframes = numframes;

	for (i = 0; i < 3; i++) {
		// these are byte values, so we don't have to worry about endianness
		pbboxmin->v[i] = pingroup->bboxmin.v[i];
		pbboxmax->v[i] = pingroup->bboxmax.v[i];
	}

	*pframeindex = (byte *) paliasgroup - (byte *) pheader;

	pin_intervals = (daliasinterval_t *) (pingroup + 1);

	poutintervals = (float *)malloc(numframes * sizeof(float));

	paliasgroup->intervals = (byte *) poutintervals - (byte *) pheader;

	for (i = 0; i < numframes; i++) {
		*poutintervals = LittleFloat(pin_intervals->interval);
		if (*poutintervals <= 0.0)
			Host_Error("Mod_LoadAliasGroup: interval<=0");

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void *)pin_intervals;

	for (i = 0; i < numframes; i++) {
		ptemp = Mod_LoadAliasFrame(ptemp,
					   &paliasgroup->frames[i].frame,
					   numv,
					   &paliasgroup->frames[i].bboxmin,
					   &paliasgroup->frames[i].bboxmax,
					   pheader, name);
	}
	return ptemp;
}

static void Mod_LoadAliasModel(model_t * mod, void *buffer, int filesize)
/* No doxygen information */
{
	int i, j, version, numframes, numskins, size,
	    skinsize /*,start, end, total */ ;
	mdl_t *pmodel, *pinmodel;
	stvert_t *pstverts, *pinstverts;
	aliashdr_t *pheader;
	mtriangle_t *ptri;
	dtriangle_t *pintriangles;
	daliasframetype_t *pframetype;
	daliasskintype_t *pskintype;
	maliasskindesc_t *pskindesc;

	// some models are special
	if (!strcmp(mod->name, "progs/player.mdl"))
		mod->modhint = MOD_PLAYER;
	else if (!strcmp(mod->name, "progs/eyes.mdl"))
		mod->modhint = MOD_EYES;
	else if (!strcmp(mod->name, "progs/flame.mdl") ||
		 !strcmp(mod->name, "progs/flame2.mdl"))
		mod->modhint = MOD_FLAME;
	else if (!strcmp(mod->name, "progs/backpack.mdl"))
		mod->modhint = MOD_BACKPACK;
	else if (!strcmp(mod->name, "progs/e_spike1.mdl"))
		mod->modhint = MOD_RAIL;
	else if (!strcmp(mod->name, "progs/dgib.mdl") ||
		 !strcmp(mod->name, "progs/dgib2.mdl") ||
		 !strcmp(mod->name, "progs/dgib3.mdl") ||
		 !strcmp(mod->name, "progs/tesgib1.mdl") ||
		 !strcmp(mod->name, "progs/tesgib2.mdl") ||
		 !strcmp(mod->name, "progs/tesgib3.mdl") ||
		 !strcmp(mod->name, "progs/tesgib4.mdl") ||
		 !strcmp(mod->name, "progs/tgib1.mdl") ||
		 !strcmp(mod->name, "progs/tgib2.mdl") ||
		 !strcmp(mod->name, "progs/tgib3.mdl"))
		mod->modhint = MOD_BUILDINGGIBS;

/*
    if (mod->modhint == MOD_PLAYER || mod->modhint == MOD_EYES)
        mod->crc = CRC_Block (buffer, filesize);

    start = Hunk_LowMark ();
*/

	pinmodel = (mdl_t *) buffer;

	version = LittleLong(pinmodel->version);
	if (version != ALIAS_VERSION)
		Host_Error
		    ("Mod_LoadAliasModel: %s has wrong version number (%i should be %i)",
		     mod->name, version, ALIAS_VERSION);

	// allocate space for a working header, plus all the data except the frames, skin and group info
	size = sizeof(aliashdr_t) + (LittleLong(pinmodel->numframes) - 1) *
	    sizeof(pheader->frames[0]) +
	    sizeof(mdl_t) +
	    LittleLong(pinmodel->numverts) * sizeof(stvert_t) +
	    LittleLong(pinmodel->numtris) * sizeof(mtriangle_t);
	printf("%s: size = %d numframes = %d\n", __func__, size,
	       LittleLong(pinmodel->numframes));

	pheader = (aliashdr_t *) malloc(size);
	pmodel = (mdl_t *) ((byte *) & pheader[1] +
			    (LittleLong(pinmodel->numframes) - 1) *
			    sizeof(pheader->frames[0]));

	mod->cache.data = pheader;
	mod->flags = LittleLong(pinmodel->flags);

	// endian-adjust and copy the data, starting with the alias model header
	pmodel->boundingradius = LittleFloat(pinmodel->boundingradius);
	pmodel->numskins = LittleLong(pinmodel->numskins);
	pmodel->skinwidth = LittleLong(pinmodel->skinwidth);
	pmodel->skinheight = LittleLong(pinmodel->skinheight);

	if (pmodel->skinheight > MAX_LBM_HEIGHT)
		Host_Error
		    ("Mod_LoadAliasModel: model %s has a skin taller than %d",
		     mod->name, MAX_LBM_HEIGHT);

	pmodel->numverts = LittleLong(pinmodel->numverts);

	if (pmodel->numverts <= 0)
		Host_Error("Mod_LoadAliasModel: model %s has no vertices",
			   mod->name);

	if (pmodel->numverts > MAXALIASVERTS)
		Host_Error("Mod_LoadAliasModel: model %s has too many vertices",
			   mod->name);

	pmodel->numtris = LittleLong(pinmodel->numtris);

	if (pmodel->numtris <= 0)
		Host_Error("Mod_LoadAliasModel: model %s has no triangles",
			   mod->name);

	pmodel->numframes = LittleLong(pinmodel->numframes);
	pmodel->size = LittleFloat(pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->synctype = LittleLong(pinmodel->synctype);
	mod->numframes = pmodel->numframes;

	for (i = 0; i < 3; i++) {
		pmodel->scale[i] = LittleFloat(pinmodel->scale[i]);
		pmodel->scale_origin[i] =
		    LittleFloat(pinmodel->scale_origin[i]);
		pmodel->eyeposition[i] = LittleFloat(pinmodel->eyeposition[i]);
	}

	numskins = pmodel->numskins;
	numframes = pmodel->numframes;

	if (pmodel->skinwidth & 0x03)
		Host_Error("Mod_LoadAliasModel: skinwidth not multiple of 4");

	pheader->model = (byte *) pmodel - (byte *) pheader;

	// load the skins
	skinsize = pmodel->skinheight * pmodel->skinwidth;

	if (numskins < 1)
		Host_Error("Mod_LoadAliasModel: Invalid # of skins: %d\n",
			   numskins);

	pskintype = (daliasskintype_t *) & pinmodel[1];

	pskindesc =
	    (maliasskindesc_t *) malloc(numskins * sizeof(maliasskindesc_t));

	pheader->skindesc = (byte *) pskindesc - (byte *) pheader;

	for (i = 0; i < numskins; i++) {
		aliasskintype_t skintype;

		skintype = LittleLong(pskintype->type);
		pskindesc[i].type = skintype;

		if (skintype == ALIAS_SKIN_SINGLE)
			pskintype =
			    (daliasskintype_t *) Mod_LoadAliasSkin(pskintype +
								   1,
								   &pskindesc
								   [i].skin,
								   skinsize,
								   pheader);
		else
			pskintype = (daliasskintype_t *)
			    Mod_LoadAliasSkinGroup(pskintype + 1,
						   &pskindesc[i].skin, skinsize,
						   pheader);
	}

	// set base s and t vertices
	pstverts = (stvert_t *) & pmodel[1];
	pinstverts = (stvert_t *) pskintype;

	pheader->stverts = (byte *) pstverts - (byte *) pheader;

	for (i = 0; i < pmodel->numverts; i++) {
		pstverts[i].onseam = LittleLong(pinstverts[i].onseam);
		// put s and t in 16.16 format
		pstverts[i].s = LittleLong(pinstverts[i].s) << 16;
		pstverts[i].t = LittleLong(pinstverts[i].t) << 16;
	}

	// set up the triangles
	ptri = (mtriangle_t *) & pstverts[pmodel->numverts];
	pintriangles = (dtriangle_t *) & pinstverts[pmodel->numverts];

	pheader->triangles = (byte *) ptri - (byte *) pheader;

	for (i = 0; i < pmodel->numtris; i++) {
		ptri[i].facesfront = LittleLong(pintriangles[i].facesfront);

		for (j = 0; j < 3; j++)
			ptri[i].vertindex[j] =
			    LittleLong(pintriangles[i].vertindex[j]);
	}

	// load the frames
	if (numframes < 1)
		Host_Error("Mod_LoadAliasModel: Invalid # of frames: %d\n",
			   numframes);

	pframetype = (daliasframetype_t *) & pintriangles[pmodel->numtris];

	for (i = 0; i < numframes; i++) {
		aliasframetype_t frametype;

		frametype = LittleLong(pframetype->type);
		pheader->frames[i].type = frametype;

		if (frametype == ALIAS_SINGLE) {
			pframetype = (daliasframetype_t *)
			    Mod_LoadAliasFrame(pframetype + 1,
					       &pheader->frames[i].frame,
					       pmodel->numverts,
					       &pheader->frames[i].bboxmin,
					       &pheader->frames[i].bboxmax,
					       pheader,
					       pheader->frames[i].name);
		} else {
			pframetype = (daliasframetype_t *)
			    Mod_LoadAliasGroup(pframetype + 1,
					       &pheader->frames[i].frame,
					       pmodel->numverts,
					       &pheader->frames[i].bboxmin,
					       &pheader->frames[i].bboxmax,
					       pheader,
					       pheader->frames[i].name);
		}
	}

	mod->type = mod_alias;

	// FIXME: do this right
	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;
//    mod->cache.data = malloc(size + numskins * sizeof (maliasskindesc_t));
//    memcpy(mod->cache.data, pheader, size);
//    printf("%p\n", mod->cache.data);

	// move the complete, relocatable alias model to the cache
#if 0
	end = Hunk_LowMark();
	total = end - start;

	Cache_Alloc(&mod->cache, total, loadname);
	if (!mod->cache.data)
		return;
	memcpy(mod->cache.data, pheader, total);

	Hunk_FreeToLowMark(start);
#endif
}

//Loads a model into the cache
static model_t *Mod_LoadModel(model_t * mod, qbool crash)
/*
No doxygen info for this function
*/
{
	unsigned *buf;
	int filesize;

	if (!mod->needload) {
		return mod;	// not cached at all
	}
	// because the world is so huge, load it one piece at a time

	// load the file
	buf = (unsigned *)FS_LoadTempFile(mod->name, &filesize);
	if (!buf) {
		if (crash)
			Host_Error("Mod_LoadModel: %s not found", mod->name);
		return NULL;
	}
	/* FMod_CheckModel(mod->name, buf, filesize); */

	// call the apropriate loader
	mod->needload = false;
	mod->modhint = 0;

	switch (LittleLong(*(unsigned *)buf)) {
	case IDPOLYHEADER:
		Mod_LoadAliasModel(mod, buf, filesize);
		break;
#if 0
	case IDSPRITEHEADER:
		Mod_LoadSpriteModel(mod, buf);
		break;
	default:
		Mod_LoadBrushModel(mod, buf);
		break;
#endif
	default:
		return NULL;
	}

	return mod;
}

static void *Mod_Extradata(model_t * mod)
/*
Definition at line 54 of file r_model.c.

References model_s::cache, Cache_Check(), cache_user_s::data, Mod_LoadModel(), and Sys_Error().
*/
{
	Mod_LoadModel(mod, true);

	if (!mod->cache.data)
		Sys_Error("Mod_Extradata: caching failed");

	return mod->cache.data;
}

#define DEG2RAD(a) (((a) * M_PI) / 180.0F)

static void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
/*
Definition at line 148 of file mathlib.c.

References DEG2RAD, PITCH, ROLL, and YAW.

Referenced by CameraRandomPoint(), CameraUpdate(), CL_AddFlagModels(), CL_LinkPacketEntities(), CL_MuzzleFlash(), CL_PredictMove(), CL_UpdateBeams(), DrawMuzzleflash(), FireballTrailWave(), FuelRodGunTrail(), InfernoFire_f(), InitFlyby(), NQD_LinkEntities(), PF_makevectors(), PM_PlayerMove(), QMB_DetpackExplosion(), QMB_TeleportSplash(), R_AliasSetUpTransform(), R_DrawBrushModel(), R_DrawSprite(), R_DrawSpriteModel(), R_DrawTrySimpleItem(), R_SetupFrame(), TP_FindPoint(), V_AddViewWeapon(), V_CalcRefdef(), V_CalcRoll(), V_ParseDamage(), VX_DetpackExplosion(), VXGunshot(), VXNailhit(), and VXTeleport().
*/
{
	float angle, sr, sp, sy, cr, cp, cy, temp;

	if (angles[YAW]) {
		angle = DEG2RAD(angles[YAW]);
		sy = sin(angle);
		cy = cos(angle);
	} else {
		sy = 0;
		cy = 1;
	}

	if (angles[PITCH]) {
		angle = DEG2RAD(angles[PITCH]);
		sp = sin(angle);
		cp = cos(angle);
	} else {
		sp = 0;
		cp = 1;
	}

	if (forward) {
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}

	if (right || up) {

		if (angles[ROLL]) {
			angle = DEG2RAD(angles[ROLL]);
			sr = sin(angle);
			cr = cos(angle);

			if (right) {
				temp = sr * sp;
				right[0] = -1 * temp * cy + cr * sy;
				right[1] = -1 * temp * sy - cr * cy;
				right[2] = -1 * sr * cp;
			}

			if (up) {
				temp = cr * sp;
				up[0] = (temp * cy + sr * sy);
				up[1] = (temp * sy - sr * cy);
				up[2] = cr * cp;
			}
		} else {
			if (right) {
				right[0] = sy;
				right[1] = -cy;
				right[2] = 0;
			}

			if (up) {
				up[0] = sp * cy;
				up[1] = sp * sy;
				up[2] = cp;
			}
		}
	}
}

static void R_ConcatTransforms(float in1[3][4], float in2[3][4],
			       float out[3][4])
/*
Definition at line 279 of file mathlib.c.

Referenced by R_AliasSetUpTransform().
*/
{
	out[0][0] =
	    in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
	    in1[0][2] * in2[2][0];
	out[0][1] =
	    in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
	    in1[0][2] * in2[2][1];
	out[0][2] =
	    in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
	    in1[0][2] * in2[2][2];
	out[0][3] =
	    in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
	    in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] =
	    in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
	    in1[1][2] * in2[2][0];
	out[1][1] =
	    in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
	    in1[1][2] * in2[2][1];
	out[1][2] =
	    in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
	    in1[1][2] * in2[2][2];
	out[1][3] =
	    in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
	    in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] =
	    in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
	    in1[2][2] * in2[2][0];
	out[2][1] =
	    in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
	    in1[2][2] * in2[2][1];
	out[2][2] =
	    in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
	    in1[2][2] * in2[2][2];
	out[2][3] =
	    in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
	    in1[2][2] * in2[2][3] + in1[2][3];
}

static int dump_matrix(char *func, int l, char *prefix, char *m,
		       float matrix[3][4])
{
	int i, j;
	for (i = 0; i < 3; i++)
		for (j = 0; j < 4; j++)
			printf("%s:%d %s: %s[%d][%d] = %f\n", func, l, prefix,
			       m, i, j, matrix[i][j]);
	return 0;
}

#define DUMP_MATRIX(m) dump_matrix(__func__, __LINE__, "matrix", #m, m);

static void R_AliasSetUpTransform(entity_t * ent, int trivial_accept,
				  mdl_t * pmdl, vec3_t modelorg,
				  float aliasxscale, float aliasyscale,
				  float r_viewmodelsize, struct r_vects *rve)
/*
Definition at line 122 of file r_alias.c.

References ALIAS_BOTTOM_CLIP, ALIAS_LEFT_CLIP, ALIAS_RIGHT_CLIP, ALIAS_TOP_CLIP, ALIAS_XY_CLIP_MASK, ALIAS_Z_CLIP, finalvert_s::flags, entity_s::frame, auxvert_t::fv, fv, refdef_t::fvrectbottom, refdef_t::fvrectright, refdef_t::fvrectx, refdef_t::fvrecty, aedge_t::index0, max, min, entity_s::oldframe, R_AliasCheckBBoxFrame(), R_AliasSetUpTransform(), R_AliasTransformVector, r_aliastransition, r_framelerp, r_nearclip, r_refdef, r_resfudge, mdl_t::size, entity_s::trivial_accept, trivertx_t::v, cvar_s::value, xcenter, xscale, ycenter, and yscale.

*/
{
	int i;
	float scale, rotationmatrix[3][4], t2matrix[3][4];
	vec3_t angles;
	static float tmatrix[3][4], viewmatrix[3][4];

	// TODO: should really be stored with the entity instead of being reconstructed
	// TODO: should use a look-up table
	// TODO: could cache lazily, stored in the entity

	angles[ROLL] = ent->angles[ROLL];
	angles[PITCH] = -ent->angles[PITCH];
	angles[YAW] = ent->angles[YAW];
	AngleVectors(angles, rve->alias_forward, rve->alias_right,
		     rve->alias_up);

	tmatrix[0][0] = pmdl->scale[0];
	tmatrix[1][1] = pmdl->scale[1];
	tmatrix[2][2] = pmdl->scale[2];

	if ((ent->renderfx & RF_WEAPONMODEL)) {
		scale = 0.5 + bound(0, r_viewmodelsize, 1) / 2;
		tmatrix[0][0] *= scale;
	}

	tmatrix[0][3] = pmdl->scale_origin[0];
	tmatrix[1][3] = pmdl->scale_origin[1];
	tmatrix[2][3] = pmdl->scale_origin[2];

	// TODO: can do this with simple matrix rearrangement
	for (i = 0; i < 3; i++) {
		t2matrix[i][0] = rve->alias_forward[i];
		t2matrix[i][1] = -rve->alias_right[i];
		t2matrix[i][2] = rve->alias_up[i];
	}

	t2matrix[0][3] = -modelorg[0];
	t2matrix[1][3] = -modelorg[1];
	t2matrix[2][3] = -modelorg[2];

	// FIXME: can do more efficiently than full concatenation
	R_ConcatTransforms(t2matrix, tmatrix, rotationmatrix);
#if 0
	DUMP_MATRIX(rotationmatrix);
#endif

	// TODO: should be global, set when vright, etc., set
	VectorCopy(rve->vright, viewmatrix[0]);
	VectorNegate(rve->vup, viewmatrix[1]);
	VectorCopy(rve->vpn, viewmatrix[2]);
#if 0
	DUMP_MATRIX(viewmatrix);
#endif

	R_ConcatTransforms(viewmatrix, rotationmatrix, aliastransform);
#if 0
	DUMP_MATRIX(aliastransform);
#endif

	// do the scaling up of x and y to screen coordinates as part of the transform
	// for the unclipped case (it would mess up clipping in the clipped case).
	// Also scale down z, so 1/z is scaled 31 bits for free, and scale down x and y
	// correspondingly so the projected x and y come out right
	// FIXME: make this work for clipped case too?
	if (trivial_accept) {
		for (i = 0; i < 4; i++) {
			aliastransform[0][i] *=
			    aliasxscale * (1.0 / ((float)0x8000 * 0x10000));
			aliastransform[1][i] *=
			    aliasyscale * (1.0 / ((float)0x8000 * 0x10000));
			aliastransform[2][i] *= 1.0 / ((float)0x8000 * 0x10000);

		}
	}
}

static void R_AliasCheckBBoxFrame(int frame, trivertx_t ** mins,
				  trivertx_t ** maxs, aliashdr_t * paliashdr, double reftime)
/*
Definition at line 95 of file r_alias.c.

References ALIAS_SINGLE, maliasgroupframedesc_s::bboxmax, maliasframedesc_s::bboxmax, maliasgroupframedesc_s::bboxmin, maliasframedesc_s::bboxmin, maliasframedesc_s::frame, maliasgroup_s::frames, aliashdr_s::frames, int, maliasgroup_s::intervals, maliasgroup_s::numframes, r_refdef2, refdef2_t::time, and maliasframedesc_s::type.

Referenced by R_AliasCheckBBox().
*/
{
	int i, numframes;
	maliasgroup_t *paliasgroup;
	float *pintervals, fullinterval, targettime;

	if (paliashdr->frames[frame].type == ALIAS_SINGLE) {
		*mins = &paliashdr->frames[frame].bboxmin;
		*maxs = &paliashdr->frames[frame].bboxmax;
	} else {
		printf("%s: frame %d, frameidx = %d\n", __func__, frame,
		       paliashdr->frames[frame].frame);
		paliasgroup =
		    (maliasgroup_t *) ((byte *) paliashdr +
				       paliashdr->frames[frame].frame);
		pintervals =
		    (float *)((byte *) paliashdr + paliasgroup->intervals);
		numframes = paliasgroup->numframes;
		fullinterval = pintervals[numframes - 1];

		// when loading in Mod_LoadAliasGroup, we guaranteed all interval values
		// are positive, so we don't have to worry about division by 0
		targettime =
		    reftime -
		    ((int)(reftime / fullinterval)) * fullinterval;

		for (i = 0; i < numframes - 1; i++) {
			if (pintervals[i] > targettime)
				break;
		}
		*mins = &paliasgroup->frames[i].bboxmin;
		*maxs = &paliasgroup->frames[i].bboxmax;
	}
}

static qbool R_AliasCheckBBox(entity_t * ent, mdl_t * pmdl,
			      aliashdr_t * paliashdr, vec3_t modelorg,
			      float r_framelerp, struct r_scale *rs,
			      float r_viewmodelsize, float xscale, float yscale,
			      float xcenter, float ycenter,
			      float r_aliastransition, struct r_vects *rve,
			      float r_resfudge, double reftime)
/*
Definition at line 122 of file r_alias.c.

References ALIAS_BOTTOM_CLIP, ALIAS_LEFT_CLIP, ALIAS_RIGHT_CLIP, ALIAS_TOP_CLIP, ALIAS_XY_CLIP_MASK, ALIAS_Z_CLIP, finalvert_s::flags, entity_s::frame, auxvert_t::fv, fv, refdef_t::fvrectbottom, refdef_t::fvrectright, refdef_t::fvrectx, refdef_t::fvrecty, aedge_t::index0, max, min, entity_s::oldframe, R_AliasCheckBBoxFrame(), R_AliasSetUpTransform(), R_AliasTransformVector, r_aliastransition, r_framelerp, r_nearclip, r_refdef, r_resfudge, mdl_t::size, entity_s::trivial_accept, trivertx_t::v, cvar_s::value, xcenter, xscale, ycenter, and yscale.

Referenced by R_AliasDrawModel().
*/
{
	int i, flags, numv, minz;
	float zi, basepts[8][3], v0, v1, frac;
	finalvert_t *pv0, *pv1, viewpts[16];
	auxvert_t *pa0, *pa1, viewaux[16];
	qbool zclipped, zfullyclipped;
	unsigned anyclip, allclip;
	trivertx_t *mins, *maxs, *oldmins, *oldmaxs;
#if 0
	int j, k;
	for (j = 0; j < 3; j++)
		for (k = 0; k < 4; k++)
			printf("%s:%d aliastransform[%d][%d] = %f\n", __func__,
			       __LINE__, j, k, aliastransform[j][k]);
#endif

	ent->trivial_accept = 0;

	// expand, rotate, and translate points into worldspace
	R_AliasSetUpTransform(ent, 0, pmdl, modelorg, rs->aliasxscale,
			      rs->aliasyscale, r_viewmodelsize, rve);
#if 0
	for (j = 0; j < 3; j++)
		for (k = 0; k < 4; k++)
			printf("%s:%d aliastransform[%d][%d] = %f\n", __func__,
			       __LINE__, j, k, aliastransform[j][k]);

	printf("%s:%d\n", __func__, __LINE__);
#endif
	// construct the base bounding box for this frame
	if (r_framelerp == 1) {
		R_AliasCheckBBoxFrame(ent->frame, &mins, &maxs, paliashdr, reftime);

		// x worldspace coordinates
		basepts[0][0] = basepts[1][0] = basepts[2][0] = basepts[3][0] =
		    (float)mins->v[0];
		basepts[4][0] = basepts[5][0] = basepts[6][0] = basepts[7][0] =
		    (float)maxs->v[0];
		// y worldspace coordinates
		basepts[0][1] = basepts[3][1] = basepts[5][1] = basepts[6][1] =
		    (float)mins->v[1];
		basepts[1][1] = basepts[2][1] = basepts[4][1] = basepts[7][1] =
		    (float)maxs->v[1];
		// z worldspace coordinates
		basepts[0][2] = basepts[1][2] = basepts[4][2] = basepts[5][2] =
		    (float)mins->v[2];
		basepts[2][2] = basepts[3][2] = basepts[6][2] = basepts[7][2] =
		    (float)maxs->v[2];
	} else {
		R_AliasCheckBBoxFrame(ent->oldframe, &oldmins, &oldmaxs,
				      paliashdr, reftime);
		R_AliasCheckBBoxFrame(ent->frame, &mins, &maxs, paliashdr, reftime);

		// x worldspace coordinates
		basepts[0][0] = basepts[1][0] = basepts[2][0] = basepts[3][0] =
		    (float)min(mins->v[0], oldmins->v[0]);
		basepts[4][0] = basepts[5][0] = basepts[6][0] = basepts[7][0] =
		    (float)max(maxs->v[0], oldmaxs->v[0]);
		// y worldspace coordinates
		basepts[0][1] = basepts[3][1] = basepts[5][1] = basepts[6][1] =
		    (float)min(mins->v[1], oldmins->v[1]);
		basepts[1][1] = basepts[2][1] = basepts[4][1] = basepts[7][1] =
		    (float)max(maxs->v[1], oldmaxs->v[1]);
		// z worldspace coordinates
		basepts[0][2] = basepts[1][2] = basepts[4][2] = basepts[5][2] =
		    (float)min(mins->v[2], oldmins->v[2]);
		basepts[2][2] = basepts[3][2] = basepts[6][2] = basepts[7][2] =
		    (float)max(maxs->v[2], oldmaxs->v[2]);
	}

	zclipped = false;
	zfullyclipped = true;

	minz = 9999;
	for (i = 0; i < 8; i++) {
		R_AliasTransformVector(&basepts[i][0], &viewaux[i].fv[0]);
#if 0
		printf("%s:%d viewaux[i].fv[0] = %f\n", __func__, __LINE__,
		       viewaux[i].fv[0]);
		printf("%s:%d viewaux[i].fv[1] = %f\n", __func__, __LINE__,
		       viewaux[i].fv[1]);
		printf("%s:%d viewaux[i].fv[2] = %f\n", __func__, __LINE__,
		       viewaux[i].fv[2]);
		printf("%s:%d basepts[i][0] = %f\n", __func__, __LINE__,
		       basepts[i][0]);
		printf("%s:%d basepts[i][1] = %f\n", __func__, __LINE__,
		       basepts[i][1]);
		printf("%s:%d basepts[i][2] = %f\n", __func__, __LINE__,
		       basepts[i][2]);
#endif

		if (viewaux[i].fv[2] < rs->r_nearclip) {
			// we must clip points that are closer than the near clip plane
			viewpts[i].flags = ALIAS_Z_CLIP;
			zclipped = true;
#if 0
			printf("%s:%d viewaux[i].fv[0] = %f\n", __func__,
			       __LINE__, viewaux[i].fv[0]);
			printf("%s:%d viewaux[i].fv[1] = %f\n", __func__,
			       __LINE__, viewaux[i].fv[1]);
			printf("%s:%d viewaux[i].fv[2] = %f\n", __func__,
			       __LINE__, viewaux[i].fv[2]);
#endif
		} else {
			if (viewaux[i].fv[2] < minz)
				minz = viewaux[i].fv[2];
			viewpts[i].flags = 0;
			zfullyclipped = false;
		}
	}
	if (zfullyclipped)
		return false;	// everything was near-z-clipped

	numv = 8;

	if (zclipped) {
		// organize points by edges, use edges to get new points (possible trivial reject)
		for (i = 0; i < 12; i++) {
			// edge endpoints
			pv0 = &viewpts[aedges[i].index0];
			pv1 = &viewpts[aedges[i].index1];
			pa0 = &viewaux[aedges[i].index0];
			pa1 = &viewaux[aedges[i].index1];

			// if one end is clipped and the other isn't, make a new point
			if (pv0->flags ^ pv1->flags) {
				frac =
				    (rs->r_nearclip -
				     pa0->fv[2]) / (pa1->fv[2] - pa0->fv[2]);
				viewaux[numv].fv[0] =
				    pa0->fv[0] + (pa1->fv[0] -
						  pa0->fv[0]) * frac;
				viewaux[numv].fv[1] =
				    pa0->fv[1] + (pa1->fv[1] -
						  pa0->fv[1]) * frac;
				viewaux[numv].fv[2] = rs->r_nearclip;
				viewpts[numv].flags = 0;
				numv++;
			}
		}
	}
	// project the vertices that remain after clipping
	anyclip = 0;
	allclip = ALIAS_XY_CLIP_MASK;

	// TODO: probably should do this loop in ASM, especially if we use floats
	for (i = 0; i < numv; i++) {
		// we don't need to bother with vertices that were z-clipped
		if (viewpts[i].flags & ALIAS_Z_CLIP)
			continue;

		zi = 1.0 / viewaux[i].fv[2];

		// FIXME: do with chop mode in ASM, or convert to float
		v0 = (viewaux[i].fv[0] * xscale * zi) + xcenter;
		v1 = (viewaux[i].fv[1] * yscale * zi) + ycenter;

		flags = 0;

		if (v0 < r_refdef.fvrectx)
			flags |= ALIAS_LEFT_CLIP;
		if (v1 < r_refdef.fvrecty)
			flags |= ALIAS_TOP_CLIP;
		if (v0 > r_refdef.fvrectright)
			flags |= ALIAS_RIGHT_CLIP;
		if (v1 > r_refdef.fvrectbottom)
			flags |= ALIAS_BOTTOM_CLIP;

		anyclip |= flags;
		allclip &= flags;
	}

	if (allclip)
		return false;	// trivial reject off one side

	ent->trivial_accept = !anyclip & !zclipped;

#if 0
	printf("%s:%d ent->trivial_accept = %d\n", __func__, __LINE__,
	       ent->trivial_accept);
#endif

	if (ent->trivial_accept) {
		if (minz > (r_aliastransition + (pmdl->size * r_resfudge)))
			ent->trivial_accept |= 2;
	}
#if 0
	printf("%s:%d\n", __func__, __LINE__);
#endif

	return true;
}

void write_png_file(char *file_name, int width,
		    int height, const unsigned char *buffer);

static void R_AliasSetupSkin(entity_t * ent, mdl_t * pmdl,
			     aliashdr_t * paliashdr, struct r_finalmesh *fm, double reftime)
/*
Definition at line 484 of file r_alias.c.

References a_skinwidth, ALIAS_SKIN_GROUP, Com_DPrintf(), int, maliasskingroup_s::intervals, maliasskingroup_s::numskins, mdl_t::numskins, numskins, affinetridesc_t::pskin, pskindesc, affinetridesc_t::pskindesc, r_refdef2, entity_s::scoreboard, affinetridesc_t::seamfixupX16, player_info_s::skin, maliasskindesc_t::skin, Skin_Cache(), Skin_Find(), aliashdr_s::skindesc, maliasskingroup_s::skindescs, mdl_t::skinheight, affinetridesc_t::skinheight, entity_s::skinnum, affinetridesc_t::skinwidth, mdl_t::skinwidth, refdef2_t::time, and maliasskindesc_t::type.

Referenced by R_AliasDrawModel().
*/
{
	/* byte *base; */
	int skinnum, i, numskins;
	maliasskingroup_t *paliasskingroup;
	maliasskindesc_t *pskindesc;
	float *pskinintervals, fullskininterval, skintargettime, skintime;
	int a_skinwidth;

	skinnum = ent->skinnum;
	if (skinnum >= pmdl->numskins || skinnum < 0) {
		Com_DPrintf("R_AliasSetupSkin: no such skin # %d\n", skinnum);
		skinnum = 0;
	}

	pskindesc =
	    ((maliasskindesc_t *) ((byte *) paliashdr + paliashdr->skindesc)) +
	    skinnum;
	a_skinwidth = pmdl->skinwidth;

	if (pskindesc->type == ALIAS_SKIN_GROUP) {
		paliasskingroup =
		    (maliasskingroup_t *) ((byte *) paliashdr +
					   pskindesc->skin);
		pskinintervals =
		    (float *)((byte *) paliashdr + paliasskingroup->intervals);
		numskins = paliasskingroup->numskins;
		fullskininterval = pskinintervals[numskins - 1];

		skintime = reftime;

		// when loading in Mod_LoadAliasSkinGroup, we guaranteed all interval
		// values are positive, so we don't have to worry about division by 0
		skintargettime =
		    skintime -
		    ((int)(skintime / fullskininterval)) * fullskininterval;

		for (i = 0; i < numskins - 1; i++) {
			if (pskinintervals[i] > skintargettime)
				break;
		}

		pskindesc = &paliasskingroup->skindescs[i];
	}

	fm->pskin = (void *)((byte *) paliashdr + pskindesc->skin);
	fm->seamfixupX16 = (a_skinwidth >> 1) << 16;
	write_png_file("skin.png", pmdl->skinwidth,
		       pmdl->skinheight, (void *)((byte *) paliashdr + pskindesc->skin));

}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/
static int RecursiveLightPoint(mnode_t * node, vec3_t start, vec3_t end,
			       model_t * worldmodel)
/*
Definition at line 175 of file r_light.c.

References mnode_s::children, cl, mnode_s::contents, d_lightstylevalue, mplane_s::dist, DotProduct, msurface_s::extents, mnode_s::firstsurface, msurface_s::flags, MAXLIGHTMAPS, mplane_s::normal, mnode_s::numsurfaces, mnode_s::plane, RecursiveLightPoint(), s, msurface_s::samples, msurface_s::styles, SURF_DRAWTILED, model_s::surfaces, t, msurface_s::texinfo, msurface_s::texturemins, mplane_s::type, mtexinfo_s::vecs, and clientState_t::worldmodel.
*/
{
	int r;
	float front, back, frac;
	int side;
	mplane_t *plane;
	vec3_t mid;
	msurface_t *surf;
	int s, t, ds, dt;
	int i;
	mtexinfo_t *tex;
	byte *lightmap;
	unsigned scale;
	int maps;

	if (node->contents < 0)
		return -1;	// didn't hit anything

// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	if (plane->type < 3) {
		front = start[plane->type] - plane->dist;
		back = end[plane->type] - plane->dist;
	} else {
		front = DotProduct(start, plane->normal) - plane->dist;
		back = DotProduct(end, plane->normal) - plane->dist;
	}
	side = front < 0;

	if ((back < 0) == side)
		return RecursiveLightPoint(node->children[side], start, end,
					   worldmodel);

	frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;

// go down front side   
	r = RecursiveLightPoint(node->children[side], start, mid, worldmodel);
	if (r >= 0)
		return r;	// hit something

	if ((back < 0) == side)
		return -1;	// didn't hit anuthing

// check for impact on this node

	surf = worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++) {
		if (surf->flags & SURF_DRAWTILED)
			continue;	// no lightmaps

		tex = surf->texinfo;

		s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];;

		if (s < surf->texturemins[0] || t < surf->texturemins[1])
			continue;

		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];

		if (ds > surf->extents[0] || dt > surf->extents[1])
			continue;

		if (!surf->samples)
			return 0;

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->samples;
		r = 0;
		if (lightmap) {

			lightmap += dt * ((surf->extents[0] >> 4) + 1) + ds;

			for (maps = 0;
			     maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
			     maps++) {
				scale = d_lightstylevalue[surf->styles[maps]];
				r += *lightmap * scale;
				lightmap += ((surf->extents[0] >> 4) + 1) *
				    ((surf->extents[1] >> 4) + 1);
			}

			r >>= 8;
		}

		return r;
	}

// go down back side
	return RecursiveLightPoint(node->children[!side], mid, end, worldmodel);
}

static int R_LightPoint(vec3_t p, model_t * worldmodel)
/*
Definition at line 277 of file r_light.c.

References refdef_t::ambientlight, cl, full_light, lightcolor, model_s::lightdata, model_s::nodes, R_FullBrightAllowed(), r_refdef, r_shadows, RecursiveLightPoint(), cvar_s::value, and clientState_t::worldmodel.

Referenced by R_AliasSetupLighting().
*/
{
	vec3_t end;
	int r;

	if (!worldmodel->lightdata)
		return 255;

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 8192;

	r = RecursiveLightPoint(worldmodel->nodes, p, end, worldmodel);

	if (r == -1)
		r = 0;

	if (r < r_refdef.ambientlight)
		r = r_refdef.ambientlight;

	return r;
}

static vec_t VectorLength(vec3_t v)
/*
Definition at line 248 of file mathlib.c.

Referenced by AddParticleTrail(), Cam_Track(), Cam_TryFlyby(), Classic_ParticleTrail(), EmitSkyPolys(), EmitSkyVert(), Mod_LoadTexinfo(), PM_Friction(), PM_SpectatorMove(), QMB_UpdateParticles(), R_AliasSetupLighting(), R_DrawCoronas(), RadiusFromBounds(), SCR_DrawSpeed(), SV_CheckVelocity(), SV_Multicast(), SV_Physics_Pusher(), TP_RankPoint(), and VX_TeslaCharge().

*/
{
	float length;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	return sqrt(length);
}

static void R_AliasSetupLighting(entity_t * ent, int *r_ambientlight,
				 float r_fullbrightSkins, struct r_vects *rve,
				 float *r_shadelight, model_t * worldmodel, double reftime, float max_fbskins)
/*
Definition at line 537 of file r_alias.c.

References alias_forward, alias_right, alias_up, ambientlight, bound, cl, cl_dlights, currententity, LIGHT_MIN, max, MAX_DLIGHTS, refdef2_t::max_fbskins, clientState_t::minlight, MOD_PLAYER, entity_s::model, model_s::modhint, dlight_t::origin, entity_s::origin, r_ambientlight, r_fullbrightSkins, R_LightPoint(), r_plightvec, r_refdef2, r_shadelight, dlight_t::radius, entity_s::renderfx, RF_PLAYERMODEL, RF_WEAPONMODEL, shadelight, refdef2_t::time, cvar_s::value, VectorLength(), VectorSet, VectorSubtract, VID_CBITS, and VID_GRADES.

Referenced by R_AliasDrawModel(), R_DrawAlias3Model(), and R_DrawAliasModel().
*/
{
	int lnum, minlight, ambientlight, shadelight;
	float add, fbskins;
	vec3_t dist;

	ambientlight = shadelight = R_LightPoint(ent->origin, worldmodel);

	for (lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
		if (cl_dlights[lnum].die < reftime
		    || !cl_dlights[lnum].radius)
			continue;

		VectorSubtract(ent->origin, cl_dlights[lnum].origin, dist);
		add = cl_dlights[lnum].radius - VectorLength(dist);

		if (add > 0)
			ambientlight += add;
	}

	// clamp lighting so it doesn't overbright as much
	if (ambientlight > 128)
		ambientlight = 128;
	if (ambientlight + shadelight > 192)
		shadelight = 192 - ambientlight;

	// always give the gun some light
	if ((ent->renderfx & RF_WEAPONMODEL) && ambientlight < 24)
		ambientlight = shadelight = 24;

	// never allow players to go totally black
	if (ent->model->modhint == MOD_PLAYER || ent->renderfx & RF_PLAYERMODEL) {
		if (ambientlight < 8)
			ambientlight = shadelight = 8;
	}

	if (ent->model->modhint == MOD_PLAYER || ent->renderfx & RF_PLAYERMODEL) {
		fbskins = bound(0, r_fullbrightSkins, max_fbskins);
		if (fbskins) {
			ambientlight = max(ambientlight, 8 + fbskins * 120);
			shadelight = max(shadelight, 8 + fbskins * 120);
		}
	}

	minlight = cl_minlight;

	if (ambientlight < minlight)
		ambientlight = shadelight = minlight;

	// guarantee that no vertex will ever be lit below LIGHT_MIN, so we don't have to clamp off the bottom
	*r_ambientlight = max(ambientlight, LIGHT_MIN);
	*r_ambientlight = (255 - *r_ambientlight) << VID_CBITS;
	*r_ambientlight = max(*r_ambientlight, LIGHT_MIN);
	*r_shadelight = max(shadelight, 0);
	*r_shadelight *= VID_GRADES;

	// rotate the lighting vector into the model's frame of reference
	VectorSet(rve->r_plightvec, -rve->alias_forward[0], rve->alias_right[0],
		  -rve->alias_up[0]);
}

static void R_AliasSetupFrameVerts(int frame, trivertx_t ** verts,
				   aliashdr_t * paliashdr, double reftime)
{
	int i, numframes;
	maliasgroup_t *paliasgroup;
	float *pintervals, fullinterval, targettime;

	if (paliashdr->frames[frame].type == ALIAS_SINGLE) {
		*verts =
		    (trivertx_t *) ((byte *) paliashdr +
				    paliashdr->frames[frame].frame);
	} else {
		paliasgroup =
		    (maliasgroup_t *) ((byte *) paliashdr +
				       paliashdr->frames[frame].frame);
		pintervals =
		    (float *)((byte *) paliashdr + paliasgroup->intervals);
		numframes = paliasgroup->numframes;
		fullinterval = pintervals[numframes - 1];

		// when loading in Mod_LoadAliasGroup, we guaranteed all interval values
		// are positive, so we don't have to worry about division by 0
		targettime =
		    reftime -
		    ((int)(reftime / fullinterval)) * fullinterval;

		for (i = 0; i < numframes - 1; i++) {
			if (pintervals[i] > targettime)
				break;
		}
		*verts =
		    (trivertx_t *) ((byte *) paliashdr +
				    paliasgroup->frames[i].frame);
	}
}

//set r_oldapverts, r_apverts
static void R_AliasSetupFrame(entity_t * ent, aliashdr_t * paliashdr,
			      struct r_verts *rv, double reftime)
{
	R_AliasSetupFrameVerts(ent->oldframe, &rv->r_oldapverts, paliashdr, reftime);
	R_AliasSetupFrameVerts(ent->frame, &rv->r_apverts, paliashdr, reftime);
}

static void D_PolysetUpdateTables(byte **skintable, void *pskin, int skinwidth)
{
	int i;
	byte *s;

	s = pskin;
	for (i = 0; i < MAX_LBM_HEIGHT; i++, s += skinwidth)
		skintable[i] = s;
}

void set_rs(struct r_scale *rs,
	float aliasxscale,
	float aliasyscale,
	float aliasxcenter,
	float aliasycenter,
	float nearclip)
{
	rs->aliasxscale = aliasxscale;
	rs->aliasyscale = aliasyscale;
	rs->aliasxcenter = aliasxcenter;
	rs->aliasycenter = aliasycenter;
	rs->r_nearclip = nearclip;
}

void set_rve(struct r_vects *rve, vec3_t angles)
{
	AngleVectors(angles, rve->vpn, rve->vright, rve->vup);
}


void set_view(struct r_view *view, pixel_t *viewbuffer, short *zbuffer,
		unsigned int zwidth,
		int screenwidth)
{
	view->viewbuffer = viewbuffer;
	view->zbuffer = zbuffer;
	view->zwidth = zwidth;
	view->screenwidth = screenwidth;
}

struct main_scale {
	float xscale, yscale;
	float xcenter, ycenter;
};

void set_main_scale(struct main_scale *ms, float xscale,
		float yscale, float xcenter, float ycenter)
{
	ms->xscale = xscale;
	ms->yscale = yscale;
	ms->xcenter = xcenter;
	ms->ycenter = ycenter;
}

void R_AliasDrawModel(entity_t * ent, const struct r_view *view,
			     vec3_t r_origin,
			     int r_lerpframes, float r_lerpdistance,
			     struct r_scale *rs,
			     float r_viewmodelsize,
			     struct main_scale *ms,
			     float r_aliastransition,
			     model_t * worldmodel, struct r_vects *rve,
			     float r_resfudge,
			     byte **skintable)
/*
Definition at line 629 of file r_alias.c.

References acolormap, CACHE_SIZE, entity_s::colormap, Com_DPrintf(), D_Aff8Patch(), D_PolysetUpdateTables(), affinetridesc_t::drawtype, entity_s::frame, entity_s::framelerp, MAXALIASVERTS, min, Mod_Extradata(), aliashdr_s::model, entity_s::model, modelorg, mdl_t::numframes, entity_s::oldframe, entity_s::origin, R_AliasCheckBBox(), R_AliasPreparePoints(), R_AliasPrepareUnclippedPoints(), R_AliasSetupFrame(), R_AliasSetupLighting(), R_AliasSetupSkin(), R_AliasSetUpTransform(), r_amodels_drawn, r_entorigin, r_framelerp, r_origin, entity_s::renderfx, RF_WEAPONMODEL, Sys_Error(), entity_s::trivial_accept, cvar_s::value, VectorCopy, VectorSubtract, and ziscale.

Referenced by R_DrawEntitiesOnList(), and R_DrawViewModel().
*/
{
	mdl_t *pmdl;
	aliashdr_t *paliashdr;
	auxvert_t *pauxverts;
	struct r_verts rv;
	finalvert_t *pfinalverts;
	vec3_t modelorg;
	model_t *pmodel;
	float r_framelerp;
	float ziscale;
	int r_ambientlight;
	float r_shadelight;
	struct r_finalmesh fm;
	int skinwidth;
	int drawtype;
	double reftime = 0.0;

	finalvert_t finalverts[MAXALIASVERTS +
			       ((CACHE_SIZE - 1) / sizeof(finalvert_t)) + 1];
	auxvert_t auxverts[MAXALIASVERTS];

	VectorSubtract(r_origin, ent->origin, modelorg);

	pmodel = ent->model;
	paliashdr = pmodel->cache.data;
	if (!paliashdr)
		return;
	pmdl = (mdl_t *) ((byte *) paliashdr + paliashdr->model);

	if (ent->frame >= pmdl->numframes || ent->frame < 0) {
		Com_DPrintf("R_AliasDrawModel: no such frame %d\n", ent->frame);
		ent->frame = 0;
	}
	if (ent->oldframe >= pmdl->numframes || ent->oldframe < 0) {
		Com_DPrintf("R_AliasDrawModel: no such oldframe %d\n",
			    ent->oldframe);
		ent->oldframe = 0;
	}

	if (!r_lerpframes || ent->framelerp < 0 || ent->oldframe == ent->frame)
		r_framelerp = 1.0;
	else
		r_framelerp = min(ent->framelerp, 1);

	if (!(ent->renderfx & RF_WEAPONMODEL)) {
		if (!R_AliasCheckBBox(ent, pmdl, paliashdr, modelorg,
				      r_framelerp, rs, r_viewmodelsize, ms->xscale,
				      ms->yscale, ms->xcenter, ms->ycenter,
				      r_aliastransition, rve, r_resfudge, reftime))
			return;
	}
	// cache align
	pfinalverts =
	    (finalvert_t *) (((long)&finalverts[0] + CACHE_SIZE - 1) &
			     ~(CACHE_SIZE - 1));
	pauxverts = &auxverts[0];

	R_AliasSetupSkin(ent, pmdl, paliashdr, &fm, reftime);
	R_AliasSetUpTransform(ent, ent->trivial_accept, pmdl, modelorg,
			      rs->aliasxscale, rs->aliasyscale, r_viewmodelsize,
			      rve);
	R_AliasSetupLighting(ent, &r_ambientlight, 0.0, rve, &r_shadelight,
			     worldmodel, reftime, 0.0);
	R_AliasSetupFrame(ent, paliashdr, &rv, reftime);

	if (!ent->colormap)
		Sys_Error("R_AliasDrawModel: !ent->colormap");

	drawtype = (ent->trivial_accept == 3);

	skinwidth = pmdl->skinwidth;
	if (drawtype)
		D_PolysetUpdateTables(skintable, fm.pskin, skinwidth);	// FIXME: precalc...
#ifdef id386
	else
		D_Aff8Patch(ent->colormap);
#endif

	if (!(ent->renderfx & RF_WEAPONMODEL))
		ziscale = (float)0x8000 *(float)0x10000;
	else
		ziscale = (float)0x8000 *(float)0x10000 *3.0;

	rs->ziscale = ziscale;

	if (ent->trivial_accept) {
		R_AliasPrepareUnclippedPoints(view,
					      ent->colormap,
					      skinwidth, pmdl,
					      paliashdr, &rv, pfinalverts,
					      r_framelerp, r_ambientlight, rs,
					      rve, r_shadelight, skintable, &fm, drawtype);

		D_PolysetDraw(view, ent->colormap,
		      		skintable, skinwidth, &fm, drawtype);
	} else {
		R_AliasPreparePoints(ent, view,
				     ent->colormap,
				     skinwidth, pmdl,
				     paliashdr, pauxverts, &rv, pfinalverts,
				     r_framelerp, ziscale, r_lerpdistance,
				     r_ambientlight, rs, rve, r_shadelight, skintable, &fm, drawtype);
	}
}

static void load_model(int type, model_t *model, char *filename)
{
	int fd, l;
	struct stat buf;
	unsigned char *data;
	int ret = stat(filename, &buf);
	if (ret < 0)
		return;
	data = malloc(buf.st_size);
	fd = open(filename, O_RDONLY);
	l = read(fd, data, buf.st_size);
	close(fd);
	if (l != buf.st_size) {
		free(data);
		return;
	}
	strcpy(model->name, filename);
	Mod_LoadAliasModel(model, data, buf.st_size);
}

static void write_xbm(char *name, int width, int height, unsigned char *pixdata)
{
	int fd, i;
	char *buf = malloc(1024);
	memset(buf, 0, 1024);
	strcpy(buf, name);
	strcat(buf, ".xpm");
	fd = open(buf, O_RDWR | O_CREAT | O_TRUNC, 0644);
	strcpy(buf, "#define ");
	strcat(buf, name);
	strcat(buf, "_width ");
	write(fd, buf, strlen(buf));
	snprintf(buf, 1024, "%d\n", width);
	write(fd, buf, strlen(buf));
	strcpy(buf, "#define ");
	strcat(buf, name);
	strcat(buf, "_height ");
	write(fd, buf, strlen(buf));
	snprintf(buf, 1024, "%d\n", height);
	write(fd, buf, strlen(buf));
	strcpy(buf, "#define ");
	strcat(buf, name);
	strcat(buf, "_bits = {");
	write(fd, buf, strlen(buf));
	for (i = 0; i < width * height; i++) {
		snprintf(buf, 1024, "%d, ", pixdata[i]);
		write(fd, buf, strlen(buf));
	}
	strcpy(buf, "\n}\n");
	write(fd, buf, strlen(buf));
	close(fd);
	free(buf);
}

void updatescr(unsigned char *ptr);
static void loopfunc(void *data)
{
	int i;
	model_t worldmodel;
	struct r_scale rs;
	struct r_vects rve;
	struct r_view view;
	struct main_scale ms;
	struct control_data *cd = data;
	entity_t *ent = cd->data;
	pixel_t *d_viewbuffer = cd->view;
	short *d_pzbuffer = cd->zbuffer;
	byte *skintable[MAX_LBM_HEIGHT];

	memset(d_pzbuffer, 0, HEIGHT * WIDTH * 2);
	memset(d_viewbuffer, 0, HEIGHT * WIDTH);
	memset(&worldmodel, 0, sizeof(worldmodel));
	set_rve(&rve, r_refdef.viewangles);
	set_rs(&rs, cd->aliasxscale,
		cd->aliasyscale,
		cd->aliasxcenter,
		cd->aliasycenter,
		1.0);
	set_main_scale(&ms, cd->xscale, cd->yscale, cd->xcenter, cd->ycenter);
	set_view(&view, d_viewbuffer, d_pzbuffer, ZWIDTH, WIDTH);
	for (i = 0; i < HEIGHT; i++) {
		view.d_scantable[i] = i * WIDTH;
		view.zspantable[i] = d_pzbuffer + i * ZWIDTH;
	}
	R_AliasDrawModel(ent, &view,
			 cd->origin, 1,	/* r_lerpframes */
			 1.0,	/* r_lerpdistance */
			 &rs,
			 0.0, &ms, 
			 cd->r_aliastransition, &worldmodel, &rve,
			 cd->r_resfudge, skintable);
	updatescr(d_viewbuffer);
//      ent->frame = ent->frame ^ 1;
//      ent->oldframe = ent->frame;
}

int main(int argc, char *argv[])
{
	byte *pb;
	entity_t ent;
	struct control_data *cd;
	int i, j, fd, opt;
	model_t m;
	char fname[PATH_MAX];
	pixel_t *d_viewbuffer;
	short *d_pzbuffer;
	float res_scale;
	vec3_t r_origin;
	void initgfx(void);
	void do_key_loop(void (*loopfunc) (void *data), void *data);
	void stopgfx(void);

	initgfx();

	strcpy(m.name, "test");
	m.needload = true;
	m.cache.data = NULL;
	ent.model = &m;
	ent.origin[0] = -2.5;
	ent.origin[1] = 1.5;
	ent.origin[2] = -2.;
	ent.frame = 0;
	ent.oldframe = 0;
	ent.framelerp = 0;
	ent.renderfx = 0;
	ent.angles[ROLL] = 0.0;
	ent.angles[YAW] = 0.0;
	ent.angles[PITCH] = 0.0;
	ent.skinnum = 0;
	ent.colormap = malloc(sizeof(pixel_t) * 256 * 256);
	for (i = 0; i < 256; i++)
		for (j = 0; j < 256; j++)
			ent.colormap[256 * i + j] = j;
	d_pzbuffer = malloc(HEIGHT * WIDTH * 4);
	d_viewbuffer = malloc(HEIGHT * WIDTH);
	r_refdef.aliasvrect.x = 0;
	r_refdef.aliasvrect.y = 0;
	r_refdef.aliasvrectbottom = HEIGHT;
	r_refdef.aliasvrectright = WIDTH;
	r_refdef.fvrectx = (float)r_refdef.aliasvrect.x;
	r_refdef.fvrecty = (float)r_refdef.aliasvrect.y;
	r_refdef.fvrectright = (float)r_refdef.aliasvrectright;
	r_refdef.fvrectbottom = (float)r_refdef.aliasvrectbottom;
	opt = 0;
	strcpy(fname, "viewbuf.png");
	while (1) {
		opt = getopt(argc, argv, "f:");
		if (opt == -1)
			break;
		switch (opt) {
		case 'f':
			strncpy(fname, optarg, sizeof(fname));
			break;
		}
	}
	argc -= optind;
	argv += optind;
	ent.origin[0] = 0.0;
	ent.origin[1] = 5;
	ent.origin[2] = 5.0;
	cd = malloc(sizeof(struct control_data));
	memset(cd, 0, sizeof(struct control_data));
	cd->data = &ent;
	cd->origin = r_origin;
	cd->view = d_viewbuffer;
	cd->zbuffer = d_pzbuffer;
	cd->yaw = &r_refdef.viewangles[YAW];
	cd->pitch = &r_refdef.viewangles[PITCH];
	cd->roll = &r_refdef.viewangles[ROLL];
	cd->aliasxcenter =
	    r_refdef.aliasvrect.x + r_refdef.aliasvrectbottom / 2;
	cd->aliasycenter =
	    r_refdef.aliasvrect.y + r_refdef.aliasvrectbottom / 2;
	cd->xcenter = cd->aliasxcenter;
	cd->ycenter = cd->aliasycenter;
	cd->xscale = WIDTH / 2.0;
	cd->yscale =
	    cd->xscale * ((float)HEIGHT / (float)WIDTH) * (320.0 / 240.0);
	cd->aliasxscale = cd->xscale;
	cd->aliasyscale = cd->yscale;
	for (i = 0;
	     i < sizeof(d_lightstylevalue) / sizeof(d_lightstylevalue[0]); i++)
		d_lightstylevalue[i] = 256;
	res_scale = sqrt((double)(WIDTH * HEIGHT) / (320.0 * 152.0));
#define ALIASTRANSBASE	200
#define ALIASTRANSADJ	100
	cd->r_aliastransition = ALIASTRANSBASE * res_scale;
	cd->r_resfudge = ALIASTRANSADJ * res_scale;
	r_origin[0] = 0.0;
	r_origin[1] = 30.0;
	r_origin[2] = -30.0;
	r_refdef.viewangles[YAW] = 0.0;
	r_refdef.viewangles[PITCH] = 0.0;
	r_refdef.viewangles[ROLL] = 0.0;

	load_model(0, ent.model, "test");

	do_key_loop(loopfunc, cd);
	pb = d_viewbuffer;
	for (i = 0; i < HEIGHT * WIDTH * 1; i++) {
		if (*pb)
			break;
		pb++;
	}

	if ((pb - d_viewbuffer < WIDTH * HEIGHT) && *pb) {
		fd = open("viewbuf", O_CREAT | O_RDWR | O_TRUNC, 0644);
		write(fd, d_viewbuffer, HEIGHT * WIDTH * 1);
		close(fd);
		//      write_xbm("viewbuf", 1280, 1024, d_viewbuffer);
		write_png_file(fname, WIDTH, HEIGHT, d_viewbuffer);
	} else {
		unlink("viewbuf.png");
	}
	stopgfx();

	return 0;
}
