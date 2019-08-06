/**
 *@file   IPolygonFT4.h
 *@brief  �S���_���ʃ|���S����`�悷��N���X�̐錾
 */
#pragma once

#include "IDeviceDependentObject.h"

 //  Forward declaration
struct ID3D11DeviceContext;

namespace DirectX {
	struct XMFLOAT2;
	struct XMFLOAT3;
	struct XMFLOAT4;
	struct XMMATRIX;
}
struct ID3D11ShaderResourceView;

/**
*@interface  ID3D11PolygonFT4
*@brief      �S���_���ʃ|���S����`�悷��N���X�ւ̃C���^�t�F�[�X
*/

class IPolygonFT4 : public IDeviceDependentObject {
public:
	virtual ~IPolygonFT4() = 0;  //  pure virtual destructor

	virtual void Render(ID3D11DeviceContext *pContext,
						const DirectX::XMFLOAT3 (&positions)[4], const DirectX::XMFLOAT2 (&texCoords)[4],
						const DirectX::XMFLOAT4 &color, ID3D11ShaderResourceView *pTex) = 0;

	virtual void SetWorldMatrix(DirectX::XMMATRIX &matWorld) = 0;   //!< ���[���h�s���ݒ肷��
	virtual void SetViewMatrix(DirectX::XMMATRIX  &matView) = 0;    //!< �r���[�s���ݒ肷��
	virtual void SetProjectionMatrix(DirectX::XMMATRIX &matProjection) = 0; //!< �v���W�F�N�V�����s���ݒ肷��

	virtual void SetSpotLight(DirectX::XMFLOAT4 &&pos, FLOAT angle, FLOAT angle2, DirectX::XMFLOAT3 &&normal, DirectX::XMFLOAT4 &&color, FLOAT sustain, FLOAT limit) = 0;
	virtual void SetLinearFog(FLOAT start, FLOAT end, DirectX::XMFLOAT4 &&color) = 0;
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext) = 0;
	virtual HRESULT ReleaseDeviceObjects() = 0;

	static IPolygonFT4 *Create();
};