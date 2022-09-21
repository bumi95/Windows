#include "filter.h"
#include "common.h"

FILTER_DATA g_filterData = { 0 };

const UNICODE_STRING g_myExtensions[] = {
	RTL_CONSTANT_STRING(L"txt"),
	{0, 0, NULL}
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

FLT_OPERATION_REGISTRATION g_callbacks[] = {
	{
		IRP_MJ_CREATE,
		0,
		OnPreCreateFile,
		NULL
	},
#if 0
	{
		IRP_MJ_READ,
		0,
		NULL,
		OnPostReadFile
	},
#endif
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
	NULL,
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
			NULL,
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
	FltCloseCommunicationPort(g_filterData.pIocpServerPort);
	FltUnregisterFilter(g_filterData.pFilter);

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
	FLT_NOTIFICATION send;

	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(ppCompletionContext);

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
			RtlCopyMemory(send.Contents, "textFile Create Denied", 23);
			status = FltSendMessage(
				g_filterData.pFilter,
				&g_filterData.pIocpClientPort,
				&send,
				sizeof(FLT_NOTIFICATION),
				NULL,
				0,
				NULL
			);

			if (!NT_SUCCESS(status)) {
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "send failed\n");
			}
			
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

	g_filterData.pIocpClientPort = ClientPort;

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