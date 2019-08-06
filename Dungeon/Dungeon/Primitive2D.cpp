/**
*  @file  Primitive2D.cpp
*  @brief 2Dプリミティブを描画するクラス CPrimitive2D の実装
*  @date 11.May.2019
*/
#include "stdafx.h"
#include <stdio.h>
#include <D3D11.h>
#include <directxmath.h>
#include <D3Dcompiler.h>
#include "Primitive2D.h"
#include "TextureLoader.h"

//  オブジェクト解放用マクロ
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#undef SAFE_RELEASE
#define SAFE_DELETE(o)       if (o){  delete (o);  o = NULL;    }
#define SAFE_DELETE_ARRAY(o) if (o){  delete [] (o);  o = NULL; }
#define SAFE_RELEASE(o)      if (o){  (o)->Release(); o = NULL; }

#define SHADER_FILENAME L"res\\Textured2d.fx"

//  読み込むテクスチャのファイル名（NUM_TEXTURESと数が一致しなければならない）
static const TCHAR  *pFilename[] = {
	_T("res\\bricks.png")
};

//  typedef
/**
*  @brief 頂点データ構造体
*/
typedef struct {
	DirectX::XMFLOAT3 position;     //  位置座標
	DirectX::XMFLOAT2 texture;      //  テクスチャ座標
}   Simple2DVertex;

/**
*  @brief 定数バッファ用構造体（頂点シェーダで使用）
*/
typedef __declspec(align(16)) struct {
	DirectX::XMMATRIX   matProj;
	DirectX::XMFLOAT4   color;
}   Simple2DConstantBuffer;

/**
*  @brief 定数バッファ用構造体（ピクセルシェーダで使用）
*/
typedef __declspec(align(16)) struct {
	DirectX::XMFLOAT4   PSColor;
}   Simple2DPixelShaderConstantBuffer;

//  プロトタイプ
static HRESULT GetBackbufferSize(ID3D11DeviceContext *pContext, DWORD &width, DWORD &height);

DirectX::XMMATRIX CPrimitive2D::m_matProjection;

CPrimitive2D::CPrimitive2D()
{
	m_matProjection = DirectX::XMMatrixIdentity();

	m_iPhase = NULL;
	m_fAlpha = NULL;
	m_fWait = NULL;

	ZeroMemory(&m_pTextures, sizeof(m_pTextures));
	m_pSamplerState = NULL;

	m_pVertexShader = NULL;
	m_pPixelShader = NULL;

	m_pInputLayout = NULL;
	m_pVertexBuffer = NULL;
	m_pConstantBuffer = NULL;
	m_pPixelShaderConstantBuffer = NULL;

	m_bPrepared = FALSE;

	m_dwWidth = 852;
	m_dwHeight = 480;
}


CPrimitive2D::~CPrimitive2D()
{
	ReleaseDeviceObjects();
}

/**
*  @brief デバイス依存オブジェクトの再読み込みを行う
*  @param [in] pContext 有効なデバイスコンテキスト
*/
HRESULT CPrimitive2D::RestoreDeviceObjects(ID3D11DeviceContext *pContext) {
	DWORD dwWidth, dwHeight;
	HRESULT result = E_FAIL;
	ID3D11Device *pDevice = NULL;

	ReleaseDeviceObjects();

	pContext->GetDevice(&pDevice);

	if (FAILED(GetBackbufferSize(pContext, dwWidth, dwHeight))) {
		dwWidth = 852;  dwHeight = 480; //  default size
	}
	m_matProjection = DirectX::XMMatrixOrthographicOffCenterLH(0.0f, (FLOAT)dwWidth, (FLOAT)dwHeight, 0.0f, 0.1f, 100.1f);
	m_dwWidth = dwWidth;
	m_dwHeight = dwHeight;

	if (FAILED(GetBackbufferSize(pContext, dwWidth, dwHeight))) {
		dwWidth = 852;  dwHeight = 480; //  default size
	}
	m_matProjection = DirectX::XMMatrixOrthographicOffCenterLH(0.0f, (FLOAT)dwWidth, (FLOAT)dwHeight, 0.0f, 0.1f, 100.1f);

	// コンパイルフラグ.
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif
#if defined(NDEBUG) || defined(_NDEBUG)
	dwShaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	ID3DBlob    *pCode = NULL, *pErrorMsgs = NULL;

	//  頂点シェーダの生成
	result = D3DCompileFromFile(SHADER_FILENAME, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSFunc", "vs_4_0", dwShaderFlags, 0, &pCode, &pErrorMsgs);
	// エラーチェック.
	if (FAILED(result)) {
		// エラーメッセージを出力.
		if (pErrorMsgs != NULL) {
			OutputDebugStringA((char*)pErrorMsgs->GetBufferPointer());
		}
	}
	else {
		result = pDevice->CreateVertexShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), NULL, &m_pVertexShader);

		//!  頂点入力レイアウトの生成（レンダリングパイプラインからの見る）
		if (SUCCEEDED(result)) {
			D3D11_INPUT_ELEMENT_DESC layout[] = {
				{
					"POSITION",
					0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0
				},
				{
					"TEXCOORD",
					0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0
				}
			};
			UINT numElements = ARRAYSIZE(layout);

			//  入力レイアウトを生成.
			//  内部でコンパイル済みシェーダコード内の
			//  頂点フォーマットと一致するか照合しているため、シェーダコードを渡す必要がある。
			//  コンパイル済みシェーダコードといってもネイティブバイナリではないので
			//  この照合はできれば省略したい処理
			result = pDevice->CreateInputLayout(
				layout,
				numElements,
				pCode->GetBufferPointer(),
				pCode->GetBufferSize(),
				&m_pInputLayout
			);
		}
	}

	SAFE_RELEASE(pCode);
	SAFE_RELEASE(pErrorMsgs);

	//  ピクセルシェーダの生成
	if (SUCCEEDED(result)) {
		result = D3DCompileFromFile(SHADER_FILENAME, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PSFunc", "ps_4_0", dwShaderFlags, 0, &pCode, &pErrorMsgs);
		if (FAILED(result)) {
			// エラーメッセージを出力.
			if (pErrorMsgs != NULL) {
				OutputDebugStringA((char*)pErrorMsgs->GetBufferPointer());
			}
		}
		else {
			result = pDevice->CreatePixelShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), NULL, &m_pPixelShader);
		}
	}
	SAFE_RELEASE(pCode);
	SAFE_RELEASE(pErrorMsgs);

	//  定数バッファの生成
	if (SUCCEEDED(result)) {
		D3D11_BUFFER_DESC bd = { 0 };
		bd.ByteWidth = sizeof(Simple2DConstantBuffer);
		bd.Usage = D3D11_USAGE_DEFAULT; //  CPUは書き込みのみ可
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		// 定数バッファを生成.
		result = pDevice->CreateBuffer(&bd, NULL, &m_pConstantBuffer);
	}

	//  定数バッファの生成
	if (SUCCEEDED(result)) {
		D3D11_BUFFER_DESC bd = { 0 };
		bd.ByteWidth = sizeof(Simple2DPixelShaderConstantBuffer);
		bd.Usage = D3D11_USAGE_DEFAULT; //  CPUは書き込みのみ可
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		// 定数バッファを生成.
		result = pDevice->CreateBuffer(&bd, NULL, &m_pPixelShaderConstantBuffer);
	}

	//  頂点バッファの生成
	if (SUCCEEDED(result)) {
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		bd.Usage = D3D11_USAGE_DEFAULT; //  CPUは書き込みのみ可
		bd.ByteWidth = sizeof(Simple2DVertex) * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		// 頂点バッファの生成.
		result = pDevice->CreateBuffer(&bd, NULL, &m_pVertexBuffer);
	}
	if (SUCCEEDED(result)) {
		for (int i = 0; i < min(_countof(pFilename), NUM_TEXTURES); ++i) {
			result = CTextureLoader::CreateTextureFromFile(pContext, pFilename[i], &m_pTextures[i].pTexture,
				&m_pTextures[i].dwSrcWidth, &m_pTextures[i].dwSrcHeight);
			// シェーダリソースビューを生成.
			if (SUCCEEDED(result)) {
				D3D11_SHADER_RESOURCE_VIEW_DESC srvd;    //!< 
				D3D11_TEXTURE2D_DESC            texDesc; //!< テクスチャデスクリプタ
				ZeroMemory(&srvd, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
				m_pTextures[i].pTexture->GetDesc(&texDesc);
				srvd.Format = texDesc.Format;
				srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvd.Texture2D.MipLevels = texDesc.MipLevels;
				srvd.Texture2D.MostDetailedMip = 0;
				result = pDevice->CreateShaderResourceView(m_pTextures[i].pTexture, &srvd, &m_pTextures[i].pTextureShaderResourceView);
			}
			else
				break;
		}
	}
	if (SUCCEEDED(result)) {

		D3D11_SAMPLER_DESC samplerDesc;
		static const DirectX::XMFLOAT4 zero4 = DirectX::XMFLOAT4(0, 0, 0, 0);
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;         // サンプリング時に使用するフィルタ。ここでは異方性フィルターを使用する。
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 ～ 1 の範囲外にある u テクスチャー座標の描画方法
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 ～ 1 の範囲外にある v テクスチャー座標
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 ～ 1 の範囲外にある w テクスチャー座標
		samplerDesc.MipLODBias = 0;                            // 計算されたミップマップ レベルからのバイアス
		samplerDesc.MaxAnisotropy = 16;                        // サンプリングに異方性補間を使用している場合の限界値。有効な値は 1 ～ 16 。
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;  // 比較オプション。
		memcpy((void*)&samplerDesc.BorderColor, (void*)&zero4, 4 * sizeof(FLOAT));
		samplerDesc.MinLOD = 0;                                // アクセス可能なミップマップの下限値
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;                // アクセス可能なミップマップの上限値
		result = pDevice->CreateSamplerState(&samplerDesc, &m_pSamplerState);
	}
	SAFE_RELEASE(pDevice);

	if (SUCCEEDED(result)) {
		m_bPrepared = TRUE;
	}
	return result;
}
/**
*  @brief デバイス依存オブジェクトの解放を行う
*/
HRESULT CPrimitive2D::ReleaseDeviceObjects() {
	HRESULT result = S_OK;
	for (int i = 0; i < NUM_TEXTURES; ++i) {
		SAFE_RELEASE(m_pTextures[i].pTexture);
		SAFE_RELEASE(m_pTextures[i].pTextureShaderResourceView);
	}
	SAFE_RELEASE(m_pPixelShaderConstantBuffer);
	SAFE_RELEASE(m_pSamplerState);
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pConstantBuffer);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	m_bPrepared = FALSE;

	return result;
}

/**
*  @brief プリミティブの描画
*  @param [in]  pContext  有効なRenderTarget を設定されたデバイスコンテキスト
*/
void CPrimitive2D::Render(ID3D11DeviceContext *pContext, float x, float y, float w, float h) {
	if (!m_bPrepared)
		return;

	//  定数バッファの更新（全書き換え）
	Simple2DConstantBuffer  cb;
	cb.matProj = m_matProjection;
	cb.color = DirectX::XMFLOAT4(1, 1, 1, 1);
	pContext->UpdateSubresource(m_pConstantBuffer, 0, NULL, &cb, 0, 0);

	Simple2DPixelShaderConstantBuffer  pcb;
	pcb.PSColor = DirectX::XMFLOAT4(1, 1, 1, 0);
	pContext->UpdateSubresource(m_pPixelShaderConstantBuffer, 0, NULL, &pcb, 0, 0);

	//  頂点バッファの更新（全書き換え）
	static Simple2DVertex  vertices[4] = {
		{   DirectX::XMFLOAT3(0.f,  0.f,1.f),DirectX::XMFLOAT2(0.f,0.f) },
		{   DirectX::XMFLOAT3(640.f,0.f,1.f),DirectX::XMFLOAT2(1.f,0.f) },
		{   DirectX::XMFLOAT3(0.f,  480.f,1.f),DirectX::XMFLOAT2(0.f,1.f) },
		{   DirectX::XMFLOAT3(640.f,480.f,1.f),DirectX::XMFLOAT2(1.f,1.f) }
	};

	vertices[0].position.x = vertices[2].position.x = x;
	vertices[1].position.x = vertices[3].position.x = x + w;
	vertices[0].position.y = vertices[1].position.y = y;
	vertices[2].position.y = vertices[3].position.y = y + h;

	pContext->UpdateSubresource(m_pVertexBuffer, 0, NULL, vertices, 0, 0);

	//　シェーダを設定
	pContext->VSSetShader(m_pVertexShader, NULL, 0);
	pContext->GSSetShader(NULL, NULL, 0);
	pContext->PSSetShader(m_pPixelShader, NULL, 0);

	//  頂点シェーダに定数バッファを設定
	pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	pContext->PSSetConstantBuffers(0, 1, &m_pPixelShaderConstantBuffer);

	// 入力アセンブラに頂点バッファを設定.
	UINT stride = sizeof(Simple2DVertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// プリミティブの種類を設定.
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	{
		ID3D11ShaderResourceView* ppShaderResourceViews[NUM_TEXTURES + 1];
		ID3D11SamplerState    *ppSamplerStates[] = { m_pSamplerState,0 };
		for (int i = 0; i < NUM_TEXTURES; ++i) {
			ppShaderResourceViews[i] = m_pTextures[i].pTextureShaderResourceView;
		}
		ppShaderResourceViews[NUM_TEXTURES] = NULL;
		pContext->PSSetShaderResources(0, NUM_TEXTURES, ppShaderResourceViews);
		pContext->PSSetSamplers(0, 1, ppSamplerStates);
	}
	// プリミティブ描画.
	pContext->Draw(4, 0);

}


/**
*  @brief デバイスコンテキストからバックバッファのサイズを確認する
*  @param [in] pContext  デバイスコンテキスト
*  @param [out] width    バックバッファ（画面）の幅
*  @param [out] height   バックバッファ（画面）の高さ
*  @return  S_OK : 成功 / それ以外 : 失敗
*/
static HRESULT GetBackbufferSize(ID3D11DeviceContext *pContext, DWORD &width, DWORD &height) {
	ID3D11RenderTargetView *pRenderTargetView = NULL;
	ID3D11DepthStencilView *pDepthStencilView = NULL;
	HRESULT result = E_FAIL;
	pContext->OMGetRenderTargets(1, &pRenderTargetView, &pDepthStencilView);
	if (pRenderTargetView != NULL) {
		D3D11_RENDER_TARGET_VIEW_DESC desc;
		pRenderTargetView->GetDesc(&desc);
		if (desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D) {
			ID3D11Texture2D *pTexture = NULL;
			D3D11_TEXTURE2D_DESC td;
			pRenderTargetView->GetResource((ID3D11Resource**)&pTexture);
			if (pTexture != NULL) {
				pTexture->GetDesc(&td);
				width = td.Width;
				height = td.Height;
				pTexture->Release();
				pTexture = NULL;
				result = S_OK;
			}
		}
	}
	SAFE_RELEASE(pRenderTargetView);
	SAFE_RELEASE(pDepthStencilView);
	return  result;
}