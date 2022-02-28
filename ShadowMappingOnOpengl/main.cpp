#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtx\transform.hpp"
#include "glm\gtc\type_ptr.hpp"

#include "glew.h"
#include "freeglut.h"

#include <iostream>
#include <fstream>
#include <string>

#define WINDOW_SIZE_W 500		// 基礎視窗長寬
#define WINDOW_SIZE_H 500
#define WINDOW_VISION_ANGLE 45	// 視角
#define WINDOW_VISION_NEAR 0.5	// 最近視野
#define WINDOW_VISION_FAR 10001	// 最遠視野

#define SHADOW_RATE 2	// 影子貼圖倍率(愈高影子愈精細)

unsigned int windowHandle;			// 視窗控制變數(id)
int windowWidth = WINDOW_SIZE_W,	// 視窗長寬
	windowHeight = WINDOW_SIZE_H;

unsigned int vs, fs, program;		// 用來儲存shader還有program的控制變數(id) 
unsigned int FBO,	// Frame buffer object的控制變數(id)
	renderTex,		// 用來給FBO渲染「色彩」資料的材質控制變數(id)
	shadowMap;		// 同renderTex, 但是是用來渲染「深度」資料的材質(繪製影子需要)

glm::vec3 lightPosition( 100, 100, -100 );		// 起始光源座標
glm::mat4 shadowMatrix;							// 基於光源的ModelViewProjection Matrix

unsigned int createTexture( int w, int h, bool isDepth );		// 用來與openGL註冊新材質的函式, 回傳材質控制變數(id)
void loadFile( const char* filename, std::string &string );		// 讀取儲存於文件中的渲染器(shader)程式碼, 並儲存於變數string
unsigned int loadShader(std::string &source, GLenum type);		// 用來編譯渲染器(shader)的函式, 回傳渲染器控制變數(id)
void initShader(const char* vname, const char* fname);			// 總和讀取及編譯渲染器的函式
void drawsomething();		// 繪製場景

void Initialize()
{
	glEnable( GL_DEPTH_TEST );
	initShader( "vertex.vs", "fragment.frag" );		// 讀入文件vertex.vs及fragment.frag並編譯

	renderTex = createTexture( SHADOW_RATE*WINDOW_SIZE_W, SHADOW_RATE*WINDOW_SIZE_H, false );	// 註冊一「色彩」階級的材質
	shadowMap = createTexture( SHADOW_RATE*WINDOW_SIZE_W, SHADOW_RATE*WINDOW_SIZE_H, true );	// 註冊一「深度」階級的材質
	glBindTexture( GL_TEXTURE_2D, shadowMap );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
	glBindTexture( GL_TEXTURE_2D, 0 );

	// 這裡是Frame buffer object
	glGenFramebuffers( 1, &FBO );	// 註冊一新的Frame buffer object(下簡稱FBO), 控制變數回傳到變數FBO
	glBindFramebuffer( GL_FRAMEBUFFER, FBO );	// 使用FBO
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0 );	// 選擇要用來儲存FBO渲染資料的材質(色彩)
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0 );		// 選擇要用來儲存FBO渲染資料的材質(深度)
	{
		int error = glCheckFramebufferStatus( GL_FRAMEBUFFER );		// 檢查FBO的註冊是否有出錯
		if( error != GL_FRAMEBUFFER_COMPLETE ){
			std::cout << "Framebuffer is not OK, status: " << error << std::endl;
		}
	}
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );		// 關閉FBO
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

void getShadowImage()	// 用來取得在光源視角的畫面深度資料, 並儲存於shadowMap
{
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( WINDOW_VISION_ANGLE, (float)windowWidth/(float)windowHeight, WINDOW_VISION_NEAR, WINDOW_VISION_FAR );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glEnable( GL_DEPTH_TEST );

	gluLookAt( lightPosition.x, lightPosition.y, lightPosition.z, 0, 0, 0, 0, 1, 0 );	// 須設定光源的視野矩陣(view matrix)

	glm::mat4 viewM = 
		glm::lookAt( 
		glm::vec3( lightPosition.x, lightPosition.y, lightPosition.z ), 
		glm::vec3( 0, 0, 0 ), 
		glm::vec3( 0, 1, 0 ) );		// 這是取得視野矩陣的一種方式

	glm::mat4 biasMatrix;		// 關鍵字Normalized device coordinates
	// 為了將 (-1, -1)-(1, 1) 修正為 (0, 0)-(1, 1)
	biasMatrix[0][0]=0.5;biasMatrix[0][1]=0.0;biasMatrix[0][2]=0.0;biasMatrix[0][3]=0.0;
	biasMatrix[1][0]=0.0;biasMatrix[1][1]=0.5;biasMatrix[1][2]=0.0;biasMatrix[1][3]=0.0;
	biasMatrix[2][0]=0.0;biasMatrix[2][1]=0.0;biasMatrix[2][2]=0.5;biasMatrix[2][3]=0.0;
	biasMatrix[3][0]=0.5;biasMatrix[3][1]=0.5;biasMatrix[3][2]=0.5;biasMatrix[3][3]=1.0;

	glm::mat4 projM = glm::perspective( (float)WINDOW_VISION_ANGLE, (float)windowWidth/(float)windowHeight, (float)WINDOW_VISION_NEAR, (float)WINDOW_VISION_FAR );
	shadowMatrix = biasMatrix * projM * viewM;


	glBindFramebuffer( GL_FRAMEBUFFER, FBO );	// 使用FBO並綁定材質
	glFramebufferTexture2D( GL_FRAMEBUFFER,  GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0 );

	glViewport( 0, 0, SHADOW_RATE*WINDOW_SIZE_W, SHADOW_RATE*WINDOW_SIZE_H );	// 因為要渲染不同大小的材質, 所以要變更viewport

	glEnable( GL_CULL_FACE );
	glCullFace( GL_FRONT );
	// 這裡需要去除其中一個面
	// 因為當我們繪製一個形狀, 在openGL中是繪製這個形狀的正反兩面
	// 兩個面上的像素的深度極其接近(但依然不同), 會導致深度圖同時混和正面及反面的深度
	// 這會使在比對影子的時候出現錯誤
	// 同一vertex背面的會稍為遠一點點, 若因誤差關係比對到正面的深度則會出現影子
	// 而導致出現條紋狀的紋理
	// 解決方法已知有二
	// 1. 同本範例, 繪製深度圖時挖去其中一面
	// 2. 在渲染器(shader)中做偵錯, 確定深度真的相差了一定程度才畫上影子
	glClear( GL_DEPTH_BUFFER_BIT );
	glUniformMatrix4fv( glGetUniformLocation( program, "lightModelViewProjectionMatrix" ), 1, GL_FALSE,
		&shadowMatrix[0][0] );			// 傳入矩陣

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
		glm::vec3( 0, 1, 0 ) );		// 這是取得視野矩陣的一種方式
	glm::vec4 worldLightPosition = ViewMatrix * glm::vec4( lightPosition.x, lightPosition.y, lightPosition.z, 1.0f );
	// 將光座標(lightPosition), 調整為世界座標(worldLightPosition), 讓光宇場景同步

	// 傳遞參數進入渲染器(shader)
	// 傳材質的方式如下
	glActiveTexture( GL_TEXTURE0 );		// 先啟動一材質unit(GL_TEXTURE0,1,2...,31, 共32張, <附註: 這表示能同時使用32張材質來渲染一個面>)
	glBindTexture( GL_TEXTURE_2D, shadowMap );		// 在綁定一張材質到剛剛啟動的材質unit上
	glUniform1i( glGetUniformLocation( program, "shadowMap" ), 0 );		// 傳入編號給渲染器即可(看最後那個數字, 假如啟動的是GL_TEXTURE5, 就傳5進去)
	glUniformMatrix4fv( glGetUniformLocation( program, "lightModelViewProjectionMatrix" ), 1, GL_FALSE,
		&shadowMatrix[0][0] );			// 傳入矩陣
	glUniform3f( glGetUniformLocation( program, "lightPosition" ), worldLightPosition.x, worldLightPosition.y, worldLightPosition.z );	// 傳入光座標
	glUniform4f( glGetUniformLocation( program, "lambient" ), 0.4f, 0.4f, 0.4f, 1.0f );		// 設定光顏色, 環境光
	glUniform4f( glGetUniformLocation( program, "ldiffuse" ), 0.7f, 0.7f, 0.7f, 1.0f );		// 散射光
	glUniform4f( glGetUniformLocation( program, "lspecular" ), 1.0f, 1.0f, 1.0f, 1.0f );	// 反射光

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
	ShaderID = glCreateShader( type );      // 告訴OpenGL我們要創的是哪種shader 

	const char* csource = source.c_str();   // 把std::string結構轉換成const char* 

	glShaderSource( ShaderID, 1, &csource, NULL );   // 把程式碼放進去剛剛創建的shader object中 
	glCompileShader( ShaderID );                     // 編譯shader 
	char error[1000] = ""; 
	glGetShaderInfoLog( ShaderID, 1000, NULL, error );   // 這是編譯過程的訊息, 錯誤什麼的把他丟到error裡面 
	std::cout << "Complie status: \n" << error << std::endl;   // 然後輸出出來 

	return ShaderID; 
}

void initShader(const char* vname, const char* fname) 
{ 
	std::string source; 

	loadFile( vname, source );      // 把程式碼讀進source 
	vs = loadShader( source, GL_VERTEX_SHADER );   // 編譯shader並且把id傳回vs 
	source = ""; 
	loadFile( fname, source ); 
	fs = loadShader( source, GL_FRAGMENT_SHADER ); 

	program = glCreateProgram();   // 創建一個program 
	glAttachShader( program, vs );   // 把vertex shader跟program連結上 
	glAttachShader( program, fs );   // 把fragment shader跟program連結上 

	glLinkProgram( program );      // 根據被連結上的shader, link出各種processor 
	glUseProgram( program );      // 然後使用它 
}

void drawsomething()
{
	glm::mat4 tmpModelMatrix( 1.0 );	// 若對形體有座glRotate, glScale, glTranslate的動作, 擇要傳入相對應的模型矩陣(model matrix)

	glUniform3f( glGetUniformLocation( program, "color" ), 1.0f, 0.0f, 0.0f );	// 這是傳入vec3格式給渲染器的做法
	glUniformMatrix4fv( glGetUniformLocation( program, "modelMatrix" ), 1, GL_FALSE,
		&tmpModelMatrix[0][0] );
	glBegin( GL_QUADS );
	// 繪製一方形
	// 正面 
	glNormal3f( 0, 0, 1 );
	glVertex3f(-7, 7, 7); 
	glVertex3f(-7,-7, 7); 
	glVertex3f( 7,-7, 7); 
	glVertex3f( 7, 7, 7); 
	// 背面 
	glNormal3f( 0, 0, -1 ); 
	glVertex3f(-7, 7,-7 ); 
	glVertex3f( 7, 7,-7 ); 
	glVertex3f( 7,-7,-7 ); 
	glVertex3f(-7,-7,-7 ); 
	// 右側面 
	glNormal3f( 1, 0, 0 ); 
	glVertex3f( 7, 7, 7 ); 
	glVertex3f( 7,-7, 7 ); 
	glVertex3f( 7,-7,-7 ); 
	glVertex3f( 7, 7,-7 ); 
	// 左側面 
	glNormal3f(-1, 0, 0 ); 
	glVertex3f(-7, 7,-7 ); 
	glVertex3f(-7,-7,-7 ); 
	glVertex3f(-7,-7, 7 ); 
	glVertex3f(-7, 7, 7 ); 
	// 上面 
	glNormal3f( 0, 1, 0 ); 
	glVertex3f(-7, 7,-7 ); 
	glVertex3f(-7, 7, 7 ); 
	glVertex3f( 7, 7, 7 ); 
	glVertex3f( 7, 7,-7 );
	// 下面
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
	// 繪製地面
	glNormal3d( 0, 1, 0 );
	glVertex3d( 50,-10, 50 );
	glVertex3d( 50,-10,-50 );
	glVertex3d(-50,-10,-50 );
	glVertex3d(-50,-10, 50 );
	glEnd();
}