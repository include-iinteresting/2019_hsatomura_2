/**
*@file  PolygonFT4.fx
*@brief グーローなし、４頂点ポリゴンの描画処理
*/
/////////////////////////////////////////////
//  texture sampler
/////////////////////////////////////////////
SamplerState textureSampler : register(s0);
Texture2D	diffuseTexture : register(t0);


/////////////////////////////////////////////////////////////////////////////////////
// CBPerFrame constant buffer
/////////////////////////////////////////////////////////////////////////////////////
cbuffer CBPerFrame : register( b0 )
{
    matrix  World;       //!< ワールド行列.
    matrix  View;        //!< ビュー行列.
    matrix  Proj;        //!< 射影行列.
	float4  faceNormal;  //!< 面法線
    float4  faceColor;   //!< 面カラー(diffuse)
	float4  spotLitPos;  //!< スポットライトの位置(World座標系)
	float4  spotLitDir;  //!< 有向光源の向き(World座標系)
	float4  spotLitColor;//!< 有向光源の輝度(World座標系)
	float   spotLitAngle;           //!< スポットライトのコーンの頂角
	float   spotLitEdgeFalloff;     //!< コーンを外れたときの減衰量
	float   spotLitDistanceLimit;   //!< スポットライトの到達距離
	float   spotLitDistanceFalloff; //!< スポットライトが到達距離より離れたときの減衰量

	float4  fogColor;               //!< フォグカラー
	float   fogStart;               //!< Zフォグスタート
	float   fogFalloff;             //!< Zフォグ減衰量
	float   AlphaThreshold;         //!< アルファ閾値
};


/////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;     //!< 位置座標です.
	float2 texCoord : TEXCOORD0;
};

/////////////////////////////////////////////////////////////////////////////////////
// PSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct GSPSInput
{
    float4 Position    : SV_POSITION;  //!< 位置座標です.
    float4 color       : DIFFUSE;      //!< 頂点カラー
    float2 texCoord    : TEXCOORD0;    //!< uv
	float3 worldPos    : TEXCOORD1;    //!< ワールド空間での座標
	float3 worldNormal : TEXCOORD2;    //!< ワールド空間での法線
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

	//  color
	float4 worldNormal = mul ( World, float4( faceNormal.xyz, 0.0f ) );

	//  フォグの計算
	float fog = (viewPos.z - fogStart) * fogFalloff;
	fog = clamp(fog, 0, 1.0f);


	output.color = faceColor.xyzw * (1.0f - fog) + fogColor * fog;
	output.worldPos = worldPos.xyz;
	output.worldNormal = worldNormal.xyz;
	output.texCoord = input.texCoord;
    return output;
}

//------------------------------------------------------------------------------------
//! @brief      ピクセルシェーダエントリーポイントです.
//------------------------------------------------------------------------------------
float4 PSFunc( GSPSInput output ) : SV_TARGET0
{
	float4 bmp = diffuseTexture.Sample(textureSampler, output.texCoord);
	float3 diffuse;


	//! スポットライトの計算
    float3 diffuse2 = float3(1.0f, 1.0f, 1.0f);
    float3 lit_dir = output.worldPos.xyz - spotLitPos.xyz;
    float  distance = length(lit_dir);

    lit_dir /= distance;
    float3 lit_dir2 = spotLitDir.xyz;
    float  LdotN = dot(lit_dir, lit_dir2);
    LdotN -= spotLitAngle;
    LdotN *= spotLitEdgeFalloff;
    LdotN = saturate(LdotN + 1.0f);
    LdotN *= dot(-lit_dir, faceNormal);

    float atten = 1.0f - saturate((distance- spotLitDistanceLimit) * spotLitDistanceFalloff);
    diffuse2 = saturate((atten * LdotN) * spotLitColor.xyz);
 
	//	ピクセル値の計算
	diffuse = output.color.xyz + diffuse2;
	float4 pixel = float4(diffuse, output.color.w);
	pixel = pixel * bmp;

	//  最終出力
	if (pixel.w < 0.1f)
		discard;
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

	//  最終出力
	if (pixel.w < 0.1f)
		discard;
	return pixel;
}

