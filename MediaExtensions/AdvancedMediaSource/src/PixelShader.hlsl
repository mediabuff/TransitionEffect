
cbuffer PSFadingBuffer: register(b0)
{
    float fading;
    float memory_pading[7];
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;              
    float2 Tex : TEXCOORD0;
};

SamplerState Samp : register( s0 );
Texture2D<float4> Tex2D : register( t0 );

float4 PSMain(PS_INPUT Input) : SV_Target0
{
    float4 color = Tex2D.Sample(Samp, Input.Tex);
    color.a = color.a * fading;
    return color;
}