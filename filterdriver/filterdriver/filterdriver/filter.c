#include "filter.h"
#include "common.h"

FILTER_DATA g_filterData = { 0 };

const UNICODE_STRING g_myExtensions[] = {
	RTL_CONSTANT_STRING(L"txt"),
	{0, 0, NULL}
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, OnSimpleFilterInstanceSetup)
#pragma alloc_text(PAGE, OnPreCreateFile)
#endif

FLT_OPERATION_REGISTRATION g_callbacks[] = {
	{
		IRP_MJ_CREATE,
		0,
		OnPreCreateFile,
		NULL
	},
	{
		IRP_MJ_OPERATION_END
	}
};

FLT_CONTEXT_REGISTRATION g_contextRegistration[] = {
	{
		FLT_STREAMHANDLE_CONTEXT,
		0,
		NULL,
		sizeof(FILTER_STREAM_HANDLE_CONTEXT),
		SIMPLE_TAG,
		NULL,
		NULL,
		NULL
	},
	{
		FLT_CONTEXT_END
	}
};

FLT_REGISTRATION g_FilterRegistration = {
	sizeof(FLT_REGISTRATION),
	FLT_REGISTRATION_VERSION,
	0,
	g_contextRegistration,
	g_callbacks,
	OnSimpleFilterUnload,
	OnSimpleFilterInstanceSetup,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

NTSTATUS DriverEntry(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
)
{
	NTSTATUS status;
	PSECURITY_DESCRIPTOR sd;

	UNREFERENCED_PARAMETER(RegistryPath);

	status = STATUS_UNSUCCESSFUL;

	status = FltRegisterFilter(DriverObject, &g_FilterRegistration, &g_filterData.pFilter);
	if (!NT_SUCCESS(status))
		return status;

	sd = NULL;

	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
	if (NT_SUCCESS(status)) {
		OBJECT_ATTRIBUTES oa = { 0 };
		UNICODE_STRING myPortName = { 0 };
		RtlInitUnicodeString(&myPortName, FLT_PORT_NAME);

		InitializeObjectAttributes(&oa,
			&myPortName,
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			sd);
		
		status = FltCreateCommunicationPort(
			g_filterData.pFilter,
			&g_filterData.pIocpServerPort,
			&oa,
			NULL,
			ConnectNotifyCallback,
			DisconnectNotifyCallback,
			MessageNotifyCallback,
			1
		);

		FltFreeSecurityDescriptor(sd);
		if (NT_SUCCESS(status)) {
			status = FltStartFiltering(g_filterData.pFilter);
			if (NT_SUCCESS(status))
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Success Load Filter Driver!\n");
				return status;
			}
		}
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "보안서술자생성실패. status : 0x%X\n", status);
		FltUnregisterFilter(g_filterData.pFilter);
	}
	return status;
}

NTSTATUS OnSimpleFilterUnload(
	FLT_FILTER_UNLOAD_FLAGS flags
)
{
	UNREFERENCED_PARAMETER(flags);
	FltUnregisterFilter(g_filterData.pFilter);

	return STATUS_SUCCESS;
}

NTSTATUS OnSimpleFilterInstanceSetup(
	PCFLT_RELATED_OBJECTS pFltObjects,
	FLT_INSTANCE_SETUP_FLAGS flags,
	DEVICE_TYPE volumeDeviceType,
	FLT_FILESYSTEM_TYPE volumeFilesystemType
)
{
	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(volumeFilesystemType);

	PAGED_CODE();

	ASSERT(pFltObjects->Filter == g_filterData.pFilter);

	if (volumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM)
		return STATUS_FLT_DO_NOT_ATTACH;

	return STATUS_SUCCESS;
}

BOOLEAN IsMyExtension(
	PUNICODE_STRING pExtension
)
{
	const UNICODE_STRING* ext;

	if (pExtension->Length == 0)
		return FALSE;

	ext = g_myExtensions;

	while (ext->Buffer != NULL) {
		if (RtlCompareUnicodeString(pExtension, ext, TRUE) == 0)
			return TRUE;
		ext++;
	}

	return FALSE;
}

FLT_PREOP_CALLBACK_STATUS OnPreCreateFile(
	PFLT_CALLBACK_DATA pData,
	PCFLT_RELATED_OBJECTS pFltObjects,
	PVOID* ppCompletionContext
)
{
	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	BOOLEAN hasExtension;

	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(ppCompletionContext);

	PAGED_CODE();

	status = FltGetFileNameInformation(pData,						// irp_mj_create로 생성하려는 파일 또는 디렉터리의 
		FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,		// 이름 정보(FLT_FILE_NAME_INFORMATION)를 가져옴
		&nameInfo);													// 이 이름 정보는 파싱되지 않은 날 것의 정보임

	if (!NT_SUCCESS(status))
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	FltParseFileNameInformation(nameInfo);							// 이 함수로 이름 정보를 파싱하여 FLT_FILE_NAME_INFORMATION 구조체의 각 멤버 변수에 정리되어 초기화됨

	if (nameInfo->Extension.Buffer != UNICODE_NULL) {
		hasExtension = IsMyExtension(&nameInfo->Extension);

		if (hasExtension == TRUE) {
			FltReleaseFileNameInformation(nameInfo);				// 이름 정보 구조체 해제(release)
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Is text file.. can not open\n");

			FltCancelFileOpen(pFltObjects->Instance, pFltObjects->FileObject);	// 파일 생성 취소
			pData->IoStatus.Status = STATUS_ACCESS_DENIED;		// 파일 접근 차단
			pData->IoStatus.Information = 0;

			return FLT_PREOP_COMPLETE;
		}
	}

	FltReleaseFileNameInformation(nameInfo);
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS ConnectNotifyCallback(
	PFLT_PORT ClientPort,
	PVOID ServerPortCookie,
	PVOID ConnectionContext,
	ULONG SizeOfContext,
	PVOID* ConnectionPortCookie
)
{
	UNREFERENCED_PARAMETER(ClientPort);
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionPortCookie);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "user mode application(%u) connected\n", PtrToUint(PsGetCurrentProcessId()));
	return STATUS_SUCCESS;
}

VOID DisconnectNotifyCallback(
	PVOID ConnectionCookie
)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "user mode application(%u) disconnected\n", PtrToUint(PsGetCurrentProcessId()));
}

NTSTATUS MessageNotifyCallback(
	PVOID PortCookie,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength,
	PULONG ReturnOutputBufferLength
)
{
	UNREFERENCED_PARAMETER(PortCookie);
	if (InputBuffer && InputBufferLength == sizeof(FLT_NOTIFICATION)) {
		PFLT_NOTIFICATION sent = (PFLT_NOTIFICATION)InputBuffer;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "data : %s\n", sent->Contents);
	}

	if (OutputBuffer && OutputBufferLength == sizeof(FLT_REPLY)) {
		PFLT_REPLY rpy = (PFLT_REPLY)OutputBuffer;
		RtlCopyMemory(rpy->str, "Hello user", 11);
		*ReturnOutputBufferLength = 11;
	}
	return STATUS_SUCCESS;
}