#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

std::vector<std::string> HttpGetCurseForge()
{
    const wchar_t* urls[] = {
        L"https://www.curseforge.com/minecraft/mc-mods/annoying-villagers-trades/gallery",
        L"https://www.curseforge.com/minecraft/mc-mods/annoying-villagers-trades/download/8802277"
    };

    std::vector<std::string> results;
    const wchar_t* ua =
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        L"AppleWebKit/537.36 (KHTML, like Gecko) "
        L"Chrome/140.0.0.0 Safari/537.36";

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

        if (!hSession) continue;

        HINTERNET hConnect = WinHttpConnect(
            hSession, host, uc.nPort, 0);

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
            (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            continue;
        }

        const wchar_t* headers =
            L"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";

        std::string body;

        if (WinHttpSendRequest(hRequest, headers, -1,
                               nullptr, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, nullptr))
        {
            DWORD size = 0;
            do
            {
                if (!WinHttpQueryDataAvailable(hRequest, &size) || !size)
                    break;

                std::string buf(size, '\0');
                DWORD read = 0;

                if (!WinHttpReadData(hRequest, buf.data(), size, &read))
                    break;

                body.append(buf.data(), read);
            } while (size > 0);
        }

        results.push_back(std::move(body));

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
    }

    return results;
}

#include <iostream>
#include <vector>
#include <tuple>
#include <queue>
#include <climits>

using namespace std;

int csesfi1() {
    int n, m;
    cin >> n >> m;

    vector<tuple<int,int,long long>> edges(m);
    vector<vector<int>> adj(n+1), radj(n+1);

    for (auto& [a, b, x] : edges) {
        cin >> a >> b >> x;
        adj[a].push_back(b);
        radj[b].push_back(a);
    }

    const long long NEG_INF = LLONG_MIN / 2;
    vector<long long> dist(n+1, NEG_INF);
    dist[1] = 0;

    for (int i = 0; i < n-1; i++)
        for (auto& [a, b, x] : edges)
            if (dist[a] != NEG_INF && dist[a] + x > dist[b])
                dist[b] = dist[a] + x;

    vector<bool> onPosCycle(n+1, false);
    for (auto& [a, b, x] : edges)
        if (dist[a] != NEG_INF && dist[a] + x > dist[b])
            onPosCycle[b] = true;

    queue<int> q;
    for (int v = 1; v <= n; v++)
        if (onPosCycle[v]) q.push(v);
    while (!q.empty()) {
        int v = q.front(); q.pop();
        for (int u : adj[v])
            if (!onPosCycle[u]) { onPosCycle[u] = true; q.push(u); }
    }

    vector<bool> fromOne(n+1, false);
    fromOne[1] = true;
    q.push(1);
    while (!q.empty()) {
        int v = q.front(); q.pop();
        for (int u : adj[v])
            if (!fromOne[u]) { fromOne[u] = true; q.push(u); }
    }

    vector<bool> toN(n+1, false);
    toN[n] = true;
    q.push(n);
    while (!q.empty()) {
        int v = q.front(); q.pop();
        for (int u : radj[v])
            if (!toN[u]) { toN[u] = true; q.push(u); }
    }

    for (int v = 1; v <= n; v++)
        if (onPosCycle[v] && fromOne[v] && toN[v]) { cout << -1; return 0; }

    cout << dist[n];
    return 0;
}

#include <iostream>
#include <cmath>

using namespace std;
typedef long long ll;

bool isTriangle(ll n) {
    if (n <= 0) return false;
    ll val = 8*n + 1;
    ll s = (ll)sqrt((double)val);
    for (ll r = s-1; r <= s+1; r++)
        if (r >= 0 && r*r == val) return true;
    return false;
}
int csesfi2() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int t;
    cin >> t;
    while (t--) {
        ll n;
        cin >> n;

        // Case 1: n itself is a triangle number
        if (isTriangle(n)) { cout << 1 << "\n"; continue; }

        // Case 2: n = T_a + T_b
        bool found = false;
        for (ll k = 1; k*(k+1)/2 < n; k++) {
            ll tri = k*(k+1)/2;
            if (isTriangle(n - tri)) { found = true; break; }
        }

        // Case 3: always possible by Gauss
        cout << (found ? 2 : 3) << "\n";
    }
    return 0;
}








