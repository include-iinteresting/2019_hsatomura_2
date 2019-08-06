#include "stdafx.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include "CameraManager.h"


#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if(o) { (o)->Release(); o = NULL; };
#define SAFE_DELETE(o) if(o) { delete(o); o = NULL; };
#define SAFE_DELETE_ARRAY(o) if(o) { delete[](o); o = NULL; };

/**
* @class	CameraManagerImpl
* @brief	カメラ管理クラスの中身
*/
class CameraManagerImpl {
	friend CCameraManager;
private:
	CameraManagerImpl();
	~CameraManagerImpl();

	static	void	Initialize();
	static	void	Finalize();
	static	CameraManagerImpl	*GetInstance();

	void	SetViewMatrix(DirectX::XMMATRIX matView);
private:
	static	CameraManagerImpl	*m_pInstance;
};

CameraManagerImpl *CameraManagerImpl::m_pInstance = NULL;

/**
* @brief	constructor
*/
CameraManagerImpl::CameraManagerImpl()
{
}


/**
* @brief	destructor
*/
CameraManagerImpl::~CameraManagerImpl()
{
}


/**
* @brief	Initialize
*/
void CameraManagerImpl::Initialize()
{
	if (m_pInstance)
		return;

	m_pInstance = new CameraManagerImpl();
}


/**
* @brief	Finalize
*/
void CameraManagerImpl::Finalize()
{
	SAFE_DELETE(m_pInstance);
}


/**
* @brief	GetInstance
* @return	CameraManagerImplObjectInstance
*/
CameraManagerImpl * CameraManagerImpl::GetInstance()
{
	if (!m_pInstance)
		CameraManagerImpl::Initialize();

	return m_pInstance;
}


/**
* @brief	このゲームのビュー行列を一括で設定する
*/
void CameraManagerImpl::SetViewMatrix(DirectX::XMMATRIX matView)
{

}

