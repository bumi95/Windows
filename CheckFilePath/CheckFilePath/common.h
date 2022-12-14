#pragma once

#define MY_MAX_PATH		512
#define FLT_PORT_NAME	L"\\SimplefdPort"

typedef struct _FLT_NOTIFICATION {
	WCHAR filepath[MY_MAX_PATH];
}FLT_NOTIFICATION, * PFLT_NOTIFICATION;

typedef struct _SCANNER_MESSAGE {
	FILTER_MESSAGE_HEADER msgHeader;
	FLT_NOTIFICATION msg;
}SCANNER_MESSAGE, * PSCANNER_MESSAGE;