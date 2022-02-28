#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/noise.hpp"
#include <GL/glew.h>
#include <glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "FastNoiseLite.h"
#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <math.h>
#define WIDTH 800
#define HEIGHT 600
#define CX 16
#define CY 256
#define CZ 16
#define SCX 6
#define SCY 16
#define SCZ 6
#define TEXTUREXSIZE 16
static GLint posAttrib;
static GLint uniModel;
GLuint shaderProgram;
glm::mat4 model;
GLuint vao;
typedef glm::tvec4<GLbyte> byte4;
unsigned int width, height;
void key_callback(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
bool place = false;
struct vec8 {
	vec8(){
	}
  vec8(float a, float b, float c, float d, float e, float f, float g, float h) {
    p[0] = a;
    p[1] = b;
    p[2] = c;
    p[3] = d;
    p[4] = e;
    p[5] = f;
    p[6] = g;
    p[7] = h;
  }

  float p[8];
};

struct block{
	block(){
	}
	block(int W, int E, int B, int T, int N, int S){
		p[0]=W;
		p[1]=E;
		p[2]=B;
		p[3]=T;
		p[4]=N;
		p[5]=S;
	}
	int p[6];
};
static block blocks[256];
void initBlocks()
{
	//    ID        W E B T N S    
	blocks[0]=block(1,1,2,0,1,1);//cBP#1 alias grass
	blocks[1]=block(4,4,3,3,4,4);//cBP#2 alias stone
	blocks[2]=block(10,10,10,10,10,10);//cBP#3 alias water
	blocks[3]=block(9,9,9,9,9,9);//cBP#4 alias leaves
	blocks[4]=block(7,7,8,8,7,7);//cBP#5 alias logs
}

struct chunk
{
	int chunkx1, chunkz1;
	char chunkBlockPositions[CX][CY][CZ];
	GLuint vboc;
	int elements;
	bool changed;
	float caveheight;
	float terheight;
	float riverheight;
	int Seed;

	void create(int chunkx, int chunkz, int seed)	
	{
		seed = Seed;
		chunkx1 = chunkx;
		chunkz1 = chunkz;
		elements = 0;
		changed = true;
		glGenBuffers(1,&vboc);
		glBindBuffer(GL_ARRAY_BUFFER, vboc);
	}
	
	void generateTerrain(int xxx, int zzz) {
		FastNoiseLite noise;
		noise.SetSeed(1313);
		noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
		noise.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXYPlanes);
		noise.SetFrequency(0.045);
		noise.SetFractalType(FastNoiseLite::FractalType_PingPong);
		noise.SetFractalOctaves(1);
		noise.SetFractalGain(0.50);
		noise.SetFractalWeightedStrength(0.80);
		noise.SetFractalPingPongStrength(1.30);
		//noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
		//noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
		//noise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2Reduced);
		//noise.SetDomainWarpAmp(2.00);
		FastNoiseLite noise2;
		noise2.SetSeed(1414);
		noise2.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
		noise2.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXYPlanes);
		noise2.SetFrequency(0.045);
		
		// Terrain noise settings
		FastNoiseLite terrnoise;
		terrnoise.SetSeed(Seed);
		terrnoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
		terrnoise.SetFrequency(0.010);
		terrnoise.SetFractalType(FastNoiseLite::FractalType_FBm);
		terrnoise.SetFractalOctaves(2);
		terrnoise.SetFractalGain(0.40);
		terrnoise.SetFractalWeightedStrength(1.00);
		//terrnoise.SetFractalPingPongStrength(3.00);
		//terrnoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
		//terrnoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add);
		
		//river noise
		FastNoiseLite rivernoise;
		rivernoise.SetSeed(Seed);
		rivernoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
		rivernoise.SetFrequency(0.010);
		rivernoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
		rivernoise.SetFractalOctaves(1);
		
		int posx, posz;
		for (int x = 0; x < CX; x++)
			for (int y = 0; y < CY; y++)
				for (int z = 0; z < CZ; z++)
					chunkBlockPositions[x][y][z] = 0;
		
        for (int x = 0; x < CX; x++){
			for(int y = 0; y < CY;y++){
				for (int z = 0; z < CZ; z++){  
					posz = z + (zzz * CX) + 1 * (CX * SCX);
					posx = x + (xxx * CZ) + 1 * (CZ * SCZ);
					
					if(y < 54)
					{
						caveheight = (pow(noise.GetNoise(posx/2.f,y/2.f,posz/2.f),2) + pow(noise2.GetNoise(posx/1.f,y/1.f,posz/1.f),2));
						if(caveheight < 0.8)
						{
							chunkBlockPositions[x][y][z] = 2;
						}
					}
						
							
					if(y > 63)
					{
						terheight = (pow(abs((terrnoise.GetNoise(posx * 0.07f,posz * 0.07f) + (0.5 * terrnoise.GetNoise(posx * 9.3f,posz * 9.3f)) + (0.25 * terrnoise.GetNoise(posx * 1.3f,posz * 1.3f)) * 25)),2.35));
						riverheight = -(rivernoise.GetNoise(posx * 0.9f,posz * 0.9f)) * 25;
						if(riverheight > 0.9)
							terheight = terheight - riverheight;
						if(terheight + 10 < 5)
						{
							chunkBlockPositions[x][int(terheight) + 100][z] = 2;
						}
						if(terheight + 10 > 4)
						{
							chunkBlockPositions[x][int(terheight) + 100][z] = 1;
						}
						for(int i = int(terheight) + 99;i > 50; i--)
						{
							chunkBlockPositions[x][i][z] = 2;
						}
						for(int i = 94;i > 53; i--)
						{
							if(chunkBlockPositions[x][i][z] == 0)
							{
								chunkBlockPositions[x][i][z] = 3;
							}
						}
					}
						
				} 
			}
               
        }
    }
	
	void set(int x, int y, int z, int type)
	{
		chunkBlockPositions[x][y][z] = type;
		changed = true;
	}
	
	float getTextureVert(int vert, int texture)
	{
		float vertCoord;
		
		if(vert == 1) vertCoord = (float)(texture * TEXTUREXSIZE)/width; //top left corner
		if(vert == 2) vertCoord = (float)((texture + 1) * TEXTUREXSIZE)/width; //top right corner
		if(vert == 3) vertCoord = (float)((texture + 1) * TEXTUREXSIZE)/width; //bottom right corner
		if(vert == 4) vertCoord = (float)(texture * TEXTUREXSIZE)/width; //bottom left corner
		
		if(vert == 5) vertCoord = 0;
		if(vert == 6) vertCoord = 0; 
		if(vert == 7) vertCoord = (float)16/height;
		if(vert == 8) vertCoord = (float)16/height;
		
		return vertCoord;
	}
    
	int getBlockTexture(int blockID, char face)
	{
		int texture;
		switch(face)
		{
		case 'W': texture = blocks[blockID-1].p[0];break;
		case 'E': texture = blocks[blockID-1].p[1];break;
		case 'B': texture = blocks[blockID-1].p[2];break;
		case 'T': texture = blocks[blockID-1].p[3];break;
		case 'N': texture = blocks[blockID-1].p[4];break;
		case 'S': texture = blocks[blockID-1].p[5];break;
		}

		return texture;
	}
	
	static vec8 *vertsi;
	void update()
	{	
		float Wx1,Wx2,Wx3,Wx4,Wy1,Wy2,Wy3,Wy4;//West/left
		float Ex1,Ex2,Ex3,Ex4,Ey1,Ey2,Ey3,Ey4;//East/right
		float Bx1,Bx2,Bx3,Bx4,By1,By2,By3,By4;//Bottom
		float Tx1,Tx2,Tx3,Tx4,Ty1,Ty2,Ty3,Ty4;//Top
		float Nx1,Nx2,Nx3,Nx4,Ny1,Ny2,Ny3,Ny4;//North/front
		float Sx1,Sx2,Sx3,Sx4,Sy1,Sy2,Sy3,Sy4;//South/back
		
		int xbound;
		int ybound;
		int zbound;
		changed = false;
		int i = 0;
		for(int x = 0; x < CX; x++)
		{
			for(int y = 0; y < CY; y++)
			{
				for(int z = 0; z < CZ; z++)
				{
					Wx1 = getTextureVert(1,getBlockTexture(chunkBlockPositions[x][y][z],'W'));Wy1 = getTextureVert(5,getBlockTexture(chunkBlockPositions[x][y][z],'W'));
					Wx2 = getTextureVert(2,getBlockTexture(chunkBlockPositions[x][y][z],'W'));Wy2 = getTextureVert(6,getBlockTexture(chunkBlockPositions[x][y][z],'W'));
					Wx3 = getTextureVert(3,getBlockTexture(chunkBlockPositions[x][y][z],'W'));Wy3 = getTextureVert(7,getBlockTexture(chunkBlockPositions[x][y][z],'W'));
					Wx4 = getTextureVert(4,getBlockTexture(chunkBlockPositions[x][y][z],'W'));Wy4 = getTextureVert(8,getBlockTexture(chunkBlockPositions[x][y][z],'W'));
					
					Ex1 = getTextureVert(1,getBlockTexture(chunkBlockPositions[x][y][z],'E'));Ey1 = getTextureVert(5,getBlockTexture(chunkBlockPositions[x][y][z],'E'));
					Ex2 = getTextureVert(2,getBlockTexture(chunkBlockPositions[x][y][z],'E'));Ey2 = getTextureVert(6,getBlockTexture(chunkBlockPositions[x][y][z],'E'));
					Ex3 = getTextureVert(3,getBlockTexture(chunkBlockPositions[x][y][z],'E'));Ey3 = getTextureVert(7,getBlockTexture(chunkBlockPositions[x][y][z],'E'));
					Ex4 = getTextureVert(4,getBlockTexture(chunkBlockPositions[x][y][z],'E'));Ey4 = getTextureVert(8,getBlockTexture(chunkBlockPositions[x][y][z],'E'));
					
					Bx1 = getTextureVert(1,getBlockTexture(chunkBlockPositions[x][y][z],'B'));By1 = getTextureVert(5,getBlockTexture(chunkBlockPositions[x][y][z],'B'));
					Bx2 = getTextureVert(2,getBlockTexture(chunkBlockPositions[x][y][z],'B'));By2 = getTextureVert(6,getBlockTexture(chunkBlockPositions[x][y][z],'B'));
					Bx3 = getTextureVert(3,getBlockTexture(chunkBlockPositions[x][y][z],'B'));By3 = getTextureVert(7,getBlockTexture(chunkBlockPositions[x][y][z],'B'));
					Bx4 = getTextureVert(4,getBlockTexture(chunkBlockPositions[x][y][z],'B'));By4 = getTextureVert(8,getBlockTexture(chunkBlockPositions[x][y][z],'B'));
					
					Tx1 = getTextureVert(1,getBlockTexture(chunkBlockPositions[x][y][z],'T'));Ty1 = getTextureVert(5,getBlockTexture(chunkBlockPositions[x][y][z],'T'));
					Tx2 = getTextureVert(2,getBlockTexture(chunkBlockPositions[x][y][z],'T'));Ty2 = getTextureVert(6,getBlockTexture(chunkBlockPositions[x][y][z],'T'));
					Tx3 = getTextureVert(3,getBlockTexture(chunkBlockPositions[x][y][z],'T'));Ty3 = getTextureVert(7,getBlockTexture(chunkBlockPositions[x][y][z],'T'));
					Tx4 = getTextureVert(4,getBlockTexture(chunkBlockPositions[x][y][z],'T'));Ty4 = getTextureVert(8,getBlockTexture(chunkBlockPositions[x][y][z],'T'));
					
					Nx1 = getTextureVert(1,getBlockTexture(chunkBlockPositions[x][y][z],'N'));Ny1 = getTextureVert(5,getBlockTexture(chunkBlockPositions[x][y][z],'N'));
					Nx2 = getTextureVert(2,getBlockTexture(chunkBlockPositions[x][y][z],'N'));Ny2 = getTextureVert(6,getBlockTexture(chunkBlockPositions[x][y][z],'N'));
					Nx3 = getTextureVert(3,getBlockTexture(chunkBlockPositions[x][y][z],'N'));Ny3 = getTextureVert(7,getBlockTexture(chunkBlockPositions[x][y][z],'N'));
					Nx4 = getTextureVert(4,getBlockTexture(chunkBlockPositions[x][y][z],'N'));Ny4 = getTextureVert(8,getBlockTexture(chunkBlockPositions[x][y][z],'N'));
					
					Sx1 = getTextureVert(1,getBlockTexture(chunkBlockPositions[x][y][z],'S'));Sy1 = getTextureVert(5,getBlockTexture(chunkBlockPositions[x][y][z],'S'));
					Sx2 = getTextureVert(2,getBlockTexture(chunkBlockPositions[x][y][z],'S'));Sy2 = getTextureVert(6,getBlockTexture(chunkBlockPositions[x][y][z],'S'));
					Sx3 = getTextureVert(3,getBlockTexture(chunkBlockPositions[x][y][z],'S'));Sy3 = getTextureVert(7,getBlockTexture(chunkBlockPositions[x][y][z],'S'));
					Sx4 = getTextureVert(4,getBlockTexture(chunkBlockPositions[x][y][z],'S'));Sy4 = getTextureVert(8,getBlockTexture(chunkBlockPositions[x][y][z],'S'));

					if(chunkBlockPositions[x][y][z] != 0)
					{
					//			      X          Y          Z             Texcoords      Normals
					if(x-1 > 0){
					if(chunkBlockPositions[x-1][y][z] ==0)
					{
						//West/left
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y,  0.5f + z,  	  Wx1,   Wy1, 	-1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Wx3,   Wy3, 	-1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y, -0.5f + z,  	  Wx2,   Wy2, 	-1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Wx3,   Wy3, 	-1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y,  0.5f + z,  	  Wx1,   Wy1, 	-1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y,  0.5f + z,  	  Wx4,   Wy4, 	-1.0f,  0.0f,  0.0f);
					}
					}else{
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y,  0.5f + z,  	  Wx1,   Wy1, 	-1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Wx3,   Wy3, 	-1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y, -0.5f + z,  	  Wx2,   Wy2, 	-1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Wx3,   Wy3, 	-1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y,  0.5f + z,  	  Wx1,   Wy1, 	-1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y,  0.5f + z,  	  Wx4,   Wy4, 	-1.0f,  0.0f,  0.0f);
					}
					if(x+1 < CX){
					if(chunkBlockPositions[x+1][y][z] ==0)
					{
						//East/right
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Ex1,   Ey1, 	 1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y, -0.5f + z,  	  Ex2,   Ey2, 	 1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y, -0.5f + z,  	  Ex3,   Ey3, 	 1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y, -0.5f + z,  	  Ex3,   Ey3, 	 1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y,  0.5f + z,  	  Ex4,   Ey4, 	 1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Ex1,   Ey1, 	 1.0f,  0.0f,  0.0f);
					}
					}else{
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Ex1,   Ey1, 	 1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y, -0.5f + z,  	  Ex2,   Ey2, 	 1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y, -0.5f + z,  	  Ex3,   Ey3, 	 1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y, -0.5f + z,  	  Ex3,   Ey3, 	 1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y,  0.5f + z,  	  Ex4,   Ey4, 	 1.0f,  0.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Ex1,   Ey1, 	 1.0f,  0.0f,  0.0f);

					}
					if(y-1 > 0){
					if(chunkBlockPositions[x][y-1][z] ==0)
					{
						//Bottom
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Bx4,   By4, 	 0.0f, -1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y,  0.5f + z,  	  Bx2,   By2, 	 0.0f, -1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y, -0.5f + z,  	  Bx3,   By3, 	 0.0f, -1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y,  0.5f + z,  	  Bx2,   By2, 	 0.0f, -1.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Bx4,   By4, 	 0.0f, -1.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y,  0.5f + z,  	  Bx1,   By1, 	 0.0f, -1.0f,  0.0f);
					}
					}else{
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Bx4,   By4, 	 0.0f, -1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y,  0.5f + z,  	  Bx2,   By2, 	 0.0f, -1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y, -0.5f + z,  	  Bx3,   By3, 	 0.0f, -1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y,  0.5f + z,  	  Bx2,   By2, 	 0.0f, -1.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Bx4,   By4, 	 0.0f, -1.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y,  0.5f + z,  	  Bx1,   By1, 	 0.0f, -1.0f,  0.0f);
					}
					if(y+1 < CY){
					if(chunkBlockPositions[x][y+1][z] ==0)
					{
						//Top
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y, -0.5f + z,  	  Tx4,   Ty4, 	 0.0f, 1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y, -0.5f + z,  	  Tx3,   Ty3, 	 0.0f, 1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Tx2,   Ty2, 	 0.0f, 1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Tx2,   Ty2, 	 0.0f, 1.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y,  0.5f + z,  	  Tx1,   Ty1, 	 0.0f, 1.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y, -0.5f + z,  	  Tx4,   Ty4, 	 0.0f, 1.0f,  0.0f);
					}
					}else{
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y, -0.5f + z,  	  Tx4,   Ty4, 	 0.0f, 1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y, -0.5f + z,  	  Tx3,   Ty3, 	 0.0f, 1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Tx2,   Ty2, 	 0.0f, 1.0f,  0.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Tx2,   Ty2, 	 0.0f, 1.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y,  0.5f + z,  	  Tx1,   Ty1, 	 0.0f, 1.0f,  0.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y, -0.5f + z,  	  Tx4,   Ty4, 	 0.0f, 1.0f,  0.0f);

					}
					if(z-1 > 0){
					if(chunkBlockPositions[x][y][z-1] ==0)
					{
						//North/front
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Nx3,   Ny3, 	 0.0f,  0.0f, -1.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y, -0.5f + z,  	  Nx4,   Ny4, 	 0.0f,  0.0f, -1.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y, -0.5f + z,  	  Nx1,   Ny1, 	 0.0f,  0.0f, -1.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y, -0.5f + z,  	  Nx1,   Ny1, 	 0.0f,  0.0f, -1.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y, -0.5f + z,  	  Nx2,   Ny2, 	 0.0f,  0.0f, -1.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Nx3,   Ny3, 	 0.0f,  0.0f, -1.0f);
					}
					}else{
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Nx3,   Ny3, 	  0.0f,  0.0f, -1.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y, -0.5f + z,  	  Nx4,   Ny4, 	  0.0f,  0.0f, -1.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y, -0.5f + z,  	  Nx1,   Ny1, 	  0.0f,  0.0f, -1.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y, -0.5f + z,  	  Nx1,   Ny1, 	  0.0f,  0.0f, -1.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y, -0.5f + z,  	  Nx2,   Ny2, 	  0.0f,  0.0f, -1.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y, -0.5f + z,  	  Nx3,   Ny3, 	  0.0f,  0.0f, -1.0f);

					}
					if(z+1 < CZ){
					if(chunkBlockPositions[x][y][z+1] ==0)
					{
						//South/back
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y,  0.5f + z,  	  Sx3,   Sy3, 	 0.0f,  0.0f,  1.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Sx1,   Sy1,    0.0f,  0.0f,  1.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y,  0.5f + z,  	  Sx4,   Sy4, 	 0.0f,  0.0f,  1.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Sx1,   Sy1, 	 0.0f,  0.0f,  1.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y,  0.5f + z,  	  Sx3,   Sy3, 	 0.0f,  0.0f,  1.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y,  0.5f + z,  	  Sx2,   Sy2, 	 0.0f,  0.0f,  1.0f);
					}
					}else{
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y,  0.5f + z,  	  Sx3,   Sy3,    0.0f,  0.0f,  1.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Sx1,   Sy1,    0.0f,  0.0f,  1.0f);
						vertsi[i++] = vec8( 0.5f + x, -0.5f + y,  0.5f + z,  	  Sx4,   Sy4, 	 0.0f,  0.0f,  1.0f);
						vertsi[i++] = vec8( 0.5f + x,  0.5f + y,  0.5f + z,  	  Sx1,   Sy1, 	 0.0f,  0.0f,  1.0f);
						vertsi[i++] = vec8(-0.5f + x, -0.5f + y,  0.5f + z,  	  Sx3,   Sy3, 	 0.0f,  0.0f,  1.0f);
						vertsi[i++] = vec8(-0.5f + x,  0.5f + y,  0.5f + z,  	  Sx2,   Sy2, 	 0.0f,  0.0f,  1.0f);

					}
					}
				}
			}
		}
		elements = i;
		glBindBuffer(GL_ARRAY_BUFFER, vboc);
		
		glBufferData(GL_ARRAY_BUFFER, elements * sizeof *vertsi, vertsi, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	void render(int x, int z)
	{
		if(changed)
			update();
		if(!elements)
			return;
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vboc);
		glEnableVertexAttribArray(0);
		
		posAttrib = glGetAttribLocation(shaderProgram, "position");
		glEnableVertexAttribArray(posAttrib);
		glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
		GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
		glEnableVertexAttribArray(texAttrib);
		glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		GLint normAttrib = glGetAttribLocation(shaderProgram, "vertNormal");
		glEnableVertexAttribArray(normAttrib);
		glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),(void*)(5 * sizeof(float)));
		
		glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x * CX, 0, z * CZ));
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, elements);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
};
vec8 *chunk::vertsi = new vec8[CX * CY * CZ * 36];
struct superChunk
{
	int seed = rand() % 9999 + 1;
	chunk chunks[SCX][SCZ];
	superChunk()
	{
		for(int x = 0; x < SCX; x++) {
            for(int z = 0; z < SCZ; z++) {
				chunks[x][z].create(x,z,seed);
			}
		}
	}
	void set(int x, int y, int z, int type)
	{
		int cx = x / CX;
		int cy = y / CY;
		int cz = z / CZ;

		x = x - (cx * CX);

		z = z - (cz * CZ);
		//if(chunks[cx][cz])
		(chunks[cx][cz]).set(x, y, z, type);
	}
	void terraingen()
	{
		
		for(int x = 0; x < SCX; x++){
			for(int z = 0; z < SCZ; z++)
			{
				(chunks[x][z]).generateTerrain(x,z);
				for(int x1 = 0; x1 < CX; x1++)
				{
					for(int y1 = 0; y1 < CY; y1++)
					{
						for(int z1 = 0; z1 < CZ; z1++)
						{
							int tree = rand() % 1000 + 1;
							if((chunks[x][z]).chunkBlockPositions[x1][y1][z1] == 1)
							{
								if(tree == 99)
								{
									set(x1 + (x * CX) , y1 + 1, z1 + (z * CZ), 5);
									set(x1 + (x * CX) , y1 + 2, z1 + (z * CZ), 5);
									set(x1 + (x * CX) , y1 + 3, z1 + (z * CZ), 5);
									
									set(x1 + (x * CX) - 1, y1 + 4, z1 + (z * CZ) + 1, 4);
									set(x1 + (x * CX) + 1, y1 + 4, z1 + (z * CZ),	  4);
									set(x1 + (x * CX) - 1, y1 + 4, z1 + (z * CZ),     4);
									set(x1 + (x * CX)    , y1 + 4, z1 + (z * CZ) + 1, 4);
									set(x1 + (x * CX)	 , y1 + 4, z1 + (z * CZ) - 1, 4);
									set(x1 + (x * CX) - 1, y1 + 4, z1 + (z * CZ) - 1, 4);
									set(x1 + (x * CX) + 1, y1 + 4, z1 + (z * CZ) + 1, 4);
									set(x1 + (x * CX) + 1, y1 + 4, z1 + (z * CZ) - 1, 4);
									
									set(x1 + (x * CX)    , y1 + 5, z1 + (z * CZ), 4);
								}
							}
						}
					}
				}
			}
		}
		
	}
	void render()
	{
		for(int x = 0; x < SCX; x++){
			for(int z = 0; z < SCZ; z++)
			{
				(chunks[x][z]).render(x,z);
			}
		}
	}
};


//camera shiz
glm::vec3 cameraPos = glm::vec3(0.0f, 4.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
GLint uniTex1;
//mouse shiz
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;

//interpolation or smth stuff
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//monitor
GLFWmonitor* monitor = glfwGetPrimaryMonitor();


// shaders ----------------------------------
const GLchar* vertexSource = R"glsl(
    #version 150 core
	
	in vec3 position;
	in vec2 texcoord;
	in vec3 vertNormal;
	
	out vec3 fragVert;
	out vec2 Texcoord;
	out vec3 fragNormal;
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 proj;
    void main()
    {
		Texcoord = texcoord;
		fragNormal = vertNormal;
		fragVert = position;
		
        gl_Position = proj * view * model * vec4(position, 1.0);
    }
)glsl";
const GLchar* fragmentSource = R"glsl(
    #version 150 core
	uniform mat4 model;
	uniform sampler2D tex;
	uniform sampler2D tex2;
	
	uniform vec3 lightPosition;
	uniform vec3 lightIntensity;
	uniform vec3 cameraPosition;
	
	in vec2 Texcoord;
	in vec3 fragNormal;
	in vec3 fragVert;
	
    out vec4 outColor;
	
    void main()
    {
		vec3 normal = normalize(transpose(inverse(mat3(model))) * fragNormal);
		vec3 fragPosition = vec3(model * vec4(fragVert, 1));
		vec4 surfaceColor = texture(tex, Texcoord);
		vec3 surfaceToLight = normalize(lightPosition - fragPosition);
		vec3 surfaceToCamera = normalize(cameraPosition - fragPosition);	
	
		//ambient
		vec3 ambient = 0.0001f * surfaceColor.rgb * lightIntensity;
		
		//diffuse
		float diffuseCoeffecient = max(0.0, dot(normal, surfaceToLight));
		vec3 diffuse = diffuseCoeffecient * surfaceColor.rgb * lightIntensity;

		//specular
		float specularCoefficient = 0.0;
		if(diffuseCoeffecient > 0.0)
			specularCoefficient = pow(max(0.0, dot(surfaceToCamera, reflect(-surfaceToLight, normal))), 20.0);
		vec3 specular = specularCoefficient * vec3(1.0f, 1.0f, 1.0f) * lightIntensity;
		
		//attenuation
		float distanceToLight = length(lightPosition - fragPosition);
		float attenuation = 1.0/ (1.0 + 0.2f * pow(distanceToLight, 2));
		
		//linear color
		vec3 linearColor = (ambient + attenuation) * (diffuse + specular);
		
		//final
		vec3 gamma = vec3(1.0/2.2);
		outColor = vec4(pow(linearColor, gamma), surfaceColor.a);
        //outColor = vec4(diffuseCoeffecient * lightIntensity * surfaceColor.rgb, surfaceColor.a);
	}
)glsl";
// end shaders ------------------------------------

int main()
{	
	initBlocks();
	// Setup ------------------------------------------------------------
	auto t_start = std::chrono::high_resolution_clock::now();
	if(!glfwInit()){fprintf(stderr, "failed to init glfw"); return -1;}
	GLFWwindow* window;	
	window = glfwCreateWindow(WIDTH, HEIGHT, "test", NULL, NULL);
	if(!window)
	{
		fprintf(stderr, "window failed to init");
		glfwTerminate();
		return -1;
	}	
	glfwMakeContextCurrent(window);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "glew failed to init");
		glfwTerminate();
		return -1;
	}
	// End Setup -----------------------------------------------------------
	srand (time(NULL));
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSwapInterval(1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glEnable(GL_BLEND);
	
    	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// compile vertex shader --------------------------------
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	// end vertex shader -------------------------------------
	//compile fragment shader --------------------------------
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    	glCompileShader(fragmentShader);
	//end fragment shader -------------------------------------
	shaderProgram = glCreateProgram();
   	glAttachShader(shaderProgram, vertexShader);
   	glAttachShader(shaderProgram, fragmentShader);
    	glBindFragDataLocation(shaderProgram, 0, "outColor");
    	glLinkProgram(shaderProgram);
	//check if shaders compiled ----------------------------
	GLint status;
   	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (status == GL_TRUE)
	{
		printf("shaders work");
	}
	//end shader check --------------------------------------
	glUseProgram(shaderProgram);
	
	
	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
    
	posAttrib = glGetAttribLocation(shaderProgram, "position");
    	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
	GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
    	glEnableVertexAttribArray(texAttrib);
    	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	GLint normAttrib = glGetAttribLocation(shaderProgram, "vertNormal");
	glEnableVertexAttribArray(normAttrib);
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),(void*)(5 * sizeof(float)));
	

	
	unsigned char* pixel_data;
	int ch;
	
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	pixel_data = stbi_load("textures.png", (int*)&width, (int*)&height, &ch, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
	glGenerateMipmap(GL_TEXTURE_2D);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	uniTex1 = glGetUniformLocation(shaderProgram, "tex");
	glUniform1i(uniTex1, 0);

	uniModel = glGetUniformLocation(shaderProgram, "model");
	GLint uniView = glGetUniformLocation(shaderProgram, "view");
	
	glm::mat4 proj = glm::perspective(glm::radians(90.0f), 800.0f / 600.0f, 0.1f, 200.0f);
   	GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
   	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
	
	// lighting shite
	GLint lightPos = glGetUniformLocation(shaderProgram, "lightPosition");
	GLint lightInten = glGetUniformLocation(shaderProgram, "lightIntensity");
	GLint glcamerapos = glGetUniformLocation(shaderProgram, "cameraPosition");
	
	superChunk *theChunk =  new superChunk();
	theChunk->terraingen();
	while(!glfwWindowShouldClose(window))
	{
		key_callback(window);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if(place) 
		{
			theChunk->set(cameraPos.x,cameraPos.y,cameraPos.z,2);
			place = true;
			//theChunk->terraingen();
		}
		//glUniform3f(lightPos, cameraPos.x, cameraPos.y, cameraPos.z);
		glUniform3f(lightPos, 30, 110, 25);
		//glUniform3f(lightInten, cameraPos.x, cameraPos.y, cameraPos.z);
		glUniform3f(lightInten, 0.7882f, 0.6666f, 0.1647f);
		//glUniform3f(lightInten, 1.0f, 0.0f, 1.0f);

		//glUniform3f(lightInten, 1.0f, 0.0f, 0.0f);
		glUniform3f(glcamerapos, cameraPos.x, cameraPos.y, cameraPos.z);
		
		auto t_now = std::chrono::high_resolution_clock::now();
       		float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;  
		
		glm::mat4 view = glm::lookAt(cameraPos, cameraFront + cameraPos, cameraUp);
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		glBindVertexArray(vao);
		
		theChunk->render();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteVertexArrays(1, &vao);
	glDeleteTextures(1, &tex);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

float gravity = -0.81f;
float velocity = 0.0f;
bool space = true;
void key_callback(GLFWwindow* window)
{
	int blockx = cameraPos.x;
	int blocky = cameraPos.y - 2;
	int blockz = cameraPos.z;
	//grabbity time
	if(velocity != 0.0f)
	{
		velocity += gravity * deltaTime;
		cameraPos.y += velocity;
	}
	if (cameraPos.y < 100.0f)
	{
		velocity = 0.0f;
		cameraPos.y += 100.0f - cameraPos.y;
	}

	glm::vec3 cameraFront2(cameraFront.x,0.0f,cameraFront.z);
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
	
	float cameraSpeed = 15.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * glm::normalize(cameraFront2);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * glm::normalize(cameraFront2);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && space == false)
	{
		space = true;
		velocity = 0.2f; //jump shpeed
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
		space = false;
	
	if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		glfwSetWindowMonitor(window, monitor, 0, 0, 1920, 1080, 0);
	if(glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && place == false)
		place = true;
	if(glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE)
		place = false;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // check da bounds neega
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
	

    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}
