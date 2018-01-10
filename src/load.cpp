#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "bcm.h"

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

void Load_assimp_Array(const aiScene* scene,BCM_Header *bcm,Model3D *model,int *option);
void Load_assimp_Index(const aiScene* scene,BCM_Header *bcm,Model3D *model,int *option);


void Load_assimp(const char *path,BCM_Header *bcm,Model3D *model,int *option)
{
	int i,l;
	float vertex;
	aiVector3D vector;
	aiFace face;
	aiMesh* mesh;
	aiString texPath;
	const aiScene* scene = NULL;
	unsigned int flags = aiProcess_Triangulate  | aiProcess_JoinIdenticalVertices | aiProcess_OptimizeMeshes | aiProcess_RemoveRedundantMaterials;

	if(option[0] == 1) bcm->flags1 ^= BCM_VERTEX;
	if(option[1] == 1) bcm->flags1 ^= BCM_TEXTCOORD;
	if(option[2] == 1) bcm->flags1 ^= BCM_NORMAL;
	if(option[3] == 1) bcm->flags1 ^= BCM_INDEX;

	if(option[6] == 1)  flags |= aiProcess_GenNormals;


	scene =  aiImportFile(path,flags );

	if( !scene)
	{
		printf("Error !\n" );
		exit(-42);
	}
	//printf("num meches : %d %d %d\n",scene->mNumMeshes,scene->mNumTextures,scene->mNumMaterials);

	//Load Texture
	bcm->ntexture = scene->mNumMaterials;
	if(bcm->ntexture > 0)
	{
		model->texture_index = (int *)malloc(scene->mNumMaterials*sizeof(int));
		model->texture_begin = (int *)malloc(scene->mNumMaterials*sizeof(int));
		model->name          = (char**)malloc(scene->mNumMaterials*sizeof(char*));

		for(i = 0;i < scene->mNumMaterials;i++)
		{
			scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
			//printf("%d : %s %d\n",i,texPath.C_Str(),texPath.length);
			model->name[i] = (char*)malloc(texPath.length*sizeof(char) +1);
			strcpy(model->name[i],texPath.C_Str());

			if(bcm->namelen < texPath.length) bcm->namelen = texPath.length;
		}
		bcm->namelen++;
	}

	//Read Meshes
	for(i = 0;i < scene->mNumMeshes;i++)
	{
		mesh = scene->mMeshes[i];

		//printf("%d %d %d %d\n",mesh->mNumVertices,mesh->mNumFaces ,mesh->mMaterialIndex,scene->mNumMaterials );

		bcm->nf += mesh->mNumFaces;
		bcm->nv += mesh->mNumVertices;
		//bcm->nbones += mesh->mNumBones;
	}

	if(bcm->flags1 & BCM_INDEX)
		Load_assimp_Index(scene,bcm,model,option);
	else
		Load_assimp_Array(scene,bcm,model,option);

	printf("vertex : %d face : %d\n",bcm->nv,bcm->nf );


}

void Load_assimp_Index(const aiScene* scene,BCM_Header *bcm,Model3D *model,int *option)
{
	int i=0,fi=0;
	aiVector3D vector;
	aiFace face;
	aiMesh* mesh;

	//Load Meshes
	model->v  = (float *)malloc(bcm->nv*sizeof(float)*3);
	model->vt = (float *)malloc(bcm->nv*sizeof(float)*2);
	model->vn = (float *)malloc(bcm->nv*sizeof(float)*3);

	model->f  = (unsigned short *)malloc(bcm->nf*sizeof(unsigned short)*3);

	float *v  = model->v;
	float *vt = model->vt;
	float *vn = model->vn;

	int l,j=0,n,k,jt = 0,tv = 0,tf = 0,test = 0;
	int cy = 1;

	int mMaterial = -1,iMaterial = 0;


	for(i = 0;i < scene->mNumMeshes;i++)
	{
		mesh = scene->mMeshes[i];

		//printf("%d %d  %d\n",i,tf,mesh->mMaterialIndex);

		if(mMaterial != mesh->mMaterialIndex)
		{
			model->texture_begin[iMaterial] = tf*3;
			model->texture_index[iMaterial] = mesh->mMaterialIndex;
			iMaterial++;

			mMaterial = mesh->mMaterialIndex;
			cy = !cy;
		}

		if(option[11] == 0 ) cy = 0;


		for(l = 0;l < mesh->mNumVertices;l++)
		{
			vector = mesh->mVertices[l];
			v[j+0] = vector.x;
			v[j+1] = vector.y;
			v[j+2] = vector.z;

			bcm->Xmin = aisgl_min(bcm->Xmin,vector.x);
			bcm->Ymin = aisgl_min(bcm->Ymin,vector.y);
			bcm->Zmin = aisgl_min(bcm->Zmin,vector.z);

			bcm->Xmax = aisgl_max(bcm->Xmax,vector.x);
			bcm->Ymax = aisgl_max(bcm->Ymax,vector.y);
			bcm->Zmax = aisgl_max(bcm->Zmax,vector.z);

			vector = mesh->mTextureCoords[0][l];
			vt[jt+0] = fabs(vector.x);
			vt[jt+1] = fabs(vector.y)+cy;

			vector = mesh->mNormals[l];
			vn[j+0] = vector.x;
			vn[j+1] = vector.y;
			vn[j+2] = vector.z;

			j+=3;
			jt += 2;
		}



		test = 0;
		for(l = 0;l < mesh->mNumFaces;l++)
		{
			face = mesh->mFaces[l];

			for(k = 0;k < face.mNumIndices;k++)
			{
				n = face.mIndices[k];
				model->f[fi++] = n+tv;
			}
		}

		tv += mesh->mNumVertices;
		tf += mesh->mNumFaces;
	}
/*
	for(i = 0;i < bcm->ntexture;i++)
    {
		printf("%d %d %d\n",i,model->texture_begin[i],model->texture_index[i]);
    }*/


}

void Load_assimp_Array(const aiScene* scene,BCM_Header *bcm,Model3D *model,int *option)
{
	int i,f=0;
	aiVector3D vector;
	aiFace face;
	aiMesh* mesh;

	//Load Meshes
	bcm->nv = bcm->nf*3;

	model->v  = (float *)malloc(bcm->nv*sizeof(float)*3);
	model->vt = (float *)malloc(bcm->nv*sizeof(float)*2);
	model->vn = (float *)malloc(bcm->nv*sizeof(float)*3);

	model->f  = (unsigned short *)malloc(bcm->nf*sizeof(unsigned short)*3);

	float *v  = model->v;
	float *vt = model->vt;
	float *vn = model->vn;

	int l,j=0,n,k,jt = 0;
	int mMaterial = -1,iMaterial = 0;
	for(i = 0;i < scene->mNumMeshes;i++)
	{
		mesh = scene->mMeshes[i];

		if(mMaterial != mesh->mMaterialIndex)
		{
			model->texture_begin[iMaterial] = j/3;
			model->texture_index[iMaterial] = mesh->mMaterialIndex;
			iMaterial++;

			mMaterial = mesh->mMaterialIndex;
		}

		for(l = 0;l < mesh->mNumFaces;l++)
		{
			face = mesh->mFaces[l];

			for(k = 0;k < face.mNumIndices;k++)
			{
				n = face.mIndices[k];
				vector = mesh->mVertices[n];
				v[j+0] = vector.x;
				v[j+1] = vector.y;
				v[j+2] = vector.z;

				bcm->Xmin = aisgl_min(bcm->Xmin,vector.x);
				bcm->Ymin = aisgl_min(bcm->Ymin,vector.y);
				bcm->Zmin = aisgl_min(bcm->Zmin,vector.z);

				bcm->Xmax = aisgl_max(bcm->Xmax,vector.x);
				bcm->Ymax = aisgl_max(bcm->Ymax,vector.y);
				bcm->Zmax = aisgl_max(bcm->Zmax,vector.z);

				vector = mesh->mTextureCoords[0][n];
				vt[jt+0] = vector.x;
				vt[jt+1] = vector.y;

				vector = mesh->mNormals[n];
				vn[j+0] = vector.x;
				vn[j+1] = vector.y;
				vn[j+2] = vector.z;

				j+=3;
				jt += 2;
			}
		}
	}
}
