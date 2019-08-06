#pragma once


struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;
struct ID3D11DepthStencilView;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11SamplerState;

class CDirectX11
{
public:
	static	HRESULT	Initialize(HWND hWnd);
	static	void	Finalize();
	static	void	BeginScene();
	static	HRESULT	EndScene();

	static	ID3D11Device				*GetDevice();
	static	ID3D11DeviceContext			*GetDeviceContext();
	static	IDXGISwapChain				*GetSwapChain();
	static	ID3D11RenderTargetView		*GetRenderTargetView();
	static	ID3D11ShaderResourceView	*GetRenderTargetShaderResourceView();
	static	ID3D11DepthStencilView		*GetDepthStenilView();
	static	ID3D11Texture2D				*GetDepthStencilTexture();
	static	ID3D11ShaderResourceView	*GetDepthStencilShaderResourceView();
	static	const	TCHAR				*GetDeviceDriverTypeText();
	static	const	TCHAR				*GetFeatureLevelText();
	static	void						GetBackBufferSize(DWORD *width, DWORD *height);

};

