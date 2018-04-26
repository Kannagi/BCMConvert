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

void Load_assimp_Array(const aiScene* scene,BCM_Header *bcm,Model3D *model,int *option);
void Load_assimp_Index(const aiScene* scene,BCM_Header *bcm,Model3D *model,int *option);
void Load_assimp_Fixe(const aiScene* scene,char flag);

void Load_assimp(const char *path,BCM_Header *bcm,Model3D *model,int *option)
{
	Assimp::Importer m_LocalImporter;
	int i,l;
	float vertex;
	aiVector3D vector;
	aiFace face;
	aiMesh* mesh;
	aiString texPath;
	const aiScene* scene = NULL;
	unsigned int flags = aiProcess_Triangulate  | aiProcess_RemoveRedundantMaterials | aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices ;

	if(option[0] == 1) bcm->flags1 ^= BCM_VERTEX;
	if(option[1] == 1) bcm->flags1 ^= BCM_TEXTCOORD;
	if(option[2] == 1) bcm->flags1 ^= BCM_NORMAL;
	if(option[3] == 1) bcm->flags1 ^= BCM_INDEX;

	if(option[6] == 1) flags |= aiProcess_GenNormals;

	if(option[7] == 1) bcm->flags1 |= BCM_INDEX_U32;
	if(option[8] == 1) bcm->flags1 |= BCM_FIXEDPOINT;

	if(option[10] == 1 || option[11] == 1 ) bcm->flags1 = BCM_VERTEX | BCM_TEXTCOORD | BCM_FIXEDPOINT | BCM_INDEX;

	if(option[11] == 1)
	{
		bcm->flags1 |= BCM_GROUP;
		flags |= aiProcess_SplitLargeMeshes;
		m_LocalImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 252);
		m_LocalImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 126);
	}

	scene = m_LocalImporter.ReadFile(path,flags );


	if(option[11] == 1)
	{
		//Load_assimp_Fixe(scene,bcm->flags1 );

		m_LocalImporter.ApplyPostProcessing(flags);
	}

	//scene =  aiImportFile(path,flags );

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


	bcm->ngroup = scene->mNumMeshes;
	printf("%d\n" ,bcm->ngroup);
	model->groupface   = (unsigned int *)malloc(bcm->ngroup*sizeof(unsigned int));
	model->groupvertex = (unsigned int *)malloc(bcm->ngroup*sizeof(unsigned int));

	//Read Meshes
	for(i = 0;i < scene->mNumMeshes;i++)
	{
		mesh = scene->mMeshes[i];

		//printf("%d %d %d %d\n",mesh->mNumVertices,mesh->mNumFaces ,mesh->mMaterialIndex,scene->mNumMaterials );
		model->groupface[i] = mesh->mNumFaces;
		model->groupvertex[i] = mesh->mNumVertices;

		//printf("%d %d\n" ,model->groupvertex[i],model->groupface[i]);

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

	int y = 1,z = 2;
	if(option[9] == 1)
	{
		y = 2;
		z = 1;
	}

	//Load Meshes
	model->v  = malloc(bcm->nv*4*3);
	model->vt = malloc(bcm->nv*4*2);
	model->vn = malloc(bcm->nv*4*3);

	model->index  = malloc(bcm->nf*4*3);

	float *v  = (float*)model->v;
	float *vt = (float*)model->vt;
	float *vn = (float*)model->vn;
	unsigned short *index = (unsigned short *)model->index;

	unsigned short *s_v  = (unsigned short*)model->v;
	unsigned short *s_vt = (unsigned short*)model->vt;
	unsigned short *s_vn = (unsigned short*)model->vn;
	unsigned int *i_index = (unsigned int *)model->index;


	int l,j=0,n,k,jt = 0,tv = 0,tf = 0,test = 0;

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
		}

		if(bcm->flags1 & BCM_FIXEDPOINT)
		{
			for(l = 0;l < mesh->mNumVertices;l++)
			{
				vector = mesh->mVertices[l];

				s_v[j+0] = ftoi4(vector.x);
				s_v[j+y] = ftoi4(vector.y);
				s_v[j+z] = ftoi4(vector.z);

				bcm->Xmin = aisgl_min(bcm->Xmin,vector.x);
				bcm->Ymin = aisgl_min(bcm->Ymin,vector.y);
				bcm->Zmin = aisgl_min(bcm->Zmin,vector.z);

				bcm->Xmax = aisgl_max(bcm->Xmax,vector.x);
				bcm->Ymax = aisgl_max(bcm->Ymax,vector.y);
				bcm->Zmax = aisgl_max(bcm->Zmax,vector.z);

				vector = mesh->mTextureCoords[0][l];
				s_vt[jt+0] = ftoi15(fabs(vector.x));
				s_vt[jt+1] = ftoi15(fabs(vector.y));

				vector = mesh->mNormals[l];
				s_vn[j+0] = ftoi4(vector.x);
				s_vn[j+y] = ftoi4(vector.y);
				s_vn[j+z] = ftoi4(vector.z);


				j+=3;
				jt += 2;
			}
		}else
		{
			for(l = 0;l < mesh->mNumVertices;l++)
			{
				vector = mesh->mVertices[l];

				v[j+0] = vector.x;
				v[j+y] = vector.y;
				v[j+z] = vector.z;

				bcm->Xmin = aisgl_min(bcm->Xmin,vector.x);
				bcm->Ymin = aisgl_min(bcm->Ymin,vector.y);
				bcm->Zmin = aisgl_min(bcm->Zmin,vector.z);

				bcm->Xmax = aisgl_max(bcm->Xmax,vector.x);
				bcm->Ymax = aisgl_max(bcm->Ymax,vector.y);
				bcm->Zmax = aisgl_max(bcm->Zmax,vector.z);

				vector = mesh->mTextureCoords[0][l];
				vt[jt+0] = fabs(vector.x);
				vt[jt+1] = fabs(vector.y);

				vector = mesh->mNormals[l];
				vn[j+0] = vector.x;
				vn[j+y] = vector.y;
				vn[j+z] = vector.z;

				j+=3;
				jt += 2;
			}


		}


		if(bcm->flags1 & BCM_INDEX_U32)
		{
			for(l = 0;l < mesh->mNumFaces;l++)
			{
				face = mesh->mFaces[l];

				for(k = 0;k < face.mNumIndices;k++)
				{
					n = face.mIndices[k];
					i_index[fi++] = n+tv;
				}
			}
		}else
		{
			for(l = 0;l < mesh->mNumFaces;l++)
			{
				face = mesh->mFaces[l];

				for(k = 0;k < face.mNumIndices;k++)
				{
					n = face.mIndices[k];
					index[fi++] = n+tv;
				}
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

	int y = 1,z = 2;
	if(option[9] == 1)
	{
		y = 2;
		z = 1;
	}


	bcm->nv = bcm->nf*3;

	model->v  = malloc(bcm->nv*4*3);
	model->vt = malloc(bcm->nv*4*2);
	model->vn = malloc(bcm->nv*4*3);

	float *v  = (float*)model->v;
	float *vt = (float*)model->vt;
	float *vn = (float*)model->vn;

	unsigned short *s_v  = (unsigned short*)model->v;
	unsigned short *s_vt = (unsigned short*)model->vt;
	unsigned short *s_vn = (unsigned short*)model->vn;

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


		if(bcm->flags1 & BCM_FIXEDPOINT)
		{
			for(l = 0;l < mesh->mNumFaces;l++)
			{
				face = mesh->mFaces[l];

				for(k = 0;k < face.mNumIndices;k++)
				{
					n = face.mIndices[k];
					vector = mesh->mVertices[n];
					s_v[j+0] = ftoi4(vector.x);
					s_v[j+y] = ftoi4(vector.y);
					s_v[j+z] = ftoi4(vector.z);

					bcm->Xmin = aisgl_min(bcm->Xmin,vector.x);
					bcm->Ymin = aisgl_min(bcm->Ymin,vector.y);
					bcm->Zmin = aisgl_min(bcm->Zmin,vector.z);

					bcm->Xmax = aisgl_max(bcm->Xmax,vector.x);
					bcm->Ymax = aisgl_max(bcm->Ymax,vector.y);
					bcm->Zmax = aisgl_max(bcm->Zmax,vector.z);

					vector = mesh->mTextureCoords[0][n];
					s_vt[jt+0] = ftoi15(fabs(vector.x));
					s_vt[jt+1] = ftoi15(fabs(vector.y));

					vector = mesh->mNormals[n];
					s_vn[j+0] = ftoi4(vector.x);
					s_vn[j+y] = ftoi4(vector.y);
					s_vn[j+z] = ftoi4(vector.z);

					j+=3;
					jt += 2;
				}
			}
		}else
		{
			for(l = 0;l < mesh->mNumFaces;l++)
			{
				face = mesh->mFaces[l];

				for(k = 0;k < face.mNumIndices;k++)
				{
					n = face.mIndices[k];
					vector = mesh->mVertices[n];
					v[j+0] = vector.x;
					v[j+y] = vector.y;
					v[j+z] = vector.z;

					bcm->Xmin = aisgl_min(bcm->Xmin,vector.x);
					bcm->Ymin = aisgl_min(bcm->Ymin,vector.y);
					bcm->Zmin = aisgl_min(bcm->Zmin,vector.z);

					bcm->Xmax = aisgl_max(bcm->Xmax,vector.x);
					bcm->Ymax = aisgl_max(bcm->Ymax,vector.y);
					bcm->Zmax = aisgl_max(bcm->Zmax,vector.z);

					vector = mesh->mTextureCoords[0][n];
					vt[jt+0] = fabs(vector.x);
					vt[jt+1] = fabs(vector.y);

					vector = mesh->mNormals[n];
					vn[j+0] = vector.x;
					vn[j+y] = vector.y;
					vn[j+z] = vector.z;

					j+=3;
					jt += 2;
				}
			}
		}

	}
}

void Load_assimp_Fixe(const aiScene* scene,char flag)
{
	int l,j=0,n,jt = 0,tv,i;
	aiVector3D vector;
	aiMesh* mesh;

	for(i = 0;i < scene->mNumMeshes;i++)
	{
		mesh = scene->mMeshes[i];


		for(l = 0;l < mesh->mNumVertices;l++)
		{
			vector = mesh->mVertices[l];

			tv = ftoi4(vector.x);
			mesh->mVertices[l].x = itof4(tv);

			tv = ftoi4(vector.y);
			mesh->mVertices[l].y = itof4(tv);

			tv = ftoi4(vector.z);
			mesh->mVertices[l].z = itof4(tv);


			vector = mesh->mTextureCoords[0][l];
			tv = ftoi15(vector.x);
			mesh->mTextureCoords[0][l].x = itof15(tv);
			tv = ftoi15(vector.y);
			mesh->mTextureCoords[0][l].y = itof15(tv);



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



			j+=3;
			jt += 2;
		}
	}
}

