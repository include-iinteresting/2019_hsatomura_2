#pragma once

#define	RT_TEX_WIDTH	(1024)
#define	RT_TEX_HEIGHT	(1024)

class CRenderableTexture2D
{
public:
	CRenderableTexture2D();
	virtual ~CRenderableTexture2D();
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext);
	virtual HRESULT ReleaseDeviceObjects();
	virtual ID3D11RenderTargetView *GetRenderTargetView() { return	m_pTexRTV; };
	virtual ID3D11ShaderResourceView *GetShaderResourceView() { return m_pTexSRV; };

protected:
	ID3D11Texture2D *m_pTexture;
	ID3D11RenderTargetView	*m_pTexRTV;
	ID3D11ShaderResourceView *m_pTexSRV;
};

