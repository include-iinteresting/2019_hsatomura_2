#include "stdafx.h"
#include <d3d11.h>
#include <directxmath.h>
#include "Primitive3DShadow.h"
#include "TextureLoader.h"
#include "Floor.h"

#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if (o){ (o)->Release(); o = NULL; }
#define SAFE_DELETE(o)  if (o){ delete (o); o = NULL; }
#define SAFE_DELETE_ARRAY(o) if (o){ delete [] (o); o = NULL; }

#define TEXTURE_FILENAME	_T("res\\dungeon\\bricks.dds")

#define NUM_VERTICES	4

CFloor::CFloor()
{
	m_pPrimitive3D = IPrimitive3DShadow::Create(NUM_VERTICES);
	m_pTexture2DSRV = NULL;
}


CFloor::~CFloor()
{
	ReleaseDeviceObjects();
	SAFE_DELETE(m_pPrimitive3D);
}

HRESULT CFloor::RestoreDeviceObjects(ID3D11DeviceContext *pContext) {
	DWORD sw, sh;
	ID3D11Texture2D *pTexture = NULL;
	if (SUCCEEDED(CTextureLoader::CreateTextureFromFile(pContext, TEXTURE_FILENAME, &pTexture, &sw, &sh))) {
		ID3D11Device *pDevice = NULL;
		D3D11_SHADER_RESOURCE_VIEW_DESC srvd;    //!< 
		D3D11_TEXTURE2D_DESC            texDesc; //!< テクスチャデスクリプタ
		pContext->GetDevice(&pDevice);
		ZeroMemory(&srvd, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		pTexture->GetDesc(&texDesc);
		srvd.Format = texDesc.Format;
		srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvd.Texture2D.MipLevels = texDesc.MipLevels;
		srvd.Texture2D.MostDetailedMip = 0;
		pDevice->CreateShaderResourceView(pTexture, &srvd, &m_pTexture2DSRV);
		SAFE_RELEASE(pDevice);
		SAFE_RELEASE(pTexture);
	}
	if (m_pPrimitive3D == NULL)
		return E_FAIL;
	return m_pPrimitive3D->RestoreDeviceObjects(pContext);
}

HRESULT CFloor::ReleaseDeviceObjects() {
	SAFE_RELEASE(m_pTexture2DSRV);
	if (m_pPrimitive3D == NULL)
		return E_FAIL;
	return m_pPrimitive3D->ReleaseDeviceObjects();
}

void CFloor::SetWorldMatrix(DirectX::XMMATRIX &&matWorld) {
	if (m_pPrimitive3D != NULL)
		m_pPrimitive3D->SetWorldMatrix(static_cast<DirectX::XMMATRIX&>(matWorld));
}

void CFloor::SetViewMatrix(DirectX::XMMATRIX &&matView) {
	if (m_pPrimitive3D != NULL)
		m_pPrimitive3D->SetViewMatrix(static_cast<DirectX::XMMATRIX&>(matView));
}

void CFloor::SetProjectionMatrix(DirectX::XMMATRIX &&matProjection) {
	if (m_pPrimitive3D != NULL)
		m_pPrimitive3D->SetProjectionMatrix(static_cast<DirectX::XMMATRIX&>(matProjection));
}

void CFloor::SetLightDirection(DirectX::XMFLOAT4 &&vecLightDirection) {
	if (m_pPrimitive3D != NULL) {
		m_pPrimitive3D->SetLightDirection(vecLightDirection);
	}
}
void CFloor::SetAmbientColor(DirectX::XMFLOAT4 &&vecAmbient) {
	if (m_pPrimitive3D != NULL) {
		m_pPrimitive3D->SetAmbientColor(vecAmbient);
	}
}

/**
*	@brief　描画メソッド
*   @param [in] pContext     描画コンテキスト
*   @param [in] pSRV         シェーダリソースビュー
*   @param [in] matCombined  シャドウ投影用マトリックス
*/
void CFloor::Render(ID3D11DeviceContext *pContext,
					ID3D11ShaderResourceView *pSRV, 
					DirectX::XMMATRIX &matCombined) {

	static DirectX::XMFLOAT3 vertices[NUM_VERTICES] = {

		DirectX::XMFLOAT3(-1, -1, 1),	//	0 back-left
		DirectX::XMFLOAT3(1, -1, 1),	//	1 back-right
		DirectX::XMFLOAT3(-1, -1,-1),	//	2 front-left
		DirectX::XMFLOAT3(1, -1,-1),	//	3 front-right

	};
	static DirectX::XMFLOAT3 normals[NUM_VERTICES] = {

		DirectX::XMFLOAT3(-1, 1, 1),	//	0 back-left
		DirectX::XMFLOAT3(1, 1, 1),		//	1 back-right
		DirectX::XMFLOAT3(-1, 1,-1),	//	2 front-left
		DirectX::XMFLOAT3(1, 1,-1),		//	3 front-right

	};
	static DirectX::XMFLOAT2 texCoords[NUM_VERTICES] = {

		DirectX::XMFLOAT2(0.0000f, 0.0000f),	//	wrap を使ったテクスチャ座標設定
		DirectX::XMFLOAT2(1.0000f, 0.0000f),	//	wrap を使ったテクスチャ座標設定
		DirectX::XMFLOAT2(0.0000f, 1.0000f),	//	wrap を使ったテクスチャ座標設定
		DirectX::XMFLOAT2(1.0000f, 1.0000f),	//	wrap を使ったテクスチャ座標設定

	};
	static DirectX::XMFLOAT2 texCoords2[NUM_VERTICES];
	static const DirectX::XMFLOAT4 white = DirectX::XMFLOAT4(1, 1, 1, 1);
	for (int i = 0; i < _countof(vertices); ++i) {
		FLOAT u, v, w;
		DirectX::XMVECTOR vec = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&vertices[i]),matCombined);
		DirectX::XMVectorGetByIndexPtr(&texCoords2[i].x, vec, 0);
		DirectX::XMVectorGetByIndexPtr(&texCoords2[i].y, vec, 1);
		DirectX::XMVectorGetByIndexPtr(&w, vec, 3);
		//w = 1.0f / w;
		//texCoords2[i].x *= w;
		//texCoords2[i].y *= w;
		//texCoords2[i].x = (texCoords2[i].x + 1.0f) * 0.5f;
		//texCoords2[i].y = (1.0f - texCoords2[i].y) * 0.5f;	//	uv のY 座標は上下逆
	}
	m_pPrimitive3D->Render(pContext, vertices, normals, texCoords, texCoords2,
						   NUM_VERTICES, white, m_pTexture2DSRV,matCombined,pSRV);
}

