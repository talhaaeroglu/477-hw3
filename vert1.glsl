#version 120 

attribute vec3 inVertex;
attribute vec3 inNormal;

uniform mat4 modelingMat;
uniform mat4 modelingMatInvTr;
uniform mat4 perspectiveMat;

varying vec4 fragPos;
varying vec3 N;

void main(void)
{

	vec4 p = modelingMat * vec4(inVertex, 1); // translate to world coordinates
	vec3 Nw = vec3(modelingMatInvTr * vec4(inNormal, 0)); // provided by the programmer

	N = normalize(Nw);
	fragPos = p;

    gl_Position = perspectiveMat * modelingMat * vec4(inVertex, 1);
}

