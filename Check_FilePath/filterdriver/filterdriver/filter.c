#include"filter.h"
#include"common.h"

FILTER_DATA F_data = { 0 };

const UNICODE_STRING myExtension[] = {
	RTL_CONSTANT_STRING(L"txt"),
	{0, 0, NULL}
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MyPostReadFile)
#endif

FLT_OPERATION_REGISTRATION callbacks[] = {
	{
		IRP_MJ_READ,
		0,
		NULL,
		MyPostReadFile
	},
	{
		IRP_MJ_OPERATION_END
	}
};

FLT_CONTEXT_REGISTRATION context_Reg[] = {
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

FLT_REGISTRATION filter_Reg = {
	sizeof(FLT_REGISTRATION),
	FLT_REGISTRATION_VERSION,
	0,
	context_Reg,
	callbacks,
	MyFilterUnload,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

BOOLEAN IsMyExtension(
	PUNICODE_STRING pExtension
)
{
	const UNICODE_STRING* ext;

	if (pExtension->Length == 0)
		return FALSE;

	ext = myExtension;
	while (ext->Buffer != NULL) {
		if (RtlCompareUnicodeString(pExtension, ext, TRUE) == 0)
			return TRUE;
		ext++;
	}

	return FALSE;
}

NTSTATUS DriverEntry(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
)
{
	NTSTATUS status;
	PSECURITY_DESCRIPTOR sd;

	UNREFERENCED_PARAMETER(RegistryPath);

	status = STATUS_UNSUCCESSFUL;

	status = FltRegisterFilter(
		DriverObject,
		&filter_Reg,
		&F_data.pFilter
	);
	if (!NT_SUCCESS(status))
		return status;

	sd = NULL;
	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
	if (NT_SUCCESS(status)) {
		OBJECT_ATTRIBUTES oa = { 0 };
		UNICODE_STRING portName = { 0 };
		RtlInitUnicodeString(&portName, FLT_PORT_NAME);

		InitializeObjectAttributes(
			&oa,
			&portName,
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			sd
		);

		status = FltCreateCommunicationPort(
			F_data.pFilter,
			&F_data.pIocpServerPort,
			&oa,
			NULL,
			ConnectNotifyCallback,
			DisconnectNotifyCallback,
			NULL,
			1
		);

		FltFreeSecurityDescriptor(sd);
		if (NT_SUCCESS(status)) {
			status = FltStartFiltering(F_data.pFilter);
			if (NT_SUCCESS(status)) {
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Success Load Filter Driver!\n");
				return status;
			}
		}
	}
	else {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, " 보안서술자생성실패. status : 0x%X\n", status);
		FltUnregisterFilter(F_data.pFilter);
	}
	DriverObject->DriverUnload = DriverUnload;
	return status;
}

VOID DriverUnload(
	PDRIVER_OBJECT DriverObject
)
{
	UNREFERENCED_PARAMETER(DriverObject);
	if (F_data.pFilter)
		FltCloseCommunicationPort(F_data.pIocpServerPort);
	if (F_data.pFilter)
		FltUnregisterFilter(F_data.pFilter);
}

NTSTATUS MyFilterUnload(
	FLT_FILTER_UNLOAD_FLAGS flags
)
{
	UNREFERENCED_PARAMETER(flags);
	FltUnregisterFilter(F_data.pFilter);

	return STATUS_SUCCESS;
}

FLT_POSTOP_CALLBACK_STATUS MyPostReadFile(
	PFLT_CALLBACK_DATA pData,
	PCFLT_RELATED_OBJECTS pFltObjects,
	PVOID CompletionContext,
	FLT_POST_OPERATION_FLAGS Flags
)
{
	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	FLT_NOTIFICATION send;
	BOOLEAN hasExtension;

	UNREFERENCED_PARAMETER(pFltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	status = FltGetFileNameInformation(
		pData,
		FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
		&nameInfo
	);

	if (!NT_SUCCESS(status))
		return FLT_POSTOP_FINISHED_PROCESSING;

	FltParseFileNameInformation(nameInfo);
	if (nameInfo->Extension.Buffer != UNICODE_NULL) {
		hasExtension = IsMyExtension(&nameInfo->Extension);
		if (hasExtension == TRUE) {
			wcscpy_s(send.filepath, nameInfo->Name.Length, nameInfo->Name.Buffer);
			FltReleaseFileNameInformation(nameInfo);

			status = FltSendMessage(
				F_data.pFilter,
				&F_data.pIocpClientPort,
				&send,
				sizeof(FLT_NOTIFICATION),
				NULL,
				0,
				NULL
			);

			if (!NT_SUCCESS(status))
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "send failed\n");
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

	F_data.pIocpClientPort = ClientPort;
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