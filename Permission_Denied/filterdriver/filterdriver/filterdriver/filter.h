#pragma once
#include<fltKernel.h>

#define SIMPLE_TAG		'SIMT'

typedef struct _FILTER_DATA {
	PFLT_FILTER pFilter;
	PFLT_PORT	pIocpServerPort;
	PFLT_PORT	pIocpClientPort;
	PEPROCESS	pUserProcess;
}FILTER_DATA, *PFILTER_DATA;

typedef struct _FILTER_STREAM_HANDLE_CONTEXT {
	ULONG	reserved1;
}FILTER_STREAM_HANDLE_CONTEXT, *PFILTER_STREAM_HANDLE_CONTEXT;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS OnSimpleFilterUnload(FLT_FILTER_UNLOAD_FLAGS flags);
//NTSTATUS OnSimpleFilterInstanceSetup(PCFLT_RELATED_OBJECTS pFltObjects, FLT_INSTANCE_SETUP_FLAGS flags, DEVICE_TYPE volumeDeviceType, FLT_FILESYSTEM_TYPE volumeFilesystemType);
BOOLEAN IsMyExtension(PUNICODE_STRING pExtension);
FLT_PREOP_CALLBACK_STATUS OnPreCreateFile(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFltObjects, PVOID* ppCompletionContext);
//FLT_POSTOP_CALLBACK_STATUS OnPostReadFile(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pFltObjects, PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags);
NTSTATUS ConnectNotifyCallback(PFLT_PORT ClientPort, PVOID ServerPortCookie, PVOID ConnectionContext, ULONG SizeOfContext, PVOID* ConnectionPortCookie);
VOID DisconnectNotifyCallback(PVOID ConnectionCookie);
NTSTATUS MessageNotifyCallback(PVOID PortCookie, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PULONG ReturnOutputBufferLength);