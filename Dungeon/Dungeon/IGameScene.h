#pragma once


#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if(o) { (o)->Relese(); o = NULL; };
#define SAFE_DELETE(o) if(o) { delete(o); o = NULL; };
#define SAFE_DELETE_ARRAY(o) if(o) { delete[](o); o = NULL; };

//!	ゲームシーンの状態
enum GameSceneResultCode : short {
	GAMESCENE_DEFAULT = 0x00,
	GAMESCENE_END_OK = 0x01,
	GAMESCENE_GO_BACK = 0x02
};


struct ID3D11DeviceContext;

/**
* @class	IGameScene
* @brief	ゲームシーンクラスのインターフェース
*/
class IGameScene
{
public:
	virtual ~IGameScene() = 0;
	virtual	GameSceneResultCode	Update() = 0;
	virtual	void	Render(ID3D11DeviceContext *pContext) = 0;
};

