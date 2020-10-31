/*
 * (C) 2020 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* TERATERM.EXE, log routines */
#include <stdio.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <process.h>
#include <windows.h>
#include <htmlhelp.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "ftdlg.h"
#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "dlglib.h"
#include "vtterm.h"
#include "ftlib.h"
#include "buffer.h"
#include "helpid.h"
#include "layer_for_unicode.h"
#include "layer_for_unicode_crt.h"
#include "codeconv.h"
#include "asprintf.h"

#include "filesys_log_res.h"
#include "filesys_log.h"

/*
   Line Head flag for timestamping
   2007.05.24 Gentaro
*/
enum enumLineEnd {
	Line_Other = 0,
	Line_LineHead = 1,
	Line_FileHead = 2,
};

typedef struct {
	wchar_t *FullName;

	HANDLE FileHandle;
	LONG FileSize, ByteCount;

	DWORD StartTime;

	enum enumLineEnd eLineEnd;

	// log rotate
	int RotateMode;  //  enum rotate_mode RotateMode;
	LONG RotateSize;
	int RotateStep;

	HANDLE LogThread;
	DWORD LogThreadId;
	HANDLE LogThreadEvent;

	BOOL IsPause;

	PFileTransDlg FLogDlg;

	LogCode_t log_code;
	BOOL bom;

} TFileVar_;
typedef TFileVar_ *PFileVar_;

#define PFileVar PFileVar_
#define TFileVar TFileVar_

static PFileVar LogVar = NULL;

static BOOL FileLog = FALSE;
static BOOL BinLog = FALSE;

static PCHAR cv_LogBuf;
static int cv_LogPtr, cv_LStart, cv_LCount;
static PCHAR cv_BinBuf;
static int cv_BinPtr, cv_BStart, cv_BCount;
static int cv_BinSkip;

// 遅延書き込み用スレッドのメッセージ
#define WM_DPC_LOGTHREAD_SEND (WM_APP + 1)

static void FileTransEnd_(void);
static void Log1Bin(BYTE b);
static void LogBinSkip(int add);
static BOOL CreateLogBuf(void);
static BOOL CreateBinBuf(void);
void LogPut1(BYTE b);
static void OutputStr(const wchar_t *str);
static void LogToFile(void);
static void FLogOutputBOM(void);

static BOOL OpenFTDlg_(PFileVar fv)
{
	PFileTransDlg FTDlg = new CFileTransDlg();
	if (FTDlg == NULL) {
		return FALSE;
	}

	wchar_t *DlgCaption;
	wchar_t uimsg[MAX_UIMSG];
#define TitLogW      L"Log"
	get_lang_msgW("FILEDLG_TRANS_TITLE_LOG", uimsg, _countof(uimsg), TitLogW, ts.UILanguageFile);
	aswprintf(&DlgCaption, L"Tera Term: %s", uimsg);

	CFileTransDlgInfo info;
	info.UILanguageFile = ts.UILanguageFile;
	info.OpId = OpLog;
	info.DlgCaption = DlgCaption;
	info.FileName = NULL;
	info.FullName = fv->FullName;
	info.HideDialog = ts.LogHideDialog ? TRUE : FALSE;
	info.HMainWin = HVTWin;
	FTDlg->Create(hInst, &info);
	FTDlg->RefreshNum(0, fv->FileSize, fv->ByteCount);

	fv->FLogDlg = FTDlg;

	free(DlgCaption);
	return TRUE;
}

/**
 *	ファイル名文字列の置き換え
 *		&h	ホスト名に置換 (2007.5.14)
 *		&p	TCPポート番号に置換 (2009.6.12)
 *		&u	ログオン中のユーザ名
 */
static void ConvertLogname(char *c, int destlen)
{
	char buf[MAXPATHLEN], buf2[MAXPATHLEN], *p = c;
	char tmphost[1024];
	char tmpuser[256+1];
	DWORD len_user = sizeof(tmpuser);

	memset(buf, 0, sizeof(buf));

	while(*p != '\0') {
		if (*p == '&' && *(p+1) != '\0') {
			switch (*(p+1)) {
			  case 'h':
				if (cv.Open) {
					if (cv.PortType == IdTCPIP) {
						// ホスト名がIPv6アドレスだと、ファイル名に使用できない文字が入るため、
						// 余計な文字は削除する。
						// (2013.3.9 yutaka)
						strncpy_s(tmphost, sizeof(tmphost), ts.HostName, _TRUNCATE);
						//strncpy_s(tmphost, sizeof(tmphost), "2001:0db8:bd05:01d2:288a:1fc0:0001:10ee", _TRUNCATE);
						replaceInvalidFileNameChar(tmphost, '_');
						strncat_s(buf,sizeof(buf), tmphost, _TRUNCATE);
					}
					else if (cv.PortType == IdSerial) {
						strncpy_s(buf2,sizeof(buf2),buf,_TRUNCATE);
						_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%sCOM%d", buf2, ts.ComPort);
					}
				}
				break;
			  case 'p':
				if (cv.Open) {
					if (cv.PortType == IdTCPIP) {
						char port[6];
						_snprintf_s(port, sizeof(port), _TRUNCATE, "%d", ts.TCPPort);
						strncat_s(buf,sizeof(buf),port,_TRUNCATE);
					}
				}
				break;
			  case 'u':
				if (GetUserName(tmpuser, &len_user) != 0) {
					strncat_s(buf,sizeof(buf),tmpuser,_TRUNCATE);
				}
				break;
			  default:
				strncpy_s(buf2,sizeof(buf2),p,2);
				strncat_s(buf,sizeof(buf),buf2,_TRUNCATE);
			}
			p++;
		}
		else {
			strncpy_s(buf2,sizeof(buf2),p,1);
			strncat_s(buf,sizeof(buf),buf2,_TRUNCATE);
		}
		p++;
	}
	strncpy_s(c, destlen, buf, _TRUNCATE);
}

static void FixLogOption(void)
{
	if (ts.LogBinary) {
		ts.LogTypePlainText = false;
		ts.LogTimestamp = false;
	}
}


// スレッドの終了とファイルのクローズ
static void CloseFileSync(PFileVar ptr)
{
	BOOL ret;

	if (ptr->FileHandle == INVALID_HANDLE_VALUE) {
		return;
	}

	if (ptr->LogThread != INVALID_HANDLE_VALUE) {
		// スレッドの終了待ち
		ret = PostThreadMessage(ptr->LogThreadId, WM_QUIT, 0, 0);
		if (ret != 0) {
			// スレッドキューにエンキューできた場合のみ待ち合わせを行う。
			WaitForSingleObject(ptr->LogThread, INFINITE);
		}
		else {
			//DWORD code = GetLastError();
		}
		CloseHandle(ptr->LogThread);
		ptr->LogThread = INVALID_HANDLE_VALUE;
	}
	CloseHandle(ptr->FileHandle);
	ptr->FileHandle = INVALID_HANDLE_VALUE;
}

// 遅延書き込み用スレッド
static unsigned _stdcall DeferredLogWriteThread(void *arg)
{
	MSG msg;
	PFileVar fv = (PFileVar)arg;
	PCHAR buf;
	DWORD buflen;
	DWORD wrote;

	PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

	// スレッドキューの作成が終わったことをスレッド生成元へ通知する。
	if (fv->LogThreadEvent != NULL) {
		SetEvent(fv->LogThreadEvent);
	}

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		switch (msg.message) {
			case WM_DPC_LOGTHREAD_SEND:
				buf = (PCHAR)msg.wParam;
				buflen = (DWORD)msg.lParam;
				WriteFile(LogVar->FileHandle, buf, buflen, &wrote, NULL);
				free(buf);   // ここでメモリ解放
				break;

			case WM_QUIT:
				goto end;
				break;
		}
	}

end:
	_endthreadex(0);
	return (0);
}

// 遅延書き込み用スレッドを起こす。
// (2013.4.19 yutaka)
// DeferredLogWriteThread スレッドが起床して、スレッドキューが作成されるより前に、
// ログファイルのクローズ(CloseFileSync)が行われると、エンキューが失敗し、デッドロック
// するという問題を修正した。
// スレッド間の同期を行うため、名前なしイベントオブジェクトを使って、スレッドキューの
// 作成まで待ち合わせするようにした。名前付きイベントオブジェクトを使う場合は、
// システム(Windows OS)上でユニークな名前にする必要がある。
// (2016.9.23 yutaka)
static void StartThread(PFileVar fv)
{
	unsigned tid;
	fv->LogThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	fv->LogThread = (HANDLE)_beginthreadex(NULL, 0, DeferredLogWriteThread, fv, 0, &tid);
	fv->LogThreadId = tid;
	if (fv->LogThreadEvent != NULL) {
		WaitForSingleObject(fv->LogThreadEvent, INFINITE);
		CloseHandle(fv->LogThreadEvent);
	}
}

/**
 *	ダイアログの内容を ts に書き戻し
 *
 *	TODO
 *		ダイアログで設定した値は一時的なもので
 *		設定を上書きするのは良くないのではないだろうか?
 */
static void SetLogFlags(HWND Dialog)
{
	WORD BinFlag, val;

	GetRB(Dialog, &BinFlag, IDC_FOPTBIN, IDC_FOPTBIN);
	ts.LogBinary = BinFlag;

	GetRB(Dialog, &val, IDC_APPEND, IDC_APPEND);
	ts.Append = val;

	if (!BinFlag) {
		GetRB(Dialog, &val, IDC_PLAINTEXT, IDC_PLAINTEXT);
		ts.LogTypePlainText = val;

		GetRB(Dialog, &val, IDC_TIMESTAMP, IDC_TIMESTAMP);
		ts.LogTimestamp = val;
	}

	GetRB(Dialog, &val, IDC_HIDEDIALOG, IDC_HIDEDIALOG);
	ts.LogHideDialog = val;

	GetRB(Dialog, &val, IDC_ALLBUFF_INFIRST, IDC_ALLBUFF_INFIRST);
	ts.LogAllBuffIncludedInFirst = val;

	ts.LogTimestampType = (WORD)(GetCurSel(Dialog, IDC_TIMESTAMPTYPE) - 1);
}

/**
 *	ログファイルチェック
 *
 *	@param[in]	filename
 *	@param[out]	exist	TURE/FALSE
 *	@param[out]	bom		0	no BOM (or file not exist)
 *						1	UTF-8
 *						2	UTF-16LE
 *						3	UTF-16BE
 */
static void CheckLogFile(const wchar_t *filename, BOOL *exist, int *bom)
{
	*exist = FALSE;
	*bom = 0;

	// ファイルが存在する?
	DWORD logdir = _GetFileAttributesW(filename);
	if ((logdir != INVALID_FILE_ATTRIBUTES) && ((logdir & FILE_ATTRIBUTE_DIRECTORY) == 0)) {
		// ファイルがあった
		*exist = TRUE;

		// BOM有り/無しチェック
		FILE *fp = __wfopen(filename, L"rb");
		if (fp != NULL) {
			unsigned char tmp[4];
			size_t l = fread(tmp, 1, sizeof(tmp), fp);
			fclose(fp);
			if (l < 2) {
				*bom = 0;
			} else if (l >= 2 && tmp[0] == 0xff && tmp[1] == 0xfe) {
				// UTF-16LE
				*bom = 2;
			} else if (l >= 2 && tmp[0] == 0xfe && tmp[1] == 0xff) {
				// UTF-16BE
				*bom = 3;
			} else if (l >= 3 && tmp[0] == 0xef && tmp[1] == 0xbb && tmp[2] == 0xbf) {
				// UTF-8
				*bom = 1;
			} else {
				*bom = 0;
			}
		}
	}
}

typedef struct {
	FLogDlgInfo_t *info;
	// work
	BOOL file_exist;
	int current_bom;
	TTTSet *pts;
} LogDlgWork_t;

static void ArrangeControls(HWND Dialog, LogDlgWork_t *work)
{
	if (work->file_exist) {
		EnableWindow(GetDlgItem(Dialog, IDC_APPEND), TRUE);
		if (work->pts->Append > 0) {
			CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_APPEND);
		}
	}
	else {
		// ファイルがない -> 新規
		EnableWindow(GetDlgItem(Dialog, IDC_APPEND), FALSE);
		CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_NEW_OVERWRITE);
	}

	if (work->file_exist && IsDlgButtonChecked(Dialog, IDC_APPEND) == BST_CHECKED) {
		// ファイルが存在する && append
		int bom = work->current_bom;
		if (bom != 0) {
			// BOM有り
			CheckDlgButton(Dialog, IDC_BOM, BST_CHECKED);
			int cur =
				bom == 1 ? 0 :
				bom == 2 ? 1 :
				bom == 3 ? 2 : 0;
			SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, cur, 0);
		}
		else {
			// BOMなし
			CheckDlgButton(Dialog, IDC_BOM, BST_UNCHECKED);
			SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, 0, 0);
		}
		if (IsDlgButtonChecked(Dialog, IDC_FOPTTEXT) == BST_CHECKED) {
			EnableWindow(GetDlgItem(Dialog, IDC_BOM), FALSE);
			if (bom != 0) {
				// BOM有り
				EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), FALSE);
			}
			else {
				// BOMなし
				EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), TRUE);
			}
		}
	}
	else {
		// ファイルがない 又は appendではない(上書き)
		CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_NEW_OVERWRITE);
		CheckDlgButton(Dialog, IDC_BOM, BST_CHECKED);
		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, 0, 0);
		if (IsDlgButtonChecked(Dialog, IDC_FOPTTEXT) == BST_CHECKED) {
			EnableWindow(GetDlgItem(Dialog, IDC_BOM), TRUE);
		}
	}
}

static void CheckLogFile(HWND Dialog, const wchar_t *filename, LogDlgWork_t *work)
{
	BOOL exist;
	int bom;
	CheckLogFile(filename, &exist, &bom);
	work->file_exist = exist;
	work->current_bom = bom;
	ArrangeControls(Dialog, work);
}

static INT_PTR CALLBACK LogFnHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_TABSHEET_TITLE_LOG" },
		{ IDC_FOPTBIN, "DLG_FOPT_BINARY" },
		{ IDC_APPEND, "DLG_FOPT_APPEND" },
		{ IDC_PLAINTEXT, "DLG_FOPT_PLAIN" },
		{ IDC_HIDEDIALOG, "DLG_FOPT_HIDEDIALOG" },
		{ IDC_ALLBUFF_INFIRST, "DLG_FOPT_ALLBUFFINFIRST" },
		{ IDC_TIMESTAMP, "DLG_FOPT_TIMESTAMP" },
	};
	static const I18nTextInfo timestamp_list[] = {
		{ "DLG_FOPT_TIMESTAMP_LOCAL", L"Local Time" },
		{ "DLG_FOPT_TIMESTAMP_UTC", L"UTC" },
		{ "DLG_FOPT_TIMESTAMP_ELAPSED_LOGGING", L"Elapsed Time (Logging)" },
		{ "DLG_FOPT_TIMESTAMP_ELAPSED_CONNECTION", L"Elapsed Time (Connection)" },
	};
	LogDlgWork_t *work = (LogDlgWork_t *)GetWindowLongPtr(Dialog, DWLP_USER);

	if (Message == 	RegisterWindowMessage(HELPMSGSTRING)) {
		// コモンダイアログからのヘルプメッセージを付け替える
		Message = WM_COMMAND;
		wParam = IDHELP;
	}
	switch (Message) {
	case WM_INITDIALOG: {
		work = (LogDlgWork_t *)lParam;
		TTTSet *pts = work->pts;
		const char *UILanguageFile = pts->UILanguageFile;
		SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)work);
		::DragAcceptFiles(Dialog, TRUE);

		SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);
		SetI18nList("Tera Term", Dialog, IDC_TIMESTAMPTYPE, timestamp_list, _countof(timestamp_list),
					UILanguageFile, 0);

		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)"UTF-8");
		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)"UTF-16LE");
		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)"UTF-16BE");
		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, 0, 0);

		_SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, work->info->filename);
		work->info->filename = NULL;

		// Binary/Text チェックボックス
		if (pts->LogBinary) {
			CheckRadioButton(Dialog, IDC_FOPTBIN, IDC_FOPTTEXT, IDC_FOPTBIN);
		}
		else {
			CheckRadioButton(Dialog, IDC_FOPTBIN, IDC_FOPTTEXT, IDC_FOPTTEXT);
		}

		// Plain Text チェックボックス
		if (pts->LogBinary) {
			// Binaryフラグが有効なときはチェックできない
			DisableDlgItem(Dialog, IDC_PLAINTEXT, IDC_PLAINTEXT);
		}
		else if (pts->LogTypePlainText) {
			SetRB(Dialog, 1, IDC_PLAINTEXT, IDC_PLAINTEXT);
		}

		// Hide dialogチェックボックス (2008.1.30 maya)
		if (pts->LogHideDialog) {
			SetRB(Dialog, 1, IDC_HIDEDIALOG, IDC_HIDEDIALOG);
		}

		// Include screen bufferチェックボックス (2013.9.29 yutaka)
		if (pts->LogAllBuffIncludedInFirst) {
			SetRB(Dialog, 1, IDC_ALLBUFF_INFIRST, IDC_ALLBUFF_INFIRST);
		}

		// timestampチェックボックス (2006.7.23 maya)
		if (pts->LogBinary) {
			// Binaryフラグが有効なときはチェックできない
			DisableDlgItem(Dialog, IDC_TIMESTAMP, IDC_TIMESTAMP);
		}
		else if (pts->LogTimestamp) {
			SetRB(Dialog, 1, IDC_TIMESTAMP, IDC_TIMESTAMP);
		}

		// timestamp 種別
		int tstype = pts->LogTimestampType == TIMESTAMP_LOCAL ? 0 :
				pts->LogTimestampType == TIMESTAMP_UTC ? 1 :
				pts->LogTimestampType == TIMESTAMP_ELAPSED_LOGSTART ? 2 :
				pts->LogTimestampType == TIMESTAMP_ELAPSED_CONNECTED ? 3 : 0;
		SendDlgItemMessage(Dialog, IDC_TIMESTAMPTYPE, CB_SETCURSEL, tstype, 0);
		if (pts->LogBinary || !pts->LogTimestamp) {
			DisableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
		}

		CenterWindow(Dialog, GetParent(Dialog));

		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			wchar_t filename[MAX_PATH];
			_GetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, filename, _countof(filename));
			work->info->filename = _wcsdup(filename);
			work->info->append = IsDlgButtonChecked(Dialog, IDC_APPEND) == BST_CHECKED;
			work->info->bom = IsDlgButtonChecked(Dialog, IDC_BOM) == BST_CHECKED;
			work->info->code = (LogCode_t)SendDlgItemMessageA(Dialog, IDC_TEXTCODING_DROPDOWN, CB_GETCURSEL, 0, 0);
			SetLogFlags(Dialog);
			EndDialog(Dialog, IDOK);
			break;
		}
		case IDCANCEL:
			EndDialog(Dialog, IDCANCEL);
			break;
		case IDHELP:
			OpenHelp(HH_HELP_CONTEXT, HlpFileLog, work->pts->UILanguageFile);
			break;
		case IDC_FOPT_FILENAME_BUTTON: {
			/* save current dir */
			const char *UILanguageFile = work->pts->UILanguageFile;
			wchar_t curdir[MAXPATHLEN];
			_GetCurrentDirectoryW(_countof(curdir), curdir);

			wchar_t fname[MAX_PATH];
			GetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, fname, _countof(fname));

			wchar_t *FNFilter = GetCommonDialogFilterW(NULL, UILanguageFile);

			wchar_t caption[MAX_PATH];
			wchar_t uimsg[MAX_UIMSG];
#define TitLogW      L"Log"
			get_lang_msgW("FILEDLG_TRANS_TITLE_LOG", uimsg, _countof(uimsg), TitLogW, UILanguageFile);
			wcsncpy_s(caption, _countof(caption), L"Tera Term: ", _TRUNCATE);
			wcsncat_s(caption, _countof(caption), uimsg, _TRUNCATE);

			OPENFILENAMEW ofn = {};
			ofn.lStructSize = get_OPENFILENAME_SIZEW();
			//ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
			ofn.Flags |= OFN_EXPLORER | OFN_ENABLESIZING;
			ofn.Flags |= OFN_SHOWHELP;
			ofn.Flags |= OFN_NOCHANGEDIR;		// うまく動作しない環境もあるようだ
			ofn.hwndOwner = Dialog;
			ofn.lpstrFilter = FNFilter;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = fname;
			ofn.nMaxFile = _countof(fname);
			ofn.lpstrTitle = caption;
			BOOL Ok = _GetSaveFileNameW(&ofn);
			free(FNFilter);
			if (Ok) {
				SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, fname);
			}

			/* restore dir */
			_SetCurrentDirectoryW(curdir);

			break;
		}
		case IDC_FOPTBIN:
			EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), FALSE);
			EnableWindow(GetDlgItem(Dialog, IDC_BOM), FALSE);
			DisableDlgItem(Dialog, IDC_PLAINTEXT, IDC_TIMESTAMP);
			DisableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
			break;
		case IDC_FOPTTEXT:
			ArrangeControls(Dialog, work);
			EnableDlgItem(Dialog, IDC_PLAINTEXT, IDC_TIMESTAMP);
			// FALLTHROUGH -- BinFlag が off の時は Timestamp 種別の有効/無効を設定する
		case IDC_TIMESTAMP:
			if (IsDlgButtonChecked(Dialog, IDC_TIMESTAMP) == BST_CHECKED) {
				EnableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
			}
			else {
				DisableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
			}
			break;
		case IDC_FOPT_FILENAME_EDIT:
			if (HIWORD(wParam) == EN_CHANGE){
				wchar_t filename[MAX_PATH];
				GetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, filename, _countof(filename));
				CheckLogFile(Dialog, filename, work);
			}
			break;
		case IDC_NEW_OVERWRITE:
			if (IsDlgButtonChecked(Dialog, IDC_FOPTTEXT) == BST_CHECKED) {
				EnableWindow(GetDlgItem(Dialog, IDC_BOM), TRUE);
				EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), TRUE);
				CheckDlgButton(Dialog, IDC_BOM, BST_CHECKED);
				SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, 0, 0);
			}
			break;
		case IDC_APPEND:
			ArrangeControls(Dialog, work);
			break;
		}
		break;
	case WM_DROPFILES: {
		// 複数ドロップされても最初の1つだけを扱う
		HDROP hDrop = (HDROP)wParam;
		const UINT len = _DragQueryFileW(hDrop, 0, NULL, 0);
		if (len == 0) {
			DragFinish(hDrop);
			return TRUE;
		}
		wchar_t *filename = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
		_DragQueryFileW(hDrop, 0, filename, len + 1);
		filename[len] = '\0';
		CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_APPEND);
		_SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, filename);
		SendDlgItemMessage(Dialog, IDC_FOPT_FILENAME_EDIT, EM_SETSEL, len, len);
		free(filename);
		DragFinish(hDrop);
		return TRUE;
	}
	}
	return FALSE;
}

static void OpenLogFile(PFileVar fv)
{
	int dwShareMode = FILE_SHARE_READ;
	if (!ts.LogLockExclusive) {
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}
	LogVar->FileHandle = _CreateFileW(LogVar->FullName, GENERIC_WRITE, dwShareMode, NULL,
									  OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

static BOOL LogStart(const wchar_t *fname)
{
	PFileVar fv = LogVar;

	fv->FullName = _wcsdup(fname);
	FixLogOption();

	if (ts.LogBinary > 0)
	{
		BinLog = TRUE;
		FileLog = FALSE;
		if (! CreateBinBuf())
		{
			return FALSE;
		}
	}
	else {
		BinLog = FALSE;
		FileLog = TRUE;
		if (! CreateLogBuf())
		{
			return FALSE;
		}
	}
	cv_LStart = cv_LogPtr;
	cv_LCount = 0;

	OpenLogFile(fv);
	if (LogVar->FileHandle == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	/* 2007.05.24 Gentaro */
	fv->eLineEnd = Line_LineHead;
	if (ts.Append > 0)
	{
		SetFilePointer(LogVar->FileHandle, 0, NULL, FILE_END);
		/* 2007.05.24 Gentaro
		   If log file already exists,
		   a newline is inserted before the first timestamp.
		*/
		fv->eLineEnd = Line_FileHead;
	}
	else {
		// 追記ではない(新規)場合は BOM を出力する
		if (fv->bom) {
			FLogOutputBOM();
		}
	}

	// Log rotate configuration
	LogVar->RotateMode = ts.LogRotate;
	LogVar->RotateSize = ts.LogRotateSize;
	LogVar->RotateStep = ts.LogRotateStep;

	// Log rotateが有効の場合、初期ファイルサイズを設定する。
	// 最初のファイルが設定したサイズでローテートしない問題の修正。
	// (2016.4.9 yutaka)
	if (LogVar->RotateMode != ROTATE_NONE) {
		DWORD size = GetFileSize(LogVar->FileHandle, NULL);
		if (size == -1) {
			return FALSE;
		}
		LogVar->ByteCount = size;
	}
	else {
		LogVar->ByteCount = 0;
	}

	if (! OpenFTDlg_(LogVar)) {
		return FALSE;
	}

	LogVar->IsPause = FALSE;
	LogVar->StartTime = GetTickCount();

	if (ts.DeferredLogWriteMode) {
		StartThread(LogVar);
	}

	if (FileLog) {
		cv.Log1Byte = LogPut1;
	}
	if (BinLog) {
		cv.Log1Bin = Log1Bin;
		cv.LogBinSkip = LogBinSkip;
	}

	return TRUE;
}

/**
 * 現在バッファにあるデータをすべてログに書き出す
 * (2013.9.29 yutaka)
 *
 *	TODO
 *		1行の長さ
 */
void FLogOutputAllBuffer(void)
{
	DWORD ofs;
	int size;
	wchar_t buf[512];
	for (ofs = 0 ;  ; ofs++ ) {
		// 1つの行を取得する。文字だけなので、エスケープシーケンスは含まれない。
		size = BuffGetAnyLineDataW(ofs, buf, _countof(buf));
		if (size == -1)
			break;

		OutputStr(buf);
		OutputStr(L"\r\n");
		LogToFile();
	}
}

/**
 * ログへ1byte書き込み
 *		バッファへ書き込まれる
 *		実際の書き込みは LogToFile() で行われる
 */
void LogPut1(BYTE b)
{
	cv_LogBuf[cv_LogPtr] = b;
	cv_LogPtr++;
	if (cv_LogPtr>=InBuffSize)
		cv_LogPtr = cv_LogPtr-InBuffSize;

	if (FileLog)
	{
		if (cv_LCount>=InBuffSize)
		{
			cv_LCount = InBuffSize;
			cv_LStart = cv_LogPtr;
		}
		else
			cv_LCount++;
	}
	else
		cv_LCount = 0;
}

static BOOL Get1(PCHAR Buf, int *Start, int *Count, PBYTE b)
{
	if (*Count<=0) return FALSE;
	*b = Buf[*Start];
	(*Start)++;
	if (*Start>=InBuffSize)
		*Start = *Start-InBuffSize;
	(*Count)--;
	return TRUE;
}



static CRITICAL_SECTION g_filelog_lock;   /* ロック用変数 */

void logfile_lock_initialize(void)
{
	InitializeCriticalSection(&g_filelog_lock);
}

static inline void logfile_lock(void)
{
	EnterCriticalSection(&g_filelog_lock);
}

static inline void logfile_unlock(void)
{
	LeaveCriticalSection(&g_filelog_lock);
}

// ログをローテートする。
// (2013.3.21 yutaka)
static void LogRotate(void)
{
	int loopmax = 10000;  // XXX
	int i, k;

	if (LogVar->RotateMode == ROTATE_NONE)
		return;

	if (LogVar->RotateMode == ROTATE_SIZE) {
		if (LogVar->ByteCount <= LogVar->RotateSize)
			return;
		//OutputDebugPrintf("%s: mode %d size %ld\n", __FUNCTION__, LogVar->RotateMode, LogVar->ByteCount);
	} else {
		return;
	}

	logfile_lock();
	// ログサイズを再初期化する。
	LogVar->ByteCount = 0;

	// いったん今のファイルをクローズして、別名のファイルをオープンする。
	CloseFileSync(LogVar);

	// 世代ローテーションのステップ数の指定があるか
	if (LogVar->RotateStep > 0)
		loopmax = LogVar->RotateStep;

	for (i = 1 ; i <= loopmax ; i++) {
		wchar_t *filename;
		aswprintf(&filename, L"%s.%d", LogVar->FullName, i);
		DWORD attr = _GetFileAttributesW(filename);
		free(filename);
		if (attr == INVALID_FILE_ATTRIBUTES)
			break;
	}
	if (i > loopmax) {
		// 世代がいっぱいになったら、最古のファイルから廃棄する。
		i = loopmax;
	}

	// 別ファイルにリネーム。
	for (k = i-1 ; k >= 0 ; k--) {
		wchar_t *oldfile;
		if (k == 0)
			oldfile = _wcsdup(LogVar->FullName);
		else
			aswprintf(&oldfile, L"%s.%d", LogVar->FullName, k);
		wchar_t *newfile;
		aswprintf(&newfile, L"%s.%d", LogVar->FullName, k+1);
		_DeleteFileW(newfile);
		if (_MoveFileW(oldfile, newfile) == 0) {
			OutputDebugPrintf("%s: rename %d\n", __FUNCTION__, errno);
		}
		free(oldfile);
		free(newfile);
	}

	// 再オープン
	OpenLogFile(LogVar);
	if (LogVar->bom) {
		FLogOutputBOM();
	}
	if (ts.DeferredLogWriteMode) {
		StartThread(LogVar);
	}

	logfile_unlock();
}

static wchar_t *TimeStampStr()
{
	char *strtime = NULL;
	switch (ts.LogTimestampType) {
	case TIMESTAMP_LOCAL:
	default:
		strtime = mctimelocal(ts.LogTimestampFormat, FALSE);
		break;
	case TIMESTAMP_UTC:
		strtime = mctimelocal(ts.LogTimestampFormat, TRUE);
		break;
	case TIMESTAMP_ELAPSED_LOGSTART:
		strtime = strelapsed(LogVar->StartTime);
		break;
	case TIMESTAMP_ELAPSED_CONNECTED:
		strtime = strelapsed(cv.ConnectedTime);
		break;
	}

	char tmp[128];
	tmp[0] = 0;
	strncat_s(tmp, sizeof(tmp), "[", _TRUNCATE);
	strncat_s(tmp, sizeof(tmp), strtime, _TRUNCATE);
	strncat_s(tmp, sizeof(tmp), "] ", _TRUNCATE);

	return ToWcharA(tmp);
}

/**
 * バッファ内のログをファイルへ書き込む
 */
static void LogToFile(void)
{
	PCHAR Buf;
	int Start, Count;
	BYTE b;

	if (FileLog)
	{
		Buf = cv_LogBuf;
		Start = cv_LStart;
		Count = cv_LCount;
	}
	else if (BinLog)
	{
		Buf = cv_BinBuf;
		Start = cv_BStart;
		Count = cv_BCount;
	}
	else
		return;

	if (Buf==NULL) return;
	if (Count==0) return;

	// ロックを取る(2004.8.6 yutaka)
	logfile_lock();

	// 書き込みデータを作成する
	DWORD WriteBufMax = 8192;
	DWORD WriteBufLen = 0;
	PCHAR WriteBuf = (PCHAR)malloc(WriteBufMax);
	while (Get1(Buf,&Start,&Count,&b)) {
		if (FLogIsPause() || (cv.ProtoFlag)) {
			continue;
		}

		if (WriteBufLen >= (WriteBufMax*4/5)) {
			WriteBufMax *= 2;
			WriteBuf = (PCHAR)realloc(WriteBuf, WriteBufMax);
		}

		WriteBuf[WriteBufLen++] = b;

		(LogVar->ByteCount)++;
	}

	// 書き込み
	if (WriteBufLen > 0) {
		if (ts.DeferredLogWriteMode) {
			PostThreadMessage(LogVar->LogThreadId, WM_DPC_LOGTHREAD_SEND, (WPARAM)WriteBuf, WriteBufLen);
		}
		else {
			DWORD wrote;
			WriteFile(LogVar->FileHandle, WriteBuf, WriteBufLen, &wrote, NULL);
			free(WriteBuf);
		}
	}

	logfile_unlock();

	if (FileLog)
	{
		cv_LStart = Start;
		cv_LCount = Count;
	}
	else {
		cv_BStart = Start;
		cv_BCount = Count;
	}
	if (FLogIsPause() || cv.ProtoFlag) return;
	LogVar->FLogDlg->RefreshNum(LogVar->StartTime, LogVar->FileSize, LogVar->ByteCount);


	// ログ・ローテート
	LogRotate();
}

static BOOL CreateLogBuf(void)
{
	if (cv_LogBuf==NULL)
	{
		cv_LogBuf = (char *)malloc(InBuffSize);
		cv_LogPtr = 0;
		cv_LStart = 0;
		cv_LCount = 0;
	}
	return (cv_LogBuf!=NULL);
}

static void FreeLogBuf(void)
{
	free(cv_LogBuf);
	cv_LogBuf = NULL;
	cv_LogPtr = 0;
	cv_LStart = 0;
	cv_LCount = 0;
}

static BOOL CreateBinBuf(void)
{
	if (cv_BinBuf==NULL)
	{
		cv_BinBuf = (PCHAR)malloc(InBuffSize);
		cv_BinPtr = 0;
		cv_BStart = 0;
		cv_BCount = 0;
	}
	return (cv_BinBuf!=NULL);
}

static void FreeBinBuf(void)
{
	free(cv_BinBuf);
	cv_BinBuf = NULL;
	cv_BinPtr = 0;
	cv_BStart = 0;
	cv_BCount = 0;
}

static void FileTransEnd_(void)
{
	if (LogVar == NULL) {
		return;
	}
	FileLog = FALSE;
	BinLog = FALSE;
	cv.Log1Byte = NULL;
	cv.Log1Bin = NULL;
	cv.LogBinSkip = NULL;
	PFileTransDlg FLogDlg = LogVar->FLogDlg;
	if (FLogDlg != NULL) {
		FLogDlg->DestroyWindow();
		FLogDlg = NULL;
		LogVar->FLogDlg = NULL;
	}
	CloseFileSync(LogVar);
	FreeLogBuf();
	FreeBinBuf();
	free(LogVar->FullName);
	LogVar->FullName = NULL;
	free(LogVar);
	LogVar = NULL;
}

/**
 *	ログをポーズする
 */
void FLogPause(BOOL Pause)
{
	if (LogVar == NULL) {
		return;
	}
	LogVar->IsPause = Pause;
	LogVar->FLogDlg->ChangeButton(Pause);
}

/**
 *	ログローテートの設定
 *	ログのサイズが<size>バイトを超えていれば、ローテーションするよう設定する
 */
void FLogRotateSize(size_t size)
{
	if (LogVar == NULL) {
		return;
	}
	LogVar->RotateMode = ROTATE_SIZE;
	LogVar->RotateSize = (LONG)size;
}

/**
 *	ログローテートの設定
 *	ログファイルの世代を設定する
 */
void FLogRotateRotate(int step)
{
	if (LogVar == NULL) {
		return;
	}
	LogVar->RotateStep = step;
}

/**
 *	ログローテートの設定
 *	ローテーションを停止
 */
void FLogRotateHalt(void)
{
	if (LogVar == NULL) {
		return;
	}
	LogVar->RotateMode = ROTATE_NONE;
	LogVar->RotateSize = 0;
	LogVar->RotateStep = 0;
}

static INT_PTR CALLBACK OnCommentDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_COMMENT_TITLE" },
		{ IDOK, "BTN_OK" }
	};

	switch (msg) {
		case WM_INITDIALOG:
			// エディットコントロールにフォーカスをあてる
			SetFocus(GetDlgItem(hDlgWnd, IDC_EDIT_COMMENT));
			SetDlgTexts(hDlgWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);
			return FALSE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK: {
					size_t len = _SendDlgItemMessageW(hDlgWnd, IDC_EDIT_COMMENT, WM_GETTEXTLENGTH, 0, 0);
					len += 1;
					wchar_t *buf = (wchar_t *)malloc(len * sizeof(wchar_t));
					_GetDlgItemTextW(hDlgWnd, IDC_EDIT_COMMENT, buf, (int)len);
					FLogWriteStr(buf);
					FLogWriteStr(L"\n");		// TODO 改行コード
					free(buf);
					TTEndDialog(hDlgWnd, IDOK);
					break;
				}
				default:
					return FALSE;
			}
			break;

		case WM_CLOSE:
			TTEndDialog(hDlgWnd, 0);
			return TRUE;

		default:
			return FALSE;
	}
	return TRUE;
}

/**
 * ログファイルへコメントを追加する (2004.8.6 yutaka)
 */
void FLogAddCommentDlg(HINSTANCE hInst, HWND hWnd)
{
	if (LogVar == NULL) {
		return;
	}
	TTDialogBox(hInst, MAKEINTRESOURCE(IDD_COMMENT_DIALOG),
				HVTWin, OnCommentDlgProc);
}

void FLogClose(void)
{
	if (LogVar == NULL) {
		return;
	}

	FileTransEnd_();
}

/**
 *	ログをオープンする
 *	@param[in]	fname	ログファイル名, CreateFile()に渡される
 *
 *	ログファイル名はstrftimeの展開などは行われない。
 *	FLogGetLogFilename() や FLogOpenDialog() で
 *	ファイル名を取得できる。
 */
BOOL FLogOpen(const wchar_t *fname, LogCode_t code, BOOL bom)
{
	BOOL ret;

	if (LogVar != NULL) {
		return FALSE;
	}
	if ((FileLog) || (BinLog)) return FALSE;

	//
	PFileVar fv = (PFileVar)malloc(sizeof(TFileVar));
	if (fv == NULL) {
		return FALSE;
	}
	LogVar = fv;
	memset(fv, 0, sizeof(TFileVar));
	fv->FileHandle = INVALID_HANDLE_VALUE;
	fv->LogThread = INVALID_HANDLE_VALUE;
	fv->eLineEnd = Line_LineHead;

	fv->log_code = code;
	fv->bom = bom;
	ret = LogStart(fname);
	if (ret == FALSE) {
		FileTransEnd_();
	}

	return ret;
}

BOOL FLogIsOpend(void)
{
	return LogVar != NULL;
}

BOOL FLogIsOpendText(void)
{
	return LogVar != NULL && FileLog;
}

BOOL FLogIsOpendBin(void)
{
	return LogVar != NULL && BinLog;
}

void FLogWriteStr(const wchar_t *str)
{
	if (LogVar != NULL) {
		OutputStr(str);
	}
}

void FLogInfo(char *param_ptr, size_t param_len)
{
	if (LogVar) {
		param_ptr[0] = '0'
			+ (ts.LogBinary != 0)
			+ ((ts.Append != 0) << 1)
			+ ((ts.LogTypePlainText != 0) << 2)
			+ ((ts.LogTimestamp != 0) << 3)
			+ ((ts.LogHideDialog != 0) << 4);
		char *filenameU8 = ToU8W(LogVar->FullName);
		strncpy_s(param_ptr + 1, param_len - 1, filenameU8, _TRUNCATE);
		free(filenameU8);
	}
	else {
		param_ptr[0] = '0' - 1;
		param_ptr[1] = 0;
	}
}

/**
 *	現在のログファイル名を取得
 */
const wchar_t *FLogGetFilename(void)
{
	if (LogVar == NULL) {
		return NULL;
	}
	return LogVar->FullName;
}

/**
 *	ログダイアログを開く
 *	@param[in,out]	info.filename	ファイル名初期値
 *									OK時、ファイル名、不要になったらfree()すること
 *	@retval	TRUE	[ok] が押された
 *	@retval	FALSE	キャンセルされた
 */
BOOL FLogOpenDialog(HINSTANCE hInst, HWND hWnd, FLogDlgInfo_t *info)
{
	LogDlgWork_t *work = (LogDlgWork_t *)calloc(sizeof(LogDlgWork_t), 1);
	wchar_t *srcfnameW = FLogGetLogFilename(info->filename);
	work->info = info;
	work->info->filename = srcfnameW;
	work->pts = &ts;
	INT_PTR ret = TTDialogBoxParam(
		hInst, MAKEINTRESOURCE(IDD_LOGDLG),
		hWnd, LogFnHook, (LPARAM)work);
	free(srcfnameW);
	free(work);
	return ret == IDOK ? TRUE : FALSE;
}

/**
 *	ログファイル名を取得
 *	ログファイル名用の修飾を行う
 *	- strftime() と同じ日付展開
 *	- 設定されたログファイルフォルダを追加
 *	- ホスト名,ポート番号展開
 *
 *	@param[in]	log_filename	ファイル名(相対/絶対どちらでもok)
 *								NULLの場合デフォルトファイル名となる
 *								strftime形式ok
 *	@return						フルパスファイル名
 *								不要になったら free() すること
 */
wchar_t *FLogGetLogFilename(const wchar_t *log_filename)
{
	// フォルダ
	char FileDirExpanded[MAX_PATH];
	const char *logdir;
	if (strlen(ts.LogDefaultPath) > 0) {
		logdir = ts.LogDefaultPath;
	}
	else if (strlen(ts.FileDir) > 0) {
		ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
		logdir = FileDirExpanded;
	}
	else {
		logdir = ts.HomeDir;
	}

	// 元となるファイル名
	char base_name[MAX_PATH];
	if (log_filename == NULL) {
		strncpy_s(base_name, _countof(base_name), ts.LogDefaultName, _TRUNCATE);
	}
	else {
		char *filenameA = ToCharW(log_filename);
		strncpy_s(base_name, _countof(base_name), filenameA, _TRUNCATE);
		free(filenameA);
	}

	// フルパス化
	char full_path[MAX_PATH];
	ConvFName(logdir, base_name, sizeof(base_name), "", full_path, sizeof(full_path));
	ParseStrftimeFileName(full_path, sizeof(full_path));
	ConvertLogname(full_path, sizeof(full_path));

	return ToWcharA(full_path);
}

BOOL FLogIsPause()
{
	if (LogVar == NULL) {
		return FALSE;
	}
	return LogVar->IsPause;
}

void FLogWindow(int nCmdShow)
{
	if (LogVar == NULL) {
		return;
	}

	HWND HWndLog = LogVar->FLogDlg->m_hWnd;
	ShowWindow(HWndLog, nCmdShow);
	if (nCmdShow == SW_RESTORE) {
		// 拡張スタイル WS_EX_NOACTIVATE 状態を解除する
		SetForegroundWindow(HWndLog);
	}
}

void FLogShowDlg(void)
{
	if (LogVar == NULL) {
		return;
	}
	HWND HWndLog = LogVar->FLogDlg->m_hWnd;
	ShowWindow(HWndLog, SW_SHOWNORMAL);
	SetForegroundWindow(HWndLog);
}

/**
 * ログへ1byte書き込み
 *		LogPut1() と違う?
 */
//void Log1Bin(PComVar cv, BYTE b)
static void Log1Bin(BYTE b)
{
	if (LogVar->IsPause || cv.ProtoFlag) {
		return;
	}
	if (cv_BinSkip > 0) {
		cv_BinSkip--;
		return;
	}
	cv_BinBuf[cv_BinPtr] = b;
	cv_BinPtr++;
	if (cv_BinPtr>=InBuffSize) {
		cv_BinPtr = cv_BinPtr-InBuffSize;
	}
	if (cv_BCount>=InBuffSize) {
		cv_BCount = InBuffSize;
		cv_BStart = cv_BinPtr;
	}
	else {
		cv_BCount++;
	}
}

static void LogBinSkip(int add)
{
	if (cv_BinBuf != NULL) {
		cv_BinSkip += add;
	}
}

/**
 *	ログバッファに溜まっているデータのバイト数を返す
 */
int FLogGetCount(void)
{
	if (FileLog) {
		return cv_LCount;
	}
	if (BinLog) {
		return cv_BCount;
	}
	return 0;
}

/**
 *	ログバッファの空きバイト数を返す
 */
int FLogGetFreeCount(void)
{
	if (FileLog) {
		return InBuffSize - cv_LCount;
	}
	if (BinLog) {
		return InBuffSize - cv_BCount;
	}
	return 0;
}

/**
 * バッファ内のログをファイルへ書き込む
 */
void FLogWriteFile(void)
{
	if (cv_LogBuf!=NULL)
	{
		if (FileLog) {
			LogToFile();
		}
	}

	if (cv_BinBuf!=NULL)
	{
		if (BinLog) {
			LogToFile();
		}
	}
}

void FLogPutUTF32(unsigned int u32)
{
	PFileVar fv = LogVar;
	size_t i;
	BOOL log_available = (cv_LogBuf != 0);

	if (!log_available) {
		// ログには出力しない
		return;
	}

	// 行頭か?(改行を出力した直後)
	if (ts.LogTimestamp && fv->eLineEnd) {
		// タイムスタンプを出力
		fv->eLineEnd = Line_Other; /* clear endmark*/
		wchar_t* strtime = TimeStampStr();
		FLogWriteStr(strtime);
		free(strtime);
	}

	switch(fv->log_code) {
	case LOG_UTF8: {
		// UTF-8
		char u8_buf[4];
		size_t u8_len = UTF32ToUTF8(u32, u8_buf, _countof(u8_buf));
		for (i = 0; i < u8_len; i++) {
			BYTE b = u8_buf[i];
			LogPut1(b);
		}
		break;
	}
	case LOG_UTF16LE:
	case LOG_UTF16BE: {
		// UTF-16
		wchar_t u16[2];
		size_t u16_len = UTF32ToUTF16(u32, u16, _countof(u16));
		size_t i;
		for (i = 0; i < u16_len; i++) {
			if (fv->log_code == LOG_UTF16LE) {
				// UTF-16LE
				LogPut1(u16[i] & 0xff);
				LogPut1((u16[i] >> 8) & 0xff);
			}
			else {
				// UTF-16BE
				LogPut1((u16[i] >> 8) & 0xff);
				LogPut1(u16[i] & 0xff);
			}
		}
	}
	}

	if (u32 == 0x0a) {
		fv->eLineEnd = Line_LineHead; /* set endmark*/
	}
}

static void FLogOutputBOM(void)
{
	PFileVar fv = LogVar;
	DWORD wrote;

	switch(fv->log_code) {
	case 0: {
		// UTF-8
		const char *bom = "\xef\xbb\xbf";
		WriteFile(LogVar->FileHandle, bom, 3, &wrote, NULL);
		LogVar->ByteCount += 3;
		break;
	}
	case 1: {
		// UTF-16LE
		const char *bom = "\xff\xfe";
		WriteFile(LogVar->FileHandle, bom, 2, &wrote, NULL);
		LogVar->ByteCount += 2;
		break;
	}
	case 2: {
		// UTF-16BE
		const char *bom = "\xfe\xff";
		WriteFile(LogVar->FileHandle, bom, 2, &wrote, NULL);
		LogVar->ByteCount += 2;
		break;
	}
	default:
		break;
	}
}

static void OutputStr(const wchar_t *str)
{
	size_t len;

	assert(str != NULL);

	len = wcslen(str);
	while(*str != 0) {
		unsigned int u32;
		size_t u16_len = UTF16ToUTF32(str, len, &u32);
		switch (u16_len) {
		case 0:
		default:
			// 変換できない
			str++;
			len--;
			break;
		case 1:
		case 2: {
			FLogPutUTF32(u32);
			str += u16_len;
			len -= u16_len;
			break;
		}
		}
	}
}
