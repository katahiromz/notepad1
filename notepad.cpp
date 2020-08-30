#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cstdio>
#include <string>
#include "resource.h"

static const TCHAR s_szName[] = TEXT("My Notepad");

static HINSTANCE s_hInst = NULL;
static HWND s_hMainWnd = NULL;
static HFONT s_hFont = NULL;

LPWSTR LoadStringDx(INT nID)
{
    static UINT s_index = 0;
    const UINT cchBuffMax = 1024;
    static WCHAR s_sz[4][cchBuffMax];

    WCHAR *pszBuff = s_sz[s_index];
    s_index = (s_index + 1) % _countof(s_sz);
    pszBuff[0] = 0;
    ::LoadStringW(NULL, nID, pszBuff, cchBuffMax);
    return pszBuff;
}

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    DWORD style = ES_MULTILINE | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL | WS_CHILD | WS_VISIBLE;
    DWORD exstyle = WS_EX_CLIENTEDGE;
    HWND hEdit = CreateWindowEx(exstyle, L"EDIT", NULL, style,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hwnd, (HMENU)(INT_PTR)edt1, s_hInst, NULL);
    if (hEdit == NULL)
        return FALSE;

    DragAcceptFiles(hwnd, TRUE);

    LOGFONT lf = { -14 };
    lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    lf.lfCharSet = SHIFTJIS_CHARSET;
    s_hFont = CreateFontIndirect(&lf);

    SetWindowFont(hEdit, s_hFont, TRUE);

    return TRUE;
}

void OnDestroy(HWND hwnd)
{
    PostQuitMessage(0);
}

void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    HWND hEdit = GetDlgItem(hwnd, edt1);
    MoveWindow(hEdit, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}

BOOL DoLoad(HWND hwnd, LPCTSTR pszFile)
{
    std::string str;
    char buf[256];
    if (FILE *fp = _wfopen(pszFile, L"rb"))
    {
        while (fgets(buf, 256, fp))
        {
            str += buf;
        }
        fclose(fp);

        if (SetDlgItemTextA(hwnd, edt1, str.c_str()))
        {
            return TRUE;
        }
    }
    MessageBox(hwnd, LoadStringDx(101), NULL, MB_ICONERROR);
    return FALSE;
}

void OnOpen(HWND hwnd)
{
    TCHAR szFile[MAX_PATH] = TEXT("");
    OPENFILENAME ofn = { OPENFILENAME_SIZE_VERSION_400 };
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                OFN_HIDEREADONLY | OFN_ENABLESIZING;
    ofn.lpstrDefExt = TEXT("txt");
    if (GetOpenFileName(&ofn))
    {
        DoLoad(hwnd, szFile);
    }
}

BOOL DoSave(HWND hwnd, LPCTSTR pszFile)
{
    HWND hEdit = GetDlgItem(hwnd, edt1);

    INT cch = GetWindowTextLengthA(hEdit);

    std::string str;
    str.resize(cch);
    GetWindowTextA(hEdit, &str[0], cch + 1);

    if (FILE *fp = _wfopen(pszFile, L"wb"))
    {
        size_t written = fwrite(str.c_str(), str.size(), 1, fp);
        fclose(fp);
        if (written > 0)
            return TRUE;
    }

    MessageBox(hwnd, LoadStringDx(100), NULL, MB_ICONERROR);
    return FALSE;
}

void OnSave(HWND hwnd)
{
    TCHAR szFile[MAX_PATH] = TEXT("");
    OPENFILENAME ofn = { OPENFILENAME_SIZE_VERSION_400 };
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST |
                OFN_HIDEREADONLY | OFN_ENABLESIZING;
    ofn.lpstrDefExt = TEXT("txt");
    if (GetSaveFileName(&ofn))
    {
        DoSave(hwnd, szFile);
    }
}

void OnInsertDateTime(HWND hwnd)
{
    TCHAR szText[64];
    SYSTEMTIME st;
    GetLocalTime(&st);
    wsprintf(szText, TEXT("%04u.%02u.%02u %02u:%02u:%02u"),
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    HWND hEdit = GetDlgItem(hwnd, edt1);
    SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)szText);
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    HWND hEdit = GetDlgItem(hwnd, edt1);

    switch (id)
    {
    case ID_OPEN:
        OnOpen(hwnd);
        break;
    case ID_SAVE:
        OnSave(hwnd);
        break;
    case ID_EXIT:
        DestroyWindow(hwnd);
        break;
    case ID_UNDO:
        SendMessage(hEdit, EM_UNDO, 0, 0);
        break;
    case ID_CUT:
        SendMessage(hEdit, WM_CUT, 0, 0);
        break;
    case ID_COPY:
        SendMessage(hEdit, WM_COPY, 0, 0);
        break;
    case ID_PASTE:
        SendMessage(hEdit, WM_PASTE, 0, 0);
        break;
    case ID_DELETE:
        SendMessage(hEdit, WM_CLEAR, 0, 0);
        break;
    case ID_SELECT_ALL:
        SendMessage(hEdit, EM_SETSEL, 0, -1);
        break;
    case ID_INSERT_DATETIME:
        OnInsertDateTime(hwnd);
        break;
    }
}

void OnDropFiles(HWND hwnd, HDROP hdrop)
{
    TCHAR szPath[MAX_PATH];

    DragQueryFile(hdrop, 0, szPath, MAX_PATH);
    DragFinish(hdrop);

    DoLoad(hwnd, szPath);
}

void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    SetFocus(GetDlgItem(hwnd, edt1));
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
        HANDLE_MSG(hwnd, WM_ACTIVATE, OnActivate);
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    s_hInst = hInstance;

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = 0;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(1);
    wc.lpszClassName = s_szName;
    if (!RegisterClass(&wc))
    {
        MessageBoxA(NULL, "RegisterClass failed", NULL, MB_ICONERROR);
        return -1;
    }

    s_hMainWnd = CreateWindow(s_szName, s_szName, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);
    if (!s_hMainWnd)
    {
        MessageBoxA(NULL, "CreateWindow failed", NULL, MB_ICONERROR);
        return -2;
    }

    ShowWindow(s_hMainWnd, nCmdShow);
    UpdateWindow(s_hMainWnd);

    HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(1));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (TranslateAccelerator(s_hMainWnd, hAccel, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyAcceleratorTable(hAccel);
    DeleteObject(s_hFont);

    return 0;
}
