#pragma once
#include "IGameScene.h"


//!	StagePhase
enum StagePhase : short {
	STAGEPHASE_INIT = 0x00,
	STAGEPHASE_FADEIN = 0x01,
	STAGEPHASE_RUN = 0x02,
	STAGEPHASE_FADEOUT = 0x03,
	STAGEPHASE_DONE = 0x04,
};

class CPrimitive2D;
class CDungeon;
class CRenderableTexture2D;

/**
* @class	CStage
* @brief	ステージシーンクラス
*/
class CStage :
	public IGameScene
{
public:
	CStage();
	virtual ~CStage();
	virtual	GameSceneResultCode	Update()	override;
	virtual	void	Render(ID3D11DeviceContext *pContext)	override;
private:
	StagePhase	m_ePhase;		//!	StagePhase
	static	CDungeon	*m_pDungeon_;	//!	DungeonObject
	static	CRenderableTexture2D	*m_pRenderTargets_;
	CPrimitive2D	*m_pPrimitive2D;
	
	float	m_fAngle;
};

