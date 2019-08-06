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
* @brief	CDirectX11 �̎����N���X
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

	DWORD						m_Width;								//!	����
	DWORD						m_Height;								//!	����
};

DirectX11Impl *DirectX11Impl::m_pInstance = NULL;

/**
* @brief	�R���X�g���N�^
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
* @brief	�f�X�g���N�^
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
* @brief	������
*/
void DirectX11Impl::Initialize()
{
	if (m_pInstance == NULL)
		m_pInstance = new DirectX11Impl();
}

/**
* @brief	�I������
*/
void DirectX11Impl::Finalize()
{
	SAFE_DELETE(m_pInstance);
}

/**
* @brief	�C���X�^���X�擾
* @return	DirectX11ImplObjectInstance
*/
DirectX11Impl *DirectX11Impl::GetInstance()
{
	if (m_pInstance == NULL)
		DirectX11Impl::Initialize();

	return m_pInstance;
}

/**
* @brief	�f�o�C�X�R���e�L�X�g�ƃX���b�v�`�F�C���̐���
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

	// �f�o�C�X�����t���O.
	UINT createDeviceFlags = 0;

	// �h���C�o�[�^�C�v���
	D3D_DRIVER_TYPE driverTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTytpes = sizeof(driverTypes) / sizeof(driverTypes[0]);

	// �@�\���x�����
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

	// �X���b�v�`�F�C���̍\���ݒ�.
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
		// �h���C�o�[�^�C�v�ݒ�.
		D3D_DRIVER_TYPE DriverType = driverTypes[idx];

		// �f�o�C�X�ƃX���b�v�`�F�C���̐���.
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

		// ���������烋�[�v��E�o.
		if (SUCCEEDED(hr))
			break;
	}
	return hr;
}


/**
* @brief	�����_�[�^�[�Q�b�g�r���[�̐���
* @param	[in]	pDevice								ID3D11Device
* @param	[in]	pSwapChain							IDXGISwapChain
* @param	[out]	ppRenderTargetView					ID3D11RenderTargetView
* @param	[out]	ppRenderTargetShaderResourceView	ID3D11ShaderResourceView
*/
HRESULT DirectX11Impl::CreateRenderTargetView(ID3D11Device * pDevice, IDXGISwapChain * pSwapChain, ID3D11RenderTargetView ** ppRenderTargetView, ID3D11ShaderResourceView ** ppRenderTargetShaderResourceView)
{
	ID3D11Texture2D    *pBackBuffer = NULL;
	HRESULT hr = E_FAIL;

	//  �X���b�v�`�F�C�����o�b�N�o�b�t�@���擾.
	hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		goto ERROR_EXIT;

	//    �o�b�N�o�b�t�@���A�����_�[�^�[�Q�b�g�𐶐�.
	hr = pDevice->CreateRenderTargetView(pBackBuffer, NULL, ppRenderTargetView);
	if (FAILED(hr))
		goto ERROR_EXIT;

	// �����_�[�^�[�Q�b�g�̃V�F�[�_���\�[�X�r���[�𐶐�.
	hr = pDevice->CreateShaderResourceView(pBackBuffer, NULL, ppRenderTargetShaderResourceView);
	if (FAILED(hr))
		goto ERROR_EXIT;

ERROR_EXIT:
	SAFE_RELEASE(pBackBuffer);    //    �o�b�N�o�b�t�@���J��

	return hr;
}

/**
* @brief	DirectX11�̏�����
* @param	[in]	hWnd	WindowHandle
* @brief	�o�b�N�o�b�t�@�̃T�C�Y�Ńr���[�|�[�g�𐶐�
*/
HRESULT DirectX11Impl::DirectX11Initialize(HWND hWnd)
{
	HRESULT hr = S_OK;

	//	�f�o�C�X�E�f�o�C�X�R���e�L�X�g�E�X���b�v�`�F�C���̐���
	hr = CreateDeviceContextAndSwapChain(hWnd, &m_pDevice, &m_pDeviceContext, &m_pSwapChain, &m_DriverType, &m_FeatureLevel);

	//	�����_�[�^�[�Q�b�g�r���[�E�����_�[�^�[�Q�b�g�̃V�F�[�_�[���\�[�X�r���[�̐���
	hr = CreateRenderTargetView(m_pDevice, m_pSwapChain, &m_pRenderTargetView, &m_pRenderTargetShaderResourceView);

	//	�[�x�X�e���V���o�b�t�@�����̂��� �o�b�N�o�b�t�@ �̃T�C�Y���擾
	hr = GetBackBufferSize(m_pSwapChain, &m_Width, &m_Height);

	//	�T�C�Y�ƃt�H�[�}�b�g���w�肵�āA�[�x�X�e���V���o�b�t�@�𐶐�
	hr = CreateDepthStencilView(m_pDevice, m_Width, m_Height, DXGI_FORMAT_D16_UNORM, &m_pDepthStencilView, &m_pDepthStencilTexture, &m_pDepthStencilShaderResourceView);

	//  �f�o�C�X�R���e�L�X�g�Ƀ����_�[�^�[�Q�b�g�Ɛ[�x�X�e���V����ݒ�
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

	//	�t�H���g�̐���
	CDirectX11Font::Initialize(m_pDeviceContext);


	// �r���[�|�[�g�̐ݒ�.
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)m_Width;
	vp.Height = (FLOAT)m_Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;

	//	�r���[�|�[�g�̃Z�b�g
	m_pDeviceContext->RSSetViewports(1, &vp);


	return hr;
}


/**
* @brief	�`��̏�����
* @param	[in]	r	Red
* @param	[in]	g	Green
* @param	[in]	b	Blue
* @param	[in]	a	Alpha
*/
void DirectX11Impl::BeginScene(float r, float g, float b, float a)
{
	// �w��F�ŉ�ʃN���A
	float ClearColor[4] = { r, g, b, a };
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, ClearColor);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}


/**
* @brief	�`��̏I��
* @detail	�E�C���h�E�ɔ��f����
*/
HRESULT DirectX11Impl::EndScene()
{
	HRESULT hr = S_OK;
	//���ʂ��E�C���h�E�ɔ��f
	hr = m_pSwapChain->Present(1, 0);

	return hr;
}


/**
* @brief	�o�b�N�o�b�t�@�̃T�C�Y���擾����
* @param	[in]	pSwapChain	IDXGISwapChain
* @param	[out]	pWidth		����
* @param	[out]	pHeight		����
*/
HRESULT DirectX11Impl::GetBackBufferSize(IDXGISwapChain * pSwapChain, DWORD * pWidth, DWORD * pHeight)
{
	ID3D11Texture2D    *pBackBuffer = NULL;
	HRESULT hr = E_FAIL;
	D3D11_TEXTURE2D_DESC desc;
	//  �X���b�v�`�F�C�����o�b�N�o�b�t�@���擾.
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
* @brief	�[�x�X�e���V���r���[�̐���
* @param	[in]	pDevice								ID3D11Device
* @param	[in]	width								����
* @param	[in]	height								����
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

	//  �[�x�X�e���V���e�N�X�`���ƃV�F�[�_���\�[�X�r���[�̃t�H�[�}�b�g��
	//  �K�؂Ȃ��̂ɕύX.
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

	//    �[�x�X�e���V���e�N�X�`���̐���.
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

	// �[�x�X�e���V���e�N�X�`���̐���.
	hr = pDevice->CreateTexture2D(&td, NULL, ppDepthStencilTexture);
	if (FAILED(hr))
		goto ERROR_EXIT;

	// �[�x�X�e���V���r���[�̐ݒ�.
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

	// �[�x�X�e���V���r���[�̐���.
	hr = pDevice->CreateDepthStencilView(*ppDepthStencilTexture, &dsvd, ppDepthStencilView);
	if (FAILED(hr))
	{
		goto ERROR_EXIT;
	}

	// �V�F�[�_���\�[�X�r���[�̐ݒ�.
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

	// �V�F�[�_���\�[�X�r���[�𐶐�.
	hr = pDevice->CreateShaderResourceView(*ppDepthStencilTexture, &srvd, ppDepthStencilShaderResourceView);
	if (FAILED(hr))
	{
		goto ERROR_EXIT;
	}
ERROR_EXIT:
	return hr;
}



/**
* @brief	�h���C�o�[�^�C�v��\���e�L�X�g���擾����
* @return	TCHAR	�h���C�o�[�^�C�v��\���e�L�X�g
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
* @brief	�t�B�[�`���[���x����\���e�L�X�g���擾����
* @return	TCHAR	�t�B�[�`���[���x����\���e�L�X�g
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
* @brief	ID3D11Device�̎擾
* @return	ID3D11Device
*/
ID3D11Device * DirectX11Impl::GetDevice()
{
	if (m_pDevice)
		m_pDevice->AddRef();

	return m_pDevice;
}


/**
* @brief	ID3D11DeviceContext�̎擾
* @return	ID3D11DeviceContext
*/
ID3D11DeviceContext * DirectX11Impl::GetDeviceContext()
{
	if (m_pDeviceContext)
		m_pDeviceContext->AddRef();

	return m_pDeviceContext;
}


/**
* @brief	IDXGISwapChain�̎擾
* @return	IDXGISwapChain
*/
IDXGISwapChain * DirectX11Impl::GetSwapChain()
{
	if (m_pSwapChain)
		m_pSwapChain->AddRef();

	return m_pSwapChain;
}


/**
* @brief	ID3D11RenderTargetView�̎擾
*/
ID3D11RenderTargetView * DirectX11Impl::GetRenderTargetView()
{
	if (m_pRenderTargetView)
		m_pRenderTargetView->AddRef();

	return m_pRenderTargetView;
}


/**
* @brief	ID3D11ShaderResourceView�̎擾
* @return	ID3D11ShaderResourceView
*/
ID3D11ShaderResourceView * DirectX11Impl::GetRenderTargetShaderResourceView()
{
	if (m_pRenderTargetShaderResourceView)
		m_pRenderTargetShaderResourceView->AddRef();

	return m_pRenderTargetShaderResourceView;
}


/**
* @brief	ID3D11DepthStencilView�̎擾
* @return	ID3D11DepthStencilView
*/
ID3D11DepthStencilView * DirectX11Impl::GetDepthStencilView()
{
	if (m_pDepthStencilView)
		m_pDepthStencilView->AddRef();

	return m_pDepthStencilView;
}


/**
* @brief	DepthStencilTexture�̎擾
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
* @brief	�h���C�o�[�^�C�v���擾
* @return	D3D_DRIVER_TYPE
*/
D3D_DRIVER_TYPE DirectX11Impl::GetDriverType()
{
	return m_DriverType;
}


/**
* @brief	�t�B�[�`���[���x�����擾
* @return	D3D_FEATURE_LEVEL
*/
D3D_FEATURE_LEVEL DirectX11Impl::GetFeatureLevel()
{
	return m_FeatureLevel;
}


/************************************************
* @class	CDirectX11							*
* @brief	DirectX11Impl �̃��b�p�[�N���X		*
************************************************/

/**
* @brief	������
* @param	[in]	hWnd	WindowHandle
*/
HRESULT CDirectX11::Initialize(HWND hWnd)
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();
	
	HRESULT hr = S_OK;

	//	DirectX11�̏�����
	hr = pObj->DirectX11Initialize(hWnd);
	if (FAILED(hr))
		goto ERROR_EXIT;


ERROR_EXIT:
	return hr;
}


/**
* @brief	�I������
*/
void CDirectX11::Finalize()
{
	DirectX11Impl::Finalize();
}


/**
* @brief	�`��J�n
*/
void CDirectX11::BeginScene()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	�`��̏�����
	pObj->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);
}


/**
* @brief	�`��̏I��
* @return	HRESULT
*/
HRESULT CDirectX11::EndScene()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	HRESULT hr = S_OK;


	//	�`��̏I��
	if (FAILED(hr = pObj->EndScene())) {
		//  �f�o�C�X���X�g�ւ̑Ώ��͉\�����A�����ł͒P�ɏI�����鎖�ɂ���
		if (hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_REMOVED) {
			MessageBox(NULL, _T("Device �������܂����̂ŏI�����܂�"), _T("Assert"), MB_OK);
			return hr;
		}
	}

	return hr;
}


/**
* @brief	ID3D11Device�̎擾
* @return	ID3D11Device
*/
ID3D11Device * CDirectX11::GetDevice()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	�f�o�C�X�̎擾
	return pObj->GetDevice();
}


/**
* @brief	ID3D11DeviceContext�̎擾
* @return	ID3D11DeviceContext
*/
ID3D11DeviceContext * CDirectX11::GetDeviceContext()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	�f�o�C�X�R���e�L�X�g�̎擾
	return pObj->GetDeviceContext();
}


/**
* @brief	�X���b�v�`�F�C���擾
* @return	IDXGISwapChain
*/
IDXGISwapChain * CDirectX11::GetSwapChain()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	�X���b�v�`�F�C���̎擾
	return pObj->GetSwapChain();
}


/**
* @brief	ID3D11RenderTargetView�̎擾
* @return	ID3D11RenderTargetView
*/
ID3D11RenderTargetView * CDirectX11::GetRenderTargetView()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	�����_�[�^�[�Q�b�g�r���[�̎擾
	return pObj->GetRenderTargetView();
}


/**
* @brief	RenderTarget��ID3D11ShaderResourceView�̎擾
* @return	ID3D11ShaderResourceView
*/
ID3D11ShaderResourceView * CDirectX11::GetRenderTargetShaderResourceView()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	�����_�[�^�[�Q�b�g�̃V�F�[�_�[���\�[�X�r���[�̎擾
	return pObj->GetRenderTargetShaderResourceView();
}


/**
* @brief	ID3D11DepthStencilView�̎擾
* @return	ID3D11DepthStencilView
*/
ID3D11DepthStencilView * CDirectX11::GetDepthStenilView()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	�[�x�X�e���V���r���[�̎擾
	return pObj->GetDepthStencilView();
}


/**
* @brief	DepthStencil��ID3D11Texture2D�̎擾
* @return	ID3D11Texture2D
*/
ID3D11Texture2D * CDirectX11::GetDepthStencilTexture()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	�[�x�X�e���V���e�N�X�`���̎擾
	return pObj->GetDepthStencilTexture();
}


/**
* @brief	DepthStencil��ShaderResourceView�̎擾
* @return	ID3D11ShaderResourceView
*/
ID3D11ShaderResourceView * CDirectX11::GetDepthStencilShaderResourceView()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	�[�x�X�e���V���̃V�F�[�_�[���\�[�X�r���[�̎擾
	return pObj->GetDepthStencilShaderResourceView();
}


/**
* @brief	�h���C�o�[�^�C�v�ɉ������e�L�X�g���擾
* @return	TCHAR	DriverType�ɉ�����������
*/
const TCHAR * CDirectX11::GetDeviceDriverTypeText()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	�h���C�o�[�^�C�v�擾
	D3D_DRIVER_TYPE dt = pObj->GetDriverType();

	//	�h���C�o�[�^�C�v�ɉ������������Ԃ�
	return pObj->GetDriverTypeText(dt);
}


/**
* @brief	�t�B�[�`���[���x���ɉ������e�L�X�g���擾
* @return	TCHAR	FeatureLevel�ɉ�����������
*/
const TCHAR * CDirectX11::GetFeatureLevelText()
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	//	�t�B�[�`���[���x�����擾
	D3D_FEATURE_LEVEL fl = pObj->GetFeatureLevel();

	//	�t�B�[�`���[���x���ɉ������������Ԃ�
	return pObj->GetFeatureLevelText(fl);
}


/**
* @brief	�o�b�N�o�b�t�@�̃T�C�Y���擾����
* @param	[out]	width	����
* @param	[out]	height	����
*/
void CDirectX11::GetBackBufferSize(DWORD *width, DWORD *height)
{
	DirectX11Impl *pObj = DirectX11Impl::GetInstance();

	pObj->GetBackBufferSize(pObj->GetSwapChain(), width, height);
}


