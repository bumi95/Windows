#pragma once
#include<Windows.h>

#ifndef MY_MAX_PATH
#define MY_MAX_PATH		512
#endif

#define FLT_PORT_NAME	L"\\SimplefdPort"

#define FLT_READ_BUFFER_SIZE	(512*2)

typedef struct _FLT_NOTIFICATION {
	ULONG BytesToScan;
	ULONG Reserved;
	WCHAR filePath[MY_MAX_PATH];
	UCHAR Contents[FLT_READ_BUFFER_SIZE];
}FLT_NOTIFICATION, *PFLT_NOTIFICATION;

typedef struct _FLT_REPLY {
	UCHAR str[FLT_READ_BUFFER_SIZE];
}FLT_REPLY, *PFLT_REPLY;