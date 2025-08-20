// CGPT.cpp : Uygulamanın giriş noktasını tanımlar.
// HATA DÜZELTMESİ: 'LogWhatsappDetails' tanımlayıcı hatası, fonksiyonların doğru sıralanmasıyla giderildi.
// Tüm önceki yükseltmeler (Bağlantı Testi, Akıllı Loglama, Harici JS Dosyası) korunmuştur.

#define UNICODE
#define _UNICODE

#include "resource.h" // Kaynak ID'lerini C++ koduna dahil eder
#include <windows.h>
#include <wrl.h>
#include "C:/Users/Admin/source/repos2/CGPT/include/wil/com.h" // Kullanıcının belirttiği düzeltilmiş yol
#include <WebView2.h>
#include <shlobj.h>   // SHGetFolderPathW için
#include <winhttp.h>  // WinHttp için
#include <string>
#include <vector>
#include <fstream>    // Dosya işlemleri için
#include <sstream>    // String stream işlemleri için
#include <locale>     // std::locale, std::codecvt_utf8 için
#include <codecvt>    // std::codecvt_utf8 için
#include <ctime>      // Zaman damgaları için
#include <algorithm>  // std::transform, std::remove_if için
#include <regex>      // E-posta, telefon numarası bulma için
#include <iomanip>    // std::put_time, std::setw, std::setfill için
#include <chrono>     // Sayfada geçirilen süre için
#include <map>        // std::map için eklendi

// WebView2Loader.dll.lib için doğru yolu projenize göre ayarlayın
#if defined(_M_X64)
#pragma comment(lib, "C:/Users/Admin/source/repos2/CGPT/packages/Microsoft.Web.WebView2.1.0.3296-prerelease/build/native/x64/WebView2Loader.dll.lib")
#elif defined(_M_IX86)
#pragma comment(lib, "C:/Users/Admin/source/repos2/CGPT/packages/Microsoft.Web.WebView2.1.0.3296-prerelease/build/native/x86/WebView2Loader.dll.lib")
#elif defined(_M_ARM64)
#pragma comment(lib, "C:/Users/Admin/source/repos2/CGPT/packages/Microsoft.Web.WebView2.1.0.3296-prerelease/build/native/arm64/WebView2Loader.dll.lib")
#else
#pragma comment(lib, "WebView2LoaderStatic.lib") 
#endif
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "Ole32.lib") // CoInitializeEx için eklendi

using namespace Microsoft::WRL;

// --- Kontrol ID'leri ---
#define ID_URLBAR           1000
#define ID_GO_BTN           1001
#define ID_BACK_BTN         1002
#define ID_FORWARD_BTN      1003
#define ID_DEV_BTN          1004
#define ID_UA_BTN           1005
#define ID_READER_BTN       1006
#define ID_JS_TOGGLE_BTN    1007 
#define ID_CLEAR_CACHE_BTN  1008
#define ID_LOG_BTN          1009
#define ID_JS_LOG_BTN       1010
#define ID_HTTP_LOG_BTN     1011
#define ID_IMG_LOG_BTN      1012 
#define ID_ALT_INFO_BTN     1013
#define ID_FOUND_FOR_BTN    1014
#define ID_NETLOG_BTN       1015
#define ID_CAPTURE_CHK      1016 
#define ID_GOOGLE_BTN       1017
#define ID_CHATGPT_BTN      1018
#define ID_GEMINI_BTN       1019
#define ID_STATUSBAR        2000
#define ID_TIMER_PAGE_TIME  1 

// --- WHATSAPP ID'LERİ ---
#define ID_WHATSAPP_BTN          1020
// IDD_WHATSAPP_DIALOG ve IDC_WHATSAPP_USER resource.h'dan geliyor
#define ID_WHATSAPP_LOGS_BTN     1023 // "L" butonu için
#define ID_WHATSAPP_CAPTURE_BTN  1024 // "C" butonu için

// --- Global Değişkenler ---
HWND g_hWnd = nullptr;
HWND g_hUrlBar = nullptr;
HWND g_hStatus = nullptr;
HWND g_hGoBtn = nullptr;
HWND g_hBackBtn = nullptr;
HWND g_hForwardBtn = nullptr;
HWND g_hDevBtn = nullptr;
HWND g_hUABtn = nullptr;
HWND g_hReaderBtn = nullptr;
HWND g_hJsToggleBtn = nullptr;
HWND g_hClearCacheBtn = nullptr;
HWND g_hLogBtn = nullptr;
HWND g_hJsLogBtn = nullptr;
HWND g_hHttpLogBtn = nullptr;
HWND g_hImgLogBtn = nullptr;
HWND g_hAltInfoBtn = nullptr;
HWND g_hFoundForBtn = nullptr;
HWND g_hNetLogBtn = nullptr;
HWND g_hCaptureChk = nullptr;
HWND g_hGoogleBtn = nullptr;
HWND g_hChatGPTBtn = nullptr;
HWND g_hGeminiBtn = nullptr;

// --- WHATSAPP Global Değişkenleri ---
HWND g_hWhatsAppBtn = nullptr;
HWND g_hWhatsAppLogsBtn = nullptr;
HWND g_hWhatsAppCaptureBtn = nullptr;

ComPtr<ICoreWebView2Controller> g_controller;
ComPtr<ICoreWebView2> g_webview;
ComPtr<ICoreWebView2_2> g_webview_v2;
ComPtr<ICoreWebView2Settings> g_settings;
ComPtr<ICoreWebView2Environment> g_env;

std::wstring g_appDir;

// Dosya yolları
std::wstring g_logStatusFile;
std::wstring g_jsDetailsFile;
std::wstring g_httpDetailsFile;
std::wstring g_imageLogFile;
std::wstring g_altInfoFile;
std::wstring g_foundForFile;
std::wstring g_searchForFile;
std::wstring g_netLogFile;
std::wstring g_configFile;
std::wstring g_bookmarksFile;
std::wstring g_blockListFile;
// --- WHATSAPP Dosya Yolları ---
std::wstring g_whatsappLogFile;
std::wstring g_whatsappDetailsFile;
std::wstring g_whatsappJsFile; // Yeni JS dosyası için yol

const std::wstring g_userDataFolderParent = L"C:\\Users\\Admin\\AppData\\Local\\CGPTViewer";
const std::wstring g_userDataFolder = g_userDataFolderParent + L"\\UserData";

std::wofstream g_logFileStream;
std::vector<std::wstring> g_history;
int g_historyPosition = -1;
const size_t MAX_HISTORY_SIZE = 50;

bool g_isJsGloballyActive = true;
bool g_isReaderModeActive = false;
bool g_isCaptureActive = true;

enum class UserAgentType { DEFAULT, LINUX, IPHONE, ANDROID, CUSTOM };
UserAgentType g_currentUserAgentType = UserAgentType::DEFAULT;
std::wstring g_customUserAgentString = L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.212 Safari/537.36 CGPTBrowser/1.0";

const wchar_t* UA_DEFAULT = L"";
const wchar_t* UA_WINDOWS_EDGE = L"Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 Edg/91.0.864.59";
const wchar_t* UA_LINUX_FIREFOX = L"Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:90.0) Gecko/20100101 Firefox/90.0";
const wchar_t* UA_IPHONE_SAFARI = L"Mozilla/5.0 (iPhone; CPU iPhone OS 14_7_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.2 Mobile/15E148 Safari/604.1";
const wchar_t* UA_ANDROID_CHROME = L"Mozilla/5.0 (Linux; Android 11; SM-A205U) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/92.0.4515.159 Mobile Safari/537.36";
std::vector<std::wstring> g_userAgents = { UA_DEFAULT, UA_WINDOWS_EDGE, UA_LINUX_FIREFOX, UA_IPHONE_SAFARI, UA_ANDROID_CHROME };
int g_currentUserAgentIndex = 0;

std::vector<std::wstring> g_blockList;
std::chrono::steady_clock::time_point g_pageLoadTimeStart;

// --- WHATSAPP Durum Takip Değişkenleri ---
std::wstring g_targetWhatsappUser;
bool g_isWhatsappUserOnline = false;
std::chrono::steady_clock::time_point g_whatsappOnlineStartTime;
std::wstring g_lastWhatsappActivity = L"";


// --- Yardımcı Fonksiyonlar ---

std::wstring Timestamp() {
    wchar_t buf[64];
    std::time_t t = std::time(nullptr);
    std::tm tm_info;
    localtime_s(&tm_info, &t);
    wcsftime(buf, sizeof(buf) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm_info);
    return buf;
}

void Log(const wchar_t* eventType, const wchar_t* detail, bool updateStatusBar = true) {
    if (!g_isCaptureActive) {
        return;
    }

    if (wcscmp(eventType, L"JSMessage (Generic)") == 0 &&
        (wcsstr(detail, L"HOVER_LEAVE") != nullptr || wcsstr(detail, L"HOVER_URL:javascript:") != nullptr)) {
        if (updateStatusBar && g_hStatus) {
        }
        return;
    }

    if (!g_logFileStream.is_open()) {
        g_logFileStream.open(g_logStatusFile, std::ios::out | std::ios::app);
        if (!g_logFileStream.is_open()) {
            MessageBox(g_hWnd, (L"Log dosyası açılamadı: " + g_logStatusFile).c_str(), L"Loglama Hatası", MB_ICONERROR);
            return;
        }
    }
    std::wstringstream ss;
    ss << Timestamp() << L" [" << eventType << L"] " << detail;
    std::wstring logEntry = ss.str();

    g_logFileStream << logEntry << L"\r\n";
    g_logFileStream.flush();

    if (updateStatusBar && g_hStatus) {
        SetWindowTextW(g_hStatus, logEntry.c_str());
    }
}

void WriteToSpecificLog(const std::wstring& filePath, const std::wstring& content, bool clearBeforeWrite = false, bool bypassCaptureCheck = false) {
    if (!bypassCaptureCheck && !g_isCaptureActive) return;

    std::wofstream logStream;
    std::ios_base::openmode mode = std::ios::out;
    if (clearBeforeWrite) {
        mode |= std::ios::trunc;
    }
    else {
        mode |= std::ios::app;
    }

    logStream.open(filePath, mode);

    if (logStream.is_open()) {
        logStream << Timestamp() << L": " << content << L"\r\n";
        logStream.flush();
        logStream.close();
    }
    else {
        Log(L"DosyaYazmaHatası", (L"Dosya açılamadı/yazılamadı: " + filePath).c_str());
    }
}

// --- WHATSAPP LOGLAMA FONKSİYONLARI ---

void LogWhatsappStatus(const std::wstring& statusMessage) {
    std::wofstream logStream;
    logStream.open(g_whatsappLogFile, std::ios::out | std::ios::app);
    if (logStream.is_open()) {
        logStream.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>()));
        wchar_t buf[64];
        std::time_t t = std::time(nullptr);
        std::tm tm_info;
        localtime_s(&tm_info, &t);
        wcsftime(buf, sizeof(buf) / sizeof(wchar_t), L"%d/%m/%Y - %H:%M:%S", &tm_info);
        logStream << buf << L" - " << statusMessage << L"\r\n";
        logStream.close();
    }
}

void LogWhatsappDetails(const std::wstring& detailMessage) {
    std::wofstream logStream;
    logStream.open(g_whatsappDetailsFile, std::ios::out | std::ios::app);
    if (logStream.is_open()) {
        logStream.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>()));
        wchar_t buf[64];
        std::time_t t = std::time(nullptr);
        std::tm tm_info;
        localtime_s(&tm_info, &t);
        wcsftime(buf, sizeof(buf) / sizeof(wchar_t), L"%d/%m/%Y - %H:%M:%S", &tm_info);
        logStream << buf << L" - " << detailMessage << L"\r\n";
        logStream.close();
    }
}

// DÜZELTME: Bu fonksiyon, kullandığı LogWhatsappDetails'den sonra tanımlanmalıdır.
std::wstring LoadScriptFromFile(const std::wstring& filePath) {
    std::wifstream file(filePath);
    if (!file.is_open()) {
        LogWhatsappDetails(L"[C++ HATA] Script dosyası okunamadı: " + filePath);
        return L"";
    }
    // UTF-8 dosya içeriğini doğru okumak için
    file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>()));
    std::wstringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}


// --- WHATSAPP DİYALOG PROSEDÜRÜ ---
INT_PTR CALLBACK WhatsAppDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t buffer[256];
            GetDlgItemTextW(hDlg, IDC_WHATSAPP_USER, buffer, _countof(buffer));
            g_targetWhatsappUser = buffer;
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


// ... (Mevcut diğer yardımcı fonksiyonlarınız)
std::wstring ReadIniValue(const std::wstring& filePath, const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue) {
    wchar_t buffer[512];
    GetPrivateProfileStringW(section.c_str(), key.c_str(), defaultValue.c_str(), buffer, _countof(buffer), filePath.c_str());
    return buffer;
}

void WriteIniValue(const std::wstring& filePath, const std::wstring& section, const std::wstring& key, const std::wstring& value) {
    WritePrivateProfileStringW(section.c_str(), key.c_str(), value.c_str(), filePath.c_str());
}

std::wstring LoadHomePageUrl() {
    return ReadIniValue(g_configFile, L"Browser", L"HomePage", L"https://www.google.com");
}

void SaveHomePageUrl(const std::wstring& url) {
    WriteIniValue(g_configFile, L"Browser", L"HomePage", url);
}

void SaveUserAgentPreference(int uaIndex) {
    WriteIniValue(g_configFile, L"Browser", L"UserAgentIndex", std::to_wstring(uaIndex));
}

int LoadUserAgentPreference() {
    std::wstring uaIndexStr = ReadIniValue(g_configFile, L"Browser", L"UserAgentIndex", L"0");
    try {
        return std::stoi(uaIndexStr);
    }
    catch (const std::exception&) {
        return 0;
    }
}

void UpdateStatusBar() {
    if (!g_hStatus) return;

    std::wstring uaStatus;
    if (g_currentUserAgentIndex >= 0 && static_cast<size_t>(g_currentUserAgentIndex) < g_userAgents.size()) {
        if (g_currentUserAgentIndex == 0) uaStatus = L"UA: Varsayılan (Sistem)";
        else if (g_currentUserAgentIndex == 1) uaStatus = L"UA: Win8.1 Edge";
        else if (g_currentUserAgentIndex == 2) uaStatus = L"UA: Linux";
        else if (g_currentUserAgentIndex == 3) uaStatus = L"UA: iPhone";
        else if (g_currentUserAgentIndex == 4) uaStatus = L"UA: Android";
        else uaStatus = L"UA: Bilinmiyor";
    }
    else {
        uaStatus = L"UA: Hatalı İndeks";
    }


    std::wstring jsStatus = g_isJsGloballyActive ? L"JS: Aktif" : L"JS: Pasif";
    std::wstring readerStatus = g_isReaderModeActive ? L"Okuma Modu: Aktif" : L"";
    std::wstring captureStatus = g_isCaptureActive ? L"Log: Açık" : L"Log: Kapalı";


    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - g_pageLoadTimeStart);
    long long totalSeconds = duration.count();
    long long minutes = totalSeconds / 60;
    long long seconds = totalSeconds % 60;

    wchar_t timeStr[10];
    swprintf_s(timeStr, L"%02lld:%02lld", minutes, seconds);

    std::wstringstream ss;
    ss << uaStatus << L" | " << jsStatus << L" | " << captureStatus << L" | Süre: " << timeStr;
    if (!readerStatus.empty()) {
        ss << L" | " << readerStatus;
    }

    SetWindowTextW(g_hStatus, ss.str().c_str());
}

void LoadBlockList() {
    g_blockList.clear();
    std::wifstream file(g_blockListFile);
    if (file.is_open()) {
        file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>()));
        std::wstring line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == L'#') {
                continue;
            }
            std::transform(line.begin(), line.end(), line.begin(), ::towlower);
            g_blockList.push_back(line);
        }
        file.close();
        Log(L"ReklamEngelleme", (std::to_wstring(g_blockList.size()) + L" kural yüklendi.").c_str(), false);
    }
    else {
        Log(L"ReklamEngelleme", L"Engelleme listesi bulunamadı. Varsayılanlar kullanılıyor.", false);
        const wchar_t* defaultBlockedDomains[] = {
            L"doubleclick.net", L"googlesyndication.com", L"googleadservices.com",
            L"adservice.google.com", L"pagead2.googlesyndication.com", L"taboola.com",
            L"outbrain.com", L"criteo.com", L"adsrvr.org"
        };
        for (const auto* domain : defaultBlockedDomains) {
            std::wstring lowerDomain = domain;
            std::transform(lowerDomain.begin(), lowerDomain.end(), lowerDomain.begin(), ::towlower);
            g_blockList.push_back(lowerDomain);
        }
        Log(L"ReklamEngelleme", (std::to_wstring(g_blockList.size()) + L" varsayılan kural eklendi.").c_str(), false);
    }
}

bool IsUrlBlocked(const std::wstring& url) {
    if (url.empty()) return false;
    std::wstring lowerUrl = url;
    std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), ::towlower);
    for (const auto& blockedDomain : g_blockList) {
        if (lowerUrl.find(blockedDomain) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

void ToggleReaderModeCSS(bool enable) {
    if (!g_webview) return;
    if (enable) {
        const wchar_t* readerCSS = LR"(
            body { max-width: 800px !important; margin: auto !important; font-family: sans-serif !important; line-height: 1.6 !important; font-size: 18px !important; background-image: none !important; background-color: #fdfdfd !important; color: #333 !important; }
            img, video, iframe { max-width: 100% !important; height: auto !important; display: block !important; margin-left: auto !important; margin-right: auto !important; }
            header, nav, footer, aside, .sidebar, .ad, .popup, [class*="banner"], [id*="banner"] { display: none !important; }
            * { background-image: none !important; }
        )";
        std::wstring script = L"var style = document.createElement('style'); style.id = 'readerModeStyle'; style.innerHTML = `";
        script += readerCSS;
        script += L"`; document.head.appendChild(style);";
        g_webview->ExecuteScript(script.c_str(), nullptr);
        Log(L"OkumaModu", L"CSS enjekte edildi.");
    }
    else {
        g_webview->ExecuteScript(L"var readerStyle = document.getElementById('readerModeStyle'); if (readerStyle) { readerStyle.parentNode.removeChild(readerStyle); }", nullptr);
        Log(L"OkumaModu", L"CSS kaldırıldı.");
    }
}

void SetPageJavaScriptEnabled(bool enable) {
    if (g_settings) {
        g_settings->put_IsScriptEnabled(enable);
        Log(L"OkumaModu", enable ? L"Sayfa JS etkinleştirildi." : L"Sayfa JS devre dışı bırakıldı.");
    }
}

void UpdateNavigationButtonStates() {
    if (g_webview) {
        BOOL canGoBack, canGoForward;
        g_webview->get_CanGoBack(&canGoBack);
        g_webview->get_CanGoForward(&canGoForward);
        EnableWindow(g_hBackBtn, canGoBack);
        EnableWindow(g_hForwardBtn, canGoForward);
    }
}

void AddToHistory(const std::wstring& url) {
    if (g_history.empty() || g_history.back() != url) {
        if (g_history.size() >= MAX_HISTORY_SIZE) {
            g_history.erase(g_history.begin());
        }
        g_history.push_back(url);
        g_historyPosition = static_cast<int>(g_history.size() - 1);
        UpdateNavigationButtonStates();
    }
}

void ClearAllAnalysisSpecificLogs() {
    if (!g_isCaptureActive) return;

    const wchar_t* filesToClear[] = {
        g_jsDetailsFile.c_str(), g_httpDetailsFile.c_str(), g_imageLogFile.c_str(),
        g_altInfoFile.c_str(), g_foundForFile.c_str(), g_netLogFile.c_str()
    };
    for (const auto* filePath : filesToClear) {
        std::wofstream clearStream(filePath, std::ios::out | std::ios::trunc);
        if (clearStream.is_open()) {
            clearStream.close();
        }
        else {
            Log(L"LogTemizleme", (std::wstring(L"Dosya temizlenemedi: ") + filePath).c_str());
        }
    }
    Log(L"AnalizLogları", L"Tüm detaylı analiz logları temizlendi (Capture Aktif).");
}


std::wstring GetResourceTypeString(COREWEBVIEW2_WEB_RESOURCE_CONTEXT context) {
    switch (context) {
    case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT: return L"Belge";
    case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_STYLESHEET: return L"CSS";
    case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE: return L"Resim";
    case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_MEDIA: return L"Medya";
    case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_FONT: return L"Font";
    case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_SCRIPT: return L"Script";
    case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_XML_HTTP_REQUEST: return L"XHR";
    case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_FETCH: return L"Fetch";
    case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_TEXT_TRACK: return L"AltYazı";
    default: return L"Diğer";
    }
}

std::wstring FormatBytes(long long bytes) {
    if (bytes < 0) return L"Bilinmiyor";
    if (bytes < 1024) return std::to_wstring(bytes) + L" B";
    double kb = static_cast<double>(bytes) / 1024.0;
    if (kb < 1024.0) {
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(1) << kb << L" KB";
        return ss.str();
    }
    double mb = kb / 1024.0;
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(1) << mb << L" MB";
    return ss.str();
}


std::vector<std::wstring> FindEmails(const std::wstring& text) {
    std::vector<std::wstring> emails;
    std::wregex email_regex(LR"(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b)");
    std::wsregex_iterator it(text.begin(), text.end(), email_regex);
    std::wsregex_iterator end;
    while (it != end) {
        emails.push_back(it->str());
        ++it;
    }
    return emails;
}

std::vector<std::wstring> FindPhoneNumbers(const std::wstring& text) {
    std::vector<std::wstring> phones;
    std::wregex phone_regex(LR"((?:\+?\d{1,3}[-.\s]?)?(?:\(?\d{3}\)?[-.\s]?)?\d{3}[-.\s]?\d{2}[-.\s]?\d{2}|\(?\d{3}\)?[-.\s]?\d{3}[-.\s]?\d{4})");
    std::wsregex_iterator it(text.begin(), text.end(), phone_regex);
    std::wsregex_iterator end;
    while (it != end) {
        phones.push_back(it->str());
        ++it;
    }
    return phones;
}

std::map<std::wstring, bool> CheckSocialMediaPresence(const std::wstring& pageHtml) {
    std::map<std::wstring, bool> socialMedia;
    const wchar_t* socialDomains[] = {
        L"facebook.com", L"twitter.com", L"instagram.com", L"linkedin.com", L"youtube.com",
        L"pinterest.com", L"tiktok.com", L"reddit.com", L"vk.com", L"snapchat.com"
    };
    std::wstring lowerHtml = pageHtml;
    std::transform(lowerHtml.begin(), lowerHtml.end(), lowerHtml.begin(), ::towlower);

    for (const auto* domain : socialDomains) {
        socialMedia[domain] = (lowerHtml.find(domain) != std::wstring::npos);
    }
    return socialMedia;
}

std::wstring GetResourceStatus(const std::wstring& url, bool& exists) {
    exists = false;
    std::wstring resultText = url + L" -> ";
    URL_COMPONENTSW uc{};
    wchar_t hostName_buf[256]{};
    wchar_t urlPath_buf[1024]{};

    uc.dwStructSize = sizeof(uc);
    uc.lpszHostName = hostName_buf;
    uc.dwHostNameLength = _countof(hostName_buf);
    uc.lpszUrlPath = urlPath_buf;
    uc.dwUrlPathLength = _countof(urlPath_buf);

    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.length()), 0, &uc)) {
        return resultText + L"URL Ayrıştırma Hatası";
    }

    HINTERNET hSession = WinHttpOpen(L"CGPTBrowser-HEAD-Check/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return resultText + L"WinHttpOpen Hatası";

    HINTERNET hConnect = WinHttpConnect(hSession, uc.lpszHostName, uc.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return resultText + L"WinHttpConnect Hatası";
    }

    DWORD dwOpenRequestFlags_local = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"HEAD", uc.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, dwOpenRequestFlags_local);

    if (!hRequest) {
        resultText += L"WinHttpOpenRequest Hatası";
    }
    else {
        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            resultText += L"WinHttpSendRequest Hatası";
        }
        else if (WinHttpReceiveResponse(hRequest, NULL)) {
            DWORD dwStatusCode = 0;
            DWORD dwSize = sizeof(dwStatusCode);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX)) {
                if (dwStatusCode == 200) {
                    exists = true;
                    resultText += L"Var (200 OK)";
                }
                else {
                    resultText += L"Yok veya Erişilemiyor (HTTP " + std::to_wstring(dwStatusCode) + L")";
                }
            }
            else {
                resultText += L"Başlıklar Alınamadı";
            }
        }
        else {
            resultText += L"Yanıt Alınamadı";
        }
        WinHttpCloseHandle(hRequest);
    }

    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return resultText;
}

EventRegistrationToken m_navigationStartingToken = {};
EventRegistrationToken m_navigationCompletedToken = {};
EventRegistrationToken m_webResourceRequestedToken = {};
EventRegistrationToken m_webResourceResponseReceivedToken = {};
EventRegistrationToken m_webMessageReceivedToken = {};
EventRegistrationToken m_newWindowRequestedToken = {};
EventRegistrationToken m_permissionRequestedToken = {};
EventRegistrationToken m_documentTitleChangedToken = {};
EventRegistrationToken m_historyChangedToken = {};
EventRegistrationToken m_scriptDialogOpeningToken = {};
EventRegistrationToken m_DOMContentLoadedToken = {};

void PerformPageAnalysis() {
    if (!g_webview || !g_isCaptureActive) return;

    g_webview->ExecuteScript(L"document.documentElement.lang", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
        [](HRESULT hr, LPCWSTR result) -> HRESULT {
            if (!g_isCaptureActive) return S_OK;
            if (SUCCEEDED(hr) && result && wcslen(result) > 0) {
                std::wstring lang = result;
                if (lang.length() >= 2 && lang.front() == L'"' && lang.back() == L'"') {
                    lang = lang.substr(1, lang.length() - 2);
                }
                WriteToSpecificLog(g_altInfoFile, L"Sayfa Dili (HTML lang): " + (lang.empty() ? L"Belirtilmemiş" : lang));
            }
            else {
                WriteToSpecificLog(g_altInfoFile, L"Sayfa Dili (HTML lang): Alınamadı");
            }
            return S_OK;
        }).Get());

    g_webview->ExecuteScript(L"document.body.innerText", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
        [](HRESULT hr, LPCWSTR scriptResult) -> HRESULT {
            if (!g_isCaptureActive) return S_OK;
            if (SUCCEEDED(hr) && scriptResult) {
                std::wstring pageText = scriptResult;
                if (pageText.length() >= 2 && pageText.front() == L'"' && pageText.back() == L'"') {
                    std::wstring actualText;
                    for (size_t i = 1; i < pageText.length() - 1; ++i) {
                        if (pageText[i] == L'\\' && i + 1 < pageText.length() - 1) {
                            switch (pageText[i + 1]) {
                            case L'n': actualText += L'\n'; i++; break;
                            case L'r': actualText += L'\r'; i++; break;
                            case L't': actualText += L'\t'; i++; break;
                            case L'\\': actualText += L'\\'; i++; break;
                            case L'"': actualText += L'"'; i++; break;
                            default: actualText += pageText[i]; break;
                            }
                        }
                        else {
                            actualText += pageText[i];
                        }
                    }
                    pageText = actualText;
                }


                auto emails = FindEmails(pageText);
                WriteToSpecificLog(g_altInfoFile, L"E-posta Adresleri:");
                for (const auto& email : emails) { WriteToSpecificLog(g_altInfoFile, L"  - " + email); }
                if (emails.empty()) { WriteToSpecificLog(g_altInfoFile, L"  (Bulunamadı)"); }

                auto phones = FindPhoneNumbers(pageText);
                WriteToSpecificLog(g_altInfoFile, L"Telefon Numaraları:");
                for (const auto& phone : phones) { WriteToSpecificLog(g_altInfoFile, L"  - " + phone); }
                if (phones.empty()) { WriteToSpecificLog(g_altInfoFile, L"  (Bulunamadı)"); }

                std::wifstream searchFile(g_searchForFile);
                if (searchFile.is_open()) {
                    searchFile.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>()));
                    std::wstring keyword;
                    std::wstring lowerPageText = pageText;
                    std::transform(lowerPageText.begin(), lowerPageText.end(), lowerPageText.begin(), ::towlower);
                    WriteToSpecificLog(g_foundForFile, L"Anahtar Kelime Arama Sonuçları:");
                    bool foundAnyKeyword = false;
                    while (std::getline(searchFile, keyword)) {
                        if (keyword.empty() || keyword[0] == L'#') continue;
                        std::wstring lowerKeyword = keyword;
                        std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::towlower);

                        size_t count = 0;
                        size_t pos = lowerPageText.find(lowerKeyword, 0);
                        while (pos != std::wstring::npos) {
                            count++;
                            pos = lowerPageText.find(lowerKeyword, pos + 1);
                        }
                        WriteToSpecificLog(g_foundForFile, L"  - \"" + keyword + L"\": " + std::to_wstring(count) + L" kez bulundu.");
                        if (count > 0) foundAnyKeyword = true;
                    }
                    if (!foundAnyKeyword) WriteToSpecificLog(g_foundForFile, L"  (Belirtilen anahtar kelimelerden hiçbiri bulunamadı)");
                    searchFile.close();
                }
                else {
                    WriteToSpecificLog(g_foundForFile, L"searchfor.txt dosyası bulunamadı.");
                }
            }
            else {
                WriteToSpecificLog(g_altInfoFile, L"Sayfa metni alınamadı.");
                WriteToSpecificLog(g_foundForFile, L"Sayfa metni alınamadığı için anahtar kelime aranamadı.");
            }
            return S_OK;
        }).Get());

    if (g_webview && g_isCaptureActive) {
        g_webview->ExecuteScript(LR"(
            (function() {
                let links = [];
                try {
                    let currentOrigin = location.origin;
                    document.querySelectorAll('a[href]').forEach(a => {
                        try {
                            let fullUrl = new URL(a.href, document.baseURI).href;
                            if (fullUrl.startsWith(currentOrigin) && 
                                !fullUrl.includes('#') && 
                                !fullUrl.startsWith('mailto:') && 
                                !fullUrl.startsWith('tel:') &&
                                fullUrl !== location.href) {
                                links.push(fullUrl);
                            }
                        } catch (e) { /* ignore invalid URLs */ }
                    });
                } catch (e) { /* ignore errors */ }
                return JSON.stringify(Array.from(new Set(links))); 
            })();
        )", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
    [](HRESULT hr, LPCWSTR result) -> HRESULT {
        if (!g_isCaptureActive) return S_OK;
        WriteToSpecificLog(g_altInfoFile, L"Dahili Sayfalar/Alt Sayfalar:");
        if (SUCCEEDED(hr) && result && wcslen(result) > 2) {
            std::wstring jsonLinks = result;
            if (jsonLinks == L"\"[]\"" || jsonLinks == L"[]") {
                WriteToSpecificLog(g_altInfoFile, L"  (Bulunamadı veya yok)");
                return S_OK;
            }
            if (jsonLinks.length() >= 2 && jsonLinks.front() == L'"' && jsonLinks.back() == L'"') {
                jsonLinks = jsonLinks.substr(1, jsonLinks.length() - 2);
            }
            if (jsonLinks.length() > 2 && jsonLinks.front() == L'"' && jsonLinks.back() == L'"' && jsonLinks.find(L"\",\"") == std::wstring::npos) {
                jsonLinks = jsonLinks.substr(1, jsonLinks.length() - 2);
                WriteToSpecificLog(g_altInfoFile, L"  - " + jsonLinks);
                return S_OK;
            }


            std::wstringstream ss(jsonLinks);
            std::wstring segment;
            bool found = false;
            while (std::getline(ss, segment, L',')) {
                if (segment.length() >= 2 && segment.front() == L'"' && segment.back() == L'"') {
                    segment = segment.substr(1, segment.length() - 2);
                }
                std::wstring unescapedSegment;
                for (size_t i = 0; i < segment.length(); ++i) {
                    if (segment[i] == L'\\' && i + 1 < segment.length()) {
                        if (segment[i + 1] == L'/') unescapedSegment += L'/';
                        else if (segment[i + 1] == L'\\') unescapedSegment += L'\\';
                        else if (segment[i + 1] == L'"') unescapedSegment += L'"';
                        else unescapedSegment += segment[i];
                        i++;
                    }
                    else {
                        unescapedSegment += segment[i];
                    }
                }
                if (!unescapedSegment.empty()) {
                    WriteToSpecificLog(g_altInfoFile, L"  - " + unescapedSegment);
                    found = true;
                }
            }
            if (!found) WriteToSpecificLog(g_altInfoFile, L"  (Bulunamadı veya yok)");
        }
        else {
            WriteToSpecificLog(g_altInfoFile, L"  (Alınamadı veya boş)");
        }
        return S_OK;
    }).Get());
    }


    if (g_webview && g_isCaptureActive) {
        g_webview->ExecuteScript(LR"(
            (function() {
                const socialDomains = [
                    "facebook.com", "twitter.com", "instagram.com", "linkedin.com", "youtube.com",
                    "pinterest.com", "tiktok.com", "reddit.com", "vk.com", "snapchat.com"
                ];
                let foundSocial = {};
                socialDomains.forEach(domain => foundSocial[domain] = { present: false, link: '' });
                try {
                    document.querySelectorAll('a[href]').forEach(a => {
                        try {
                            let hostname = new URL(a.href).hostname.toLowerCase(); 
                            socialDomains.forEach(domain => {
                                if (hostname.includes(domain)) {
                                    foundSocial[domain].present = true;
                                    if (!foundSocial[domain].link) foundSocial[domain].link = a.href;
                                }
                            });
                        } catch (e) { /* ignore invalid URLs */ }
                    });
                } catch (e) { /* ignore errors */ }
                return JSON.stringify(foundSocial);
            })();
        )", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
    [](HRESULT hr, LPCWSTR result) -> HRESULT {
        if (!g_isCaptureActive) return S_OK;
        WriteToSpecificLog(g_altInfoFile, L"Sosyal Medya Varlığı:");
        if (SUCCEEDED(hr) && result && wcslen(result) > 2) {
            std::wstring jsonResult = result;
            if (jsonResult.length() >= 2 && jsonResult.front() == L'"' && jsonResult.back() == L'"') {
                jsonResult = jsonResult.substr(1, jsonResult.length() - 2);
                std::wstring temp;
                for (size_t i = 0; i < jsonResult.length(); ++i) {
                    if (jsonResult[i] == L'\\' && i + 1 < jsonResult.length() && jsonResult[i + 1] == L'"') {
                        temp += L'"';
                        i++;
                    }
                    else {
                        temp += jsonResult[i];
                    }
                }
                jsonResult = temp;
            }

            const wchar_t* socialDomainsToCheck[] = {
                L"facebook.com", L"twitter.com", L"instagram.com", L"linkedin.com", L"youtube.com",
                L"pinterest.com", L"tiktok.com", L"reddit.com", L"vk.com", L"snapchat.com"
            };
            bool anyFoundOverall = false;
            for (const auto* domain : socialDomainsToCheck) {
                std::wstring domainStr = domain;
                std::wstring presentSearchTerm = L"\"" + domainStr + L"\":{\"present\":true";
                size_t domainPos = jsonResult.find(presentSearchTerm);

                if (domainPos != std::wstring::npos) {
                    anyFoundOverall = true;
                    std::wstring linkStr = L"Yok";
                    std::wstring linkSearchTerm = L"\"link\":\"";
                    size_t linkValPos = jsonResult.find(linkSearchTerm, domainPos);
                    if (linkValPos != std::wstring::npos && linkValPos < jsonResult.find(L"}", domainPos)) {
                        linkValPos += linkSearchTerm.length();
                        size_t linkEndPos = jsonResult.find(L"\"", linkValPos);
                        if (linkEndPos != std::wstring::npos) {
                            std::wstring extractedLink = jsonResult.substr(linkValPos, linkEndPos - linkValPos);
                            if (!extractedLink.empty()) {
                                linkStr = extractedLink;
                            }
                        }
                    }
                    WriteToSpecificLog(g_altInfoFile, L"  - " + domainStr + L": Var (Link: " + linkStr + L")");
                }
                else {
                    WriteToSpecificLog(g_altInfoFile, L"  - " + domainStr + L": Yok");
                }
            }
            if (!anyFoundOverall) {
                WriteToSpecificLog(g_altInfoFile, L"  (Belirtilen sosyal medya platformlarından link bulunamadı)");
            }

        }
        else {
            WriteToSpecificLog(g_altInfoFile, L"  (Alınamadı veya boş)");
        }
        return S_OK;
    }).Get());
    }


    PWSTR uri_pwstr = nullptr;
    if (g_webview && SUCCEEDED(g_webview->get_Source(&uri_pwstr))) {
        std::wstring currentUrl = uri_pwstr ? uri_pwstr : L"";
        CoTaskMemFree(uri_pwstr);

        if (g_isCaptureActive) {
            bool isHttps = (currentUrl.rfind(L"https://", 0) == 0);
            WriteToSpecificLog(g_altInfoFile, L"HTTPS Kullanımı: " + std::wstring(isHttps ? L"Evet" : L"Hayır (GÜVENSİZ!)"));
            if (!isHttps && !currentUrl.empty()) {
                WriteToSpecificLog(g_netLogFile, L"Uyarı: Güvensiz (HTTP) bir sayfaya gidiliyor: " + currentUrl);
            }


            std::wstring robotsUrl = L"", securityUrl = L"";
            URL_COMPONENTSW uc{};
            wchar_t scheme_buf[16]{}, hostName_buf[256]{};
            uc.dwStructSize = sizeof(uc);
            uc.lpszScheme = scheme_buf; uc.dwSchemeLength = _countof(scheme_buf);
            uc.lpszHostName = hostName_buf; uc.dwHostNameLength = _countof(hostName_buf);

            if (!currentUrl.empty() && WinHttpCrackUrl(currentUrl.c_str(), static_cast<DWORD>(currentUrl.length()), ICU_DECODE, &uc)) {
                robotsUrl = std::wstring(uc.lpszScheme) + L"://" + uc.lpszHostName + L"/robots.txt";
                securityUrl = std::wstring(uc.lpszScheme) + L"://" + uc.lpszHostName + L"/.well-known/security.txt";
            }

            if (!robotsUrl.empty()) {
                bool robotsExists = false;
                std::wstring robotsStatus = GetResourceStatus(robotsUrl, robotsExists);
                WriteToSpecificLog(g_altInfoFile, L"robots.txt Durumu: " + robotsStatus);
            }
            else { WriteToSpecificLog(g_altInfoFile, L"robots.txt Durumu: Ana URL ayrıştırılamadı."); }

            if (!securityUrl.empty()) {
                bool securityExists = false;
                std::wstring securityStatus = GetResourceStatus(securityUrl, securityExists);
                WriteToSpecificLog(g_altInfoFile, L"security.txt Durumu: " + securityStatus);
            }
            else { WriteToSpecificLog(g_altInfoFile, L"security.txt Durumu: Ana URL ayrıştırılamadı."); }
        }
    }
}


void InitializeWebView() {
    Log(L"WebView-Env", L"Ortam oluşturuluyor...");

    if (CreateDirectoryW(g_userDataFolderParent.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (CreateDirectoryW(g_userDataFolder.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS) {
            Log(L"WebView-Env", (L"Kullanıcı veri klasörü: " + g_userDataFolder).c_str());
        }
        else {
            Log(L"WebView-Env", (L"Kullanıcı veri alt klasörü oluşturulamadı: " + g_userDataFolder + L" Hata: " + std::to_wstring(GetLastError())).c_str());
        }
    }
    else {
        Log(L"WebView-Env", (L"Kullanıcı veri ana klasörü oluşturulamadı: " + g_userDataFolderParent + L" Hata: " + std::to_wstring(GetLastError())).c_str());
    }


    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, g_userDataFolder.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result)) {
                    Log(L"WebView-Env", L"Ortam oluşturma başarısız.");
                    MessageBox(g_hWnd, L"WebView2 ortamı oluşturulamadı. WebView2 Runtime yüklü ve güncel mi?", L"Başlatma Hatası", MB_ICONERROR);
                    PostMessage(g_hWnd, WM_CLOSE, 0, 0);
                    return result;
                }
                Log(L"WebView-Env", L"Ortam başarıyla oluşturuldu.");
                g_env = env;

                g_env->CreateCoreWebView2Controller(g_hWnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result)) {
                                Log(L"WebView-Ctrl", L"Controller oluşturma başarısız.");
                                return result;
                            }
                            Log(L"WebView-Ctrl", L"Controller başarıyla oluşturuldu.");
                            g_controller = controller;
                            g_controller->get_CoreWebView2(&g_webview);
                            if (g_webview) {
                                g_webview->QueryInterface(IID_PPV_ARGS(&g_webview_v2));
                            }


                            if (g_webview) g_webview->get_Settings(&g_settings);
                            if (g_settings) {
                                g_settings->put_IsScriptEnabled(g_isJsGloballyActive);
                                g_settings->put_AreDevToolsEnabled(TRUE);
                                g_settings->put_IsZoomControlEnabled(TRUE);
                                g_settings->put_IsBuiltInErrorPageEnabled(TRUE);

                                wil::com_ptr<ICoreWebView2Settings2> settings2;
                                if (SUCCEEDED(g_settings->QueryInterface(IID_PPV_ARGS(&settings2)))) {
                                    g_currentUserAgentIndex = LoadUserAgentPreference();
                                    if (g_currentUserAgentIndex >= 0 && static_cast<size_t>(g_currentUserAgentIndex) < g_userAgents.size()) {
                                        settings2->put_UserAgent(g_userAgents[g_currentUserAgentIndex].c_str());
                                    }
                                    else {
                                        g_currentUserAgentIndex = 0;
                                        settings2->put_UserAgent(UA_DEFAULT);
                                    }
                                    Log(L"UserAgent", (L"Başlangıç UserAgent: " + (g_userAgents[g_currentUserAgentIndex].empty() ? L"Varsayılan (Sistem)" : g_userAgents[g_currentUserAgentIndex].substr(0, 30) + L"...")).c_str());
                                }
                            }

                            RECT bounds;
                            GetClientRect(g_hWnd, &bounds);
                            bounds.top += 60;
                            bounds.bottom -= 20;
                            if (g_controller) g_controller->put_Bounds(bounds);
                            Log(L"BoundsSet", L"WebView sınırları ayarlandı.");

                            if (!g_webview) {
                                Log(L"WebView-Init", L"g_webview null olduğu için event handler'lar eklenemedi.");
                                return E_FAIL;
                            }

                            // --- Event Handler'lar ---
                            g_webview->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
                                [](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                    PWSTR uri_pwstr = nullptr;
                                    args->get_Uri(&uri_pwstr);
                                    std::wstring uri = uri_pwstr ? uri_pwstr : L"";
                                    CoTaskMemFree(uri_pwstr);
                                    Log(L"NavStarting", uri.c_str());
                                    SetWindowTextW(g_hUrlBar, uri.c_str());

                                    ClearAllAnalysisSpecificLogs();

                                    if (IsUrlBlocked(uri)) {
                                        args->put_Cancel(true);
                                        Log(L"ReklamEngelleme", (L"Engellendi: " + uri).c_str());
                                        WriteToSpecificLog(g_netLogFile, L"Uyarı: Bilinen bir izleme/reklam alan adına yapılan istek engellendi: " + uri);
                                    }
                                    UpdateStatusBar();
                                    g_pageLoadTimeStart = std::chrono::steady_clock::now();
                                    SetTimer(g_hWnd, ID_TIMER_PAGE_TIME, 1000, nullptr);
                                    return S_OK;
                                }).Get(), &m_navigationStartingToken);

                            g_webview->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                [](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                                    BOOL success;
                                    args->get_IsSuccess(&success);
                                    COREWEBVIEW2_WEB_ERROR_STATUS errorStatus;
                                    args->get_WebErrorStatus(&errorStatus);

                                    PWSTR uri_pwstr = nullptr;
                                    if (sender) sender->get_Source(&uri_pwstr);
                                    std::wstring currentUrl = uri_pwstr ? uri_pwstr : L"";
                                    CoTaskMemFree(uri_pwstr);

                                    if (success) {
                                        Log(L"NavCompleted", (L"Başarılı: " + currentUrl).c_str());
                                        AddToHistory(currentUrl);
                                        if (g_isCaptureActive) PerformPageAnalysis();
                                    }
                                    else {
                                        wchar_t errorMsg[256];
                                        swprintf_s(errorMsg, L"Başarısız. Hata: %d, URL: %s", errorStatus, currentUrl.c_str());
                                        Log(L"NavCompleted", errorMsg);
                                    }
                                    UpdateNavigationButtonStates();
                                    UpdateStatusBar();

                                    if (g_isReaderModeActive) {
                                        ToggleReaderModeCSS(true);
                                        SetPageJavaScriptEnabled(false);
                                    }
                                    else {
                                        if (g_settings) g_settings->put_IsScriptEnabled(g_isJsGloballyActive);
                                    }

                                    // WhatsApp sayfasına gelindiğinde detay loguna bilgi yaz
                                    if (currentUrl.find(L"web.whatsapp.com") != std::wstring::npos && !g_targetWhatsappUser.empty()) {
                                        LogWhatsappDetails(L"[C++] WhatsApp Web sayfası yüklendi. Takibi başlatmak için sohbeti açıp 'C' tuşuna basın.");
                                    }

                                    return S_OK;
                                }).Get(), &m_navigationCompletedToken);

                            g_webview->add_WebResourceRequested(Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                                [](ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {

                                    wil::com_ptr<ICoreWebView2WebResourceRequest> req;
                                    args->get_Request(&req);
                                    PWSTR uri_pwstr = nullptr;
                                    if (req) req->get_Uri(&uri_pwstr);
                                    std::wstring uri = uri_pwstr ? uri_pwstr : L"";
                                    CoTaskMemFree(uri_pwstr);

                                    PWSTR method_pwstr = nullptr;
                                    if (req) req->get_Method(&method_pwstr);
                                    std::wstring method = method_pwstr ? method_pwstr : L"";
                                    CoTaskMemFree(method_pwstr);

                                    COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext;
                                    args->get_ResourceContext(&resourceContext);

                                    if (IsUrlBlocked(uri)) {
                                        wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                                        if (g_env) {
                                            g_env->CreateWebResourceResponse(nullptr, 403, L"Blocked", L"", &response);
                                            args->put_Response(response.get());
                                        }
                                        Log(L"KaynakEngellendi", uri.c_str());
                                        if (g_isCaptureActive) WriteToSpecificLog(g_netLogFile, L"Uyarı: Bilinen bir izleme/reklam alt kaynağı engellendi: " + uri);
                                        return S_OK;
                                    }

                                    if (g_isCaptureActive) {
                                        std::wstring resourceTypeStr = GetResourceTypeString(resourceContext);
                                        std::wstring shortUri = uri;
                                        if (shortUri.length() > 70) shortUri = shortUri.substr(0, 67) + L"...";

                                        if (resourceContext == COREWEBVIEW2_WEB_RESOURCE_CONTEXT_SCRIPT) {
                                            WriteToSpecificLog(g_jsDetailsFile, L"Script Yükleme İsteği: [" + method + L"] " + uri);
                                        }
                                        else if (resourceContext == COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT ||
                                            resourceContext == COREWEBVIEW2_WEB_RESOURCE_CONTEXT_FETCH ||
                                            resourceContext == COREWEBVIEW2_WEB_RESOURCE_CONTEXT_XML_HTTP_REQUEST) {
                                            if (uri.find(L"api") != std::wstring::npos || uri.find(L"data") != std::wstring::npos || uri.find(L"service") != std::wstring::npos || resourceContext == COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT) {
                                                WriteToSpecificLog(g_httpDetailsFile, L"HTTP/Veri İstek: [" + method + L"] " + shortUri);
                                            }
                                        }
                                        else if (resourceContext != COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE) {
                                            WriteToSpecificLog(g_httpDetailsFile, L"Diğer Kaynak: [" + resourceTypeStr + L"] " + shortUri);
                                        }

                                        if (resourceContext == COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT) {
                                            WriteToSpecificLog(g_netLogFile, L"Ana doküman isteniyor: " + uri);
                                        }
                                        else if (uri.find(L"favicon.ico") != std::wstring::npos) {
                                            WriteToSpecificLog(g_netLogFile, L"Web sitesi favicon.ico istedi: " + uri);
                                        }
                                    }

                                    PWSTR mainDocUriPwstr = nullptr;
                                    if (sender && SUCCEEDED(sender->get_Source(&mainDocUriPwstr))) {
                                        std::wstring mainDocUri = mainDocUriPwstr ? mainDocUriPwstr : L"";
                                        CoTaskMemFree(mainDocUriPwstr);
                                        if (mainDocUri.rfind(L"https://", 0) == 0 && uri.rfind(L"http://", 0) == 0) {
                                            Log(L"Karışıkİçerik", (L"Tespit edildi: " + uri).c_str());
                                            if (g_isCaptureActive) {
                                                WriteToSpecificLog(g_altInfoFile, L"Karışık İçerik Tespit Edildi: Güvensiz kaynak (" + uri + L") HTTPS sayfasında yükleniyor.");
                                                WriteToSpecificLog(g_netLogFile, L"Uyarı: HTTPS sayfada karışık içerik tespit edildi. Güvensiz kaynak yüklendi: " + uri);
                                            }
                                        }
                                    }
                                    return S_OK;
                                }).Get(), &m_webResourceRequestedToken);

                            if (g_webview_v2) {
                                g_webview_v2->add_WebResourceResponseReceived(Callback<ICoreWebView2WebResourceResponseReceivedEventHandler>(
                                    [](ICoreWebView2* sender, ICoreWebView2WebResourceResponseReceivedEventArgs* args) -> HRESULT {
                                        if (!g_isCaptureActive) return S_OK;

                                        wil::com_ptr<ICoreWebView2WebResourceRequest> req;
                                        args->get_Request(&req);
                                        PWSTR uri_pwstr = nullptr;
                                        if (req) req->get_Uri(&uri_pwstr); else return S_OK;
                                        std::wstring uri = uri_pwstr ? uri_pwstr : L"";
                                        CoTaskMemFree(uri_pwstr);

                                        wil::com_ptr<ICoreWebView2WebResourceResponseView> respView;
                                        args->get_Response(&respView);
                                        if (!respView) return S_OK;

                                        int statusCode = 0;
                                        respView->get_StatusCode(&statusCode);

                                        std::wstring guessedResourceType = L"Kaynak";
                                        if (uri.find(L".js") != std::wstring::npos) guessedResourceType = L"Script";
                                        else if (uri.find(L".css") != std::wstring::npos) guessedResourceType = L"CSS";
                                        else if (uri.find(L".jpg") != std::wstring::npos || uri.find(L".jpeg") != std::wstring::npos || uri.find(L".png") != std::wstring::npos || uri.find(L".gif") != std::wstring::npos || uri.find(L".svg") != std::wstring::npos || uri.find(L".webp") != std::wstring::npos) guessedResourceType = L"Resim";
                                        else if (uri.find(L".woff") != std::wstring::npos || uri.find(L".ttf") != std::wstring::npos) guessedResourceType = L"Font";
                                        else if (uri.find(L"api") != std::wstring::npos || uri.find(L"fetch") != std::wstring::npos || uri.find(L"xmlhttprequest") != std::wstring::npos) guessedResourceType = L"Veri (API/XHR)";
                                        else if (uri.find(L".html") != std::wstring::npos || uri.find(L".htm") != std::wstring::npos || (statusCode == 200 && uri.find(L".") == std::wstring::npos && !uri.empty() && uri.back() != L'/')) guessedResourceType = L"Belge";


                                        wil::com_ptr<ICoreWebView2HttpResponseHeaders> headers;
                                        respView->get_Headers(&headers);

                                        long long contentLengthBytes = -1;
                                        std::wstring contentLengthStr = L"Bilinmiyor";
                                        std::wstring contentType = L"Bilinmiyor";

                                        if (headers) {
                                            PWSTR cl_pwstr = nullptr;
                                            if (SUCCEEDED(headers->GetHeader(L"Content-Length", &cl_pwstr)) && cl_pwstr) {
                                                try { contentLengthBytes = std::stoll(cl_pwstr); }
                                                catch (...) {}
                                                CoTaskMemFree(cl_pwstr);
                                            }
                                            PWSTR ct_pwstr = nullptr;
                                            if (SUCCEEDED(headers->GetHeader(L"Content-Type", &ct_pwstr)) && ct_pwstr) {
                                                contentType = ct_pwstr;
                                                CoTaskMemFree(ct_pwstr);
                                                size_t semiColonPos = contentType.find(L';');
                                                if (semiColonPos != std::wstring::npos) contentType = contentType.substr(0, semiColonPos);
                                            }
                                        }
                                        if (contentLengthBytes != -1) contentLengthStr = FormatBytes(contentLengthBytes);

                                        std::wstring shortUri = uri;
                                        if (guessedResourceType != L"Resim" && shortUri.length() > 60) {
                                            shortUri = shortUri.substr(0, 57) + L"...";
                                        }

                                        std::wstring securityStatus = L"Bilinmiyor";
                                        if (uri.rfind(L"https://", 0) == 0) securityStatus = L"Güvenli (HTTPS)";
                                        else if (uri.rfind(L"http://", 0) == 0) securityStatus = L"Güvensiz (HTTP)";


                                        if (guessedResourceType == L"Resim") {
                                            WriteToSpecificLog(g_imageLogFile, L"[" + contentType + L"] " + uri + L", Boyut: " + contentLengthStr + L", Durum: " + std::to_wstring(statusCode));
                                        }
                                        else if (guessedResourceType == L"Veri (API/XHR)" || guessedResourceType == L"Belge" || statusCode >= 400) {
                                            WriteToSpecificLog(g_httpDetailsFile, L"[" + guessedResourceType + L"] " + shortUri + L" yanıtladı: " + std::to_wstring(statusCode) + L" (Boyut: " + contentLengthStr + L", Tip: " + contentType + L", Güvenlik: " + securityStatus + L")");
                                        }
                                        else if (guessedResourceType == L"Font" || guessedResourceType == L"CSS" || guessedResourceType == L"Medya" || (contentLengthBytes > 10 * 1024 && guessedResourceType != L"Script")) {
                                            WriteToSpecificLog(g_httpDetailsFile, L"[" + guessedResourceType + L"] " + shortUri + L" yüklendi, Boyut: " + contentLengthStr + L", Güvenlik: " + securityStatus);
                                        }


                                        if (headers && g_isCaptureActive) {
                                            const wchar_t* securityHeaders[] = {
                                                L"Content-Security-Policy", L"X-Frame-Options", L"Strict-Transport-Security",
                                                L"X-Content-Type-Options", L"Referrer-Policy", L"Permissions-Policy",
                                                L"X-XSS-Protection", L"Cross-Origin-Opener-Policy", L"Cross-Origin-Embedder-Policy",
                                                L"Cross-Origin-Resource-Policy"
                                            };
                                            WriteToSpecificLog(g_altInfoFile, L"HTTP Güvenlik Başlıkları (" + uri + L"):");
                                            for (const auto* headerName : securityHeaders) {
                                                PWSTR headerValuePwstr = nullptr;
                                                if (SUCCEEDED(headers->GetHeader(headerName, &headerValuePwstr))) {
                                                    WriteToSpecificLog(g_altInfoFile, L"  - " + std::wstring(headerName) + L": " + (headerValuePwstr ? headerValuePwstr : L""));
                                                    CoTaskMemFree(headerValuePwstr);
                                                }
                                                else {
                                                    WriteToSpecificLog(g_altInfoFile, L"  - " + std::wstring(headerName) + L": Yok");
                                                }
                                            }

                                            PWSTR poweredByPwstr = nullptr;
                                            if (SUCCEEDED(headers->GetHeader(L"X-Powered-By", &poweredByPwstr))) {
                                                WriteToSpecificLog(g_altInfoFile, L"Sayfa Teknolojisi (Sunucu - X-Powered-By): " + (poweredByPwstr ? std::wstring(poweredByPwstr) : L""));
                                                CoTaskMemFree(poweredByPwstr);
                                            }
                                            else { WriteToSpecificLog(g_altInfoFile, L"Sayfa Teknolojisi (Sunucu - X-Powered-By): Belirtilmemiş"); }

                                            WriteToSpecificLog(g_altInfoFile, L"Sayfa Boyutu (Content-Length): " + contentLengthStr);
                                            WriteToSpecificLog(g_netLogFile, L"Kaynak (" + uri + L") boyutu: " + contentLengthStr);
                                        }

                                        if (statusCode >= 300 && statusCode < 400) {
                                            PWSTR locationHeader = nullptr;
                                            if (headers && SUCCEEDED(headers->GetHeader(L"Location", &locationHeader))) {
                                                WriteToSpecificLog(g_netLogFile, L"Yönlendirme: " + uri + L" -> " + (locationHeader ? locationHeader : L"") + L" (HTTP " + std::to_wstring(statusCode) + L")");
                                                CoTaskMemFree(locationHeader);
                                            }
                                            else {
                                                WriteToSpecificLog(g_netLogFile, L"Yönlendirme: " + uri + L" (HTTP " + std::to_wstring(statusCode) + L", hedef bilinmiyor)");
                                            }
                                        }
                                        BOOL hasSetCookie = FALSE;
                                        if (headers && SUCCEEDED(headers->Contains(L"Set-Cookie", &hasSetCookie)) && hasSetCookie) {
                                            WriteToSpecificLog(g_netLogFile, L"Cookie ayarlandı (kaynak: " + uri + L")");
                                        }
                                        return S_OK;
                                    }).Get(), &m_webResourceResponseReceivedToken);
                            }
                            else {
                                Log(L"WebView-Event", L"WebResourceResponseReceived olayı eklenemedi (ICoreWebView2_2 yok).");
                            }

                            g_webview->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                [](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                    PWSTR message_pwstr = nullptr;
                                    args->TryGetWebMessageAsString(&message_pwstr);
                                    if (message_pwstr) {
                                        std::wstring message = message_pwstr;
                                        CoTaskMemFree(message_pwstr);

                                        // --- WHATSAPP MESAJ İŞLEME MANTIĞI ---
                                        if (message.rfind(L"WHATSAPP_ACTIVITY:", 0) == 0) {
                                            std::wstring activity = message.substr(18);
                                            if (activity == L"online") {
                                                if (!g_isWhatsappUserOnline) {
                                                    g_isWhatsappUserOnline = true;
                                                    g_whatsappOnlineStartTime = std::chrono::steady_clock::now();
                                                    LogWhatsappStatus(g_targetWhatsappUser + L" çevrimiçi oldu.");
                                                }
                                            }
                                            else {
                                                if (g_isWhatsappUserOnline) {
                                                    g_isWhatsappUserOnline = false;
                                                    auto endTime = std::chrono::steady_clock::now();
                                                    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - g_whatsappOnlineStartTime);
                                                    long long totalSeconds = duration.count();
                                                    long long minutes = totalSeconds / 60;
                                                    long long seconds = totalSeconds % 60;

                                                    std::wstring durationStr = L" (Süre: " + std::to_wstring(minutes) + L"dk " + std::to_wstring(seconds) + L"sn)";
                                                    LogWhatsappStatus(g_targetWhatsappUser + L" çevrimdışı oldu." + durationStr);
                                                }
                                                // "yazıyor..." gibi diğer durumları logla
                                                if (activity != g_lastWhatsappActivity && activity != L"offline") {
                                                    LogWhatsappStatus(g_targetWhatsappUser + L" durum: " + activity);
                                                }
                                            }
                                            g_lastWhatsappActivity = activity;
                                        }
                                        else if (message.rfind(L"DETAIL:", 0) == 0) {
                                            LogWhatsappDetails(L"[JS BİLGİ] " + message.substr(7));
                                        }
                                        else if (message.rfind(L"ERROR:", 0) == 0) {
                                            LogWhatsappDetails(L"[JS HATA] " + message.substr(6));
                                        }
                                        else if (message.rfind(L"LOG_SUMMARY:", 0) == 0) {
                                            LogWhatsappDetails(L"[JS RAPOR]\r\n" + message.substr(12));
                                        }

                                        // Mevcut hover mesajı mantığı
                                        const std::wstring hoverPrefix = L"HOVER_URL:";
                                        if (message.rfind(hoverPrefix, 0) == 0) {
                                            if (message.rfind(L"javascript:", hoverPrefix.length()) != 0) {
                                                std::wstring hoveredUrl = message.substr(hoverPrefix.length());
                                                if (g_hStatus) {
                                                    SetWindowTextW(g_hStatus, (L"Link: " + hoveredUrl).c_str());
                                                }
                                            }
                                            else {
                                                UpdateStatusBar();
                                            }
                                        }
                                        else if (message == L"HOVER_LEAVE") {
                                            UpdateStatusBar();
                                        }
                                    }
                                    return S_OK;
                                }).Get(), &m_webMessageReceivedToken);


                            g_webview->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                                [](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                                    PWSTR targetUriPwstr = nullptr;
                                    if (args) args->get_Uri(&targetUriPwstr); else return S_OK;
                                    std::wstring targetUri = targetUriPwstr ? targetUriPwstr : L"";
                                    CoTaskMemFree(targetUriPwstr);

                                    PWSTR sourceUriPwstr = nullptr;
                                    if (sender) sender->get_Source(&sourceUriPwstr);
                                    std::wstring sourceUri = sourceUriPwstr ? sourceUriPwstr : L"Bilinmeyen kaynak";
                                    CoTaskMemFree(sourceUriPwstr);

                                    Log(L"YeniPencere", (L"İstek: " + targetUri + L", Kaynak: " + sourceUri).c_str());
                                    WriteToSpecificLog(g_netLogFile, L"Uyarı: " + sourceUri + L" kaynaklı sayfa " + targetUri + L" adresine bir pop-up pencere açmaya çalışıyor (İşlem: Engellendi)");

                                    if (args) args->put_Handled(TRUE);
                                    return S_OK;
                                }).Get(), &m_newWindowRequestedToken);

                            g_webview->add_PermissionRequested(Callback<ICoreWebView2PermissionRequestedEventHandler>(
                                [](ICoreWebView2* sender, ICoreWebView2PermissionRequestedEventArgs* args) -> HRESULT {
                                    COREWEBVIEW2_PERMISSION_KIND kind;
                                    if (args) args->get_PermissionKind(&kind); else return S_OK;
                                    PWSTR uri_pwstr = nullptr;
                                    if (args) args->get_Uri(&uri_pwstr);
                                    std::wstring uri = uri_pwstr ? uri_pwstr : L"";
                                    CoTaskMemFree(uri_pwstr);

                                    std::wstring permissionType;
                                    switch (kind) {
                                    case COREWEBVIEW2_PERMISSION_KIND_MICROPHONE: permissionType = L"Mikrofon"; break;
                                    case COREWEBVIEW2_PERMISSION_KIND_CAMERA: permissionType = L"Kamera"; break;
                                    case COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION: permissionType = L"Konum"; break;
                                    case COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS: permissionType = L"Bildirimler"; break;
                                    case COREWEBVIEW2_PERMISSION_KIND_OTHER_SENSORS: permissionType = L"Diğer Sensörler"; break;
                                    case COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ: permissionType = L"Pano Okuma"; break;
                                    default: permissionType = L"Bilinmeyen İzin (" + std::to_wstring(kind) + L")"; break;
                                    }
                                    Log(L"İzinİsteği", (permissionType + L" için " + uri).c_str());

                                    if (kind == COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ) {
                                        WriteToSpecificLog(g_netLogFile, L"Uyarı: " + uri + L" kaynaklı sayfa panonuza erişmeye çalışıyor. (İşlem: Varsayılan/Reddedildi)");
                                    }
                                    return S_OK;
                                }).Get(), &m_permissionRequestedToken);

                            g_webview->add_DocumentTitleChanged(Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
                                [](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
                                    PWSTR title_pwstr = nullptr;
                                    if (sender) sender->get_DocumentTitle(&title_pwstr); else return S_OK;
                                    std::wstring title = title_pwstr ? title_pwstr : L"";
                                    CoTaskMemFree(title_pwstr);
                                    SetWindowTextW(g_hWnd, (L"CGPT Browser - " + title).c_str());
                                    Log(L"BaşlıkDeğişti", title.c_str(), false);
                                    return S_OK;
                                }).Get(), &m_documentTitleChangedToken);

                            g_webview->add_HistoryChanged(Callback<ICoreWebView2HistoryChangedEventHandler>(
                                [](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
                                    UpdateNavigationButtonStates();
                                    Log(L"GeçmişDeğişti", L"Navigasyon geçmişi güncellendi.", false);
                                    return S_OK;
                                }).Get(), &m_historyChangedToken);

                            g_webview->add_ScriptDialogOpening(Callback<ICoreWebView2ScriptDialogOpeningEventHandler>(
                                [](ICoreWebView2* sender, ICoreWebView2ScriptDialogOpeningEventArgs* args) -> HRESULT {
                                    COREWEBVIEW2_SCRIPT_DIALOG_KIND kind;
                                    if (args) args->get_Kind(&kind); else return S_OK;
                                    PWSTR message_pwstr = nullptr;
                                    if (args) args->get_Message(&message_pwstr);
                                    std::wstring message = message_pwstr ? message_pwstr : L"";
                                    CoTaskMemFree(message_pwstr);
                                    PWSTR uri_pwstr = nullptr;
                                    if (args) args->get_Uri(&uri_pwstr);
                                    std::wstring uri = uri_pwstr ? uri_pwstr : L"";
                                    CoTaskMemFree(uri_pwstr);

                                    std::wstring dialogType;
                                    switch (kind) {
                                    case COREWEBVIEW2_SCRIPT_DIALOG_KIND_ALERT: dialogType = L"Alert"; break;
                                    case COREWEBVIEW2_SCRIPT_DIALOG_KIND_CONFIRM: dialogType = L"Confirm"; break;
                                    case COREWEBVIEW2_SCRIPT_DIALOG_KIND_PROMPT: dialogType = L"Prompt"; break;
                                    case COREWEBVIEW2_SCRIPT_DIALOG_KIND_BEFOREUNLOAD: dialogType = L"BeforeUnload"; break;
                                    default: dialogType = L"Bilinmeyen Dialog"; break;
                                    }
                                    std::wstring logMsg = L"[" + dialogType + L"] URI: " + uri + L", Mesaj: " + message;
                                    Log(L"JSDialog", logMsg.c_str());

                                    WriteToSpecificLog(g_jsDetailsFile, L"JavaScript Dialog: " + logMsg);

                                    return S_OK;
                                }).Get(), &m_scriptDialogOpeningToken);

                            if (g_webview_v2) {
                                g_webview_v2->add_DOMContentLoaded(Callback<ICoreWebView2DOMContentLoadedEventHandler>(
                                    [](ICoreWebView2* sender, ICoreWebView2DOMContentLoadedEventArgs* args) -> HRESULT {
                                        Log(L"DOMContentLoaded", L"Sayfa DOM yüklendi.");
                                        if (sender && g_isCaptureActive) sender->ExecuteScript(LR"( 
                                            (function() {
                                                let tech = {};
                                                if (typeof jQuery !== 'undefined') tech.jQuery = jQuery.fn.jquery || 'detected';
                                                if (typeof React !== 'undefined') tech.React = (typeof React.version !== 'undefined' ? React.version : 'detected');
                                                if (typeof Vue !== 'undefined') tech.Vue = (typeof Vue.version !== 'undefined' ? Vue.version : 'detected');
                                                if (typeof angular !== 'undefined') tech.Angular = (typeof angular.version !== 'undefined' && angular.version.full ? angular.version.full : 'detected');
                                                let generatorMeta = document.querySelector('meta[name="generator"]');
                                                if (generatorMeta) tech.Generator = generatorMeta.content;
                                            })();
                                        )", nullptr);
                                        return S_OK;
                                    }).Get(), &m_DOMContentLoadedToken);
                            }
                            else { Log(L"DOMContentLoaded", L"ICoreWebView2_2 arayüzü alınamadı, DOMContentLoaded eklenemedi."); }


                            if (g_controller) g_controller->put_IsVisible(TRUE);
                            LoadBlockList();
                            std::wstring homeUrl = LoadHomePageUrl();
                            Log(L"NavigateCall", (L"Ana Sayfa: " + homeUrl).c_str());
                            if (g_webview) g_webview->Navigate(homeUrl.c_str());
                            UpdateStatusBar();
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());

    if (FAILED(hr)) {
        Log(L"WebView-Env", L"CreateCoreWebView2EnvironmentWithOptions çağrısı başarısız.");
        MessageBox(g_hWnd, L"WebView2 ortamı başlatılamadı. Lütfen WebView2 Runtime'ın kurulu ve güncel olduğundan emin olun.", L"Kritik Hata", MB_ICONERROR | MB_OK);
        PostQuitMessage(1);
    }
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hWnd = hwnd;
        RECT rc;
        GetClientRect(hwnd, &rc);

        int currentY = 0;
        int controlHeight = 28;
        int padding = 2;
        int buttonWidth = 60;
        int smallButtonWidth = 30;

        g_hBackBtn = CreateWindowW(L"BUTTON", L"<-", WS_CHILD | WS_VISIBLE | WS_DISABLED, padding, currentY, smallButtonWidth, controlHeight, hwnd, (HMENU)ID_BACK_BTN, nullptr, nullptr);
        g_hForwardBtn = CreateWindowW(L"BUTTON", L"->", WS_CHILD | WS_VISIBLE | WS_DISABLED, padding + smallButtonWidth + padding, currentY, smallButtonWidth, controlHeight, hwnd, (HMENU)ID_FORWARD_BTN, nullptr, nullptr);
        g_hUrlBar = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, padding * 3 + smallButtonWidth * 2, currentY, rc.right - (padding * 4 + smallButtonWidth * 2 + buttonWidth), controlHeight, hwnd, (HMENU)ID_URLBAR, nullptr, nullptr);
        g_hGoBtn = CreateWindowW(L"BUTTON", L"Git", WS_CHILD | WS_VISIBLE, rc.right - buttonWidth - padding, currentY, buttonWidth, controlHeight, hwnd, (HMENU)ID_GO_BTN, nullptr, nullptr);

        currentY += controlHeight + padding;

        int btnX = padding;
        g_hGoogleBtn = CreateWindowW(L"BUTTON", L"Google", WS_CHILD | WS_VISIBLE, btnX, currentY, buttonWidth, controlHeight, hwnd, (HMENU)ID_GOOGLE_BTN, nullptr, nullptr); btnX += buttonWidth + padding;
        g_hChatGPTBtn = CreateWindowW(L"BUTTON", L"ChatGPT", WS_CHILD | WS_VISIBLE, btnX, currentY, buttonWidth + 10, controlHeight, hwnd, (HMENU)ID_CHATGPT_BTN, nullptr, nullptr); btnX += buttonWidth + 10 + padding;
        g_hGeminiBtn = CreateWindowW(L"BUTTON", L"Gemini", WS_CHILD | WS_VISIBLE, btnX, currentY, buttonWidth, controlHeight, hwnd, (HMENU)ID_GEMINI_BTN, nullptr, nullptr); btnX += buttonWidth + padding;

        g_hDevBtn = CreateWindowW(L"BUTTON", L"Dev", WS_CHILD | WS_VISIBLE, btnX, currentY, smallButtonWidth + 10, controlHeight, hwnd, (HMENU)ID_DEV_BTN, nullptr, nullptr); btnX += smallButtonWidth + 10 + padding;
        g_hUABtn = CreateWindowW(L"BUTTON", L"UA", WS_CHILD | WS_VISIBLE, btnX, currentY, smallButtonWidth + 5, controlHeight, hwnd, (HMENU)ID_UA_BTN, nullptr, nullptr); btnX += smallButtonWidth + 5 + padding;
        g_hReaderBtn = CreateWindowW(L"BUTTON", L"Oku", WS_CHILD | WS_VISIBLE, btnX, currentY, smallButtonWidth + 10, controlHeight, hwnd, (HMENU)ID_READER_BTN, nullptr, nullptr); btnX += smallButtonWidth + 10 + padding;
        g_hJsToggleBtn = CreateWindowW(L"BUTTON", L"JS+", WS_CHILD | WS_VISIBLE, btnX, currentY, smallButtonWidth + 5, controlHeight, hwnd, (HMENU)ID_JS_TOGGLE_BTN, nullptr, nullptr); btnX += smallButtonWidth + 5 + padding;
        g_hClearCacheBtn = CreateWindowW(L"BUTTON", L"Önb.", WS_CHILD | WS_VISIBLE, btnX, currentY, smallButtonWidth + 10, controlHeight, hwnd, (HMENU)ID_CLEAR_CACHE_BTN, nullptr, nullptr); btnX += smallButtonWidth + 10 + padding;

        int remainingWidthForLogButtons = rc.right - btnX - padding - (buttonWidth + 20 + padding);
        int logButtonCount = 7;
        int logButtonWidth = (remainingWidthForLogButtons > 0 && logButtonCount > 0) ? (remainingWidthForLogButtons / logButtonCount) - padding : smallButtonWidth;
        if (logButtonWidth < smallButtonWidth) logButtonWidth = smallButtonWidth;

        g_hLogBtn = CreateWindowW(L"BUTTON", L"Log", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_LOG_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hJsLogBtn = CreateWindowW(L"BUTTON", L"JSLog", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_JS_LOG_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hHttpLogBtn = CreateWindowW(L"BUTTON", L"HTTPLog", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_HTTP_LOG_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hImgLogBtn = CreateWindowW(L"BUTTON", L"ImgLog", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_IMG_LOG_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hAltInfoBtn = CreateWindowW(L"BUTTON", L"AltInfo", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_ALT_INFO_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hFoundForBtn = CreateWindowW(L"BUTTON", L"Bul", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_FOUND_FOR_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hNetLogBtn = CreateWindowW(L"BUTTON", L"NetLog", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_NETLOG_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;

        g_hCaptureChk = CreateWindowW(L"BUTTON", L"Capture Log", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, btnX, currentY, buttonWidth + 20, controlHeight, hwnd, (HMENU)ID_CAPTURE_CHK, nullptr, nullptr);
        CheckDlgButton(hwnd, ID_CAPTURE_CHK, g_isCaptureActive ? BST_CHECKED : BST_UNCHECKED);

        g_hStatus = CreateWindowW(L"STATIC", L"Hazır", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
            0, rc.bottom - 20, rc.right, 20, hwnd, (HMENU)ID_STATUSBAR, nullptr, nullptr);

        // --- WHATSAPP BUTONLARI ---
        g_hWhatsAppBtn = CreateWindowW(L"BUTTON", L"W", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            rc.right - 106, rc.bottom - 55, 30, 30, hwnd, (HMENU)ID_WHATSAPP_BTN, nullptr, nullptr);
        g_hWhatsAppCaptureBtn = CreateWindowW(L"BUTTON", L"C", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            rc.right - 74, rc.bottom - 55, 30, 30, hwnd, (HMENU)ID_WHATSAPP_CAPTURE_BTN, nullptr, nullptr);
        g_hWhatsAppLogsBtn = CreateWindowW(L"BUTTON", L"L", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            rc.right - 42, rc.bottom - 55, 30, 30, hwnd, (HMENU)ID_WHATSAPP_LOGS_BTN, nullptr, nullptr);

        InitializeWebView();
        break;
    }
    case WM_SIZE: {
        if (g_controller) {
            RECT rc;
            GetClientRect(hwnd, &rc);

            int currentY = 0;
            int controlHeight = 28;
            int padding = 2;
            int buttonWidth = 60;
            int smallButtonWidth = 30;

            MoveWindow(g_hBackBtn, padding, currentY, smallButtonWidth, controlHeight, TRUE);
            MoveWindow(g_hForwardBtn, padding + smallButtonWidth + padding, currentY, smallButtonWidth, controlHeight, TRUE);
            MoveWindow(g_hUrlBar, padding * 3 + smallButtonWidth * 2, currentY, rc.right - (padding * 4 + smallButtonWidth * 2 + buttonWidth), controlHeight, TRUE);
            MoveWindow(g_hGoBtn, rc.right - buttonWidth - padding, currentY, buttonWidth, controlHeight, TRUE);

            currentY += controlHeight + padding;
            int btnX = padding;

            MoveWindow(g_hGoogleBtn, btnX, currentY, buttonWidth, controlHeight, TRUE); btnX += buttonWidth + padding;
            MoveWindow(g_hChatGPTBtn, btnX, currentY, buttonWidth + 10, controlHeight, TRUE); btnX += buttonWidth + 10 + padding;
            MoveWindow(g_hGeminiBtn, btnX, currentY, buttonWidth, controlHeight, TRUE); btnX += buttonWidth + padding;
            MoveWindow(g_hDevBtn, btnX, currentY, smallButtonWidth + 10, controlHeight, TRUE); btnX += smallButtonWidth + 10 + padding;
            MoveWindow(g_hUABtn, btnX, currentY, smallButtonWidth + 5, controlHeight, TRUE); btnX += smallButtonWidth + 5 + padding;
            MoveWindow(g_hReaderBtn, btnX, currentY, smallButtonWidth + 10, controlHeight, TRUE); btnX += smallButtonWidth + 10 + padding;
            MoveWindow(g_hJsToggleBtn, btnX, currentY, smallButtonWidth + 5, controlHeight, TRUE); btnX += smallButtonWidth + 5 + padding;
            MoveWindow(g_hClearCacheBtn, btnX, currentY, smallButtonWidth + 10, controlHeight, TRUE); btnX += smallButtonWidth + 10 + padding;

            int remainingWidthForLogButtons = rc.right - btnX - padding - (buttonWidth + 20 + padding);
            int logButtonCount = 7;
            int logButtonWidth = (remainingWidthForLogButtons > 0 && logButtonCount > 0) ? (remainingWidthForLogButtons / logButtonCount) - padding : smallButtonWidth;
            if (logButtonWidth < smallButtonWidth) logButtonWidth = smallButtonWidth;


            MoveWindow(g_hLogBtn, btnX, currentY, logButtonWidth, controlHeight, TRUE); btnX += logButtonWidth + padding;
            MoveWindow(g_hJsLogBtn, btnX, currentY, logButtonWidth, controlHeight, TRUE); btnX += logButtonWidth + padding;
            MoveWindow(g_hHttpLogBtn, btnX, currentY, logButtonWidth, controlHeight, TRUE); btnX += logButtonWidth + padding;
            MoveWindow(g_hImgLogBtn, btnX, currentY, logButtonWidth, controlHeight, TRUE); btnX += logButtonWidth + padding;
            MoveWindow(g_hAltInfoBtn, btnX, currentY, logButtonWidth, controlHeight, TRUE); btnX += logButtonWidth + padding;
            MoveWindow(g_hFoundForBtn, btnX, currentY, logButtonWidth, controlHeight, TRUE); btnX += logButtonWidth + padding;
            MoveWindow(g_hNetLogBtn, btnX, currentY, logButtonWidth, controlHeight, TRUE); btnX += logButtonWidth + padding;
            MoveWindow(g_hCaptureChk, btnX, currentY, buttonWidth + 20, controlHeight, TRUE);


            RECT wvBounds = { 0, currentY + controlHeight + padding, rc.right, rc.bottom - 20 };
            if (g_controller) g_controller->put_Bounds(wvBounds);
            MoveWindow(g_hStatus, 0, rc.bottom - 20, rc.right, 20, TRUE);

            // --- WHATSAPP BUTONLARININ KONUMUNU GÜNCELLE ---
            if (g_hWhatsAppBtn) MoveWindow(g_hWhatsAppBtn, rc.right - 106, rc.bottom - 55, 30, 30, TRUE);
            if (g_hWhatsAppCaptureBtn) MoveWindow(g_hWhatsAppCaptureBtn, rc.right - 74, rc.bottom - 55, 30, 30, TRUE);
            if (g_hWhatsAppLogsBtn) MoveWindow(g_hWhatsAppLogsBtn, rc.right - 42, rc.bottom - 55, 30, 30, TRUE);
        }
        return 0;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {

        case ID_GO_BTN: {
            wchar_t url[2048];
            GetWindowTextW(g_hUrlBar, url, _countof(url));
            std::wstring urlStr = url;
            if (urlStr.find(L"://") == std::wstring::npos && urlStr.find(L".") != std::wstring::npos) {
                urlStr = L"http://" + urlStr;
            }
            else if (urlStr.find(L"://") == std::wstring::npos && !urlStr.empty()) {
                urlStr = L"https://www.google.com/search?q=" + urlStr;
            }

            if (g_webview) {
                Log(L"NavReq", urlStr.c_str());
                g_webview->Navigate(urlStr.c_str());
            }
            break;
        }
        case ID_BACK_BTN:
            if (g_webview) g_webview->GoBack();
            break;
        case ID_FORWARD_BTN:
            if (g_webview) g_webview->GoForward();
            break;
        case ID_DEV_BTN:
            if (g_webview) g_webview->OpenDevToolsWindow();
            Log(L"DevTools", L"Geliştirici Araçları açıldı.");
            break;
        case ID_GOOGLE_BTN: if (g_webview) g_webview->Navigate(L"https://www.google.com"); break;
        case ID_CHATGPT_BTN: if (g_webview) g_webview->Navigate(L"https://chat.openai.com"); break;
        case ID_GEMINI_BTN: if (g_webview) g_webview->Navigate(L"https://gemini.google.com"); break;
        case ID_UA_BTN: {
            if (g_settings) {
                wil::com_ptr<ICoreWebView2Settings2> settings2;
                if (SUCCEEDED(g_settings->QueryInterface(IID_PPV_ARGS(&settings2)))) {
                    g_currentUserAgentIndex = (g_currentUserAgentIndex + 1) % g_userAgents.size();
                    settings2->put_UserAgent(g_userAgents[g_currentUserAgentIndex].c_str());
                    SaveUserAgentPreference(g_currentUserAgentIndex);
                    Log(L"UserAgent", (L"Değiştirildi: " + (g_userAgents[g_currentUserAgentIndex].empty() ? L"Varsayılan (Sistem)" : g_userAgents[g_currentUserAgentIndex].substr(0, 30) + L"...")).c_str());
                    UpdateStatusBar();
                    if (g_webview) g_webview->Reload();
                }
            }
            break;
        }
        case ID_READER_BTN: {
            g_isReaderModeActive = !g_isReaderModeActive;
            ToggleReaderModeCSS(g_isReaderModeActive);
            if (g_isReaderModeActive) {
                if (g_settings) g_settings->put_IsScriptEnabled(FALSE);
                Log(L"OkumaModu", L"Aktif");
            }
            else {
                if (g_settings) g_settings->put_IsScriptEnabled(g_isJsGloballyActive);
                Log(L"OkumaModu", L"Pasif");
            }
            UpdateStatusBar();
            if (g_webview) g_webview->Reload();
            break;
        }
        case ID_JS_TOGGLE_BTN: {
            g_isJsGloballyActive = !g_isJsGloballyActive;
            if (g_settings) {
                if (!g_isReaderModeActive) {
                    g_settings->put_IsScriptEnabled(g_isJsGloballyActive);
                }
            }
            SetWindowTextW(g_hJsToggleBtn, g_isJsGloballyActive ? L"JS-" : L"JS+");
            Log(L"JSToggle", g_isJsGloballyActive ? L"JavaScript Etkin" : L"JavaScript Devre Dışı");
            UpdateStatusBar();
            if (g_webview && !g_isReaderModeActive) g_webview->Reload();
            break;
        }
        case ID_CLEAR_CACHE_BTN: {
            if (MessageBoxW(hwnd, L"Tarayıcı önbelleğini (WebView2 kullanıcı verileri) silmek istediğinizden emin misiniz?\nBu işlem mevcut oturumu sonlandıracak ve tarayıcıyı yeniden başlatacaktır.", L"Önbelleği Temizle", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                Log(L"Önbellek", L"Temizleme isteği.");
                if (g_controller) {
                    g_controller->Close();
                    g_controller = nullptr;
                    g_webview = nullptr;
                    g_webview_v2 = nullptr;
                    g_settings = nullptr;
                }
                std::wstring msgText = L"Önbellek temizleme işlemi için tarayıcının kapatılması gerekiyor. Lütfen uygulamayı kapatın ve kullanıcı veri klasörünü manuel olarak silin: \n" + g_userDataFolder + L"\nArdından uygulamayı tekrar açın.";
                MessageBoxW(hwnd, msgText.c_str(), L"Önbellek Temizleme", MB_OK | MB_ICONINFORMATION);
                PostQuitMessage(0);
            }
            break;
        }
        case ID_LOG_BTN:
            if (g_isCaptureActive) ShellExecute(NULL, L"open", g_logStatusFile.c_str(), NULL, NULL, SW_SHOWNORMAL);
            else MessageBox(hwnd, L"Logları görüntülemek için 'Capture Log' aktif olmalıdır.", L"Bilgi", MB_OK | MB_ICONINFORMATION);
            break;
        case ID_JS_LOG_BTN:
            if (g_isCaptureActive) ShellExecute(NULL, L"open", g_jsDetailsFile.c_str(), NULL, NULL, SW_SHOWNORMAL);
            else MessageBox(hwnd, L"JS Loglarını görüntülemek için 'Capture Log' aktif olmalıdır.", L"Bilgi", MB_OK | MB_ICONINFORMATION);
            break;
        case ID_HTTP_LOG_BTN:
            if (g_isCaptureActive) ShellExecute(NULL, L"open", g_httpDetailsFile.c_str(), NULL, NULL, SW_SHOWNORMAL);
            else MessageBox(hwnd, L"HTTP Loglarını görüntülemek için 'Capture Log' aktif olmalıdır.", L"Bilgi", MB_OK | MB_ICONINFORMATION);
            break;
        case ID_IMG_LOG_BTN:
            if (g_isCaptureActive) ShellExecute(NULL, L"open", g_imageLogFile.c_str(), NULL, NULL, SW_SHOWNORMAL);
            else MessageBox(hwnd, L"Resim Loglarını görüntülemek için 'Capture Log' aktif olmalıdır.", L"Bilgi", MB_OK | MB_ICONINFORMATION);
            break;
        case ID_ALT_INFO_BTN:
            if (g_isCaptureActive) ShellExecute(NULL, L"open", g_altInfoFile.c_str(), NULL, NULL, SW_SHOWNORMAL);
            else MessageBox(hwnd, L"Alt Bilgileri görüntülemek için 'Capture Log' aktif olmalıdır.", L"Bilgi", MB_OK | MB_ICONINFORMATION);
            break;
        case ID_FOUND_FOR_BTN:
            if (g_isCaptureActive) ShellExecute(NULL, L"open", g_foundForFile.c_str(), NULL, NULL, SW_SHOWNORMAL);
            else MessageBox(hwnd, L"Arama Sonuçlarını görüntülemek için 'Capture Log' aktif olmalıdır.", L"Bilgi", MB_OK | MB_ICONINFORMATION);
            break;
        case ID_NETLOG_BTN:
            if (g_isCaptureActive) ShellExecute(NULL, L"open", g_netLogFile.c_str(), NULL, NULL, SW_SHOWNORMAL);
            else MessageBox(hwnd, L"Ağ Loglarını görüntülemek için 'Capture Log' aktif olmalıdır.", L"Bilgi", MB_OK | MB_ICONINFORMATION);
            break;
        case ID_CAPTURE_CHK:
        {
            bool wasCaptureActive = g_isCaptureActive;
            g_isCaptureActive = (IsDlgButtonChecked(hwnd, ID_CAPTURE_CHK) == BST_CHECKED);

            if (g_isCaptureActive != wasCaptureActive) {
                bool originalCaptureState = g_isCaptureActive;
                g_isCaptureActive = true;
                Log(L"CaptureToggle", originalCaptureState ? L"Log yakalama AKTİF" : L"Log yakalama PASİF");
                g_isCaptureActive = originalCaptureState;
            }

            UpdateStatusBar();
            if (g_isCaptureActive && !wasCaptureActive) {
                WriteToSpecificLog(g_httpDetailsFile, L"--- YAKALAMA BAŞLADI ---", true, true);
                WriteToSpecificLog(g_jsDetailsFile, L"--- YAKALAMA BAŞLADI ---", true, true);
                WriteToSpecificLog(g_imageLogFile, L"--- YAKALAMA BAŞLADI ---", true, true);
                WriteToSpecificLog(g_netLogFile, L"--- YAKALAMA BAŞLADI ---", true, true);
                WriteToSpecificLog(g_altInfoFile, L"--- YAKALAMA BAŞLADI ---", true, true);
                WriteToSpecificLog(g_foundForFile, L"--- YAKALAMA BAŞLADI ---", true, true);
            }
            break;
        }

        // --- WHATSAPP BUTON KONTROLLERİ ---
        case ID_WHATSAPP_BTN: {
            LogWhatsappDetails(L"[C++] 'W' butonuna tıklandı.");
            if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_WHATSAPP_DIALOG), hwnd, WhatsAppDialogProc) == IDOK) {
                if (!g_targetWhatsappUser.empty()) {
                    LogWhatsappDetails(L"[C++] Kullanıcı adı alındı: " + g_targetWhatsappUser);
                    std::wofstream(g_whatsappLogFile, std::ios::out | std::ios::trunc).close();
                    std::wofstream(g_whatsappDetailsFile, std::ios::out | std::ios::trunc).close();

                    LogWhatsappStatus(L"Kayıt başladı. Hedef: " + g_targetWhatsappUser);
                    LogWhatsappDetails(L"[C++] WhatsApp Web'e yönlendiriliyor...");
                    g_webview->Navigate(L"https://web.whatsapp.com");
                }
                else {
                    LogWhatsappDetails(L"[C++] Kullanıcı adı girilmeden diyalog kapatıldı.");
                }
            }
            break;
        }
        case ID_WHATSAPP_CAPTURE_BTN: {
            LogWhatsappDetails(L"[C++] 'C' butonuna tıklandı. Durum takip ve bağlantı test script'i enjekte ediliyor.");
            if (g_targetWhatsappUser.empty()) {
                MessageBoxW(hwnd, L"Önce 'W' butonuna basarak bir hedef kullanıcı adı belirlemelisiniz.", L"Hata", MB_OK | MB_ICONERROR);
                LogWhatsappDetails(L"[C++] HATA: Hedef kullanıcı belirlenmeden 'C' butonuna basıldı.");
                break;
            }

            std::wstring capture_script = LoadScriptFromFile(g_whatsappJsFile);
            if (capture_script.empty()) {
                MessageBoxW(hwnd, (L"whatsapp_tracker.js dosyası bulunamadı veya boş.\nLütfen dosyanın " + g_appDir + L" konumunda olduğundan emin olun.").c_str(), L"Script Hatası", MB_OK | MB_ICONERROR);
                break;
            }

            g_webview->ExecuteScript(capture_script.c_str(), nullptr);
            break;
        }
        case ID_WHATSAPP_LOGS_BTN: {
            LogWhatsappDetails(L"[C++] 'L' butonuna tıklandı. Log dosyaları açılıyor.");
            ShellExecute(NULL, L"open", g_whatsappLogFile.c_str(), NULL, NULL, SW_SHOWNORMAL);
            ShellExecute(NULL, L"open", g_whatsappDetailsFile.c_str(), NULL, NULL, SW_SHOWNORMAL);
            break;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        break;
    }
    case WM_TIMER: {
        if (wParam == ID_TIMER_PAGE_TIME) {
            UpdateStatusBar();
        }
        break;
    }
    case WM_DESTROY:
        g_isCaptureActive = true;
        Log(L"Kapanış", L"Uygulama kapatılıyor.");
        if (g_logFileStream.is_open()) {
            g_logFileStream.close();
        }

        if (g_webview) {
            if (m_navigationStartingToken.value != 0) g_webview->remove_NavigationStarting(m_navigationStartingToken);
            if (m_navigationCompletedToken.value != 0) g_webview->remove_NavigationCompleted(m_navigationCompletedToken);
            if (m_webResourceRequestedToken.value != 0) g_webview->remove_WebResourceRequested(m_webResourceRequestedToken);
            if (m_webMessageReceivedToken.value != 0) g_webview->remove_WebMessageReceived(m_webMessageReceivedToken);
            if (m_newWindowRequestedToken.value != 0) g_webview->remove_NewWindowRequested(m_newWindowRequestedToken);
            if (m_permissionRequestedToken.value != 0) g_webview->remove_PermissionRequested(m_permissionRequestedToken);
            if (m_documentTitleChangedToken.value != 0) g_webview->remove_DocumentTitleChanged(m_documentTitleChangedToken);
            if (m_historyChangedToken.value != 0) g_webview->remove_HistoryChanged(m_historyChangedToken);
            if (m_scriptDialogOpeningToken.value != 0) g_webview->remove_ScriptDialogOpening(m_scriptDialogOpeningToken);
        }
        if (g_webview_v2) {
            if (m_webResourceResponseReceivedToken.value != 0) g_webview_v2->remove_WebResourceResponseReceived(m_webResourceResponseReceivedToken);
            if (m_DOMContentLoadedToken.value != 0) g_webview_v2->remove_DOMContentLoaded(m_DOMContentLoadedToken);
        }


        if (g_controller) {
            g_controller->Close();
        }
        KillTimer(hwnd, ID_TIMER_PAGE_TIME);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    wchar_t path_buf[MAX_PATH];
    GetModuleFileNameW(NULL, path_buf, MAX_PATH);
    *wcsrchr(path_buf, L'\\') = L'\0';
    g_appDir = path_buf;
    g_appDir += L"\\";

    // --- DOSYA YOLLARI TANIMLAMA ---
    g_logStatusFile = g_appDir + L"logstatus.txt";
    g_jsDetailsFile = g_appDir + L"javascript_details.txt";
    g_httpDetailsFile = g_appDir + L"http_details.txt";
    g_imageLogFile = g_appDir + L"image_log.txt";
    g_altInfoFile = g_appDir + L"alt_info.txt";
    g_foundForFile = g_appDir + L"foundfor.txt";
    g_searchForFile = g_appDir + L"searchfor.txt";
    g_netLogFile = g_appDir + L"network_logstatus.txt";
    g_configFile = g_appDir + L"browser_config.ini";
    g_bookmarksFile = g_appDir + L"browser_bookmarks.txt";
    g_blockListFile = g_appDir + L"browser_blocklist.txt";
    // --- WHATSAPP LOG DOSYALARI ---
    g_whatsappLogFile = g_appDir + L"whatsapp.txt";
    g_whatsappDetailsFile = g_appDir + L"whatsapp_details.txt";
    g_whatsappJsFile = g_appDir + L"whatsapp_tracker.js";

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX); // <<< HATA BURADAYDI, DÜZELTİLDİ
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"CGPTBrowserWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassExW(&wcex)) {
        MessageBox(NULL, L"Pencere sınıfı kaydı başarısız!", L"Hata", MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    g_hWnd = CreateWindowW(wcex.lpszClassName, L"CGPT Browser", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, nullptr, nullptr, hInstance, nullptr);

    if (!g_hWnd) {
        MessageBox(NULL, L"Pencere oluşturma başarısız!", L"Hata", MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    bool initialCaptureState = true;
    g_isCaptureActive = true;
    Log(L"Başlangıç", L"Pencere oluşturuldu ve WebView başlatılıyor.");
    g_isCaptureActive = initialCaptureState;

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_isCaptureActive = true;
    Log(L"Kapanış", L"Mesaj döngüsü bitti.");
    if (g_logFileStream.is_open()) {
        g_logFileStream.close();
    }

    CoUninitialize();
    return (int)msg.wParam;
}
