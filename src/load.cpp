#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>      // C++ importer interface
#include "bcm.h"

void Load_assimp_Array(const aiScene* scene,BCM_Header *bcm,Model3D *model,int *option);
void Load_assimp_Index(const aiScene* scene,BCM_Header *bcm,Model3D *model,int *option);
void Load_assimp_FixeA(const aiScene* scene,char flag,float zoom);
void Load_assimp_FixeB(const aiScene* scene,BCM_Header *bcm,Model3D *model);
void Sort_assimp(const aiScene* scene,BCM_Header *bcm,Model3D *model);


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
	unsigned int flags = aiProcess_Triangulate  | aiProcess_RemoveRedundantMaterials | aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_ImproveCacheLocality;

	if(option[0] == 1) bcm->flags1 ^= BCM_VERTEX;
	if(option[1] == 1) bcm->flags1 ^= BCM_TEXTCOORD;
	if(option[2] == 1) bcm->flags1 ^= BCM_NORMAL;
	if(option[3] == 1) bcm->flags1 ^= BCM_INDEX;

	if(option[6] == 1) flags |= aiProcess_GenNormals;

	if(option[7] == 1) bcm->flags1 |= BCM_INDEX_U32;
	if(option[8] == 1) bcm->flags1 |= BCM_FIXEDPOINT;

	if(option[10] == 1 || option[11] == 1 ) bcm->flags1 = BCM_VERTEX | BCM_TEXTCOORD | BCM_FIXEDPOINT | BCM_INDEX;

	//flags = aiProcess_JoinIdenticalVertices | aiProcess_RemoveRedundantMaterials  | aiProcess_SortByPType | aiProcess_ImproveCacheLocality;

	scene = m_LocalImporter.ReadFile(path,flags );


    Load_assimp_FixeA(scene,bcm->flags1,1);

	//flags |= aiProcess_SplitByBoneCount;
	//m_LocalImporter.SetPropertyInteger(AI_CONFIG_PP_SBBC_MAX_BONES,1);


	m_LocalImporter.ApplyPostProcessing(flags);



	if(option[11] == 100)
	{
		bcm->flags1 |= BCM_GROUP;

		flags |= aiProcess_SplitLargeMeshes;


		m_LocalImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 128);
		m_LocalImporter.ApplyPostProcessing(flags);

		int n = 0;
		for(i = 0;i < scene->mNumMeshes;i++)
		{
			mesh = scene->mMeshes[i];
			n = mesh->mNumFaces;
			if(n > 212) break;
		}
		if(n > 212)
		{
			m_LocalImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 212);
			m_LocalImporter.ApplyPostProcessing(flags);
		}
		//flags |= aiProcess_SplitByBoneCount;
		//m_LocalImporter.SetPropertyInteger(AI_CONFIG_PP_SBBC_MAX_BONES,2);
		//m_LocalImporter.ApplyPostProcessing(flags);

		//flags |= aiProcess_SplitLargeMeshes;

		//m_LocalImporter.ApplyPostProcessing(flags);*/

	}


	//scene =  aiImportFile(path,flags );

	if( !scene)
	{
		printf("ERROR::ASSIMP:: %s\n", m_LocalImporter.GetErrorString() );
		printf("Error !\n" );
		exit(-42);
	}
	printf("num meches : %d %d %d\n",scene->mNumMeshes,scene->mNumTextures,scene->mNumMaterials);

	//Load Texture
	bcm->ntexture = scene->mNumMaterials;
	bcm->namelen = 5;

	if(bcm->ntexture > 0)
	{
		model->texture_index = (int *)malloc(scene->mNumMaterials*sizeof(int));
		model->texture_begin = (int *)malloc(scene->mNumMaterials*sizeof(int));
		model->name          = (char**)malloc(scene->mNumMaterials*sizeof(char*));

		for(i = 0;i < scene->mNumMaterials;i++)
		{
			scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
			printf("%d : %s %d\n",i,texPath.C_Str(),texPath.length);
			model->name[i] = (char*)malloc(texPath.length*sizeof(char) +1);
			strcpy(model->name[i],texPath.C_Str());

			for(int l = 0;l < texPath.length;l++)
				if(model->name[i][l] < ' ') model->name[i][l] = 0;


			if(bcm->namelen < texPath.length) bcm->namelen = texPath.length;

			scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
		}
		bcm->namelen++;
	}


	bcm->ngroup = scene->mNumMeshes;
	printf("g : %d  t: %d\n" ,bcm->ngroup,bcm->ntexture);
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


		//printf("%d\n" ,mesh->HasNormals());

		bcm->nf += mesh->mNumFaces;
		bcm->nv += mesh->mNumVertices;
		bcm->nbones += mesh->mNumBones;
	}

	printf("vertex : %d face : %d bones %d\n",bcm->nv,bcm->nf,bcm->nbones );
    if(bcm->nbones)
	{
		model->bones_begin = ( int *)malloc(bcm->nbones*sizeof(int));
		model->matrix_bones = ( float *)malloc(bcm->nbones*sizeof(float)*16);
	}

	printf("anim : %d\n",scene->mNumAnimations );
	//if(bcm->flags1 & BCM_INDEX)

	Load_assimp_Index(scene,bcm,model,option);

	//Sort_assimp(scene,bcm,model);

	Load_assimp_FixeB(scene,bcm,model );


}


