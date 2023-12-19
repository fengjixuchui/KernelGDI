#pragma once
namespace GdiDraw
{
	extern PVOID NtUserGetDCPtr;
	extern PVOID NtGdiSelectBrushPtr;
	extern PVOID NtGdiPatBltPtr;
	extern PVOID NtUserReleaseDCPtr;
	extern PVOID NtGdiCreateSolidBrushPtr;
	extern PVOID NtGdiDeleteObjectAppPtr;
	extern PVOID NtGdiExtTextOutWPtr;
	extern PVOID NtGdiHfontCreatePtr;
	extern PVOID NtGdiSelectFontPtr;

	extern PEPROCESS currentProcess;
	extern PETHREAD currentThread ;
	extern CLIENT_ID currentCid;

	extern bool UnDraw;

	bool Init();

	HDC pfnNtUserGetDC(HWND hwnd);

	HBRUSH pfnNtGdiSelectBrush(HDC hdc, HBRUSH hbrush);

	BOOL pfnNtGdiPatBlt(HDC hdcDest, INT x, INT y, INT cx, INT cy, DWORD dwRop);

	int pfnNtUserReleaseDC(HDC hdc);

	HBRUSH pfnNtGdiCreateSolidBrush(COLORREF cr, HBRUSH hbr);

	BOOL pfnNtGdiDeleteObjectApp(HANDLE hobj);

	BOOL pfnNtGdiExtTextOutW(HDC hDC, INT XStart, INT YStart, UINT fuOptions, LPRECT UnsafeRect, LPWSTR UnsafeString, INT Count, LPINT UnsafeDx, DWORD dwCodePage);

	HFONT pfnNtGdiHfontCreate(PENUMLOGFONTEXDVW pelfw, ULONG cjElfw, DWORD lft, FLONG fl, PVOID pvCliData);

	HFONT pfnNtGdiSelectFont(HDC hdc, HFONT hfont);

	PETHREAD GetValidWin32Thread(PVOID* win32Thread);

	void SpoofWin32Thread(PVOID newWin32Value, PEPROCESS newProcess, CLIENT_ID newClientId);

	bool BeginFrame();

	void EndFrame();

	INT FrameRect(RECT rect, int thickness);
}

