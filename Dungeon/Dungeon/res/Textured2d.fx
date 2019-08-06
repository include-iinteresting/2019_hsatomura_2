//-----------------------------------------------------------------------------------
//! texture sampler
//-----------------------------------------------------------------------------------
SamplerState textureSampler    : register(s0);
Texture2D    diffuseTexture[1] : register(t0);

//-----------------------------------------------------------------------------------
//! Constant buffer
//-----------------------------------------------------------------------------------
cbuffer VSConstantBuffer : register(b0)
{
	matrix  Proj;   //!< 射影行列.
	float4  Color;  //!< モジュレ―トする色
};

cbuffer PSConstantBuffer : register(b0)
{
	float4  PSColor;    //  アルファ
}
//-----------------------------------------------------------------------------------
//! VSInput structure
//-----------------------------------------------------------------------------------
struct VSInput
{
	float3 Position : POSITION;     //!<  位置座標
	float2 texCoord : TEXCOORD0;    //!<  テクスチャ座標
};

//-----------------------------------------------------------------------------------
//! GSPSInput structure
//-----------------------------------------------------------------------------------
struct GSPSInput
{
	float4 Position : SV_POSITION;  //!<  位置座標
	float2 texCoord : TEXCOORD0;    //!<  テクスチャ座標
	float4 diffuse  : COLOR;        //!<  文字色
};

//-----------------------------------------------------------------------------------
//  頂点シェーダエントリーポイント
//-----------------------------------------------------------------------------------
GSPSInput VSFunc(VSInput input)
{
	GSPSInput output = (GSPSInput)0;

	//! 入力データをfloat4 へ変換.
	float4 pos = float4(input.Position, 1.0f);

	//! 射影空間に変換.
	float4 projPos = mul(Proj, pos);

	output.Position = projPos;
	output.texCoord = input.texCoord;
	output.diffuse = Color;
	return output;
}

//------------------------------------------------------------------------------------
//! ピクセルシェーダエントリーポイント・テクスチャあり
//------------------------------------------------------------------------------------
float4 PSFunc(GSPSInput psin) : SV_TARGET0
{
	float4  pixel;
	pixel = diffuseTexture[0].Sample(textureSampler, psin.texCoord);
    pixel *= psin.diffuse * pixel;
    return pixel;
}
//------------------------------------------------------------------------------------
//!    ピクセルシェーダエントリーポイント・テクスチャなし
//------------------------------------------------------------------------------------
float4 PSFuncNoTex(GSPSInput psin) : SV_TARGET0
{
	return psin.diffuse;
}