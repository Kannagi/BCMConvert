
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>      // C++ importer interface
#include "bcm.h"

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

#define ftoi4(ft4) (int)(( (float)ft4 )*16.0f)
#define ftoi8(ft8) (int)(( (float)ft8 )*256.0f)
#define ftoi12(ft12) (int)(( (float)ft12 )*4096.0f)
#define ftoi15(ft15) (int)(( (float)ft15 )*32768.0f)

#define itof4(it4) (float)(( (float)it4 )/16.0f)
#define itof8(it8) (float)(( (float)it8 )/256.0f)
#define itof12(it12) (float)(( (float)it12 )/4096.0f)
#define itof15(it15) (float)(( (float)it15 )/32768.0f)


void Load_assimp_FixeA(const aiScene* scene,char flag,float zoom)
{
	int l,tv,i;
	aiVector3D vector;
	aiMesh* mesh;

	bool bnormal,btexture,bface;

	for(i = 0;i < scene->mNumMeshes;i++)
	{
		mesh = scene->mMeshes[i];

		bface = mesh->HasFaces ();
		bnormal = mesh->HasNormals();
		btexture = mesh->HasTextureCoords(0);

		for(l = 0;l < mesh->mNumVertices;l++)
		{
			vector = mesh->mVertices[l];

			tv = ftoi4(vector.x*zoom);
			mesh->mVertices[l].x = itof4(tv);

			tv = ftoi4(vector.y*zoom);
			mesh->mVertices[l].y = itof4(tv);

			tv = ftoi4(vector.z*zoom);
			mesh->mVertices[l].z = itof4(tv);

			if(btexture == true)
			{
				if(flag & BCM_TEXTCOORD)
				{
					vector = mesh->mTextureCoords[0][l];
					tv = ftoi15(vector.x);
					mesh->mTextureCoords[0][l].x = itof15(tv);
					tv = ftoi15(vector.y);
					mesh->mTextureCoords[0][l].y = itof15(tv);
				}else
				{
					mesh->mTextureCoords[0][l].x = 0;
					mesh->mTextureCoords[0][l].y = 0;
				}
			}


			if(bnormal == true)
			{
				if(flag & BCM_NORMAL)
				{
					vector = mesh->mNormals[l];
					tv = ftoi4(vector.x);
					mesh->mNormals[l].x = itof4(tv);

					tv = ftoi4(vector.y);
					mesh->mNormals[l].y = itof4(tv);

					tv = ftoi4(vector.z);
					mesh->mNormals[l].z = itof4(tv);
				}else
				{
					mesh->mNormals[l].x = 0;
					mesh->mNormals[l].y = 0;
					mesh->mNormals[l].z = 0;
				}
			}

		}
	}
}

void Load_assimp_FixeB(const aiScene* scene,BCM_Header *bcm,Model3D *model)
{
	int l,i,j;
	aiVector3D vector;
	aiMesh* mesh;

	float *vf  = (float*)model->v;
	float *vtf = (float*)model->vt;
	float *vnf = (float*)model->vn;
	unsigned int *uindex = (unsigned int *)model->index;

	if(bcm->flags1 & BCM_FIXEDPOINT)
	{
		unsigned short *v  = (unsigned short *)malloc(bcm->nv*2*3);
		unsigned short *vt = (unsigned short *)malloc(bcm->nv*2*3);
		unsigned short *vn = (unsigned short *)malloc(bcm->nv*2*3);


		for(i = 0;i < bcm->nv;i++)
		{
			l = i*3;
			j = i*2;
			v[l+0] = ftoi4(vf[l+0]);
			v[l+1] = ftoi4(vf[l+1]);
			v[l+2] = ftoi4(vf[l+2]);

			vt[j+0] = ftoi15(vtf[j+0]);
			vt[j+1] = ftoi15(vtf[j+1]);
		}


		free(model->v);
		free(model->vt);
		free(model->vn);

		model->v  = v;
		model->vt = vt;
		model->vn = vn;
	}

	if(bcm->flags1 & BCM_INDEX_U32)
		return;

	unsigned short *index = (unsigned short *)malloc(bcm->nf*2*3);

	for(i = 0;i < bcm->nf*3;i++)
		index[i] = uindex[i];

	free(model->index);
	model->index = index;
}

