#version 410

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

uniform sampler2D s_NoTextures;
uniform sampler2D s_Diffuse;
uniform sampler2D s_Diffuse2;
uniform sampler2D s_Specular;

uniform vec3  u_AmbientCol;
uniform float u_AmbientStrength;

uniform vec3  u_LightPos;
uniform vec3  u_LightCol;
uniform float u_AmbientLightStrength;
uniform float u_SpecularLightStrength;
uniform float u_Shininess;
// NEW in week 7, see https://learnopengl.com/Lighting/Light-casters for a good reference on how this all works, or
// https://developer.valvesoftware.com/wiki/Constant-Linear-Quadratic_Falloff
uniform float u_LightAttenuationConstant;
uniform float u_LightAttenuationLinear;
uniform float u_LightAttenuationQuadratic;
uniform bool u_NoLight;
uniform bool u_AmbientOnly;
uniform bool u_SpecularOnly;
uniform bool u_AmbientAndSpecularLight;
uniform bool u_UseTextures;
uniform bool u_UseDiffuseRamp;
uniform bool u_UseSpecularRamp;

uniform sampler2D s_DiffuseRamp;
uniform sampler2D s_SpecularRamp;

uniform float u_TextureMix;

uniform vec3  u_CamPos;

out vec4 frag_color;

// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
void main() {
	// Lecture 5
	vec3 ambient;
	if (u_SpecularOnly || u_NoLight)
	{
		ambient = 0.0 * u_LightCol;
	}
	else
	{
		ambient = u_AmbientLightStrength * u_LightCol;
	}
	//vec3 ambient = u_AmbientLightStrength * u_LightCol;

	// Diffuse
	vec3 N = normalize(inNormal);
	vec3 lightDir = normalize(u_LightPos - inPos);

	float dif = max(dot(N, lightDir), 0.0);

	vec3 diffuse = dif * u_LightCol;// add diffuse intensity
	vec4 diffuseRamped = texture(s_DiffuseRamp, vec2(0.0, diffuse));
	vec3 finalDiffuse;
	if (u_UseDiffuseRamp)
	{
		finalDiffuse = diffuseRamped.rgb;
	}
	else
	{
		finalDiffuse = diffuse;
	}

	//Attenuation
	float dist = length(u_LightPos - inPos);
	float attenuation = 1.0f / (
		u_LightAttenuationConstant + 
		u_LightAttenuationLinear * dist +
		u_LightAttenuationQuadratic * dist * dist);

	// Specular
	vec3 viewDir  = normalize(u_CamPos - inPos);
	vec3 h        = normalize(lightDir + viewDir);

	// Get the specular power from the specular map
	float texSpec;
	if (u_UseTextures)
	{
		texSpec = texture(s_Specular, inUV).x;
	}
	else
	{
		texSpec = texture(s_NoTextures, inUV).x;
	}
	float spec = pow(max(dot(N, h), 0.0), u_Shininess); // Shininess coefficient (can be a uniform)

	vec3 specular;
	vec4 specularRamped;
	vec3 finalSpecular;
	if (u_AmbientOnly || u_NoLight)
	{
		specular = 0.0 * u_LightCol; // Can also use a specular color
		finalSpecular = specular;
	}
	else
	{
		specular = u_SpecularLightStrength * texSpec * spec * u_LightCol; // Can also use a specular color
		specularRamped = texture(s_SpecularRamp, vec2(0.0, specular));
		if (u_UseSpecularRamp)
		{
			finalSpecular = specularRamped.rgb;
		}
		else
		{
			finalSpecular = specular;
		}
	}
	//vec3 specular = u_SpecularLightStrength * texSpec * spec * u_LightCol; // Can also use a specular color

	// Get the albedo from the diffuse / albedo map
	vec4 textureColor1;
	vec4 textureColor2;
	vec4 textureColor;

	if (u_UseTextures)
	{
		textureColor1 = texture(s_Diffuse, inUV);
		textureColor2 = texture(s_Diffuse2, inUV);
		textureColor = mix(textureColor1, textureColor2, u_TextureMix);
	}
	else
	{
		textureColor1 = texture(s_NoTextures, inUV);
		textureColor2 = texture(s_NoTextures, inUV);
		textureColor = mix(textureColor1, textureColor2, u_TextureMix);
	}

	vec3 result;
	if (u_SpecularOnly)
	{
		result = (
		(ambient + finalDiffuse + finalSpecular) * attenuation // light factors from our single light
		) * inColor * textureColor.rgb; // Object color
	}
	else if (u_NoLight)
	{
		result = inColor * textureColor.rgb; // Object color
	}
	else
	{
		result = (
		(u_AmbientCol * u_AmbientStrength) + // global ambient light
		(ambient + finalDiffuse + finalSpecular) * attenuation // light factors from our single light
		) * inColor * textureColor.rgb; // Object color
	}
	frag_color = vec4(result, textureColor.a);
}