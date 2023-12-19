#include "pch.h"

bool g_UnThread{};

void MainThread()
{
	GdiDraw::currentProcess = IoGetCurrentProcess();
	
	GdiDraw::currentThread = KeGetCurrentThread();

	memcpy(&GdiDraw::currentCid, (PVOID)((char*)GdiDraw::currentThread + Function::Offset__ETHREAD_Cid), sizeof(CLIENT_ID));

	while (true)
	{
		GdiDraw::BeginFrame();

		GdiDraw::FrameRect({ 100 , 100 , 200 , 200 }, 3);

		GdiDraw::EndFrame();

		if (g_UnThread) {
			break;
		}	
	}

	if (!GdiDraw::UnDraw)
	{
		GdiDraw::EndFrame();
	}
	PsTerminateSystemThread(STATUS_SUCCESS);
}

EXTERN_C auto DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg_Path)->NTSTATUS
{
	NTSTATUS status = STATUS_SUCCESS;

	if (!Function::init())
		return STATUS_SEVERITY_ERROR;


	if (!GdiDraw::Init())
		return STATUS_SEVERITY_ERROR;

	Function::MyCreateThread(MainThread);

	return status;

}