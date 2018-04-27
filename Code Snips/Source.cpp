#include "SceneRenderer.h"
#include "D3DAccess.h"


int WINAPI WinMain(_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow)
{
	std::shared_ptr<D3DAccess> accessD3D = std::make_shared<D3DAccess>();
	WinApp::SetInstance(hInstance);

	if (!WinApp::CreateWCEX())
		return 1;

	if (!WinApp::CreateHWND())
		return 1;

	ShowWindow(WinApp::GetHWND(), nCmdShow);

	accessD3D->InitD3D(WinApp::GetHWND());

	SceneRenderer::GetInstance();

	SceneRenderer::GetInstance()->Init(accessD3D);

	MSG msg = { NULL };

	while(msg.message != WM_QUIT)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		accessD3D->RenderFrame();
	}

	accessD3D->CleanD3D();

	SceneRenderer::DestroyInstance();

	return (int)msg.wParam;
}