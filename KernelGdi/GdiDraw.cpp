#include "pch.h"

PVOID GdiDraw::NtUserGetDCPtr = nullptr;
PVOID GdiDraw::NtGdiSelectBrushPtr = nullptr;
PVOID GdiDraw::NtGdiPatBltPtr = nullptr;
PVOID GdiDraw::NtUserReleaseDCPtr = nullptr;
PVOID GdiDraw::NtGdiCreateSolidBrushPtr = nullptr;
PVOID GdiDraw::NtGdiDeleteObjectAppPtr = nullptr;
PVOID GdiDraw::NtGdiExtTextOutWPtr = nullptr;
PVOID GdiDraw::NtGdiHfontCreatePtr = nullptr;
PVOID GdiDraw::NtGdiSelectFontPtr = nullptr;

PEPROCESS GdiDraw::currentProcess{};
PETHREAD GdiDraw::currentThread{};
CLIENT_ID GdiDraw::currentCid{};

bool GdiDraw::UnDraw{};
KAPC_STATE g_Draw_apc{};
HDC g_hdc{};
HBRUSH g_brush{};
PVOID currentWin32Thread = 0;


bool GdiDraw::Init()
{
	HANDLE ProcessId{};
	auto status = Function::GetProcessIdByName(L"explorer.exe", &ProcessId);

	if (status != STATUS_SUCCESS)
		return 1;

	PEPROCESS pEprocess{};
	status = PsLookupProcessByProcessId(ProcessId, &pEprocess);
	if (status != STATUS_SUCCESS)
		return 1;

	KAPC_STATE kApc{};
	KeStackAttachProcess(pEprocess, &kApc);

	PVOID win32kBase = (PVOID)Function::GetKernelModuleBase("win32kbase.sys");
	if (win32kBase == nullptr)
	{
		KeUnstackDetachProcess(&kApc);
		ObDereferenceObject(pEprocess);
		return false;
	}

	PVOID win32kfullBase = (PVOID)Function::GetKernelModuleBase("win32kfull.sys");
	if (win32kfullBase == nullptr)
	{
		KeUnstackDetachProcess(&kApc);
		ObDereferenceObject(pEprocess);
		return false;
	}

	NtUserGetDCPtr = pRtlFindExportedRoutineByName(win32kBase, "NtUserGetDC");
	if (!NtUserGetDCPtr)
		goto exit;

	NtGdiPatBltPtr = pRtlFindExportedRoutineByName(win32kfullBase, "NtGdiPatBlt");
	if (!NtGdiPatBltPtr)
		goto exit;

	NtGdiSelectBrushPtr = pRtlFindExportedRoutineByName(win32kBase, "GreSelectBrush");
	if (!NtGdiSelectBrushPtr)
		goto exit;

	NtUserReleaseDCPtr = pRtlFindExportedRoutineByName(win32kBase, "NtUserReleaseDC");
	if (!NtUserReleaseDCPtr)
		goto exit;

	NtGdiCreateSolidBrushPtr = pRtlFindExportedRoutineByName(win32kfullBase, "NtGdiCreateSolidBrush");
	if (!NtGdiCreateSolidBrushPtr)
		goto exit;

	NtGdiDeleteObjectAppPtr = pRtlFindExportedRoutineByName(win32kBase, "NtGdiDeleteObjectApp");
	if (!NtGdiDeleteObjectAppPtr)
		goto exit;

	NtGdiExtTextOutWPtr = pRtlFindExportedRoutineByName(win32kfullBase, "NtGdiExtTextOutW");
	if (!NtGdiExtTextOutWPtr)
		goto exit;

	NtGdiHfontCreatePtr = pRtlFindExportedRoutineByName(win32kfullBase, "hfontCreate");
	if (!NtGdiHfontCreatePtr)
		goto exit;

	NtGdiSelectFontPtr = pRtlFindExportedRoutineByName(win32kfullBase, "NtGdiSelectFont");
	if (!NtGdiSelectFontPtr)
		goto exit;

	KeUnstackDetachProcess(&kApc);
	ObDereferenceObject(pEprocess);
	return true;
exit:
	KeUnstackDetachProcess(&kApc);
	ObDereferenceObject(pEprocess);
	return false;
}

HDC GdiDraw::pfnNtUserGetDC(HWND hwnd)
{
	auto fn = reinterpret_cast<HDC(*)(HWND hwnd)>(NtUserGetDCPtr);
	return fn(hwnd);
}

HBRUSH GdiDraw::pfnNtGdiSelectBrush(HDC hdc, HBRUSH hbrush)
{
	auto fn = reinterpret_cast<HBRUSH(*)(HDC hdc, HBRUSH hbrush)>(NtGdiSelectBrushPtr);
	return fn(hdc, hbrush);
}

BOOL GdiDraw::pfnNtGdiPatBlt(HDC hdcDest, INT x, INT y, INT cx, INT cy, DWORD dwRop)
{
	auto fn = reinterpret_cast<BOOL(*)(HDC hdcDest, INT x, INT y, INT cx, INT cy, DWORD dwRop)>(NtGdiPatBltPtr);
	return fn(hdcDest, x, y, cx, cy, dwRop);
}

int GdiDraw::pfnNtUserReleaseDC(HDC hdc)
{
	auto fn = reinterpret_cast<int(*)(HDC hdc)>(NtUserReleaseDCPtr);
	return fn(hdc);
}

HBRUSH GdiDraw::pfnNtGdiCreateSolidBrush(COLORREF cr, HBRUSH hbr)
{
	auto fn = reinterpret_cast<HBRUSH(*)(COLORREF cr, HBRUSH hbr)>(NtGdiCreateSolidBrushPtr);
	return fn(cr, hbr);
}

BOOL GdiDraw::pfnNtGdiDeleteObjectApp(HANDLE hobj)
{
	auto fn = reinterpret_cast<BOOL(*)(HANDLE hobj)>(NtGdiDeleteObjectAppPtr);
	return fn(hobj);
}

BOOL GdiDraw::pfnNtGdiExtTextOutW(HDC hDC, INT XStart, INT YStart, UINT fuOptions, LPRECT UnsafeRect, LPWSTR UnsafeString, INT Count, LPINT UnsafeDx, DWORD dwCodePage)
{
	auto fn = reinterpret_cast<BOOL(*)(HDC hDC, INT XStart, INT YStart, UINT fuOptions, LPRECT UnsafeRect, LPWSTR UnsafeString, INT Count, LPINT UnsafeDx, DWORD dwCodePage)>(NtGdiExtTextOutWPtr);
	return fn(hDC, XStart, YStart, fuOptions, UnsafeRect, UnsafeString, Count, UnsafeDx, dwCodePage);
}

HFONT GdiDraw::pfnNtGdiHfontCreate(PENUMLOGFONTEXDVW pelfw, ULONG cjElfw, DWORD lft, FLONG fl, PVOID pvCliData)
{
	auto fn = reinterpret_cast<HFONT(*)(PENUMLOGFONTEXDVW pelfw, ULONG cjElfw, DWORD lft, FLONG fl, PVOID pvCliData)>(NtGdiHfontCreatePtr);
	return fn(pelfw, cjElfw, lft, fl, pvCliData);
}

HFONT GdiDraw::pfnNtGdiSelectFont(HDC hdc, HFONT hfont)
{
	auto fn = reinterpret_cast<HFONT(*)(HDC hdc, HFONT hfont)>(NtGdiSelectFontPtr);
	return fn(hdc, hfont);
}

PETHREAD GdiDraw::GetValidWin32Thread(PVOID* win32Thread)
{
	int currentThreadId = 1;
	NTSTATUS status = STATUS_SUCCESS;
	do
	{
		PETHREAD currentEthread = 0;
		status = PsLookupThreadByThreadId((HANDLE)currentThreadId, &currentEthread);

		if (!NT_SUCCESS(status) || !currentEthread)
		{
			currentThreadId++;
			continue;
		}

		if (PsIsThreadTerminating(currentEthread))
		{
			currentThreadId++;
			continue;
		}

		PVOID Win32Thread;
		memcpy(&Win32Thread, (PVOID)((UINT64)currentEthread + Function::Offset__KTHREAD_Win32Thread), sizeof(PVOID));

		if (Win32Thread)
		{
			PEPROCESS threadOwner = PsGetThreadProcess(currentEthread);
			char procName[15];
			memcpy(&procName, (PVOID)((UINT64)threadOwner + Function::Offset__EPROCESS_ImageFileName), sizeof(procName));
			if (!strcmp(procName, "explorer.exe"))
			{
				*win32Thread = Win32Thread;
				return currentEthread;
			}
		}
		currentThreadId++;
	} while (0x3000 > currentThreadId);
	return PETHREAD();
}

void GdiDraw::SpoofWin32Thread(PVOID newWin32Value, PEPROCESS newProcess, CLIENT_ID newClientId)
{
	PKTHREAD currentThread = KeGetCurrentThread();

	PVOID win32ThreadPtr = (PVOID)((char*)currentThread + Function::Offset__KTHREAD_Win32Thread);
	memcpy(win32ThreadPtr, &newWin32Value, sizeof(PVOID));

	PVOID processPtr = (PVOID)((char*)currentThread + Function::Offset__KTHREAD_Process);
	memcpy(processPtr, &newProcess, sizeof(PEPROCESS));

	PVOID clientIdPtr = (PVOID)((char*)currentThread + Function::Offset__ETHREAD_Cid);
	memcpy(clientIdPtr, &newClientId, sizeof(CLIENT_ID));
}

bool GdiDraw::BeginFrame()
{
	PVOID targetWin32Thread = 0;
	PETHREAD targetThread = GetValidWin32Thread(&targetWin32Thread);
	if (!targetWin32Thread || !targetThread)
		return false;
	PEPROCESS targetProcess = PsGetThreadProcess(targetThread);

	CLIENT_ID targetCid = { 0 };
	memcpy(&targetCid, (PVOID)((char*)targetThread + Function::Offset__ETHREAD_Cid), sizeof(CLIENT_ID));

	KeStackAttachProcess(targetProcess, &g_Draw_apc);
	SpoofWin32Thread(targetWin32Thread, targetProcess, targetCid);

	g_hdc = pfnNtUserGetDC(0);
	if (!g_hdc)
	{
		SpoofWin32Thread(currentWin32Thread, currentProcess, currentCid);
		KeUnstackDetachProcess(&g_Draw_apc);
		return false;
	}

	g_brush = pfnNtGdiCreateSolidBrush(RGB(255, 0, 0), NULL);
	if (!g_brush)
	{
		pfnNtUserReleaseDC(g_hdc);
		SpoofWin32Thread(currentWin32Thread, currentProcess, currentCid);
		KeUnstackDetachProcess(&g_Draw_apc);
		return false;
	}
	return true;
}

void GdiDraw::EndFrame()
{
	UnDraw = false;
	pfnNtGdiDeleteObjectApp(g_brush);
	pfnNtUserReleaseDC(g_hdc);
	SpoofWin32Thread(currentWin32Thread, currentProcess, currentCid);
	KeUnstackDetachProcess(&g_Draw_apc);
	UnDraw = true;
}

INT GdiDraw::FrameRect(RECT rect, int thickness)
{
	HBRUSH oldBrush = pfnNtGdiSelectBrush(g_hdc, g_brush);
	if (!oldBrush)
		return 0;


	pfnNtGdiPatBlt(g_hdc, rect.left, rect.top, thickness, rect.bottom - rect.top, PATCOPY);
	pfnNtGdiPatBlt(g_hdc, rect.right - thickness, rect.top, thickness, rect.bottom - rect.top, PATCOPY);
	pfnNtGdiPatBlt(g_hdc, rect.left, rect.top, rect.right - rect.left, thickness, PATCOPY);
	pfnNtGdiPatBlt(g_hdc, rect.left, rect.bottom - thickness, rect.right - rect.left, thickness, PATCOPY);
	pfnNtGdiSelectBrush(g_hdc, oldBrush);
	return 1;
}