#include "stdafx.h"
#include <D3D11.h>
#include <D3Dcompiler.h>
#include "DirectX11Font.h"

#define SAFE_RELEASE(o) if(o) { (o)->Release(); o = NULL; };
#define SAFE_DELETE(o) if(o) { delete(o); o = NULL; };
#define SADE_DELETE_ARRAY(o) if(o) {delete[](o); o = NULL; };

#define D3DFONT_BOLD        0x0001
#define D3DFONT_ITALIC      0x0002
#define D3DFONT_SIZE_IN_PIXELS    0x1000

//  �t�H���g�̃����_�����O���Ɏw�肷��t���O
#define D3DFONT_FILTERED    0x0004
#ifndef __d3d11_h__
#include <d3d11.h>
#endif
#ifndef DIRECTX_MATH_VERSION
#include <directxmath.h>
#endif




static char shadercode[] = "\
//-----------------------------------------------------------------------------------\n\
//  texture sampler\n\
//-----------------------------------------------------------------------------------\n\
SamplerState textureSampler : register(s0);\n\
Texture2D    diffuseTexture : register(t0);\n\
\n\
\n\
//-----------------------------------------------------------------------------------\n\
// Constant buffer\n\
//-----------------------------------------------------------------------------------\n\
cbuffer VSConstantBuffer : register( b0 )\n\
{\n\
    matrix  Proj;   //  projection matrix\n\
    float4  Color;  //  font color\n\
};\n\
\n\
\n\
//-----------------------------------------------------------------------------------\n\
// VSInput structure\n\
//-----------------------------------------------------------------------------------\n\
struct VSInput\n\
{\n\
    float3 Position : POSITION;     //  position\n\
    float2 texCoord : TEXCOORD0;    //  texture coordinates\n\
};\n\
\n\
//-----------------------------------------------------------------------------------\n\
// GSPSInput structure\n\
//-----------------------------------------------------------------------------------\n\
struct GSPSInput\n\
{\n\
    float4 Position : SV_POSITION;  //  position\n\
    float2 texCoord : TEXCOORD0;    //  texture coordinates\n\
    float4 diffuse  : COLOR;        //  font color\n\
};\n\
\n\
//-----------------------------------------------------------------------------------\n\
//  Vertex Shader Entry point\n\
//-----------------------------------------------------------------------------------\n\
GSPSInput VSFunc( VSInput input )\n\
{\n\
    GSPSInput output = (GSPSInput)0;\n\
\n\
    // ���̓f�[�^��float4 �֕ϊ�.\n\
    float4 pos = float4( input.Position, 1.0f );\n\
\n\
    // �ˉe��Ԃɕϊ�.\n\
    float4 projPos  = mul( Proj,  pos );\n\
\n\
    output.Position = projPos;\n\
    output.texCoord = input.texCoord;\n\
    output.diffuse  = Color;\n\
    return output;\n\
}\n\
\n\
//------------------------------------------------------------------------------------\n\
//  Pixel shader entry point\n\
//------------------------------------------------------------------------------------\n\
float4 PSFunc( GSPSInput psin ) : SV_TARGET0\n\
{\n\
    float4 pixel = diffuseTexture.Sample(textureSampler, psin.texCoord);\n\
    pixel = psin.diffuse * pixel;\n\
    return pixel;\n\
}\n\
\n";


struct AnkFontVertex
{
	DirectX::XMFLOAT3 position;     //  position
	DirectX::XMFLOAT2 texture;      //  texture coordinates
};

typedef struct {
	DirectX::XMMATRIX    matProj;
	DirectX::XMFLOAT4    color;
}    AnkFontConstantBuffer;

__declspec(align(16))	class DirectX11FontImpl
{
	friend CDirectX11Font;
private:
	DirectX11FontImpl();
	virtual ~DirectX11FontImpl(void);
	
	static	void				Initialize();
	static	void				Finalize();
	static	DirectX11FontImpl	*GetInstance();

	virtual HRESULT	RestoreDeviceObjects(ID3D11DeviceContext *pContext);
	virtual HRESULT	ReleaseDeviceObjects(ID3D11DeviceContext *pContext);

	virtual void    Render(ID3D11DeviceContext *pContext);
	HRESULT			CreateFontTexture();

	virtual void	DrawAnkText(ID3D11DeviceContext *pContext, const TCHAR *pString, DirectX::XMFLOAT4 color, FLOAT x, FLOAT y);
	virtual void	DrawChar(ID3D11DeviceContext *pContext, const char ankCode, DirectX::XMFLOAT4 color, FLOAT *px, FLOAT *py);

	//  keep 16-byte aligned
	void *operator new(size_t size) {
		return _mm_malloc(size, 16);
	}
	void operator delete(void *p) {
		return _mm_free(p);
	}

	HRESULT CompileShaderFromMemory(BYTE *code, DWORD dwSize, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob **ppBlobOut);


	static	DirectX11FontImpl	*m_pInstance;

	ID3D11DeviceContext			*m_pDeviceContext;
	ID3D11Device				*m_pDevice;
	ID3D11Texture2D				*m_pTexture;
	ID3D11ShaderResourceView    *m_pTextureShaderResourceView;
	ID3D11SamplerState			*m_pTextureSamplerState;
	ID3D11VertexShader			*m_pVertexShader;
	ID3D11PixelShader			*m_pPixelShader;
	ID3D11InputLayout			*m_pInputLayout;
	ID3D11BlendState			*m_pBlendState;

	ID3D11Buffer				*m_pVertexBuffer;
	ID3D11Buffer				*m_pConstantBuffer;

	DWORD						m_dwFontFlags;
	DWORD						m_dwFontHeight;

	DirectX::XMMATRIX			m_matProj;

	DWORD						m_dwTexWidth;
	DWORD						m_dwTexHeight;
	TCHAR						*m_strFontName;
	BOOL						m_bInitialized;
	FLOAT						m_fTexCoords[128 - 32][4];
};

//	InitInstance
DirectX11FontImpl	*DirectX11FontImpl::m_pInstance = NULL;


/**
* @brief	�R���X�g���N�^
*/
DirectX11FontImpl::DirectX11FontImpl()
{
	//  �������񂷂ׂ� NULL �ŏ�����
	m_pDeviceContext = NULL;
	m_pDevice = NULL;
	m_pTexture = NULL;
	m_pTextureShaderResourceView = NULL;
	m_pTextureSamplerState = NULL;
	m_pVertexShader = NULL;
	m_pPixelShader = NULL;
	m_pInputLayout = NULL;
	m_pBlendState = NULL;
	m_pVertexBuffer = NULL;
	m_pConstantBuffer = NULL;

	//  �f�o�b�O�p�f�t�H���g
	m_strFontName = (TCHAR*)_T("�l�r �S�V�b�N");
	m_dwFontHeight = 20;
	m_dwFontFlags = 0;

	m_bInitialized = false;
}


/**
* @brief	�f�X�g���N�^
*/
DirectX11FontImpl::~DirectX11FontImpl()
{
	ReleaseDeviceObjects(NULL);
}


/**
* @brief	������
*/
void DirectX11FontImpl::Initialize()
{
	if (m_pInstance == NULL)
		m_pInstance = new DirectX11FontImpl();
}


/**
* @brief	�I������
*/
void DirectX11FontImpl::Finalize()
{
	SAFE_DELETE(m_pInstance);
}


/**
* @brief	�C���X�^���X�擾
* @return	DirectX11FontImplObjectInstance
*/
DirectX11FontImpl *DirectX11FontImpl::GetInstance()
{
	if (m_pInstance == NULL)
		DirectX11FontImpl::Initialize();

	return m_pInstance;
}


/**
* @brief	�f�o�C�X�̏����擾����
* @param	[in]	pContext	ID3D11DeviceContext
* @dateil	�V�F�[�_�[�̐������s��
*/
HRESULT DirectX11FontImpl::RestoreDeviceObjects(ID3D11DeviceContext * pContext)
{
	ID3DBlob    *pShader = NULL;
	HRESULT hr;

	m_bInitialized = false;

	pContext->AddRef();
	ReleaseDeviceObjects(pContext);

	m_pDeviceContext = pContext;
	m_pDeviceContext->GetDevice(&m_pDevice);


	//  ���_�V�F�[�_�̐���
	hr = CompileShaderFromMemory((BYTE*)shadercode, sizeof(shadercode), "VSFunc", "vs_4_0", &pShader);
	if (FAILED(hr)) {
		return    hr;
	}
	hr = m_pDevice->CreateVertexShader(pShader->GetBufferPointer(), pShader->GetBufferSize(), NULL, &m_pVertexShader);
	if (FAILED(hr)) {
		return    hr;
	}


	// ���̓��C�A�E�g�̒�`.
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

	// ���̓��C�A�E�g�𐶐�.
	hr = m_pDevice->CreateInputLayout(
		layout,
		numElements,
		pShader->GetBufferPointer(),
		pShader->GetBufferSize(),
		&m_pInputLayout
	);
	SAFE_RELEASE(pShader);
	if (FAILED(hr))
	{
		return hr;
	}

	// ���̓A�Z���u���ɓ��̓��C�A�E�g��ݒ�.
	m_pDeviceContext->IASetInputLayout(m_pInputLayout);

	//  �s�N�Z���V�F�[�_�̐���
	hr = CompileShaderFromMemory((BYTE*)shadercode, sizeof(shadercode), "PSFunc", "ps_4_0", &pShader);
	if (FAILED(hr))
		return hr;
	hr = m_pDevice->CreatePixelShader(pShader->GetBufferPointer(), pShader->GetBufferSize(), NULL, &m_pPixelShader);
	SAFE_RELEASE(pShader);
	if (FAILED(hr))
		return    hr;

	//	ANK �t�H���g�p�e�N�X�`���̐���
	hr = CreateFontTexture();

	if (FAILED(hr))
		return hr;

	// �V�F�[�_���\�[�X�r���[�𐶐�.
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
		D3D11_TEXTURE2D_DESC    texDesc;
		ZeroMemory(&srvd, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		m_pTexture->GetDesc(&texDesc);
		srvd.Format = texDesc.Format;
		srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvd.Texture2D.MipLevels = texDesc.MipLevels;
		srvd.Texture2D.MostDetailedMip = 0;
		hr = m_pDevice->CreateShaderResourceView(m_pTexture, &srvd, &m_pTextureShaderResourceView);
		if (FAILED(hr))
			return hr;
	}

	//  �T���v���\�̃Z�b�g�A�b�v
	{
		D3D11_SAMPLER_DESC samplerDesc;
		static const FLOAT zero[] = { 0.0f,0.0f,0.0f,0.0f };
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;         // �T���v�����O���Ɏg�p����t�B���^�B�����ł͈ٕ����t�B���^�[���g�p����B
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 �` 1 �͈̔͊O�ɂ��� u �e�N�X�`���[���W�̕`����@
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 �` 1 �͈̔͊O�ɂ��� v �e�N�X�`���[���W
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 �` 1 �͈̔͊O�ɂ��� w �e�N�X�`���[���W
		samplerDesc.MipLODBias = 0;                            // �v�Z���ꂽ�~�b�v�}�b�v ���x������̃o�C�A�X
		samplerDesc.MaxAnisotropy = 16;                        // �T���v�����O�Ɉٕ�����Ԃ��g�p���Ă���ꍇ�̌��E�l�B�L���Ȓl�� 1 �` 16 �B
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;  // ��r�I�v�V�����B
		memcpy((void*)samplerDesc.BorderColor, (void*)zero, sizeof(zero));
		samplerDesc.MinLOD = 0;                                // �A�N�Z�X�\�ȃ~�b�v�}�b�v�̉����l
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;                // �A�N�Z�X�\�ȃ~�b�v�}�b�v�̏���l
															   // ID3D11Device::CreateSamplerState
		hr = m_pDevice->CreateSamplerState(&samplerDesc, &m_pTextureSamplerState);
		if (FAILED(hr))
			return    hr;

	}
	// ���_�o�b�t�@�̐ݒ�.
	{
		// ���_�̒�`.
		AnkFontVertex vertices[] = {
		{ DirectX::XMFLOAT3(-0.5f,  0.5f, 0.0f),  DirectX::XMFLOAT2(0.0f,  0.0f) },
		{ DirectX::XMFLOAT3(0.5f,  0.5f, 0.0f),  DirectX::XMFLOAT2(1.0f,  0.0f) },
		{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f),  DirectX::XMFLOAT2(0.0f,  1.0f) },
		{ DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f),  DirectX::XMFLOAT2(1.0f,  1.0f) },
		};

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(AnkFontVertex) * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		// �T�u���\�[�X�̐ݒ�.
		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
		initData.pSysMem = vertices;

		// ���_�o�b�t�@�̐���.
		hr = m_pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
		if (FAILED(hr))
		{
			return    hr;
		}
	}
	// �萔�o�b�t�@�̐���.
	{
		// �萔�o�b�t�@�̐ݒ�.
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		bd.ByteWidth = sizeof(AnkFontConstantBuffer);
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;

		// �萔�o�b�t�@�𐶐�.
		hr = m_pDevice->CreateBuffer(&bd, NULL, &m_pConstantBuffer);
		if (FAILED(hr))
		{
			return    hr;
		}
	}
	{
		//  �A���t�@�����u�����h�X�e�[�g�̐���
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
			return    hr;
		}
	}
	// �s��̐ݒ�.
	{
		//  non scaled ortho matrix (2D)
		m_matProj = DirectX::XMMatrixIdentity();
		m_matProj.r[0].m128_f32[0] = 1.0f;
		m_matProj.r[1].m128_f32[1] = 1.0f;
		m_matProj.r[2].m128_f32[2] = 0.0f;
		m_matProj.r[2].m128_f32[3] = 1.0f;
		m_matProj.r[3].m128_f32[2] = 0.0f;
		m_matProj.r[3].m128_f32[3] = 0.0f;
	}
	m_bInitialized = true;
	return S_OK;
}


/**
* @brief	�f�o�C�X�̏I������
* @param	[in]	pContext	ID3D11DeviceContext
*/
HRESULT DirectX11FontImpl::ReleaseDeviceObjects(ID3D11DeviceContext * pContext)
{
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pTextureShaderResourceView);
	SAFE_RELEASE(m_pTextureSamplerState);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pBlendState);
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pConstantBuffer);
	return    S_OK;
}


/**
* @brief	�t�H���g�e�N�X�`���̕\��
* @param	[in]	pContext	ID3D11DeviceContext
*/
void DirectX11FontImpl::Render(ID3D11DeviceContext * pContext)
{
	if (!m_bInitialized)
		return;

	//�@�V�F�[�_��ݒ肵�ĕ`��.
	pContext->VSSetShader(m_pVertexShader, NULL, 0);
	pContext->GSSetShader(NULL, NULL, 0);
	pContext->PSSetShader(m_pPixelShader, NULL, 0);

	// �萔�o�b�t�@�̐ݒ�.
	AnkFontConstantBuffer cb;
	cb.matProj = m_matProj;
	//memcpy((void*)&cb.matProj,(void*)&m_matProj,sizeof(DirectX::XMMATRIX));
	cb.color = DirectX::XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	// �T�u���\�[�X���X�V.
	pContext->UpdateSubresource(m_pConstantBuffer, 0, NULL, &cb, 0, 0);

	// ���_�V�F�[�_�ɒ萔�o�b�t�@��ݒ�.
	pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	// ���̓A�Z���u���ɒ��_�o�b�t�@��ݒ�.
	UINT stride = sizeof(AnkFontVertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// �v���~�e�B�u�̎�ނ�ݒ�.
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3D11ShaderResourceView* ppShaderResourceViews[] = { m_pTextureShaderResourceView, 0 };
	ID3D11SamplerState    *ppSamplerStates[] = { m_pTextureSamplerState, 0 };
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
	pContext->PSSetSamplers(0, 1, ppSamplerStates);

	pContext->Draw(4, 0);    //  Draw 4 vertices.

	ppSamplerStates[0] = NULL;
	ppShaderResourceViews[0] = NULL;
	pContext->PSSetSamplers(0, 1, ppSamplerStates);
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
}


/**
* @brief	ANK �t�H���g�p�e�N�X�`���̐���
*/
HRESULT DirectX11FontImpl::CreateFontTexture()
{
	HDC        hDC = NULL;
	HBITMAP hbmBitmap = NULL;
	HFONT hFont = NULL;
	hDC = CreateCompatibleDC(NULL);
	HRESULT hr;
	INT    nHeight;

	DWORD dwBold = (m_dwFontFlags&D3DFONT_BOLD) ? FW_BOLD : FW_NORMAL;
	DWORD dwItalic = (m_dwFontFlags&D3DFONT_ITALIC) ? TRUE : FALSE;


	if (m_dwFontFlags & D3DFONT_SIZE_IN_PIXELS) {
		nHeight = (INT)(m_dwFontHeight);
	}
	else {
		__int64    tmp64 = m_dwFontHeight;
		tmp64 *= (INT)(GetDeviceCaps(hDC, LOGPIXELSY));
		nHeight = (INT)(tmp64 / 72);    //  �t�H���g�� * dpi * testScale / 72
	}

	// Large fonts need larger textures
	if (nHeight > 64)
		m_dwTexWidth = m_dwTexHeight = 1024;
	else if (nHeight > 32)
		m_dwTexWidth = m_dwTexHeight = 512;
	else
		m_dwTexWidth = m_dwTexHeight = 256;

	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(D3D11_TEXTURE2D_DESC));
	td.Width = m_dwTexWidth;
	td.Height = m_dwTexHeight;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.SampleDesc.Count = 1;    //  MULTI SAMPLE COUNT
	td.SampleDesc.Quality = 0;    //  MULtI SAMPLE QUALITY
	td.Usage = D3D11_USAGE_DYNAMIC;    //  Make it writeable
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	td.MiscFlags = 0;

	// 2D�e�N�X�`���̐���
	hr = m_pDevice->CreateTexture2D(&td, NULL, &this->m_pTexture);
	if (FAILED(hr))
		return hr;


	DWORD*      pBitmapBits;
	BITMAPINFO bmi;
	ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = (int)m_dwTexWidth;
	bmi.bmiHeader.biHeight = -(int)m_dwTexHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32;

	// �r�b�g�}�b�v�̍쐬
	hbmBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS,
		(VOID**)&pBitmapBits, NULL, 0);
	SetMapMode(hDC, MM_TEXT);

	//  ANTIALIASED_QUALITY���w�肵�ăt�H���g�𐶐�����B
	//  �A���`�G�C���A�X�����������t�H���g�𓾂悤�Ƃ��邪�A
	//  �A���`�G�C���A�X��������Ƃ͕ۏ؂���Ȃ��B
	hFont = CreateFont(-nHeight, 0, 0, 0, dwBold, dwItalic,
		FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		VARIABLE_PITCH, m_strFontName);
	if (NULL == hFont) {
		hr = E_FAIL;
	}
	else {
		SelectObject(hDC, hbmBitmap);
		SelectObject(hDC, hFont);

		// �e�L�X�g�v���p�e�B�[�̐ݒ�
		SetTextColor(hDC, RGB(255, 255, 255));
		SetBkColor(hDC, 0x00000000);
		SetTextAlign(hDC, TA_TOP);

		//  texture coordinates��ۑ����Ȃ���A�o�͉\�ȕ������r�b�g�}�b�v��
		//  �o�͂��Ă䂭�B
		DWORD x = 0;
		DWORD y = 0;
		TCHAR str[2] = _T("x");
		SIZE size;

		for (TCHAR c = 32; c<127; c++)
		{
			str[0] = c;
			GetTextExtentPoint32(hDC, str, 1, &size);

			if ((DWORD)(x + size.cx + 1) > m_dwTexWidth)
			{
				x = 0;
				y += size.cy + 1;
			}

			ExtTextOut(hDC, x + 0, y + 0, ETO_OPAQUE, NULL, str, 1, NULL);

			m_fTexCoords[c - 32][0] = ((FLOAT)(x + 0)) / m_dwTexWidth;
			m_fTexCoords[c - 32][1] = ((FLOAT)(y + 0)) / m_dwTexHeight;
			m_fTexCoords[c - 32][2] = ((FLOAT)(x + 0 + size.cx)) / m_dwTexWidth;
			m_fTexCoords[c - 32][3] = ((FLOAT)(y + 0 + size.cy)) / m_dwTexHeight;

			x += size.cx + 1;
		}

		// Lock the surface and write the alpha values for the set pixels
		D3D11_MAPPED_SUBRESOURCE hMappedResource;
		hr = m_pDeviceContext->Map(
			m_pTexture,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&hMappedResource);
		if (SUCCEEDED(hr)) {
			// �����ŏ�������
			BYTE* pBits = (BYTE*)hMappedResource.pData;

			BYTE bAlpha; // 4-bit measure of pixel intensity

			for (y = 0; y < m_dwTexHeight; y++)
			{
				DWORD* pDst32 = (DWORD*)(pBits + (y*hMappedResource.RowPitch));
				for (x = 0; x < m_dwTexWidth; x++)
				{
					bAlpha = (BYTE)(pBitmapBits[m_dwTexWidth*y + x] & 0xff);
					if (bAlpha > 0) {
						*pDst32++ = 0x00ffffff | (bAlpha << 24);
					}
					else
					{
						*pDst32++ = 0x00000000;
					}
				}
			}
			m_pDeviceContext->Unmap(m_pTexture, 0);
		}
	}

	if (hbmBitmap)
		DeleteObject(hbmBitmap);
	if (hFont)
		DeleteObject(hFont);
	if (hDC)
		DeleteDC(hDC);

	return S_OK;
}


/**
* @brief	������̕\��
* @param	[in]	pContext	ID3D11DeviceContext
* @param	[in]	pString		������̃|�C���^
* @param	[in]	color		�J���[�R�[�h(R:G:B:A)
* @param	[in]	x			�\�����镶�����X���W
* @param	[in]	y			�\�����镶�����Y���W
*/
void DirectX11FontImpl::DrawAnkText(ID3D11DeviceContext * pContext, const TCHAR * pString, DirectX::XMFLOAT4 color, FLOAT x, FLOAT y)
{
	FLOAT tmpx = x;
	FLOAT tmpy = y;

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pContext->OMSetBlendState(m_pBlendState, blendFactor, 0xffffffff);

	const TCHAR *p = pString;
	while (*p != (TCHAR)0) {
		DrawChar(pContext, (char)(*p & 0x7f), color, &tmpx, &tmpy);
		p++;
	}

	pContext->OMSetBlendState(NULL, blendFactor, 0xffffffff);
}


/**
* @brief	�����̕\��
* @param	[in]		pContext	ID3D11DeviceContext
* @param	[in]		ankCode		������ ANSI code
* @param	[in]		color		�J���[�R�[�h(R:G:B:A)
* @param	[in-out]	px			�\�����镶�����X���W
* @param	[in-out]	py			�\�����镶�����Y���W
*/
void DirectX11FontImpl::DrawChar(ID3D11DeviceContext * pContext, const char ankCode, DirectX::XMFLOAT4 color, FLOAT * px, FLOAT * py)
{
	if (!m_bInitialized)
		return;

	if (m_pDeviceContext != pContext)
		return;

	char c = ankCode;
	if (c < ' ')
		return;
	FLOAT fTexWidth = (FLOAT)m_dwTexWidth;
	FLOAT fTexHeight = (FLOAT)m_dwTexHeight;
	FLOAT fTexWidthInv = 1.0f / fTexWidth;
	FLOAT fTexHeightInv = 1.0f / fTexHeight;

	FLOAT tx1 = m_fTexCoords[c - 32][0];
	FLOAT ty1 = m_fTexCoords[c - 32][1];
	FLOAT tx2 = m_fTexCoords[c - 32][2];
	FLOAT ty2 = m_fTexCoords[c - 32][3];
	FLOAT w = (tx2 - tx1) * fTexWidth;
	FLOAT h = (ty2 - ty1) * fTexHeight;


	D3D11_VIEWPORT  vp;
	UINT            uiNumViewport = 1;
	m_pDeviceContext->RSGetViewports(&uiNumViewport, &vp);
	if (uiNumViewport != 1)
		return;

	//  transform 2D -> clipping space
	FLOAT l = *px;
	FLOAT t = *py;
	FLOAT r = l + w;
	FLOAT b = t + h;
	FLOAT z = 1.0f;
	l /= vp.Width*0.5f;
	t /= vp.Height*0.5f;
	r /= vp.Width*0.5f;
	b /= vp.Height*0.5f;
	l -= 1.0f;
	r -= 1.0f;
	t = 1.0f - t;
	b = 1.0f - b;

	*px += w;

	// ���_�o�b�t�@�̐ݒ�.
	{
		// ���_�̒�`.
		AnkFontVertex vertices[] = {
		{ DirectX::XMFLOAT3(l, t, z),  DirectX::XMFLOAT2(tx1,  ty1) },
		{ DirectX::XMFLOAT3(r, t, z),  DirectX::XMFLOAT2(tx2,  ty1) },
		{ DirectX::XMFLOAT3(l, b, z),  DirectX::XMFLOAT2(tx1,  ty2) },
		{ DirectX::XMFLOAT3(r, b, z),  DirectX::XMFLOAT2(tx2,  ty2) },
		};

		pContext->UpdateSubresource(m_pVertexBuffer, 0, NULL, vertices, 0, 0);

	}

	//�@�V�F�[�_��ݒ�
	pContext->VSSetShader(m_pVertexShader, NULL, 0);
	pContext->GSSetShader(NULL, NULL, 0);
	pContext->PSSetShader(m_pPixelShader, NULL, 0);

	// �萔�o�b�t�@�̐ݒ�.
	AnkFontConstantBuffer cb;
	cb.matProj = m_matProj;
	//memcpy((void*)&cb.matProj,(void*)&m_matProj,sizeof(DirectX::XMMATRIX));
	cb.color = DirectX::XMFLOAT4(color.x, color.y, color.z, color.w);
	DirectX::XMMatrixTranspose(cb.matProj);
	// �T�u���\�[�X���X�V.
	pContext->UpdateSubresource(m_pConstantBuffer, 0, NULL, &cb, 0, 0);

	// ���_�V�F�[�_�ɒ萔�o�b�t�@��ݒ�.
	pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);

	// ���̓A�Z���u���ɒ��_�o�b�t�@��ݒ�.
	UINT stride = sizeof(AnkFontVertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// ���̓A�Z���u���ɓ��̓��C�A�E�g��ݒ�.
	pContext->IASetInputLayout(m_pInputLayout);

	// �v���~�e�B�u�̎�ނ�ݒ�.
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3D11ShaderResourceView* ppShaderResourceViews[] = { m_pTextureShaderResourceView, 0 };
	ID3D11SamplerState    *ppSamplerStates[] = { m_pTextureSamplerState, 0 };
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);
	pContext->PSSetSamplers(0, 1, ppSamplerStates);

	pContext->Draw(4, 0);    //  Draw 4 vertices.

	ppSamplerStates[0] = NULL;
	ppShaderResourceViews[0] = NULL;
	pContext->PSSetSamplers(0, 1, ppSamplerStates);
	pContext->PSSetShaderResources(0, 1, ppShaderResourceViews);

}


#define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude*)(UINT_PTR)1)

/**
* @brief	����������V�F�[�_�[���R���p�C������
* @param	[in]	code			�V�F�[�_�[�R�[�h
* @param	[in]	dwSize			�V�F�[�_�[�̃T�C�Y
* @param	[in]	szEntryPoint	�G���g���[�|�C���g
* @param	[in]	szShaderModel	�V�F�[�_�[�̃o�[�W����
* @param	[out]	ppBlobOut		ID3D11Blob
*/
HRESULT DirectX11FontImpl::CompileShaderFromMemory(BYTE * code, DWORD dwSize, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob ** ppBlobOut)
{
	// ���^�[���R�[�h��������.
	HRESULT hr = S_OK;

	// �R���p�C���t���O.
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(DEBUG) || defined(_DEBUG)
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif//defiend(DEBUG) || defined(_DEBUG)

#if defined(NDEBUG) || defined(_NDEBUG)
	//dwShaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif//defined(NDEBUG) || defined(_NDEBUG)

	ID3DBlob* pErrorBlob = NULL;

	// �V�F�[�_���R���p�C��.
	hr = D3DCompile(code, dwSize, NULL, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

	// �G���[�`�F�b�N.
	if (FAILED(hr))
	{
		// �G���[���b�Z�[�W���o��.
		if (pErrorBlob != NULL)
		{
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		}
	}

	// �������.
	if (pErrorBlob)
	{
		pErrorBlob->Release();
		pErrorBlob = NULL;
	}

	// ���^�[���R�[�h��ԋp.
	return hr;
}


/*************************************
*		class:CDirectX11Font		 *
*************************************/

/**
* @brief	������
*/
void CDirectX11Font::Initialize(ID3D11DeviceContext *pContext)
{
	DirectX11FontImpl *pObj = DirectX11FontImpl::GetInstance();

	//	�t�H���g�̏�����
	pObj->RestoreDeviceObjects(pContext);
}


/**
* @brief	�I������
*/
void CDirectX11Font::Finalize()
{
	DirectX11FontImpl::Finalize();
}


/**
* @brief	������̕\��
* @param	[in]	pContext	ID3D11DeviceContext
* @param	[in]	pString		������̃|�C���^
* @param	[in]	color		�J���[�R�[�h(R:G:B:A)
* @param	[in]	x			�\�����镶�����X���W
* @param	[in]	y			�\�����镶�����Y���W
*/
void CDirectX11Font::DrawAnkText(ID3D11DeviceContext * pContext, const TCHAR * pString, DirectX::XMFLOAT4 color, FLOAT x, FLOAT y)
{
	DirectX11FontImpl *pObj = DirectX11FontImpl::GetInstance();

	//	������̕\��
	pObj->DrawAnkText(pContext, pString, color, x, y);
}
