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
	matrix  Proj;   //!< �ˉe�s��.
	float4  Color;  //!< ���W�����\�g����F
};

cbuffer PSConstantBuffer : register(b0)
{
	float4  PSColor;    //  �A���t�@
}
//-----------------------------------------------------------------------------------
//! VSInput structure
//-----------------------------------------------------------------------------------
struct VSInput
{
	float3 Position : POSITION;     //!<  �ʒu���W
	float2 texCoord : TEXCOORD0;    //!<  �e�N�X�`�����W
};

//-----------------------------------------------------------------------------------
//! GSPSInput structure
//-----------------------------------------------------------------------------------
struct GSPSInput
{
	float4 Position : SV_POSITION;  //!<  �ʒu���W
	float2 texCoord : TEXCOORD0;    //!<  �e�N�X�`�����W
	float4 diffuse  : COLOR;        //!<  �����F
};

//-----------------------------------------------------------------------------------
//  ���_�V�F�[�_�G���g���[�|�C���g
//-----------------------------------------------------------------------------------
GSPSInput VSFunc(VSInput input)
{
	GSPSInput output = (GSPSInput)0;

	//! ���̓f�[�^��float4 �֕ϊ�.
	float4 pos = float4(input.Position, 1.0f);

	//! �ˉe��Ԃɕϊ�.
	float4 projPos = mul(Proj, pos);

	output.Position = projPos;
	output.texCoord = input.texCoord;
	output.diffuse = Color;
	return output;
}

//------------------------------------------------------------------------------------
//! �s�N�Z���V�F�[�_�G���g���[�|�C���g�E�e�N�X�`������
//------------------------------------------------------------------------------------
float4 PSFunc(GSPSInput psin) : SV_TARGET0
{
	float4  pixel;
	pixel = diffuseTexture[0].Sample(textureSampler, psin.texCoord);
    pixel *= psin.diffuse * pixel;
    return pixel;
}
//------------------------------------------------------------------------------------
//!    �s�N�Z���V�F�[�_�G���g���[�|�C���g�E�e�N�X�`���Ȃ�
//------------------------------------------------------------------------------------
float4 PSFuncNoTex(GSPSInput psin) : SV_TARGET0
{
	return psin.diffuse;
}