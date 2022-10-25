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
TCHAR* StrLower(TCHAR*);

int _tmain(int argc, TCHAR* argv[]) {
	_tsetlocale(LC_ALL, __T("Korean"));

	DWORD isExit;
	while (1) {
		isExit = CmdProcessing();
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