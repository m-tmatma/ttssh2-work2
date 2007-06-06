/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, input dialog box */

#include "stdafx.h"
#include "teraterm.h"
#include "ttlib.h"
#ifdef TERATERM32
#include "ttm_res.h"
#else
#include "ttm_re16.h"
#endif
#include "ttmlib.h"

#include "inpdlg.h"

#define MaxStrLen 256

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CInpDlg dialog
CInpDlg::CInpDlg(PCHAR Input, PCHAR Text, PCHAR Title,
		 BOOL Paswd, int x, int y)
	: CDialog(CInpDlg::IDD)
{
  //{{AFX_DATA_INIT(CInpDlg)
  //}}AFX_DATA_INIT
  RestoreNewLine(Text); // (2006.7.29 maya)
  InputStr = Input;
  TextStr = Text;
  TitleStr = Title;
  PaswdFlag = Paswd;
  PosX = x;
  PosY = y;
}

BEGIN_MESSAGE_MAP(CInpDlg, CDialog)
	//{{AFX_MSG_MAP(CInpDlg)
	ON_MESSAGE(WM_EXITSIZEMOVE, OnExitSizeMove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CInpDlg message handler

// msgdlg のように、メッセージが長い場合にはダイアログを拡げるようにした (2006.7.29 maya)
BOOL CInpDlg::OnInitDialog()
{
  RECT R;
  HDC TmpDC;
  HWND HEdit, HOk;
#ifndef NO_I18N
  char uimsg[MAX_UIMSG];
  LOGFONT logfont;
  HFONT font;
#endif

  CDialog::OnInitDialog();
#ifndef NO_I18N
  font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
  GetObject(font, sizeof(LOGFONT), &logfont);
  if (get_lang_font("DLG_SYSTEM_FONT", m_hWnd, &logfont, &DlgFont, UILanguageFile)) {
    SendDlgItemMessage(IDC_INPTEXT, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
    SendDlgItemMessage(IDC_INPEDIT, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
    SendDlgItemMessage(IDOK, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
  }

  GetDlgItemText(IDOK, uimsg, sizeof(uimsg));
  get_lang_msg("BTN_OK", uimsg, UILanguageFile);
  SetDlgItemText(IDOK, uimsg);
#endif

  SetWindowText(TitleStr);
  SetDlgItemText(IDC_INPTEXT,TextStr);

  TmpDC = ::GetDC(GetSafeHwnd());
  CalcTextExtent(TmpDC,TextStr,&s);
  ::ReleaseDC(GetSafeHwnd(),TmpDC);
  TW = s.cx + s.cx/10;
  TH = s.cy;

  HEdit = ::GetDlgItem(GetSafeHwnd(), IDC_INPEDIT);
  ::GetWindowRect(HEdit,&R);
  EW = R.right-R.left;
  EH = R.bottom-R.top;

  HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
  ::GetWindowRect(HOk,&R);
  BW = R.right-R.left;
  BH = R.bottom-R.top;

  GetWindowRect(&R);
  WW = R.right-R.left;
  WH = R.bottom-R.top;

  Relocation(TRUE, WW);

#ifdef TERATERM32
  SetForegroundWindow();
#else
  SetActiveWindow();
#endif
  return TRUE;
}

void CInpDlg::OnOK()
{
  GetDlgItemText(IDC_INPEDIT,InputStr,MaxStrLen-1);
  EndDialog(IDOK);
}

LONG CInpDlg::OnExitSizeMove(UINT wParam, LONG lParam)
{
  RECT R;

  GetWindowRect(&R);
  if (R.bottom-R.top == WH && R.right-R.left == WW) {
    // サイズが変わっていなければ何もしない
  }
  else if (R.bottom-R.top != WH || R.right-R.left < init_WW) {
    // 高さが変更されたか、最初より幅が狭くなった場合は元に戻す
    SetWindowPos(&wndTop,R.left,R.top,WW,WH,0);
  }
  else {
    // そうでなければ再配置する
    Relocation(FALSE, R.right-R.left);
  }

  return CDialog::DefWindowProc(WM_EXITSIZEMOVE,wParam,lParam);
}

void CInpDlg::Relocation(BOOL is_init, int new_WW)
{
  RECT R;
  HDC TmpDC;
  HWND HText, HOk, HEdit;
  int CW, CH;

  GetClientRect(&R);
  CW = R.right-R.left;
  CH = R.bottom-R.top;

  // 初回のみ
  if (is_init) {
    // テキストコントロールサイズを補正
    if (TW < CW)
      TW = CW;
    if (EW < s.cx)
      EW = s.cx;
	// ウインドウサイズの計算
    WW = WW + TW - CW;
    WH = WH + 2*TH+3*BH/2 - CH + EH/2+BH/2+12;
	init_WW = WW;
  }
  else {
    TW = CW;
    WW = new_WW;
  }

  HText = ::GetDlgItem(GetSafeHwnd(), IDC_INPTEXT);
  HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
  HEdit = ::GetDlgItem(GetSafeHwnd(), IDC_INPEDIT);

  ::MoveWindow(HText,(TW-s.cx)/2,TH/2,TW,TH,TRUE);
  ::MoveWindow(HEdit,(WW-EW)/2-4,2*TH+5,EW,EH,TRUE);
  ::MoveWindow(HOk,(TW-BW)/2,2*TH+EH/2+BH/2+12,BW,BH,TRUE);

  if (PaswdFlag)
    SendDlgItemMessage(IDC_INPEDIT,EM_SETPASSWORDCHAR,(UINT)'*',0);

  SendDlgItemMessage(IDC_INPEDIT, EM_LIMITTEXT, MaxStrLen, 0);

  if (PosX<=-100)
  {
    GetWindowRect(&R);
    TmpDC = ::GetDC(GetSafeHwnd());
    PosX = (GetDeviceCaps(TmpDC,HORZRES)-R.right+R.left) / 2;
    PosY = (GetDeviceCaps(TmpDC,VERTRES)-R.bottom+R.top) / 2;
    ::ReleaseDC(GetSafeHwnd(),TmpDC);
  }
  SetWindowPos(&wndTop,PosX,PosY,WW,WH,0);

  InvalidateRect(NULL);
}
