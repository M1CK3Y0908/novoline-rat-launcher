#ifndef NOVOLINE_WINHTTP_FLAT_H
#define NOVOLINE_WINHTTP_FLAT_H

#include <windows.h>

typedef LPVOID HINTERNET;

#define WINHTTP_ACCESS_TYPE_DEFAULT_PROXY 0
#define WINHTTP_ACCESS_TYPE_NO_PROXY 1
#define WINHTTP_NO_PROXY_NAME ((LPCWSTR)0)
#define WINHTTP_NO_PROXY_BYPASS ((LPCWSTR)0)
#define WINHTTP_NO_REFERER ((LPCWSTR)0)
#define WINHTTP_DEFAULT_ACCEPT_TYPES ((LPCWSTR *)0)
#define WINHTTP_NO_ADDITIONAL_HEADERS ((LPCWSTR)0)
#define WINHTTP_NO_REQUEST_DATA ((LPVOID)0)
#define WINHTTP_FLAG_SECURE 0x00800000
#define WINHTTP_OPTION_REDIRECT_POLICY 88
#define WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS 2
#define WINHTTP_QUERY_STATUS_CODE 19
#define WINHTTP_QUERY_FLAG_NUMBER 0x20000000

HINTERNET WINAPI WinHttpOpen(LPCWSTR pszAgentW, DWORD dwAccessType,
                             LPCWSTR pszProxyW, LPCWSTR pszProxyBypassW, DWORD dwFlags);
HINTERNET WINAPI WinHttpConnect(HINTERNET hSession, LPCWSTR pszServerName,
                                WORD nServerPort, DWORD dwReserved);
HINTERNET WINAPI WinHttpOpenRequest(HINTERNET hConnect, LPCWSTR pszVerb,
                                    LPCWSTR pszObjectName, LPCWSTR pszVersion,
                                    LPCWSTR pszReferrer, LPCWSTR *ppszAcceptTypes,
                                    DWORD dwFlags);
BOOL WINAPI WinHttpSetOption(HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, DWORD dwBufferLength);
BOOL WINAPI WinHttpSetTimeouts(HINTERNET hInternet, int nResolveTimeout, int nConnectTimeout,
                               int nSendTimeout, int nReceiveTimeout);
BOOL WINAPI WinHttpSendRequest(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength,
                               LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength,
                               DWORD_PTR dwContext);
BOOL WINAPI WinHttpReceiveResponse(HINTERNET hRequest, LPVOID lpReserved);
BOOL WINAPI WinHttpQueryHeaders(HINTERNET hRequest, DWORD dwInfoLevel, LPCWSTR pwszName,
                                LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex);
BOOL WINAPI WinHttpQueryDataAvailable(HINTERNET hRequest, LPDWORD lpdwNumberOfBytesAvailable);
BOOL WINAPI WinHttpReadData(HINTERNET hRequest, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead,
                            LPDWORD lpdwNumberOfBytesRead);
BOOL WINAPI WinHttpCloseHandle(HINTERNET hInternet);

#endif
