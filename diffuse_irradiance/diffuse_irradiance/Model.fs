#version 330 core

// material parameters
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

//uniform sampler2D albedoTexture;
//uniform sampler2D metallicTexture;
//uniform sampler2D roughnessTexture;
//uniform sampler2D aoTexture;
//uniform sampler2D normalTexture;
uniform samplerCube irradianceMap;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 cameraPosition;

const float PI = 3.14159265359;

in VS_OUT {
	vec3 FragPos;
    vec3 Normal;
    vec2 Uv;
	mat3 TBN;
} fs_in;

out vec4 FragColor;

// ----------------------------------------------------------------------------
// GGX / Trowbridge-Reitz
// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
float D_GGX( float a2, float NoH )
{
    float d = ( NoH * a2 - NoH ) * NoH + 1; // 2 mad
    return a2 / ( PI*d*d );         // 4 mul, 1 rcp
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	// Specular D
    // chrome-extension://bocbaocobfecmglnmeaeppambideimao/pdf/viewer.html?file=https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
    // α=Roughness²
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom; // prevent divide by zero for roughness=0.0 and NdotH=1.0
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
} 

void main()
{
	//// 需要注意的是一般来说反射率(albedo)纹理在美术人员创建的时候就已经在sRGB空间了，因此我们需要在光照计算之前先把他们转换到线性空间
	//vec3 albedo = pow(texture(albedoTexture, fs_in.Uv).rgb, vec3(2.2));
	//// 金属性(Metallic)和粗糙度(Roughness)贴图大多数时间都会保证在线性空间中
	//float metallic = texture(metallicTexture, fs_in.Uv).r;
	//float roughness = texture(roughnessTexture, fs_in.Uv).r;
	//// 一般来说，环境光遮蔽贴图(ambient occlusion maps)也需要我们转换到线性空间。
	//// float ao = texture(aoTexture, fs_in.Uv).r;
	//float ao = pow(texture(aoTexture, fs_in.Uv).r, 2.2);
	//vec3 N = normalize(fs_in.TBN * ((texture(normalTexture, fs_in.Uv).rgb) * 2.0 - 1.0));

	vec3 N = normalize(fs_in.Normal);
    vec3 V = normalize(cameraPosition - fs_in.FragPos);
    // 验证法线是否正确
    //FragColor = vec4(lightColor * (1.0f - dot(N,V)), 1);
    vec3 L = normalize(lightPosition - fs_in.FragPos);
    vec3 H = normalize(V + L);

    // 目的：为了非金属和金属用同一个公式
    // F0基础反射率/法向入射（利用所谓折射指数(Indices of Refraction)或者说IOR计算得出）
    // 非金属F0 ≈ 0.04，金属F0 = 反照率（表面颜色），中间的话插值
	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, albedo, metallic);

	// 辐射率
	// 原始的辐射率方程: dΦ*dΦ / (dA*dw*cosθ), θ为n.L夹角，等同于渲染方程中的n*wi
	// 多了一项距离衰减的系数
	// 对于直接点光源的情况，辐射率函数L先获取光源的颜色值，然后光源和某点p的距离衰减，接着按照n⋅wi缩放，
	// 但是仅仅有一条入射角为wi的光线打在点p上， 这个wi同时也等于在p点光源的方向向量
	float distance = length(lightPosition - fs_in.FragPos);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = lightColor * attenuation;

	// BRDF
	float NDF = DistributionGGX(N, H, roughness);
	//float NDF = D_GGX(roughness*roughness, max(dot(N, H), 0.0));
	vec3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
    float G   = GeometrySmith(N, V, L, roughness);

    vec3 nominator    = NDF * G * F; 
    float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    // 把kD乘以金属的反比，这样只有非金属才有漫射光，如果部分金属(纯金属没有漫射光)就有线性混合光
    kD *= 1.0 - metallic; // 金属不会折射(没有漫反射)。
    //vec3 kD = mix(vec3(1.0f) - F, vec3(0.0), metallic);
    //vec3 diffuse = kD * albedo;

    float NdotL = max(dot(N, L), 0.0);

	// 直接光照
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

	// 使用IBL作为环境光（间接光照）
    vec3 ambient;
	//ambient = vec3(0.03) * albedo * ao;
	{
		//vec3 kS = fresnelSchlick(max(dot(N, V), 0.0), F0);
		vec3 kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
		vec3 kD = 1.0 - kS;
		kD *= 1.0 - metallic;	  
		vec3 irradiance = texture(irradianceMap, N).rgb;
		vec3 diffuse = irradiance * albedo;
		ambient = (kD * diffuse) * ao;
	}
    vec3 color = ambient + Lo;
	// HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));

	FragColor = vec4(color,1);
}