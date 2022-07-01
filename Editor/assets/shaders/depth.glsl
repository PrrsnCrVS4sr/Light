#type vertex
#version 330 core
layout (location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec3 a_Normal;

uniform mat4 model;

void main()
{
    gl_Position = model * vec4(a_Position, 1.0);
}

#type geometry
#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 21) out;


uniform	mat4 lightSpaceMatrices[16];


void main()
{
	for (int i = 0; i < 7; ++i)
	{
		for (int j = 0; j < gl_in.length(); j++)
		{
			gl_Position = lightSpaceMatrices[i] * gl_in[j].gl_Position;
			gl_Layer = i;
			EmitVertex();
		}
		EndPrimitive();
	}
	
}


#type fragment
#version 330 core
layout(location = 0) out vec4 color;
layout(location = 1) out int entity;


void main()
{             
    // gl_FragDepth = gl_FragCoord.z;

}