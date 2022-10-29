#pragma warning(disable:4996)
#include<stdio.h>
#include<stdlib.h>
#include<tchar.h>
#include<locale.h>
#include<Windows.h>

#define STR_LEN		256
#define CMD_TOKEN_NUM	10

TCHAR ERROR_CMD[] = __T("'%s'은(는) 실행할 수 있는 프로그램이 아닙니다. \n");

int CmdProcessing();
int cmdprocessing_2(TCHAR**, int);

TCHAR* StrLower(TCHAR*);

int _tmain(int argc, TCHAR* argv[]) {
	_tsetlocale(LC_ALL, __T("Korean"));

	DWORD isExit;
	while (1) {
		if (argc > 1) {
			isExit = cmdprocessing_2(argv, argc);
			argc = 1;
		}
		else {
			isExit = CmdProcessing();
		}
		if (isExit == TRUE) {
			_fputts(__T("명령어 처리를 종료합니다.\n"), stdout);
			break;
		}
	}
	return 0;
}

TCHAR cmdString[STR_LEN];
TCHAR cmdTokenList[CMD_TOKEN_NUM][STR_LEN];
TCHAR seps[] = __T(" ,\t\n");


int cmdprocessing_2(TCHAR** argv, int len) {
	int token_list = 0;
	for (int i = 1; i < len; i++) {
		_tcscpy(cmdTokenList[token_list++], StrLower(argv[i]));
	}
	if (!_tcscmp(cmdTokenList[0], __T("exit")))
		return TRUE;
	else if (!_tcscmp(cmdTokenList[0], __T("echo"))) {
		for (int i = 1; i < token_list; i++) {
			if (i != token_list - 1) {
				_fputts(cmdTokenList[i], stdout);
				_fputtc(' ', stdout);
			}
			else
				_fputts(cmdTokenList[i], stdout);
		}
		_fputts(__T("\n"), stdout);
	}
	else {
		_tprintf(ERROR_CMD, cmdTokenList[0]);
	}
	return 0;
}
int CmdProcessing() {
	_fputts(__T("Best command prompt>> "), stdout);
	_getts_s(cmdString);
	TCHAR* token = _tcstok(cmdString, seps);
	int tokenNum = 0;
	while (token != NULL) {
		_tcscpy(cmdTokenList[tokenNum++], StrLower(token));
		token = _tcstok(NULL, seps);
	}
	if (!_tcscmp(cmdTokenList[0], __T("exit"))) {
		return TRUE;
	}
	else if (!_tcscmp(cmdTokenList[0], __T("start"))) {
		STARTUPINFO si = { 0, };
		PROCESS_INFORMATION pi;
		si.cb = sizeof(si);

		TCHAR str[STR_LEN] = __T("WindowsSystem.exe ");
		//size_t count = _tcslen(str);
		for (int i = 1; i < tokenNum; i++) {
			_tcscpy(str + _tcslen(str), cmdTokenList[i]);
			_tcscpy(str + _tcslen(str), __T(" "));
			//count += _tcslen(cmdTokenList[i]);
		}
		str[_tcslen(str) - 1] = 0;
		BOOL isrun = CreateProcess(NULL, str, NULL, NULL,
			TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
		if (isrun == TRUE) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else
			_tprintf(ERROR_CMD, cmdTokenList[0]);
	}
	else if (!_tcscmp(cmdTokenList[0], __T("notepad"))) {
		STARTUPINFO si = { 0, };
		PROCESS_INFORMATION pi;
		si.cb = sizeof(si);

		BOOL isrun = CreateProcess(NULL, cmdTokenList[0], NULL, NULL, TRUE,
			0, NULL, NULL, &si, &pi);
		if (isrun == TRUE) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else
			_tprintf(ERROR_CMD, cmdTokenList[0]);
	}
	else if (!_tcscmp(cmdTokenList[0], __T("echo"))) {
		for (int i = 1; i < tokenNum; i++) {
			_fputts(cmdTokenList[i], stdout);
			_fputtc(__T(' '), stdout);
		}
		_fputts(__T("\n"), stdout);
	}
	else {
		_tprintf(ERROR_CMD, cmdTokenList[0]);
	}
	return 0;
}

TCHAR* StrLower(TCHAR* pStr) {
	TCHAR* ret = pStr;

	while (*pStr) {
		if (_istupper(*pStr))
			*pStr = _totlower(*pStr);
		pStr++;
	}

	return ret;
}