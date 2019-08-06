#pragma once

class IPrimitive3DShadow;
class CFloor
{
public:
	CFloor();
	~CFloor();
	inline void SetWorldMatrix(DirectX::XMMATRIX &matWorld) {   //!< ���[���h�s���ݒ肷��
		SetWorldMatrix(static_cast<DirectX::XMMATRIX&&>(matWorld));
	}
	inline void SetViewMatrix(DirectX::XMMATRIX  &matView) {    //!< �r���[�s���ݒ肷��
		SetViewMatrix(static_cast<DirectX::XMMATRIX&&>(matView));
	}
	inline void SetProjectionMatrix(DirectX::XMMATRIX &matProjection) { //!< �v���W�F�N�V�����s���ݒ肷��
		SetProjectionMatrix(static_cast<DirectX::XMMATRIX&&>(matProjection));
	}
	inline void SetLightDirection(DirectX::XMFLOAT4 &vecLightDirection) {
		SetLightDirection(static_cast<DirectX::XMFLOAT4&&>(vecLightDirection));
	}
	inline void SetAmbientColor(DirectX::XMFLOAT4 &vecAmbientLight) {
		SetAmbientColor(static_cast<DirectX::XMFLOAT4&&>(vecAmbientLight));
	}
	virtual void SetWorldMatrix(DirectX::XMMATRIX &&matWorld);   //!< ���[���h�s���ݒ肷��
	virtual void SetViewMatrix(DirectX::XMMATRIX  &&matView);    //!< �r���[�s���ݒ肷��
	virtual void SetProjectionMatrix(DirectX::XMMATRIX &&matProjection); //!< �v���W�F�N�V�����s���ݒ肷��
	virtual void SetLightDirection(DirectX::XMFLOAT4 &&vecLightDirection);  //  �L�������̕�����ݒ肷��
	virtual void SetAmbientColor(DirectX::XMFLOAT4 &&vecAmbient);
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext);
	virtual HRESULT ReleaseDeviceObjects();

	void Render(ID3D11DeviceContext *pContext, ID3D11ShaderResourceView *pSRV, DirectX::XMMATRIX &matCombined);
protected:
	IPrimitive3DShadow *m_pPrimitive3D;
	ID3D11ShaderResourceView    *m_pTexture2DSRV;
};