#include "WinApp.h"
#include "resource.h"
#include "SceneRenderer.h"

TCHAR WinApp::szWindowClass[] = _T("win32app");
TCHAR WinApp::szTitle[] = _T("Fictional Parakeet FBX Viewer");
HINSTANCE WinApp::hInst;
WNDCLASSEX WinApp::wcex;
HWND WinApp::hWnd;
DirectX::XMINT2 WinApp::pos;
bool WinApp::isRMdown = false;
FBXExporter WinApp::exp;
OtterObjectLoader WinApp::otter;


WinApp::WinApp()
{

}


WinApp::~WinApp()
{

}

bool WinApp::CreateWCEX()
{
	SecureZeroMemory(&wcex, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = (HICON)LoadImage(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("LMAO wcex failed"), _T("You Suck"), NULL);

		return false;
	}

	return true;
}

bool WinApp::CreateHWND()
{
	SecureZeroMemory(&hWnd, sizeof(HWND));
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		450, 150, 1000, 800, NULL, NULL, hInst, NULL);

	auto result = GetLastError();

	if (!hWnd)
	{
		MessageBox(NULL,
			_T("LMAO hwnd failed"), _T("You Suck"), NULL);

		return false;
	}

	return true;
}

void WinApp::SetInstance(HINSTANCE hInstance)
{
	hInst = hInstance;
}

DirectX::XMINT2 WinApp::getMousePos()
{
	return pos;
}

bool WinApp::getMouseState()
{
	return isRMdown;
}

WNDCLASSEX WinApp::GetWCEX()
{
	return wcex;
}

HWND WinApp::GetHWND()
{
	return hWnd;
}

LRESULT WinApp::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case ID_ANIMATION_START: // start the animation
		{
			SceneRenderer::GetInstance()->ViewBindpose = false;
			SceneRenderer::GetInstance()->PauseAnim = false;
		}
		break;

		case ID_ANIMATION_STOP: //stop the animation
		{
			SceneRenderer::GetInstance()->ViewBindpose = false;
			SceneRenderer::GetInstance()->PauseAnim = true;
		}
		break;

		case ID_ANIMATION_NEXT: // next frame
		{
			SceneRenderer::GetInstance()->ViewBindpose = false;
			SceneRenderer::GetInstance()->ChangeCurrentFrame(1);
			
		}
		break;

		case ID_ANIMATION_PREV: // prev frame 
		{
			SceneRenderer::GetInstance()->ViewBindpose = false;
			SceneRenderer::GetInstance()->ChangeCurrentFrame(-1);
		
		}
		break;
		
		case ID_ANIMATION_VIEWBINDPOSE: // prev frame 
		{
			SceneRenderer::GetInstance()->ViewBindpose = true;
		}
		break;


		case ID_FILE_OPEN:
		{
			wchar_t fname[MAX_PATH];

			OPENFILENAME ofn;

			SecureZeroMemory(&fname, sizeof(fname));
			SecureZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hwnd;
			ofn.lpstrFilter = L"FBX Files (*.fbx)\0*.fbx\0 Waffle Files (*.wafl)\0*.wafl\0";
			ofn.lpstrFile = fname;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrTitle = L"Choose a file...";
			ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_EXPLORER;

			std::string file;
			std::string texture;

			if (GetOpenFileNameW(&ofn))
			{
				std::wstring wf(fname);
				file = std::string(wf.begin(), wf.end());
			}

			SecureZeroMemory(&fname, sizeof(fname));
			ofn.lpstrFilter = L"DirectDraw Surface Files (*.dds)\0*.dds\0";
			ofn.lpstrFile = fname;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrTitle = L"Choose a texture...";
			ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_EXPLORER;

			if (GetOpenFileNameW(&ofn))
			{
				std::wstring wf(fname);
				texture = std::string(wf.begin(), wf.end());
			}

			SecureZeroMemory(&fname, sizeof(fname));
			ofn.lpstrFilter = L"DirectDraw Surface Files (*.dds)\0*.dds\0";
			ofn.lpstrFile = fname;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrTitle = L"Choose a ground texture...";
			ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_EXPLORER;

			if (GetOpenFileNameW(&ofn))
			{
				std::wstring wf(fname);
				SceneRenderer::GetInstance()->SetFloorPlaneTexturePath(wf);
			}

			SecureZeroMemory(&fname, sizeof(fname));
			ofn.lpstrFilter = L"Wavefront Object Files (*.obj)\0*.obj\0";
			ofn.lpstrTitle = L"Choose a sphere object...";

			if (GetOpenFileNameW(&ofn))
			{
				std::wstring wf(fname);
				otter.LoadModelsAsync(std::string(wf.begin(), wf.end()));
				while (!otter.IsLoadComplete())
				{

				}

				SceneRenderer::GetInstance()->SetSkybox(Model(otter.GetModelInformation()));

			}

			SecureZeroMemory(&fname, sizeof(fname));
			ofn.lpstrFilter = L"DirectDraw Surface Files (*.dds)\0*.dds\0";
			ofn.lpstrTitle = L"Choose a skybox texture...";

			if (GetOpenFileNameW(&ofn))
			{
				std::wstring wf(fname);
				SceneRenderer::GetInstance()->SetTexturePath(wf);
			}

			exp.ReadFBX(file, texture);

			SceneRenderer::GetInstance()->fbexp = &exp;
			SceneRenderer::GetInstance()->mloaded = true;

			SceneRenderer::GetInstance()->SetWindowHandle(hwnd);
			SceneRenderer::GetInstance()->CreateDeviceDependentResources();
			SceneRenderer::GetInstance()->CreateWindowSizeDependentResources();
		}
		break;
		case ID_FILE_SAVE:
		{
#if 0
			wchar_t fname[MAX_PATH];

			OPENFILENAME ofn;

			SecureZeroMemory(&fname, sizeof(fname));
			SecureZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hwnd;
			ofn.lpstrFilter = L"FBX Files (*.fbx)\0*.fbx\0 Waffle Files (*.wafl)\0*.wafl\0";
			ofn.lpstrFile = fname;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrTitle = L"Choose a file...";
			ofn.Flags = OFN_DONTADDTORECENT | OFN_EXPLORER;

			if (GetSaveFileNameW(&ofn))
			{
				std::wstring wf(fname);
				std::string file(wf.begin(), wf.end());

				if (ofn.nFilterIndex == 1)
					file.append(".fbx");
				else
					file.append(".wafl");

				char ext[_MAX_EXT];

				_splitpath_s(file.c_str(), NULL, 0, NULL, 0, NULL, 0, ext, _MAX_EXT);

				if (!strcmp(".fbx", ext))
				{
					//FBXExporter exp;
					//exp.ReadFBX(file);

					//FBXExporter::WaffleModel* m = exp.GetModels();
				}
				else
				{

				}
			}
#endif
		}
		break;
		case ID_DEBUG_NONE:
			SceneRenderer::GetInstance()->SetDebugMode(SceneRenderer::DebugMode::NONE);
			break;
		case ID_DEBUG_BONES:
			SceneRenderer::GetInstance()->SetDebugMode(SceneRenderer::DebugMode::BONES);
			break;
		case ID_FILE_EXIT:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}
		break;
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	break;

	case WM_MOUSEMOVE:
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		RECT  theWindow;
		if (GetClientRect(GetHWND(), &theWindow))
		{
			if (y > 0 && x > 0 && x < theWindow.right && y < theWindow.bottom) //they are within the range of the window;
			{	//update the cur mouse position
				pos.x = x;
				pos.y = y;
			}
		}
		else
		{
			DWORD temp = GetLastError();
		}

	}
	break;

	case WM_RBUTTONDOWN: // the right mouse button is pressed
	{
		isRMdown = true;
	}
	break;

	case WM_RBUTTONUP: // the right mouse button is pressed
	{
		isRMdown = false;
	}
	break;

	case WM_SIZE:
	{
		RECT  thewindow;
		if (GetClientRect(GetHWND(), &thewindow))
		{
			SceneRenderer::GetInstance()->ResizeWindow((float)thewindow.right, (float)thewindow.bottom);
		}
		else
		{
			DWORD temp = GetLastError();
		}
	}
	break;
	}


	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

