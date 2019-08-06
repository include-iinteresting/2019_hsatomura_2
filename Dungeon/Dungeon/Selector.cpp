#include "stdafx.h"
#include "Selector.h"

//	include GameScene's
#include "Stage.h"

#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if(o) { (o)->Release(); o = NULL; };
#define SAFE_DELETE(o) if(o) { delete(o); o = NULL; };
#define SAFE_DELETE_ARRAY(o) if(o) { delete[](o); o = NULL; };

/**
* @brief	�R���X�g���N�^
*/
CSelector::CSelector()
{
	m_pScene = NULL;
	m_ePhase = GamePhase::GAMEPHASE_INIT;
}


/**
* @brief	�f�X�g���N�^
*/
CSelector::~CSelector()
{
	SAFE_DELETE(m_pScene);
}


/**
* @brief	�X�V
* @return	bool	false�Ȃ�Q�[���I��
*/
bool CSelector::Update()
{
	switch (m_ePhase) {
	case GAMEPHASE_INIT:
		m_ePhase = GAMEPHASE_TITLE;
	case GAMEPHASE_TITLE:

		SAFE_DELETE(m_pScene);
		m_pScene = new CStage();
		m_ePhase = GAMEPHASE_STAGE;
		break;
		
	case GAMEPHASE_STAGE:
		
		if (m_pScene)
			m_pScene->Update();

		break;

	case GAMEPHASE_END:
		break;

	case GAMEPHASE_DONE:
		return false;
		break;
	}

	return true;
}


/**
* @brief	�`��
* @param	[in]	pConrtext	ID3D11DeviceContext
*/
void CSelector::Render(ID3D11DeviceContext * pContext)
{
	if (m_pScene)
		m_pScene->Render(pContext);
}
