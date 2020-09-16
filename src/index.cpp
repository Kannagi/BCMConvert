#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>      // C++ importer interface
#include "bcm.h"
#include <iostream>
#include <vector>
#include <iterator>
#include <array>
#include <numeric>
#include <cassert>
#include <algorithm>

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

// return a couple of mapping to reorder indices per bones
// the first contains mapping from old indices to new indices, and the second from new indices to old indices
std::pair<std::vector<size_t>, std::vector<size_t>> reorder_indices_per_bone(const aiMesh& mesh){
    std::vector<size_t> mapping;
    size_t reverse_mapping[mesh.mNumVertices];
    size_t l, n, new_idx = 0;
    aiBone **bone = mesh.mBones;
    mapping.reserve(mesh.mNumVertices);
    unsigned int vertex_id;


    if(mesh.HasBones())
    {
        n = 0;

        for(l = 0;l < mesh.mNumBones;l++)
        {
            size_t k;
            aiVertexWeight *weights;
            weights = bone[l]->mWeights;
            n = bone[l]->mNumWeights;


            printf("%s\n",bone[l]->mName.C_Str());

            for(k = 0; k < n ; k++)
            {
                vertex_id = weights[k].mVertexId;
                // new index -> old index
                mapping.push_back(vertex_id);
                // old index -> new index
                reverse_mapping[vertex_id] = new_idx++;
            }
        }

    }
    else{
        //TODO: decide what to do in this case
        //suggestion would be… to return the vertices unchanged
        size_t range[mesh.mNumVertices];
        std::iota(range, range + mesh.mNumVertices, 0);
        mapping.assign(range, range + mesh.mNumVertices);
        return std::make_pair(mapping, mapping);
    }

    // take the whole index range
    size_t range[mesh.mNumVertices];
    std::iota(range, range + mesh.mNumVertices, 0);

    std::vector<size_t> remaining;
    remaining.reserve(mesh.mNumVertices);

    // find points that were not referenced by bones, and store them in `remaining`
    std::set_difference(
                range,
                range + mesh.mNumVertices,
                mapping.begin(),
                mapping.end(),
                remaining.begin()
    );
    // place the remaining points at the end
    for(auto r: remaining) {
        mapping.push_back(r);
        reverse_mapping[r] = new_idx++;
    }
    return std::make_pair(std::vector<size_t>(reverse_mapping, reverse_mapping + mesh.mNumVertices), mapping);
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
	unsigned int *index = (unsigned int *)model->index;

	//unsigned int *anim_begin = (unsigned int *)malloc(bcm->nbones*4);
	//model->bones = anim_begin;

	int l,j=0,jt = 0,tv = 0,tf = 0,n;

	int mMaterial = -1,iMaterial = 0;
	bool normal,skl;
	bool vtexture = true;

	if(option[1] == 1) vtexture = false;

	int tanim = 0;
	int ianim = 0;


	aiBone** bone;
	aiVertexWeight *weights;




	for(i = 0;i < scene->mNumMeshes;i++)
	{
		mesh = scene->mMeshes[i];
        auto maps = reorder_indices_per_bone(*mesh);
        std::vector<size_t> from_old_to_new_idx = maps.first;
        std::vector<size_t> from_new_to_old_idx = maps.second;

        normal = mesh->HasNormals();
//		bone = mesh->mBones;
//		skl = mesh->HasBones();

//		if(skl == true)
//		{
//			//bcm->flags1 |= BCM_ANIM;
//			n = 0;

//            for(l = 0;l < mesh->mNumBones;l++)
//			{
//				int k;
//				aiVertexWeight *weights;
//				weights = bone[l]->mWeights;
//				n = bone[l]->mNumWeights;
//                std::cout << "n " << n << std::endl;

//				printf("%s\n",bone[l]->mName.C_Str());

//				printf("%f %f %f %f\n",bone[l]->mOffsetMatrix.a1,bone[l]->mOffsetMatrix.a2,bone[l]->mOffsetMatrix.a3,bone[l]->mOffsetMatrix.a4);
//				printf("%f %f %f %f\n",bone[l]->mOffsetMatrix.b1,bone[l]->mOffsetMatrix.b2,bone[l]->mOffsetMatrix.b3,bone[l]->mOffsetMatrix.b4);
//				printf("%f %f %f %f\n",bone[l]->mOffsetMatrix.c1,bone[l]->mOffsetMatrix.c2,bone[l]->mOffsetMatrix.c3,bone[l]->mOffsetMatrix.c4);
//				printf("%f %f %f %f\n",bone[l]->mOffsetMatrix.d1,bone[l]->mOffsetMatrix.d2,bone[l]->mOffsetMatrix.d3,bone[l]->mOffsetMatrix.d4);
//                printf("\n");
//				for(k = 0;k < n;k++)
//				{
//                    std::cout << weights[k].mWeight << std::endl;
//					//xweights_id[weights[k].mVertexId]= w++;
//                    printf("%d,",weights[k].mVertexId);
//				}
//				//printf("\n%d\n",n);

//				//anim_begin[ianim++] = tanim;
//				tanim += n;
//			}

//		}


		if(mMaterial != mesh->mMaterialIndex)
		{
			model->texture_begin[iMaterial] = tf*3;
			model->texture_index[iMaterial] = mesh->mMaterialIndex;
			iMaterial++;

			mMaterial = mesh->mMaterialIndex;

			printf("Texture %d-------------------\n",iMaterial);
		}


		printf("%d :%d %d\n",i,mesh->mNumVertices,mesh->mNumFaces);

		for(l = 0;l < mesh->mNumVertices;l++)
		{
            vector = mesh->mVertices[from_new_to_old_idx[l]];

			float fx,fy,fz;

			fx = vector.x;
			fy = vector.y;
			fz = vector.z;

			bcm->Xmin = aisgl_min(bcm->Xmin,fx);
			bcm->Xmax = aisgl_max(bcm->Xmax,fx);

			if(y == 2)
			{
				bcm->Ymin = aisgl_min(bcm->Ymin,fz);
				bcm->Zmin = aisgl_min(bcm->Zmin,fy);

				bcm->Ymax = aisgl_max(bcm->Ymax,fz);
				bcm->Zmax = aisgl_max(bcm->Zmax,fy);
			}else
			{
				bcm->Ymin = aisgl_min(bcm->Ymin,fy);
				bcm->Zmin = aisgl_min(bcm->Zmin,fz);

				bcm->Ymax = aisgl_max(bcm->Ymax,fy);
				bcm->Zmax = aisgl_max(bcm->Zmax,fz);
			}

			v[j+0] = fx;
			v[j+y] = fy;
			v[j+z] = fz;

			if(vtexture == true)
			{
                vector = mesh->mTextureCoords[0][from_new_to_old_idx[l]];
				vt[jt+0] = (vector.x);
				vt[jt+1] = (-vector.y);
			}

			if(normal == true)
			{
                vector = mesh->mNormals[from_new_to_old_idx[l]];
				vn[j+0] = vector.x;
				vn[j+y] = vector.y;
				vn[j+z] = vector.z;
			}

			j+=3;
			jt += 2;
		}

		for(l = 0;l < mesh->mNumFaces;l++)
		{
			face = mesh->mFaces[l];
            for(size_t k = 0;k < 3;k++)
			{
                n = from_old_to_new_idx[face.mIndices[k]]+tv;
				index[fi] = n;
				fi++;
			}
			/*
			face = mesh->mFaces[l];

			printf("%d,",face.mNumIndices);

			for(int k = 0;k < face.mNumIndices;k++)
			{
				n = face.mIndices[k]+tv;
				index[fi] = n;
				fi++;
			}

			printf("%d\n",face.mNumIndices);*/

		}
		tv += mesh->mNumVertices;
		tf += mesh->mNumFaces;
	}

	//printf("%d %d, %d\n",j,jt,tf*3);
	//printf("%d %d ,%d\n",bcm->nv*3,bcm->nv*2,bcm->nf*3);


}
