#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtx\transform.hpp"
#include "glm\gtc\type_ptr.hpp"

#include "glew.h"
#include "freeglut.h"

#include <iostream>
#include <fstream>
#include <string>

#define WINDOW_SIZE_W 500		// ��¦�������e
#define WINDOW_SIZE_H 500
#define WINDOW_VISION_ANGLE 45	// ����
#define WINDOW_VISION_NEAR 0.5	// �̪����
#define WINDOW_VISION_FAR 10001	// �̻�����

#define SHADOW_RATE 2	// �v�l�K�ϭ��v(�U���v�l�U���)

unsigned int windowHandle;			// ���������ܼ�(id)
int windowWidth = WINDOW_SIZE_W,	// �������e
	windowHeight = WINDOW_SIZE_H;

unsigned int vs, fs, program;		// �Ψ��x�sshader�٦�program�������ܼ�(id) 
unsigned int FBO,	// Frame buffer object�������ܼ�(id)
	renderTex,		// �Ψӵ�FBO��V�u��m�v��ƪ����豱���ܼ�(id)
	shadowMap;		// �PrenderTex, ���O�O�ΨӴ�V�u�`�סv��ƪ�����(ø�s�v�l�ݭn)

glm::vec3 lightPosition( 100, 100, -100 );		// �_�l�����y��
glm::mat4 shadowMatrix;							// ��������ModelViewProjection Matrix

unsigned int createTexture( int w, int h, bool isDepth );		// �ΨӻPopenGL���U�s���誺�禡, �^�ǧ��豱���ܼ�(id)
void loadFile( const char* filename, std::string &string );		// Ū���x�s���󤤪���V��(shader)�{���X, ���x�s���ܼ�string
unsigned int loadShader(std::string &source, GLenum type);		// �ΨӽsĶ��V��(shader)���禡, �^�Ǵ�V�������ܼ�(id)
void initShader(const char* vname, const char* fname);			// �`�MŪ���νsĶ��V�����禡
void drawsomething();		// ø�s����

void Initialize()
{
	glEnable( GL_DEPTH_TEST );
	initShader( "vertex.vs", "fragment.frag" );		// Ū�J���vertex.vs��fragment.frag�ýsĶ

	renderTex = createTexture( SHADOW_RATE*WINDOW_SIZE_W, SHADOW_RATE*WINDOW_SIZE_H, false );	// ���U�@�u��m�v���Ū�����
	shadowMap = createTexture( SHADOW_RATE*WINDOW_SIZE_W, SHADOW_RATE*WINDOW_SIZE_H, true );	// ���U�@�u�`�סv���Ū�����
	glBindTexture( GL_TEXTURE_2D, shadowMap );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
	glBindTexture( GL_TEXTURE_2D, 0 );

	// �o�̬OFrame buffer object
	glGenFramebuffers( 1, &FBO );	// ���U�@�s��Frame buffer object(�U²��FBO), �����ܼƦ^�Ǩ��ܼ�FBO
	glBindFramebuffer( GL_FRAMEBUFFER, FBO );	// �ϥ�FBO
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0 );	// ��ܭn�Ψ��x�sFBO��V��ƪ�����(��m)
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0 );		// ��ܭn�Ψ��x�sFBO��V��ƪ�����(�`��)
	{
		int error = glCheckFramebufferStatus( GL_FRAMEBUFFER );		// �ˬdFBO�����U�O�_���X��
		if( error != GL_FRAMEBUFFER_COMPLETE ){
			std::cout << "Framebuffer is not OK, status: " << error << std::endl;
		}
	}
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );		// ����FBO
}

void Reshape( int w, int h )
{
	glViewport( 0, 0, w, h );

	if( h == 0 ) h = 1;

	windowWidth = w;
	windowHeight = h;

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( WINDOW_VISION_ANGLE, (float)w/(float)h, WINDOW_VISION_NEAR, WINDOW_VISION_FAR );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();	
}

void getShadowImage()	// �ΨӨ��o�b�����������e���`�׸��, ���x�s��shadowMap
{
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( WINDOW_VISION_ANGLE, (float)windowWidth/(float)windowHeight, WINDOW_VISION_NEAR, WINDOW_VISION_FAR );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glEnable( GL_DEPTH_TEST );

	gluLookAt( lightPosition.x, lightPosition.y, lightPosition.z, 0, 0, 0, 0, 1, 0 );	// ���]�w�����������x�}(view matrix)

	glm::mat4 viewM = 
		glm::lookAt( 
		glm::vec3( lightPosition.x, lightPosition.y, lightPosition.z ), 
		glm::vec3( 0, 0, 0 ), 
		glm::vec3( 0, 1, 0 ) );		// �o�O���o�����x�}���@�ؤ覡

	glm::mat4 biasMatrix;		// ����rNormalized device coordinates
	// ���F�N (-1, -1)-(1, 1) �ץ��� (0, 0)-(1, 1)
	biasMatrix[0][0]=0.5;biasMatrix[0][1]=0.0;biasMatrix[0][2]=0.0;biasMatrix[0][3]=0.0;
	biasMatrix[1][0]=0.0;biasMatrix[1][1]=0.5;biasMatrix[1][2]=0.0;biasMatrix[1][3]=0.0;
	biasMatrix[2][0]=0.0;biasMatrix[2][1]=0.0;biasMatrix[2][2]=0.5;biasMatrix[2][3]=0.0;
	biasMatrix[3][0]=0.5;biasMatrix[3][1]=0.5;biasMatrix[3][2]=0.5;biasMatrix[3][3]=1.0;

	glm::mat4 projM = glm::perspective( (float)WINDOW_VISION_ANGLE, (float)windowWidth/(float)windowHeight, (float)WINDOW_VISION_NEAR, (float)WINDOW_VISION_FAR );
	shadowMatrix = biasMatrix * projM * viewM;


	glBindFramebuffer( GL_FRAMEBUFFER, FBO );	// �ϥ�FBO�øj�w����
	glFramebufferTexture2D( GL_FRAMEBUFFER,  GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0 );

	glViewport( 0, 0, SHADOW_RATE*WINDOW_SIZE_W, SHADOW_RATE*WINDOW_SIZE_H );	// �]���n��V���P�j�p������, �ҥH�n�ܧ�viewport

	glEnable( GL_CULL_FACE );
	glCullFace( GL_FRONT );
	// �o�̻ݭn�h���䤤�@�ӭ�
	// �]����ڭ�ø�s�@�ӧΪ�, �bopenGL���Oø�s�o�ӧΪ������Ϩ⭱
	// ��ӭ��W���������`�׷��䱵��(���̵M���P), �|�ɭP�`�׹ϦP�ɲV�M�����Τϭ����`��
	// �o�|�Ϧb���v�l���ɭԥX�{���~
	// �P�@vertex�I�����|�y�����@�I�I, �Y�]�~�t���Y���쥿�����`�׫h�|�X�{�v�l
	// �ӾɭP�X�{�����������z
	// �ѨM��k�w�����G
	// 1. �P���d��, ø�s�`�׹Ϯɫ��h�䤤�@��
	// 2. �b��V��(shader)��������, �T�w�`�ׯu���ۮt�F�@�w�{�פ~�e�W�v�l
	glClear( GL_DEPTH_BUFFER_BIT );
	glUniformMatrix4fv( glGetUniformLocation( program, "lightModelViewProjectionMatrix" ), 1, GL_FALSE,
		&shadowMatrix[0][0] );			// �ǤJ�x�}

	drawsomething();

	glDisable( GL_CULL_FACE );
	glViewport( 0, 0, windowWidth, windowHeight );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void Display()
{
	getShadowImage();

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glClearColor( 0.0, 0.0, 0.0, 1.0 );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( WINDOW_VISION_ANGLE, windowWidth/(float)windowHeight, WINDOW_VISION_NEAR, WINDOW_VISION_FAR );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	gluLookAt( 100, 100, 100, 0, 0, 0, 0, 1, 0 );

	glm::mat4 ViewMatrix = 
		glm::lookAt( 
		glm::vec3( 100, 100, 100 ), 
		glm::vec3( 0, 0, 0 ), 
		glm::vec3( 0, 1, 0 ) );		// �o�O���o�����x�}���@�ؤ覡
	glm::vec4 worldLightPosition = ViewMatrix * glm::vec4( lightPosition.x, lightPosition.y, lightPosition.z, 1.0f );
	// �N���y��(lightPosition), �վ㬰�@�ɮy��(worldLightPosition), �����t�����P�B

	// �ǻ��Ѽƶi�J��V��(shader)
	// �ǧ��誺�覡�p�U
	glActiveTexture( GL_TEXTURE0 );		// ���Ұʤ@����unit(GL_TEXTURE0,1,2...,31, �@32�i, <����: �o��ܯ�P�ɨϥ�32�i����Ӵ�V�@�ӭ�>)
	glBindTexture( GL_TEXTURE_2D, shadowMap );		// �b�j�w�@�i�������Ұʪ�����unit�W
	glUniform1i( glGetUniformLocation( program, "shadowMap" ), 0 );		// �ǤJ�s������V���Y�i(�ݳ̫ᨺ�ӼƦr, ���p�Ұʪ��OGL_TEXTURE5, �N��5�i�h)
	glUniformMatrix4fv( glGetUniformLocation( program, "lightModelViewProjectionMatrix" ), 1, GL_FALSE,
		&shadowMatrix[0][0] );			// �ǤJ�x�}
	glUniform3f( glGetUniformLocation( program, "lightPosition" ), worldLightPosition.x, worldLightPosition.y, worldLightPosition.z );	// �ǤJ���y��
	glUniform4f( glGetUniformLocation( program, "lambient" ), 0.4f, 0.4f, 0.4f, 1.0f );		// �]�w���C��, ���ҥ�
	glUniform4f( glGetUniformLocation( program, "ldiffuse" ), 0.7f, 0.7f, 0.7f, 1.0f );		// ���g��
	glUniform4f( glGetUniformLocation( program, "lspecular" ), 1.0f, 1.0f, 1.0f, 1.0f );	// �Ϯg��

	drawsomething();

	glutSwapBuffers();
}

void Timer( int te )
{
	glutPostRedisplay();
	glutTimerFunc( 1000/60, Timer, 1 );
}

int main( int argc, char** argv )
{
	glutInit( &argc, argv );

	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
	glutInitWindowPosition( 0, 0 );
	glutInitWindowSize( WINDOW_SIZE_W, WINDOW_SIZE_H );

	windowHandle = glutCreateWindow( "Shadow Mapping" );
	glewInit();

	Initialize();

	glutDisplayFunc( Display );
	glutReshapeFunc( Reshape );
	glutTimerFunc( 1000/60, Timer, 0 );

	glutMainLoop();

	return 0;
}

unsigned int createTexture( int w, int h, bool isDepth )
{
	unsigned int textureID;

	glGenTextures( 1, &textureID );
	glBindTexture( GL_TEXTURE_2D, textureID );
	glTexImage2D( GL_TEXTURE_2D, 0,
		isDepth ? GL_DEPTH_COMPONENT : GL_RGBA8,
		w, h, 0,
		isDepth ? GL_DEPTH_COMPONENT : GL_RGBA,
		GL_FLOAT, NULL );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );

	int error;
	error = glGetError();
	if( glGetError() != 0 ){
		std::cout << "Error happened while loading the texture: " << gluErrorString(error) << std::endl;
	}
	glBindTexture( GL_TEXTURE_2D, 0 );

	return textureID;
}
void loadFile( const char* filename, std::string &string ) 
{ 
	std::ifstream fp(filename); 
	if( !fp.is_open() ){ 
		std::cout << "Open <" << filename << "> error." << std::endl; 
		return; 
	} 

	char temp[300]; 
	while( !fp.eof() ){ 
		fp.getline( temp, 300 ); 
		string += temp; 
		string += '\n'; 
	} 

	fp.close(); 
}

unsigned int loadShader(std::string &source, GLenum type) 
{ 
	GLuint ShaderID; 
	ShaderID = glCreateShader( type );      // �i�DOpenGL�ڭ̭n�Ъ��O����shader 

	const char* csource = source.c_str();   // ��std::string���c�ഫ��const char* 

	glShaderSource( ShaderID, 1, &csource, NULL );   // ��{���X��i�h���Ыت�shader object�� 
	glCompileShader( ShaderID );                     // �sĶshader 
	char error[1000] = ""; 
	glGetShaderInfoLog( ShaderID, 1000, NULL, error );   // �o�O�sĶ�L�{���T��, ���~���򪺧�L���error�̭� 
	std::cout << "Complie status: \n" << error << std::endl;   // �M���X�X�� 

	return ShaderID; 
}

void initShader(const char* vname, const char* fname) 
{ 
	std::string source; 

	loadFile( vname, source );      // ��{���XŪ�isource 
	vs = loadShader( source, GL_VERTEX_SHADER );   // �sĶshader�åB��id�Ǧ^vs 
	source = ""; 
	loadFile( fname, source ); 
	fs = loadShader( source, GL_FRAGMENT_SHADER ); 

	program = glCreateProgram();   // �Ыؤ@��program 
	glAttachShader( program, vs );   // ��vertex shader��program�s���W 
	glAttachShader( program, fs );   // ��fragment shader��program�s���W 

	glLinkProgram( program );      // �ھڳQ�s���W��shader, link�X�U��processor 
	glUseProgram( program );      // �M��ϥΥ� 
}

void drawsomething()
{
	glm::mat4 tmpModelMatrix( 1.0 );	// �Y����馳�yglRotate, glScale, glTranslate���ʧ@, �ܭn�ǤJ�۹������ҫ��x�}(model matrix)

	glUniform3f( glGetUniformLocation( program, "color" ), 1.0f, 0.0f, 0.0f );	// �o�O�ǤJvec3�榡����V�������k
	glUniformMatrix4fv( glGetUniformLocation( program, "modelMatrix" ), 1, GL_FALSE,
		&tmpModelMatrix[0][0] );
	glBegin( GL_QUADS );
	// ø�s�@���
	// ���� 
	glNormal3f( 0, 0, 1 );
	glVertex3f(-7, 7, 7); 
	glVertex3f(-7,-7, 7); 
	glVertex3f( 7,-7, 7); 
	glVertex3f( 7, 7, 7); 
	// �I�� 
	glNormal3f( 0, 0, -1 ); 
	glVertex3f(-7, 7,-7 ); 
	glVertex3f( 7, 7,-7 ); 
	glVertex3f( 7,-7,-7 ); 
	glVertex3f(-7,-7,-7 ); 
	// �k���� 
	glNormal3f( 1, 0, 0 ); 
	glVertex3f( 7, 7, 7 ); 
	glVertex3f( 7,-7, 7 ); 
	glVertex3f( 7,-7,-7 ); 
	glVertex3f( 7, 7,-7 ); 
	// ������ 
	glNormal3f(-1, 0, 0 ); 
	glVertex3f(-7, 7,-7 ); 
	glVertex3f(-7,-7,-7 ); 
	glVertex3f(-7,-7, 7 ); 
	glVertex3f(-7, 7, 7 ); 
	// �W�� 
	glNormal3f( 0, 1, 0 ); 
	glVertex3f(-7, 7,-7 ); 
	glVertex3f(-7, 7, 7 ); 
	glVertex3f( 7, 7, 7 ); 
	glVertex3f( 7, 7,-7 );
	// �U��
	glNormal3f( 0,-1, 0); 
	glVertex3f( 7,-7,-7 ); 
	glVertex3f( 7,-7, 7 ); 
	glVertex3f(-7,-7, 7 ); 
	glVertex3f(-7,-7,-7 ); 
	glEnd();


	glUniform3f( glGetUniformLocation( program, "color" ), 1.0f, 1.0f, 1.0f );	
	glUniformMatrix4fv( glGetUniformLocation( program, "modelMatrix" ), 1, GL_FALSE,
		&tmpModelMatrix[0][0] );
	glBegin( GL_QUADS );
	// ø�s�a��
	glNormal3d( 0, 1, 0 );
	glVertex3d( 50,-10, 50 );
	glVertex3d( 50,-10,-50 );
	glVertex3d(-50,-10,-50 );
	glVertex3d(-50,-10, 50 );
	glEnd();
}