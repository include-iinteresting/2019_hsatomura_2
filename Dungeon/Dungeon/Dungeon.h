#pragma once

#include "IDeviceDependentObject.h"

class CPrimitive2D;
struct ID3D11DeviceContext;
class  IPolygonFT4;
class CFloor;

__declspec(align(16)) class CDungeon : public IDeviceDependentObject
{
public:
	CDungeon(SHORT screenWidth, SHORT screenHeight, SHORT displayWidth, SHORT displayHeight, SHORT column, SHORT row);
	virtual ~CDungeon();
	void GenerateDungeon();
	void CheckInvisibleWall();

	inline void SetViewMatrix(DirectX::XMMATRIX &viewMatrix) {
		SetViewMatrix(static_cast<DirectX::XMMATRIX&&>(viewMatrix));
	}
	void SetViewMatrix(DirectX::XMMATRIX &&viewMatrix);
	void Update();

	void Render(ID3D11DeviceContext *pContext, CPrimitive2D *pPrimitive, DirectX::XMMATRIX &matCombined, ID3D11ShaderResourceView *pSRV = NULL);
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext) override;
	virtual HRESULT ReleaseDeviceObjects() override;

	//!  newしたとき、16バイトにアライメントされているようにする
	void *operator new(size_t size) {
		return _mm_malloc(size, 16);
	}
	void *operator new[](size_t size) {
		return _mm_malloc(size, 16);
	}
		void operator delete(void *p) {
		return _mm_free(p);
	}
protected:
	float m_fScreenWidth;
	float m_fScreenHeight;
	float m_fDisplayWidth;
	float m_fDisplayHeight;
	SHORT   m_sColumns;
	SHORT   m_sRows;
	SHORT   m_sPitch;
	BYTE    *m_pMap;

	DirectX::XMMATRIX	m_matView;	//!<	SSE2 命令を使うため、16バイトアライメントが必須！

	SHORT   m_sPosX;
	SHORT   m_sPosZ;
	SHORT   m_sPosXGoal;
	SHORT   m_sPosZGoal;
	FLOAT   m_fAngle;
	FLOAT   m_fAngleGoal;
	FLOAT   m_fLerp;


	IPolygonFT4 *m_pPolygon;
	ID3D11ShaderResourceView *m_pTexSRV;
	ID3D11ShaderResourceView *m_pStartSRV, *m_pGoalSRV;

	//  隠ぺいされた壁を消すためのフラグ
	static const BYTE DEFAULT_WALL_FLAG = 1;    //  ブロックの有無
	static const BYTE FRONT_WALL_FLAG = 2;
	static const BYTE LEFT_WALL_FLAG = 4;
	static const BYTE BACK_WALL_FLAG = 8;
	static const BYTE RIGHT_WALL_FLAG = 16;

	CFloor	*m_pFloor;
};