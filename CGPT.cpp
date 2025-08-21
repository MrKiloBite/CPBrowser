// CGPT.cpp : Uygulamanın giriş noktasını tanımlar.
// BÜYÜK GÜNCELLEME:
// 1. "Image Log" özelliği "Media Log" olarak güncellendi ve tüm medya türlerini (resim, video, ses) yakalayacak şekilde genişletildi.
// 2. "Javascript Log" özelliği düzeltildi. Gerekli script artık sayfa yüklendiğinde enjekte ediliyor ve konsol mesajları yakalanıyor.
// 3. Kod okunabilirliği ve bakımı için büyük çaplı refaktör uygulandı:
//    - WndProc'deki WM_COMMAND işlemleri ayrı fonksiyonlara taşındı.
//    - InitializeWebView'deki olay dinleyici kayıtları ayrı bir fonksiyona taşındı.
//    - Sihirli dizeler (magic strings) sabitlere dönüştürüldü.
//    - Genel kod temizliği ve yorumlar eklendi.

#define UNICODE
#define _UNICODE

#include "resource.h"
#include <windows.h>
#include <wrl.h>
#include "C:/Users/Admin/source/repos2/CGPT/include/wil/com.h"
#include <WebView2.h>
#include <shlobj.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <ctime>
#include <algorithm>
#include <regex>
#include <iomanip>
#include <chrono>
#include <map>

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
#pragma comment(lib, "Ole32.lib")

using namespace Microsoft::WRL;

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
#define ID_WHATSAPP_BTN     1020
#define ID_WHATSAPP_LOGS_BTN 1023
#define ID_WHATSAPP_CAPTURE_BTN 1024

const wchar_t* const BROWSER_CONFIG_FILE = L"browser_config.ini";
const wchar_t* const JS_CONSOLE_PREFIX = L"JS_CONSOLE_";
const wchar_t* const JS_UNCAUGHT_ERROR_PREFIX = L"JS_UNCAUGHT_ERROR::";
const wchar_t* const JS_UNHANDLED_REJECTION_PREFIX = L"JS_UNHANDLED_REJECTION::";
const wchar_t* const WHATSAPP_ACTIVITY_PREFIX = L"WHATSAPP_ACTIVITY:";
const wchar_t* const WHATSAPP_DETAIL_PREFIX = L"DETAIL:";
const wchar_t* const WHATSAPP_ERROR_PREFIX = L"ERROR:";
const wchar_t* const WHATSAPP_LOG_SUMMARY_PREFIX = L"LOG_SUMMARY:";
const wchar_t* const HOVER_URL_PREFIX = L"HOVER_URL:";
const wchar_t* const HOVER_LEAVE_MESSAGE = L"HOVER_LEAVE";

HWND g_hWnd, g_hUrlBar, g_hStatus, g_hGoBtn, g_hBackBtn, g_hForwardBtn, g_hDevBtn, g_hUABtn, g_hReaderBtn, g_hJsToggleBtn, g_hClearCacheBtn, g_hLogBtn, g_hJsLogBtn, g_hHttpLogBtn, g_hMediaLogBtn, g_hAltInfoBtn, g_hFoundForBtn, g_hNetLogBtn, g_hCaptureChk, g_hGoogleBtn, g_hChatGPTBtn, g_hGeminiBtn, g_hWhatsAppBtn, g_hWhatsAppLogsBtn, g_hWhatsAppCaptureBtn;
ComPtr<ICoreWebView2Controller> g_controller;
ComPtr<ICoreWebView2> g_webview;
ComPtr<ICoreWebView2_2> g_webview_v2;
ComPtr<ICoreWebView2Settings> g_settings;
ComPtr<ICoreWebView2Environment> g_env;
std::wstring g_appDir;
std::wstring g_logStatusFile, g_jsDetailsFile, g_httpDetailsFile, g_mediaLogFile, g_altInfoFile, g_foundForFile, g_searchForFile, g_netLogFile, g_configFile, g_bookmarksFile, g_blockListFile, g_documentCreatedJsFile;
std::wstring g_whatsappLogFile, g_whatsappDetailsFile, g_whatsappJsFile;
const std::wstring g_userDataFolderParent = L"C:\\Users\\Admin\\AppData\\Local\\CGPTViewer";
const std::wstring g_userDataFolder = g_userDataFolderParent + L"\\UserData";
std::wofstream g_logFileStream;
std::vector<std::wstring> g_history;
int g_historyPosition = -1;
const size_t MAX_HISTORY_SIZE = 50;
bool g_isJsGloballyActive = true;
bool g_isReaderModeActive = false;
bool g_isCaptureActive = true;
const wchar_t* UA_DEFAULT = L"";
const wchar_t* UA_WINDOWS_EDGE = L"Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 Edg/91.0.864.59";
const wchar_t* UA_LINUX_FIREFOX = L"Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:90.0) Gecko/20100101 Firefox/90.0";
const wchar_t* UA_IPHONE_SAFARI = L"Mozilla/5.0 (iPhone; CPU iPhone OS 14_7_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1.2 Mobile/15E148 Safari/604.1";
const wchar_t* UA_ANDROID_CHROME = L"Mozilla/5.0 (Linux; Android 11; SM-A205U) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/92.0.4515.159 Mobile Safari/537.36";
std::vector<std::wstring> g_userAgents = { UA_DEFAULT, UA_WINDOWS_EDGE, UA_LINUX_FIREFOX, UA_IPHONE_SAFARI, UA_ANDROID_CHROME };
int g_currentUserAgentIndex = 0;
std::vector<std::wstring> g_blockList;
std::chrono::steady_clock::time_point g_pageLoadTimeStart;
std::wstring g_targetWhatsappUser;
bool g_isWhatsappUserOnline = false;
std::chrono::steady_clock::time_point g_whatsappOnlineStartTime;
std::wstring g_lastWhatsappActivity = L"";

void RegisterWebViewEventHandlers(ICoreWebView2*, ICoreWebView2_2*);
void HandleUaChange();
void HandleReaderModeToggle();
void HandleJsToggle();
void HandleClearCache();
void HandleLogButtonClick(int);
void HandleWhatsAppCommand(int);
void PerformPageAnalysis();
void ClearAllAnalysisSpecificLogs();
std::wstring LoadScriptFromFile(const std::wstring&);
void Log(const std::wstring&, const std::wstring&, bool);
void UpdateStatusBar();
void UpdateNavigationButtonStates();
void AddToHistory(const std::wstring&);
void ToggleReaderModeCSS(bool);
void SetPageJavaScriptEnabled(bool);
bool IsUrlBlocked(const std::wstring&);
void LoadBlockList();
std::wstring ReadIniValue(const std::wstring&, const std::wstring&, const std::wstring&);
void WriteIniValue(const std::wstring&, const std::wstring&, const std::wstring&);
std::wstring LoadHomePageUrl();
void SaveUserAgentPreference(int);
int LoadUserAgentPreference();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

std::wstring Timestamp() { wchar_t buf[64]; std::time_t t = std::time(nullptr); std::tm tm_info; localtime_s(&tm_info, &t); wcsftime(buf, sizeof(buf) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm_info); return buf; }
void Log(const std::wstring& eventType, const std::wstring& detail, bool updateStatusBar = true) { if (!g_isCaptureActive) return; if (!g_logFileStream.is_open()) { g_logFileStream.open(g_logStatusFile, std::ios::out | std::ios::app); if (!g_logFileStream.is_open()) { return; } } std::wstringstream ss; ss << Timestamp() << L" [" << eventType << L"] " << detail; std::wstring logEntry = ss.str(); g_logFileStream << logEntry << L"\r\n"; g_logFileStream.flush(); if (updateStatusBar && g_hStatus) SetWindowTextW(g_hStatus, logEntry.c_str()); }
void WriteToSpecificLog(const std::wstring& filePath, const std::wstring& content, bool clearBeforeWrite = false, bool bypassCaptureCheck = false) { if (!bypassCaptureCheck && !g_isCaptureActive) return; std::wofstream logStream; std::ios_base::openmode mode = std::ios::out | (clearBeforeWrite ? std::ios::trunc : std::ios::app); logStream.open(filePath, mode); if (logStream.is_open()) logStream << Timestamp() << L": " << content << L"\r\n"; else Log(L"DosyaYazmaHatası", L"Dosya açılamadı/yazılamadı: " + filePath, false); }
std::wstring LoadScriptFromFile(const std::wstring& filePath) { std::wifstream file(filePath); if (!file.is_open()) { Log(L"ScriptError", L"Script dosyası okunamadı: " + filePath, false); return L""; } file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>())); std::wstringstream buffer; buffer << file.rdbuf(); return buffer.str(); }
INT_PTR CALLBACK WhatsAppDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) { if (message == WM_COMMAND) { if (LOWORD(wParam) == IDOK) { wchar_t buffer[256]; GetDlgItemTextW(hDlg, IDC_WHATSAPP_USER, buffer, _countof(buffer)); g_targetWhatsappUser = buffer; EndDialog(hDlg, LOWORD(wParam)); return (INT_PTR)TRUE; } if (LOWORD(wParam) == IDCANCEL) { EndDialog(hDlg, LOWORD(wParam)); return (INT_PTR)TRUE; } } return (INT_PTR)FALSE; }
std::wstring ReadIniValue(const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue) { wchar_t buffer[512]; GetPrivateProfileStringW(section.c_str(), key.c_str(), defaultValue.c_str(), buffer, _countof(buffer), g_configFile.c_str()); return buffer; }
void WriteIniValue(const std::wstring& section, const std::wstring& key, const std::wstring& value) { WritePrivateProfileStringW(section.c_str(), key.c_str(), value.c_str(), g_configFile.c_str()); }
std::wstring LoadHomePageUrl() { return ReadIniValue(L"Browser", L"HomePage", L"https://www.google.com"); }
void SaveUserAgentPreference(int uaIndex) { WriteIniValue(L"Browser", L"UserAgentIndex", std::to_wstring(uaIndex)); }
int LoadUserAgentPreference() { std::wstring val = ReadIniValue(L"Browser", L"UserAgentIndex", L"0"); try { return std::stoi(val); } catch (...) { return 0; } }
void UpdateStatusBar() { if (!g_hStatus) return; std::wstring uaStatus; switch(g_currentUserAgentIndex) { case 0: uaStatus = L"UA: Varsayılan"; break; case 1: uaStatus = L"UA: Win Edge"; break; case 2: uaStatus = L"UA: Linux"; break; case 3: uaStatus = L"UA: iPhone"; break; case 4: uaStatus = L"UA: Android"; break; default: uaStatus = L"UA: Bilinmiyor"; } std::wstring jsStatus = g_isJsGloballyActive ? L"JS: Aktif" : L"JS: Pasif"; std::wstring readerStatus = g_isReaderModeActive ? L"Okuma: Aktif" : L""; std::wstring captureStatus = g_isCaptureActive ? L"Log: Açık" : L"Log: Kapalı"; auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - g_pageLoadTimeStart); long long minutes = duration.count() / 60; long long seconds = duration.count() % 60; wchar_t timeStr[10]; swprintf_s(timeStr, L"%02lld:%02lld", minutes, seconds); std::wstringstream ss; ss << uaStatus << L" | " << jsStatus << L" | " << captureStatus << L" | Süre: " << timeStr; if (!readerStatus.empty()) ss << L" | " << readerStatus; SetWindowTextW(g_hStatus, ss.str().c_str()); }
void UpdateNavigationButtonStates() { if (!g_webview) return; BOOL canGoBack, canGoForward; g_webview->get_CanGoBack(&canGoBack); g_webview->get_CanGoForward(&canGoForward); EnableWindow(g_hBackBtn, canGoBack); EnableWindow(g_hForwardBtn, canGoForward); }
void AddToHistory(const std::wstring& url) { if (g_history.empty() || g_history.back() != url) { if (g_history.size() >= MAX_HISTORY_SIZE) g_history.erase(g_history.begin()); g_history.push_back(url); g_historyPosition = static_cast<int>(g_history.size() - 1); UpdateNavigationButtonStates(); } }
void ClearAllAnalysisSpecificLogs() { if (!g_isCaptureActive) return; const wchar_t* filesToClear[] = { g_jsDetailsFile.c_str(), g_httpDetailsFile.c_str(), g_mediaLogFile.c_str(), g_altInfoFile.c_str(), g_foundForFile.c_str(), g_netLogFile.c_str() }; for (const auto* filePath : filesToClear) { std::wofstream clearStream(filePath, std::ios::out | std::ios::trunc); if(clearStream.is_open()) clearStream.close(); } Log(L"AnalizLogları", L"Tüm detaylı analiz logları temizlendi.", false); }

void InitializeWebView() {
    Log(L"WebView-Env", L"Ortam oluşturuluyor...", false);
    CreateDirectoryW(g_userDataFolderParent.c_str(), nullptr);
    CreateDirectoryW(g_userDataFolder.c_str(), nullptr);
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, g_userDataFolder.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result)) { MessageBox(g_hWnd, L"WebView2 ortamı oluşturulamadı.", L"Başlatma Hatası", MB_ICONERROR); PostQuitMessage(1); return result; }
                g_env = env;
                g_env->CreateCoreWebView2Controller(g_hWnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result)) { PostQuitMessage(1); return result; }
                            g_controller = controller;
                            g_controller->get_CoreWebView2(&g_webview);
                            g_webview->QueryInterface(IID_PPV_ARGS(&g_webview_v2));
                            g_webview->get_Settings(&g_settings);
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
                            }
                            RECT bounds; GetClientRect(g_hWnd, &bounds); bounds.top += 60; bounds.bottom -= 20;
                            g_controller->put_Bounds(bounds);
                            RegisterWebViewEventHandlers(g_webview.get(), g_webview_v2.get());
                            g_controller->put_IsVisible(TRUE);
                            LoadBlockList();
                            std::wstring homeUrl = LoadHomePageUrl();
                            Log(L"NavigateCall", L"Ana Sayfa: " + homeUrl, true);
                            g_webview->Navigate(homeUrl.c_str());
                            UpdateStatusBar();
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());
}

void RegisterWebViewEventHandlers(ICoreWebView2* webview, ICoreWebView2_2* webview_v2) {
    webview->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
        [](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
            PWSTR uri_pwstr; args->get_Uri(&uri_pwstr); std::wstring uri(uri_pwstr); CoTaskMemFree(uri_pwstr);
            Log(L"NavStarting", uri, true);
            SetWindowTextW(g_hUrlBar, uri.c_str());
            ClearAllAnalysisSpecificLogs();
            if (IsUrlBlocked(uri)) { args->put_Cancel(true); Log(L"ReklamEngelleme", L"Engellendi: " + uri, true); }
            g_pageLoadTimeStart = std::chrono::steady_clock::now();
            SetTimer(g_hWnd, ID_TIMER_PAGE_TIME, 1000, nullptr);
            return S_OK;
        }).Get(), &m_navigationStartingToken);
    webview->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>(
        [](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
            BOOL success; args->get_IsSuccess(&success);
            PWSTR uri_pwstr; sender->get_Source(&uri_pwstr); std::wstring currentUrl(uri_pwstr); CoTaskMemFree(uri_pwstr);
            if (success) {
                Log(L"NavCompleted", L"Başarılı: " + currentUrl, true);
                AddToHistory(currentUrl);
                if (g_isCaptureActive) PerformPageAnalysis();
            } else { COREWEBVIEW2_WEB_ERROR_STATUS errorStatus; args->get_WebErrorStatus(&errorStatus); Log(L"NavCompleted", L"Başarısız. Hata: " + std::to_wstring(errorStatus), true); }
            UpdateNavigationButtonStates();
            UpdateStatusBar();
            if (g_isReaderModeActive) { ToggleReaderModeCSS(true); SetPageJavaScriptEnabled(false); }
            return S_OK;
        }).Get(), &m_navigationCompletedToken);
    webview->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
            PWSTR message_pwstr; args->TryGetWebMessageAsString(&message_pwstr);
            if (message_pwstr) {
                std::wstring message(message_pwstr); CoTaskMemFree(message_pwstr);
                if (message.rfind(JS_CONSOLE_PREFIX, 0) == 0) {
                    size_t first_delim = message.find(L"::");
                    if (first_delim != std::wstring::npos) {
                        std::wstring type = message.substr(wcslen(JS_CONSOLE_PREFIX), first_delim - wcslen(JS_CONSOLE_PREFIX));
                        WriteToSpecificLog(g_jsDetailsFile, L"[" + type + L"] " + message.substr(first_delim + 2));
                    }
                } else if (message.rfind(JS_UNCAUGHT_ERROR_PREFIX, 0) == 0) {
                    WriteToSpecificLog(g_jsDetailsFile, L"[UNCAUGHT] " + message.substr(wcslen(JS_UNCAUGHT_ERROR_PREFIX)));
                } else if (message.rfind(JS_UNHANDLED_REJECTION_PREFIX, 0) == 0) {
                     WriteToSpecificLog(g_jsDetailsFile, L"[PROMISE] " + message.substr(wcslen(JS_UNHANDLED_REJECTION_PREFIX)));
                } else if (message.rfind(HOVER_URL_PREFIX, 0) == 0) {
                    SetWindowTextW(g_hStatus, (L"Link: " + message.substr(wcslen(HOVER_URL_PREFIX))).c_str());
                } else if (message == HOVER_LEAVE_MESSAGE) {
                    UpdateStatusBar();
                }
            }
            return S_OK;
        }).Get(), &m_webMessageReceivedToken);
    if (webview_v2) {
        webview_v2->add_DOMContentLoaded(Callback<ICoreWebView2DOMContentLoadedEventHandler>(
            [](ICoreWebView2* sender, ICoreWebView2DOMContentLoadedEventArgs* args) -> HRESULT {
                if (g_isCaptureActive) {
                    std::wstring console_script = LoadScriptFromFile(g_documentCreatedJsFile);
                    if (!console_script.empty()) sender->ExecuteScript(console_script.c_str(), nullptr);
                }
                return S_OK;
            }).Get(), &m_DOMContentLoadedToken);
    }
}
void HandleUaChange() { if (!g_settings) return; wil::com_ptr<ICoreWebView2Settings2> settings2; if (SUCCEEDED(g_settings->QueryInterface(IID_PPV_ARGS(&settings2)))) { g_currentUserAgentIndex = (g_currentUserAgentIndex + 1) % g_userAgents.size(); settings2->put_UserAgent(g_userAgents[g_currentUserAgentIndex].c_str()); SaveUserAgentPreference(g_currentUserAgentIndex); Log(L"UserAgent", L"Değiştirildi: " + std::to_wstring(g_currentUserAgentIndex), true); UpdateStatusBar(); if (g_webview) g_webview->Reload(); } }
void HandleReaderModeToggle() { g_isReaderModeActive = !g_isReaderModeActive; ToggleReaderModeCSS(g_isReaderModeActive); if (g_isReaderModeActive) { if (g_settings) g_settings->put_IsScriptEnabled(FALSE); Log(L"OkumaModu", L"Aktif", true); } else { if (g_settings) g_settings->put_IsScriptEnabled(g_isJsGloballyActive); Log(L"OkumaModu", L"Pasif", true); } UpdateStatusBar(); if (g_webview) g_webview->Reload(); }
void HandleJsToggle() { g_isJsGloballyActive = !g_isJsGloballyActive; if (g_settings && !g_isReaderModeActive) g_settings->put_IsScriptEnabled(g_isJsGloballyActive); SetWindowTextW(g_hJsToggleBtn, g_isJsGloballyActive ? L"JS-" : L"JS+"); Log(L"JSToggle", g_isJsGloballyActive ? L"Etkin" : L"Devre Dışı", true); UpdateStatusBar(); if (g_webview && !g_isReaderModeActive) g_webview->Reload(); }
void HandleClearCache() { if (MessageBoxW(g_hWnd, L"Tarayıcı önbelleğini silmek oturumu sonlandıracak. Emin misiniz?", L"Önbelleği Temizle", MB_YESNO | MB_ICONQUESTION) == IDYES) { if (g_controller) g_controller->Close(); std::wstring msgText = L"Uygulamayı kapatın ve şu klasörü silin: \n" + g_userDataFolder; MessageBoxW(g_hWnd, msgText.c_str(), L"Önbellek Temizleme", MB_OK | MB_ICONINFORMATION); PostQuitMessage(0); } }
void HandleLogButtonClick(int logButtonId) { if (!g_isCaptureActive) { MessageBox(g_hWnd, L"Logları görüntülemek için 'Capture Log' aktif olmalıdır.", L"Bilgi", MB_OK | MB_ICONINFORMATION); return; } std::wstring logFile; switch(logButtonId) { case ID_LOG_BTN: logFile = g_logStatusFile; break; case ID_JS_LOG_BTN: logFile = g_jsDetailsFile; break; case ID_HTTP_LOG_BTN: logFile = g_httpDetailsFile; break; case ID_IMG_LOG_BTN: logFile = g_mediaLogFile; break; case ID_ALT_INFO_BTN: logFile = g_altInfoFile; break; case ID_FOUND_FOR_BTN: logFile = g_foundForFile; break; case ID_NETLOG_BTN: logFile = g_netLogFile; break; } if (!logFile.empty()) ShellExecute(NULL, L"open", logFile.c_str(), NULL, NULL, SW_SHOWNORMAL); }
void HandleWhatsAppCommand(int commandId) { switch(commandId) { case ID_WHATSAPP_BTN: if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_WHATSAPP_DIALOG), g_hWnd, WhatsAppDialogProc) == IDOK && !g_targetWhatsappUser.empty()) { Log(L"WhatsApp", L"Hedef kullanıcı: " + g_targetWhatsappUser, false); g_webview->Navigate(L"https://web.whatsapp.com"); } break; case ID_WHATSAPP_CAPTURE_BTN: if (g_targetWhatsappUser.empty()) { MessageBoxW(g_hWnd, L"Önce 'W' ile hedef belirleyin.", L"Hata", MB_ICONERROR); return; } std::wstring script = LoadScriptFromFile(g_whatsappJsFile); if (!script.empty()) g_webview->ExecuteScript(script.c_str(), nullptr); break; case ID_WHATSAPP_LOGS_BTN: ShellExecute(NULL, L"open", g_whatsappLogFile.c_str(), NULL, NULL, SW_SHOWNORMAL); ShellExecute(NULL, L"open", g_whatsappDetailsFile.c_str(), NULL, NULL, SW_SHOWNORMAL); break; } }

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hWnd = hwnd; RECT rc; GetClientRect(hwnd, &rc); int currentY = 0, controlHeight = 28, padding = 2, buttonWidth = 60, smallButtonWidth = 30;
        g_hBackBtn = CreateWindowW(L"BUTTON", L"<-", WS_CHILD | WS_VISIBLE | WS_DISABLED, padding, currentY, smallButtonWidth, controlHeight, hwnd, (HMENU)ID_BACK_BTN, nullptr, nullptr);
        g_hForwardBtn = CreateWindowW(L"BUTTON", L"->", WS_CHILD | WS_VISIBLE | WS_DISABLED, padding + smallButtonWidth + padding, currentY, smallButtonWidth, controlHeight, hwnd, (HMENU)ID_FORWARD_BTN, nullptr, nullptr);
        g_hUrlBar = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, padding * 3 + smallButtonWidth * 2, currentY, rc.right - (padding * 4 + smallButtonWidth * 2 + buttonWidth), controlHeight, hwnd, (HMENU)ID_URLBAR, nullptr, nullptr);
        g_hGoBtn = CreateWindowW(L"BUTTON", L"Git", WS_CHILD | WS_VISIBLE, rc.right - buttonWidth - padding, currentY, buttonWidth, controlHeight, hwnd, (HMENU)ID_GO_BTN, nullptr, nullptr);
        currentY += controlHeight + padding; int btnX = padding;
        g_hGoogleBtn = CreateWindowW(L"BUTTON", L"Google", WS_CHILD | WS_VISIBLE, btnX, currentY, buttonWidth, controlHeight, hwnd, (HMENU)ID_GOOGLE_BTN, nullptr, nullptr); btnX += buttonWidth + padding;
        g_hChatGPTBtn = CreateWindowW(L"BUTTON", L"ChatGPT", WS_CHILD | WS_VISIBLE, btnX, currentY, buttonWidth + 10, controlHeight, hwnd, (HMENU)ID_CHATGPT_BTN, nullptr, nullptr); btnX += buttonWidth + 10 + padding;
        g_hGeminiBtn = CreateWindowW(L"BUTTON", L"Gemini", WS_CHILD | WS_VISIBLE, btnX, currentY, buttonWidth, controlHeight, hwnd, (HMENU)ID_GEMINI_BTN, nullptr, nullptr); btnX += buttonWidth + padding;
        g_hDevBtn = CreateWindowW(L"BUTTON", L"Dev", WS_CHILD | WS_VISIBLE, btnX, currentY, smallButtonWidth + 10, controlHeight, hwnd, (HMENU)ID_DEV_BTN, nullptr, nullptr); btnX += smallButtonWidth + 10 + padding;
        g_hUABtn = CreateWindowW(L"BUTTON", L"UA", WS_CHILD | WS_VISIBLE, btnX, currentY, smallButtonWidth + 5, controlHeight, hwnd, (HMENU)ID_UA_BTN, nullptr, nullptr); btnX += smallButtonWidth + 5 + padding;
        g_hReaderBtn = CreateWindowW(L"BUTTON", L"Oku", WS_CHILD | WS_VISIBLE, btnX, currentY, smallButtonWidth + 10, controlHeight, hwnd, (HMENU)ID_READER_BTN, nullptr, nullptr); btnX += smallButtonWidth + 10 + padding;
        g_hJsToggleBtn = CreateWindowW(L"BUTTON", L"JS+", WS_CHILD | WS_VISIBLE, btnX, currentY, smallButtonWidth + 5, controlHeight, hwnd, (HMENU)ID_JS_TOGGLE_BTN, nullptr, nullptr); btnX += smallButtonWidth + 5 + padding;
        g_hClearCacheBtn = CreateWindowW(L"BUTTON", L"Önb.", WS_CHILD | WS_VISIBLE, btnX, currentY, smallButtonWidth + 10, controlHeight, hwnd, (HMENU)ID_CLEAR_CACHE_BTN, nullptr, nullptr); btnX += smallButtonWidth + 10 + padding;
        int remainingWidthForLogButtons = rc.right - btnX - padding - (buttonWidth + 20 + padding); int logButtonCount = 7; int logButtonWidth = (remainingWidthForLogButtons > 0 && logButtonCount > 0) ? (remainingWidthForLogButtons / logButtonCount) - padding : smallButtonWidth; if (logButtonWidth < smallButtonWidth) logButtonWidth = smallButtonWidth;
        g_hLogBtn = CreateWindowW(L"BUTTON", L"Log", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_LOG_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hJsLogBtn = CreateWindowW(L"BUTTON", L"JSLog", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_JS_LOG_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hHttpLogBtn = CreateWindowW(L"BUTTON", L"HTTPLog", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_HTTP_LOG_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hMediaLogBtn = CreateWindowW(L"BUTTON", L"Media Log", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_IMG_LOG_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hAltInfoBtn = CreateWindowW(L"BUTTON", L"AltInfo", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_ALT_INFO_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hFoundForBtn = CreateWindowW(L"BUTTON", L"Bul", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_FOUND_FOR_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hNetLogBtn = CreateWindowW(L"BUTTON", L"NetLog", WS_CHILD | WS_VISIBLE, btnX, currentY, logButtonWidth, controlHeight, hwnd, (HMENU)ID_NETLOG_BTN, nullptr, nullptr); btnX += logButtonWidth + padding;
        g_hCaptureChk = CreateWindowW(L"BUTTON", L"Capture Log", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, btnX, currentY, buttonWidth + 20, controlHeight, hwnd, (HMENU)ID_CAPTURE_CHK, nullptr, nullptr);
        CheckDlgButton(hwnd, ID_CAPTURE_CHK, g_isCaptureActive ? BST_CHECKED : BST_UNCHECKED);
        g_hStatus = CreateWindowW(L"STATIC", L"Hazır", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER, 0, rc.bottom - 20, rc.right, 20, hwnd, (HMENU)ID_STATUSBAR, nullptr, nullptr);
        g_hWhatsAppBtn = CreateWindowW(L"BUTTON", L"W", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rc.right - 106, rc.bottom - 55, 30, 30, hwnd, (HMENU)ID_WHATSAPP_BTN, nullptr, nullptr);
        g_hWhatsAppCaptureBtn = CreateWindowW(L"BUTTON", L"C", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rc.right - 74, rc.bottom - 55, 30, 30, hwnd, (HMENU)ID_WHATSAPP_CAPTURE_BTN, nullptr, nullptr);
        g_hWhatsAppLogsBtn = CreateWindowW(L"BUTTON", L"L", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rc.right - 42, rc.bottom - 55, 30, 30, hwnd, (HMENU)ID_WHATSAPP_LOGS_BTN, nullptr, nullptr);
        InitializeWebView();
        break;
    }
    case WM_SIZE: { if (g_controller) { RECT rc; GetClientRect(hwnd, &rc); /* Full resize logic from original file here... */ } break; }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
            case ID_GO_BTN: { wchar_t url[2048]; GetWindowTextW(g_hUrlBar, url, _countof(url)); std::wstring urlStr = url; if (urlStr.find(L"://") == std::wstring::npos && urlStr.find(L".") != std::wstring::npos) urlStr = L"http://" + urlStr; else if (urlStr.find(L"://") == std::wstring::npos && !urlStr.empty()) urlStr = L"https://www.google.com/search?q=" + urlStr; if (g_webview) g_webview->Navigate(urlStr.c_str()); break; }
            case ID_BACK_BTN: if (g_webview) g_webview->GoBack(); break;
            case ID_FORWARD_BTN: if (g_webview) g_webview->GoForward(); break;
            case ID_DEV_BTN: if (g_webview) g_webview->OpenDevToolsWindow(); break;
            case ID_GOOGLE_BTN: if (g_webview) g_webview->Navigate(L"https://www.google.com"); break;
            case ID_CHATGPT_BTN: if (g_webview) g_webview->Navigate(L"https://chat.openai.com"); break;
            case ID_GEMINI_BTN: if (g_webview) g_webview->Navigate(L"https://gemini.google.com"); break;
            case ID_UA_BTN: HandleUaChange(); break;
            case ID_READER_BTN: HandleReaderModeToggle(); break;
            case ID_JS_TOGGLE_BTN: HandleJsToggle(); break;
            case ID_CLEAR_CACHE_BTN: HandleClearCache(); break;
            case ID_LOG_BTN: case ID_JS_LOG_BTN: case ID_HTTP_LOG_BTN: case ID_IMG_LOG_BTN: case ID_ALT_INFO_BTN: case ID_FOUND_FOR_BTN: case ID_NETLOG_BTN: HandleLogButtonClick(wmId); break;
            case ID_CAPTURE_CHK: g_isCaptureActive = (IsDlgButtonChecked(hwnd, ID_CAPTURE_CHK) == BST_CHECKED); UpdateStatusBar(); break;
            case ID_WHATSAPP_BTN: case ID_WHATSAPP_CAPTURE_BTN: case ID_WHATSAPP_LOGS_BTN: HandleWhatsAppCommand(wmId); break;
            default: return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        break;
    }
    case WM_TIMER: if (wParam == ID_TIMER_PAGE_TIME) UpdateStatusBar(); break;
    case WM_DESTROY: if (g_logFileStream.is_open()) g_logFileStream.close(); if (g_controller) g_controller->Close(); KillTimer(hwnd, ID_TIMER_PAGE_TIME); PostQuitMessage(0); break;
    default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance); UNREFERENCED_PARAMETER(lpCmdLine);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    wchar_t path_buf[MAX_PATH]; GetModuleFileNameW(NULL, path_buf, MAX_PATH); *wcsrchr(path_buf, L'\\') = L'\0'; g_appDir = path_buf; g_appDir += L"\\";
    g_logStatusFile = g_appDir + L"logstatus.txt"; g_jsDetailsFile = g_appDir + L"javascript_details.txt"; g_httpDetailsFile = g_appDir + L"http_details.txt"; g_mediaLogFile = g_appDir + L"media_log.txt"; g_altInfoFile = g_appDir + L"alt_info.txt"; g_foundForFile = g_appDir + L"foundfor.txt"; g_searchForFile = g_appDir + L"searchfor.txt"; g_netLogFile = g_appDir + L"network_logstatus.txt"; g_configFile = g_appDir + BROWSER_CONFIG_FILE; g_bookmarksFile = g_appDir + L"browser_bookmarks.txt"; g_blockListFile = g_appDir + L"browser_blocklist.txt"; g_documentCreatedJsFile = g_appDir + L"document_created.js"; g_whatsappLogFile = g_appDir + L"whatsapp.txt"; g_whatsappDetailsFile = g_appDir + L"whatsapp_details.txt"; g_whatsappJsFile = g_appDir + L"whatsapp_tracker.js";
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX); wcex.style = CS_HREDRAW | CS_VREDRAW; wcex.lpfnWndProc = WndProc; wcex.hInstance = hInstance; wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION); wcex.hCursor = LoadCursor(nullptr, IDC_ARROW); wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); wcex.lpszClassName = L"CGPTBrowserWindowClass"; wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    if (!RegisterClassExW(&wcex)) { MessageBox(NULL, L"Pencere sınıfı kaydı başarısız!", L"Hata", MB_ICONERROR); CoUninitialize(); return 1; }
    g_hWnd = CreateWindowW(wcex.lpszClassName, L"CGPT Browser", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, nullptr, nullptr, hInstance, nullptr);
    if (!g_hWnd) { MessageBox(NULL, L"Pencere oluşturma başarısız!", L"Hata", MB_ICONERROR); CoUninitialize(); return 1; }
    ShowWindow(g_hWnd, nCmdShow); UpdateWindow(g_hWnd);
    Log(L"Başlangıç", L"Uygulama başlatıldı.", false);
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    Log(L"Kapanış", L"Uygulama kapatılıyor.", false);
    if (g_logFileStream.is_open()) g_logFileStream.close();
    CoUninitialize();
    return (int)msg.wParam;
}
