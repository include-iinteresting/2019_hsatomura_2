
#include "stdafx.h"  //!< プリコンパイルヘッダ
#include <crtdbg.h>
#include <tchar.h>
#include <stdio.h>
#include <D3D11.h>
#include <directxmath.h>
#include <D3Dcompiler.h>
#include "DirectX11.h"
#include "DirectX11Font.h"
#include "TextureLoader.h"
#include "Selector.h"
#include "main.h"


using namespace DirectX;

//  ライブラリのインポート
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

//  マクロ定義（解放用）
#define SAFE_DELETE(o)       if (o){  delete (o);  o = NULL;    }
#define SAFE_DELETE_ARRAY(o) if (o){  delete [] (o);  o = NULL; }
#define SAFE_RELEASE(o)      if (o){  (o)->Release(); o = NULL; }

//  グローバル変数
const TCHAR *g_pDriverType = NULL;
const TCHAR *g_pFeatureLevel = NULL;
UINT  g_uiCount = 0;

//  プロトタイプ宣言
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


/**
*@brief アプリケーションのエントリーポイント
*/
int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
	//! デバッグ用フラグのセット
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	//! 変数宣言
	WNDCLASSEX wcex;  //  ウィンドウクラス構造体
	HWND hWnd;        //  ウィンドウハンドル
	RECT    bounds, client;  //  RECT 構造体

							 //  (1)初期化部分

							 //  (1)-a ウィンドウクラスの登録
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;  //  ウィンドウプロシージャの登録
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;  //  アプリケーションインスタンス
	wcex.hIcon = (HICON)LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = _T("Dungeon");  //  ウィンドウクラス名
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&wcex);

	//  (1)-b ウィンドウの生成
	hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, wcex.lpszClassName, _T("Dungeon"),
		WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		CW_USEDEFAULT, 0, 852, 480, NULL, NULL, hInstance, NULL);
	if (!hWnd)
		return FALSE;

	//  ウィンドウサイズの調整
	//  ここは無くても動く
	GetWindowRect(hWnd, &bounds);
	GetClientRect(hWnd, &client);
	MoveWindow(hWnd, bounds.left, bounds.top,
		852 * 2 - client.right,
		480 * 2 - client.bottom,
		false);

	//  ウィンドウの再表示
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//====================初期化処理===================

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);          //  COM を初期化


	CDirectX11::Initialize(hWnd);
	HRESULT hr;
	
	//==========各種オブジェクト生成===================

	CSelector	*pSelector = new CSelector();
	ID3D11DeviceContext *pContext = CDirectX11::GetDeviceContext();
	
	//=================================================



	//  (2)メッセージループ
	MSG        msg;
	while (true) {
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {

			if (pSelector)
				pSelector->Update();

			//	描画開始
			CDirectX11::BeginScene();

			//  フォントを描画
			float x = 320.0f;
			float y = 432.0f;
			TCHAR tmp[32];
			_stprintf_s(tmp, _T("Satomura Hikaru"));
			CDirectX11Font::DrawAnkText(pContext, tmp, XMFLOAT4(1.0f, 1.0f, 1.0f, 0.75f), x, y);

			pSelector->Render(pContext);



			//	描画終了
			hr = CDirectX11::EndScene();
			if (FAILED(hr))
				goto ERROR_EXIT;
		}
	}

	//=======================終了処理=====================
ERROR_EXIT:
	
	CTextureLoader::Destroy();
	CDirectX11::Finalize();

	CoUninitialize();    //  COM の終了
	//====================================================

	return (int)msg.wParam;
}


/**
* @brief ウインドウに渡されたイベントのハンドラ
*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return    0;
}
