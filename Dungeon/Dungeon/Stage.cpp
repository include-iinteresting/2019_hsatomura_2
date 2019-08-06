#include "stdafx.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include "Stage.h"
#include "Dungeon.h"
#include "DirectX11.h"
#include "DirectX11Font.h"
#include "Primitive2D.h"
#include "RenderableTexture2D.h"

/**
* @brief	constructor
*/
CStage::CStage()
{
	m_ePhase = StagePhase::STAGEPHASE_INIT;
	DWORD width, height;
	CDirectX11::GetBackBufferSize(&width, &height);

	ID3D11DeviceContext *pContext = CDirectX11::GetDeviceContext();

	m_pDungeon_ = new CDungeon(width, height, 480, 480, 6, 6);
	m_pDungeon_->RestoreDeviceObjects(pContext);

	m_pRenderTargets_ = new CRenderableTexture2D();
	m_pRenderTargets_->RestoreDeviceObjects(pContext);

	m_pPrimitive2D = new CPrimitive2D();
	m_pPrimitive2D->RestoreDeviceObjects(pContext);

	m_fAngle = 0.0f;
}


/**
* @brief	pure virtual destructor
*/
CStage::~CStage()
{
	SAFE_DELETE(m_pDungeon_);
	SAFE_DELETE(m_pRenderTargets_);
	SAFE_DELETE(m_pPrimitive2D);
}


/**
* @brief	更新
*@return	GameSceneResultCode		ゲームシーンの状態
*/
GameSceneResultCode CStage::Update()
{
	switch (m_ePhase) {
	case STAGEPHASE_INIT:
		m_ePhase = STAGEPHASE_RUN;
		
		break;

	case STAGEPHASE_FADEIN:

		break;
		
	case STAGEPHASE_RUN:
		if (m_pDungeon_)
			m_pDungeon_->Update();

		m_fAngle += 0.005f;
		
		
		break;

	case STAGEPHASE_FADEOUT:
		break;
		
	case STAGEPHASE_DONE:
		return	GameSceneResultCode::GAMESCENE_END_OK;
		break;
	}

	return GameSceneResultCode::GAMESCENE_DEFAULT;
}


/**
* @brief	描画
* @param	[in]	pContext	ID3D11DeviceContext
*/
void CStage::Render(ID3D11DeviceContext * pContext)
{

	DirectX::XMMATRIX matLight;
	DirectX::XMMATRIX lightProjection = DirectX::XMMatrixOrthographicLH(12.0f, 12.0f, 0.1f, 200.0f);
	DirectX::XMMATRIX lightView;

	DirectX::XMVECTOR   litPos = DirectX::XMVectorSet(5.0f * cosf(m_fAngle), 5.0f, 5.0f * sinf(m_fAngle), 0);
	DirectX::XMVECTOR   litAt = DirectX::XMVectorSet(0, 0, 0, 0);
	DirectX::XMVECTOR   litUp = DirectX::XMVectorSet(0, 1, 0, 0);
	lightView = DirectX::XMMatrixLookAtLH(litPos, litAt, litUp);
	matLight = lightView * lightProjection;

	
	m_pDungeon_->Render(pContext,m_pPrimitive2D,matLight,m_pRenderTargets_->GetShaderResourceView());


	if (m_ePhase == STAGEPHASE_INIT) {
		//  フォントを描画
		float x = 380.0f;
		float y = 240.0f;
		TCHAR tmp[32];
		_stprintf_s(tmp, _T("START"));
		CDirectX11Font::DrawAnkText(pContext, tmp, DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.75f), x, y);

	}
}


//	static fields
CDungeon *CStage::m_pDungeon_ = NULL;
CRenderableTexture2D *CStage::m_pRenderTargets_ = NULL;
