//***************************
//
// TaskMan - Task Manager
// Mock version of windows Task Manager
// (C)Todd Withers
// 
//***************************

#define _WIN32_IE 0x300

#include <windows.h>
#include <commctrl.h>
#include <tlhelp32.h>

#define IDLISTVIEW 100
#define IDREFRESH  101
#define IDENDPROC  102

LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

HINSTANCE hInst;
HWND hList, hRef, hEnd;

char szClassName[ ] = "TaskMan";

RECT rc;

int iSelect = -1;

void GetProcs(HWND hView);
void AddIndex(HWND hView, int Index);
void AddRow(HWND hView, LPSTR text, int Index, int Col);
void AddColumn(HWND hView, LPSTR text, int Col, int Width, DWORD dStyle);

BOOL EnablePriv(LPCSTR lpszPriv);

int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil)
{
    HWND hwnd;
    MSG messages;
    WNDCLASSEX wincl;
    
    INITCOMMONCONTROLSEX icc = { 0 };
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);
    
    hInst = hThisInstance;
    
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.style = CS_DBLCLKS; 
    wincl.cbSize = sizeof (WNDCLASSEX);

    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL; 
    wincl.cbClsExtra = 0;
    wincl.cbWndExtra = 0; 
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    if (!RegisterClassEx (&wincl)){ return 0; }

    hwnd = CreateWindowEx (0, szClassName,"TaskMan The Task Manager",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,544,375,HWND_DESKTOP,NULL,hThisInstance,NULL);
    
    EnablePriv(SE_DEBUG_NAME);
    
    ShowWindow (hwnd, nFunsterStil);
    UpdateWindow(hwnd); 
    
    while (GetMessage (&messages, NULL, 0, 0))
    {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    return messages.wParam;
}


LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
        case WM_CREATE:
             {
                GetClientRect(hwnd, &rc);
                hList = CreateWindowEx(0, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | LVS_REPORT, 10, 10, (rc.right - 120), (rc.bottom - 20), hwnd, (HMENU)IDLISTVIEW, hInst, NULL); 
                ListView_SetExtendedListViewStyle(hList, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
                
                hRef = CreateWindowEx(0, "button", "Refresh", WS_CHILD | WS_VISIBLE, (rc.right - 100), 10, 90, 20, hwnd, (HMENU)IDREFRESH, hInst, NULL); 
                
                GetClientRect(hList, &rc);
                AddColumn(hList, "Image Name", 0, (rc.right / 5), LVCFMT_LEFT);
                AddColumn(hList, "Process ID", 1, (rc.right / 5), LVCFMT_CENTER);
                AddColumn(hList, "Thread Count", 2, (rc.right / 5), LVCFMT_CENTER);
                AddColumn(hList, "Parent Process ID", 3, (rc.right / 5), LVCFMT_CENTER);
                AddColumn(hList, "Priority Base", 4, (rc.right / 5), LVCFMT_CENTER);
                //AddColumn(hList, "Memory Usage", 5, (rc.right / 6), LVCFMT_RIGHT); - I Couldnt Figure This Out Yet =/
                GetProcs(hList);
             }
             break;  
        case WM_NOTIFY:
        {
           switch(LOWORD(wParam))
           {
              case IDLISTVIEW: 
                  if(((LPNMHDR)lParam)->code == NM_RCLICK)
                  {
                     iSelect = SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                     if(iSelect >= 0)
                     {
                        POINT ptCursor; GetCursorPos(&ptCursor);
                        HMENU hContextMenu = CreatePopupMenu();
                        InsertMenu(hContextMenu, 0, MF_BYCOMMAND | MF_STRING,    1, "End Process");
                        int iChoice = TrackPopupMenu(hContextMenu, TPM_TOPALIGN | TPM_RETURNCMD, ptCursor.x, ptCursor.y, 0, hwnd, NULL);
                        DestroyMenu(hContextMenu);
                        switch(iChoice){
                            case 1: 
                                 LPTSTR pszProc;
                                 ListView_GetItemText(hList, iSelect, 1, pszProc, 5);
                                 short procID = atoi(pszProc);
                                 HANDLE hProcs = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID );
                                 if(hProcs)
                                 {
                                    if(TerminateProcess(hProcs,0))
                                    {
                                       ListView_DeleteItem(hList, iSelect);
                                       iSelect = -1;
                                    }
                                 } 
                                 CloseHandle(hProcs);
                                 break;
                        }
                     }  
                  }
              break;
           }
        }
        case WM_SIZE:
             if(wParam != SIZE_MINIMIZED)
             {
                GetClientRect(hwnd, &rc);       
                MoveWindow(hList, 10, 10, (rc.right - 120), (rc.bottom - 20), TRUE);
                MoveWindow(hRef, (rc.right - 100), 10, 90, 20, TRUE);
                MoveWindow(hEnd, (rc.right - 100), 40, 90, 20, TRUE);
             }
             break;
        case WM_COMMAND:
             switch(LOWORD(wParam))
             {
                case IDREFRESH:
                     ListView_DeleteAllItems(hList);
                     GetProcs(hList);
                     break;
             }
             break;
        case WM_DESTROY:
             PostQuitMessage (0);
             break;
        default: 
             return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

void GetProcs(HWND hView)
{
     int x=0;
     char buffer[255];
     
     HANDLE hProcess, hProc;
     PROCESSENTRY32 proc = {sizeof( PROCESSENTRY32 )};
     hProcess = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
     if(!Process32First(hProcess, &proc))
     {
         CloseHandle(hProcess);
     }
     while(Process32Next(hProcess, &proc))
     {
        hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, proc.th32ProcessID );
        if(hProc != NULL)
        {
           AddIndex(hView, x);
           AddRow(hView, proc.szExeFile, x, 0);   // Name
        
           itoa(proc.th32ProcessID, buffer, 10); 
           AddRow(hView, buffer, x, 1);           // Proc ID
           
           itoa(proc.cntThreads, buffer, 10);     
           AddRow(hView, buffer, x, 2);           //Thread Count

           itoa(proc.th32ParentProcessID, buffer, 10); 
           AddRow(hView, buffer, x, 3);           // Parent Proc ID
               
           itoa(proc.pcPriClassBase, buffer, 10); 
           AddRow(hView, buffer, x, 4);           // Priority Base
           
           ListView_Update(hView, x);
           
           x++;
        }
     }
     CloseHandle(hProcess);
}
void AddIndex(HWND hView, int Index)
{
     LVITEM lv;
     memset(&lv, 0, sizeof(LVITEM));
     
     lv.iItem = Index;
     ListView_InsertItem(hList, &lv);
}
void AddRow(HWND hView, LPSTR text, int Index, int Col)
{
     ListView_SetItemText(hView, Index, Col, text);
}
void AddColumn(HWND hView, LPSTR text, int Col, int Width, DWORD dStyle)
{
     LVCOLUMN lvc = { 0 };
     memset(&lvc, 0, sizeof(LVCOLUMN));
     lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH  | LVCF_FMT;
     lvc.fmt  = dStyle;
     
     lvc.iSubItem = Col;
     lvc.cx       = Width;
     lvc.pszText  = text;
     
     ListView_InsertColumn(hView, Col, &lvc);
}

BOOL EnablePriv(LPCSTR lpszPriv)
{
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tkprivs;
    ZeroMemory(&tkprivs, sizeof(tkprivs));
    
    if(!OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken))
        return FALSE;
    
    if(!LookupPrivilegeValue(NULL, lpszPriv, &luid)){
        CloseHandle(hToken); return FALSE;
    }
    
    tkprivs.PrivilegeCount = 1;
    tkprivs.Privileges[0].Luid = luid;
    tkprivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    BOOL bRet = AdjustTokenPrivileges(hToken, FALSE, &tkprivs, sizeof(tkprivs), NULL, NULL);
    CloseHandle(hToken);
    return bRet;
}
