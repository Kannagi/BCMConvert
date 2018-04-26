#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcm.h"

void BCM_Init(BCM_Header *bcm)
{
	bcm->tag[0] = 'B';
	bcm->tag[1] = 'C';
	bcm->tag[2] = 'M';
	bcm->tag[3] = '\0';

	bcm->namelen = 0;
	bcm->weightslen = 0;
	bcm->flags1 = 0x0F;
	bcm->flags2 = 0;

	bcm->Xmin = 0.0f;
	bcm->Xmax = 0.0f;
	bcm->Ymin = 0.0f;
	bcm->Ymax = 0.0f;
	bcm->Zmin = 0.0f;
	bcm->Zmax = 0.0f;

	bcm->nv = 0;
	bcm->nf = 0;

	bcm->ntexture = 0;
	bcm->nbones = 0;
	bcm->ntime = 0;

	bcm->unused[0] = 0;
	bcm->unused[1] = 0;
	bcm->unused[2] = 0;
	bcm->unused[3] = 0;
}

void BCM_Write( char *path,BCM_Header *bcm,Model3D *model)
{
	FILE *file;
	file = fopen(path,"wb");
	int i,l,n,j,sizev = sizeof(float),sizeindex = sizeof(unsigned short);

	if(file == NULL) return;

	fwrite(bcm,1,sizeof(BCM_Header),file);

	printf("%d %x\n",sizeof(BCM_Header),sizeof(BCM_Header));


	if(bcm->flags1 & BCM_INDEX_U32) sizeindex = sizeof(unsigned int);
	if(bcm->flags1 & BCM_FIXEDPOINT) sizev = sizeof(unsigned short);


	if(bcm->nv > 0)
	{
		if(bcm->flags1 & BCM_VERTEX)
			fwrite(model->v,sizev,bcm->nv*3,file);

		if(bcm->flags1 & BCM_TEXTCOORD)
			fwrite(model->vt,sizev,bcm->nv*2,file);

		if(bcm->flags1 & BCM_NORMAL)
			fwrite(model->vn,sizev,bcm->nv*3,file);
	}

	if(bcm->flags1 & BCM_INDEX)
		fwrite(model->index,sizeindex,bcm->nf*3,file);


	if(bcm->ntexture > 0)
	{
		fwrite(model->texture_begin,sizeof(int),bcm->ntexture,file);

		for(i = 0;i < bcm->ntexture;i++)
		{
			j = model->texture_index[i];
			fputs(model->name[j],file);
			n = bcm->namelen - strlen(model->name[j]);
			for(l = 0;l < n;l++)
				fputc(0,file);
		}
	}

	if(bcm->flags1 & BCM_GROUP)
	{
		fwrite(model->groupvertex,sizeof(unsigned int),bcm->ngroup,file);
		fwrite(model->groupface,sizeof(unsigned int),bcm->ngroup,file);
	}


	if(bcm->flags1 & BCM_ANIM)
	{
		fwrite(model->id,sizeof(unsigned char),bcm->nv,file);
	}


	fclose(file);

}
