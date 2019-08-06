#pragma once
#include "IDeviceDependentObject.h"


//前方宣言
struct ID3D11DeviceContext;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11SamplerState;
struct ID3D11InputLayout;
struct ID3D11Buffer;

//  Forward declaration
namespace DirectX {
	struct XMFLOAT2;
	struct XMFLOAT3;
	struct XMFLOAT4;
	struct XMMATRIX;
}
struct ID3D11ShaderResourceView;


/**
*@interface  IPrimitive3D2Shadow
*@brief      形状を３Ｄポリゴンで影付きで描画するクラスへのインタフェース
*/
class IPrimitive3DShadow :
	public IDeviceDependentObject
{
public:
	virtual ~IPrimitive3DShadow();

	virtual void Render(ID3D11DeviceContext *pContext,
		const DirectX::XMFLOAT3 *positions,
		const DirectX::XMFLOAT3 *normals,
		const DirectX::XMFLOAT2 *texCoords1,
		const DirectX::XMFLOAT2 *texCoords2,
		const DWORD         numVertices,
		const DirectX::XMFLOAT4 &color,
		ID3D11ShaderResourceView *pTex1,
		const DirectX::XMMATRIX &matrix,
		ID3D11ShaderResourceView *pTex2) = 0;

	virtual	void RenderDepth(ID3D11DeviceContext *pContext,
		const DirectX::XMFLOAT3 *positions,
		const DWORD numVertices) = 0;

	virtual void SetLightDirection(DirectX::XMFLOAT4 &vecDir) = 0;
	virtual void SetLightColor(DirectX::XMFLOAT4 &vecColor) = 0;
	virtual void	SetAmbientColor(DirectX::XMFLOAT4 &vecAmbient) = 0;
	virtual void	SetSpecularColor(DirectX::XMFLOAT4 &vecSpecular) = 0;
	virtual void	SetSpecularPower(FLOAT power) = 0;

	virtual void SetWorldMatrix(DirectX::XMMATRIX &matWorld) = 0;   //!< ワールド行列を設定する
	virtual void SetViewMatrix(DirectX::XMMATRIX  &matView) = 0;    //!< ビュー行列を設定する
	virtual void SetProjectionMatrix(DirectX::XMMATRIX &matProjection) = 0; //!< プロジェクション行列を設定する

	virtual	HRESULT	RestoreDeviceObjects(ID3D11DeviceContext *pContext) = 0;
	virtual	HRESULT	ReleaseDeviceObjects() = 0;
	static	IPrimitive3DShadow	*Create(DWORD numVertices);
};

