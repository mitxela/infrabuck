/*   infrabuck - GUI for windows/x86 brainfuck compiler
 *   invokes bf.exe, must be in the same folder.
 *   written by mitxela
 *   http://mitxela.com/projects/infrabuck
 */

#include <windows.h>
#include <stdio.h>
#include "ide.h"

#define wClass "ideClass"
#define ideTitle "infrabuck"
#define fileFilter "Brainfuck (*.bf;*.b)\0*.bf;*.b\0All Files (*.*)\0*.*\0"

char workingFileName[MAX_PATH]="";
BOOL fileChanged=FALSE;

void SetTitleBar(HWND hwnd) {
  char titleBar[MAX_PATH + 20];
  char unt[] = "Untitled";
  char * filename = unt;

  if (workingFileName!= NULL && workingFileName[0]!=0) filename = workingFileName;

  sprintf(titleBar, "%s%s - %s",fileChanged?"*":"",filename,ideTitle);

  SetWindowText(hwnd, titleBar);
}

void UpdateStatusBar(HWND hwnd) {
  DWORD selstart, selend;
  char status[100] ="";

  SendMessage(GetDlgItem(hwnd, IDC_MAIN_EDIT), EM_GETSEL, (WPARAM)&selstart, (LPARAM)&selend);

  if (selend!=selstart) sprintf(status, "Selection: %d", selend-selstart);
  
  SendMessage(GetDlgItem(hwnd, IDC_STATUSBAR), SB_SETTEXT, 0, (LPARAM)&status);
}

BOOL LoadTextFileToEdit(HWND hEdit, LPCTSTR pszFileName) {
  HANDLE hFile;
  BOOL bSuccess = FALSE;

  hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,OPEN_EXISTING, 0, NULL);
  if(hFile != INVALID_HANDLE_VALUE)
  {
    DWORD dwFileSize;

    dwFileSize = GetFileSize(hFile, NULL);
    if(dwFileSize != 0xFFFFFFFF)
    {
      LPSTR pszFileText;

      pszFileText = (LPSTR)GlobalAlloc(GPTR, dwFileSize + 1);
      if(pszFileText != NULL)
      {
        DWORD dwRead;

        if(ReadFile(hFile, pszFileText, dwFileSize, &dwRead, NULL))
        {
          pszFileText[dwFileSize] = 0;
          if(SetWindowText(hEdit, pszFileText))
            bSuccess = TRUE;
        }
        GlobalFree(pszFileText);
      }
    }
    CloseHandle(hFile);
  }
  return bSuccess;
}

BOOL SaveTextFileFromEdit(HWND hEdit, LPCTSTR pszFileName)
{
  HANDLE hFile;
  BOOL bSuccess = FALSE;

  hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if(hFile != INVALID_HANDLE_VALUE)
  {
    DWORD dwTextLength;

    dwTextLength = GetWindowTextLength(hEdit);
    // No need to bother if there's no text.
    if(dwTextLength > 0)
    {
      LPSTR pszText;
      DWORD dwBufferSize = dwTextLength + 1;

      pszText = (LPSTR)GlobalAlloc(GPTR, dwBufferSize);
      if(pszText != NULL)
      {
        if(GetWindowText(hEdit, pszText, dwBufferSize))
        {
          DWORD dwWritten;

          if(WriteFile(hFile, pszText, dwTextLength, &dwWritten, NULL))
            bSuccess = TRUE;
        }
        GlobalFree(pszText);
      }
    }
    CloseHandle(hFile);
  }
  return bSuccess;
}

void DoFileOpen(HWND hwnd)
{
  OPENFILENAME ofn;
  char szFileName[MAX_PATH] = "";

  ZeroMemory(&ofn, sizeof(ofn));

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFilter = fileFilter;
  ofn.lpstrFile = szFileName;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  ofn.lpstrDefExt = "bf";

  if(GetOpenFileName(&ofn))
  {
    HWND hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
    LoadTextFileToEdit(hEdit, szFileName);
    strcpy(workingFileName, szFileName);
    fileChanged=FALSE;
    SetTitleBar(hwnd);
  }
}

void DoFileSaveAs(HWND hwnd)
{
  OPENFILENAME ofn;
  char szFileName[MAX_PATH] = "";

  ZeroMemory(&ofn, sizeof(ofn));

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFilter = fileFilter;
  ofn.lpstrFile = szFileName;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrDefExt = "bf";
  ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

  if(GetSaveFileName(&ofn)) {
    HWND hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
    SaveTextFileFromEdit(hEdit, szFileName);
    strcpy(workingFileName, szFileName);
    fileChanged=FALSE;
    SetTitleBar(hwnd);
  }
}
void DoFileSave(HWND hwnd){
  if (workingFileName!= NULL && workingFileName[0]!=0)  {
    HWND hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
    SaveTextFileFromEdit(hEdit, workingFileName);
    fileChanged=FALSE;
    SetTitleBar(hwnd);
  } else DoFileSaveAs(hwnd);
}
BOOL checkSave(HWND hwnd) {
  if (!fileChanged) return 0;

  char msg[MAX_PATH + 20]="Save file?";
  char fname[MAX_PATH];
  char ext[MAX_PATH];
  _splitpath(workingFileName,NULL,NULL,fname,ext);

  if (workingFileName!= NULL && workingFileName[0]!=0) sprintf(msg,"Save changes to %s%s?",fname,ext);

  int q = MessageBox(hwnd, msg, "Save", MB_YESNOCANCEL);
  if (q == IDYES) {
    DoFileSave(hwnd);
  }
  return (q==IDCANCEL);
}
int compile(HWND hwnd){

    if (checkSave(hwnd)) return 0;
    if (workingFileName== NULL || workingFileName[0]==0) {
      MessageBox(hwnd,"File must be saved in order to compile", "Error", MB_ICONEXCLAMATION | MB_OK);
      return 1;
    }

    char fname[MAX_PATH];
    _splitpath(workingFileName,NULL,NULL,fname,NULL);

    char outfile[MAX_PATH];
    sprintf(outfile, "%s.exe",fname);

    HANDLE stdOutRd = NULL;
    HANDLE stdOutWr = NULL;
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if ( !CreatePipe(&stdOutRd, &stdOutWr, &sa, 0)
      || !SetHandleInformation(stdOutRd, HANDLE_FLAG_INHERIT, 0)) {
        MessageBox(hwnd,"Error: create pipe", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    char cmd[MAX_PATH + 200];
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;

    sprintf(cmd,"bf.exe \"%s\" \"%s\"", workingFileName, outfile);

    ZeroMemory( &StartupInfo, sizeof(STARTUPINFO) );
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.hStdOutput = stdOutWr;
    StartupInfo.dwFlags = STARTF_USESTDHANDLES;

  if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &StartupInfo, &ProcessInfo)){
    MessageBox(hwnd,"Unable to invoke bf.exe", "Error", MB_ICONEXCLAMATION | MB_OK);
    return 1;
  }
  WaitForSingleObject(ProcessInfo.hProcess,INFINITE);

  LPDWORD ecode;
  GetExitCodeProcess(ProcessInfo.hProcess,&ecode);
  if (ecode!=0) {
    DWORD dwRead;
    CHAR chBuf[200];
    ReadFile( stdOutRd, chBuf, 200, &dwRead, NULL);
	chBuf[dwRead]=0;
    MessageBox(hwnd, chBuf, "Compile Error", MB_ICONEXCLAMATION | MB_OK);
  } else {
    PROCESS_INFORMATION pinfo;
    STARTUPINFO sinfo;
    ZeroMemory( &sinfo, sizeof(STARTUPINFO) );
    sinfo.cb = sizeof(STARTUPINFO);
    CreateProcess(NULL, outfile, NULL, NULL, FALSE, 0, NULL, NULL, &sinfo, &pinfo);
  }
  CloseHandle(stdOutWr);
  CloseHandle(stdOutRd);
  return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch(msg) {
    case WM_CREATE:
    {
      HFONT hfDefault;
      HWND hEdit;
      HWND hStatus; 

      hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, (HMENU)IDC_STATUSBAR, GetModuleHandle(NULL), NULL);
      if (hStatus == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
      }

      hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
        0, 0, 100, 100, hwnd, (HMENU)IDC_MAIN_EDIT, GetModuleHandle(NULL), NULL);
      if(hEdit == NULL)
        MessageBox(hwnd, "Could not create edit box.", "Error", MB_OK | MB_ICONERROR);

      hfDefault = (HFONT)GetStockObject(ANSI_FIXED_FONT);
      SendMessage(hEdit, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));

      if (workingFileName!= NULL && workingFileName[0]!=0) {
        LoadTextFileToEdit(hEdit, workingFileName);
      }
      SetTitleBar(hwnd);
      SetFocus(hEdit);
    }
    break;
    case WM_SIZE:
    {
      HWND hEdit;
      RECT rcClient;
      HWND hStatus;
      RECT rcStatus;

      hStatus = GetDlgItem(hwnd, IDC_STATUSBAR);
      SendMessage(hStatus, WM_SIZE, 0, 0);
      GetWindowRect(hStatus, &rcStatus);

      GetClientRect(hwnd, &rcClient);

      hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
      SetWindowPos(hEdit, NULL, 0, 0, rcClient.right, rcClient.bottom-(rcStatus.bottom - rcStatus.top), SWP_NOZORDER);
    }
    break;
    case WM_CLOSE:
      if (checkSave(hwnd)) return 0;
      DestroyWindow(hwnd);
    break;
    case WM_DESTROY:
      PostQuitMessage(0);
    break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case ID_FILE_EXIT:
          PostMessage(hwnd, WM_CLOSE, 0, 0);
        break;
        case ID_FILE_NEW:
          if (checkSave(hwnd)) return 0;
          SetDlgItemText(hwnd, IDC_MAIN_EDIT, "");
          sprintf(workingFileName,"");
          fileChanged=FALSE;
          SetTitleBar(hwnd);
        break;
        case ID_FILE_OPEN:
          if (checkSave(hwnd)) return 0;
          DoFileOpen(hwnd);
        break;
        case ID_FILE_SAVE:
          DoFileSave(hwnd);
        break;
        case ID_FILE_SAVEAS:
          DoFileSaveAs(hwnd);
        break;
        case ID_SELECTALL:
          SendMessage(GetDlgItem(hwnd, IDC_MAIN_EDIT), EM_SETSEL, 0, -1);
        break;
        case ID_ABOUT:
          MessageBox(hwnd,"Infrabuck\n\nA simple brainfuck compiler for windows\n\nWritten by mitxela","About infrabuck", MB_ICONINFORMATION | MB_OK);
        break;
        case IDC_MAIN_EDIT:
          if (HIWORD(wParam)==EN_CHANGE && !fileChanged) {
            fileChanged=TRUE;
            SetTitleBar(hwnd);
          }
        break;
        case ID_COMPILE:
          compile(hwnd);
        break;
      }
    break;
    case WM_ACTIVATE:
      if (LOWORD(wParam)!=WA_INACTIVE) {
        SetFocus(GetDlgItem(hwnd, IDC_MAIN_EDIT));
      }
    break;
    default:
      if (msg==WM_CTLCOLOREDIT || msg==WM_MOUSEACTIVATE) UpdateStatusBar(hwnd);

      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  WNDCLASSEX wc;
  MSG Msg;

  if (lpCmdLine != NULL && lpCmdLine[0]!=0) strcpy(workingFileName, lpCmdLine);

  wc.cbSize        = sizeof(WNDCLASSEX);
  wc.style         = 0;
  wc.lpfnWndProc   = WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon         = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(ID_ICON1),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
  wc.lpszClassName = wClass;
  wc.hIconSm       = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(ID_ICON1),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);

  if (!RegisterClassEx(&wc)) {
    MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
    return 0;
  }

  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  InitCommonControlsEx(&icex); 

  HWND hwnd = CreateWindowEx(0, wClass, ideTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

  if (hwnd == NULL) {
    MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
    return 0;
  }

  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL1));

  while (GetMessage(&Msg, NULL, 0, 0) > 0) {
    if (Msg.message == WM_KEYDOWN || Msg.message == WM_KEYUP) UpdateStatusBar(hwnd);

    if (!TranslateAccelerator(hwnd, hAccel, &Msg)) {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }
  }
  return Msg.wParam;
}
