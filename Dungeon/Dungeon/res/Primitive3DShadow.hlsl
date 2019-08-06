/**
*@file  Primitive3D.fx
*@brief グーロー実装予定、3Dプリミティブの描画処理
*/
/////////////////////////////////////////////
//  texture sampler
/////////////////////////////////////////////
SamplerState textureSampler[2] : register(s0);
Texture2D   diffuseTexture[2] : register(t0);


/////////////////////////////////////////////////////////////////////////////////////
// CBPerFrame constant buffer
/////////////////////////////////////////////////////////////////////////////////////
cbuffer CBPerFrame : register( b0 )
{
    matrix  World;       //!< ワールド行列.
    matrix  View;        //!< ビュー行列.
    matrix  Proj;        //!< 射影行列.
	matrix  matShadow;   //!< 射影行列.
	float4  faceNormal;  //!< 面法線
    float4  faceColor;   //!< 面カラー(diffuse)
    float4  litDir;      //!< ライト方向（Directional Light)
    float4  litDiffuseColor;    //!< 拡散ライトカラー
    float4  litAmbientColor;    //!< 環境ライトカラー
    float4  litSpecularColor;   //!< 鏡面反射光カラー
    float   litPower;           //!< 鏡面反射光パワー
    float   AlphaThreshold;     //!< アルファ閾値
};


/////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position  : POSITION;     //!< 位置座標です.
    float3 Normal    : NORMAL;       //!< 頂点法線ベクトル
    float2 texCoord1 : TEXCOORD0;
    float2 texCoord2 : TEXCOORD1;
};

/////////////////////////////////////////////////////////////////////////////////////
// PSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct GSPSInput
{
    float4 Position    : SV_POSITION;  //!< 位置座標です.
    float4 color       : DIFFUSE;      //!< 頂点カラー
    float2 texCoord1   : TEXCOORD0;    //!< uv1
    float3 texCoord2   : TEXCOORD1;    //!< uv2
    float3 viewPos     : TEXCOORD2;    //!< ビュー空間での座標
    float3 viewNormal  : TEXCOORD3;    //!< ビュー空間での法線
};

//-----------------------------------------------------------------------------------
//! @brief      頂点シェーダエントリーポイントです.
//-----------------------------------------------------------------------------------
GSPSInput VSFunc( VSInput input )
{
    GSPSInput output = (GSPSInput)0;

    float4 pos = float4( input.Position, 1.0f );

    // ワールド空間に変換.
    float4 worldPos = mul( World, pos );

    // ビュー空間に変換.
    float4 viewPos  = mul( View,  worldPos );

    // 射影空間に変換.
    float4 projPos  = mul( Proj,  viewPos );

    output.Position = projPos;

	float4 shadowPos = float4(input.Position, 1);
	shadowPos = mul( matShadow, shadowPos);
	shadowPos.xyz = mul(1.0f  / shadowPos.w, shadowPos.xyz);
	output.texCoord2 = shadowPos;
	//output.texCoord2.x = (shadowPos.x + 1.0f)*0.5f;
	//output.texCoord2.y = (1.0f - shadowPos.y)*0.5f;

    //  color
    float4 worldNormal = mul(World, float4(input.Normal, 0.0f));
    float4 viewNormal = mul( View, worldNormal );
    
    //float atten = dot(worldNormal, litDir);
    float4 color4;
    color4.xyz = faceColor.xyz;// *saturate(litDiffuseColor.xyz * atten);
    color4.w = faceColor.w;
    output.color      = color4;
    output.viewPos    = viewPos.xyz;
    output.viewNormal = viewNormal.xyz;
    output.texCoord1  = input.texCoord1;
    //output.texCoord2  = input.texCoord2;
    return output;
}

//-----------------------------------------------------------------------------------
//! @brief      ジオメトリシェーダエントリーポイントです.
//! @param [in] input   vertices of a triangle 
//! @param [in] stream  output stream
//-----------------------------------------------------------------------------------
[maxvertexcount(3)]
void GSFunc(triangle GSPSInput input[3], inout TriangleStream<GSPSInput> stream)
{
    GSPSInput output = (GSPSInput)0;
    float3  v1, v2;
    float3  facenormal;

	float3  shadownormal;
	float3    cancel = { 0,0,0 };
	float    a = 1.0f;
	float    b = 0.0f;


    v1 = input[1].viewPos - input[0].viewPos;
    v2 = input[2].viewPos - input[0].viewPos;
    facenormal = normalize(cross(v1, v2));

	v1 = input[1].texCoord2 - input[0].texCoord2;
	v2 = input[2].texCoord2 - input[0].texCoord2;
	shadownormal = normalize(cross(v1, v2));

	a = (shadownormal.z > 0) ? 0.0f : 1.0f;
	b = (shadownormal.z > 0) ? 1.0f : 0.0f;

	float3 uv;
	uv.x = (input[0].texCoord2.x + 1.0f)*0.5f;
	uv.y = (1.0f - input[0].texCoord2.y)*0.5f;
	uv.z = input[0].texCoord2.z;
	cancel.z = uv.z;
    
    output.Position   = input[0].Position;
    output.color      = input[0].color;
    output.texCoord1  = input[0].texCoord1;
    output.texCoord2  = mul(uv, a) + mul(cancel, b);
    output.viewPos    = input[0].viewPos;
    output.viewNormal = facenormal;
    stream.Append(output);

	uv.x = (input[1].texCoord2.x + 1.0f)*0.5f;
	uv.y = (1.0f - input[1].texCoord2.y)*0.5f;
	uv.z = input[1].texCoord2.z;
	cancel.z = uv.z;



    output.Position   = input[1].Position;
    output.color      = input[1].color;
    output.texCoord1  = input[1].texCoord1;
	output.texCoord2 = mul(uv, a) + mul(cancel, b);
    output.viewPos    = input[1].viewPos;
    output.viewNormal = facenormal;
    stream.Append(output);


	uv.x = (input[2].texCoord2.x + 1.0f)*0.5f;
	uv.y = (1.0f - input[2].texCoord2.y)*0.5f;
	uv.z = input[2].texCoord2.z;
	cancel.z = uv.z;




    output.Position   = input[2].Position;
    output.color      = input[2].color;
    output.texCoord1  = input[2].texCoord1;
    output.texCoord2  = mul(uv, a) + mul(cancel, b);
    output.viewPos    = input[2].viewPos;
    output.viewNormal = facenormal;
    stream.Append(output);

    stream.RestartStrip();
}

//------------------------------------------------------------------------------------
//! @brief      ピクセルシェーダエントリーポイントです.
//------------------------------------------------------------------------------------
float4 PSFunc( GSPSInput output ) : SV_TARGET0
{
	float4 bmp = diffuseTexture[0].Sample(textureSampler[0], output.texCoord1);
	float4 shadow = diffuseTexture[1].Sample(textureSampler[1], output.texCoord2.xy);
	//  シャドウテクスチャのZ座標が、描画モデルのZ座標より手前なら
	//  影を描画する
	if (shadow.x < (output.texCoord2.z - 0.0001f)) {
		bmp *= float4(0.25f, 0.25f, 0.25f, 1);
	}

	//bmp *= diffuseTexture[1].Sample(textureSampler[1], output.texCoord2);
	//! ライトの合成
	float4 pixel = float4(output.color.xyz, output.color.w);

	//  per pixel lighting
	float3 lit_dir;
	lit_dir = (float3)mul(View, float4(litDir.xyz,0));
	//float3 normal = output.viewNormal.xyz;
	float3 normal = normalize(output.viewNormal.xyz);
	float atten = saturate(dot(lit_dir, normal));
	pixel.xyz *= (litDiffuseColor.xyz * atten);
	//pixel.xyz = saturate(pixel.xyz + litAmbientColor.xyz);

	//  specular
	float3 hv = normalize(-output.viewPos.xyz) + lit_dir;
	hv = normalize(hv*0.5f);    //  half vector
	atten = pow(saturate(dot(hv, normal)), litPower);
	pixel.xyz += litSpecularColor.xyz * atten;

	pixel.xyz = saturate(pixel.xyz + litAmbientColor.xyz);
	pixel.xyz *= bmp.xyz;

	//  最終出力
	if (pixel.w < 0.1f)
		discard;
	return pixel;
}



//------------------------------------------------------------------------------------
//! @brief      ピクセルシェーダエントリーポイントです.
//------------------------------------------------------------------------------------
float4 PSFuncDepthShadow(GSPSInput output) : SV_TARGET0
{
	float4 pixel = float4(output.Position.z,0,0,1.0f);
	pixel.x /= output.Position.w;
	return pixel;
}




//------------------------------------------------------------------------------------
//! @brief      ピクセルシェーダエントリーポイントです.
//------------------------------------------------------------------------------------
float4 PSFuncNoTex( GSPSInput output ) : SV_TARGET0
{
	//! ライトの合成
	float3 diffuse = output.color.xyz;
	float4 pixel = float4(diffuse, output.color.w);

	//  per pixel lighting
	float3 lit_dir;
	lit_dir = (float3)mul(View, float4(litDir.xyz,0));
	//float3 normal = output.viewNormal.xyz;
	float3 normal = normalize(output.viewNormal.xyz);
	float atten = saturate(dot(lit_dir, normal));
	pixel.xyz *= (litDiffuseColor.xyz * atten);
	//pixel.xyz = saturate(pixel.xyz + litAmbientColor.xyz);

	//  specular
	float3 hv = normalize(-output.viewPos.xyz) + lit_dir;
	hv = normalize(hv*0.5f);    //  half vector
	atten = pow(saturate(dot(hv, normal)), litPower);
	pixel.xyz += (float3)litSpecularColor * atten;

	pixel.xyz = saturate(pixel.xyz + litAmbientColor.xyz);

	//  最終出力
	if (pixel.w < 0.1f)
		discard;
	return pixel;
}