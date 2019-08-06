/**
*  @file  Primitive2D.cpp
*  @brief 2D�v���~�e�B�u��`�悷��N���X CPrimitive2D �̎���
*  @date 11.May.2019
*/
#include "stdafx.h"
#include <stdio.h>
#include <D3D11.h>
#include <directxmath.h>
#include <D3Dcompiler.h>
#include "Primitive2D.h"
#include "TextureLoader.h"

//  �I�u�W�F�N�g����p�}�N��
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#undef SAFE_RELEASE
#define SAFE_DELETE(o)       if (o){  delete (o);  o = NULL;    }
#define SAFE_DELETE_ARRAY(o) if (o){  delete [] (o);  o = NULL; }
#define SAFE_RELEASE(o)      if (o){  (o)->Release(); o = NULL; }

#define SHADER_FILENAME L"res\\Textured2d.fx"

//  �ǂݍ��ރe�N�X�`���̃t�@�C�����iNUM_TEXTURES�Ɛ�����v���Ȃ���΂Ȃ�Ȃ��j
static const TCHAR  *pFilename[] = {
	_T("res\\bricks.png")
};

//  typedef
/**
*  @brief ���_�f�[�^�\����
*/
typedef struct {
	DirectX::XMFLOAT3 position;     //  �ʒu���W
	DirectX::XMFLOAT2 texture;      //  �e�N�X�`�����W
}   Simple2DVertex;

/**
*  @brief �萔�o�b�t�@�p�\���́i���_�V�F�[�_�Ŏg�p�j
*/
typedef __declspec(align(16)) struct {
	DirectX::XMMATRIX   matProj;
	DirectX::XMFLOAT4   color;
}   Simple2DConstantBuffer;

/**
*  @brief �萔�o�b�t�@�p�\���́i�s�N�Z���V�F�[�_�Ŏg�p�j
*/
typedef __declspec(align(16)) struct {
	DirectX::XMFLOAT4   PSColor;
}   Simple2DPixelShaderConstantBuffer;

//  �v���g�^�C�v
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
*  @brief �f�o�C�X�ˑ��I�u�W�F�N�g�̍ēǂݍ��݂��s��
*  @param [in] pContext �L���ȃf�o�C�X�R���e�L�X�g
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

	// �R���p�C���t���O.
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif
#if defined(NDEBUG) || defined(_NDEBUG)
	dwShaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	ID3DBlob    *pCode = NULL, *pErrorMsgs = NULL;

	//  ���_�V�F�[�_�̐���
	result = D3DCompileFromFile(SHADER_FILENAME, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSFunc", "vs_4_0", dwShaderFlags, 0, &pCode, &pErrorMsgs);
	// �G���[�`�F�b�N.
	if (FAILED(result)) {
		// �G���[���b�Z�[�W���o��.
		if (pErrorMsgs != NULL) {
			OutputDebugStringA((char*)pErrorMsgs->GetBufferPointer());
		}
	}
	else {
		result = pDevice->CreateVertexShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), NULL, &m_pVertexShader);

		//!  ���_���̓��C�A�E�g�̐����i�����_�����O�p�C�v���C������̌���j
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

			//  ���̓��C�A�E�g�𐶐�.
			//  �����ŃR���p�C���ς݃V�F�[�_�R�[�h����
			//  ���_�t�H�[�}�b�g�ƈ�v���邩�ƍ����Ă��邽�߁A�V�F�[�_�R�[�h��n���K�v������B
			//  �R���p�C���ς݃V�F�[�_�R�[�h�Ƃ����Ă��l�C�e�B�u�o�C�i���ł͂Ȃ��̂�
			//  ���̏ƍ��͂ł���Ώȗ�����������
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

	//  �s�N�Z���V�F�[�_�̐���
	if (SUCCEEDED(result)) {
		result = D3DCompileFromFile(SHADER_FILENAME, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"PSFunc", "ps_4_0", dwShaderFlags, 0, &pCode, &pErrorMsgs);
		if (FAILED(result)) {
			// �G���[���b�Z�[�W���o��.
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

	//  �萔�o�b�t�@�̐���
	if (SUCCEEDED(result)) {
		D3D11_BUFFER_DESC bd = { 0 };
		bd.ByteWidth = sizeof(Simple2DConstantBuffer);
		bd.Usage = D3D11_USAGE_DEFAULT; //  CPU�͏������݂̂݉�
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		// �萔�o�b�t�@�𐶐�.
		result = pDevice->CreateBuffer(&bd, NULL, &m_pConstantBuffer);
	}

	//  �萔�o�b�t�@�̐���
	if (SUCCEEDED(result)) {
		D3D11_BUFFER_DESC bd = { 0 };
		bd.ByteWidth = sizeof(Simple2DPixelShaderConstantBuffer);
		bd.Usage = D3D11_USAGE_DEFAULT; //  CPU�͏������݂̂݉�
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		// �萔�o�b�t�@�𐶐�.
		result = pDevice->CreateBuffer(&bd, NULL, &m_pPixelShaderConstantBuffer);
	}

	//  ���_�o�b�t�@�̐���
	if (SUCCEEDED(result)) {
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		bd.Usage = D3D11_USAGE_DEFAULT; //  CPU�͏������݂̂݉�
		bd.ByteWidth = sizeof(Simple2DVertex) * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		// ���_�o�b�t�@�̐���.
		result = pDevice->CreateBuffer(&bd, NULL, &m_pVertexBuffer);
	}
	if (SUCCEEDED(result)) {
		for (int i = 0; i < min(_countof(pFilename), NUM_TEXTURES); ++i) {
			result = CTextureLoader::CreateTextureFromFile(pContext, pFilename[i], &m_pTextures[i].pTexture,
				&m_pTextures[i].dwSrcWidth, &m_pTextures[i].dwSrcHeight);
			// �V�F�[�_���\�[�X�r���[�𐶐�.
			if (SUCCEEDED(result)) {
				D3D11_SHADER_RESOURCE_VIEW_DESC srvd;    //!< 
				D3D11_TEXTURE2D_DESC            texDesc; //!< �e�N�X�`���f�X�N���v�^
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
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;         // �T���v�����O���Ɏg�p����t�B���^�B�����ł͈ٕ����t�B���^�[���g�p����B
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 �` 1 �͈̔͊O�ɂ��� u �e�N�X�`���[���W�̕`����@
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 �` 1 �͈̔͊O�ɂ��� v �e�N�X�`���[���W
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // 0 �` 1 �͈̔͊O�ɂ��� w �e�N�X�`���[���W
		samplerDesc.MipLODBias = 0;                            // �v�Z���ꂽ�~�b�v�}�b�v ���x������̃o�C�A�X
		samplerDesc.MaxAnisotropy = 16;                        // �T���v�����O�Ɉٕ�����Ԃ��g�p���Ă���ꍇ�̌��E�l�B�L���Ȓl�� 1 �` 16 �B
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;  // ��r�I�v�V�����B
		memcpy((void*)&samplerDesc.BorderColor, (void*)&zero4, 4 * sizeof(FLOAT));
		samplerDesc.MinLOD = 0;                                // �A�N�Z�X�\�ȃ~�b�v�}�b�v�̉����l
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;                // �A�N�Z�X�\�ȃ~�b�v�}�b�v�̏���l
		result = pDevice->CreateSamplerState(&samplerDesc, &m_pSamplerState);
	}
	SAFE_RELEASE(pDevice);

	if (SUCCEEDED(result)) {
		m_bPrepared = TRUE;
	}
	return result;
}
/**
*  @brief �f�o�C�X�ˑ��I�u�W�F�N�g�̉�����s��
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
*  @brief �v���~�e�B�u�̕`��
*  @param [in]  pContext  �L����RenderTarget ��ݒ肳�ꂽ�f�o�C�X�R���e�L�X�g
*/
void CPrimitive2D::Render(ID3D11DeviceContext *pContext, float x, float y, float w, float h) {
	if (!m_bPrepared)
		return;

	//  �萔�o�b�t�@�̍X�V�i�S���������j
	Simple2DConstantBuffer  cb;
	cb.matProj = m_matProjection;
	cb.color = DirectX::XMFLOAT4(1, 1, 1, 1);
	pContext->UpdateSubresource(m_pConstantBuffer, 0, NULL, &cb, 0, 0);

	Simple2DPixelShaderConstantBuffer  pcb;
	pcb.PSColor = DirectX::XMFLOAT4(1, 1, 1, 0);
	pContext->UpdateSubresource(m_pPixelShaderConstantBuffer, 0, NULL, &pcb, 0, 0);

	//  ���_�o�b�t�@�̍X�V�i�S���������j
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

	//�@�V�F�[�_��ݒ�
	pContext->VSSetShader(m_pVertexShader, NULL, 0);
	pContext->GSSetShader(NULL, NULL, 0);
	pContext->PSSetShader(m_pPixelShader, NULL, 0);

	//  ���_�V�F�[�_�ɒ萔�o�b�t�@��ݒ�
	pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	pContext->PSSetConstantBuffers(0, 1, &m_pPixelShaderConstantBuffer);

	// ���̓A�Z���u���ɒ��_�o�b�t�@��ݒ�.
	UINT stride = sizeof(Simple2DVertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// �v���~�e�B�u�̎�ނ�ݒ�.
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
	// �v���~�e�B�u�`��.
	pContext->Draw(4, 0);

}


/**
*  @brief �f�o�C�X�R���e�L�X�g����o�b�N�o�b�t�@�̃T�C�Y���m�F����
*  @param [in] pContext  �f�o�C�X�R���e�L�X�g
*  @param [out] width    �o�b�N�o�b�t�@�i��ʁj�̕�
*  @param [out] height   �o�b�N�o�b�t�@�i��ʁj�̍���
*  @return  S_OK : ���� / ����ȊO : ���s
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