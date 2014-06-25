struct VSOutput
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};

cbuffer Frame0: register(b0)
{
	struct SVideoShaderData
	{
		float2 scale;
		float2 offset;
		float fading;
		
		float pad[3];
	} vsd0;
};

//SVideoShaderData Frame0: register(b0);
SamplerState samp : register(s0);
Texture2D<float4> Input : register(t0);

float4 main(VSOutput Index) : SV_Target0
{
	float4 color = Input.Sample(samp, Index.Tex) * vsd0.fading;
	//color.rgb = 1.0 - color.rgb;
	
	return color;
}