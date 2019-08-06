#include "stdafx.h"
#include "DirectX11.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <vector>

#include "DirectX11Font.h"

#define SAFE_RELEASE(o) if (o){ (o)->Release(); o = NULL; };
#define SAFE_DELETE(o) if (o){ delete(o); o = NULL; };
#define SAFE_DELETE_ARRAY(o) if (o){ delete[](o); o = NULL; };

using namespace DirectX;
using namespace std;

//  import libraries
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")


/**
* @class	DirectX11Impl
* @brief	CDirectX11 の実装クラス
*/
class DirectX11Impl
{
	friend CDirectX11;
private:
	DirectX11Impl();		
	~DirectX11Impl();		

	static	void			Initialize();
	static	void			Finalize();
	static	DirectX11Impl	*GetInstance();

	HRESULT DirectX11Initialize(HWND hWnd);
	void	BeginScene(float r, float g, float b, float a);
	HRESULT	EndScene();

	HRESULT GetBackBufferSize(IDXGISwapChain *pSwapChain, DWORD *pWidth, DWORD *pHeight);
	HRESULT    CreateDepthStencilView(
		ID3D11Device *pDevice,
		INT          width,
		INT          height,
		DXGI_FORMAT  fmtDepthStencil,
		ID3D11DepthStencilView    **ppDepthStencilView,
		ID3D11Texture2D           **ppDepthStencilTexture,
		ID3D11ShaderResourceView  **ppDepthStencilShaderResourceView
	);
	HRESULT CreateRenderTargetView(
		ID3D11Device             *pDevice,
		IDXGISwapChain           *pSwapChain,
		ID3D11RenderTargetView   **ppRenderTargetView,
		ID3D11ShaderResourceView **ppRenderTargetShaderResourceView
	);
	HRESULT CreateDeviceContextAndSwapChain(
		HWND                  hWnd,
		ID3D11Device          **ppDevice,
		ID3D11DeviceContext   **ppContext,
		IDXGISwapChain        **ppSwapChain,
		D3D_DRIVER_TYPE       *pDriverType,
		D3D_FEATURE_LEVEL     *pFeatureLevel
	);

	const TCHAR *GetDriverTypeText(D3D_DRIVER_TYPE dt);
	const TCHAR *GetFeatureLevelText(D3D_FEATURE_LEVEL fl);

	ID3D11Device				*GetDevice();
	ID3D11DeviceContext			*GetDeviceContext();
	IDXGISwapChain				*GetSwapChain();
	ID3D11RenderTargetView		*GetRenderTargetView();
	ID3D11ShaderResourceView	*GetRenderTargetShaderResourceView();
	ID3D11DepthStencilView		*GetDepthStencilView();
	ID3D11Texture2D				*GetDepthStencilTexture();
	ID3D11ShaderResourceView	*GetDepthStencilShaderResourceView();
	D3D_DRIVER_TYPE				GetDriverType();
	D3D_FEATURE_LEVEL			GetFeatureLevel();

	static	DirectX11Impl		*m_pInstance;

	ID3D11Device				*m_pDevice;								//!	Device
	ID3D11DeviceContext			*m_pDeviceContext;						//!	DeviceContext
	IDXGISwapChain				*m_pSwapChain;							//!	SwapChain
	ID3D11RenderTargetView		*m_pRenderTargetView;					//!	RenderTargetView
	ID3D11ShaderResourceView	*m_pRenderTargetShaderResourceView;		//!	RenderTargetShaderResourceView
	ID3D11DepthStencilView		*m_pDepthStencilView;					//!	DepthStencilView
	ID3D11Texture2D				*m_pDepthStencilTexture;				//!	DepthStencilTexture
	ID3D11ShaderResourceView	*m_pDepthStencilShaderResourceView;		//!	DepthStencilShaderResourceView
	D3D_DRIVER_TYPE				m_DriverType;							//!	DriverType
	D3D_FEATURE_LEVEL			m_FeatureLevel;							//!	FeatureLevel

	DWORD						m_Width;								//!	横幅
	DWORD						m_Height;								//!	高さ
};

DirectX11Impl *DirectX11Impl::m_pInstance = NULL;

/**
* @brief	コンストラクタ
*/
DirectX11Impl::DirectX11Impl()
{
	m_pDevice = NULL;
	m_pDeviceContext = NULL;
	m_pSwapChain = NULL;
	m_pRenderTargetView = NULL;
	m_pRenderTargetShaderResourceView = NULL;
	m_pDepthStencilView = NULL;
	m_pDepthStencilTexture = NULL;
	m_pDepthStencilShaderResourceView = NULL;
	m_DriverType = D3D_DRIVER_TYPE_NULL;

	m_Width = 0;
	m_Height = 0;
}

/**
* @brief	デストラクタ
*/
DirectX11Impl::~DirectX11Impl()
{
	CDirectX11Font::Finalize();

	if (m_pDeviceContext) {
		m_pDeviceContext->ClearState();
		m_pDeviceContext->Flush();
	}
	
	SAFE_RELEASE(m_pDepthStencilShaderResourceView);
	SAFE_RELEASE(m_pDepthStencilTexture);
	SAFE_RELEASE(m_pDepthStencilView);
	SAFE_RELEASE(m_pRenderTargetShaderResourceView);
	SAFE_RELEASE(m_pRenderTargetView);
	SAFE_RELEASE(m_pSwapChain);
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
}

/**
* @brief	初期化
*/
void DirectX11Impl::Initialize()
{
	if (m_pInstance == NULL)
		m_pInstance = new DirectX11Impl();
}

/**
* @brief	終了処理
*/
void DirectX11Impl::Finalize()
{
	SAFE_DELETE(m_pInstance);
}

/**
* @brief	インスタンス取得
* @return	DirectX11ImplObjectInstance
*/
DirectX11Impl *DirectX11Impl::GetInstance()
{
	if (m_pInstance == NULL)
		DirectX11Impl::Initialize();

	return m_pInstance;
}

/**
* @brief	デバイスコンテキストとスワップチェインの生成
* @param	[in]	hWnd			HWND
* @param	[out]	ppDevice		ID3D11Device
* @param	[out]	ppContext		ID3D11DeviceContext
* @param	[out]	ppSwapChain		IDXGISwapChain
* @param	[out]	pDriverType		D3D_DRIVER_TYPE
* @param	[out]	pFeatureLevel	D3D_FEATURE_LEVEL
*/
HRESULT DirectX11Impl::CreateDeviceContextAndSwapChain(HWND hWnd, ID3D11Device ** ppDevice, ID3D11DeviceContext ** ppContext, IDXGISwapChain ** ppSwapChain, D3D_DRIVER_TYPE * pDriverType, D3D_FEATURE_LEVEL * pFeatureLevel)
{
	HRESULT hr = E_FAIL;

	// デバイス生成フラグ.
	UINT createDeviceFlags = 0;

	// ドライバータイプ候補
	D3D_DRIVER_TYPE driverTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTytpes = sizeof(driverTypes) / sizeof(driverTypes[0]);

	// 機能レベル候補
	static const D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = sizeof(featureLevels) / sizeof(featureLevels[0]);
	RECT rc;
	GetClientRect(hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	// スワップチェインの構成設定.
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(DXGI_SWAP_CHAIN_DESC));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT idx = 0; idx < numDriverTytpes; ++idx) {
		// ドライバータイプ設定.
		D3D_DRIVER_TYPE DriverType = driverTypes[idx];

		// デバイスとスワップチェインの生成.
		hr = D3D11CreateDeviceAndSwapChain(
			NULL,
			DriverType,
			NULL,
			createDeviceFlags,
			featureLevels,
			numFeatureLevels,
			D3D11_SDK_VERSION,
			&sd,
			ppSwapChain,
			ppDevice,
			pFeatureLevel,
			ppContext
		);
		*pDriverType = DriverType;

		// 成功したらループを脱出.
		if (SUCCEEDED(hr))
			break;
	}
	return hr;
}


/**
* @brief	レンダーターゲットビューの生成
* @param	[in]	pDevice								ID3D11Device
* @param	[in]	pSwapChain							IDXGISwapChain
* @param	[out]	ppRenderTargetView					ID3D11RenderTargetView
* @param	[out]	ppRenderTargetShaderResourceView	ID3D11ShaderResourceView
*/
HRESULT DirectX11Impl::CreateRenderTargetView(ID3D11Device * pDevice, IDXGISwapChain * pSwapChain, ID3D11RenderTargetView ** ppRenderTargetView, ID3D11ShaderResourceView ** ppRenderTargetShaderResourceView)
{
	ID3D11Texture2D    *pBackBuffer = NULL;
	HRESULT hr = E_FAIL;

	//  スワップチェインよりバックバッファを取得.
	hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		goto ERROR_EXIT;

	//    バックバッファより、レンダーターゲットを生成.
	hr = pDevice->CreateRenderTargetView(pBackBuffer, NULL, ppRenderTargetView);
	if (FAILED(hr))
		goto ERROR_EXIT;

	// レンダーターゲットのシェーダリソースビューを生成.
	hr = pDevice->CreateShaderResourceView(pBackBuffer, NULL, ppRenderTargetShaderResourceView);
	if (FAILED(hr))
		goto ERROR_EXIT;

ERROR_EXIT:
	SAFE_RELEASE(pBackBuffer);    //    バックバッファを開放

	return hr;
}

/**
* @brief	DirectX11の初期化
* @param	[in]	hWnd	WindowHandle
* @brief	バックバッファのサイズでビューポートを生成
*/
HRESULT DirectX11Impl::DirectX11Initialize(HWND hWnd)
{
	HRESULT hr = S_OK;

	//	デバイス・デバイスコンテキスト・スワップチェインの生成
	hr = CreateDeviceContextAndSwapChain(hWnd, &m_pDevice, &m_pDeviceContext, &m_pSwapChain, &m_DriverType, &m_FeatureLevel);

	//	レンダーターゲットビュー・レンダーターゲットのシェーダーリソースビューの生成
	hr = CreateRenderTargetView(m_pDevice, m_pSwapChain, &m_pRenderTargetView, &m_pRenderTargetShaderResourceView);

	//	深度ステンシルバッファ生成のため バックバッファ のサイズを取得
	hr = GetBackBufferSize(m_pSwapChain, &m_Width, &m_Height);

	//	サイズとフォーマットを指定して、深度ステンシルバッファを生成
	hr = CreateDepthStencilView(m_pDevice, m_Width, m_Height, DXGI_FORMAT_D16_UNORM, &m_pDepthStencilView, &m_pDepthStencilTexture, &m_pDepthStencilShaderResourceView);

	//  デバイスコンテキストにレンダーターゲットと深度ステンシルを設定
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

	//	フォントの生成
	CDirectX11Font::Initialize(m_pDeviceContext);


	// ビューポートの設定.
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)m_Width;
	vp.Height = (FLOAT)m_Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;

	//	ビューポートのセット
	m_pDeviceContext->RSSetViewports(1, &vp);


	return hr;
}


/**
* @brief	描画の初期化
* @param	[in]	r	Red
* @param	[in]	g	Green
* @param	[in]	b	Blue
* @param	[in]	a	Alpha
*/
void DirectX11Impl::BeginScene(float r, float g, float b, float a)
{
	// 指定色で画面クリア
	float ClearColor[4] = { r, g, b, a };
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, ClearColor);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}


/**
* @brief	描画の終了
* @detail	ウインドウに反映する
*/
HRESULT DirectX11Impl::EndScene()
{
	HRESULT hr = S_OK;
	//結果をウインドウに反映
	hr = m_pSwapChain->Present(1, 0);

	return hr;
}


/**
* @brief	バックバッファのサイズを取得する
* @param	[in]	pSwapChain	IDXGISwapChain
* @param	[out]	pWidth		横幅
* @param	[out]	pHeight		高さ
*/
HRESULT DirectX11Impl::GetBackBufferSize(IDXGISwapChain * pSwapChain, DWORD * pWidth, DWORD * pHeight)
{
	ID3D11Texture2D    *pBackBuffer = NULL;
	HRESULT hr = E_FAIL;
	D3D11_TEXTURE2D_DESC desc;
	//  スワップチェインよりバックバッファを取得.
	hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		(LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		goto ERROR_EXIT;
	pBackBuffer->GetDesc(&desc);

	*pWidth = (DWORD)desc.Width;
	*pHeight = (DWORD)desc.Height;
	hr = S_OK;

ERROR_EXIT:
	SAFE_RELEASE(pBackBuffer);
	return hr;
}


/**
* @brief	深度ステンシルビューの生成
* @param	[in]	pDevice								ID3D11Device
* @param	[in]	width								横幅
* @param	[in]	height								高さ
* @param	[in]	fmtDepthStencil						DXGI_FORMAT
* @param	[out]	ppDepthStencilView					ID3D11DepthStencilView
* @param	[out]	ppDepthStencilTexture				ID3D11Texture2D
* @param	[out]	ppDepthStencilShaderResourceView	ID3D11ShaderResourceView
*/
HRESULT DirectX11Impl::CreateDepthStencilView(ID3D11Device * pDevice, INT width, INT height, DXGI_FORMAT fmtDepthStencil, ID3D11DepthStencilView ** ppDepthStencilView, ID3D11Texture2D ** ppDepthStencilTexture, ID3D11ShaderResourceView ** ppDepthStencilShaderResourceView)
{
	DXGI_FORMAT textureFormat = DXGI_FORMAT_R16_TYPELESS;
	DXGI_FORMAT resourceFormat = DXGI_FORMAT_R16_UNORM;
	HRESULT hr = E_FAIL;

	//  深度ステンシルテクスチャとシェーダリソースビューのフォーマットを
	//  適切なものに変更.
	switch (fmtDepthStencil)
	{
	case DXGI_FORMAT_D16_UNORM:
		textureFormat = DXGI_FORMAT_R16_TYPELESS;
		resourceFormat = DXGI_FORMAT_R16_UNORM;
		break;

	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		textureFormat = DXGI_FORMAT_R24G8_TYPELESS;
		resourceFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		break;

	case DXGI_FORMAT_D32_FLOAT:
		textureFormat = DXGI_FORMAT_R32_TYPELESS;
		resourceFormat = DXGI_FORMAT_R32_FLOAT;
		break;

	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		textureFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
		resourceFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		break;
	default:
		hr = E_FAIL;
		goto ERROR_EXIT;
	}

	//    深度ステンシルテクスチャの生成.
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(D3D11_TEXTURE2D_DESC));
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = textureFormat;
	td.SampleDesc.Count = 1;    //    MULTI SAMPLE COUNT
	td.SampleDesc.Quality = 0;    //    MULtI SAMPLE QUALITY
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags = 0;
	td.MiscFlags = 0;

	// 深度ステンシルテクスチャの生成.
	hr = pDevice->CreateTexture2D(&td, NULL, ppDepthStencilTexture);
	if (FAILED(hr))
		goto ERROR_EXIT;

	// 深度ステンシルビューの設定.
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	dsvd.Format = fmtDepthStencil;
	if (td.SampleDesc.Count == 0)
	{
		dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvd.Texture2D.MipSlice = 0;
	}
	else
	{
		dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	}

	// 深度ステンシルビューの生成.
	hr = pDevice->CreateDepthStencilView(*ppDepthStencilTexture, &dsvd, ppDepthStencilView);
	if (FAILED(hr))
	{
		goto ERROR_EXIT;
	}

	// シェーダリソースビューの設定.
	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	ZeroMemory(&srvd, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvd.Format = resourceFormat;

	if (td.SampleDesc.Count == 0)
	{
		srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvd.Texture2D.MostDetailedMip = 0;
		srvd.Texture2D.MipLevels = 1;
	}
	else
	{
		srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	}

	// シェーダリソースビューを生成.
	hr = pDevice->CreateShaderResourceView(*ppDepthStencilTexture, &srvd, ppDepthStencilShaderResourceView);
	if (FAILED(hr))
	{
		goto ERROR_EXIT;
	}
ERROR_EXIT:
	return hr;
}



/**
* @brief	ドライバータイプを表すテキストを取得する
* @return	TCHAR	ドライバータイプを表すテキスト
*/
const TCHAR * DirectX11Impl::GetDriverTypeText(D3D_DRIVER_TYPE dt)
{
	switch (dt) {
	case    D3D_DRIVER_TYPE_UNKNOWN:
		return _T("D3D_DRIVER_TYPE_UNKNOWN");
	case    D3D_DRIVER_TYPE_HARDWARE:
		return _T("D3D_DRIVER_TYPE_HARDWARE");
	case    D3D_DRIVER_TYPE_REFERENCE:
		return _T("D3D_DRIVER_TYPE_REFERENCE");
	case    D3D_DRIVER_TYPE_NULL:
		return _T("D3D_DRIVER_TYPE_NULL");
	case    D3D_DRIVER_TYPE_SOFTWARE:
		return _T("D3D_DRIVER_TYPE_SOFTWARE");
	case    D3D_DRIVER_TYPE_WARP:
		return _T("D3D_DRIVER_TYPE_WARP");
	}
	return NULL;
}


/**
* @brief	フィーチャーレベルを表すテキストを取得する
* @return	TCHAR	フィーチャーレベルを表すテキスト
*/
const TCHAR * DirectX11Impl::GetFeatureLevelText(D3D_FEATURE_LEVEL fl)
{
	switch (fl) {
	case    D3D_FEATURE_LEVEL_9_1:
		return _T("D3D_FEATURE_LEVEL_9_1");
	case    D3D_FEATURE_LEVEL_9_2:
		return _T("D3D_FEATURE_LEVEL_9_2");
	case    D3D_FEATURE_LEVEL_9_3:
		return _T("D3D_FEATURE_LEVEL_9_3");
	case    D3D_FEATURE_LEVEL_10_0:
		return _T("D3D_FEATURE_LEVEL_10_0");
	case    D3D_FEATURE_LEVEL_10_1:
		return _T("D3D_FEATURE_LEVEL_10_1");
	case    D3D_FEATURE_LEVEL_11_0:
		return _T("D3D_FEATURE_LEVEL_11_0");
	case    D3D_FEATURE_LEVEL_11_1:
		return _T("D3D_FEATURE_LEVEL_11_1");
	}
	return NULL;
}


/**
* @brief	ID3D11Deviceの取得
* @return	ID3D11Device
*/
ID3D11Device * DirectX11Impl::GetDevice()
{
	if (m_pDevice)
		m_pDevice->AddRef();

	return m_pDevice;
}


/**
* @brief	ID3D11DeviceContextの取得
* @return	ID3D11DeviceContext
*/
ID3D11DeviceContext * DirectX11Impl::GetDeviceContext()
{
	if (m_pDeviceContext)
		m_pDeviceContext->AddRef();

	return m_pDeviceContext;
}


/**
* @brief	IDXGISwapChainの取得
* @return	IDXGISwapChain
*/
IDXGISwapChain * DirectX11Impl::GetSwapChain()
{
	if (m_pSwapChain)
		m_pSwapChain->AddRef();

	return m_pSwapChain;
}


/**
* @brief	ID3D11RenderTargetViewの取得
*/
ID3D11RenderTargetView * DirectX11Impl::GetRenderTargetView()
{
	if (m_pRenderTargetView)
		m_pRenderTargetView->AddRef();

	return m_pRenderTargetView;
}


/**
* @brief	ID3D11ShaderResourceViewの取得
* @return	ID3D11ShaderResourceView
*/
ID3D11ShaderResourceView * DirectX11Impl::GetRenderTargetShaderResourceView()
{
	if (m_pRenderTargetShaderResourceView)
		m_pRenderTargetShaderResourceView->AddRef();

	return m_pRenderTargetShaderResourceView;
}


/**
* @brief	ID3D11DepthStencilViewの取得
* @return	ID3D11DepthStencilView
*/
ID3D11DepthStencilView * DirectX11Impl::GetDepthStencilView()
{
	if (m_pDepthStencilView)
		m_pDepthStencilView->AddRef();

	return m_pDepthStencilView;
}


/**
* @brief	DepthStencilTextureの取得
* @return	ID3D11Texture2D
*/
ID3D11Texture2D * DirectX11Impl::GetDepthStencilTexture()
{
	if (m_pDepthStencilTexture)
		m_pDepthStencilTexture->AddRef();

	return m_pDepthStencilTexture;
}


/**
* @brief	DepthStencilShaderResourceView
* @retrun	ID3D11ShaderResourceView
*/
ID3D11ShaderResourceView * DirectX11Impl::GetDepthStencilShaderResourceView()
{
	if (m_pDepthStencilShaderResourceView)
		m_pDepthStencilShaderResourceView->AddRef();

	return m_pDepthStencilShaderResourceView;
}


/**
* @brief	ドライバータイプを取得
* @return	D3D_DRIVER_TYPE
*/
D3D_DRIVER_TYPE DirectX11Impl::GetDriverType()
{
	return m_DriverType;
}


/**
* @brief	フィーチャーレベルを取得
* @return	D3D_FEATURE_LEVEL
*/
D3D_FEATURE_LEVEL DirectX11Impl::GetFeatureLevel()
{
	return m_FeatureLevel;
}


/************************************************
* @class	CDirectX11							*
* @brief	DirectX11Impl のラッパークラス		*
************************************************/

/**
* @brief	初期化
* @param	[in]	hWnd	WindowHandle
*/
HRESULT CDirectX11::Initialize(HWND hWnd)
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();
	
	HRESULT hr = S_OK;

	//	DirectX11の初期化
	hr = pObj->DirectX11Initialize(hWnd);
	if (FAILED(hr))
		goto ERROR_EXIT;


ERROR_EXIT:
	return hr;
}


/**
* @brief	終了処理
*/
void CDirectX11::Finalize()
{
	DirectX11Impl::Finalize();
}


/**
* @brief	描画開始
*/
void CDirectX11::BeginScene()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	描画の初期化
	pObj->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);
}


/**
* @brief	描画の終了
* @return	HRESULT
*/
HRESULT CDirectX11::EndScene()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	HRESULT hr = S_OK;


	//	描画の終了
	if (FAILED(hr = pObj->EndScene())) {
		//  デバイスロストへの対処は可能だが、ここでは単に終了する事にする
		if (hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_REMOVED) {
			MessageBox(NULL, _T("Device が失われましたので終了します"), _T("Assert"), MB_OK);
			return hr;
		}
	}

	return hr;
}


/**
* @brief	ID3D11Deviceの取得
* @return	ID3D11Device
*/
ID3D11Device * CDirectX11::GetDevice()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	デバイスの取得
	return pObj->GetDevice();
}


/**
* @brief	ID3D11DeviceContextの取得
* @return	ID3D11DeviceContext
*/
ID3D11DeviceContext * CDirectX11::GetDeviceContext()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	デバイスコンテキストの取得
	return pObj->GetDeviceContext();
}


/**
* @brief	スワップチェイン取得
* @return	IDXGISwapChain
*/
IDXGISwapChain * CDirectX11::GetSwapChain()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	スワップチェインの取得
	return pObj->GetSwapChain();
}


/**
* @brief	ID3D11RenderTargetViewの取得
* @return	ID3D11RenderTargetView
*/
ID3D11RenderTargetView * CDirectX11::GetRenderTargetView()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	レンダーターゲットビューの取得
	return pObj->GetRenderTargetView();
}


/**
* @brief	RenderTargetのID3D11ShaderResourceViewの取得
* @return	ID3D11ShaderResourceView
*/
ID3D11ShaderResourceView * CDirectX11::GetRenderTargetShaderResourceView()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	レンダーターゲットのシェーダーリソースビューの取得
	return pObj->GetRenderTargetShaderResourceView();
}


/**
* @brief	ID3D11DepthStencilViewの取得
* @return	ID3D11DepthStencilView
*/
ID3D11DepthStencilView * CDirectX11::GetDepthStenilView()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	深度ステンシルビューの取得
	return pObj->GetDepthStencilView();
}


/**
* @brief	DepthStencilのID3D11Texture2Dの取得
* @return	ID3D11Texture2D
*/
ID3D11Texture2D * CDirectX11::GetDepthStencilTexture()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	深度ステンシルテクスチャの取得
	return pObj->GetDepthStencilTexture();
}


/**
* @brief	DepthStencilのShaderResourceViewの取得
* @return	ID3D11ShaderResourceView
*/
ID3D11ShaderResourceView * CDirectX11::GetDepthStencilShaderResourceView()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	深度ステンシルのシェーダーリソースビューの取得
	return pObj->GetDepthStencilShaderResourceView();
}


/**
* @brief	ドライバータイプに応じたテキストを取得
* @return	TCHAR	DriverTypeに応じた文字列
*/
const TCHAR * CDirectX11::GetDeviceDriverTypeText()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	ドライバータイプ取得
	D3D_DRIVER_TYPE dt = pObj->GetDriverType();

	//	ドライバータイプに応じた文字列を返す
	return pObj->GetDriverTypeText(dt);
}


/**
* @brief	フィーチャーレベルに応じたテキストを取得
* @return	TCHAR	FeatureLevelに応じた文字列
*/
const TCHAR * CDirectX11::GetFeatureLevelText()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	フィーチャーレベルを取得
	D3D_FEATURE_LEVEL fl = pObj->GetFeatureLevel();

	//	フィーチャーレベルに応じた文字列を返す
	return pObj->GetFeatureLevelText(fl);
}


/**
* @brief	バックバッファのサイズを取得する
* @param	[out]	width	横幅
* @param	[out]	height	高さ
*/
void CDirectX11::GetBackBufferSize(DWORD *width, DWORD *height)
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	pObj->GetBackBufferSize(pObj->GetSwapChain(), width, height);
}


