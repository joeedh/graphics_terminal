// graphics_terminal.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "graphics_terminal.h"

#include <io.h>
#include <fcntl.h>

extern "C" {
#include "../../../simple_raster.h"
#include "../../../raster_types.h"
#include "../../../alloc.h"
}

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int, HWND *);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

typedef struct AppState {
	Raster *raster;
	BYTE *buffer;
	HWND hWnd;
	HBITMAP hBm;
	int width, height;
	HDC hdc, lasthdc;
	PIXELFORMATDESCRIPTOR  pfd;
} AppState;

AppState appstate = {0,};

void MyShowConsole() {
	AllocConsole();

	HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    int hCrt = _open_osfhandle((long) handle_out, _O_TEXT);
    FILE* hf_out = _fdopen(hCrt, "w");
    setvbuf(hf_out, NULL, _IONBF, 1);
    *stdout = *hf_out;

	HANDLE handle_err = GetStdHandle(STD_ERROR_HANDLE);
    hCrt = _open_osfhandle((long) handle_err, _O_TEXT);
    FILE* hf_err = _fdopen(hCrt, "w");
    setvbuf(hf_err, NULL, _IONBF, 1);
    *stderr = *hf_err;

    HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
    hCrt = _open_osfhandle((long) handle_in, _O_TEXT);
    FILE* hf_in = _fdopen(hCrt, "r");
    setvbuf(hf_in, NULL, _IONBF, 128);
    *stdin = *hf_in;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	init_default_styles();

	MSG msg;
	HACCEL hAccelTable;
	HWND hWnd;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_GRAPHICS_TERMINAL, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	PIXELFORMATDESCRIPTOR  pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		0, 0, 0, 0, 0, 0,      // color bits ignored  
		0,                     // no alpha buffer  
		0,                     // shift bit ignored  
		0,                     // no accumulation buffer  
		0, 0, 0, 0,            // accum bits ignored  
		0,                    // 32-bit z-buffer      
		0,                     // no stencil buffer  
		0,                     // no auxiliary buffer  
		PFD_MAIN_PLANE,        // main layer  
		0,                     // reserved  
		0, 0, 0                // layer masks ignored  
	};

	appstate.pfd = pfd;

	MyShowConsole();

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow, &hWnd))
	{
		return FALSE;
	}
	
	//GetWindowRgn(hWnd, hRgn);

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GRAPHICS_TERMINAL));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		Sleep(10);
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GRAPHICS_TERMINAL));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_GRAPHICS_TERMINAL);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int, HWND *)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND *hWndOut)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   *hWndOut = hWnd;

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
int AppPaint() {
	if (!appstate.raster) {
		//resend paint request
		UpdateWindow(appstate.hWnd);

		return 0;
	}

	if (appstate.hBm) {
		DeleteObject(appstate.hBm);
	}

	appstate.hBm = CreateCompatibleBitmap(appstate.hdc, appstate.width, appstate.height);
	
	HDC hdcMem = CreateCompatibleDC(appstate.hdc);
    HBITMAP hBmMem = static_cast<HBITMAP>(SelectObject(hdcMem, appstate.hBm));

	/*
	DWORD flags = appstate.pfd.dwFlags & ~PFD_DRAW_TO_WINDOW;
	flags |= PFD_DRAW_TO_BITMAP;

	DWORD old = appstate.pfd.dwFlags;

	appstate.pfd.dwFlags = flags;
	ChoosePixelFormat(hdcMem, &appstate.pfd);
	appstate.pfd.dwFlags = old;
	*/

	BITMAP bm;
    BITMAPINFO bmInfo;

    if (!GetObject(appstate.hBm, sizeof(bm), &bm)) {
		//error
		return 0;
	}

	const int bytesPerPixel = bm.bmBitsPixel / 8;
    
	//rgba only
	if (bm.bmBitsPixel % 8) {
		return 0;
	}

	memset(&bmInfo, 0, sizeof(bmInfo));

	bmInfo.bmiHeader.biSize = sizeof(bmInfo);
	bmInfo.bmiHeader.biWidth = bm.bmWidth;
	bmInfo.bmiHeader.biHeight = bm.bmHeight;
	bmInfo.bmiHeader.biPlanes = bm.bmPlanes;
	bmInfo.bmiHeader.biBitCount = bm.bmBitsPixel;
	bmInfo.bmiHeader.biCompression = BI_RGB;
	
	raster_raster(appstate.raster);

	BYTE *buffer = (BYTE*) appstate.raster->buffer;
	BYTE *buffer2 = appstate.buffer;

	for (int i=0; i<appstate.width*appstate.height; i++) {
		int x = i % appstate.width, y1 = appstate.height - 1 - i / appstate.width;

		int idx1 = i*4;
		int idx2 = (y1*appstate.width + x)*4;

		buffer2[idx1] = buffer[idx2+2];
		buffer2[idx1+1] = buffer[idx2+1];
		buffer2[idx1+2] = buffer[idx2];
		buffer2[idx1+3] = buffer[idx2+3];
	}

	int ret = SetDIBits(hdcMem, appstate.hBm, 0, bm.bmHeight, buffer2, &bmInfo, DIB_RGB_COLORS);
	//printf("ret: %d\n", ret);

	ret = BitBlt(appstate.hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
	//printf("ret: %d\n", ret);
    
	//SelectObject(appstate.hdc, appstate.hBm);

	if (appstate.lasthdc)
		DeleteDC(appstate.lasthdc);

	appstate.lasthdc = hdcMem;
		
	return 1;
}

void rasterInit() {
	Raster *raster = appstate.raster;
	int eidgen = 0;

	PathVertex *v1 = raster_make_vertex(raster, eidgen++, 0, 7);
	PathVertex *v2 = raster_make_vertex(raster, eidgen++, 20, 400);
	PathVertex *v3 = raster_make_vertex(raster, eidgen++, 600, 420);
	PathVertex *v4 = raster_make_vertex(raster, eidgen++, 200, 70);

	PathSegment *s1 = raster_make_segment(raster, eidgen++, v1->head.eid, v1->head.eid, v2->head.eid, v2->head.eid);
	PathSegment *s2 = raster_make_segment(raster, eidgen++, v2->head.eid, v2->head.eid, v3->head.eid, v3->head.eid);
	PathSegment *s3 = raster_make_segment(raster, eidgen++, v3->head.eid, v3->head.eid, v4->head.eid, v4->head.eid);
	PathSegment *s4 = raster_make_segment(raster, eidgen++, v4->head.eid, v4->head.eid, v1->head.eid, v1->head.eid);

	Path *path = raster_make_path(raster, eidgen++, 0);

	raster_path_append(raster, path, s1);
	raster_path_append(raster, path, s2);
	raster_path_append(raster, path, s3);
	raster_path_append(raster, path, s4);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	if (!appstate.raster) {
		RECT rect = {0,};
		GetWindowRect(hWnd, &rect);

		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		appstate.width = width;
		appstate.height = height;
		appstate.hWnd = hWnd;

		if (width>1 && height>1) {
			appstate.raster = raster_new(width, height);
			rasterInit();
			appstate.buffer = (BYTE*) MEM_calloc(width*height*4);
		}
	}

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		
		//ChoosePixelFormat(hdc, &appstate.pfd);

		appstate.hdc = hdc;

		if (appstate.lasthdc) {
			int ret = BitBlt(appstate.hdc, 0, 0, appstate.width, appstate.height, appstate.lasthdc, 0, 0, SRCCOPY);
		}

		AppPaint();
		
		// TODO: Add any drawing code here...

		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
