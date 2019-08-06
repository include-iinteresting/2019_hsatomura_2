/**
*  @file   Dungeon.cpp
*  @brief  自動生成ダンジョンの実装クラス
*/
#include "stdafx.h"
#include <stdint.h>
#include <time.h>
#include <d3d11.h>
#include <directxmath.h>
#include "Dungeon.h"
#include "Primitive2D.h"
#include "PolygonFT4.h"
#include "TextureLoader.h"
#include "Floor.h"

///@ macros
#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if (o){ (o)->Release(); o = NULL; }
#define SAFE_DELETE(o)  if (o){ delete (o); o = NULL; }
#define SAFE_DELETE_ARRAY(o) if (o){ delete [] (o); o = NULL; }

/**
* @brief コンストラクタ
* @param [in] screenWidth 画面の横幅（ピクセル数）
* @paran [in] screenHeight 画面の高（ピクセル数）
* @param [in] displayWidth 迷路の表示幅（ピクセル数）
* @param [in] displayHeight 迷路の非表示高（ピクセル数）
* @param [in] columns 迷路の横方向のレーン数
* @param [in] rows 迷路の縦方向のレーン数
*/
CDungeon::CDungeon(SHORT screenWidth, SHORT screenHeight, SHORT displayWidth, SHORT displayHeight, SHORT columns, SHORT rows)
{
	m_fScreenWidth = screenWidth;
	m_fScreenHeight = screenHeight;
	m_fDisplayWidth = displayWidth;
	m_fDisplayHeight = displayHeight;
	m_sColumns = columns * 2 + 1;
	m_sRows = rows * 2 + 1;
	m_sPitch = m_sColumns + 4;
	m_pMap = new BYTE[m_sPitch * (m_sRows + 4)];
	m_pPolygon = IPolygonFT4::Create();
	GenerateDungeon();
	CheckInvisibleWall();
	m_pFloor = new CFloor();
	m_pStartSRV = NULL;
	m_pGoalSRV = NULL;
}


CDungeon::~CDungeon()
{
	ReleaseDeviceObjects();
	SAFE_DELETE(m_pPolygon);
	SAFE_DELETE(m_pMap);
}

typedef struct {
	uint8_t x;
	uint8_t y;
}   SITE;


/**
* @brief	カメラの設定
* @param	[in]	viewMatrix	設定するビュー行列（右辺値参照）
*/
void CDungeon::SetViewMatrix(DirectX::XMMATRIX &&matrix)
{
	m_matView = matrix;	//  Make a deep copy of the matrix (64 bytes).
}


/**
*   @brief  アニメーション
*/
void CDungeon::Update() {
	static const DirectX::XMVECTOR camUp = DirectX::XMVectorSet(0, 1.0f, 0, 0);
	//  static float angle = 0.0f;
	DirectX::XMVECTOR camPos;
	DirectX::XMVECTOR camTarget = DirectX::XMVectorSet(0, 0, 0, 0);
	float x, z, w, d;

	float angle;
	w = 2;
	d = 2;
	x = (FLOAT)m_sPosX;
	z = (FLOAT)m_sPosZ;
	angle = m_fAngle;

	if (m_fLerp > 0) {
		x = x * (1.0f - m_fLerp) + m_sPosXGoal * m_fLerp;
		z = z * (1.0f - m_fLerp) + m_sPosZGoal * m_fLerp;
		angle = angle * (1.0f - m_fLerp) + m_fAngleGoal * m_fLerp;
		m_fLerp += 1.0f / 60.0f;
		if (m_fLerp >= 1.0f) {
			m_fLerp = 0.0f;
			m_sPosX = m_sPosXGoal;
			m_sPosZ = m_sPosZGoal;
			m_fAngle = m_fAngleGoal;

			if (m_fAngle > DirectX::XM_PI * 2.0f)
				m_fAngle -= DirectX::XM_PI * 2.0f;

			if (m_fAngle < 0)
				m_fAngle += DirectX::XM_PI * 2.0f;

			//	誤差修正、0.5pi でスナップ
			float c = m_fAngle / DirectX::XM_PIDIV2;
			m_fAngle = DirectX::XM_PIDIV2 * (int)(c + 0.5f);
			m_fAngleGoal = m_fAngle;
		}
	}
	if (m_fLerp == 0) {
		if (GetAsyncKeyState(VK_UP)) {
			SHORT goalX = m_sPosX + (SHORT)roundf(cosf(m_fAngle));
			SHORT goalZ = m_sPosZ - (SHORT)roundf(sinf(m_fAngle));
			if (m_pMap[(goalZ + 2) * m_sPitch + goalX + 2] == 0) {
				m_sPosXGoal = goalX;
				m_sPosZGoal = goalZ;
				m_fLerp = 1.0f / 60.0f;
			}
		}
		else if (GetAsyncKeyState(VK_DOWN)) {
			SHORT goalX = m_sPosX - (SHORT)roundf(cosf(m_fAngle));
			SHORT goalZ = m_sPosZ + (SHORT)roundf(sinf(m_fAngle));
			if (m_pMap[(goalZ + 2) * m_sPitch + goalX + 2] == 0) {
				m_sPosXGoal = goalX;
				m_sPosZGoal = goalZ;
				m_fLerp = 1.0f / 60.0f;
			}
		}
		else if (GetAsyncKeyState(VK_LEFT)) {
			m_fAngleGoal = m_fAngle + DirectX::XM_PI * 0.5f;
			m_fLerp = 1.0f / 60.0f;
		}
		else if (GetAsyncKeyState(VK_RIGHT)) {
			m_fAngleGoal = m_fAngle - DirectX::XM_PI * 0.5f;
			m_fLerp = 1.0f / 60.0f;
		}
	}
	else {
		GetAsyncKeyState(VK_UP);
		GetAsyncKeyState(VK_LEFT);
		GetAsyncKeyState(VK_RIGHT);
		GetAsyncKeyState(VK_DOWN);
	}

	x *= w;
	z *= d;
	x -= w * 0.5f * m_sColumns;
	z = d * 0.5f * m_sRows + 5.0f - z;


	camPos = DirectX::XMVectorSet(x, 0.0f, z, 0);
	camTarget = DirectX::XMVectorSet(x + cosf(angle), 0.0f, z + sinf(angle), 0);


	//  右辺値参照で、SetViewMatrix を呼び出してみる、
	//  ただし、
	//  m_matView = DirectX::XMMatrixLookAtLH(camPos, camTarget, camUp); 
	//  と変わらない。
	SetViewMatrix(DirectX::XMMatrixLookAtLH(camPos, camTarget, camUp));
	m_pFloor->SetViewMatrix(DirectX::XMMatrixLookAtLH(camPos, camTarget, camUp));
	m_pPolygon->SetSpotLight(DirectX::XMFLOAT4(x, 0, z, 0), 0.33f, 0.6f, DirectX::XMFLOAT3(cosf(angle), 0, sinf(angle)), DirectX::XMFLOAT4(1.0f, 1.0f, 0.75f, 1.0f), 5, 8);
}


/**
*  @brief 描画処理
*  @param [in] pContext 有効なデバイスコンテキスト
*  @param [in] pPrimitive2D 四角形を描画するオブジェクト
*/
void CDungeon::Render(ID3D11DeviceContext *pContext, CPrimitive2D *pPrimitive, DirectX::XMMATRIX &matCombined, ID3D11ShaderResourceView *pSRV) {
	float x, z, w, d;

	static const DirectX::XMFLOAT3   vert[][4] = {
		{	//	上
			DirectX::XMFLOAT3(-1, 1, 1),
			DirectX::XMFLOAT3(1, 1, 1),
			DirectX::XMFLOAT3(-1, 1,-1),
			DirectX::XMFLOAT3(1, 1,-1)
		},
			{	//	Front
				DirectX::XMFLOAT3(-1, 1,-1),
				DirectX::XMFLOAT3(1, 1,-1),
				DirectX::XMFLOAT3(-1,-1,-1),
				DirectX::XMFLOAT3(1,-1,-1)
			},
		{	//	Left
			DirectX::XMFLOAT3(-1, 1, 1),
			DirectX::XMFLOAT3(-1, 1, -1),
			DirectX::XMFLOAT3(-1, -1, 1),
			DirectX::XMFLOAT3(-1, -1, -1)
		},
		{	//	Back
			DirectX::XMFLOAT3(1, 1, 1),
			DirectX::XMFLOAT3(-1, 1, 1),
			DirectX::XMFLOAT3(1,-1, 1),
			DirectX::XMFLOAT3(-1,-1, 1)
		},
		{	//	Right
			DirectX::XMFLOAT3(1, 1,-1),
			DirectX::XMFLOAT3(1, 1, 1),
			DirectX::XMFLOAT3(1,-1,-1),
			DirectX::XMFLOAT3(1,-1, 1)
		},
	};
	DirectX::XMFLOAT2   tex[] = {
		DirectX::XMFLOAT2(0,0),
		DirectX::XMFLOAT2(1,0),
		DirectX::XMFLOAT2(0,1),
		DirectX::XMFLOAT2(1,1)
	};
	DirectX::XMFLOAT4   color[] = {
		DirectX::XMFLOAT4(1, 1, 1, 1),			//	top
		DirectX::XMFLOAT4(0.5, 0.5, 0.5, 1),	//	front
		DirectX::XMFLOAT4(0.2, 0.2, 0.2, 1),	//	left
		DirectX::XMFLOAT4(0.2, 0.2, 0.2, 1),	//	back
		DirectX::XMFLOAT4(0.5, 0.5, 0.5, 1),	//	right
	};


	BYTE    flags[] = {
		DEFAULT_WALL_FLAG,
		FRONT_WALL_FLAG,
		LEFT_WALL_FLAG,
		BACK_WALL_FLAG,
		RIGHT_WALL_FLAG
	};
	BYTE    current;


	DirectX::XMMATRIX   world;
	w = 2;
	d = 2;
	m_pPolygon->SetLinearFog(4, 10, DirectX::XMFLOAT4(0, 0, 0, 1));
	m_pPolygon->SetViewMatrix(m_matView);
	for (int row = 0; row < m_sRows; ++row) {
		for (int col = 0; col < m_sColumns; ++col) {
			if (m_pMap[(row + 2)*m_sPitch + (col + 2)] != 0) {

				x = (w * col);
				z = (d * row);
				x -= w * 0.5f * m_sColumns;
				z = d * 0.5f * m_sRows + 5.0f - z;

				world = DirectX::XMMatrixTranslation(x, 0, z);
				m_pPolygon->SetWorldMatrix(world);
				current = m_pMap[(row + 2)*m_sPitch + (col + 2)];
				for (int i = 0; i < min(_countof(color), _countof(vert)); ++i) {
					if (current&flags[i])
						m_pPolygon->Render(pContext, vert[i], tex, color[i], m_pTexSRV);
				}
			}
			else {
				x = (w * col);
				z = (d * row);
				x -= w * 0.5f * m_sColumns;
				z = d * 0.5f * m_sRows + 5.0f - z;

				world = DirectX::XMMatrixTranslation(x, 0, z);
				m_pFloor->SetWorldMatrix(world);
				DirectX::XMMATRIX matLight = world * matCombined;
				m_pFloor->SetAmbientColor(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
				m_pFloor->Render(pContext, pSRV, matLight);


				//	頂点
				DirectX::XMFLOAT3 vert2[2][4] = {
					{
						DirectX::XMFLOAT3(-1.0f, 1.0f, 0.0f),
						DirectX::XMFLOAT3(-1.0f, 1.0f,-2.0f),
						DirectX::XMFLOAT3(-1.0f,-1.0f, 0.0f),
						DirectX::XMFLOAT3(-1.0f,-1.0f,-2.0f)
					},
					{	//	Back
						DirectX::XMFLOAT3(-1.0f, 1.0f, -1.0f),
						DirectX::XMFLOAT3(-1.0f, 1.0f,  1.0f),
						DirectX::XMFLOAT3(-1.0f,-1.0f, -1.0f),
						DirectX::XMFLOAT3(-1.0f,-1.0f,  1.0f)
					}
				};
				
				//	テクスチャ座標
				DirectX::XMFLOAT2 tex2[4] = {
					DirectX::XMFLOAT2(0,0),
					DirectX::XMFLOAT2(1,0),
					DirectX::XMFLOAT2(0,1),
					DirectX::XMFLOAT2(1,1)
				};

				//	入口なら
				if ((row + 2)*m_sPitch + (col + 2) == (m_sRows + 2 - 2)*m_sPitch + m_sColumns + 2 - 1) { 
					world = DirectX::XMMatrixTranslation(x, -0.5f, z + 0.5f);
					m_pPolygon->SetWorldMatrix(world);

					for (int i = 0; i < 2; ++i) {
						m_pPolygon->Render(pContext, vert2[i], tex2, color[0], m_pStartSRV);
					}
				}else if ((row + 2)*m_sPitch + (col + 2) == (2 + 1)*m_sPitch + 2) {
					world = DirectX::XMMatrixTranslation(x + 2.0f, -0.5f, z + 0.5f);
					m_pPolygon->SetWorldMatrix(world);

					for (int i = 0; i < 2; ++i) {
						m_pPolygon->Render(pContext, vert2[i], tex2, color[0], m_pGoalSRV);
					}
				}
			}
		}
	}
}

/**
* @brief 保存されたポイントからランダムに選んでバッファから取り出す
* @param [out]    pt    取り出されたポイント
* @param [in/out] array ポイントを保存したバッファ
* @param [in/out] nSite array に保存されたポイント数
*/
static SITE *selectsite(SITE *pt, SITE *array, uint16_t &nSite) {
	int index;
	if (nSite == 0)
		return NULL;
	index = (rand() >> 2) % nSite;
	*pt = array[index];
	array[index] = array[--nSite];
	return pt;
}



/**
* @brief 保存された数値からランダムに選んでバッファから取り出す
* @param [in/out] array データが保存された配列
* @param [in/out] size  配列に入ったデータの数
* @return 取り出された数値
*/
static int selectint(int *array, int &size) {
	int index, result;
	if (size == 0)
		return -1;
	index = (rand() >> 2) % size;
	result = array[index];
	array[index] = array[--size];
	return result;
}

/**
*  @brief ダンジョンの生成
*/
void CDungeon::GenerateDungeon() {
	memset((void*)m_pMap, 1, (m_sColumns + 4)*(m_sRows + 4));
	int     i, j, k, d;

	//  周囲の壁を残していったん中を空にする
	for (i = 1; i < m_sRows - 1; ++i) {
		for (j = 1; j < m_sColumns - 1; ++j) {
			m_pMap[(i + 2)*m_sPitch + (j + 2)] = 0;
		}
	}
	m_pMap[(2 + 1)*m_sPitch + 2] = 0;         //  左上に出口
	m_pMap[(m_sRows + 2 - 2)*m_sPitch + m_sColumns + 2 - 1] = 0;    //  右下に入り口

	m_sPosX = m_sColumns - 1;       //  入口
	m_sPosZ = m_sRows - 2;          //
	m_sPosXGoal = m_sPosX;          //
	m_sPosZGoal = m_sPosZ;          //
	m_fAngle = DirectX::XM_PI;      //  180°
	m_fAngleGoal = DirectX::XM_PI;  //
	m_fLerp = 0;

	uint16_t    nSite;
	SITE    *psite = (SITE*)_alloca(((m_sRows*m_sPitch) >> 2) * sizeof(SITE));
	SITE    pt;
	int16_t diffX[] = { 2, 0, -2, 0 };
	int16_t diffY[] = { 0, 2, 0, -2 };
	int     direction[] = { 0,1,2,3 };

	srand((unsigned int)time(NULL));
	rand();
	nSite = 0;

	//  迷路作成（壁伸ばし法）
	//  壁伸ばしポイントの追加
	for (i = 2; i <= m_sColumns - 3; i += 2) {
		psite[nSite].x = i + 2;  psite[nSite++].y = 0 + 2;        //  上の壁
		psite[nSite].x = i + 2;  psite[nSite++].y = m_sRows - 1 + 2; //  下の壁
	}

	//  壁伸ばしポイントの追加
	for (j = 2; j <= m_sRows - 3; j += 2) {
		psite[nSite].x = 0 + 2;  psite[nSite++].y = j + 2;            //  左の壁
		psite[nSite].x = m_sColumns - 1 + 2;   psite[nSite++].y = j + 2;    //  右の壁
	}

	//
	//  壁を伸ばす処理
	//
	while (selectsite(&pt, psite, nSite)) {
		while (true) {
			for (d = 0; d < ARRAYSIZE(direction); ++d)
				direction[d] = d;
			while (d > 0) {
				k = selectint(direction, d);
				i = pt.x + diffX[k];
				j = pt.y + diffY[k];
				if (m_pMap[j*m_sPitch + i] == 0)
					break;
			}
			if (d == 0)
				break;
			m_pMap[((pt.y + j) >> 1)*m_sPitch + ((pt.x + i) >> 1)] = 1;
			m_pMap[j*m_sPitch + i] = 1;
			psite[nSite].x = i;  psite[nSite++].y = j;    //  選択元追加
			pt.x = i;
			pt.y = j;
		}
	}
}


/**
* @brief	ブロックの隙間がちらつくのを防ぐため、ブロックに挟まれたポリゴンを描画しないようにする
*/
void CDungeon::CheckInvisibleWall()
{
	BYTE curr;
	for (int row = 0; row < m_sRows; ++row) {
		for (int col = 0; col < m_sColumns; ++col) {
			curr = m_pMap[(row + 2)*m_sPitch + (col + 2)];
			if (curr != 0) {
				curr = 0xff;
				if (m_pMap[(row + 2 - 1)*m_sPitch + (col + 2)] != 0) {
					curr &= ~BACK_WALL_FLAG;    //  後ろをクリア
				}
				if (m_pMap[(row + 2 + 1)*m_sPitch + (col + 2)] != 0) {
					curr &= ~FRONT_WALL_FLAG;   //  後ろをクリア
				}
				if (m_pMap[(row + 2)*m_sPitch + (col + 2 - 1)] != 0) {
					curr &= ~LEFT_WALL_FLAG;    //  左をクリア
				}
				if (m_pMap[(row + 2)*m_sPitch + (col + 2 + 1)] != 0) {
					curr &= ~RIGHT_WALL_FLAG;   //  左をクリア
				}
				m_pMap[(row + 2)*m_sPitch + (col + 2)] = curr;
			}
		}
	}
}


/**
*   @brief  Restore device dependent objects.
*/
HRESULT CDungeon::RestoreDeviceObjects(ID3D11DeviceContext *pContext) {
	m_pPolygon->RestoreDeviceObjects(pContext);
	m_pFloor->RestoreDeviceObjects(pContext);
	DWORD sw, sh;
	ID3D11Texture2D *pTexture = NULL;
	if (SUCCEEDED(CTextureLoader::CreateTextureFromFile(pContext, _T("res\\dungeon\\bricks.dds"), &pTexture, &sw, &sh))) {
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
		pDevice->CreateShaderResourceView(pTexture, &srvd, &m_pTexSRV);
		SAFE_RELEASE(pDevice);
		SAFE_RELEASE(pTexture);
	}
	if (SUCCEEDED(CTextureLoader::CreateTextureFromFile(pContext, _T("res\\start.png"), &pTexture, &sw, &sh))) {
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
		pDevice->CreateShaderResourceView(pTexture, &srvd, &m_pStartSRV);
		SAFE_RELEASE(pDevice);
		SAFE_RELEASE(pTexture);
	}
	if (SUCCEEDED(CTextureLoader::CreateTextureFromFile(pContext, _T("res\\goal.png"), &pTexture, &sw, &sh))) {
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
		pDevice->CreateShaderResourceView(pTexture, &srvd, &m_pGoalSRV);
		SAFE_RELEASE(pDevice);
		SAFE_RELEASE(pTexture);
	}
	return  S_OK;
}


/**
*   @brief  Release device dependent objects.
*/
HRESULT CDungeon::ReleaseDeviceObjects() {
	m_pFloor->ReleaseDeviceObjects();
	m_pPolygon->ReleaseDeviceObjects();
	SAFE_RELEASE(m_pTexSRV);
	SAFE_RELEASE(m_pStartSRV);
	SAFE_RELEASE(m_pGoalSRV);
	return S_OK;
}