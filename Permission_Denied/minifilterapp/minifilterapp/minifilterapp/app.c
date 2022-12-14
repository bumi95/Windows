#include<Windows.h>
#include<fltUser.h>
#include<stdio.h>
#include"common.h"

int main() {
	HANDLE port_handle;
	HRESULT h_result;
	DWORD status;

	SCANNER_MESSAGE s_msg;
	DWORD returned_bytes = 0;
	OVERLAPPED ov = { 0 };

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
	
	while (TRUE) {
		ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!ov.hEvent) {
			printf("create event error = %u\n", GetLastError());
			break;
		}

		h_result = FilterGetMessage(port_handle, &s_msg.msgHeader, sizeof(s_msg), &ov);

		if (h_result != HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
			printf("filter get message error = %x\n", h_result);
			break;
		}

		status = WaitForSingleObject(ov.hEvent, INFINITE);
		if (GetOverlappedResult(port_handle, &ov, &returned_bytes, FALSE)) {
			printf("%s\n", s_msg.msg.Contents);
		}
		if (ov.hEvent != NULL)
			CloseHandle(ov.hEvent);
	}

	return 0;
}