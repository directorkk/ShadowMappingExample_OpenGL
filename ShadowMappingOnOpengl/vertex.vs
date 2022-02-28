// vertex.vs 

varying vec3 outNormal; 
varying vec3 position; 
varying vec4 lightVertexPosition;

uniform mat4 modelMatrix;
uniform mat4 lightModelViewProjectionMatrix;

void main() 
{ 
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; 
	lightVertexPosition = lightModelViewProjectionMatrix * ( modelMatrix * gl_Vertex );
    
	position = vec3( gl_ModelViewMatrix * gl_Vertex ); 
	outNormal = gl_NormalMatrix * gl_Normal; 
}