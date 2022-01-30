#type vertex
#version 330 core

layout(location = 0) in vec2 a_Position;

out vec2 v_texcoord;

void main()
{
	v_texcoord = (a_Position + vec2(1.0,1.0))/2;
	gl_Position = vec4(a_Position, 0.0, 1.0);
}

#type fragment
#version 330 core

in vec2 v_texcoord;
layout(location = 0) out vec4 v_color;

uniform isampler2D IDTexture;

void main()
{
	int pixelId = texture(IDTexture, v_texcoord).r;

	bool nearby = false;
	bool border = false;

	for(int i = -5; i < 5; i++)
	{
		for(int j = -5; j < 5; j++)
		{
			if(texture(IDTexture, v_texcoord + vec2(i,j)/1000).r != 0)
				nearby = true;
			if(v_texcoord.x + float(i)/1000 <= 0 || v_texcoord.x + float(i)/1000 >= 1 || v_texcoord.y + float(j)/1000 <= 0 || v_texcoord.y + float(j)/1000 >= 1)
				border = true;
		}
	}

	if ((pixelId == 0 && nearby) || (pixelId != 0 && border))
		v_color = vec4(0.9, 0.6, 0.0, 1.0);
	else
		discard;
}