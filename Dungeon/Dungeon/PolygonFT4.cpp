/**
*@file   PolygonFT4.cpp
*@brief  ポリゴンを描画するオブジェクトの実装
*   使いかた
*       このオブジェクトを生成する
*           IPolygonFT4 *pPolygon = IPolygonFT4::Create();
*       ビデオカードへ転送
*           pPolygon->RestoreDeviceObjects();
*       描画実行
*           pPolygon->Render(
*               pContext,   //< デバイスコンテキスト
*               vertices,   //< 頂点配列（4要素・・4つの頂点に対応)
*               texCoords,  //< テクスチャ座標(4要素)
*               color,      //< 面カラー(1要素)
*               pTexSRV     //< テクスチャ番号(テクスチャシェーダリソースビュー)
*           );
*/
#include "stdafx.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include "PolygonFT4.h"
#include <vector>
#include "D3DCompiler.h"
#include "TextureLoader.h"

///@ macros
#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if (o){ (o)->Release(); o = NULL; }
#define SAFE_DELETE(o)  if (o){ delete (o); o = NULL; }
#define SAFE_DELETE_ARRAY(o) if (o){ delete [] (o); o = NULL; }

///@destructor
///@brief  pure virtual destructor
IPolygonFT4::~IPolygonFT4() {
}
#if 0
//  structure to keep a texture.
struct PolygonFT4Texture {
	TCHAR               *pFilename;
	ID3D11Texture2D     *pTexture;
	ID3D11ShaderResourceView    *pTextureShaderResourceView;
	DWORD               dwSrcWidth;
	DWORD               dwSrcHeight;

	PolygonFT4Texture() {
		pFilename = NULL;
		pTexture = NULL;
		pTextureShaderResourceView = NULL;
	}
	~PolygonFT4Texture() {
		SAFE_DELETE(pFilename);
		SAFE_RELEASE(pTexture);
		SAFE_RELEASE(pTextureShaderResourceView);
	}
};
#endif

//  Vertex Structure
typedef __declspec(align(16)) struct {
	DirectX::XMFLOAT3 position;   //  position
	DirectX::XMFLOAT2 texCoord;   //  texCoord
}   PolygonFT4Vertex;

//  Constant buffer structure
typedef __declspec(align(16)) struct {
	DirectX::XMMATRIX  matWorld;     ///@ world matrix
	DirectX::XMMATRIX  matView;      ///@ view matrix
	DirectX::XMMATRIX  matProj;      ///@ projection matrix
	DirectX::XMFLOAT4  faceNormal;   ///@ face normal
	DirectX::XMFLOAT4  faceColor;    ///@ face color

	DirectX::XMFLOAT4  spotLitPos;   //!< スポットライトの位置(World座標系)
	DirectX::XMFLOAT4  spotLitDir;   //!< 有向光源の向き(World座標系)
	DirectX::XMFLOAT4  spotLitColor; //!< 有向光源の輝度(World座標系)
	FLOAT   spotLitAngle;            //!< スポットライトのコーンの頂角
	FLOAT   spotLitEdgeFalloff;      //!< スポットライトのコーンの頂角をはずてからの減衰量
	FLOAT   spotLitDistanceSustain;  //!< スポットライトの到達距離（ここまで減衰しない）
	FLOAT   spotLitDistanceFalloff;  //!< スポットライトが到達距離より離れたときの減衰量
	DirectX::XMFLOAT4  fogColor;     //!< フォグカラー
	FLOAT   fogStart;                //!< Zフォグスタート
	FLOAT   fogFalloff;              //!< Zフォグ減衰量
	FLOAT   AlphaThreshold;


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

}   PolygonFT4ConstantBuffer;

//  Instance class
#define kNumVertex  4

__declspec(align(16)) class CPolygonFT4 : public IPolygonFT4
{
public:
	CPolygonFT4(void);
	virtual ~CPolygonFT4(void);

	virtual void Render(ID3D11DeviceContext *pContext,
		const DirectX::XMFLOAT3(&positions)[4],
		const DirectX::XMFLOAT2(&texCoords)[4],
		const DirectX::XMFLOAT4 &color,
		ID3D11ShaderResourceView *pTex) override;

	virtual void SetSpotLight(DirectX::XMFLOAT4 &&pos, FLOAT angle, FLOAT angle2, DirectX::XMFLOAT3 &&normal, DirectX::XMFLOAT4 &&color, FLOAT sustain, FLOAT limit) override;
	virtual void SetLinearFog(FLOAT start, FLOAT end, DirectX::XMFLOAT4 &&color) override;


	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext) override;
	virtual HRESULT ReleaseDeviceObjects() override;

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
	static void CompileShaderCodes();   //  
	static void RemoveShaderCodes();    //  

protected:
	virtual HRESULT RestoreClassObjects();
	virtual HRESULT ReleaseClassObjects();
	virtual HRESULT RestoreInstanceObjects();
	virtual HRESULT ReleaseInstanceObjects();

	//PolygonFT4DataContext   *m_pDataContext;
	PolygonFT4Vertex m_pVertices[kNumVertex];

	ID3D11DeviceContext *m_pDeviceContext;
	ID3D11Device        *m_pDevice;

	ID3D11SamplerState  *m_pTextureSamplerState;
	ID3D11BlendState    *m_pBlendState;
	ID3D11BlendState    *m_pBlendStateAdd;
	ID3D11Buffer        *m_pVertexBuffer;

	INT                 m_iActiveTextureNo;

	static ID3D11Buffer         *m_pConstantBuffer;
	static ID3D11VertexShader   *m_pVertexShader;
	static ID3D11PixelShader    *m_pPixelShader;
	static ID3D11PixelShader    *m_pNoTexPixelShader;
	static ID3D11InputLayout    *m_pInputLayout;

	PolygonFT4ConstantBuffer    m_vsConstantBuffer;

	static BYTE  *m_pVertexShaderCode;
	static DWORD m_dwVertexShaderCodeSize;
	static BYTE  *m_pPixelShaderCode;
	static DWORD m_dwPixelShaderCodeSize;
	static BYTE  *m_pNoTexPixelShaderCode;
	static DWORD m_dwNoTexPixelShaderCodeSize;
	static INT   m_iShaderReferenceCount;
};

/// static fields
ID3D11Buffer        *CPolygonFT4::m_pConstantBuffer = NULL;
ID3D11VertexShader  *CPolygonFT4::m_pVertexShader = NULL;
ID3D11PixelShader   *CPolygonFT4::m_pPixelShader = NULL;
ID3D11PixelShader   *CPolygonFT4::m_pNoTexPixelShader = NULL;
ID3D11InputLayout   *CPolygonFT4::m_pInputLayout = NULL;

BYTE  *CPolygonFT4::m_pVertexShaderCode = NULL;
DWORD CPolygonFT4::m_dwVertexShaderCodeSize = 0;
BYTE  *CPolygonFT4::m_pPixelShaderCode = NULL;
DWORD CPolygonFT4::m_dwPixelShaderCodeSize = 0;
BYTE  *CPolygonFT4::m_pNoTexPixelShaderCode = NULL;
DWORD CPolygonFT4::m_dwNoTexPixelShaderCodeSize = 0;
INT  CPolygonFT4::m_iShaderReferenceCount = 0;

///@brief   ファクトリーメソッド
IPolygonFT4 *IPolygonFT4::Create() {
	CPolygonFT4 *pRetvalue = new CPolygonFT4();
	return pRetvalue;
}


///@class D3D11PolygonFT4ShaderBuilder
///@brief 
//    明示的呼び出し無しで、シェーダをコンパイルするトリック
class D3D11PolygonFT4ShaderBuilder {
public:
	D3D11PolygonFT4ShaderBuilder() {
		CPolygonFT4::CompileShaderCodes();
	}
	~D3D11PolygonFT4ShaderBuilder() {
		CPolygonFT4::RemoveShaderCodes();
	}
}   d3d11PolygonFT4Initializer;

// ctor
CPolygonFT4::CPolygonFT4() {

	m_pDeviceContext = NULL;
	m_pDevice = NULL;

	m_pTextureSamplerState = NULL;
	m_pBlendState = NULL;
	m_pBlendStateAdd = NULL;
	m_pVertexBuffer = NULL;
	m_iActiveTextureNo = -1;

	DirectX::XMMATRIX matView, matProj;
	{
		DirectX::XMVECTOR camPos = DirectX::XMVectorSet(0.0f, 9.0f, -18.00f, 0.0f);
		DirectX::XMVECTOR camTarget = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR camUpward = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		matView = DirectX::XMMatrixLookAtLH(camPos, camTarget, camUpward);
	}
	matProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2*0.5f, 16.0f / 9.0f, 0.1f, 1000.0f);

	m_vsConstantBuffer.spotLitDistanceSustain = 5000.0f;
	m_vsConstantBuffer.spotLitColor = DirectX::XMFLOAT4(0, 0, 0, 0);
	m_vsConstantBuffer.spotLitDistanceFalloff = 0.0f;

	m_vsConstantBuffer.matProj = matProj;
	m_vsConstantBuffer.matView = matView;
	m_vsConstantBuffer.matWorld = DirectX::XMMatrixIdentity();
	m_vsConstantBuffer.faceNormal = DirectX::XMFLOAT4(1, 1, 1, 0);

	m_vsConstantBuffer.faceColor = DirectX::XMFLOAT4(1, 1, 1, 1);
	m_vsConstantBuffer.fogStart = 5000.0f;
	m_vsConstantBuffer.fogFalloff = 0.0f;
	m_vsConstantBuffer.fogColor = DirectX::XMFLOAT4(0, 0, 0, 0);


	m_iShaderReferenceCount++;
}


//  destructor
CPolygonFT4::~CPolygonFT4() {
	--m_iShaderReferenceCount;
	if (m_iShaderReferenceCount <= 0) {
		m_iShaderReferenceCount = 0;
		ReleaseClassObjects();
	}
	ReleaseInstanceObjects();
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pDeviceContext);

}


///@method Render
///@brief  描画実行
///@param[in]   pContext  : Device context
///@param[in]   positions : 頂点座標 ( 4成分必ず )
///@param[in]   texCoords : テクスチャ座標 ( 4成分必ず )
///@param[in]   pcolor    : 面の色（1成分)
void CPolygonFT4::Render(ID3D11DeviceContext *pContext,
	const DirectX::XMFLOAT3(&positions)[4], const DirectX::XMFLOAT2(&texCoords)[4],
	const DirectX::XMFLOAT4 &color, ID3D11ShaderResourceView *pTex)
{
	//ID3D11Texture2D           *pTexture = NULL;
	ID3D11ShaderResourceView *pTextureShaderResourceView = NULL;

	//pTexture = pSpriteTexture->pTexture;
	pTextureShaderResourceView = pTex;
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
	}

	pContext->UpdateSubresource(m_pConstantBuffer, 0, NULL, &m_vsConstantBuffer, 0, 0);

	for (int i = 0; i < kNumVertex; ++i) {
		PolygonFT4Vertex *pVertex = &m_pVertices[i];
		pVertex->position = positions[i];
		pVertex->texCoord = texCoords[i];
	}

	//  頂点バッファの設定
	{
		pContext->UpdateSubresource(m_pVertexBuffer, 0, NULL, m_pVertices, 0, 0);
	}

	// install the vertex buffer to input assembler
	UINT stride = sizeof(PolygonFT4Vertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// setup the input layout.
	pContext->IASetInputLayout(m_pInputLayout);

	// setup the primitive type
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3D11ShaderResourceView* ppShaderResourceViews[] = { pTextureShaderResourceView, 0 };
	ID3D11SamplerState  *ppSamplerStates[] = { m_pTextureSamplerState, 0 };
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
	pContext->PSSetSamplers(0, 1, ppSamplerStates);

	//　install the shaders
	pContext->VSSetShader(m_pVertexShader, NULL, 0);
	pContext->GSSetShader(NULL, NULL, 0);
	if (pTextureShaderResourceView == NULL) {
		pContext->PSSetShader(m_pNoTexPixelShader, NULL, 0);    //  without texture
	}
	else {
		pContext->PSSetShader(m_pPixelShader, NULL, 0);     //  with texture
	}
	// 頂点シェーダに定数バッファを設定.
	pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	//pContext->GSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	pContext->PSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pContext->OMSetBlendState(m_pBlendStateAdd, blendFactor, 0xffffffff);

	pContext->Draw(kNumVertex, 0);

	ppSamplerStates[0] = NULL;
	ppShaderResourceViews[0] = NULL;
	pContext->PSSetSamplers(0, 1, ppSamplerStates);
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);

	//  recover the blend state.
	pContext->OMSetBlendState(NULL, blendFactor, 0xffffffff);

}


/**
*  @spotlight を設定する
*  @param [in] pos   ライトの位置(WORLD座標)
*  @param [in] angle コーン部分の中央から明るいエッジまでの角度（内側）
*  @param [in] angle コーン部分の中央からフォールオフ終了までの角度（外側）
*  @param [in] normal ライトを向ける方向
*  @param [in] sustain
*/
void CPolygonFT4::SetSpotLight(DirectX::XMFLOAT4 &&pos, FLOAT angle, FLOAT angle2, DirectX::XMFLOAT3 &&normal, DirectX::XMFLOAT4 &&color, FLOAT sustain, FLOAT limit) {
	m_vsConstantBuffer.spotLitPos = pos;
	m_vsConstantBuffer.spotLitDir = DirectX::XMFLOAT4(normal.x, normal.y, normal.z, 0.0f);
	m_vsConstantBuffer.spotLitAngle = cosf(angle);
	m_vsConstantBuffer.spotLitEdgeFalloff = 1.0f / (cosf(angle) - cosf(angle2));
	m_vsConstantBuffer.spotLitDistanceSustain = sustain;
	m_vsConstantBuffer.spotLitDistanceFalloff = 1.0f / (limit - sustain);
	m_vsConstantBuffer.spotLitColor = color;
}



/**
*  @brief リニアフォグを設定する
*/	
void    CPolygonFT4::SetLinearFog(FLOAT fogStart, FLOAT fogEnd, DirectX::XMFLOAT4 &&color) {
	if (fogEnd  > fogStart) {
		m_vsConstantBuffer.fogStart = fogStart;
		m_vsConstantBuffer.fogFalloff = 1.0f / (fogEnd - fogStart);
		m_vsConstantBuffer.fogColor = color;
	}
}



HRESULT CPolygonFT4::RestoreClassObjects() {
	HRESULT hr = S_OK;
	if (m_pVertexShader != NULL /*&& m_pGeometryShader != NULL*/ && m_pPixelShader != NULL && m_pNoTexPixelShader != NULL && m_pInputLayout != NULL && m_pConstantBuffer != NULL) {
		return S_OK;
	}
	SAFE_RELEASE(this->m_pVertexShader);
	//SAFE_RELEASE(this->m_pGeometryShader);
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
			"TEXCOORD",
			0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0
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

	//  create the pixel shader
	hr = m_pDevice->CreatePixelShader(m_pPixelShaderCode, m_dwPixelShaderCodeSize, NULL, &m_pPixelShader);
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
		bd.ByteWidth = sizeof(PolygonFT4ConstantBuffer);
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

HRESULT CPolygonFT4::RestoreInstanceObjects() {
	HRESULT hr = S_OK;

	//  setup the texture sampler
	{
		D3D11_SAMPLER_DESC samplerDesc;
		FLOAT zero4[] = { 0,0,0,0 };
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

	}
	// setup the vertex buffer
	{
		// definition of the vertex buffer

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(this->m_pVertices);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		// setup the sub resource
		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
		initData.pSysMem = this->m_pVertices;

		// do create the vertex buffer..
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
HRESULT CPolygonFT4::ReleaseInstanceObjects() {
	HRESULT hr = S_OK;
	SAFE_RELEASE(m_pTextureSamplerState);
	SAFE_RELEASE(m_pBlendState);
	SAFE_RELEASE(m_pBlendStateAdd);
	SAFE_RELEASE(m_pVertexBuffer);

	return hr;
}
HRESULT CPolygonFT4::ReleaseClassObjects() {
	SAFE_RELEASE(this->m_pVertexShader);
	//SAFE_RELEASE(this->m_pGeometryShader);
	SAFE_RELEASE(this->m_pPixelShader);
	SAFE_RELEASE(this->m_pNoTexPixelShader);
	SAFE_RELEASE(this->m_pInputLayout);
	SAFE_RELEASE(this->m_pConstantBuffer);

	return S_OK;
}


//
//  Restore device dependent objects.
//
HRESULT CPolygonFT4::RestoreDeviceObjects(ID3D11DeviceContext *pContext) {
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

//
//  Release device dependent objects.
//
HRESULT CPolygonFT4::ReleaseDeviceObjects() {
	ReleaseInstanceObjects();
	ReleaseClassObjects();
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	return S_OK;
}

void    CPolygonFT4::SetWorldMatrix(DirectX::XMMATRIX &matWorld) {
	m_vsConstantBuffer.matWorld = matWorld;
}

void    CPolygonFT4::SetViewMatrix(DirectX::XMMATRIX  &matView) {
	m_vsConstantBuffer.matView = matView;
}

void    CPolygonFT4::SetProjectionMatrix(DirectX::XMMATRIX &matProjection) {
	m_vsConstantBuffer.matProj = matProjection;
}

#define FILENAME "res\\PolygonFT4.fx"
void CPolygonFT4::CompileShaderCodes() {
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

void CPolygonFT4::RemoveShaderCodes() {
	SAFE_DELETE_ARRAY(m_pVertexShaderCode);
	//SAFE_DELETE_ARRAY(m_pGeometryShaderCode);
	SAFE_DELETE_ARRAY(m_pPixelShaderCode);
	SAFE_DELETE_ARRAY(m_pNoTexPixelShaderCode);
}