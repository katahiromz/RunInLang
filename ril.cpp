#include <cstdlib>
#include <cstdio>

#include <string>
#include <vector>

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

void version(void)
{
#ifdef _WIN64
    puts("ril64 ver.1.3 by katahiromz");
#else
    puts("ril32 ver.1.3 by katahiromz");
#endif
}

void usage(void)
{
    puts(
        "Usage: ril32 <lang-id> <command-line>\n"
        "\n"
        "Options:\n"
        "  --help        Display this message.\n"
        "  --version     Display version info.\n"
        "  --list        List the installed languages.");
}

typedef std::wstring string_t;

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
std::vector<LANGID> g_LangIDs;

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

VOID AddLangs(VOID)
{
    // add the neutral language
    {
        LANG_ENTRY entry;
        entry.LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        entry.str = L"Neutral";
        g_langs.push_back(entry);
    }

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

BOOL DoRunInLang(HWND hwnd, LPCWSTR cmdline, LANGID wLangID, INT nCmdShow)
{
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { NULL };

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = nCmdShow;

    LPWSTR pszCmdLine = _wcsdup(cmdline);
    if (pszCmdLine == NULL)
        return FALSE;
    BOOL ret = CreateProcessW(NULL, pszCmdLine, NULL, NULL, FALSE,
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

void do_list(void)
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

    _putws(str.c_str());
}

INT ril_main(INT argc, LPWSTR *argv)
{
    _wsetlocale(LC_ALL, L"");

    if (argc <= 1)
    {
        usage();
        return -1;
    }

    if (argc == 2)
    {
        std::wstring arg = argv[1];
        if (arg[0] == '-')
        {
            if (arg == L"--help" || arg == L"-help" || arg == L"/?")
            {
                usage();
                return 0;
            }
            if (arg == L"--version" || arg == L"-version")
            {
                version();
                return 0;
            }
            if (arg == L"--list" || arg == L"-list")
            {
                do_list();
                return 0;
            }
#ifdef _WIN64
            fwprintf(stderr, L"ril64: ERROR: invalid argument - '%s'\n", arg.c_str());
#else
            fwprintf(stderr, L"ril32: ERROR: invalid argument - '%s'\n", arg.c_str());
#endif
        }
        else
        {
#ifdef _WIN64
            fwprintf(stderr, L"ril64: ERROR: invalid syntax\n");
#else
            fwprintf(stderr, L"ril32: ERROR: invalid syntax\n");
#endif
        }
        usage();
        return -1;
    }

    LANGID LangID = (LANGID)wcstoul(argv[1], NULL, 0);

    std::wstring cmdline;
    for (INT iarg = 2; iarg < argc; ++iarg)
    {
        if (cmdline.size())
            cmdline += L' ';

        auto arg = argv[iarg];
        if (wcschr(arg, L' ') || wcschr(arg, L'\t'))
        {
            cmdline += L'"';
            cmdline += arg;
            cmdline += L'"';
        }
        else
        {
            cmdline += arg;
        }
    }

    //_putws(cmdline.c_str());

    if (!DoRunInLang(NULL, cmdline.c_str(), LangID, SW_SHOWNORMAL))
    {
        DWORD dwError = GetLastError();
#ifdef _WIN64
        fwprintf(stderr, L"ril64: ERROR: Unable to execute: %s (error code: %ld)\n",
                cmdline.c_str(), dwError);
#else
        fwprintf(stderr, L"ril32: ERROR: Unable to execute: %s (error code: %ld)\n",
                cmdline.c_str(), dwError);
#endif
        return -1;
    }

    return 0;
}

int main(void)
{
    INT argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    INT ret = ril_main(argc, argv);
    LocalFree(argv);
    return ret;
}
