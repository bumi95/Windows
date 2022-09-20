#include<Windows.h>
#include<fltUser.h>
#include<stdio.h>
#include"common.h"

int main() {
	HANDLE port_handle;
	HRESULT h_result;

	FLT_NOTIFICATION sent;
	FLT_REPLY rpy;
	DWORD returned_bytes = 0;

	h_result = FilterConnectCommunicationPort(
		FLT_PORT_NAME,
		0,
		NULL,
		0,
		NULL,
		&port_handle
	);
	if (IS_ERROR(h_result)) {
		printf("FilterConnectCommunicationPort failed (HRESULT = 0x%x)", h_result);
		return -1;
	}

	RtlCopyMemory(sent.Contents, "Hello minifilter", 17);
	h_result = FilterSendMessage(
		port_handle,
		&sent,
		sizeof(sent),
		&rpy,
		sizeof(rpy),
		&returned_bytes
	);
	if (IS_ERROR(h_result))
		printf("FilterSendMessage faild (HRESULT = 0x%x)", h_result);

	if (returned_bytes > 0)
		printf("%s\n", rpy.str);
	else
		printf("no reply\n");

	return 0;
}