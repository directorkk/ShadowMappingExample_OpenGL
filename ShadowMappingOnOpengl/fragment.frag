// fragment.frag 

uniform sampler2DShadow shadowMap;
uniform vec3 color;
uniform vec3 lightPosition; 

uniform vec4 lambient; 
uniform vec4 ldiffuse; 
uniform vec4 lspecular; 

varying vec2 texcoord; 
varying vec3 outNormal; 
varying vec3 position; 
varying vec4 lightVertexPosition;

void main() 
{ 
	float shadowValue = 0.0;
	// 勻化影子
	for( float x=-1.5 ; x<=1.5 ; x+=0.6 ){
		for( float y=-1.5 ; y<=1.5 ; y+=0.6 ){
			if( shadow2DProj( shadowMap, lightVertexPosition + vec4( x, y, 0, 0 ) ).r == 1.0 )
				shadowValue += 1.0;
		}
	}
	shadowValue /= 25.0;


	// 不勻化影子
	/*
	if( shadow2DProj( shadowMap, lightVertexPosition ).r == 1.0 )
		shadowValue = 1.0;
    */

	vec4 texcolor = vec4( color, 1.0 );
	
	vec3 normal = normalize( outNormal ); 
	vec3 surf2light = normalize( lightPosition - position ); 
	vec3 surf2camera = normalize( -position ); 
	vec3 reflection = -reflect( surf2camera, normal ); 

	vec3 ambient = vec3( lambient * texcolor ); 

	float diffuseContribution = max( 0.0, dot( normal, surf2light ) ); 
	vec3 diffuse = vec3( shadowValue * diffuseContribution * ldiffuse * texcolor ); 
    
	float specularContribution = pow( max( 0.0, dot( reflection, surf2light ) ), 32.0); 
	vec3 specular = shadowValue < 1.0 ? vec3( 0.0, 0.0, 0.0 ) : vec3( specularContribution * lspecular ); 

	gl_FragColor = vec4( ambient + diffuse + specular, 1.0 ); 
}