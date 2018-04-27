


cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
};

#define MAX_BONES 50

cbuffer AnimationConstantBuffer : register(b1)
{
    matrix bones[MAX_BONES];
};


struct VertexPosNormBoneWeightUV
{
    float4 pos : POSITION;
    int4 bone : BLENDINDICES;
    float4 weight : BLENDWEIGHT;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 PositionT : TEXCOORD1;
    float3 Normal : TEXCOORD2;
    float2 UV : TEXCOORD0;
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPosNormBoneWeightUV input)
{
    VertexShaderOutput valr;

    float4 skinMat = mul(input.pos, bones[input.bone.x]) * input.weight.x;
    skinMat += mul(input.pos, bones[input.bone.y]) * input.weight.y;
    skinMat += mul(input.pos, bones[input.bone.z]) * input.weight.z;
    skinMat += mul(input.pos, bones[input.bone.w]) * input.weight.w;
                                                  
    float4 pos = skinMat;

    pos = mul(pos, model);
    pos.w = 1.0f;

    valr.PositionT = pos;

    pos = mul(pos, view);
    pos = mul(pos, projection);
    valr.Position = pos;


    float3 skinMatN = mul(input.norm,(float3x4) bones[input.bone.x]) * input.weight.x;
    skinMatN += mul(input.norm, (float3x4)bones[input.bone.y]) * input.weight.y;
    skinMatN += mul(input.norm, (float3x4)bones[input.bone.z]) * input.weight.z;
    valr.Normal = mul(skinMatN, (float3x3) model);

    valr.PositionT = mul(skinMat, model);
    valr.PositionT.w = 1.0f;
    valr.UV = input.uv;
    //mul(input.uv, skinMat);

    return valr;
}