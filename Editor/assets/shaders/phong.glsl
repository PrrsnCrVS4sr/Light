#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec3 a_Normal;

out vec4 v_color;
out vec3 v_normal;
out vec3 v_worldPos;

uniform mat4 u_viewProjectionMatrix;
uniform mat4 lightSpaceMatrix;
uniform mat4 u_transform; //model 
uniform mat3 u_normal;


void main()
{
	gl_Position = u_viewProjectionMatrix *  u_transform * vec4(a_Position, 1.0);
	v_color = a_Color;
	v_normal = u_normal * a_Normal;
	v_worldPos = vec3(u_transform * vec4(a_Position, 1.0));
}

#type fragment
#version 330 core

in vec3 v_normal;
in vec4 v_color;
in vec3 v_worldPos;
layout(location = 0) out vec4 color;
layout(location = 1) out int entity;

struct PointLight
{
	vec4 position;
	vec4 color;
	float range;
};
struct SpotLight
{
		vec4 position;
		vec4 color;
		vec4 direction;
		float innerCutoff;
		float outerCutoff;
		float range;
};

struct DirectionalLight
{	
	vec4 position;
	vec4 direction;
	vec4 color;

};

uniform sampler2DArray depthMap;
uniform PointLight u_pointLights[4];
uniform SpotLight u_spotLights[4];
uniform DirectionalLight u_directionalLights[4];
uniform vec3 cameraPosition;
uniform int u_numPointLights;
uniform int u_numSpotLights;
uniform int u_id;
uniform int u_selectionId;


uniform float farPlane;

uniform mat4 view;


uniform mat4 lightSpaceMatrices[16];

uniform float cascadePlaneDistances[16];
uniform int cascadeCount;

float shadowCalculate( vec4 lightDirection, vec3 norm)
{	


    vec4 fragPosViewSpace = view * vec4(v_worldPos, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(v_worldPos, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0)
    {
        return 0.0;
    }
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(norm);
	vec3 lightDir = normalize(lightDirection.xyz);
	
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    const float biasModifier = 0.5f;
    if (layer == cascadeCount || layer == cascadeCount -1)
    {
        bias *= 1 / (farPlane * biasModifier);
    }
    else
    {
        bias *= 1 / (cascadePlaneDistances[layer] * biasModifier);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(depthMap, 0));
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {   

            float pcfDepth = texture(depthMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
 
}

vec4 pointLightCalculate(PointLight light, vec3 norm, vec3 viewDir)
{
	float distance = length(light.position.xyz - v_worldPos);
	vec3 lightDir = (light.position.xyz - v_worldPos) / distance;

	float attentuation = clamp(1 - (distance * distance)/(light.range * light.range), 0.0 , 1.0);

	float diff = max(dot(norm, lightDir), 0.0);
	vec4 diffuse = diff * light.color;

	//specular
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(norm, viewDir), 0.0), 64);
	vec4 specular = spec * light.color;
	diffuse = diffuse * attentuation;
	specular = specular * attentuation;
	vec4 result = diffuse + specular;
	return result;
}

vec4 directionalLightCalculate(DirectionalLight light, vec3 norm, vec3 viewDir)
{	
	vec4 color = vec4(light.color);
	vec3 lightDir = normalize(light.direction.xyz);
	float diff = max(dot(norm, lightDir), 0.0);
	vec4 diffuse = diff * color;


	//specular
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(halfwayDir, norm), 0.0), 64);
	vec4 specular = spec * color;
	float shadow = shadowCalculate(light.direction, norm);
	vec4 result = (1.0f - shadow)*(diffuse + specular);
	return result;
}

vec4 spotLightCalculate(SpotLight light, vec3 norm, vec3 viewDir)
{
	float distance = length(light.position.xyz - v_worldPos);
	vec3 lightDir = normalize(light.position.xyz - v_worldPos);
	vec4 result;

	
	float attentuation = clamp(1 - distance/light.range, 0.0 , 1.0);
	float theta = dot(lightDir, light.direction.xyz);
	if (theta > light.outerCutoff)
	{	
		float intensity = clamp((theta - light.outerCutoff) / (light.innerCutoff - light.outerCutoff), 0.0, 1.0);
		float diff = max(dot(norm, lightDir), 0.0);
		vec4 diffuse = diff * light.color;
		vec3 halfwayDir = normalize(lightDir + viewDir);
		float spec = pow(max(dot(halfwayDir, norm), 0.0), 64);
		vec4 specular = spec * light.color;
		diffuse = diffuse * intensity * attentuation;
		specular = specular * intensity * attentuation;
		result = diffuse + specular;
	} else
	{
		result = vec4(0.0 , 0.0 , 0.0 , 0.0);
	}
	
	
	
	return result;
}


void main()
{	
	
	vec3 norm = normalize(v_normal);
	vec3 viewDir = normalize(cameraPosition - v_worldPos);
	color = vec4(0.3, 0.3, 0.3, 1.0);
	// for (int i = 0; i < u_numPointLights; i++)
	// {
	// 	color += vec4(pointLightCalculate(u_pointLights[i]), 0.0);
	// }
	color += pointLightCalculate(u_pointLights[0], norm, viewDir);
	color += pointLightCalculate(u_pointLights[1], norm, viewDir);
	color += pointLightCalculate(u_pointLights[2], norm, viewDir);
	color += pointLightCalculate(u_pointLights[3], norm, viewDir);
	color += spotLightCalculate(u_spotLights[0], norm, viewDir);
	color += spotLightCalculate(u_spotLights[1], norm, viewDir);
	color += spotLightCalculate(u_spotLights[2], norm, viewDir);
	color += spotLightCalculate(u_spotLights[3], norm, viewDir);
	color += directionalLightCalculate(u_directionalLights[0], norm, viewDir);
	color += directionalLightCalculate(u_directionalLights[1], norm, viewDir);
	color += directionalLightCalculate(u_directionalLights[2], norm, viewDir);
	color += directionalLightCalculate(u_directionalLights[3], norm, viewDir);
	color.a = 1.0;
	color *= v_color;
	entity = u_id;
}
