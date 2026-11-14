#ifndef NOVOLINE_WIN_EXTRA_H
#define NOVOLINE_WIN_EXTRA_H

#include <windows.h>

#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif
#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

int WINAPI MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr,
                               int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
int WINAPI WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr,
                               int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte,
                               LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar);
HINSTANCE WINAPI ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile,
                               LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
BOOL WINAPI SetProcessDPIAware(void);

int WINAPI SHCreateDirectoryExW(HWND, LPCWSTR, const SECURITY_ATTRIBUTES *);

UINT WINAPI PrivateExtractIconsW(LPCWSTR, int, int, int, HICON *, UINT *, UINT, UINT);

typedef void *LPITEMIDLIST;
typedef const void *LPCITEMIDLIST;
typedef struct BrowseInfoW {
    HWND hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPWSTR pszDisplayName;
    LPCWSTR lpszTitle;
    UINT ulFlags;
    void *lpfn;
    LPARAM lParam;
    int iImage;
} BROWSEINFOW;
LPITEMIDLIST WINAPI SHBrowseForFolderW(BROWSEINFOW *);
BOOL WINAPI SHGetPathFromIDListW(LPCITEMIDLIST, LPWSTR);
void WINAPI CoTaskMemFree(LPVOID);

#endif
