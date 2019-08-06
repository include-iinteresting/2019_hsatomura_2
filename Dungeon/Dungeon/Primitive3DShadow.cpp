#include "stdafx.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include "D3DCompiler.h"
#include "Primitive3DShadow.h"



///@ macros
#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if (o){ (o)->Release(); o = NULL; }
#define SAFE_DELETE(o)  if (o){ delete (o); o = NULL; }
#define SAFE_DELETE_ARRAY(o) if (o){ delete [] (o); o = NULL; }


#define FILENAME "res\\Primitive3DShadow.hlsl"

/**
* @brief	destructor
*/
IPrimitive3DShadow::~IPrimitive3DShadow()
{
}


//  Vertex Structure
__declspec(align(16)) typedef struct {
	DirectX::XMFLOAT3 position;   //  position
	DirectX::XMFLOAT3 normal;   //  per vertex normal
	DirectX::XMFLOAT2 texCoord1;   //  texCoord
	DirectX::XMFLOAT2 texCoord2;   //  texCoord
								   //  keep this 16-byte aligned
	void *operator new(size_t size) {
		return _mm_malloc(size, 16);
	}
	void *operator new[](size_t size) {
		return _mm_malloc(size, 16);
	}
		void operator delete(void *p) {
		return _mm_free(p);
	}
	void operator delete[](void *p) {
		return _mm_free(p);
	}
}   Primitive3D2TexVertex;

//  Constant buffer structure
__declspec(align(16)) typedef struct {
	DirectX::XMMATRIX   matWorld;    ///@ world matrix
	DirectX::XMMATRIX   matView;     ///@ view matrix
	DirectX::XMMATRIX   matProj;     ///@ projection matrix
	DirectX::XMMATRIX	matShadow;	 ///  shadow texture matrix
	DirectX::XMFLOAT4   faceNormal;  ///@ face normal
	DirectX::XMFLOAT4   faceColor;   ///@ face color
	DirectX::XMFLOAT4   vecLightDirection;  //!< ライト方向（Directional Light)
	DirectX::XMFLOAT4   vecLightColor;      //!< 拡散ライトカラー
	DirectX::XMFLOAT4   vecAmbientColor;    //!< 環境ライトカラー
	DirectX::XMFLOAT4   vecSpecularColor;   //!< 鏡面反射光カラー
	float               litPower;           //!< 鏡面反射光パワー

	FLOAT               AlphaThreshold;

	//  keep this 16-byte aligned
	void *operator new(size_t size) {
		return _mm_malloc(size, 16);
	}
	void *operator new[](size_t size) {
		return _mm_malloc(size, 16);
	}
		void operator delete(void *p) {
		return _mm_free(p);
	}

}   Primitive3DConstantBuffer;


/**
* @class	CPrimitive3DShadow
* @brief	ポリゴンのプリミティブ描画を担当するクラスの実装
*/
__declspec(align(16)) class CPrimitive3DShadow : public IPrimitive3DShadow 
{
public:
	CPrimitive3DShadow(DWORD numVertices);
	CPrimitive3DShadow();
	~CPrimitive3DShadow();

	virtual void Init(DWORD numVertices);

	virtual void Render(ID3D11DeviceContext *pContext,
		const DirectX::XMFLOAT3 *positions,
		const DirectX::XMFLOAT3 *normals,
		const DirectX::XMFLOAT2 *texCoords1,
		const DirectX::XMFLOAT2 *texCoords2,
		const DWORD         numVertices,
		const DirectX::XMFLOAT4 &color,
		ID3D11ShaderResourceView *pTex1,
		const DirectX::XMMATRIX &matrix,
		ID3D11ShaderResourceView *pTex2) override;

	virtual void RenderDepth(ID3D11DeviceContext *pContext,
		const DirectX::XMFLOAT3 *positions,
		const DWORD         numVertices) override;


	virtual	HRESULT	RestoreDeviceObjects(ID3D11DeviceContext *pContext)	override;
	virtual	HRESULT	ReleaseDeviceObjects()	override;

	virtual void    SetLightDirection(DirectX::XMFLOAT4 &vecDir) override;
	virtual void    SetLightColor(DirectX::XMFLOAT4 &vecColor) override;
	virtual void    SetAmbientColor(DirectX::XMFLOAT4 &vecAmbient) override;
	virtual void    SetSpecularColor(DirectX::XMFLOAT4 &vecSpecular) override;
	virtual void    SetSpecularPower(FLOAT power) override;


	virtual void    SetWorldMatrix(DirectX::XMMATRIX &matWorld) override;
	virtual void    SetViewMatrix(DirectX::XMMATRIX  &matView)  override;
	virtual void    SetProjectionMatrix(DirectX::XMMATRIX &matProjection) override;

	//  keep this 16-byte aligned
	void *operator new(size_t size) {
		return _mm_malloc(size, 16);
	}
	void *operator new[](size_t size) {
		return _mm_malloc(size, 16);
	}
		void operator delete(void *p) {
		return _mm_free(p);
	}

	static void CompileShaderCodes();  
	static void RemoveShaderCodes();
private:
	virtual HRESULT RestoreClassObjects();
	virtual HRESULT ReleaseClassObjects();
	virtual HRESULT RestoreInstanceObjects();
	virtual HRESULT ReleaseInstanceObjects();

	Primitive3D2TexVertex   *m_pVertices;
	DWORD               m_dwMaxNumVertices;

	ID3D11DeviceContext *m_pDeviceContext;
	ID3D11Device        *m_pDevice;

	ID3D11SamplerState  *m_pTextureSamplerState;
	ID3D11SamplerState  *m_pTextureSamplerState2;
	ID3D11BlendState    *m_pBlendState;
	ID3D11BlendState    *m_pBlendStateAdd;
	ID3D11Buffer        *m_pVertexBuffer;

	INT                 m_iActiveTextureNo;

	static ID3D11Buffer         *m_pConstantBuffer;
	static ID3D11VertexShader   *m_pVertexShader;
	static ID3D11GeometryShader *m_pGeometryShader;
	static ID3D11PixelShader    *m_pPixelShader;
	static ID3D11PixelShader	*m_pShadowPixelShader;
	static ID3D11PixelShader    *m_pNoTexPixelShader;
	static ID3D11InputLayout    *m_pInputLayout;

	Primitive3DConstantBuffer    m_vsConstantBuffer;

	static BYTE  *m_pVertexShaderCode;
	static DWORD m_dwVertexShaderCodeSize;

	static BYTE  *m_pGeometryShaderCode;
	static DWORD m_dwGeometryShaderCodeSize;

	static BYTE  *m_pPixelShaderCode;
	static DWORD m_dwPixelShaderCodeSize;
	static BYTE  *m_pShadowPixelShaderCode;
	static DWORD m_dwShadowPixelShaderCodeSize;
	static BYTE  *m_pNoTexPixelShaderCode;
	static DWORD m_dwNoTexPixelShaderCodeSize;
	static INT   m_iShaderReferenceCount;
};


/// static fields
ID3D11Buffer         *CPrimitive3DShadow::m_pConstantBuffer = NULL;
ID3D11VertexShader   *CPrimitive3DShadow::m_pVertexShader = NULL;
ID3D11GeometryShader *CPrimitive3DShadow::m_pGeometryShader = NULL;
ID3D11PixelShader    *CPrimitive3DShadow::m_pPixelShader = NULL;
ID3D11PixelShader    *CPrimitive3DShadow::m_pShadowPixelShader = NULL;
ID3D11PixelShader    *CPrimitive3DShadow::m_pNoTexPixelShader = NULL;
ID3D11InputLayout    *CPrimitive3DShadow::m_pInputLayout = NULL;

BYTE  *CPrimitive3DShadow::m_pVertexShaderCode = NULL;
DWORD CPrimitive3DShadow::m_dwVertexShaderCodeSize = 0;
BYTE  *CPrimitive3DShadow::m_pGeometryShaderCode = NULL;
DWORD CPrimitive3DShadow::m_dwGeometryShaderCodeSize = 0;
BYTE  *CPrimitive3DShadow::m_pPixelShaderCode = NULL;
DWORD CPrimitive3DShadow::m_dwPixelShaderCodeSize = 0;
BYTE  *CPrimitive3DShadow::m_pShadowPixelShaderCode = NULL;
DWORD CPrimitive3DShadow::m_dwShadowPixelShaderCodeSize = 0;
BYTE  *CPrimitive3DShadow::m_pNoTexPixelShaderCode = NULL;
DWORD CPrimitive3DShadow::m_dwNoTexPixelShaderCodeSize = 0;
INT  CPrimitive3DShadow::m_iShaderReferenceCount = 0;

/**
* @brief	インスタンスの生成
*/
IPrimitive3DShadow * IPrimitive3DShadow::Create(DWORD numVertices)
{
	CPrimitive3DShadow *pRetvalue = new CPrimitive3DShadow(numVertices);
	return pRetvalue;
}



///@class D3D11PolygonFT4ShaderBuilder
///@brief 
//    明示的呼び出し無しで、シェーダをコンパイルするトリック
class Primitive3DShaderBuilder {
public:
	Primitive3DShaderBuilder() {
		CPrimitive3DShadow::CompileShaderCodes();
	}
	~Primitive3DShaderBuilder() {
		CPrimitive3DShadow::RemoveShaderCodes();
	}
}   Primitive3DShaderInitializer;


/**
* @brief	constructor
* @param	[in]	numVertices		ポリゴンの頂点の数
*/
CPrimitive3DShadow::CPrimitive3DShadow(DWORD numVertices)
{
	this->Init(numVertices);
}


/**
* @brief	construct
*/
CPrimitive3DShadow::CPrimitive3DShadow()
{
	this->Init(4);
}


/**
* @brief	destructor
*/
CPrimitive3DShadow::~CPrimitive3DShadow()
{
	--m_iShaderReferenceCount;
	if (m_iShaderReferenceCount <= 0) {
		m_iShaderReferenceCount = 0;
		ReleaseClassObjects();
	}
	ReleaseInstanceObjects();
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_DELETE_ARRAY(m_pVertices);
}


/**
* @brief	Initialize
* @param	[in]	vertex num
*/
void CPrimitive3DShadow::Init(DWORD numVertices)
{
	m_dwMaxNumVertices = numVertices;

	m_pDeviceContext = NULL;
	m_pDevice = NULL;
	m_pTextureSamplerState = NULL;
	m_pTextureSamplerState2 = NULL;
	m_pBlendState = NULL;
	m_pBlendStateAdd = NULL;
	m_pVertexBuffer = NULL;
	m_pVertices = NULL;
	m_iActiveTextureNo = -1;

	DirectX::XMMATRIX matView, matProj;
	{
		DirectX::XMVECTOR camPos = DirectX::XMVectorSet(0.0f, 9.0f, -18.00f, 0.0f);
		DirectX::XMVECTOR camTarget = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR camUpward = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		matView = DirectX::XMMatrixLookAtLH(camPos, camTarget, camUpward);
	}
	matProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2*0.5f, 16.0f / 9.0f, 0.1f, 1000.0f);

	m_pVertices = new Primitive3D2TexVertex[m_dwMaxNumVertices];
	ZeroMemory((void*)m_pVertices, sizeof(Primitive3D2TexVertex)*m_dwMaxNumVertices);
	m_vsConstantBuffer.vecLightColor = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_vsConstantBuffer.vecLightDirection = DirectX::XMFLOAT4(0, 0.707f, -0.707f, 0);
	m_vsConstantBuffer.vecAmbientColor = DirectX::XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	m_vsConstantBuffer.vecSpecularColor = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 0);
	m_vsConstantBuffer.litPower = 32.0f;

	m_vsConstantBuffer.matProj = matProj;
	m_vsConstantBuffer.matView = matView;
	m_vsConstantBuffer.matWorld = DirectX::XMMatrixIdentity();
	m_vsConstantBuffer.faceNormal = DirectX::XMFLOAT4(1, 1, 1, 0);
	m_vsConstantBuffer.faceColor = DirectX::XMFLOAT4(1, 1, 1, 1);

	m_iShaderReferenceCount++;
}


/**
* @brief	描画
* @param	[in]	pContext	ID3D11DeviceContext
* @param	[in]	positions	頂点座標
* @param	[in]	normals		法線
* @param	[in]	texCoords1	テクスチャ座標1
* @param	[in]	texCoords2	テクスチャ座標2
* @param	[in]	numVertices	頂点の数
* @param	[in]	color		色
* @param	[in]	pTex1		オブジェクトのテクスチャののシェーダーリソースビュー
* @param	[in]	matrix		オブジェクトのワールド行列・ライトのビュー行列・ライトの射影行列を組み合わせた行列
* @param	[in]	pTex2		シャドウのテクスチャのシェーダーリソースビュー
*/
void CPrimitive3DShadow::Render(ID3D11DeviceContext * pContext, const DirectX::XMFLOAT3 * positions, const DirectX::XMFLOAT3 * normals, const DirectX::XMFLOAT2 * texCoords1, const DirectX::XMFLOAT2 * texCoords2, const DWORD numVertices, const DirectX::XMFLOAT4 & color, ID3D11ShaderResourceView * pTex1, const DirectX::XMMATRIX & matrix, ID3D11ShaderResourceView * pTex2)
{
	ID3D11ShaderResourceView *pTextureShaderResourceView = NULL;
	pTextureShaderResourceView = pTex1;

	// update the constant buffer.
	{
		using namespace DirectX;
		XMVECTOR v0 = XMLoadFloat3(&positions[0]);
		XMVECTOR v1 = XMLoadFloat3(&positions[1]);
		XMVECTOR v2 = XMLoadFloat3(&positions[2]);
		v1 -= v0;
		v2 -= v0;
		XMVECTOR normal = XMVector3Cross(v1, v2);
		normal = XMVector3Normalize(normal);
		XMStoreFloat4(&m_vsConstantBuffer.faceNormal, normal);
		m_vsConstantBuffer.faceNormal.w = 0.0f;
		m_vsConstantBuffer.faceColor = color;
		m_vsConstantBuffer.matShadow = matrix;
	}
	pContext->UpdateSubresource(m_pConstantBuffer, 0, NULL, &m_vsConstantBuffer, 0, 0);
	static const DirectX::XMFLOAT2 defaultTex[] = {
		DirectX::XMFLOAT2(0, 0),DirectX::XMFLOAT2(1, 0),DirectX::XMFLOAT2(0, 1),DirectX::XMFLOAT2(1, 1)
	};
	for (unsigned int i = 0; i < min(numVertices, m_dwMaxNumVertices); ++i) {
		m_pVertices[i].position = positions[i];
		m_pVertices[i].normal = normals[i];
		m_pVertices[i].texCoord1 = (texCoords1 != NULL) ? texCoords1[i] : DirectX::XMFLOAT2(0, 0);
		m_pVertices[i].texCoord2 = (texCoords2 != NULL) ? texCoords2[i] : defaultTex[i];
	}
	//  Update the vertex buffer
	{
		pContext->UpdateSubresource(m_pVertexBuffer, 0, NULL, m_pVertices, 0, 0);
	}

	// install the vertex buffer to input assembler
	UINT stride = sizeof(Primitive3D2TexVertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	// setup the input layout.
	pContext->IASetInputLayout(m_pInputLayout);

	// setup the primitive type
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3D11ShaderResourceView* ppShaderResourceViews[] = { pTextureShaderResourceView, pTex2,0 };
	ID3D11SamplerState  *ppSamplerStates[] = { m_pTextureSamplerState, m_pTextureSamplerState2, 0 };

	pContext->PSSetShaderResources(0, 2, ppShaderResourceViews);
	pContext->PSSetSamplers(0, 2, ppSamplerStates);

	//　install the shaders
	pContext->VSSetShader(m_pVertexShader, NULL, 0);
	pContext->GSSetShader(m_pGeometryShader, NULL, 0);
	if (pTextureShaderResourceView == NULL) {
		pContext->PSSetShader(m_pNoTexPixelShader, NULL, 0);    //  without texture
	}
	else {
		pContext->PSSetShader(m_pPixelShader, NULL, 0);     //  with texture
	}
	// 頂点シェーダに定数バッファを設定.
	pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	pContext->GSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	pContext->PSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pContext->OMSetBlendState(m_pBlendStateAdd, blendFactor, 0xffffffff);

	pContext->Draw(numVertices, 0);

	ppSamplerStates[0] = NULL;
	ppShaderResourceViews[0] = NULL;
	pContext->PSSetSamplers(0, 1, ppSamplerStates);
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);

	//  recover the blend state.
	pContext->OMSetBlendState(NULL, blendFactor, 0xffffffff);

}


/**
* @brief	深度を描画する
* @param	[in]	pContext	ID3D11DeviceContext
* @param	[in]	positions	頂点座標
* @param	[in]	numVertices	頂点の数
*/
void CPrimitive3DShadow::RenderDepth(ID3D11DeviceContext * pContext, const DirectX::XMFLOAT3 * positions, const DWORD numVertices)
{
	// update the constant buffer.
	{
		using namespace DirectX;
		XMVECTOR v0 = XMLoadFloat3(&positions[0]);
		XMVECTOR v1 = XMLoadFloat3(&positions[1]);
		XMVECTOR v2 = XMLoadFloat3(&positions[2]);
		v1 -= v0;
		v2 -= v0;
		XMVECTOR normal = XMVector3Cross(v1, v2);
		normal = XMVector3Normalize(normal);
		XMStoreFloat4(&m_vsConstantBuffer.faceNormal, normal);
		m_vsConstantBuffer.faceNormal.w = 0.0f;
		m_vsConstantBuffer.faceColor = DirectX::XMFLOAT4(0, 0, 0, 0);
		pContext->UpdateSubresource(m_pConstantBuffer, 0, NULL, &m_vsConstantBuffer, 0, 0);
	}

	for (unsigned int i = 0; i < min(numVertices, m_dwMaxNumVertices); ++i) {
		m_pVertices[i].position = positions[i];
		m_pVertices[i].normal = DirectX::XMFLOAT3(0, 0, 1.0f);
		m_pVertices[i].texCoord1 = DirectX::XMFLOAT2(0, 0);
		m_pVertices[i].texCoord2 = DirectX::XMFLOAT2(0, 0);
	}
	//  Update the vertex buffer
	{
		pContext->UpdateSubresource(m_pVertexBuffer, 0, NULL, m_pVertices, 0, 0);
	}

	// install the vertex buffer to input assembler
	UINT stride = sizeof(Primitive3D2TexVertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	// setup the input layout.
	pContext->IASetInputLayout(m_pInputLayout);

	// setup the primitive type
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//　install the shaders
	pContext->VSSetShader(m_pVertexShader, NULL, 0);
	pContext->GSSetShader(m_pGeometryShader, NULL, 0);
	pContext->PSSetShader(m_pShadowPixelShader, NULL, 0);    //  shadow
															 // 頂点シェーダに定数バッファを設定.
	pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	pContext->GSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	pContext->PSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pContext->OMSetBlendState(m_pBlendStateAdd, blendFactor, 0xffffffff);

	pContext->Draw(numVertices, 0);

	//  recover the blend state.
	pContext->OMSetBlendState(NULL, blendFactor, 0xffffffff);
}


/**
* @brief	デバイス依存オブジェクトの再構築
* @param	[in]	pContext	ID3D11DeviceContext
*/
HRESULT CPrimitive3DShadow::RestoreDeviceObjects(ID3D11DeviceContext * pContext)
{
	ID3DBlob    *pShader = NULL;
	HRESULT hr = E_FAIL;

	pContext->AddRef();

	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	m_pDeviceContext = pContext;
	m_pDeviceContext->GetDevice(&m_pDevice);

	RestoreClassObjects();
	hr = RestoreInstanceObjects();
	return hr;
}


/**
* @brief	デバイス依存オブジェクトの解放
*/
HRESULT CPrimitive3DShadow::ReleaseDeviceObjects()
{
	ReleaseInstanceObjects();
	ReleaseClassObjects();
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	return S_OK;
}


/**
* @brief	ライトの向きをセットする
* @param	[in]	vecDir	セットするライトの向き
*/
void CPrimitive3DShadow::SetLightDirection(DirectX::XMFLOAT4 & vecDir)
{
	m_vsConstantBuffer.vecLightDirection = vecDir;
}


/**
* @brief	ライトの色をセットする
* @param	[in]	vecColor	ライトの色
*/
void CPrimitive3DShadow::SetLightColor(DirectX::XMFLOAT4 & vecColor)
{
	m_vsConstantBuffer.vecLightColor = vecColor;
}


/**
* @brief	環境光の色をセットする
* @param	[in]	vecAmbient	セットする環境光の色
*/
void CPrimitive3DShadow::SetAmbientColor(DirectX::XMFLOAT4 & vecAmbient)
{
	m_vsConstantBuffer.vecAmbientColor = vecAmbient;
}


/**
* @brief	スペキュラーの色をセットする
* @param	[in]	vecSpecular		セットするスペキュラーの色
*/
void CPrimitive3DShadow::SetSpecularColor(DirectX::XMFLOAT4 & vecSpecular)
{
	m_vsConstantBuffer.vecSpecularColor = vecSpecular;
}


/**
* @brief	スペキュラーのパワーをセットする
* @param	[in]	power	セットするスペキュラーのパワー
*/
void CPrimitive3DShadow::SetSpecularPower(FLOAT power)
{
	m_vsConstantBuffer.litPower = power;
}




/**
* @brief	ワールド行列をセットする
* @param	[in]	matWorld	ワールド行列
*/
void CPrimitive3DShadow::SetWorldMatrix(DirectX::XMMATRIX & matWorld)
{
	m_vsConstantBuffer.matWorld = matWorld;
}


/**
* @brief	ビュー行列をセットする
* @param	[in]	matView		ビュー行列
*/
void CPrimitive3DShadow::SetViewMatrix(DirectX::XMMATRIX & matView)
{
	m_vsConstantBuffer.matView = matView;
}


/**
* @brief	射影行列をセットする
* @param	[in]	matProjection	セットする射影行列
*/
void CPrimitive3DShadow::SetProjectionMatrix(DirectX::XMMATRIX & matProjection)
{
	m_vsConstantBuffer.matProj = matProjection;
}


/**
* @brief	クラス依存オブジェクトの再構築
*/
HRESULT CPrimitive3DShadow::RestoreClassObjects()
{
	HRESULT hr = S_OK;
	if (m_pVertexShader != NULL && m_pGeometryShader != NULL && m_pPixelShader != NULL && m_pNoTexPixelShader != NULL && m_pInputLayout != NULL && m_pConstantBuffer != NULL) {
		return S_OK;
	}
	SAFE_RELEASE(this->m_pVertexShader);
	SAFE_RELEASE(this->m_pGeometryShader);
	SAFE_RELEASE(this->m_pPixelShader);
	SAFE_RELEASE(this->m_pNoTexPixelShader);
	SAFE_RELEASE(this->m_pInputLayout);
	SAFE_RELEASE(this->m_pConstantBuffer);

	//  Compile shaders
	hr = m_pDevice->CreateVertexShader(m_pVertexShaderCode, m_dwVertexShaderCodeSize, NULL, &m_pVertexShader);
	if (FAILED(hr))
		return hr;

	// definition of the input layout.
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{
			"POSITION",
			0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0
		},
		{
			"NORMAL",
			0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
		},
		{
			"TEXCOORD",
			0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
		},
		{
			"TEXCOORD",
			1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
		}
	};
	UINT numElements = ARRAYSIZE(layout);

	// create the input layout
	hr = m_pDevice->CreateInputLayout(
		layout,
		numElements,
		m_pVertexShaderCode,
		m_dwVertexShaderCodeSize,
		&m_pInputLayout
	);
	if (FAILED(hr))
	{
		return hr;
	}

	//  create the geometry shader
	hr = m_pDevice->CreateGeometryShader(m_pGeometryShaderCode, m_dwGeometryShaderCodeSize, NULL, &m_pGeometryShader);
	if (FAILED(hr))
		return hr;


	//  create the pixel shader
	hr = m_pDevice->CreatePixelShader(m_pPixelShaderCode, m_dwPixelShaderCodeSize, NULL, &m_pPixelShader);
	if (FAILED(hr))
		goto EXIT;

	//	create the shadow pixel shader
	hr = m_pDevice->CreatePixelShader(m_pShadowPixelShaderCode, m_dwShadowPixelShaderCodeSize, NULL, &m_pShadowPixelShader);
	if (FAILED(hr))
		goto EXIT;

	//  create the pixel shader without the texture.
	hr = m_pDevice->CreatePixelShader(m_pNoTexPixelShaderCode, m_dwNoTexPixelShaderCodeSize, NULL, &m_pNoTexPixelShader);
	if (FAILED(hr))
		goto EXIT;

	// create the constant buffer
	{
		// setup the constant buffer.
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		bd.ByteWidth = sizeof(Primitive3DConstantBuffer);
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;

		// do create the constant buffer
		hr = m_pDevice->CreateBuffer(&bd, NULL, &m_pConstantBuffer);
		if (FAILED(hr))
		{
			return  hr;
		}
	}
	hr = S_OK;
EXIT:
	return hr;
}


/**
* @brief	クラス依存オブジェクトの解放
*/
HRESULT CPrimitive3DShadow::ReleaseClassObjects()
{
	SAFE_RELEASE(this->m_pVertexShader);
	SAFE_RELEASE(this->m_pGeometryShader);
	SAFE_RELEASE(this->m_pPixelShader);
	SAFE_RELEASE(this->m_pShadowPixelShader);
	SAFE_RELEASE(this->m_pNoTexPixelShader);
	SAFE_RELEASE(this->m_pInputLayout);
	SAFE_RELEASE(this->m_pConstantBuffer);

	return S_OK;
}


/**
* @brief	インスタンス依存オブジェクトの再構築
*/
HRESULT CPrimitive3DShadow::RestoreInstanceObjects()
{
	HRESULT hr = S_OK;

	//  setup the texture sampler
	{
		D3D11_SAMPLER_DESC samplerDesc;
		FLOAT zero4[] = { 0,0,0,0 };
		FLOAT one4[] = { 1,1,1,1 };
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // select linear filter to sample the pixel.
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // u operation about outside of the texture area.
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // v operation about outside of the texture area.
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // w operation about outside of the texture area.
		samplerDesc.MipLODBias = 0;                            // bias from the calculated mipmap level.
		samplerDesc.MaxAnisotropy = 16;                        // the max value of the anisotoropy.
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;  // comparison option
		memcpy((void*)&samplerDesc.BorderColor, (void*)&zero4, 4 * sizeof(FLOAT));
		samplerDesc.MinLOD = 0;                                // lowest number of accessible mipmaps.
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;                // highest number of accessible mipmaps.

															   // ID3D11Device::CreateSamplerState
		hr = m_pDevice->CreateSamplerState(&samplerDesc, &m_pTextureSamplerState);
		if (FAILED(hr))
			return  hr;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;     // u operation about outside of the texture area.
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;     // v operation about outside of the texture area.
		memcpy((void*)&samplerDesc.BorderColor, (void*)&one4, 4 * sizeof(FLOAT));
		hr = m_pDevice->CreateSamplerState(&samplerDesc, &m_pTextureSamplerState2);
		if (FAILED(hr))
			return  hr;
	}
	// setup the vertex buffer
	{
		// definition of the vertex buffer

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(Primitive3D2TexVertex) * m_dwMaxNumVertices;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		// setup the sub resource
		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
		initData.pSysMem = this->m_pVertices;

		// do create the vertex buffer the number of vertices is specified in constructor
		hr = m_pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
		if (FAILED(hr))
		{
			return  hr;
		}
	}
	{
		//  setup the blend state
		//  semi transparent
		D3D11_BLEND_DESC BlendDesc;
		ZeroMemory(&BlendDesc, sizeof(BlendDesc));
		BlendDesc.AlphaToCoverageEnable = FALSE;
		BlendDesc.IndependentBlendEnable = FALSE;
		BlendDesc.RenderTarget[0].BlendEnable = TRUE;
		BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		for (int i = 1; i < 8; ++i) {
			BlendDesc.RenderTarget[i] = BlendDesc.RenderTarget[0];
		}
		hr = m_pDevice->CreateBlendState(&BlendDesc, &m_pBlendState);
		if (FAILED(hr))
		{
			return  hr;
		}
		//  overlapping
		BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		for (int i = 1; i < 8; ++i) {
			BlendDesc.RenderTarget[i] = BlendDesc.RenderTarget[0];
		}
		hr = m_pDevice->CreateBlendState(&BlendDesc, &m_pBlendStateAdd);
		if (FAILED(hr))
		{
			return  hr;
		}
	}
	return hr;
}


/**
* @brief	インスタンス依存オブジェクトの解放
*/
HRESULT CPrimitive3DShadow::ReleaseInstanceObjects()
{
	HRESULT hr = S_OK;
	SAFE_RELEASE(m_pTextureSamplerState2);
	SAFE_RELEASE(m_pTextureSamplerState);
	SAFE_RELEASE(m_pBlendState);
	SAFE_RELEASE(m_pBlendStateAdd);
	SAFE_RELEASE(m_pVertexBuffer);

	return hr;
}


/**
* @brief	シェーダーのコンパイルをする
*/
void CPrimitive3DShadow::CompileShaderCodes()
{
	RemoveShaderCodes();

	HRESULT hr;

	// コンパイルフラグ.
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif
#if defined(NDEBUG) || defined(_NDEBUG)
	dwShaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
	//  Compile shaders
	ID3DBlob    *pCode = NULL, *pErrorMsgs = NULL;
	size_t len;
	hr = D3DCompileFromFile(_T(FILENAME), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSFunc", "vs_4_0", dwShaderFlags, 0, &pCode, &pErrorMsgs);

	if (FAILED(hr)) {
		// エラーメッセージを出力.
		if (pErrorMsgs != NULL) {
			OutputDebugStringA((char*)pErrorMsgs->GetBufferPointer());
		}
	}
	else {
		len = pCode->GetBufferSize();
		m_pVertexShaderCode = new BYTE[len];
		memcpy(m_pVertexShaderCode, pCode->GetBufferPointer(), len);
		m_dwVertexShaderCodeSize = (DWORD)len;
		SAFE_RELEASE(pCode);
	}
	SAFE_RELEASE(pErrorMsgs);


	if (SUCCEEDED(hr)) {
		//  ジオメトリシェーダの生成
		hr = D3DCompileFromFile(_T(FILENAME), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"GSFunc", "gs_4_0", dwShaderFlags, 0, &pCode, &pErrorMsgs);

		if (FAILED(hr)) {
			// エラーメッセージを出力.
			if (pErrorMsgs != NULL) {
				OutputDebugStringA((char*)pErrorMsgs->GetBufferPointer());
			}
		}
		else {
			len = pCode->GetBufferSize();
			m_pGeometryShaderCode = new BYTE[len];
			memcpy(m_pGeometryShaderCode, pCode->GetBufferPointer(), len);
			m_dwGeometryShaderCodeSize = (DWORD)len;
			SAFE_RELEASE(pCode);
		}
		SAFE_RELEASE(pErrorMsgs);
	}


	if (SUCCEEDED(hr)) {

		//  ピクセルシェーダの生成
		hr = D3DCompileFromFile(_T(FILENAME), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PSFunc", "ps_4_0", dwShaderFlags, 0, &pCode, &pErrorMsgs);

		if (FAILED(hr)) {
			// エラーメッセージを出力.
			if (pErrorMsgs != NULL) {
				OutputDebugStringA((char*)pErrorMsgs->GetBufferPointer());
			}
		}
		else {
			len = pCode->GetBufferSize();
			m_pPixelShaderCode = new BYTE[len];
			memcpy(m_pPixelShaderCode, pCode->GetBufferPointer(), len);
			m_dwPixelShaderCodeSize = (DWORD)len;
			SAFE_RELEASE(pCode);
		}
		SAFE_RELEASE(pErrorMsgs);
	}


	if (SUCCEEDED(hr)) {
		//  ピクセルシェーダの生成
		hr = D3DCompileFromFile(_T(FILENAME), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PSFuncDepthShadow", "ps_4_0", dwShaderFlags, 0, &pCode, &pErrorMsgs);

		if (FAILED(hr)) {
			// エラーメッセージを出力.
			if (pErrorMsgs != NULL) {
				OutputDebugStringA((char*)pErrorMsgs->GetBufferPointer());
			}
		}
		else {
			len = pCode->GetBufferSize();
			m_pShadowPixelShaderCode = new BYTE[len];
			memcpy(m_pShadowPixelShaderCode, pCode->GetBufferPointer(), len);
			m_dwShadowPixelShaderCodeSize = (DWORD)len;
			SAFE_RELEASE(pCode);
		}
		SAFE_RELEASE(pErrorMsgs);
	}

	if (SUCCEEDED(hr)) {
		//  ピクセルシェーダの生成
		hr = D3DCompileFromFile(_T(FILENAME), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PSFuncNoTex", "ps_4_0", dwShaderFlags, 0, &pCode, &pErrorMsgs);

		if (FAILED(hr)) {
			// エラーメッセージを出力.
			if (pErrorMsgs != NULL) {
				OutputDebugStringA((char*)pErrorMsgs->GetBufferPointer());
			}
		}
		else {
			len = pCode->GetBufferSize();
			m_pNoTexPixelShaderCode = new BYTE[len];
			memcpy(m_pNoTexPixelShaderCode, pCode->GetBufferPointer(), len);
			m_dwNoTexPixelShaderCodeSize = (DWORD)len;
			SAFE_RELEASE(pCode);
		}
		SAFE_RELEASE(pErrorMsgs);
	}
	if (FAILED(hr)) {
		RemoveShaderCodes();
	}
}


/**
*  @brief  remove compiled shader codes
*/
void CPrimitive3DShadow::RemoveShaderCodes() {
	SAFE_DELETE_ARRAY(m_pVertexShaderCode);
	SAFE_DELETE_ARRAY(m_pGeometryShaderCode);
	SAFE_DELETE_ARRAY(m_pPixelShaderCode);
	SAFE_DELETE_ARRAY(m_pShadowPixelShaderCode);
	SAFE_DELETE_ARRAY(m_pNoTexPixelShaderCode);
}
