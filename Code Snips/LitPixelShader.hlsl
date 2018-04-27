Texture2D pTex : register(t0);
sampler samp : register(s0);

struct Material
{
    float4 emissive;
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float specPower;
    bool textured;
    float2 pad;
};

cbuffer MaterialProperties : register(b0)
{
    Material mat;
}

struct Light
{
    float4 pos;
    float4 dir;
    float4 col;
    float4 pad;
};

cbuffer LightProperties : register(b1)
{
    float4 Position;
    float4 Ambient;
    Light light;
}

struct PixelShaderInput
{
    float4 position : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float2 uv : TEXCOORD0;
    float4 Position : SV_Position;
};

struct LightInformation
{
    float4 diffuse;
    float specular;
};

float CalculateSpecular(Light l, float3 vNorm, float3 norm, float3 dir)
{
    float specIntent = 0.0f;
    if (dot(dir, norm) > 0.0f)
    {
        float3 refl = normalize(reflect(-dir, norm));
        float spec = saturate(dot(refl, vNorm));
        specIntent = pow(spec, mat.specPower);
    }

    return l.col * specIntent;
}

LightInformation CalculateDirectional(PixelShaderInput input)
{
    LightInformation lamp;

    float3 viewNormalized = normalize((Position - input.position)).xyz;
    float3 lightVec = -light.pos.xyz;

    float ratio = clamp(dot(-light.dir.xyz, input.normal), 0, 1);
    lamp.diffuse = (light.col * ratio);
    lamp.specular = CalculateSpecular(light, viewNormalized, input.normal.xyz, -light.dir.xyz);

    return lamp;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    LightInformation lamp = CalculateDirectional(input);

    float4 dirResult;
    float4 dirLight;
    float3 lightDir = -normalize(light.dir.xyz);
    float3 worldNorm = normalize(input.normal);
    float intensity = saturate(dot(lightDir, worldNorm));


    float4 emis = mat.emissive;
    float4 amb = mat.ambient;
    float4 diff = mat.diffuse * lamp.diffuse;
    float4 spec = mat.specular * lamp.specular;
    float4 tex = float4(1.0f, 1.0f, 1.0f, 1.0f);

    if (mat.textured)
    {
        tex = pTex.Sample(samp, input.uv);
        amb = tex.xyzw * float4(0.75, 0.75, 0.75, 1.0);
    }

    float4 col = (emis + amb + diff + spec) * tex;

    dirLight = intensity * light.col * col;
    dirResult = saturate(dirLight + amb);

    return saturate(dirResult);
}