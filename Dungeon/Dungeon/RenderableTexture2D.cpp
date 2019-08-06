#include "stdafx.h"
#include <d3d11.h>
#include <directxmath.h>
#include <crtdbg.h>
#include "RenderableTexture2D.h"

#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if (o){ (o)->Release(); o = NULL; }
#define SAFE_DELETE(o)  if (o){ delete (o); o = NULL; }
#define SAFE_DELETE_ARRAY(o) if (o){ delete [] (o); o = NULL; }

/**
 *  @brief  ctor
 */
CRenderableTexture2D::CRenderableTexture2D(){
	m_pTexture = NULL;
	m_pTexRTV  = NULL;
	m_pTexSRV  = NULL;
}


/**
 *  @brief  destructor
 */
CRenderableTexture2D::~CRenderableTexture2D(){
	ReleaseDeviceObjects();
}

/**
 *  @brief  Restore device dependent objects
 */
HRESULT CRenderableTexture2D::RestoreDeviceObjects(ID3D11DeviceContext *pContext) {

	D3D11_TEXTURE2D_DESC texDesc;
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ID3D11Device *pDevice = NULL;
	HRESULT	hr;

	pContext->GetDevice(&pDevice);

	memset(&texDesc, 0, sizeof(texDesc));
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS;;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Width = RT_TEX_WIDTH;
	texDesc.Height = RT_TEX_HEIGHT;
	texDesc.CPUAccessFlags = 0;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	hr = pDevice->CreateTexture2D(&texDesc, NULL, &m_pTexture);
	if (SUCCEEDED(hr)){
		memset(&rtvDesc, 0, sizeof(rtvDesc));
		rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		// レンダーターゲットビューの生成
		hr = pDevice->CreateRenderTargetView(m_pTexture, &rtvDesc, &m_pTexRTV);
	}
	if (SUCCEEDED(hr)) {
		memset(&srvDesc, 0, sizeof(srvDesc));
		srvDesc.Format = rtvDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		// シェーダリソースビューの生成
		hr = pDevice->CreateShaderResourceView(m_pTexture, &srvDesc, &m_pTexSRV);
	}
	SAFE_RELEASE(pDevice);
	return hr;
}

/**
*  @brief  Release device dependent objects
*/
HRESULT CRenderableTexture2D::ReleaseDeviceObjects() {
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pTexRTV);
	SAFE_RELEASE(m_pTexSRV);
	return S_OK;
}

