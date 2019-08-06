/**
*@file  PolygonFT4.fx
*@brief �O�[���[�Ȃ��A�S���_�|���S���̕`�揈��
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
    matrix  World;       //!< ���[���h�s��.
    matrix  View;        //!< �r���[�s��.
    matrix  Proj;        //!< �ˉe�s��.
	float4  faceNormal;  //!< �ʖ@��
    float4  faceColor;   //!< �ʃJ���[(diffuse)
	float4  spotLitPos;  //!< �X�|�b�g���C�g�̈ʒu(World���W�n)
	float4  spotLitDir;  //!< �L�������̌���(World���W�n)
	float4  spotLitColor;//!< �L�������̋P�x(World���W�n)
	float   spotLitAngle;           //!< �X�|�b�g���C�g�̃R�[���̒��p
	float   spotLitEdgeFalloff;     //!< �R�[�����O�ꂽ�Ƃ��̌�����
	float   spotLitDistanceLimit;   //!< �X�|�b�g���C�g�̓��B����
	float   spotLitDistanceFalloff; //!< �X�|�b�g���C�g�����B������藣�ꂽ�Ƃ��̌�����

	float4  fogColor;               //!< �t�H�O�J���[
	float   fogStart;               //!< Z�t�H�O�X�^�[�g
	float   fogFalloff;             //!< Z�t�H�O������
	float   AlphaThreshold;         //!< �A���t�@臒l
};


/////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;     //!< �ʒu���W�ł�.
	float2 texCoord : TEXCOORD0;
};

/////////////////////////////////////////////////////////////////////////////////////
// PSInput structure
/////////////////////////////////////////////////////////////////////////////////////
struct GSPSInput
{
    float4 Position    : SV_POSITION;  //!< �ʒu���W�ł�.
    float4 color       : DIFFUSE;      //!< ���_�J���[
    float2 texCoord    : TEXCOORD0;    //!< uv
	float3 worldPos    : TEXCOORD1;    //!< ���[���h��Ԃł̍��W
	float3 worldNormal : TEXCOORD2;    //!< ���[���h��Ԃł̖@��
};

//-----------------------------------------------------------------------------------
//! @brief      ���_�V�F�[�_�G���g���[�|�C���g�ł�.
//-----------------------------------------------------------------------------------
GSPSInput VSFunc( VSInput input )
{
    GSPSInput output = (GSPSInput)0;

    float4 pos = float4( input.Position, 1.0f );

    // ���[���h��Ԃɕϊ�.
    float4 worldPos = mul( World, pos );

    // �r���[��Ԃɕϊ�.
    float4 viewPos  = mul( View,  worldPos );

    // �ˉe��Ԃɕϊ�.
    float4 projPos  = mul( Proj,  viewPos );

    output.Position = projPos;

	//  color
	float4 worldNormal = mul ( World, float4( faceNormal.xyz, 0.0f ) );

	//  �t�H�O�̌v�Z
	float fog = (viewPos.z - fogStart) * fogFalloff;
	fog = clamp(fog, 0, 1.0f);


	output.color = faceColor.xyzw * (1.0f - fog) + fogColor * fog;
	output.worldPos = worldPos.xyz;
	output.worldNormal = worldNormal.xyz;
	output.texCoord = input.texCoord;
    return output;
}

//------------------------------------------------------------------------------------
//! @brief      �s�N�Z���V�F�[�_�G���g���[�|�C���g�ł�.
//------------------------------------------------------------------------------------
float4 PSFunc( GSPSInput output ) : SV_TARGET0
{
	float4 bmp = diffuseTexture.Sample(textureSampler, output.texCoord);
	float3 diffuse;


	//! �X�|�b�g���C�g�̌v�Z
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
 
	//	�s�N�Z���l�̌v�Z
	diffuse = output.color.xyz + diffuse2;
	float4 pixel = float4(diffuse, output.color.w);
	pixel = pixel * bmp;

	//  �ŏI�o��
	if (pixel.w < 0.1f)
		discard;
	return pixel;
}

//------------------------------------------------------------------------------------
//! @brief      �s�N�Z���V�F�[�_�G���g���[�|�C���g�ł�.
//------------------------------------------------------------------------------------
float4 PSFuncNoTex( GSPSInput output ) : SV_TARGET0
{
	//! ���C�g�̍���
	float3 diffuse = output.color.xyz;
	float4 pixel = float4(diffuse, output.color.w);

	//  �ŏI�o��
	if (pixel.w < 0.1f)
		discard;
	return pixel;
}

