#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tchar.h>
#include <strsafe.h>

#include "resource.h"

#ifdef UNICODE
    typedef std::wstring string_t;
#else
    typedef std::string string_t;
#endif

HINSTANCE g_hInstance = NULL;
HICON g_hIcon = NULL, g_hIconSm = NULL;
std::vector<string_t> g_history;
std::vector<LANGID> g_LangIDs;
LPWSTR g_arg = NULL;

// structure for language information
struct LANG_ENTRY
{
    WORD LangID;    // language ID
    string_t str;   // string

    // for sorting
    bool operator<(const LANG_ENTRY& ent) const
    {
        return str < ent.str;
    }
};
std::vector<LANG_ENTRY> g_langs;

static BOOL CALLBACK
EnumLocalesProc(LPTSTR lpLocaleString)
{
    // get the locale ID from string
    LCID lcid = _tcstoul(lpLocaleString, NULL, 16);

    LANG_ENTRY entry;
    entry.LangID = LANGIDFROMLCID(lcid);    // store the language ID

    // get the localized language and country
    TCHAR sz[128] = TEXT("");
    if (lcid == 0)
        return TRUE;    // continue
    if (!GetLocaleInfo(lcid, LOCALE_SLANGUAGE, sz, _countof(sz)))
        return TRUE;    // continue
    if (!IsValidLocale(lcid, LCID_SUPPORTED))
        return TRUE;    // continue

    entry.str = sz;     // store the text

    // add it
    g_langs.push_back(entry);

    return TRUE;    // continue
}

static BOOL CALLBACK
EnumEngLocalesProc(LPTSTR lpLocaleString)
{
    // get the locale ID from string
    LCID lcid = _tcstoul(lpLocaleString, NULL, 16);

    LANG_ENTRY entry;
    entry.LangID = LANGIDFROMLCID(lcid);    // store the language ID

    // get the language and country in English
    TCHAR sz1[128] = TEXT(""), sz2[128] = TEXT("");
    if (lcid == 0)
        return TRUE;    // continue
    if (!GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, sz1, _countof(sz1)))
        return TRUE;    // continue
    if (!GetLocaleInfo(lcid, LOCALE_SENGCOUNTRY, sz2, _countof(sz2)))
        return TRUE;    // continue
    if (!IsValidLocale(lcid, LCID_SUPPORTED))
        return TRUE;    // continue

    // join them and store it
    entry.str = sz1;
    entry.str += TEXT(" (");
    entry.str += sz2;
    entry.str += TEXT(")");

    // add it
    g_langs.push_back(entry);

    return TRUE;    // continue
}

static BOOL CALLBACK
EnumUILanguagesProc(LPTSTR pszName, LONG_PTR lParam)
{
    g_LangIDs.push_back((LANGID)_tcstoul(pszName, NULL, 16));
    return TRUE;
}

BOOL DoLoadSettings(HWND hwnd)
{
    HKEY hKey;
    LONG error;

    error = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Katayama Hirofumi MZ\\RunInLang"), 0, KEY_READ, &hKey);
    if (error)
        return FALSE;

    DWORD dwCount = 0, dwLangId = 0;
    {
        DWORD dwValue;
        DWORD cbValue = sizeof(dwValue);
        error = RegQueryValueEx(hKey, TEXT("NumCmdLine"), NULL, NULL, (LPBYTE)&dwValue, &cbValue);
        if (!error)
            dwCount = dwValue;
        error = RegQueryValueEx(hKey, TEXT("LangID"), NULL, NULL, (LPBYTE)&dwValue, &cbValue);
        if (!error)
            dwLangId = dwValue;
    }

    TCHAR szName[64], szValue[1024];
    for (size_t i = 0; i < dwCount; ++i)
    {
        StringCchPrintf(szName, _countof(szName), TEXT("CmdLine-%u"), (INT)i);
        DWORD cbValue = sizeof(szValue);
        error = RegQueryValueEx(hKey, szName, NULL, NULL, (LPBYTE)szValue, &cbValue);
        if (error)
            break;

        szValue[_countof(szValue) - 1] = 0; // Avoid buffer overrun

        INT iItem = (INT)SendDlgItemMessage(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)szValue);
        g_history.push_back(szValue);
        if (i == 0)
            SendDlgItemMessage(hwnd, cmb1, CB_SETCURSEL, iItem, 0);
    }

    RegCloseKey(hKey);

    if (dwLangId != 0)
    {
        for (size_t i = 0; i < g_langs.size(); ++i)
        {
            if (g_langs[i].LangID == dwLangId)
            {
                SendDlgItemMessage(hwnd, cmb2, CB_SETCURSEL, (WPARAM)i, 0);
                break;
            }
        }
    }

    return TRUE;
}

BOOL DoSaveSettings(HWND hwnd, LPCTSTR cmdline, DWORD dwLangId)
{
    HKEY hCompanyKey, hAppKey;
    LONG error;

    for (size_t i = 0; i < g_history.size(); ++i)
    {
        if (g_history[i] == cmdline)
        {
            g_history.erase(g_history.begin() + i);
            break;
        }
    }
    g_history.insert(g_history.begin(), cmdline);

    error = RegCreateKey(HKEY_CURRENT_USER, TEXT("Software\\Katayama Hirofumi MZ"), &hCompanyKey);
    if (error)
        return FALSE;

    error = RegCreateKey(hCompanyKey, TEXT("RunInLang"), &hAppKey);
    if (error)
    {
        RegCloseKey(hCompanyKey);
        return FALSE;
    }

    DWORD dwCount = (DWORD)g_history.size();
    if (dwCount > 16)
        dwCount = 16;
    RegSetValueEx(hAppKey, TEXT("NumCmdLine"), 0, REG_DWORD, (const BYTE*)&dwCount, sizeof(dwCount));

    TCHAR szName[64];
    for (size_t i = 0; i < dwCount; ++i)
    {
        StringCchPrintf(szName, _countof(szName), TEXT("CmdLine-%u"), (INT)i);
        DWORD cbValue = (DWORD)((g_history[i].size() + 1) * sizeof(WCHAR));
        RegSetValueEx(hAppKey, szName, 0, REG_SZ, (const BYTE*)g_history[i].c_str(), cbValue);
    }

    RegSetValueEx(hAppKey, TEXT("LangID"), 0, REG_DWORD, (const BYTE*)&dwLangId, sizeof(dwLangId));

    RegCloseKey(hAppKey);
    RegCloseKey(hCompanyKey);
    return TRUE;
}

string_t LoadStringDx(INT id)
{
    TCHAR text[1024];
    text[0] = 0;
    ::LoadString(NULL, id, text, _countof(text));
    return text;
}

std::wstring WideFromAnsi(UINT codepage, LPCSTR ansi)
{
    WCHAR wide[1024];
    ::MultiByteToWideChar(codepage, 0, ansi, -1, wide, _countof(wide));
    wide[_countof(wide) - 1] = 0; // Avoid buffer overrun
    return wide;
}

std::string AnsiFromWide(UINT codepage, LPCWSTR wide)
{
    char ansi[1024];
    ::WideCharToMultiByte(codepage, 0, wide, -1, ansi, _countof(ansi), NULL, NULL);
    ansi[_countof(ansi) - 2] = ansi[_countof(ansi) - 1] = 0; // Avoid buffer overrun
    return ansi;
}

BOOL GetPathOfShortcut(HWND hWnd, LPCTSTR pszLnkFile, LPTSTR pszPath)
{
    TCHAR           szPath[MAX_PATH];
#ifndef UNICODE
    WCHAR           wsz[MAX_PATH];
#endif
    IShellLink*     pShellLink;
    IPersistFile*   pPersistFile;
    WIN32_FIND_DATA find;
    BOOL            bRes = FALSE;

    szPath[0] = '\0';
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = CoCreateInstance(CLSID_ShellLink, NULL, 
            CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&pShellLink)))
        {
            if (SUCCEEDED(hr = pShellLink->QueryInterface(IID_IPersistFile, 
                (VOID **)&pPersistFile)))
            {
#ifndef UNICODE
                MultiByteToWideChar(CP_ACP, 0, pszLnkFile, -1, wsz, MAX_PATH);
                hr = pPersistFile->Load(wsz, STGM_READ);
#else
                hr = pPersistFile->Load(pszLnkFile,  STGM_READ);
#endif
                if (SUCCEEDED(hr))
                {
                    if (SUCCEEDED(hr = pShellLink->GetPath(szPath, _countof(szPath), &find, 0)))
                    {
                        if ('\0' != szPath[0])
                        {
                            StringCchCopy(pszPath, MAX_PATH, szPath);
                            bRes = TRUE;
                        }
                    }
                }
                pPersistFile->Release();
            }
            pShellLink->Release();
        }
        CoUninitialize();
    }
    return bRes;
}

VOID AddLangs(VOID)
{
    // add the neutral language
    {
        LANG_ENTRY entry;
        entry.LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        entry.str = LoadStringDx(IDS_NEUTRAL);
        g_langs.push_back(entry);
    }

    // enumerate localized languages
    EnumSystemLocales(EnumLocalesProc, LCID_INSTALLED);
    // enumerate English languages
    EnumSystemLocales(EnumEngLocalesProc, LCID_INSTALLED);
    // enumerate UI languages
    EnumUILanguages(EnumUILanguagesProc, 0, 0);

    // Erase non-UI languages
    size_t i = g_langs.size();
    while (i > 0)
    {
        --i;
        if (g_langs[i].LangID == 0) // Neutral
            continue;

        bool found = false;
        for (size_t k = 0; k < g_LangIDs.size(); ++k)
        {
            if (g_LangIDs[k] == g_langs[i].LangID)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            g_langs.erase(g_langs.begin() + i);
        }
    }
}

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    ::DragAcceptFiles(hwnd, TRUE);

    g_hIcon = ::LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    g_hIconSm = (HICON)::LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_MAINICON),
        IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON),
        0);

    // Set dialog icons
    ::SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon);
    ::SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSm);

    AddLangs();

    // Set languages to cmb2
    HWND hCmb2 = GetDlgItem(hwnd, cmb2);
    for (size_t i = 0; i < g_langs.size(); ++i)
    {
        ComboBox_AddString(hCmb2, g_langs[i].str.c_str());
    }

    // Set default language
    for (size_t i = 0; i < g_langs.size(); ++i)
    {
        if (g_langs[i].LangID == GetUserDefaultLangID())
        {
            ComboBox_SetCurSel(hCmb2, (INT)i);
            break;
        }
    }

    // Enable AutoComplete
    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    COMBOBOXINFO ComboInfo = { sizeof(ComboInfo) };
    ::GetComboBoxInfo(hCmb1, &ComboInfo);
    HWND hwndEdit = ComboInfo.hwndItem;
    SHAutoComplete(hwndEdit, SHACF_FILESYSTEM | SHACF_FILESYS_ONLY | SHACF_URLALL);

    DoLoadSettings(hwnd);

    if (g_arg)
    {
        WCHAR szPath[MAX_PATH];
        if (lstrcmpiW(PathFindExtension(g_arg), L".LNK") == 0 &&
            GetPathOfShortcut(hwnd, g_arg, szPath))
        {
            SetDlgItemTextW(hwnd, cmb1, szPath);
        }
        else
        {
            SetDlgItemTextW(hwnd, cmb1, g_arg);
        }
    }

    return TRUE;
}

typedef LANGID (WINAPI *FN_SetLang)(LANGID);

BOOL IsWindowsVistaOrLater(VOID)
{
    OSVERSIONINFO osver = { sizeof(osver) };
    return (GetVersionEx(&osver) && osver.dwMajorVersion >= 6);
}

FN_SetLang GetLangProc(void)
{
    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32"));
    FN_SetLang fn;
    if (IsWindowsVistaOrLater())
        fn = (FN_SetLang)GetProcAddress(hKernel32, "SetThreadUILanguage");
    else
        fn = (FN_SetLang)GetProcAddress(hKernel32, "SetThreadLocale");
    return fn;
}

BOOL SetLangToThread(HANDLE hThread, LANGID wLangID)
{
    BOOL ret = FALSE;
    FN_SetLang fn = GetLangProc();
    if (fn)
        ret = (BOOL)QueueUserAPC((PAPCFUNC)fn, hThread, wLangID);

    return ret;
}

BOOL DoRunInLang(HWND hwnd, LPCTSTR cmdline, LANGID wLangID, INT nCmdShow)
{
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi = { NULL };

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = nCmdShow;

    LPTSTR pszCmdLine = _tcsdup(cmdline);
    if (pszCmdLine == NULL)
        return FALSE;
    BOOL ret = CreateProcess(NULL, pszCmdLine, NULL, NULL, FALSE,
                             CREATE_SUSPENDED | CREATE_NEW_CONSOLE,
                             NULL, NULL, &si, &pi);
    free(pszCmdLine);

    if (!ret)
        return FALSE;

    SetLangToThread(pi.hThread, wLangID);

    ResumeThread(pi.hThread);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return TRUE;
}

BOOL OnOK(HWND hwnd)
{
    // Get text from cmb1
    TCHAR szCmdLine[1024];
    GetDlgItemText(hwnd, cmb1, szCmdLine, _countof(szCmdLine));
    StrTrim(szCmdLine, TEXT(" \t\r\n"));

    LANGID LangID = 0;
    INT iLang = (INT)SendDlgItemMessage(hwnd, cmb2, CB_GETCURSEL, 0, 0);
    if (0 <= iLang && iLang < (INT)g_langs.size())
        LangID = g_langs[iLang].LangID;

    if (!DoRunInLang(hwnd, szCmdLine, LangID, SW_SHOWNORMAL))
    {
        string_t strError = LoadStringDx(IDS_FAILTORUN);
        MessageBox(hwnd, strError.c_str(), NULL, MB_ICONERROR);
        return FALSE;
    }

    DoSaveSettings(hwnd, szCmdLine, LangID);

    return TRUE;
}

void OnBrowse(HWND hwnd)
{
    // Get combobox text
    TCHAR szCmdLine[1024];
    GetDlgItemText(hwnd, cmb1, szCmdLine, _countof(szCmdLine));
    StrTrim(szCmdLine, TEXT(" \t\r\n"));

    if (!PathFileExists(szCmdLine))
        szCmdLine[0] = 0;

    // Ask the user for command line
    OPENFILENAME ofn = { sizeof(ofn), hwnd };

    string_t strFilter = LoadStringDx(IDS_OPENFILTER);
    for (size_t ich = 0; ich < strFilter.size(); ++ich)
    {
        if (strFilter[ich] == TEXT('|'))
            strFilter[ich] = 0;
    }
    ofn.lpstrFilter = &strFilter[0];

    ofn.lpstrFile = szCmdLine;
    ofn.nMaxFile = _countof(szCmdLine);
    string_t strTitle = LoadStringDx(IDS_RUNTITLE);
    ofn.lpstrTitle = strTitle.c_str();

    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = TEXT("EXE");

    if (GetOpenFileName(&ofn))
    {
        // Quote
        string_t str = TEXT("\"");
        str += szCmdLine;
        str += TEXT('\"');

        // Set the text
        ComboBox_SetText(GetDlgItem(hwnd, cmb1), str.c_str());
    }
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        if (OnOK(hwnd))
        {
            EndDialog(hwnd, id);
        }
        break;
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    case psh1: // Browse...
        OnBrowse(hwnd);
        break;
    }
}

void OnDropFiles(HWND hwnd, HDROP hdrop)
{
    TCHAR szPath[MAX_PATH];
    DragQueryFile(hdrop, 0, szPath, _countof(szPath));

    // Check if shortcut
    if (lstrcmpi(PathFindExtension(szPath), TEXT(".LNK")) == 0)
    {
        TCHAR szResolvedPath[MAX_PATH];
        if (GetPathOfShortcut(hwnd, szPath, szResolvedPath))
        {
            StringCchCopy(szPath, _countof(szPath), szResolvedPath);
        }
    }

    // Quote
    string_t str = TEXT("\"");
    str += szPath;
    str += TEXT('\"');

    SetDlgItemText(hwnd, cmb1, str.c_str());
    DragFinish(hdrop);
}

void OnDestroy(HWND hwnd)
{
    if (g_hIcon)
    {
        ::DestroyIcon(g_hIcon);
        g_hIcon = NULL;
    }
    if (g_hIconSm)
    {
        ::DestroyIcon(g_hIconSm);
        g_hIconSm = NULL;
    }
}

INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
    }
    return 0;
}

VOID Usage(VOID)
{
    string_t title = LoadStringDx(IDS_APPNAME);
    string_t text = LoadStringDx(IDS_USAGE);
    MessageBox(NULL, text.c_str(), title.c_str(), MB_ICONINFORMATION);
}

VOID Version(VOID)
{
    string_t title = LoadStringDx(IDS_APPNAME);
    string_t text = LoadStringDx(IDS_VERSIONSTRING);
    MessageBox(NULL, text.c_str(), title.c_str(), MB_ICONINFORMATION);
}

VOID DoList(HWND hwnd)
{
    AddLangs();

    string_t str;
    TCHAR szText[256];
    for (size_t i = 0; i < g_langs.size(); ++i)
    {
        StringCchPrintf(szText, _countof(szText), TEXT("0x%X - %s\n"),
                        g_langs[i].LangID, g_langs[i].str.c_str());
        str += szText;
    }

    MessageBox(hwnd, str.c_str(), LoadStringDx(IDS_APPNAME).c_str(), MB_ICONINFORMATION);
}

INT ParseCommandLine(INT argc, LPWSTR *argv, INT nCmdShow)
{
    if (argc >= 2)
    {
        if (lstrcmpiW(argv[1], L"--help") == 0 ||
            lstrcmpiW(argv[1], L"-help") == 0 ||
            lstrcmpiW(argv[1], L"/?") == 0)
        {
            Usage();
            return 0;
        }
        if (lstrcmpiW(argv[1], L"--version") == 0 ||
            lstrcmpiW(argv[1], L"-version") == 0)
        {
            Version();
            return 0;
        }
        if (lstrcmpiW(argv[1], L"--list") == 0 ||
            lstrcmpiW(argv[1], L"-list") == 0)
        {
            DoList(NULL);
            return 0;
        }
    }

    if (argc <= 2)
    {
        return -1;
    }

    LANGID LangID = (LANGID)_tcstoul(argv[1], NULL, 0);
    string_t cmdline;
    for (INT iarg = 2; iarg < argc; ++iarg)
    {
        if (iarg != 2)
            cmdline += L' ';

        LPWSTR arg = argv[iarg];
        if (wcschr(arg, L' ') || wcschr(arg, L'\t'))
        {
            cmdline += L'\"';
            cmdline += arg;
            cmdline += L'\"';
        }
        else
        {
            cmdline += arg;
        }
    }

    if (!DoRunInLang(NULL, cmdline.c_str(), LangID, nCmdShow))
        return -1;

    return 0;
}

INT RunInLang_Main(HINSTANCE hInstance, INT nCmdShow, INT argc, LPWSTR *argv)
{
    g_arg = NULL;
    if (argc >= 2)
    {
        if (argc != 2 || argv[1][0] == L'/' || argv[1][0] == L'-')
            return ParseCommandLine(argc, argv, nCmdShow);

        g_arg = argv[1];
    }

    g_hInstance = hInstance;
    InitCommonControls();
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DialogProc);
    return 0;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    INT argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    RunInLang_Main(hInstance, nCmdShow, argc, argv);
    LocalFree(argv);
    return 0;
}
