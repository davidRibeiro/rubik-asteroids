#include "StopWatch.h"
#ifdef _WIN32
#include <GL/glew.h>
#else
#define GLFW_INCLUDE_GLCOREARB
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <FreeImage.h>
#include "objloader.h"

#define PI		3.14

bool inited = false; //Have we done initialization?
float colD;
bool asteroidsCamera = false;
bool auxBool = true;
CStopWatch auxClock;

GLuint vaoArwing, vaoWing, vaoRubikCube, vaoSphere, vaoLaser, vaoUniverse;
GLuint geomIdArwing, geomIdWing, geomIdRubikCube, geomIdSphere, geomIdLaser, geomIdUniverse;

GLuint indicesId;
GLuint texUVId;
GLuint normalsId;
GLuint baseTextureArwing, baseTextureRubik, baseTextureLaser, baseTextureUniverse;
GLuint particleTexture1, particleTexture2;

CStopWatch wingTime;
CStopWatch spawnTime;
CStopWatch keysTime;

GLuint vertexShaderId;
GLuint fragShaderId;
GLuint programId;

glm::mat4 perspectiveMatrix;
glm::mat4 cameraMatrix;

glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
glm::vec3 cameraView(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

glm::vec3 arwingPos(0.0f, 0.0f, 0.0f);
bool inCurve = false;
CStopWatch curveTime;
CStopWatch colTime;
float angle = 0.0f;

const float velocity = 0.025f;

GLfloat lightDir[] = {-1.0f, -1.0f, -1.0};
GLfloat lightIntensity[] = {0.9f, 0.9f, 0.9f, 1.0f};

GLfloat ambientComponent[] = {0.4f, 0.4f, 0.4f, 1.0f};
GLfloat diffuseColor[] = {1.0f, 1.0f, 1.0f, 1.0f};

OBJLoader arwing("../models/arwingship.obj");
OBJLoader leftWing("../models/arwingLeftWing.obj");
OBJLoader rubik("../models/rubik.obj");
OBJLoader sphere("../models/sphere.obj");
OBJLoader laser("../models/laser.obj");
OBJLoader universe("../models/universe.obj");

typedef struct{
	bool isItAlive;
	float lifetime;
	float decay;
	glm::vec3 pos;
	glm::vec3 velocity;
	GLuint texture;
}Particle;

const int nparticles = 400;
Particle arenaParticles[nparticles];

typedef struct{
	glm::vec3 rotAxis;
	bool isItAlive;
	glm::vec3 velocity;
	float rot;
	CStopWatch time;
	glm::vec3 currentPos;
}Asteroid;

const int maxCubes = 26;
Asteroid asteroids[maxCubes];


typedef struct{
	glm::vec3 direction;
	glm::vec3 spawnPos;
	CStopWatch bulletTime;
	glm::vec3 pos;
	bool itLives;
	float angle;
}Laser;
const int maxLasers = 30;
Laser firingLasers[maxLasers];

glm::vec3 curve[4];
float t;
int cAngle;

void blockKeys(){
	keysTime.Reset();
}
void restart(){
	arwingPos=glm::vec3(0.0);
	curveTime.Reset();
	spawnTime.Reset();
	angle=0.0f;
	for(int i=0; i<maxLasers; i++){
		firingLasers[i].itLives=false;
	}
	for(int j=0; j<maxCubes; j++){
		asteroids[j].isItAlive=false;
	}
}

void checkError(const char *functionName)
{
    GLenum error;
    while (( error = glGetError() ) != GL_NO_ERROR) {
        fprintf (stderr, "GL error 0x%X detected in %s\n", error, functionName);
    }
}

void dumpInfo(void)
{
    printf ("Vendor: %s\n", glGetString (GL_VENDOR));
    printf ("Renderer: %s\n", glGetString (GL_RENDERER));
    printf ("Version: %s\n", glGetString (GL_VERSION));
    printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));
    checkError ("dumpInfo");
}

void createAndCompileProgram(GLuint vertexId, GLuint fragId, GLuint *programId)
{
	*programId = glCreateProgram(); //1.
	glAttachShader(*programId, vertexId); //2. Attach the shader vertexId to the program programId
	glAttachShader(*programId, fragId); //2. Attach the shader fragId to the program programId
    
	glLinkProgram (*programId);//4.
    
	//5. Until the end of the if clause, is to check for COMPILE errors, and only for these. *not* related with the checkError procedure
	GLint status;
	glGetProgramiv (*programId, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLint infoLogLength;
		char *infoLog;
		glGetProgramiv (*programId, GL_INFO_LOG_LENGTH, &infoLogLength);
		infoLog = new char[infoLogLength];
		glGetProgramInfoLog (*programId, infoLogLength, NULL, infoLog);
		fprintf (stderr, "link log: %s\n", infoLog);
		delete infoLog;
	}
	checkError ("createAndCompileProgram");
}

void createAndCompileShader(GLuint* id, GLenum type, GLsizei count, const char **shaderSource)
{
	*id = glCreateShader(type); //1. create the shader with type. (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER. others will follow in the future)
    
	glShaderSource(*id, count, shaderSource, NULL); //2. the shader's source. *id: shader's id; count: memory size of the contents; shaderSource: shader contents; NULL ;)
	
	glCompileShader(*id); //3.
    
	//4. Until the end of the if clause, is to check for COMPILE errors, and only for these. *not* related with the checkError procedure
	GLint status;
	glGetShaderiv (*id, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint infoLogLength;
		char *infoLog;
		glGetShaderiv (*id, GL_INFO_LOG_LENGTH, &infoLogLength);
		infoLog = new char[infoLogLength];
		glGetShaderInfoLog (*id, infoLogLength, NULL, infoLog);
		fprintf (stderr, "compile log: %s\n", infoLog);
		delete(infoLog);
	}
	
	checkError ("createAndCompileShader");
    
}

void loadShader (char *file, GLuint *id, GLenum type)
{
	//Open e load  shader file
	FILE* f = fopen(file, "r");
	if (!f) {
		fprintf(stderr, "Unable to open shader file: %s", file);
		return;
	}
    
    
    
	int fileSize = 0;
    while(!feof(f))
    {
        fgetc(f);
        ++fileSize;
    }
	rewind(f);
    
	char *fileContents = new char[fileSize];
	memset(fileContents, 0, fileSize);
	fread(fileContents, sizeof(char), fileSize, f);
	//up until here is to load the contents of the file
    
	const char* t = fileContents;
	
	//2. create the shader. arguments (future) shader id, type of shader, memory size of the file contents, file contents
	createAndCompileShader(id, type, sizeof(fileContents)/sizeof(char*), &t);
    
	//cleanup
	fclose(f);
	delete fileContents;
}

void loadShaderProgram(char* vertexFile, char* fragmentFile)
{
	//load each shader seperately. arguments: the file, (future) shader id, shader type
	
	loadShader(vertexFile, &vertexShaderId, GL_VERTEX_SHADER);
	loadShader(fragmentFile, &fragShaderId, GL_FRAGMENT_SHADER);
	
	//one the shaders loaded, create the program with the two shaders. arguments: vertex shader id, fragment shader id, (future) program id
	createAndCompileProgram(vertexShaderId, fragShaderId, &programId);
}

void setupPerspective(int width, int height)
{
	glViewport(0, 0, width, height);
    
	float aspect = (float)width/(float)height; //aspect ratio.
    
	perspectiveMatrix = glm::perspective(45.0f, aspect, 1.0f, 1000.0f); //field of view; aspect ratio; z near; z far;
}

void initGeometryArwing(){
	const float *vertices = arwing.getVerticesArray();
	const float *textureCoords = arwing.getTextureCoordinatesArray();
	const float *normals = arwing.getNormalsArray();
	const unsigned int *indices = arwing.getIndicesArray();
    
	glGenVertexArrays(1, &vaoArwing); //1.
	glBindVertexArray(vaoArwing); //2.
    
	glEnableVertexAttribArray(0); //3.
	glGenBuffers(1, &geomIdArwing); //4.
	glBindBuffer(GL_ARRAY_BUFFER, geomIdArwing); //5.
	glBufferData(GL_ARRAY_BUFFER, arwing.getNVertices() * 3 * sizeof(float), vertices, GL_STATIC_DRAW); //6. GL_ARRAY_BUFFER: the type of buffer; sizeof(vertices): the memory size; vertices: the pointer to data; GL_STATIC_DRAW: data will remain on the graphics card's memory and will not be changed
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); //7. 0: the *ATTRIBUTE* number; 3: the number of coordinates; GL_FLOAT: the type of data; GL_FALSE: is the data normalized? (usually it isn't), 0: stride (forget for now); 0: data position (forget for now)
    
	glEnableVertexAttribArray(1);
	glGenBuffers(1, &texUVId);
	glBindBuffer(GL_ARRAY_BUFFER, texUVId);
	glBufferData(GL_ARRAY_BUFFER, arwing.getNVertices() * 2 * sizeof(float), textureCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    

	glEnableVertexAttribArray(2);
	glGenBuffers(1, &normalsId);
	glBindBuffer(GL_ARRAY_BUFFER, normalsId);
	glBufferData(GL_ARRAY_BUFFER, arwing.getNVertices() * 3 * sizeof(float), normals, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    
	glGenBuffers(1, &indicesId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, arwing.getNIndices() * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    
	checkError("initBuffer");
	glBindBuffer(GL_ARRAY_BUFFER, 0); //9.
	glBindVertexArray(0); //9.
    

}
void initGeometryLeftWing(){
	const float *vertices = leftWing.getVerticesArray();
	const float *textureCoords = leftWing.getTextureCoordinatesArray();
	const float *normals = leftWing.getNormalsArray();
	const unsigned int *indices = leftWing.getIndicesArray();
    
	glGenVertexArrays(1, &vaoWing); //1.
	glBindVertexArray(vaoWing); //2.
    
	glEnableVertexAttribArray(0); //3.
	glGenBuffers(1, &geomIdWing); //4.
	glBindBuffer(GL_ARRAY_BUFFER, geomIdWing); //5.
	glBufferData(GL_ARRAY_BUFFER, leftWing.getNVertices() * 3 * sizeof(float), vertices, GL_STATIC_DRAW); //6. GL_ARRAY_BUFFER: the type of buffer; sizeof(vertices): the memory size; vertices: the pointer to data; GL_STATIC_DRAW: data will remain on the graphics card's memory and will not be changed
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); //7. 0: the *ATTRIBUTE* number; 3: the number of coordinates; GL_FLOAT: the type of data; GL_FALSE: is the data normalized? (usually it isn't), 0: stride (forget for now); 0: data position (forget for now)
    
	glEnableVertexAttribArray(1);
	glGenBuffers(1, &texUVId);
	glBindBuffer(GL_ARRAY_BUFFER, texUVId);
	glBufferData(GL_ARRAY_BUFFER, leftWing.getNVertices() * 2 * sizeof(float), textureCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    

	glEnableVertexAttribArray(2);
	glGenBuffers(1, &normalsId);
	glBindBuffer(GL_ARRAY_BUFFER, normalsId);
	glBufferData(GL_ARRAY_BUFFER, leftWing.getNVertices() * 3 * sizeof(float), normals, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    
	glGenBuffers(1, &indicesId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, leftWing.getNIndices() * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    
	checkError("initBuffer");
	glBindBuffer(GL_ARRAY_BUFFER, 0); //9.
	glBindVertexArray(0); //9.
    

}
void initGeometryRubikCube(){
	const float *vertices = rubik.getVerticesArray();
	const float *textureCoords = rubik.getTextureCoordinatesArray();
	const float *normals = rubik.getNormalsArray();
	const unsigned int *indices = rubik.getIndicesArray();
    
	glGenVertexArrays(1, &vaoRubikCube); //1.
	glBindVertexArray(vaoRubikCube); //2.
    
	glEnableVertexAttribArray(0); //3.
	glGenBuffers(1, &geomIdRubikCube); //4.
	glBindBuffer(GL_ARRAY_BUFFER, geomIdRubikCube); //5.
	glBufferData(GL_ARRAY_BUFFER, rubik.getNVertices() * 3 * sizeof(float), vertices, GL_STATIC_DRAW); //6. GL_ARRAY_BUFFER: the type of buffer; sizeof(vertices): the memory size; vertices: the pointer to data; GL_STATIC_DRAW: data will remain on the graphics card's memory and will not be changed
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); //7. 0: the *ATTRIBUTE* number; 3: the number of coordinates; GL_FLOAT: the type of data; GL_FALSE: is the data normalized? (usually it isn't), 0: stride (forget for now); 0: data position (forget for now)
    
	glEnableVertexAttribArray(1);
	glGenBuffers(1, &texUVId);
	glBindBuffer(GL_ARRAY_BUFFER, texUVId);
	glBufferData(GL_ARRAY_BUFFER, rubik.getNVertices() * 2 * sizeof(float), textureCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    

	glEnableVertexAttribArray(2);
	glGenBuffers(1, &normalsId);
	glBindBuffer(GL_ARRAY_BUFFER, normalsId);
	glBufferData(GL_ARRAY_BUFFER, rubik.getNVertices() * 3 * sizeof(float), normals, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    
	glGenBuffers(1, &indicesId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, rubik.getNIndices() * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    
	checkError("initBuffer");
	glBindBuffer(GL_ARRAY_BUFFER, 0); //9.
	glBindVertexArray(0); //9.
    

}
void initGeometrySphere(){
	const float *vertices = sphere.getVerticesArray();
	const float *textureCoords = sphere.getTextureCoordinatesArray();
	const float *normals = sphere.getNormalsArray();
	const unsigned int *indices = sphere.getIndicesArray();
    
	glGenVertexArrays(1, &vaoSphere); //1.
	glBindVertexArray(vaoSphere); //2.
    
	glEnableVertexAttribArray(0); //3.
	glGenBuffers(1, &geomIdSphere); //4.
	glBindBuffer(GL_ARRAY_BUFFER, geomIdSphere); //5.
	glBufferData(GL_ARRAY_BUFFER, sphere.getNVertices() * 3 * sizeof(float), vertices, GL_STATIC_DRAW); //6. GL_ARRAY_BUFFER: the type of buffer; sizeof(vertices): the memory size; vertices: the pointer to data; GL_STATIC_DRAW: data will remain on the graphics card's memory and will not be changed
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); //7. 0: the *ATTRIBUTE* number; 3: the number of coordinates; GL_FLOAT: the type of data; GL_FALSE: is the data normalized? (usually it isn't), 0: stride (forget for now); 0: data position (forget for now)
    
	glEnableVertexAttribArray(1);
	glGenBuffers(1, &texUVId);
	glBindBuffer(GL_ARRAY_BUFFER, texUVId);
	glBufferData(GL_ARRAY_BUFFER, sphere.getNVertices() * 2 * sizeof(float), textureCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
	/*
     !![NEW]!!
     
     Normals information for lighting
     */
    
	glEnableVertexAttribArray(2);
	glGenBuffers(1, &normalsId);
	glBindBuffer(GL_ARRAY_BUFFER, normalsId);
	glBufferData(GL_ARRAY_BUFFER, sphere.getNVertices() * 3 * sizeof(float), normals, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
	glGenBuffers(1, &indicesId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere.getNIndices() * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    
	checkError("initBuffer");
	glBindBuffer(GL_ARRAY_BUFFER, 0); //9.
	glBindVertexArray(0); //9.
 
}
void initGeometryLaser(){
	const float *vertices = laser.getVerticesArray();
	const float *textureCoords = laser.getTextureCoordinatesArray();
	const float *normals = laser.getNormalsArray();
	const unsigned int *indices = laser.getIndicesArray();
    
	glGenVertexArrays(1, &vaoLaser); //1.
	glBindVertexArray(vaoLaser); //2.
    
	glEnableVertexAttribArray(0); //3.
	glGenBuffers(1, &geomIdLaser); //4.
	glBindBuffer(GL_ARRAY_BUFFER, geomIdLaser); //5.
	glBufferData(GL_ARRAY_BUFFER, laser.getNVertices() * 3 * sizeof(float), vertices, GL_STATIC_DRAW); //6. GL_ARRAY_BUFFER: the type of buffer; sizeof(vertices): the memory size; vertices: the pointer to data; GL_STATIC_DRAW: data will remain on the graphics card's memory and will not be changed
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); //7. 0: the *ATTRIBUTE* number; 3: the number of coordinates; GL_FLOAT: the type of data; GL_FALSE: is the data normalized? (usually it isn't), 0: stride (forget for now); 0: data position (forget for now)
    
	glEnableVertexAttribArray(1);
	glGenBuffers(1, &texUVId);
	glBindBuffer(GL_ARRAY_BUFFER, texUVId);
	glBufferData(GL_ARRAY_BUFFER, laser.getNVertices() * 2 * sizeof(float), textureCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
	/*
     !![NEW]!!
     
     Normals information for lighting
     */
    
	glEnableVertexAttribArray(2);
	glGenBuffers(1, &normalsId);
	glBindBuffer(GL_ARRAY_BUFFER, normalsId);
	glBufferData(GL_ARRAY_BUFFER, laser.getNVertices() * 3 * sizeof(float), normals, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
	glGenBuffers(1, &indicesId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, laser.getNIndices() * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    
	checkError("initBuffer");
	glBindBuffer(GL_ARRAY_BUFFER, 0); //9.
	glBindVertexArray(0); //9.
 
}
void initGeometryUniverse(){
	const float *vertices = universe.getVerticesArray();
	const float *textureCoords = universe.getTextureCoordinatesArray();
	const float *normals = universe.getNormalsArray();
	const unsigned int *indices = universe.getIndicesArray();
    
	glGenVertexArrays(1, &vaoUniverse); //1.
	glBindVertexArray(vaoUniverse); //2.
    
	glEnableVertexAttribArray(0); //3.
	glGenBuffers(1, &geomIdUniverse); //4.
	glBindBuffer(GL_ARRAY_BUFFER, geomIdUniverse); //5.
	glBufferData(GL_ARRAY_BUFFER, universe.getNVertices() * 3 * sizeof(float), vertices, GL_STATIC_DRAW); //6. GL_ARRAY_BUFFER: the type of buffer; sizeof(vertices): the memory size; vertices: the pointer to data; GL_STATIC_DRAW: data will remain on the graphics card's memory and will not be changed
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); //7. 0: the *ATTRIBUTE* number; 3: the number of coordinates; GL_FLOAT: the type of data; GL_FALSE: is the data normalized? (usually it isn't), 0: stride (forget for now); 0: data position (forget for now)
    
	glEnableVertexAttribArray(1);
	glGenBuffers(1, &texUVId);
	glBindBuffer(GL_ARRAY_BUFFER, texUVId);
	glBufferData(GL_ARRAY_BUFFER, universe.getNVertices() * 2 * sizeof(float), textureCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
	glEnableVertexAttribArray(2);
	glGenBuffers(1, &normalsId);
	glBindBuffer(GL_ARRAY_BUFFER, normalsId);
	glBufferData(GL_ARRAY_BUFFER, universe.getNVertices() * 3 * sizeof(float), normals, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
	glGenBuffers(1, &indicesId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, universe.getNIndices() * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    
	checkError("initBuffer");
	glBindBuffer(GL_ARRAY_BUFFER, 0); //9.
	glBindVertexArray(0); //9.
 
}
void initGeometry(){
	initGeometryArwing();
	initGeometryLeftWing();
	initGeometryRubikCube();
	initGeometrySphere();
	initGeometryLaser();
	initGeometryUniverse();
}

GLuint loadTexture(char* textureFile)
{
	GLuint tId;
    
	FIBITMAP *tf = FreeImage_Load(FIF_JPEG, textureFile);
	if (tf) {
        
		fprintf(stderr, "Texture: %s loaded\n", textureFile);
        
		glGenTextures(1, &tId);
		glBindTexture(GL_TEXTURE_2D, tId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FreeImage_GetWidth(tf), FreeImage_GetHeight(tf), 0, GL_BGR, GL_UNSIGNED_BYTE, FreeImage_GetBits(tf));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        
		checkError("loadTexture");
		glBindTexture(GL_TEXTURE_2D, 0);
        
		FreeImage_Unload(tf);
        
		return tId;
	}
	return 0;
}

void init(void)
{
#ifdef _WIN32
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
    } else {
        if (GLEW_VERSION_3_3)
        {
            fprintf(stderr, "Driver supports OpenGL 3.3\n");
        }
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif
    
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //Defines the clear color, i.e., the color used to wipe the display
	glEnable(GL_DEPTH_TEST);
    
    dumpInfo();
    
	FreeImage_Initialise();
	baseTextureArwing = loadTexture("../models/textures/arwing.jpg");
	baseTextureRubik = loadTexture("../models/textures/rubiktexture.jpg");
	baseTextureUniverse = loadTexture("../models/textures/spheremapgalaxyasteroid.jpg");
	baseTextureLaser = loadTexture("../models/textures/lasertex.jpg");
	particleTexture1 = loadTexture("../models/textures/metal2.jpg");
	particleTexture2 = loadTexture("../models/textures/SunTexture_2048.jpg");


	arwing.init();
	leftWing.init();
	rubik.init();
    sphere.init();
	laser.init();
	universe.init();
	initGeometry();
    
	loadShaderProgram("../shaders/vertex_shader.vs", "../shaders/frag_shader.fs");
    
    setupPerspective(640, 480);
    
	checkError("init");
}

void createParticle(int i){
	arenaParticles[i].isItAlive=true;
	arenaParticles[i].lifetime=rand()%1000+1;
	arenaParticles[i].decay=rand()%10 +1;
	if(i%4==0){
		arenaParticles[i].pos.x=75.0f;
		arenaParticles[i].pos.z=(rand()%150)-75;
		arenaParticles[i].velocity.z=(float) ((rand()%10)/(float)100);
		arenaParticles[i].velocity.x=0.0f;
	}
	else if(i%4==1){
		arenaParticles[i].pos.x=-75.0f;
		arenaParticles[i].pos.z=(rand()%150)-75;
		arenaParticles[i].velocity.z=(float) ((rand()%10)/(float)100);
		arenaParticles[i].velocity.x=0.0f;
	}
	else if(i%4==2){
		arenaParticles[i].pos.z=75.0f;
		arenaParticles[i].pos.x=(rand()%150)-75;
		arenaParticles[i].velocity.x=(float) ((rand()%10)/(float)100);
		arenaParticles[i].velocity.z=0.0f;
	}
	else if(i%4==3){
		arenaParticles[i].pos.z=-75.0f;
		arenaParticles[i].pos.x=(rand()%150)-75;
		arenaParticles[i].velocity.x=(float) ((rand()%10)/(float)100);
		arenaParticles[i].velocity.z=0.0f;
	}
	arenaParticles[i].pos.y=(rand()%20)-10;
	arenaParticles[i].velocity.y=(float) ((rand()%10)/(float)100);
	if(rand()%2) arenaParticles[i].texture=particleTexture1;
	else arenaParticles[i].texture=particleTexture2;
}

void evolveParticles(){
	for(int i=0; i<nparticles; i++){
		arenaParticles[i].pos+=arenaParticles[i].velocity;
		if(arenaParticles[i].pos.y>=10 || arenaParticles[i].pos.y<=-10) arenaParticles[i].velocity.y*=-1;
		if(arenaParticles[i].velocity.z!=0 && (arenaParticles[i].pos.z<=-75 || arenaParticles[i].pos.z>=75)) arenaParticles[i].velocity.z*=-1;
		if(arenaParticles[i].velocity.x!=0 && (arenaParticles[i].pos.x<=-75 || arenaParticles[i].pos.x>=75)) arenaParticles[i].velocity.x*=-1;
		arenaParticles[i].lifetime-=arenaParticles[i].decay;
	}
}

void drawParticles(){
	for(int i=0; i<nparticles; i++){
		if(arenaParticles[i].isItAlive && arenaParticles[i].lifetime>0){
				glUseProgram(programId);
				glBindVertexArray(vaoSphere); 
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, arenaParticles[i].texture);

				GLuint loc = glGetUniformLocation(programId, "pMatrix");
				glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&perspectiveMatrix[0]);
				loc = glGetUniformLocation(programId, "vMatrix");
				glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&cameraMatrix[0]);
		
				loc = glGetUniformLocation(programId, "mMatrix");
	
				glm::mat4 translate = glm::translate(glm::mat4(1.0), arenaParticles[i].pos);
				glm::mat4 scale = glm::scale(glm::mat4(1.0), glm::vec3(0.2f));
				glm::mat4 model = translate*scale;
				glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&model[0]);


				glDrawElements(GL_TRIANGLES, sphere.getNIndices(), GL_UNSIGNED_INT, 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, 0);
				glBindVertexArray(0);
				glUseProgram(0); 			
		}
		else{
			createParticle(i);
		}
	}
	evolveParticles();

}

void checkArwingPos(){
	if(arwingPos.x>=75.0f  && !inCurve){
		curve[0]=arwingPos;
		curve[1]=glm::vec3(150.0f, 0.0f, arwingPos.z+100.0f);
		curve[2]=glm::vec3(150.0f, 0.0f, arwingPos.z-100.0f);
		curve[3]=arwingPos-glm::vec3 (1.0f, 0.0f, 0.0f);
		inCurve=true;
		curveTime.Reset();
		cAngle=-90;
	}
	if(arwingPos.x<=-75.0f && !inCurve){
		curve[0]=arwingPos;
		curve[1]=glm::vec3(-150.0f, 0.0f, arwingPos.z+100.0f);
		curve[2]=glm::vec3(-150.0f, 0.0f, arwingPos.z-100.0f);
		curve[3]=arwingPos+glm::vec3(1.0f, 0.0f, 0.0f);
		inCurve=true;
		curveTime.Reset();
		cAngle=0;
	}
	if(arwingPos.z>=75.0f && !inCurve){
		curve[0]=arwingPos;
		curve[1]=glm::vec3(arwingPos.x+100.0f, 0.0f, 150.0f);
		curve[2]=glm::vec3(arwingPos.x-100.0f, 0.0f, 150.0f);
		curve[3]=arwingPos-glm::vec3(0.0f, 0.0f, 1.0f);
		inCurve=true;
		curveTime.Reset();
		cAngle=-180;
	}
	if(arwingPos.z<=-75.0f && !inCurve){
		curve[0]=arwingPos;
		curve[1]=glm::vec3(arwingPos.x+100.0f, 0.0f, -150.0f);
		curve[2]=glm::vec3(arwingPos.x-100.0f, 0.0f, -150.0f);
		curve[3]=arwingPos+glm::vec3(0.0f, 0.0f, 1.0f);
		inCurve=true;
		curveTime.Reset();
		cAngle=0;
	}
}

int fact(int n){
	if(n==1 || n==0) return 1;
	return n*fact(n-1);
}

glm::vec3 bezierCurve(float t){
	glm::vec3 aux = glm::vec3(0.0f);
	int ncp=4;
	for(int i=0; i<ncp; i++){
		float aux0 = (float) (fact(ncp-1) /(fact(i)*fact(ncp-1-i)));
		float aux1 = (float) (pow ( (1-t), (ncp-i-1)));
		float aux2 = (float) (pow (t, i));
		aux+=aux0*aux1*aux2*curve[i];
	}
	return aux;
}

glm::vec3 getArwingPosition(){
	if(inCurve){
		t= curveTime.GetElapsedSeconds()/(float) 5;
		if(t<=1.0f){
			arwingPos = bezierCurve(t);
		}
		else{
			inCurve=false;
			angle+=PI;
		}
	}
	return arwingPos;
}

float getArwingRotation(){
	if(inCurve) return (float) (curveTime.GetElapsedSeconds() * 36)+cAngle;
	return -(angle*360) / (3.14*2)-90;
}

void drawArwing(){
	glUseProgram(programId); //1.
	glBindVertexArray(vaoArwing); //2.
    
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, baseTextureArwing);
    
	GLuint loc = glGetUniformLocation(programId, "tex");
	glUniform1i(loc, 0); //ATTENTION:  0 corresponds to GL_TEXTURE0
    
	loc = glGetUniformLocation(programId, "pMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&perspectiveMatrix[0]);
	loc = glGetUniformLocation(programId, "vMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&cameraMatrix[0]);
	loc = glGetUniformLocation(programId, "mMatrix");
    
	

	glm::mat4 translate = glm::translate(glm::mat4(1.0), getArwingPosition());
	glm::mat4 rotate = glm::rotate(glm::mat4(1.0), getArwingRotation(), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 scale = glm::scale(glm::mat4(1.0), glm::vec3(0.1f));
	glm::mat4 model = translate * rotate * scale;
    
	glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&model[0]);
 
    
	glm::mat4 cameraModelMatrix = cameraMatrix * model;
	
	loc = glGetUniformLocation(programId, "nMatrix");
	glm::mat3 normalMatrix = glm::mat3(cameraMatrix);
	glUniformMatrix3fv(loc, 1, GL_FALSE, (GLfloat *)&normalMatrix[0]);

    
	loc = glGetUniformLocation(programId, "lightDir");
	glm::vec4 transformedLightDir = cameraMatrix * glm::vec4(lightDir[0], lightDir[1], lightDir[2], 0.0f);
	glUniform3fv(loc, 1, (GLfloat *)&transformedLightDir[0]);
    
    
	loc = glGetUniformLocation(programId, "lightIntensity");
	glUniform4fv(loc, 1, lightIntensity);
    
    
	loc = glGetUniformLocation(programId, "ambientIntensity");
	glUniform4fv(loc, 1, ambientComponent);
    
    
	loc = glGetUniformLocation(programId, "diffuseColor");
	glUniform4fv(loc, 1, diffuseColor);
    
	glDrawElements(GL_TRIANGLES, arwing.getNIndices(), GL_UNSIGNED_INT, 0); //type of geometry; number of indices; type of indices array, indices pointer
    
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0); //4.
	glUseProgram(0); //4.
}

void evolveArwingPos(){
	arwingPos.x += cos(angle) * velocity*2;
	arwingPos.z += sin(angle) * velocity*2;
}

float rotateWing(bool wing){
	float angleWing = (float) 5*glm::cos(5*wingTime.GetElapsedSeconds());
	if(wing) return angleWing;
	return -1*angleWing;
}

void drawWing(bool wing){
	glUseProgram(programId); //1.
	glBindVertexArray(vaoWing); //2.
    
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, baseTextureArwing);
    
	GLuint loc = glGetUniformLocation(programId, "tex");
	glUniform1i(loc, 0); //ATTENTION:  0 corresponds to GL_TEXTURE0
    
	loc = glGetUniformLocation(programId, "pMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&perspectiveMatrix[0]);
	loc = glGetUniformLocation(programId, "vMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&cameraMatrix[0]);
	loc = glGetUniformLocation(programId, "mMatrix");
    
	
	glm::mat4 translate = glm::translate(glm::mat4(1.0), getArwingPosition());
	
	// Need the 3 matrixes because of gimbal lock
	glm::mat4 rotateX=glm::mat4(1.0f);
	glm::mat4 rotateY = glm::rotate(glm::mat4(1.0), getArwingRotation(), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 rotateZ = glm::rotate(glm::mat4(1.0), rotateWing(wing), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 rotate = rotateX * rotateY * rotateZ;
	
	glm::mat4 scale = glm::scale(glm::mat4(1.0), glm::vec3(0.1f));
	
	if(!wing){		//Right Wing, rotates at the same rate and is a mirror of the left wing
		//rotate = glm::rotate(glm::mat4(1.0), rotateWing(), glm::vec3(0.0f, 0.0f, 1.0f));
		scale = glm::scale(glm::mat4(1.0), glm::vec3(-0.1f, 0.1f, 0.1f));
	}
	glm::mat4 model = translate * rotate  * scale;
    
	glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&model[0]);
    
    
	glm::mat4 cameraModelMatrix = cameraMatrix * model;
	
	loc = glGetUniformLocation(programId, "nMatrix");
	glm::mat3 normalMatrix = glm::mat3(cameraMatrix);
	glUniformMatrix3fv(loc, 1, GL_FALSE, (GLfloat *)&normalMatrix[0]);
	
    
	loc = glGetUniformLocation(programId, "lightDir");
	glm::vec4 transformedLightDir = cameraMatrix * glm::vec4(lightDir[0], lightDir[1], lightDir[2], 0.0f);
	glUniform3fv(loc, 1, (GLfloat *)&transformedLightDir[0]);
    
    
	loc = glGetUniformLocation(programId, "lightIntensity");
	glUniform4fv(loc, 1, lightIntensity);
    
    
	loc = glGetUniformLocation(programId, "ambientIntensity");
	glUniform4fv(loc, 1, ambientComponent);
    
    
	loc = glGetUniformLocation(programId, "diffuseColor");
	glUniform4fv(loc, 1, diffuseColor);
    
	glDrawElements(GL_TRIANGLES, leftWing.getNIndices(), GL_UNSIGNED_INT, 0); //type of geometry; number of indices; type of indices array, indices pointer
    
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0); //4.
	glUseProgram(0); //4.
}

void evolveAsteroid(){
	for(int i=0; i<maxCubes; i++){
		if(asteroids[i].currentPos.x>=75 || asteroids[i].currentPos.x<=-75) asteroids[i].velocity.x*=-1;
		if(asteroids[i].currentPos.z>=75 || asteroids[i].currentPos.z<=-75) asteroids[i].velocity.z*=-1;
		if(asteroids[i].currentPos.y>=-0.2 && asteroids[i].currentPos.y<=0.2){
			asteroids[i].currentPos+=asteroids[i].velocity;
		}
		else{
			if(asteroids[i].currentPos.y>0){
				asteroids[i].currentPos.y-=0.1f;
			}
			else if(asteroids[i].currentPos.y<0){
				asteroids[i].currentPos.y+=0.1f;
			}
		}
	}
}

void createAsteroid(int i){
	asteroids[i].rotAxis=glm::vec3(rand()*0.0001, rand()*0.0001, rand()*0.0001);
	
	asteroids[i].currentPos=glm::vec3(rand()%50, 10.0f, rand()%50);
	if(rand()%2) asteroids[i].currentPos.x *=-1;
	if(rand()%2) asteroids[i].currentPos.z *=-1;
	if(rand()%2) asteroids[i].currentPos.y *=-1;

	asteroids[i].isItAlive=true;
	
	asteroids[i].velocity=glm::vec3(rand()%5 * 0.01, 0.0f, rand()%5 * 0.01);
	if(rand()%2) asteroids[i].velocity.x*=-1;
	if(rand()%2) asteroids[i].velocity.z*=-1;
	
	asteroids[i].rot = rand()*0.005;
	asteroids[i].time.Reset();
}

void drawAsteroids(){
	for(int i=0; i<maxCubes; i++){
		if(asteroids[i].isItAlive){
				glUseProgram(programId);
				glBindVertexArray(vaoRubikCube); 
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, baseTextureRubik);

				GLuint loc = glGetUniformLocation(programId, "pMatrix");
				glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&perspectiveMatrix[0]);
				loc = glGetUniformLocation(programId, "vMatrix");
				glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&cameraMatrix[0]);
		
				loc = glGetUniformLocation(programId, "mMatrix");
				
				glm::mat4 translate = glm::translate(glm::mat4(1.0), asteroids[i].currentPos);
				glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), asteroids[i].rot*asteroids[i].time.GetElapsedSeconds(), asteroids[i].rotAxis);
				glm::mat4 scale = glm::scale(glm::mat4(1.0), glm::vec3(0.5f));
				glm::mat4 model = translate * rotate * scale;
				glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&model[0]);


				glDrawElements(GL_TRIANGLES, rubik.getNIndices(), GL_UNSIGNED_INT, 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, 0);
				glBindVertexArray(0);
				glUseProgram(0);
		}
		
		else{
			createAsteroid(i);
		}
	}
	evolveAsteroid();
}

void drawUniverse(){
	glUseProgram(programId); //1.
	glBindVertexArray(vaoUniverse); //2.
    
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, baseTextureUniverse);
    
	GLuint loc = glGetUniformLocation(programId, "tex");
	glUniform1i(loc, 0); //ATTENTION:  0 corresponds to GL_TEXTURE0
    
	loc = glGetUniformLocation(programId, "pMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&perspectiveMatrix[0]);
	loc = glGetUniformLocation(programId, "vMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&cameraMatrix[0]);
	loc = glGetUniformLocation(programId, "mMatrix");
    

	glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f));
	//glm::mat4 rotate = glm::rotate(glm::mat4(1.0), getRotationAngle()/10, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 scale = glm::scale(glm::mat4(1.0f),glm::vec3(150.0f));
	glm::mat4 model = translateMatrix   * scale;
    
	glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&model[0]);
    
	/*
     !![NEW]!!
     
     The normal matrix to transform normals according to camera.
     */
    
	glm::mat4 cameraModelMatrix = cameraMatrix * model;
	
	loc = glGetUniformLocation(programId, "nMatrix");
	glm::mat3 normalMatrix = glm::mat3(cameraMatrix);
	glUniformMatrix3fv(loc, 1, GL_FALSE, (GLfloat *)&normalMatrix[0]);
	
	loc = glGetUniformLocation(programId, "lightDir");
	glm::vec4 transformedLightDir = cameraMatrix * glm::vec4(lightDir[0], lightDir[1], lightDir[2], 0.0f);
	glUniform3fv(loc, 1, (GLfloat *)&transformedLightDir[0]);
    
    
	loc = glGetUniformLocation(programId, "lightIntensity");
	glUniform4fv(loc, 1, lightIntensity);
    
    
	loc = glGetUniformLocation(programId, "ambientIntensity");
	glUniform4fv(loc, 1, ambientComponent);
    
    
	loc = glGetUniformLocation(programId, "diffuseColor");
	glUniform4fv(loc, 1, diffuseColor);
    
	glDrawElements(GL_TRIANGLES, universe.getNIndices(), GL_UNSIGNED_INT, 0); //type of geometry; number of indices; type of indices array, indices pointer
    
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0); //4.
	glUseProgram(0); //4.
}

void evolveLasers(){
	for(int i=0; i<maxLasers; i++){
		if(firingLasers[i].bulletTime.GetElapsedSeconds()>7){
			firingLasers[i].itLives=false;
		}
		float aux = firingLasers[i].angle;
		glm::vec3 v = glm::vec3 (10*velocity*cos(aux), 0.0f, 10*velocity*sin(aux));
		firingLasers[i].pos+=v;
	
	}
}

int nextLaser(){
	for(int i=0; i<maxLasers; i++){
		if(!firingLasers[i].itLives) return i;
	}
	return -1;
}

void createLaser(){
	int i = nextLaser();
	if(i!=-1){
		firingLasers[i].bulletTime.Reset();
		firingLasers[i].itLives=true;
		firingLasers[i].angle=angle;
		glm::vec3 aux (0.5*cos(firingLasers[i].angle-3.14/2),-0.1f, 0.5*sin(firingLasers[i].angle-3.14/2));
		if(i%2){ 
			aux.x*=-1;
			aux.z*=-1;
		}
		firingLasers[i].spawnPos=arwingPos;
		firingLasers[i].direction=arwingPos-cameraPos;
		firingLasers[i].pos=arwingPos+aux;
		firingLasers[i].angle=angle;
	}
}

void drawLasers(){
	for(int i=0; i<maxLasers; i++){
		if(firingLasers[i].itLives){
				glUseProgram(programId);
				glBindVertexArray(vaoLaser); 
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, baseTextureLaser);

				GLuint loc = glGetUniformLocation(programId, "pMatrix");
				glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&perspectiveMatrix[0]);
				loc = glGetUniformLocation(programId, "vMatrix");
				glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&cameraMatrix[0]);
		
				loc = glGetUniformLocation(programId, "mMatrix");
				
				glm::mat4 translate = glm::translate(glm::mat4(1.0), firingLasers[i].pos);
				float aux = -(firingLasers[i].angle*360) / (3.14*2)-90;
				glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), aux, glm::vec3(0.0f, 1.0f, 0.0f));
				glm::mat4 scale = glm::scale(glm::mat4(1.0), glm::vec3(0.1f));
				glm::mat4 model = translate * rotate * scale;
				glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&model[0]);


				glDrawElements(GL_TRIANGLES, laser.getNIndices(), GL_UNSIGNED_INT, 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, 0);
				glBindVertexArray(0);
				glUseProgram(0);
		}
		
	}
	evolveLasers();

}

bool spawnOrNot(){
	if(spawnTime.GetElapsedSeconds()>5) return true;	
	float x = spawnTime.GetElapsedSeconds();
	while(true){
		if(x>0.5) x-=0.5;
		if(x<0.125 || x<0.375) return true;
		if(x<0.25 || x<0.5) return false;
	}

}

void draw(void)
{
    if (!inited) {
        init();
        inited = true;
    }
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clears the display with the defined clear color
	
	if(spawnOrNot()) {
		drawArwing();
		drawWing(true);		//draw left wing
		drawWing(false);	//draw right wing
	}
	drawAsteroids();
    drawLasers();
	drawUniverse();
	drawParticles();

    glFlush(); //Instructes OpenGL to finish all rendering operations
    checkError ("draw");
}

void window_size_callback(GLFWwindow * window, int width, int height)
{
	setupPerspective(width, height);
    
}

glm::vec3 camShake(){
	if(colTime.GetElapsedSeconds()>PI/2 || auxBool) return glm::vec3(0.0f);
	else return glm::vec3(colD*sin(colTime.GetElapsedSeconds()*10));
}

void setupCamera(void){
	if(asteroidsCamera){
		cameraView = getArwingPosition()+camShake();
		cameraPos = glm::vec3(0.0f, 20.0f, 0.0f);
	}
	else{
		cameraView = getArwingPosition()+camShake();
		if(!inCurve){
			cameraPos = arwingPos - glm::vec3 (7*cos(angle), -2.0f, 7*sin(angle));
		}
		else cameraPos = curve[3] - glm::vec3 (7*cos(angle), -2.0f, 7*sin(angle));
	}
		//Creates the view matrix based on the position, the view direction and the up vector
		cameraMatrix = glm::lookAt(cameraPos,
								   cameraView,
								   cameraUp);
	

}

void checkKeyboardInput(GLFWwindow * window)
{
    
	if (glfwGetKey(window, GLFW_KEY_LEFT) && !inCurve){
		angle -= velocity;
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) && !inCurve){
		angle += velocity;
	}
	if (glfwGetKey(window, GLFW_KEY_UP) && !inCurve){
		arwingPos.x += cos(angle) * velocity*6;
		arwingPos.z += sin(angle) * velocity*6;
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) && !inCurve){
		arwingPos.x -= cos(angle) * velocity*2;
		arwingPos.z -= sin(angle) * velocity*2;
	}
	if(glfwGetKey(window, GLFW_KEY_SPACE) && keysTime.GetElapsedSeconds()>0.5f){
		createLaser();
		blockKeys();
	}
	if(glfwGetKey(window, GLFW_KEY_ENTER) && keysTime.GetElapsedSeconds()>0.5f){
		if(asteroidsCamera) asteroidsCamera=false;
		else asteroidsCamera=true;
		blockKeys();
	}
	if(glfwGetKey(window, GLFW_KEY_R) && keysTime.GetElapsedSeconds()>0.5f){
		restart();
		blockKeys();
	}
}

bool checkCollision(glm::vec3 obj1, glm::vec3 obj2, float r1, float r2){
	float distance = sqrt( (obj1.x-obj2.x)*(obj1.x-obj2.x)+(obj1.y-obj2.y)*(obj1.y-obj2.y)+(obj1.z-obj2.z)*(obj1.z-obj2.z));
	if(distance<=r1+r2) {
		return true;
	}
	return false;
}

bool samePartOfTheArena(glm::vec3 obj1, glm::vec3 obj2){
	if(obj1.x>=0 && obj2.x>=0){
		if(obj1.z>=0 && obj2.z>=0) return true;
		if(obj1.z<0 && obj2.z<0) return true;
	}
	if(obj1.x<0 && obj2.x<0){
		if(obj1.z>=0 && obj2.z>=0) return true;
		if(obj1.z<0 && obj2.z<0) return true;
	}
	return false;
}

void arwingOnCubeCollision(){
	for(int i=0; i<maxCubes; i++){
		if(samePartOfTheArena(arwingPos, asteroids[i].currentPos)){
			if(checkCollision(arwingPos, asteroids[i].currentPos, 1.5f, 2.0f) && spawnTime.GetElapsedSeconds()>5.0f && asteroids[i].isItAlive){
				restart();
				colD=1.5;
				colTime.Reset();
			}
		}
	}
}

float calculateDistante(glm::vec3 a, glm::vec3 b){
	float x = a.x-b.x;
	float y = a.y-b.y;
	float z = a.z-b.z;
	return sqrt ( x*x+y*y+z*z);
}

void cubeOnCubeCollisionCheck(int i, int j){
	if(checkCollision(asteroids[i].currentPos, asteroids[j].currentPos, 1, 1)){
		glm::vec3 aux = asteroids[i].velocity;
		asteroids[i].velocity=asteroids[j].velocity;
		asteroids[j].velocity=aux;
		
		colD=1/(calculateDistante(arwingPos, asteroids[i].currentPos)+ 5);
		colTime.Reset();
	}
}

void cubeOnCubeCollisionSystem(){
	for(int i=0; i<maxCubes; i++){
		for(int j=i+1; j<maxCubes;j++){
			if(samePartOfTheArena(asteroids[i].currentPos, asteroids[j].currentPos) && asteroids[i].isItAlive) {
				cubeOnCubeCollisionCheck(i, j);	
			}
		}
	}
}

void cubeOnLaserCollision(){
	for(int i=0; i<maxCubes; i++){
		for(int j=0; j<maxLasers; j++){
			if(samePartOfTheArena(asteroids[i].currentPos, firingLasers[j].pos)){
				if(checkCollision(asteroids[i].currentPos, firingLasers[j].pos, 1.0f, 0.5f) && asteroids[i].isItAlive){
					asteroids[i].isItAlive=false;
					firingLasers[j].itLives=false;
					colD=1/calculateDistante(arwingPos, asteroids[i].currentPos);
					colTime.Reset();
				}
			}
		}
	}
}

void mainLoop(GLFWwindow * window){
	checkArwingPos();
	checkKeyboardInput(window);
	setupCamera();
	draw();
	if(!inCurve) evolveArwingPos();
	arwingOnCubeCollision();
	cubeOnCubeCollisionSystem();
	cubeOnLaserCollision();
	if(auxClock.GetElapsedSeconds()>5) auxBool=false;
	
}

 int main (int argc, char const *argv[]){
    GLFWwindow * window = 0;
    
    glfwInit(); //[GLFW] init glfw
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2); //[GLFW] request for Opengl 3.2 (MAJOR).(MINOR)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //[GLFW] Allow forward compatible
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //[GLFW] Request a Core profile
    window = glfwCreateWindow(640, 480, "Rubik Asteroids", NULL, NULL);// [GLFW] Create window
    
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window); //[GLFW] Make the current window the context for opengl calls
    glfwSetWindowSizeCallback(window, window_size_callback);
    

	restart();

    while (!glfwGetKey(window, GLFW_KEY_ESCAPE) && !glfwWindowShouldClose(window)) {
        mainLoop(window); 
        
        glfwSwapBuffers(window); //Swaps the display in a double buffering scenario. In double buffering, rendering is done in a offline buffer (not directly on the screen); this avoid flickering
        
        /* Poll for and process events */
        glfwPollEvents();
    }
    
    glfwTerminate();
    exit(EXIT_SUCCESS);
}