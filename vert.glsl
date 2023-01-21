#version 330 compatibility

vec3 lightPos = vec3(5, 5, 5);
vec3 eyePos = vec3(0, 0, 0);

vec3 I = vec3(2, 2, 2);
vec3 Iamb = vec3(0.8, 0.8, 0.8);

vec3 kd = vec3(1, 0.2, 0.2);
vec3 ka = vec3(0.1, 0.1, 0.1);
vec3 ks = vec3(0.8, 0.8, 0.8);

layout(location=0) in vec3 inVertex;
layout(location=1) in vec3 inNormal;

/*
varying vec4 fragPos;
varying vec3 N;
*/

void main(void)
{
    //gl_FrontColor = gl_Color; // vertex color defined by the programmer

	vec4 p = gl_ModelViewMatrix * vec4(inVertex, 1); // translate to world coordinates

	vec3 L = normalize(lightPos - vec3(p));
	vec3 V = normalize(eyePos - vec3(p));
	vec3 H = normalize(L + V);
	vec3 N = vec3(gl_ModelViewMatrixInverseTranspose * vec4(inNormal, 0)); // provided by the programmer
	N = normalize(N);
	float NdotL = dot(N, L);
	float NdotH = dot(N, H);

	vec3 diffuseColor = I * kd * max(0, NdotL);
	vec3 ambientColor = Iamb * ka;
	vec3 specularColor = I * ks * pow(max(0, NdotH), 20);

	gl_FrontColor = vec4(diffuseColor + ambientColor + specularColor, 1);

	/*
	fragPos = p;
	N = gl_Normal;
	*/

    gl_Position = gl_ModelViewProjectionMatrix * vec4(inVertex, 1);
}

