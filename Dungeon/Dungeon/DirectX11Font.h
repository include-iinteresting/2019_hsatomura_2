#pragma once

#include <DirectXMath.h>

struct ID3D11DeviceContext;

class CDirectX11Font
{
public:
	static	void	Initialize(ID3D11DeviceContext *pContext);
	static	void	Finalize();
	static	void	DrawAnkText(ID3D11DeviceContext * pContext, const TCHAR * pString, DirectX::XMFLOAT4 color, FLOAT x, FLOAT y);
};

