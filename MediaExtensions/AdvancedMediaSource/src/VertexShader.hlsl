
cbuffer VSTransformBuffer: register(b0)
{
    matrix transform;
};

struct VSInput
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;              
    float2 Tex : TEXCOORD0;
};

VSOutput VSMain( VSInput Input )
{
    VSOutput Output;
    Output.Pos = mul(Input.Pos, transform);
    Output.Tex = Input.Tex;
    return Output;
}
