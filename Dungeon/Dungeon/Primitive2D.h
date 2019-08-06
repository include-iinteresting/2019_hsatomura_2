/**
*  @file  Primitive2D.h
*  @brief 2Dプリミティブを描画するクラス CPrimitive2D の宣言
*  @date  11.May.2019
*/
#pragma once
#include "IDeviceDependentObject.h"

//前方宣言
struct ID3D11DeviceContext;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11SamplerState;
struct ID3D11InputLayout;
struct ID3D11Buffer;
namespace DirectX {
	struct XMMATRIX;
};

#define NUM_TEXTURES	1	
typedef struct {
	ID3D11Texture2D *pTexture;
	ID3D11ShaderResourceView *pTextureShaderResourceView;
	DWORD			dwSrcWidth;
	DWORD			dwSrcHeight;
}	TEXTURE_NODE;

class CPrimitive2D : public IDeviceDependentObject
{
public:
	CPrimitive2D();
	~CPrimitive2D();
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext) override;
	virtual HRESULT ReleaseDeviceObjects() override;
	virtual void Render(ID3D11DeviceContext *pContext, float x, float y, float w, float h);
protected:
	static DirectX::XMMATRIX    m_matProjection;

	DWORD						m_dwWidth;
	DWORD						m_dwHeight;

	ID3D11VertexShader          *m_pVertexShader;
	ID3D11PixelShader           *m_pPixelShader;
	TEXTURE_NODE				m_pTextures[NUM_TEXTURES];
	ID3D11SamplerState			*m_pSamplerState;
	ID3D11InputLayout   *m_pInputLayout;    //  頂点入力レイアウト
	ID3D11Buffer        *m_pVertexBuffer;   //  頂点データを格納するバッファ
	ID3D11Buffer        *m_pConstantBuffer; //  定数データを格納するバッファ
	ID3D11Buffer        *m_pPixelShaderConstantBuffer; //  定数データを格納するバッファ

	INT					m_iPhase;
	FLOAT				m_fAlpha;
	FLOAT				m_fWait;

	BOOL                m_bPrepared;
};
