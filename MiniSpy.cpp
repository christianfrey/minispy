#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
#include <psapi.h>
#include "resource.h"
#include "Styles.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "psapi.lib")

HINSTANCE hInst;
HWND hMain, hTab, hFoundWin, hWndBestCtrl;
HWND ctrls[3];
DWORD dwArea;
BOOL bSeeking, bOnTop, bAutoHide;

// Made by BruNews (return ptr on final NULL)
__declspec(naked) char* __fastcall bnitoa(int inum, char* szdst)
{ // ECX = inum, EDX = szdst
	__asm {
		test		ecx, ecx
		jz			short L0
		mov			[esp-4], edi
		mov			[esp-8], esi
		mov			edi, edx
		jge			short L2
		jmp			short L1
 L0:
		lea			eax, [edx+1]
		mov			byte ptr[edx], 48
		mov			byte ptr[eax], cl
		ret			0
 L1:
		mov			byte ptr[edi], 45
		neg			ecx
		inc			edi
 L2:
		mov			esi, edi
 L3:
		mov			eax, -858993459
		mul			ecx
		mov			eax, edx
		shr			eax, 3
		mov			edx, ecx
		lea			ecx, [eax+eax*4]
		add			ecx, ecx
		sub			edx, ecx
		add			dl, 48
		mov			[edi], dl
		mov			ecx, eax
		inc			edi
		test		eax, eax
		jnz			short L3
		mov			byte ptr[edi], al
		mov			eax, edi
 L4:
		cmp			esi, edi
		jae			L5
		dec			edi
		mov			cl, [esi]
		mov			dl, [edi]
		mov			[edi], cl
		mov			[esi], dl
		inc			esi
		jmp			short L4
 L5:
		mov			edi, [esp-4]
		mov			esi, [esp-8]
		ret			0
	}
}

BOOL CALLBACK TabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_CTLCOLORDLG:
			SetBkMode((HDC)wParam, TRANSPARENT);
			return (BOOL)GetStockObject(NULL_BRUSH);
		case WM_CTLCOLORSTATIC:
			if(GetWindowLong((HWND)lParam, GWL_ID) == -1) {
				SetBkMode((HDC)wParam, TRANSPARENT);
				return (BOOL)GetStockObject(NULL_BRUSH);
			}
	}
	return 0;
}

BOOL CALLBACK FindBestChildProc(HWND hWndCtrl, LPARAM lParam)
{
	RECT	rct;
	DWORD	a;
	POINT	pt;
	pt.x = (short)LOWORD(lParam);
	pt.y = (short)HIWORD(lParam);
	GetWindowRect(hWndCtrl, &rct);
	if(PtInRect(&rct, pt)) {
		a = (rct.right-rct.left) * (rct.bottom-rct.top);
		if(a < dwArea && IsWindowVisible(hWndCtrl)) {
			dwArea = a;
			hWndBestCtrl = hWndCtrl;
		}
	}
	return 1;
}

HWND FindBestChild(HWND hFound, POINT pt)
{
	HWND hParent;
	DWORD dwStyle;
	dwArea = -1;
	hWndBestCtrl = 0;
	hParent = GetParent(hFound);
	dwStyle = GetWindowLong(hFound, GWL_STYLE);
	if(hParent == 0 || (dwStyle & WS_POPUP)) hParent = hFound;
	EnumChildWindows(hParent, FindBestChildProc, MAKELPARAM(pt.x, pt.y));
	if(hWndBestCtrl == 0) hWndBestCtrl = hParent;
	return hWndBestCtrl;
}

HWND WindowFromPointEx(POINT pt)
{
	HWND hPoint;
	hPoint = WindowFromPoint(pt);
	if(!hPoint) return 0;
	hPoint = FindBestChild(hPoint, pt);
	return hPoint;
}

void DisplayInfo(HWND hFound)
{
	int j;
	DWORD dwStyles, dwStyle, dwProcessId, dwThreadId, cbNeeded;
	HMODULE hModule;
	HANDLE hProcess;
	char	szCaption[256], szClass[256], szBuffer[256], *p,
				szModName[256] = "Unknown", szModPath[256] = "Unknown", szId[16];
	RECT	rct;

	//===============================================================
	// Tab [General]
	//===============================================================
	p = szBuffer; *p++ = '0'; *p++ = 'x';
	for(j = 28; j >= 0; j-=4) {
      *p = (BYTE)((int)hFound >> j) & 0xF;
      if(*p > 9) *p += 'A' - 10;
      else *p += '0';
      p++;
	}
	*p++ = 0;
	SetDlgItemText(ctrls[0], IDED_HANDLE, szBuffer);

	GetClassName(hFound, szClass, 256);
	if((strstr(szClass, "Edit") == 0) && (strstr(szClass, "Box") == 0))
		GetWindowText(hFound, szCaption, 256);
	else SendDlgItemMessage(GetParent(hFound), GetWindowLong(hFound, GWL_ID), WM_GETTEXT, 256, (long)szCaption);
	SetDlgItemText(ctrls[0], IDED_CAPTION, szCaption);
	SetDlgItemText(ctrls[0], IDED_CLASS, szClass);

	GetWindowRect(hFound, &rct);
	p = szBuffer; *p++ = '(';
	p = bnitoa(rct.left, p); *p++ = ',';
	p = bnitoa(rct.top, p); *p++ = ')'; *p++ = '-'; *p++ = '(';
	p = bnitoa(rct.right, p); *p++ = ',';
	p = bnitoa(rct.bottom, p); *p++ = ')'; *p++ = ' ';
	p = bnitoa(rct.right - rct.left, p); *p++ = 'x';
	p = bnitoa(rct.bottom - rct.top, p); *p++ = 0;
	SetDlgItemText(ctrls[0], IDED_RECT, szBuffer);

	GetClientRect(hFound, &rct);
	p = szBuffer; *p++ = '('; 
	*p++ = '0'; *p++ = ',';
	*p++ = '0'; *p++ = ')'; *p++ = '-'; *p++ = '(';
	p = bnitoa(rct.right, p); *p++ = ',';
	p = bnitoa(rct.bottom, p); *p++ = ')'; *p++ = ' ';
	p = bnitoa(rct.right - rct.left, p); *p++ = 'x';
	p = bnitoa(rct.bottom - rct.top, p); *p++ = 0;
	SetDlgItemText(ctrls[0], IDED_CLIENTRECT, szBuffer);

	//===============================================================
	// Tab [Styles]
	//===============================================================
	dwStyles = GetWindowLong(hFound, GWL_STYLE);
	p = szBuffer; *p++ = '0'; *p++ = 'x';
	for(j = 28; j >= 0; j-=4) {
		*p = (BYTE)(dwStyles >> j) & 0xF;
		if(*p > 9) *p += 'A' - 10;
		else *p += '0';
		p++;
	}
	*p++ = 0;
	SetDlgItemText(ctrls[1], IDED_STYLES, szBuffer);

	SendMessage(GetDlgItem(ctrls[1], IDLST_STYLES), LB_RESETCONTENT, 0, 0);
	for(j = 0; j <= 20; j++) {
		dwStyle = WinStyles[j].dwStyle;
		if((dwStyles & 0x00CF0000) == 0x00CF0000) {
			dwStyles -= 0x00CF0000;
			SendMessage(GetDlgItem(ctrls[1], IDLST_STYLES), LB_ADDSTRING, 0, (long)WinStyles[19].szStyle);
		}
		else if((dwStyles & 0x80880000) == 0x80880000) {
			dwStyles -= 0x80880000;
			SendMessage(GetDlgItem(ctrls[1], IDLST_STYLES), LB_ADDSTRING, 0, (long)WinStyles[20].szStyle);
		}
		if((dwStyles & dwStyle) == dwStyle) {
			SendMessage(GetDlgItem(ctrls[1], IDLST_STYLES), LB_ADDSTRING, 0, (long)WinStyles[j].szStyle);
		}
	}

	dwStyles = GetWindowLong(hFound, GWL_EXSTYLE);
	p = szBuffer; *p++ = '0'; *p++ = 'x';
	for(j = 28; j >= 0; j-=4) {
		*p = (BYTE)(dwStyles >> j) & 0xF;
		if(*p > 9) *p += 'A' - 10;
		else *p += '0';
		p++;
	}
	*p++ = 0;
	SetDlgItemText(ctrls[1], IDED_EXSTYLES, szBuffer);

	SendMessage(GetDlgItem(ctrls[1], IDLST_EXSTYLES), LB_RESETCONTENT, 0, 0);
	for(j = 0; j <= 20; j++) {
		dwStyle = ExWinStyles[j].dwStyle;
		if((dwStyles & dwStyle) == dwStyle) {
			SendMessage(GetDlgItem(ctrls[1], IDLST_EXSTYLES), LB_ADDSTRING, 0, (long)ExWinStyles[j].szStyle);
		}
	}

	//===============================================================
	// Tab [Process]
	//===============================================================
	dwThreadId = GetWindowThreadProcessId(hFound, &dwProcessId);
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, dwProcessId);
	if(hProcess) {
		if(EnumProcessModules(hProcess, &hModule, sizeof(hModule), &cbNeeded))
			GetModuleBaseName(hProcess, hModule, szModName, sizeof(szModName));
	}
	SetDlgItemText(ctrls[2], IDED_MODNAME, szModName);

	GetModuleFileNameEx(hProcess, hModule, szModPath, sizeof(szModPath));
	SetDlgItemText(ctrls[2], IDED_MODPATH, szModPath);

	p = szId; *p++ = '0'; *p++ = 'x';
	for(j = 28; j >= 0; j-=4) {
		*p = (BYTE)(dwProcessId >> j) & 0xF;
		if(*p > 9) *p += 'A' - 10;
		else *p += '0';
		p++;
	}
	*p++ = 0;
	SetDlgItemText(ctrls[2], IDED_PROCID, szId);

	p = szId; *p++ = '0'; *p++ = 'x';
	for(j = 28; j >= 0; j-=4) {
    *p = (BYTE)(dwThreadId >> j) & 0xF;
    if(*p > 9) *p += 'A' - 10;
    else *p += '0';
    p++;
	}
	*p++ = 0;
	SetDlgItemText(ctrls[2], IDED_THRDID, szId);
}

void HighlightWindow()
{
  HDC hDC;
	HRGN hRgn = CreateRectRgn(0, 0, 0, 0);
  int regionType;

  hDC = GetWindowDC(hFoundWin);
  if(!hDC) return;
  SetROP2(hDC, R2_NOT); // Dessine en inversé

  regionType = GetWindowRgn(hFoundWin, hRgn);
  if(regionType == ERROR)
  { // Fenêtre rectangulaire
    RECT rct;
    DeleteObject(hRgn);
    GetWindowRect(hFoundWin, &rct);
    // Fabrique une région rectangulaire à partir de ce rct
    hRgn = CreateRectRgn(0, 0, rct.right - rct.left, rct.bottom - rct.top);
  }
  FrameRgn(hDC, hRgn, (HBRUSH)GetStockObject(WHITE_BRUSH), 3, 3);
  DeleteObject(hRgn);
  ReleaseDC(hFoundWin, hDC);
}

void CreateTabs()
{
	TCITEM tie;
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.iImage = -1;
  tie.pszText = "General";
	TabCtrl_InsertItem(hTab, 0, &tie);
  tie.pszText = "Styles";
	TabCtrl_InsertItem(hTab, 1, &tie);
  tie.pszText = "Process";
	TabCtrl_InsertItem(hTab, 2, &tie);
}

BOOL CALLBACK AppDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
		case WM_INITDIALOG:
			hMain = hDlg;
			SetClassLong(hDlg, GCL_HICON, (long)LoadIcon(hInst, (LPCTSTR)IDI_APP));
			hTab = GetDlgItem(hDlg, ID_TABCTRL);
			CreateTabs();
			ctrls[0] = CreateDialog(hInst, (LPCTSTR)IDD_TAB1, hDlg, TabDlgProc);
			ctrls[1] = CreateDialog(hInst, (LPCTSTR)IDD_TAB2, hDlg, TabDlgProc);
			ctrls[2] = CreateDialog(hInst, (LPCTSTR)IDD_TAB3, hDlg, TabDlgProc);
			SetWindowPos(ctrls[0], 0, 15, 100, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);
			SetWindowPos(ctrls[1], 0, 15, 100, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);
			SetWindowPos(ctrls[2], 0, 15, 100, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);
			ShowWindow(ctrls[0], SW_SHOW);
			return 1;
		case WM_NOTIFY: {
			LPNMHDR lpnm;
			lpnm = (LPNMHDR)lParam;
			if(lpnm->idFrom == ID_TABCTRL) {
				if(lpnm->code == TCN_SELCHANGE) {
					int i, j;
					i = SendMessage(lpnm->hwndFrom, TCM_GETCURSEL, 0, 0);
					for(j = 0; j < 3; j++) ShowWindow(ctrls[j], i == j ? SW_SHOW: SW_HIDE);
					PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)ctrls[i], 1);
				}
			}
			}
			return 0;
		case WM_MOUSEMOVE:
			if(bSeeking) {
				POINT pt;
				HWND hPoint;
				HWND hParent;
				pt.x = (short)LOWORD(lParam);
				pt.y = (short)HIWORD(lParam);
				ClientToScreen(hDlg, (POINT*)&pt);
				hPoint = WindowFromPointEx(pt);
				hParent = GetParent(hPoint);
				if(hPoint == 0 || !IsWindow(hPoint)) return 0;
				if(hPoint == hMain || hPoint == hFoundWin) return 0;
				if(hParent == hMain || GetParent(hParent) == hMain) return 0;
				DisplayInfo(hPoint);
				if(hFoundWin != hPoint) HighlightWindow();
				hFoundWin = hPoint;
				HighlightWindow();
			}
			return 0;
		case WM_LBUTTONUP:
			if(bSeeking) {
				if(hFoundWin) RedrawWindow(hFoundWin, 0, 0, RDW_ERASE | 
					RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
				SendDlgItemMessage(hDlg, IDST_BMP, STM_SETIMAGE, IMAGE_BITMAP,
										(long)LoadBitmap(hInst, (LPCTSTR)IDBMP_FILLED));
				if(bAutoHide) SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				if(bOnTop) SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				hFoundWin = 0;
				ReleaseCapture();
				bSeeking = 0;
			}
			return 0;
		case WM_COMMAND:
		switch(wParam) {
			case IDST_BMP:
				SendDlgItemMessage(hDlg, IDST_BMP, STM_SETIMAGE, IMAGE_BITMAP, 
					(long)LoadBitmap(hInst, (LPCTSTR)IDBMP_EMPTY));
				if(bAutoHide) SetWindowPos(hDlg, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				SetCursor(LoadCursor(hInst, (LPCTSTR)IDCUR_FINDER));
				SetCapture(hDlg); 
				bSeeking = 1;
				return 0;
			case IDCHK_ONTOP:
				bOnTop = IsDlgButtonChecked(hDlg, IDCHK_ONTOP);
				if(bOnTop) SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				else SetWindowPos(hDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				return 0;
			case IDCHK_AUTOHIDE:
				bAutoHide = IsDlgButtonChecked(hDlg, IDCHK_AUTOHIDE);
				return 0;
			case IDCANCEL: EndDialog(hDlg, 0);
		}
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE x, PSTR y, int z)
{
	InitCommonControls();
	hInst = hInstance;
	DialogBoxParam(hInstance, (LPCTSTR)IDD_APP, 0, AppDlgProc, 0);
	return 0;
}
