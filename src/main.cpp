#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcm.h"

void Load_assimp(const char *path,BCM_Header *bcm,Model3D *model,int *option);
void output_filename(char *path,char *str);

int main(int argc, char** argv)
{
	int i;
	int option[20];
	for(i = 0; i < 20;i++) option[i] = 0;
	int ok = 0;

	char inname[500],outname[500];
	inname[0] = 0;
	outname[0] = 0;

	for(i = 1; i < argc;i++)
	{
		if(argv[i][0] == '-')
		{
			if(strcmp(argv[i],"-nov")     == 0) option[0] = 1;
			if(strcmp(argv[i],"-novt")    == 0) option[1] = 1;
			if(strcmp(argv[i],"-novn")    == 0) option[2] = 1;
			if(strcmp(argv[i],"-noindex") == 0) option[3] = 1;
			if(strcmp(argv[i],"-noanim")  == 0) option[4] = 1;

			if(strcmp(argv[i],"-trisp")   == 0) option[5] = 1;
			if(strcmp(argv[i],"-genN")    == 0) option[6] = 1;

			if(strcmp(argv[i],"-index32")     == 0) option[7] = 1;
			if(strcmp(argv[i],"-fixedpoint") == 0) option[8] = 1;



			if(strcmp(argv[i],"-invYZ") == 0) option[9] = 1;

			if(strcmp(argv[i],"-PS1") == 0) option[10] = 1;
			if(strcmp(argv[i],"-PS2") == 0) option[11] = 1;

			//if(strcmp(argv[i],"-index32") == 0) option[11] = 1;

			ok = 0;

			if(strcmp(argv[i],"-o") == 0)
			{
				ok = 1;
			}


		}else
		{
			if(ok == 0) strcpy(inname,argv[i]);
			if(ok == 1) strcpy(outname,argv[i]);
			ok = 0;
		}
	}

	if(inname[0] == 0)
	{
		printf("Option : -nov -novt -novn - noindex -noanim -trisp -genN -index32 -fixedpoint -PS1 -PS2 -invYZ\n");
		printf("By default : v / vt / vn / index , if possible with anim\n");
		printf("Exemple :\nBCMconvert myfile.obj [option]\n");
		return 0;
	}


	BCM_Header bcm;
	Model3D model;
	BCM_Init(&bcm);

	Load_assimp(inname,&bcm,&model,option);


	if(outname[0] == 0) output_filename(inname,outname);
	strcat(outname,".bcm");

	BCM_Write(outname,&bcm,&model);


    return 0;
}

void output_filename(char *path,char *str)
{
    int l = 0;
    int i = 0;
    while(path[i] != 0 && path[i] != '.' )
    {
        str[l] = path[i];
        l++;

        if(path[i] == '/' || path[i] == '\\') l = 0;
        i++;
    }
    str[l] = 0;
}
