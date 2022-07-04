#type vertex
#version 330 core
// depth.glsl
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec2 a_Texcoord;

// uniform mat4 u_lightSpaceMatrix;
uniform mat4 u_transform;

void main()
{
	gl_Position = u_transform * vec4(a_Position, 1.0);
}

#type geometry
#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 21) out;


uniform	mat4 u_lightSpaceMatrices[16];


void main()
{
	for (int i = 0; i < 7; ++i)
	{
		for (int j = 0; j < gl_in.length(); j++)
		{
			gl_Position = u_lightSpaceMatrices[i] * gl_in[j].gl_Position;
			gl_Layer = i;
			EmitVertex();
		}
		EndPrimitive();
	}
	
}

#type fragment
#version 330 core
//depth.glsl

void main()
{	
	// gl_FragDepth = gl_FragCoord.z;
}