#ifndef _BCM_
#define _BCM_

typedef struct
{
    char tag[4];
    unsigned char flags1,flags2,namelen,weightslen;
    float Xmin,Xmax,Ymin,Ymax,Zmin,Zmax;
    int nv,nf;
    int ntexture,nbones,ntime,ngroup;
    unsigned int size,unused[3];

}BCM_Header;

enum
{
	//Flag 1
	BCM_VERTEX = 0x01,
	BCM_TEXTCOORD = 0x02,
	BCM_NORMAL = 0x04,
	BCM_INDEX = 0x08,
	BCM_ANIM = 0x10,

	BCM_INDEX_U32 = 0x20,
	BCM_FIXEDPOINT = 0x40,
	BCM_GROUP = 0x80,

	//Flag 2
	BCM_POLY_TRIANGLE = 0x00,
	BCM_POLY_TRIANGLE_STRIP = 0x01,
	BCM_POLY_TRIANGLE_AND_STRIPS = 0x02,

	BCM_VERSION2 = 0x80
};


typedef struct
{
    void *v,*vt,*vn,*vc;
    void *index;
    unsigned int *groupvertex,*groupface;
    int *texture_index,*texture_begin,*bones_begin;
	char **name;
	unsigned int *bones,nbones;

}Model3D;

void BCM_Init(BCM_Header *bcm);
void BCM_Write( char *path,BCM_Header *bcm,Model3D *model);

#endif

