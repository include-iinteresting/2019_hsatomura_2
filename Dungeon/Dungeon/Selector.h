#pragma once


//!	GamePhase
enum GamePhase : short {
	GAMEPHASE_INIT = 0x00,
	GAMEPHASE_TITLE = 0x01,
	GAMEPHASE_STAGE = 0x02,
	GAMEPHASE_END = 0x03,
	GAMEPHASE_DONE = 0x04
};


class IGameScene;
struct ID3D11DeviceContext;

/**
* @class	CSelector
* @brief	シーン管理クラス
*/
class CSelector
{
public:
	CSelector();
	virtual ~CSelector();
	bool	Update();
	void	Render(ID3D11DeviceContext *pContext);
private:
	GamePhase	m_ePhase;	//!	GamePhase
	IGameScene	*m_pScene;	//!	GameScene
};

