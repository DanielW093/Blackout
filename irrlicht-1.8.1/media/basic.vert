// VERTEX SHADER
#version 330

// Matrices
uniform mat4 matrixProjection;
uniform mat4 matrixView;
uniform mat4 matrixModelView;

// Materials
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float shininess;
 
layout (location = 0) in vec3 aVertex;
layout (location = 2) in vec3 aNormal;

out vec4 color;
vec4 position;
vec3 normal;

void main(void) 
{
	// calculate position
	position = matrixModelView * vec4(aVertex, 1.0);
	gl_Position = matrixProjection * position;

	// calculate light
	color = vec4(0, 0, 0, 1);
}
