#pragma once

class IPrimitive3DShadow;
class CFloor
{
public:
	CFloor();
	~CFloor();
	inline void SetWorldMatrix(DirectX::XMMATRIX &matWorld) {   //!< ワールド行列を設定する
		SetWorldMatrix(static_cast<DirectX::XMMATRIX&&>(matWorld));
	}
	inline void SetViewMatrix(DirectX::XMMATRIX  &matView) {    //!< ビュー行列を設定する
		SetViewMatrix(static_cast<DirectX::XMMATRIX&&>(matView));
	}
	inline void SetProjectionMatrix(DirectX::XMMATRIX &matProjection) { //!< プロジェクション行列を設定する
		SetProjectionMatrix(static_cast<DirectX::XMMATRIX&&>(matProjection));
	}
	inline void SetLightDirection(DirectX::XMFLOAT4 &vecLightDirection) {
		SetLightDirection(static_cast<DirectX::XMFLOAT4&&>(vecLightDirection));
	}
	inline void SetAmbientColor(DirectX::XMFLOAT4 &vecAmbientLight) {
		SetAmbientColor(static_cast<DirectX::XMFLOAT4&&>(vecAmbientLight));
	}
	virtual void SetWorldMatrix(DirectX::XMMATRIX &&matWorld);   //!< ワールド行列を設定する
	virtual void SetViewMatrix(DirectX::XMMATRIX  &&matView);    //!< ビュー行列を設定する
	virtual void SetProjectionMatrix(DirectX::XMMATRIX &&matProjection); //!< プロジェクション行列を設定する
	virtual void SetLightDirection(DirectX::XMFLOAT4 &&vecLightDirection);  //  有向光源の方向を設定する
	virtual void SetAmbientColor(DirectX::XMFLOAT4 &&vecAmbient);
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext);
	virtual HRESULT ReleaseDeviceObjects();

	void Render(ID3D11DeviceContext *pContext, ID3D11ShaderResourceView *pSRV, DirectX::XMMATRIX &matCombined);
protected:
	IPrimitive3DShadow *m_pPrimitive3D;
	ID3D11ShaderResourceView    *m_pTexture2DSRV;
};