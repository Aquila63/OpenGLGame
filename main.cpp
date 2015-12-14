
//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"

#include "ImageLoader.h"

// Assimp includes

#include <assimp/cimport.h> // C importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.
#include <assert.h>

#include <random>
#include "Camera.h"
#include "text.h"

/*----------------------------------------------------------------------------
						MESHES
  ----------------------------------------------------------------------------*/

#define GRASS_TERRAIN_MESH "../Meshes/grass_terrain.obj"
#define TREE_MESH "../Meshes/lowpolytree_big.obj"
#define MEDKIT_MESH "../Meshes/FirstAidMedKit2.obj"



/*----------------------------------------------------------------------------
					TEXTURE FILES
------------------------------------------------------------------------------*/

#define SKY_RIGHT "../Textures/Skybox/sky_pos_x.bmp"
#define SKY_LEFT "../Textures/Skybox/sky_neg_x.bmp"
#define SKY_TOP "../Textures/Skybox/sky_pos_y.bmp"
#define SKY_BOTTOM "../Textures/Skybox/sky_neg_y.bmp"
#define SKY_FRONT "../Textures/Skybox/sky_pos_z.bmp"
#define SKY_BACK "../Textures/Skybox/sky_neg_z.bmp"

#define GRASS_TERRAIN_TEX "../Textures/grass.jpg"
#define TREE_LEAVES_TEX "../Textures/green.jpg"
//#define MEDKIT_TEX "../Textures/medkit.tga"
#define MEDKIT_TEX "../Textures/red.jpg"


#define VERTEX_SHADER_LOCATION "../Shaders/simpleVertexShader.txt"
#define FRAGMENT_SHADER_LOCATION "../Shaders/simpleFragmentShader.txt"
#define SKYBOX_VERTEX_SHADER_LOC "../Shaders/skyboxVertexShader.txt"
#define SKYBOX_FRAG_SHADER_LOC "../Shaders/skyboxFragShader.txt"
#define FOG_VERTEX_SHADER_LOC "../Shaders/fogVertexShader.txt"
#define FOG_FRAG_SHADER_LOC "../Shaders/fogFragShader.txt"

/*----------------------------------------------------------------------------
					OBJECT CONSTANTS
------------------------------------------------------------------------------*/

#define TERRAIN_SIZE	75
//#define TREE_COUNT		75
//#define MEDKIT_COUNT	10


/*----------------------------------------------------------------------------
					TEXT CONSTANTS
------------------------------------------------------------------------------*/

#define TEXT_FONT "../freemono.png"
#define TEXT_META "../freemomo.meta"

/*----------------------------------------------------------------------------
					SOUNDS
------------------------------------------------------------------------------*/

#define AMBIENT_SOUND "../Sounds/ambient2.wav"
#define MEDKIT_SOUND "../Sounds/medkit.wav"

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

std::vector<float> g_vp, g_vn, g_vt;
std::vector<float> mat;

int g_point_count = 0;


// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID;
GLuint skyboxShaderID;
GLuint fogShaderID;

GLuint ground_tex;
GLuint table_tex;

unsigned int mesh_vao = 0;
int width = 1280;
int height = 720;

//Middle of screen
int midWidth = width / 2;
int midHeight = height / 2;

float orientY, orientX;

GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;

float camX = 5.0;
float camY = 5.0;
float camZ = 8.0;

float  delta;

int fogEquation = 1;
//vec4 fogColor = { 0.5f, 0.5f, 0.5f, 1.0f };
float fogDensity = 0.10;

vec3 lightPostion(30, -10, 50);
vec3 eyePosition(0, 10, 85);

ImageLoader il;

enum ObjectType {
	TREE,
	MEDKIT
};

/*
 *-------------------------------------------------
 *----------------GAME CLASSES---------------------
 *-------------------------------------------------
 */

class BoundingBox
{
public:

	GLuint vbo_verts;
	GLuint ibo_elements;

	vec3 size, center;
	mat4 transform;

	GLfloat vertices[36] = {
		-0.5, -0.5, -0.5, 1.0,
		 0.5, -0.5, -0.5, 1.0,
		 0.5,  0.5, -0.5, 1.0,
		-0.5,  0.5, -0.5, 1.0,
		-0.5, -0.5,  0.5, 1.0,
		 0.5, -0.5,  0.5, 1.0,
		 0.5,  0.5,  0.5, 1.0,
		-0.5,  0.5,  0.5, 1.0,
	};
	
	GLushort elements[16] = {
		0, 1, 2, 3,
		4, 5, 6, 7,
		0, 4, 1, 5, 2, 6, 3, 7
	};

	GLfloat 
		minX, maxX,
		minY, maxY,
		minZ, maxZ;
	
	vec3 worldPosition;

	void init()
	{
		glGenBuffers(1, &this->vbo_verts);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo_verts);
		glBufferData(GL_ARRAY_BUFFER, sizeof(this->vertices), this->vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &this->ibo_elements);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo_elements);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(this->ibo_elements), &this->ibo_elements, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void findMaxMinValues(const aiMesh* mesh)
	{
		this->minX = this->maxX = mesh->mVertices[0].x;
		this->minY = this->maxY = mesh->mVertices[0].y;
		this->minZ = this->maxZ = mesh->mVertices[0].z;

		for (int i = 0; i < mesh->mNumVertices; i++)
		{
			if (mesh->mVertices[i].x < this->minX) this->minX = mesh->mVertices[i].x;
			if (mesh->mVertices[i].x > this->maxX) this->maxX = mesh->mVertices[i].x;
			
			if (mesh->mVertices[i].x < this->minY) this->minY = mesh->mVertices[i].y;
			if (mesh->mVertices[i].y > this->maxY) this->maxY = mesh->mVertices[i].y;
			
			if (mesh->mVertices[i].z < this->minZ) this->minZ = mesh->mVertices[i].z;
			if (mesh->mVertices[i].z > this->maxZ) this->maxZ = mesh->mVertices[i].z;
		}

		this->size = vec3(this->maxX - this->minX, this->maxY - this->minY, this->maxZ - this->minZ);
		this->center = vec3((this->minX + this->maxX) / 2, (this->minY + this->maxY) / 2, (this->minZ + this->maxZ) / 2);
		this->transform = translate(identity_mat4(), this->center);
	}

	void setUp()
	{
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo_verts);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
		glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, 0);
		glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, (GLvoid*)(4 * sizeof(GLushort)));
		glDrawElements(GL_LINES, 8, GL_UNSIGNED_SHORT, (GLvoid*)(8 * sizeof(GLushort)));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		
		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glDeleteBuffers(1, &this->vbo_verts);
		glDeleteBuffers(1, &this->ibo_elements);
	}

	void updateCoordinates() {}
};

class Terrain
{
public:
	std::vector<float> vp, vn, vt;
	int point_count;
	GLuint loc1, loc2, loc3;
	GLuint vao;
	GLuint vbo;

	GLuint tex;

	vec3 position;

	bool load_mesh(const char* file_name) {
		const aiScene* scene = aiImportFile(file_name, aiProcess_Triangulate); // TRIANGLES!
		if (!scene) {
			fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
			return false;
		}
		printf("  %i animations\n", scene->mNumAnimations);
		printf("  %i cameras\n", scene->mNumCameras);
		printf("  %i lights\n", scene->mNumLights);
		printf("  %i materials\n", scene->mNumMaterials);
		printf("  %i meshes\n", scene->mNumMeshes);
		printf("  %i textures\n", scene->mNumTextures);

		for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
			const aiMesh* mesh = scene->mMeshes[m_i];
			printf("    %i vertices in mesh\n", mesh->mNumVertices);
			this->point_count = mesh->mNumVertices;
			for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
				if (mesh->HasPositions()) {
					const aiVector3D* vp = &(mesh->mVertices[v_i]);
					//printf ("      vp %i (%f,%f,%f)\n", v_i, vp->x, vp->y, vp->z);
					this->vp.push_back(vp->x);
					this->vp.push_back(vp->y);
					this->vp.push_back(vp->z);
				}
				if (mesh->HasNormals()) {
					const aiVector3D* vn = &(mesh->mNormals[v_i]);
					//printf ("      vn %i (%f,%f,%f)\n", v_i, vn->x, vn->y, vn->z);
					this->vn.push_back(vn->x);
					this->vn.push_back(vn->y);
					this->vn.push_back(vn->z);
				}
				if (mesh->HasTextureCoords(0)) {
					const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
					//printf ("      vt %i (%f,%f)\n", v_i, vt->x, vt->y);
					this->vt.push_back(vt->x);
					this->vt.push_back(vt->y);
				}
				if (mesh->HasTangentsAndBitangents()) {
					// NB: could store/print tangents here
				}
			}
		}
		printf("\n");
		aiReleaseImport(scene);
		return true;
	}

	void generateBuffer(const char* mesh_loc) 
	{
		load_mesh(mesh_loc);
		unsigned int vp_vbo = 0;
		this->loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
		this->loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
		this->loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

		glGenVertexArrays(1, &this->vao);
		glBindVertexArray(this->vao);

		glGenBuffers(1, &vp_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);

		glBufferData(GL_ARRAY_BUFFER, this->point_count * 3 * sizeof(float), &this->vp[0], GL_STATIC_DRAW);
		unsigned int vn_vbo = 0;
		glGenBuffers(1, &vn_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glBufferData(GL_ARRAY_BUFFER, this->point_count * 3 * sizeof(float), &this->vn[0], GL_STATIC_DRAW);

		unsigned int vt_vbo = 0;
		glGenBuffers (1, &vt_vbo);
		glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
		glBufferData (GL_ARRAY_BUFFER, this->point_count * 2 * sizeof (float), &this->vt[0], GL_STATIC_DRAW);

		//unsigned int vao = 0;
		glBindVertexArray(this->vao);

		//Points
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		//Normals 
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		//Textures
		glEnableVertexAttribArray (2);
		glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
		glVertexAttribPointer (2, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void loadTexture(const char *filename)
	{
		//GLuint texture;
		unsigned char *data;

		int width, height;
		int n = 0;

		//data = stbi_load(filename, &width, &height, &n, 0);
		data = il.load_image(filename, &width, &height, &n, 0);

		glGenTextures(1, &this->tex);
		glBindTexture(GL_TEXTURE_2D, this->tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glGenerateMipmap(GL_TEXTURE_2D);

		//stbi_image_free(data);
		il.free_image(data);
	}

	void draw()
	{
		glBindVertexArray(this->vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, this->tex);
		glDrawArrays(GL_TRIANGLES, 0, this->point_count);
		glBindVertexArray(0);
	}
};

Terrain terrain;

class Trees
{
#define TREE_AMOUNT	1000;
public:
	std::vector<float> vp, vn, vt;
	int point_count;
	GLuint loc1, loc2, loc3;
	GLuint vao;
	GLuint vbo;

	GLuint tex;

	int amount;

	std::vector<GLfloat> posX;
	std::vector<GLfloat> posZ;
	
	std::vector<vec3> positions;

	BoundingBox bbox;
	std::vector<BoundingBox> boxes;

	Trees()
	{
		generateRandomPositionCoords();
	}

	bool load_mesh(const char* file_name) {
		const aiScene* scene = aiImportFile(file_name, aiProcess_Triangulate); // TRIANGLES!
																			   //fprintf (stderr, "ERROR: reading mesh %s\n", file_name);
		if (!scene) {
			fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
			return false;
		}
		printf("  %i animations\n", scene->mNumAnimations);
		printf("  %i cameras\n", scene->mNumCameras);
		printf("  %i lights\n", scene->mNumLights);
		printf("  %i materials\n", scene->mNumMaterials);
		printf("  %i meshes\n", scene->mNumMeshes);
		printf("  %i textures\n", scene->mNumTextures);

		for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) 
		{
			const aiMesh* mesh = scene->mMeshes[m_i];
			printf("    %i vertices in mesh\n", mesh->mNumVertices);
			this->point_count = mesh->mNumVertices;
			for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) 
			{
				if (mesh->HasPositions()) 
				{
					const aiVector3D* vp = &(mesh->mVertices[v_i]);
					//printf ("      vp %i (%f,%f,%f)\n", v_i, vp->x, vp->y, vp->z);
					this->vp.push_back(vp->x);
					this->vp.push_back(vp->y);
					this->vp.push_back(vp->z);
				}
				if (mesh->HasNormals()) 
				{
					const aiVector3D* vn = &(mesh->mNormals[v_i]);
					//printf ("      vn %i (%f,%f,%f)\n", v_i, vn->x, vn->y, vn->z);
					this->vn.push_back(vn->x);
					this->vn.push_back(vn->y);
					this->vn.push_back(vn->z);
				}
				if (mesh->HasTextureCoords(0)) 
				{
					const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
					//printf ("      vt %i (%f,%f)\n", v_i, vt->x, vt->y);
					this->vt.push_back(vt->x);
					this->vt.push_back(vt->y);
				}
			}

			bbox.findMaxMinValues(mesh);

		}
		printf("\n");
		aiReleaseImport(scene);
		return true;
	}

	std::vector<vec3> getWorldPositions()
	{
		return this->positions;
	}

	void generateTree(const char* mesh_loc)
	{
		bbox.init();
		load_mesh(mesh_loc);
		unsigned int vp_vbo = 0;
		this->loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
		this->loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
		this->loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

		glGenVertexArrays(1, &this->vao);
		glBindVertexArray(this->vao);

		glGenBuffers(1, &vp_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);

		glBufferData(GL_ARRAY_BUFFER, this->point_count * 3 * sizeof(float), &this->vp[0], GL_STATIC_DRAW);
		unsigned int vn_vbo = 0;
		glGenBuffers(1, &vn_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glBufferData(GL_ARRAY_BUFFER, this->point_count * 3 * sizeof(float), &this->vn[0], GL_STATIC_DRAW);

		unsigned int vt_vbo = 0;
		glGenBuffers(1, &vt_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
		glBufferData(GL_ARRAY_BUFFER, this->point_count * 2 * sizeof(float), &this->vt[0], GL_STATIC_DRAW);

		//unsigned int vao = 0;
		glBindVertexArray(this->vao);

		//Points
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		//Normals 
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		//Textures
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

	}

	void loadTexture(const char *filename)
	{
		//GLuint texture;
		unsigned char *data;

		int width, height;
		int n = 0;

		//data = stbi_load(filename, &width, &height, &n, 0);
		data = il.load_image(filename, &width, &height, &n, 0);

		glGenTextures(1, &this->tex);
		glBindTexture(GL_TEXTURE_2D, this->tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glGenerateMipmap(GL_TEXTURE_2D);

		//stbi_image_free(data);
		il.free_image(data);
	}

	void generateRandomPositionCoords()
	{
		for (int i = 0; i < TREE_AMOUNT i++)
		{
			random_device rd;
			mt19937 gen(rd());
			uniform_real_distribution<float> dis(-240, 240);
			float x = dis(gen);
			float z = dis(gen);

			this->posX.push_back(x);
			this->posZ.push_back(z);
		}
	}

	void draw(bool regen)
	{
		int model_location = glGetUniformLocation(shaderProgramID, "model");
		mat4 model = identity_mat4();

		if (regen)
		{
			generateRandomPositionCoords();
		}

		for (int i = 0; i < TREE_AMOUNT i++)
		{
			float xVal = this->posX[i];
			float zVal = this->posZ[i];

			vec3 position = { xVal, 0, zVal };
			positions.push_back(position);

			//mat4 bboxMat = bbox.transform;

			//model = model * bbox.transform; 
			//model = translate(model, {0, 0, 0});
			//model = translate(model, { xVal, -4.5, zVal });
			model = translate(model, position);

			glBindVertexArray(this->vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->tex);
			
			glUniformMatrix4fv(model_location, 1, GL_FALSE, model.m);
			glDrawArrays(GL_TRIANGLES, 0, point_count);
			glBindVertexArray(0);
			bbox.setUp();
			model = identity_mat4();
		}

	}
};

Trees trees;

class Medkits
{
#define MEDKIT_AMOUNT 50;
public:
	std::vector<float> vp, vn, vt;
	int point_count;
	GLuint loc1, loc2, loc3;
	GLuint vao;
	GLuint vbo;

	GLuint tex;

	std::vector<GLfloat> posX;
	std::vector<GLfloat> posZ;
	std::vector<vec3> positions;

	BoundingBox bbox;

	vec3 position;

	Medkits()
	{
		generateRandomPositionCoords();
	}

	bool load_mesh(const char* file_name)
	{
		//Initialize the bounding box
		bbox.init();

		const aiScene* scene = aiImportFile(file_name, aiProcess_Triangulate); // TRIANGLES!
																			   //fprintf (stderr, "ERROR: reading mesh %s\n", file_name);
		if (!scene) {
			fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
			return false;
		}
		printf("  %i animations\n", scene->mNumAnimations);
		printf("  %i cameras\n", scene->mNumCameras);
		printf("  %i lights\n", scene->mNumLights);
		printf("  %i materials\n", scene->mNumMaterials);
		printf("  %i meshes\n", scene->mNumMeshes);
		printf("  %i textures\n", scene->mNumTextures);

		for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
			const aiMesh* mesh = scene->mMeshes[m_i];
			printf("    %i vertices in mesh\n", mesh->mNumVertices);
			this->point_count = mesh->mNumVertices;
			for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
				if (mesh->HasPositions()) {
					const aiVector3D* vp = &(mesh->mVertices[v_i]);
					//printf ("      vp %i (%f,%f,%f)\n", v_i, vp->x, vp->y, vp->z);
					this->vp.push_back(vp->x);
					this->vp.push_back(vp->y);
					this->vp.push_back(vp->z);
				}
				if (mesh->HasNormals()) {
					const aiVector3D* vn = &(mesh->mNormals[v_i]);
					//printf ("      vn %i (%f,%f,%f)\n", v_i, vn->x, vn->y, vn->z);
					this->vn.push_back(vn->x);
					this->vn.push_back(vn->y);
					this->vn.push_back(vn->z);
				}
				if (mesh->HasTextureCoords(0)) {
					const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
					//printf ("      vt %i (%f,%f)\n", v_i, vt->x, vt->y);
					this->vt.push_back(vt->x);
					this->vt.push_back(vt->y);
				}
				if (mesh->HasTangentsAndBitangents()) {
					// NB: could store/print tangents here
				}
			}

			bbox.findMaxMinValues(mesh);
		}



		printf("\n");
		aiReleaseImport(scene);
		return true;
	}

	void generateBuffer(const char* mesh_loc)
	{
		load_mesh(mesh_loc);
		unsigned int vp_vbo = 0;
		this->loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
		this->loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
		this->loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

		glGenVertexArrays(1, &this->vao);
		glBindVertexArray(this->vao);

		glGenBuffers(1, &vp_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);

		glBufferData(GL_ARRAY_BUFFER, this->point_count * 3 * sizeof(float), &this->vp[0], GL_STATIC_DRAW);
		unsigned int vn_vbo = 0;
		glGenBuffers(1, &vn_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glBufferData(GL_ARRAY_BUFFER, this->point_count * 3 * sizeof(float), &this->vn[0], GL_STATIC_DRAW);

		unsigned int vt_vbo = 0;
		glGenBuffers(1, &vt_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
		glBufferData(GL_ARRAY_BUFFER, this->point_count * 2 * sizeof(float), &this->vt[0], GL_STATIC_DRAW);

		//unsigned int vao = 0;
		glBindVertexArray(this->vao);

		//Points
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		//Normals 
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		//Textures
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void loadTexture(const char *filename)
	{
		//GLuint texture;
		unsigned char *data;

		int width, height;
		int n = 0;
		//data = stbi_load(filename, &width, &height, &n, 0);
		data = il.load_image(filename, &width, &height, &n, 0);

		glGenTextures(1, &this->tex);
		glBindTexture(GL_TEXTURE_2D, this->tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glGenerateMipmap(GL_TEXTURE_2D);

		//stbi_image_free(data);
		il.free_image(data);
	}

	void removeKit(int index)
	{
		this->posX.erase(remove(posX.begin(), posX.end(), posX[index]), posX.end());

		this->posZ.erase(remove(posZ.begin(), posZ.end(), posZ[index]), posZ.end());
	}

	void generateRandomPositionCoords()
	{
		for (int i = 0; i < MEDKIT_AMOUNT i++)
		{
			random_device rd;
			mt19937 gen(rd());
			uniform_real_distribution<float> dis(-240, 240);

			float x = dis(gen);
			float z = dis(gen);

			this->posX.push_back(x);
			this->posZ.push_back(z);
		}
	}

	void draw()
	{
		int model_location = glGetUniformLocation(shaderProgramID, "model");
		mat4 model = identity_mat4();
		//I really should use a single vector for these positions. TODO
		for (int i = 0; i < posX.size(); i++)
		{
			float xVal = this->posX[i];
			float zVal = this->posZ[i];

			vec3 position = { xVal, -0.5, zVal };

			model = model * bbox.transform;
			//model = translate(model, {0, 0, 0});
			model = translate(model, position);
			positions.push_back(position);
			//model = model * translate(bboxMat, { xVal, 0, zVal });

			glBindVertexArray(this->vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->tex);

			glUniformMatrix4fv(model_location, 1, GL_FALSE, model.m);
			glDrawArrays(GL_TRIANGLES, 0, point_count);
			glBindVertexArray(0);
			model = identity_mat4();
		}
	}
};

Medkits medkits;

class Skybox
{
public:

	GLuint vao;
	GLuint vbo;

	GLuint texCube;

	float cube_points[108] = {
		-10.0f,  10.0f, -10.0f,
		-10.0f, -10.0f, -10.0f,
		10.0f, -10.0f, -10.0f,
		10.0f, -10.0f, -10.0f,
		10.0f,  10.0f, -10.0f,
		-10.0f,  10.0f, -10.0f,

		-10.0f, -10.0f,  10.0f,
		-10.0f, -10.0f, -10.0f,
		-10.0f,  10.0f, -10.0f,
		-10.0f,  10.0f, -10.0f,
		-10.0f,  10.0f,  10.0f,
		-10.0f, -10.0f,  10.0f,

		10.0f, -10.0f, -10.0f,
		10.0f, -10.0f,  10.0f,
		10.0f,  10.0f,  10.0f,
		10.0f,  10.0f,  10.0f,
		10.0f,  10.0f, -10.0f,
		10.0f, -10.0f, -10.0f,

		-10.0f, -10.0f,  10.0f,
		-10.0f,  10.0f,  10.0f,
		10.0f,  10.0f,  10.0f,
		10.0f,  10.0f,  10.0f,
		10.0f, -10.0f,  10.0f,
		-10.0f, -10.0f,  10.0f,

		-10.0f,  10.0f, -10.0f,
		10.0f,  10.0f, -10.0f,
		10.0f,  10.0f,  10.0f,
		10.0f,  10.0f,  10.0f,
		-10.0f,  10.0f,  10.0f,
		-10.0f,  10.0f, -10.0f,

		-10.0f, -10.0f, -10.0f,
		-10.0f, -10.0f,  10.0f,
		10.0f, -10.0f, -10.0f,
		10.0f, -10.0f, -10.0f,
		-10.0f, -10.0f,  10.0f,
		10.0f, -10.0f,  10.0f
	};

	//Convient helper function which does all of the initialization work for you
	void init()
	{
		createBox();
		createCubeMap(SKY_BACK, SKY_FRONT, SKY_TOP, SKY_BOTTOM, SKY_LEFT,
			SKY_RIGHT, &this->texCube);
	}

	void createBox()
	{
		glGenBuffers(1, &this->vbo);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
		glBufferData(GL_ARRAY_BUFFER, 3 * 36 * sizeof(float), &this->cube_points, GL_STATIC_DRAW);

		glGenVertexArrays(1, &this->vao);
		glBindVertexArray(this->vao);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	bool loadCubeMapSide(GLuint texture, GLenum side_target, const char* filename)
	{
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
		int width, height, n;
		int force_channels = 4;

		//unsigned char* data = stbi_load(filename, &width, &height, &n, force_channels);
		unsigned char* data = il.load_image(filename, &width, &height, &n, force_channels);
		if (!data) {
			fprintf(stderr, "ERROR: could not load %s\n", filename);
			return false;
		}
		glTexImage2D(side_target, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		//stbi_image_free(data);
		il.free_image(data);
		return true;
	}

	void createCubeMap(
		const char* front,
		const char* back,
		const char* top,
		const char* bottom,
		const char* left,
		const char* right,
		GLuint* texCube
		)
	{
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &this->texCube);
		assert(this->loadCubeMapSide(this->texCube, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, front));
		assert(this->loadCubeMapSide(this->texCube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, back));
		assert(this->loadCubeMapSide(this->texCube, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, top));
		assert(this->loadCubeMapSide(this->texCube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, bottom));
		assert(this->loadCubeMapSide(this->texCube, GL_TEXTURE_CUBE_MAP_POSITIVE_X, right));
		assert(this->loadCubeMapSide(this->texCube, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, left));

		// format cube map texture
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void drawSkybox()
	{
		//glDepthMask(GL_FALSE);
		glBindVertexArray(this->vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, this->texCube);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		//glDepthMask(GL_TRUE);
	}
};

Skybox skybox;

class Player
{
#define MAX_HEALTH			100
#define HEALTH_DRAIN_RATE	10
#define HEALTH_GAIN_RATE	20
#define COLLISION_COEFF		3

public:
	Camera cam; //The player is essentially a flying camera
	vec3 worldPosition; //Position of player in the world
	//Mouse vars
	bool fMouse;
	//Last mouse position starts at the screen centre
	GLfloat lastX = 640, lastY = 360;

	//BUG - glut doesn't keep the cursor in the screen, so mouse movement is awkward when windowed

	int health; //Player health
	bool godMode;
	const char* txtOut;
	int healthTxt;
	int gameOverTxt;
	int gameWinTxt;

	Player()
	{
		cam.setWorldPosition(vec3(0.0, 3.0, 5.0));
		this->health = MAX_HEALTH;
		this->godMode = false;
	}

	void hudInit()
	{
		string sTxtOut = to_string(this->health);
		txtOut = sTxtOut.c_str();
		this->healthTxt = add_text(txtOut, -0.95f, -0.8f, 36.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	}

	void gameOver()
	{
		this->gameOverTxt = add_text("GAME OVER", -0.0f, 0.5f, 36.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	}

	void checkWin()
	{
		vec3 pos = getPosition();
		if (-235 >= pos.v[0] >= 235 || -235 >= pos.v[2] >= 235)
		{
			this->gameWinTxt = add_text("YOU HAVE ESCAPED!", -0.0f, 0.5f, 36.0f, 0.0f, 1.0f, 0.0f, 1.0f);
		}
	}

	void hudUpdate()
	{
		string sTxtOut = to_string(this->health);
		txtOut = sTxtOut.c_str();
		update_text(this->healthTxt, txtOut);
	}

	void decreaseHealth()
	{
		if (this->godMode) { return; }
		else
		{
			health -= HEALTH_DRAIN_RATE;
			if (health == 0)
			{
				hudUpdate();
				gameOver();
			}
			else
			{
				hudUpdate();
			}
		}
	}

	void increaseHealth()
	{
		PlaySound(MEDKIT_SOUND, NULL, SND_ASYNC | SND_FILENAME);
		health += HEALTH_GAIN_RATE;
		hudUpdate();
	}

	void drawHUD()
	{
		draw_texts();
	}

	vec3 getPosition()
	{
		return cam.getWorldPosition();
	}

	mat4 getViewMat()
	{
		return cam.GetViewMatrix();
	}

	void setGodMode()
	{
		if (!this->godMode) { this->godMode = true; }
		else { this->godMode = false; }
	}	

	/**
	* Containers facilitating player movement
	*/
	void moveForward()
	{
		//cam.ProcessKeyboard(FORWARD, delta * 4);
		vec3 pos = cam.calculateNextPosition(FORWARD, delta * 4);
		if (detectCollision(TREE, trees.getWorldPositions(), pos))
		{
			cout << "Collision detected!" << endl;
		}
		else if (detectCollision(MEDKIT, medkits.positions, pos))
		{
			increaseHealth();
			cam.ProcessKeyboard(FORWARD, delta * 4);
		}
		else
		{
			checkWin();
			cam.ProcessKeyboard(FORWARD, delta * 4);
		}
	}

	void moveBackwards()
	{
		//cam.ProcessKeyboard(BACKWARD, delta * 4);
		vec3 pos = cam.calculateNextPosition(BACKWARD, delta * 4);
		if (detectCollision(TREE, trees.positions, pos))
		{
			cout << "Collision detected!" << endl;
		}
		else if (detectCollision(MEDKIT, medkits.positions, pos))
		{
			increaseHealth();
			cam.ProcessKeyboard(BACKWARD, delta * 4);
		}
		else
		{
			checkWin();			
			cam.ProcessKeyboard(BACKWARD, delta * 4);
		}
	}

	void strafeLeft()
	{
		//cam.ProcessKeyboard(LEFT, delta * 4);
		vec3 pos = cam.calculateNextPosition(LEFT, delta * 4);
		if (detectCollision(TREE, trees.positions, pos))
		{
			cout << "Collision detected!" << endl;
		}
		else if (detectCollision(MEDKIT, medkits.positions, pos))
		{
			increaseHealth();
			cam.ProcessKeyboard(LEFT, delta * 4);
		}
		else
		{
			checkWin();
			cam.ProcessKeyboard(LEFT, delta * 4);
		}
	}

	void strafeRight()
	{
		//cam.ProcessKeyboard(RIGHT, delta * 4);
		vec3 pos = cam.calculateNextPosition(RIGHT, delta * 4);
		if (detectCollision(TREE, trees.positions, pos))
		{
			cout << "Collision detected!" << endl;
		}
		else if (detectCollision(MEDKIT, medkits.positions, pos))
		{
			increaseHealth();
			cam.ProcessKeyboard(RIGHT, delta * 4);
		}
		else
		{
			checkWin();
			cam.ProcessKeyboard(RIGHT, delta * 4);
		}
	}

	void look(int x, int y)
	{
		if (fMouse)
		{
			lastX = x;
			lastY = y;
			fMouse = false;
		}

		GLfloat xOff = x - lastX;
		GLfloat yOff = lastY - y;

		lastX = x;
		lastY = y;

		cam.ProcessMouseMovement(xOff, yOff);
	}

	float calcDistance(float pX, float pZ, float oX, float oZ)
	{
		float distX = oX - pX;
		float distZ = oZ - pZ;

		float dist = sqrt((pow(distX, 2) + pow(distZ, 2)));
		return dist;
	}

	void printPos()	//Debug Information
	{
		vec3 pos = this->getPosition();
		vec3 pos2 = cam.calculateNextPosition(FORWARD, delta * 4);
		std::vector<vec3> treePos = trees.getWorldPositions();

		float treePosX = roundf(treePos[0].v[0] * 100) / 100;
		float treePosZ = roundf(treePos[0].v[2] * 100) / 100;

		float playerPosX = roundf(pos.v[0] * 100) / 100;
		float playerPosZ = roundf(pos.v[2] * 100) / 100;

		float playerPosX2 = roundf(pos2.v[0] * 100) / 100;
		float playerPosZ2 = roundf(pos2.v[2] * 100) / 100;


		float top = treePosZ + COLLISION_COEFF;
		float bottom = treePosZ - COLLISION_COEFF;
		float left = treePosX - COLLISION_COEFF;
		float right = treePosX + COLLISION_COEFF;

		float distance = calcDistance(playerPosX, playerPosZ, treePosX, treePosZ);

		cout << "-------------------------------------------------------------------------------" << endl;
		cout << "X: " << treePosX << "    "  << "Y: " << treePos[0].v[1] << "    "  << "Z: " << treePosZ << endl;
		cout << endl;
		cout << "ACTUAL POSITION" << endl;
		cout << "PLAYER X: " << playerPosX << "    "  << "PLAYER Y: " << pos.v[1] << "    "  << "PLAYER Z: " << playerPosZ << endl;
		cout << "PREDICTED POSITION" << endl;
		cout << "PLAYER X: " << playerPosX2 << "    " << "PLAYER Y: " << pos.v[1] << "    " << "PLAYER Z: " << playerPosZ2 << endl;
		cout << endl;

		cout << "DISTANCE: " << distance << endl;
		cout << endl;
	}

	bool detectCollision(ObjectType type, std::vector<vec3> objectPositions, vec3 playerPos)
	{
		float playerX = playerPos.v[0];
		float playerZ = playerPos.v[2];

		for (int i = 0; i < objectPositions.size(); i++)
		{
			float objX = objectPositions[i].v[0];
			float objZ = objectPositions[i].v[2];

			float dist = calcDistance(playerX, playerZ, objX, objZ);

			if(dist <= COLLISION_COEFF)
			{
				if (type == MEDKIT)
				{
					//Remove medkit from the screen
					medkits.removeKit(i);
				}
				return true;
			}
		}
		return false;
	}
};

Player player;

/*
 *-------------------------------------------------
 *------------------------------------------------
 *-------------------------------------------------
 */


// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {   
    FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

    if ( fp == NULL ) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';

    fclose(fp);

    return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
	const char* pShaderSource = readShaderSource( pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
	// check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
	        exit(1);
    }
	// Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders(const char* vertex, const char* frag)
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

	// Create two shader objects, one for the vertex, and one for the fragment shader
    //AddShader(shaderProgramID, "../Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
    //AddShader(shaderProgramID, "../Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

	AddShader(shaderProgramID, vertex, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, frag, GL_FRAGMENT_SHADER);


    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
	        exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    //glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

void drawSkybox(mat4 view, mat4 persp_proj)
{
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glUseProgram(skyboxShaderID);
	view = player.getViewMat();
	int sky_view_mat_location = glGetUniformLocation(skyboxShaderID, "view");
	int sky_proj_mat_location = glGetUniformLocation(skyboxShaderID, "proj");
	glUniformMatrix4fv(sky_proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(sky_view_mat_location, 1, GL_FALSE, view.m);
	skybox.drawSkybox();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
}

#pragma endregion VBO_FUNCTIONS

int frame_count = 0;
void display()
{
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram (shaderProgramID);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation (shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation (shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation (shaderProgramID, "proj");

	mat4 view = player.getViewMat();
	mat4 persp_proj = perspective(80.0, (float)width/(float)height, 0.1, 100.0);
	mat4 model = identity_mat4 ();

	// update uniforms & draw
	glUniformMatrix4fv (proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv (view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv (matrix_location, 1, GL_FALSE, model.m);

	
	//Decrease health every two seconds
	//Linking anything to frame rate is an awful idea - take a look at Fallout 4's physics
	//However, it should serve its purpose here since the fps here should never go below 60
	//if (frame_count == 120)

	//Turns out it runs at 30 fps, even on an 860M - lol
	//Somehow I can run Battlefield 4 on High/Ultra at 60fps, but can't get above 30 here
	//Unless it's capped, of course
	if (frame_count == 60)
	{
		player.decreaseHealth();
		frame_count = 0;
	}
	frame_count++;
	
	terrain.draw();
	trees.draw(false);
	medkits.draw();
	player.drawHUD();

	//drawSkybox(view, persp_proj);

    glutSwapBuffers();
}


void updateScene() {	

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	// rotate the model slowly around the y axis
	//rotate_y+=0.2f;
	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders(VERTEX_SHADER_LOCATION, FRAGMENT_SHADER_LOCATION);
	//skyboxShaderID = CompileShaders(SKYBOX_VERTEX_SHADER_LOC, SKYBOX_FRAG_SHADER_LOC);
	skybox.init();

	//Initialize text rendering
	init_text_rendering(TEXT_FONT, TEXT_META, width, height);

	player.hudInit();

	terrain.loadTexture(GRASS_TERRAIN_TEX);
	terrain.generateBuffer(GRASS_TERRAIN_MESH);

	trees.loadTexture(TREE_LEAVES_TEX);
	trees.generateTree(TREE_MESH);

	medkits.loadTexture(MEDKIT_TEX);
	medkits.generateBuffer(MEDKIT_MESH);

	PlaySound(AMBIENT_SOUND, NULL, SND_ASYNC | SND_LOOP);
}

void keypress(unsigned char key, int x, int y) 
{

	if(key=='w')
	{
		player.moveForward();
	}
	if (key == 's')
	{
		player.moveBackwards();
	}
	if (key == 'd')
	{ 
		player.strafeRight();
	}
	if (key == 'a') 
	{
		player.strafeLeft();
	}
	if (key == 'p')
	{
		player.printPos();
	}
}

void handleMouseMove(int x, int y)
{
	player.look(x, y);
}

int main(int argc, char** argv){

	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("The Zone");

	glutSetCursor(GLUT_CURSOR_NONE);
	glutWarpPointer(width / 2, height / 2);

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);
	glutPassiveMotionFunc(handleMouseMove);
	const char* str = (const char*) glGetString(GL_VENDOR);
	printf(str);
	cout << endl;

	// A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}
