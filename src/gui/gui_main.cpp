#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Include WinSock2 Network Monitor Header first to prevent winsock.h collisions
#include "core/network_monitor.h"

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wincrypt.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <psapi.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <iomanip>

// ImGui Headers
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// SysCore Engine Headers
#include "core/core.h"
#include "core/module_manager.h"
#include "core/thread_manager.h"
#include "core/ipc_manager.h"
#include "core/token_manager.h"
#include "core/file_watcher.h"
#include "core/hardware_info.h"
#include "core/service_manager.h"
#include "core/benchmark.h"
#include "core/lua_engine.h"
#include "core/process_tree.h"
#include "memory_diagnostic.h"
#include "core/logger.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Global DirectX 11 State
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*      g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward Declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Language Enum
enum AppLanguage {
    APP_LANG_ENGLISH = 0,
    APP_LANG_RUSSIAN = 1,
    APP_LANG_CHINESE = 2
};

static AppLanguage g_CurrentLanguage = APP_LANG_ENGLISH;

// Localization Strings Dictionary
struct LocString {
    const char* en;
    const char* ru;
    const char* zh;
};

static const char* Tr(const LocString& s)
{
    switch (g_CurrentLanguage)
    {
    case APP_LANG_RUSSIAN: return s.ru;
    case APP_LANG_CHINESE: return s.zh;
    case APP_LANG_ENGLISH:
    default:               return s.en;
    }
}

// UI Dictionary (EN, RU, ZH)
static const LocString STR_TITLE          = { "SysCore v1.0.0", "SysCore v1.0.0", "SysCore v1.0.0" };
static const LocString STR_SUBTITLE       = { "| Diagnostics", "| Диагностика", "| 诊断" };

static const LocString STR_TAB_SYS        = { "System", "Система", "系统" };
static const LocString STR_TAB_MOD        = { "Modules", "Модули", "模块" };
static const LocString STR_TAB_PROC       = { "Processes", "Процессы", "进程" };
static const LocString STR_TAB_DRV        = { "Drivers", "Драйверы", "驱动程序" };
static const LocString STR_TAB_AUTO       = { "Autoruns", "Автозагрузка", "自启动" };
static const LocString STR_TAB_HASH       = { "Hashing", "Хеширование", "哈希" };
static const LocString STR_TAB_LUA        = { "Lua Engine", "Lua Скрипты", "Lua 脚本" };
static const LocString STR_TAB_NET        = { "Network", "Сеть", "网络" };
static const LocString STR_TAB_DIR        = { "Files & IPC", "Файлы и IPC", "文件与 IPC" };

static const LocString STR_RAM_LOAD       = { "RAM Load", "Загрузка ОЗУ", "内存利用率" };
static const LocString STR_RAM_PHYS       = { "Available RAM", "Доступно ОЗУ", "可用内存" };
static const LocString STR_CPU_TOP        = { "CPU Cores", "Ядра CPU", "CPU 核心" };
static const LocString STR_GPU_INFO       = { "GPU Adapter", "Видеокарта", "GPU 适配器" };
static const LocString STR_SERVICES       = { "Win32 Services", "Службы Win32", "Win32 服务" };
static const LocString STR_RAM_HISTORY    = { "RAM Utilization History", "История загрузки ОЗУ", "内存利用率历史记录" };
static const LocString STR_ENGINE_CTRL    = { "Controls", "Управление", "控制" };
static const LocString STR_RUN_BENCH      = { "Run Benchmark", "Запустить бенчмарк", "运行基准测试" };
static const LocString STR_DISPATCH_TASK  = { "Dispatch Task", "Отправить задачу", "分发任务" };
static const LocString STR_EXPORT_REPORT  = { "Export JSON Report", "Экспорт отчета JSON", "导出 JSON 报告" };

static const LocString STR_MOD_TITLE      = { "Loaded Modules", "Загруженные модули", "已加载模块" };
static const LocString STR_MOD_DESC       = { "Dynamic DLL modules loaded at runtime.", "Динамические модули DLL, загруженные в рантайме.", "运行时加载的动态 DLL 模块。" };
static const LocString STR_EXEC_MODS      = { "Execute All Modules", "Выполнить все модули", "执行所有模块" };
static const LocString STR_HOT_RELOAD     = { "Reload Module", "Перезагрузить модуль", "重载模块" };

static const LocString STR_PROC_TREE_TITLE= { "Process List", "Список процессов", "进程列表" };
static const LocString STR_SEARCH_PROC    = { "Filter", "Фильтр", "筛选" };
static const LocString STR_CLEAR_SEARCH   = { "Clear", "Очистить", "清除" };
static const LocString STR_REFRESH_PROC   = { "Refresh Snapshot", "Обновить снимок", "刷新快照" };

static const LocString STR_COL_PID        = { "PID", "PID", "PID" };
static const LocString STR_COL_PNAME      = { "Process Name", "Имя процесса", "进程名称" };
static const LocString STR_COL_PPID       = { "Parent PID", "Родительский PID", "父进程 PID" };
static const LocString STR_COL_THREADS    = { "Threads", "Потоки", "线程数" };
static const LocString STR_COL_RAM        = { "RAM (Working Set)", "Память (ОЗУ)", "工作集内存" };
static const LocString STR_COL_ACTION     = { "Actions", "Действия", "操作" };

static const LocString STR_TERMINATE_PROC = { "Kill", "Завершить", "结束" };
static const LocString STR_DUMP_PROC      = { "Dump .DMP", "Дамп .DMP", "转储 .DMP" };

static const LocString STR_AUDIT_WX_TITLE  = { "Memory Protection Audit", "Аудит защиты памяти", "内存保护审计" };
static const LocString STR_TARGET_PID     = { "Target PID", "PID процесса", "目标 PID" };
static const LocString STR_SCAN_VM        = { "Scan Memory", "Сканировать память", "扫描内存" };
static const LocString STR_AUDIT_PID_BTN  = { "Audit ", "Аудит ", "审计 " };
static const LocString STR_WX_PASS        = { "PASS (No W^X violation)", "ПРОЙДЕНО (Нет нарушений W^X)", "通过 (无 W^X 违规)" };
static const LocString STR_WX_FAIL        = { "FAIL (PAGE_EXECUTE_READWRITE)", "НАРУШЕНИЕ (PAGE_EXECUTE_READWRITE)", "违规 (PAGE_EXECUTE_READWRITE)" };

static const LocString STR_COL_BASE_ADDR  = { "Base Address", "Базовый адрес", "基地址" };
static const LocString STR_COL_REG_SIZE   = { "Size", "Размер", "大小" };
static const LocString STR_COL_TYPE       = { "Type", "Тип", "类型" };
static const LocString STR_COL_PROTECT    = { "Protection", "Защита", "保护" };

static const LocString STR_DRV_TITLE      = { "Loaded Kernel Drivers", "Загруженные драйверы ядра", "已加载的内核驱动程序" };
static const LocString STR_REFRESH_DRV    = { "Refresh", "Обновить", "刷新" };

static const LocString STR_AUTO_TITLE     = { "Startup Items (HKCU & HKLM)", "Элементы автозагрузки", "自启动项" };
static const LocString STR_REFRESH_AUTO   = { "Refresh", "Обновить", "刷新" };

static const LocString STR_COL_ITEM_NAME  = { "Name", "Имя", "名称" };
static const LocString STR_COL_ITEM_CMD   = { "Command / Path", "Команда / Путь", "命令 / 路径" };
static const LocString STR_COL_ITEM_LOC   = { "Location", "Расположение", "位置" };

static const LocString STR_HASH_TITLE     = { "File Integrity Hashing", "Хеширование файлов", "文件哈希" };
static const LocString STR_FILE_PATH_INPUT= { "File Path", "Путь к файлу", "文件路径" };
static const LocString STR_COMPUTE_HASHES = { "Compute Hashes", "Вычислить хеши", "计算哈希" };

static const LocString STR_LUA_TITLE      = { "Lua Script Engine", "Движок скриптов Lua", "Lua 脚本引擎" };
static const LocString STR_LUA_DESC       = { "Execute custom Lua code against SysCore APIs.", "Выполнение скриптов Lua для взаимодействия с API SysCore.", "针对 SysCore API 执行自定义 Lua 代码。" };
static const LocString STR_EXEC_LUA       = { "Run Script", "Запустить скрипт", "运行脚本" };
static const LocString STR_PRESET_MEM     = { "Preset 1", "Шаблон 1", "预设 1" };
static const LocString STR_PRESET_BENCH   = { "Preset 2", "Шаблон 2", "预设 2" };
static const LocString STR_LUA_STREAM     = { "Script Output:", "Вывод скрипта:", "脚本输出:" };

static const LocString STR_NET_TITLE      = { "Active Network Connections", "Активные сетевые соединения", "活动网络连接" };
static const LocString STR_REFRESH_NET    = { "Refresh", "Обновить", "刷新" };

static const LocString STR_COL_PROTO      = { "Protocol", "Протокол", "协议" };
static const LocString STR_COL_LADDR      = { "Local Address", "Локальный адрес", "本地地址" };
static const LocString STR_COL_RADDR      = { "Remote Address", "Удаленный адрес", "远程地址" };
static const LocString STR_COL_STATE      = { "State", "Состояние", "状态" };

static const LocString STR_DIR_TITLE      = { "Directory Watcher", "Инспектор папок", "目录监视" };
static const LocString STR_DIR_STATUS     = { "Watching 'syscore_watch_temp' | Status: ", "Отслеживается 'syscore_watch_temp' | Статус: ", "正在监视 'syscore_watch_temp' | 状态: " };
static const LocString STR_DIR_ACTIVE     = { "Active", "Активен", "活动" };
static const LocString STR_IPC_TITLE      = { "Shared Memory IPC", "Разделяемая память IPC", "共享内存 IPC" };
static const LocString STR_PUBLISH_IPC    = { "Send Test IPC Message", "Отправить тестовое IPC сообщение", "发送测试 IPC 消息" };
static const LocString STR_CONSOLE_LOG    = { "Log Stream", "Журнал событий", "日志流" };

// Console Log Event LocStrings
static const LocString LOG_INIT_1 = {
    "[INIT] Multi-language support enabled.",
    "[ИНИЦ] Многоязычная поддержка включена.",
    "[初始化] 多语言支持已启用。"
};
static const LocString LOG_INIT_2 = {
    "[INIT] DirectX 11 backend initialized.",
    "[ИНИЦ] Бэкенд DirectX 11 инициализирован.",
    "[初始化] DirectX 11 后端已初始化。"
};
static const LocString LOG_INIT_3 = {
    "[INIT] System diagnostic tools ready.",
    "[ИНИЦ] Системные инструменты готовы.",
    "[初始化] 系统诊断工具就绪。"
};
static const LocString LOG_THREADPOOL_QUEUED = {
    "[THREADPOOL] Task queued.",
    "[THREADPOOL] Задача отправлена.",
    "[THREADPOOL] 任务已排队。"
};
static const LocString LOG_MODS_EXECUTED = {
    "[MODULES] Executed dynamic modules.",
    "[МОДУЛИ] Выполнены динамические модули.",
    "[模块] 已执行动态模块。"
};
static const LocString LOG_IPC_PUBLISHED = {
    "[IPC] Message published.",
    "[IPC] Сообщение отправлено.",
    "[IPC] 消息已发布。"
};
static const LocString LOG_PROC_KILLED = {
    "[PROCESS] Terminated PID: ",
    "[ПРОЦЕСС] Завершен PID: ",
    "[进程] 已结束 PID: "
};
static const LocString LOG_DUMP_CREATED = {
    "[DUMP] Dump saved to: ",
    "[ДАМП] Дамп сохранен в: ",
    "[转储] 转储已保存至: "
};
static const LocString LOG_REPORT_EXPORTED = {
    "[REPORT] Report saved to: ",
    "[ОТЧЕТ] Отчет сохранен в: ",
    "[报告] 报告已保存至: "
};
static const LocString LOG_AUTORUNS_REFRESHED = {
    "[AUTORUNS] Refreshed items: ",
    "[АВТОЗАГРУЗКА] Обновлено элементов: ",
    "[自启动] 已刷新条目: "
};
static const LocString LOG_DRIVERS_REFRESHED = {
    "[DRIVERS] Refreshed drivers: ",
    "[ДРАЙВЕРЫ] Обновлено драйверов: ",
    "[驱动程序] 已刷新驱动程序: "
};

// DXGI GPU Detection Helper
struct GpuHardwareInfo {
    std::string adapterName;
    size_t dedicatedVramBytes;
};

static GpuHardwareInfo GetPrimaryGpuInfo()
{
    GpuHardwareInfo info{ "Generic Graphics Adapter", 0 };
    IDXGIFactory1* pFactory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory)))
    {
        IDXGIAdapter1* pAdapter = nullptr;
        if (SUCCEEDED(pFactory->EnumAdapters1(0, &pAdapter)))
        {
            DXGI_ADAPTER_DESC1 desc;
            if (SUCCEEDED(pAdapter->GetDesc1(&desc)))
            {
                char nameBuf[256] = { 0 };
                ::WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, nameBuf, sizeof(nameBuf), nullptr, nullptr);
                info.adapterName = nameBuf;
                info.dedicatedVramBytes = desc.DedicatedVideoMemory;
            }
            pAdapter->Release();
        }
        pFactory->Release();
    }
    return info;
}

// Loaded DLL Modules Helper
struct ProcessDllModule {
    std::string moduleName;
    std::string modulePath;
    uintptr_t baseAddress;
    size_t sizeBytes;
};

static std::vector<ProcessDllModule> GetProcessLoadedDlls(DWORD pid)
{
    std::vector<ProcessDllModule> modules;
    HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnap == INVALID_HANDLE_VALUE) return modules;

    MODULEENTRY32W me;
    me.dwSize = sizeof(me);
    if (::Module32FirstW(hSnap, &me))
    {
        do
        {
            char nameBuf[256] = { 0 };
            char pathBuf[512] = { 0 };
            ::WideCharToMultiByte(CP_UTF8, 0, me.szModule, -1, nameBuf, sizeof(nameBuf), nullptr, nullptr);
            ::WideCharToMultiByte(CP_UTF8, 0, me.szExePath, -1, pathBuf, sizeof(pathBuf), nullptr, nullptr);

            ProcessDllModule mod;
            mod.moduleName = nameBuf;
            mod.modulePath = pathBuf;
            mod.baseAddress = reinterpret_cast<uintptr_t>(me.modBaseAddr);
            mod.sizeBytes = me.modBaseSize;
            modules.push_back(mod);
        } while (::Module32NextW(hSnap, &me));
    }
    ::CloseHandle(hSnap);
    return modules;
}

// MiniDump Memory Dump Helper
static bool CreateProcessDump(DWORD pid, const std::string& dumpPath)
{
    HANDLE hProc = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProc) return false;

    HANDLE hFile = ::CreateFileA(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(hProc);
        return false;
    }

    BOOL res = ::MiniDumpWriteDump(hProc, pid, hFile, MiniDumpNormal, nullptr, nullptr, nullptr);
    ::CloseHandle(hFile);
    ::CloseHandle(hProc);
    return res != FALSE;
}

// Kernel Drivers Helper
struct KernelDriverInfo {
    std::string driverName;
    uintptr_t baseAddress;
};

static std::vector<KernelDriverInfo> GetLoadedKernelDrivers()
{
    std::vector<KernelDriverInfo> drivers;
    LPVOID driversBuf[1024];
    DWORD cbNeeded = 0;
    if (::EnumDeviceDrivers(driversBuf, sizeof(driversBuf), &cbNeeded) && cbNeeded > 0)
    {
        int count = cbNeeded / sizeof(LPVOID);
        for (int i = 0; i < count; ++i)
        {
            char nameBuf[256] = { 0 };
            if (::GetDeviceDriverBaseNameA(driversBuf[i], nameBuf, sizeof(nameBuf)))
            {
                KernelDriverInfo drv;
                drv.driverName = nameBuf;
                drv.baseAddress = reinterpret_cast<uintptr_t>(driversBuf[i]);
                drivers.push_back(drv);
            }
        }
    }
    return drivers;
}

// Windows Registry Autoruns Helper
struct AutorunsEntry {
    std::string name;
    std::string command;
    std::string location;
};

static std::vector<AutorunsEntry> GetWindowsAutoruns()
{
    std::vector<AutorunsEntry> entries;
    auto readRegKey = [&](HKEY hKeyRoot, const char* subKey, const char* locName) {
        HKEY hKey;
        if (RegOpenKeyExA(hKeyRoot, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            DWORD index = 0;
            char valName[256];
            BYTE valData[1024];
            DWORD valNameLen = sizeof(valName);
            DWORD valDataLen = sizeof(valData);
            DWORD type = 0;

            while (RegEnumValueA(hKey, index, valName, &valNameLen, nullptr, &type, valData, &valDataLen) == ERROR_SUCCESS)
            {
                if (type == REG_SZ || type == REG_EXPAND_SZ)
                {
                    AutorunsEntry item;
                    item.name = valName;
                    item.command = reinterpret_cast<char*>(valData);
                    item.location = locName;
                    entries.push_back(item);
                }
                index++;
                valNameLen = sizeof(valName);
                valDataLen = sizeof(valData);
            }
            RegCloseKey(hKey);
        }
    };

    readRegKey(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "HKCU\\Run");
    readRegKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "HKLM\\Run");

    return entries;
}

// Process Token Privileges Helper
struct TokenPrivilegeItem {
    std::string name;
    bool enabled;
};

static std::vector<TokenPrivilegeItem> GetProcessTokenPrivileges(DWORD pid)
{
    std::vector<TokenPrivilegeItem> privs;
    HANDLE hProc = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProc) return privs;

    HANDLE hToken = NULL;
    if (::OpenProcessToken(hProc, TOKEN_QUERY, &hToken))
    {
        DWORD len = 0;
        ::GetTokenInformation(hToken, TokenPrivileges, nullptr, 0, &len);
        if (len > 0)
        {
            std::vector<BYTE> buf(len);
            if (::GetTokenInformation(hToken, TokenPrivileges, buf.data(), len, &len))
            {
                PTOKEN_PRIVILEGES pPrivs = reinterpret_cast<PTOKEN_PRIVILEGES>(buf.data());
                for (DWORD i = 0; i < pPrivs->PrivilegeCount; ++i)
                {
                    char nameBuf[256] = { 0 };
                    DWORD nameLen = sizeof(nameBuf);
                    if (::LookupPrivilegeNameA(nullptr, &pPrivs->Privileges[i].Luid, nameBuf, &nameLen))
                    {
                        TokenPrivilegeItem item;
                        item.name = nameBuf;
                        item.enabled = (pPrivs->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) != 0;
                        privs.push_back(item);
                    }
                }
            }
        }
        ::CloseHandle(hToken);
    }
    ::CloseHandle(hProc);
    return privs;
}

// Win32 CryptoAPI File Hashing Helper (MD5 & SHA-256)
static bool CalculateFileHashes(const std::string& filePath, std::string& md5Out, std::string& sha256Out)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hMd5 = 0, hSha256 = 0;

    if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return false;

    CryptCreateHash(hProv, CALG_MD5, 0, 0, &hMd5);
    CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hSha256);

    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
    {
        DWORD bytesRead = static_cast<DWORD>(file.gcount());
        if (hMd5) CryptHashData(hMd5, (BYTE*)buffer, bytesRead, 0);
        if (hSha256) CryptHashData(hSha256, (BYTE*)buffer, bytesRead, 0);
    }

    auto getHashString = [](HCRYPTHASH hHash) -> std::string {
        if (!hHash) return "N/A";
        DWORD cbHash = 0;
        CryptGetHashParam(hHash, HP_HASHVAL, nullptr, &cbHash, 0);
        std::vector<BYTE> hashBuf(cbHash);
        CryptGetHashParam(hHash, HP_HASHVAL, hashBuf.data(), &cbHash, 0);
        std::ostringstream ss;
        for (BYTE b : hashBuf)
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        return ss.str();
    };

    md5Out = getHashString(hMd5);
    sha256Out = getHashString(hSha256);

    if (hMd5) CryptDestroyHash(hMd5);
    if (hSha256) CryptDestroyHash(hSha256);
    if (hProv) CryptReleaseContext(hProv, 0);

    return true;
}

// Process Control Helper (Terminate Process)
static bool TerminateProcessByPid(DWORD pid)
{
    HANDLE hProc = ::OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProc) return false;
    BOOL res = ::TerminateProcess(hProc, 1);
    ::CloseHandle(hProc);
    return res != FALSE;
}

// Clean Readability Segoe UI Theme
void ApplyCleanSegoeTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 5.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 6.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.WindowPadding     = ImVec2(14, 14);
    style.FramePadding      = ImVec2(10, 8);
    style.ItemSpacing       = ImVec2(12, 10);

    const ImVec4 brightWhite   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    const ImVec4 silverText    = ImVec4(0.85f, 0.90f, 0.98f, 1.00f);
    const ImVec4 neonCyan      = ImVec4(0.00f, 0.88f, 1.00f, 1.00f);
    const ImVec4 neonGreen     = ImVec4(0.00f, 0.95f, 0.55f, 1.00f);
    
    const ImVec4 darkBg        = ImVec4(0.08f, 0.10f, 0.14f, 1.00f);
    const ImVec4 childBg       = ImVec4(0.12f, 0.15f, 0.21f, 1.00f);
    const ImVec4 frameBg       = ImVec4(0.16f, 0.20f, 0.28f, 1.00f);

    colors[ImGuiCol_Text]                  = brightWhite;
    colors[ImGuiCol_TextDisabled]          = silverText;
    colors[ImGuiCol_WindowBg]              = darkBg;
    colors[ImGuiCol_ChildBg]               = childBg;
    colors[ImGuiCol_PopupBg]               = ImVec4(0.11f, 0.14f, 0.19f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.22f, 0.28f, 0.38f, 1.00f);
    colors[ImGuiCol_FrameBg]               = frameBg;
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.22f, 0.28f, 0.40f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.18f, 0.28f, 0.42f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.00f, 0.55f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.00f, 0.75f, 1.00f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.20f, 0.30f, 0.44f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.18f, 0.23f, 0.32f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.14f, 0.18f, 0.25f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.00f, 0.65f, 0.95f, 1.00f);
}

struct GuiMemoryRegionInfo {
    uintptr_t baseAddress;
    size_t sizeBytes;
    DWORD type;
    DWORD protection;
};

// Helper memory scan function for current or target process
static std::vector<GuiMemoryRegionInfo> ScanProcessMemoryRegions(DWORD pid)
{
    std::vector<GuiMemoryRegionInfo> regions;
    HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) return regions;

    unsigned char* addr = nullptr;
    MEMORY_BASIC_INFORMATION mbi;
    while (::VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi)) == sizeof(mbi))
    {
        if (mbi.State == MEM_COMMIT)
        {
            GuiMemoryRegionInfo regInfo{};
            regInfo.baseAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            regInfo.sizeBytes = mbi.RegionSize;
            regInfo.type = mbi.Type;
            regInfo.protection = mbi.Protect;
            regions.push_back(regInfo);
        }
        addr += mbi.RegionSize;
    }
    ::CloseHandle(hProcess);
    return regions;
}

// Win32 Main Entry Point
INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow)
{
    // Register Win32 Window Class
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"SysCoreGuiClass", nullptr };
    ::RegisterClassExW(&wc);

    // Create Application Window
    HWND hwnd = ::CreateWindowExW(
        0L, wc.lpszClassName,
        L"SysCore Dashboard v1.0.0 — Modern C++20 System Engineering Framework",
        WS_OVERLAPPEDWINDOW,
        60, 40, 1440, 920,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    // Initialize Direct3D 11
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);

    // Initialize ImGui Context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Load Base Vector Font with Full Latin & Cyrillic Ranges
    ImFontConfig baseConfig;
    baseConfig.OversampleH = 2;
    baseConfig.OversampleV = 2;
    static const ImWchar cyrillicRanges[] = { 
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0 
    };

    ImFont* mainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f, &baseConfig, cyrillicRanges);
    if (!mainFont)
    {
        mainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 18.0f, &baseConfig, cyrillicRanges);
    }

    // MERGE Chinese Font Glyphs into the Same FontAtlas Texture!
    ImFontConfig mergeConfig;
    mergeConfig.MergeMode = true;
    mergeConfig.OversampleH = 2;
    mergeConfig.OversampleV = 2;
    static const ImWchar chineseRanges[] = { 
        0x4E00, 0x9FA5, // CJK Unified Ideographs
        0x3000, 0x30FF, // CJK Symbols & Punctuation
        0xFF00, 0xFFEF, // Halfwidth & Fullwidth Forms
        0 
    };
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.0f, &mergeConfig, chineseRanges);

    ApplyCleanSegoeTheme();

    // Setup Platform/Renderer Backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Initialize SysCore Engine Core
    SysCore::Core::ApplicationCore core;
    core.Initialize();

    // Telemetry micro-history buffer for RAM
    std::vector<float> ramHistory(60, 0.0f);
    int ramHistoryOffset = 0;
    auto lastSampleTime = std::chrono::steady_clock::now();

    // Query GPU Hardware Telemetry
    auto gpuInfo = GetPrimaryGpuInfo();

    // Console Log Event Entries (Stores LocString for dynamic multi-language translation!)
    struct ConsoleLogEntry {
        LocString locStr;
        std::string extraData;
    };
    std::vector<ConsoleLogEntry> consoleEntries;
    consoleEntries.push_back({ LOG_INIT_1, "" });
    consoleEntries.push_back({ LOG_INIT_2, "" });
    consoleEntries.push_back({ LOG_INIT_3, "" });

    // State Variables for Tabs
    char pidInput[32] = "";
    snprintf(pidInput, sizeof(pidInput), "%lu", ::GetCurrentProcessId());

    char fileHashInput[260] = "build\\Release\\SysCoreGuiApp.exe";
    std::string md5Result = "", sha256Result = "";
    
    std::vector<GuiMemoryRegionInfo> scannedRegions;
    std::vector<ProcessDllModule> loadedDlls;
    bool hasScannedMemory = false;
    SysCore::Security::ProcessSecurityContext tokenInfo = SysCore::Security::TokenManager::GetCurrentProcessSecurityContext();
    auto cpuInfo = SysCore::Hardware::HardwareInfo::GetCpuTopology();
    auto osInfo = SysCore::Hardware::HardwareInfo::GetOsVersion();

    // Autoruns & Drivers Snapshots
    static auto autorunsList = GetWindowsAutoruns();
    static auto driversList = GetLoadedKernelDrivers();

    // Lua Editor State
    char luaEditorBuffer[2048] = 
        "-- SysCore Diagnostic Lua Script\n"
        "SysCore.Log(\"Initializing High-Speed System Audit...\")\n"
        "SysCore.GetMemoryLoad()\n"
        "SysCore.GetProcessCount()\n"
        "SysCore.RunBenchmark()\n"
        "SysCore.Log(\"Lua System Audit Finished Successfully.\")\n";
    std::string luaOutputLog = "";

    // Process Tree Search Filter State
    char procFilterBuffer[64] = "";

    // Main GUI Render Loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Periodic Telemetry Sampling (Every 1 second)
        auto now = std::chrono::steady_clock::now();
        MEMORYSTATUSEX memStatus{};
        memStatus.dwLength = sizeof(memStatus);
        ::GlobalMemoryStatusEx(&memStatus);

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSampleTime).count() >= 1000)
        {
            ramHistory[ramHistoryOffset] = static_cast<float>(memStatus.dwMemoryLoad);
            ramHistoryOffset = (ramHistoryOffset + 1) % ramHistory.size();
            lastSampleTime = now;
        }

        // Start ImGui Frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Create Full-Screen Dashboard Window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("SysCore Dashboard Main Window", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        // Header Bar with Title, Subtitle, and Language Selector
        ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_TITLE));
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "%s", Tr(STR_SUBTITLE));
        
        // Language Selector Controls (EN | RU | ZH)
        ImGui::SameLine(ImGui::GetWindowWidth() - 440);
        ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "🌐 Lang:");
        ImGui::SameLine();
        if (ImGui::RadioButton("English", g_CurrentLanguage == APP_LANG_ENGLISH)) { g_CurrentLanguage = APP_LANG_ENGLISH; }
        ImGui::SameLine();
        if (ImGui::RadioButton("Русский", g_CurrentLanguage == APP_LANG_RUSSIAN)) { g_CurrentLanguage = APP_LANG_RUSSIAN; }
        ImGui::SameLine();
        if (ImGui::RadioButton("中文", g_CurrentLanguage == APP_LANG_CHINESE)) { g_CurrentLanguage = APP_LANG_CHINESE; }

        ImGui::Separator();

        // Tab Bar (NOW WITH 9 ENTERPRISE TABS!)
        if (ImGui::BeginTabBar("SysCoreMainTabBar", ImGuiTabBarFlags_None))
        {
            // TAB 1: System Telemetry & Overview (RAM, CPU, GPU, Services)
            if (ImGui::BeginTabItem(Tr(STR_TAB_SYS)))
            {
                ImGui::Spacing();

                // 5 Metrics Cards Column Layout
                ImGui::Columns(5, "MetricsCols5", false);
                
                // Card 1: RAM Utilization
                ImGui::BeginChild("CardRAM", ImVec2(0, 115), true);
                ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "%s", Tr(STR_RAM_LOAD));
                ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%lu %%", memStatus.dwMemoryLoad);
                ImGui::ProgressBar((float)memStatus.dwMemoryLoad / 100.0f, ImVec2(-1, 20));
                ImGui::EndChild();
                ImGui::NextColumn();

                // Card 2: Physical Memory
                ImGui::BeginChild("CardMem", ImVec2(0, 115), true);
                ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "%s", Tr(STR_RAM_PHYS));
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%.1f GB / %.1f GB", 
                    (float)memStatus.ullAvailPhys / (1024.0f * 1024.0f * 1024.0f),
                    (float)memStatus.ullTotalPhys / (1024.0f * 1024.0f * 1024.0f));
                ImGui::Text("OS: %s", osInfo.osName.c_str());
                ImGui::EndChild();
                ImGui::NextColumn();

                // Card 3: CPU & Cores
                ImGui::BeginChild("CardProc", ImVec2(0, 115), true);
                ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "%s", Tr(STR_CPU_TOP));
                ImGui::TextColored(ImVec4(1.00f, 0.80f, 0.20f, 1.00f), "%u Cores (%u Logical)", cpuInfo.physicalCoreCount, cpuInfo.logicalCoreCount);
                ImGui::Text("Host PID: %lu", ::GetCurrentProcessId());
                ImGui::EndChild();
                ImGui::NextColumn();

                // Card 4: GPU Graphics Adapter Telemetry
                ImGui::BeginChild("CardGPU", ImVec2(0, 115), true);
                ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "%s", Tr(STR_GPU_INFO));
                ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%.2f GB VRAM", (float)gpuInfo.dedicatedVramBytes / (1024.0f * 1024.0f * 1024.0f));
                ImGui::TextUnformatted(gpuInfo.adapterName.c_str());
                ImGui::EndChild();
                ImGui::NextColumn();

                // Card 5: Win32 Services Status
                ImGui::BeginChild("CardSvc", ImVec2(0, 115), true);
                ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "%s", Tr(STR_SERVICES));
                auto svcs = SysCore::Services::ServiceManager::EnumActiveServices();
                size_t runningCount = 0;
                for (const auto& s : svcs) if (s.isRunning) runningCount++;
                ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%zu Running / %zu Active", runningCount, svcs.size());
                ImGui::Text("SCM Database: Active");
                ImGui::EndChild();
                ImGui::Columns(1);

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_RAM_HISTORY));
                ImGui::PlotLines("##RAMPlot", ramHistory.data(), (int)ramHistory.size(), ramHistoryOffset, 
                    "RAM Load (%)", 0.0f, 100.0f, ImVec2(-1, 160));

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_ENGINE_CTRL));
                if (ImGui::Button(Tr(STR_RUN_BENCH), ImVec2(320, 42)))
                {
                    auto res = SysCore::Profiling::BenchmarkEngine::RunBenchmark("GUI_Benchmark", 10000, []() {
                        volatile int sum = 0;
                        for (int i = 0; i < 10000; ++i) sum += i;
                    });
                    static const LocString benchLog = {
                        "[BENCHMARK] 10,000 Loop Iterations completed in ",
                        "[БЕНЧМАРК] 10 000 итераций цикла выполнено за ",
                        "[基准测试] 10,000 次循环迭代完成于 "
                    };
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(2) << res.nsPerOp << " ns/op (" << res.opsPerSecond << " ops/sec)";
                    consoleEntries.push_back({ benchLog, ss.str() });
                }
                ImGui::SameLine();
                if (ImGui::Button(Tr(STR_DISPATCH_TASK), ImVec2(280, 42)))
                {
                    core.GetThreadManager().GetThreadPool().EnqueueTask([]() {
                        // Worker task
                    });
                    consoleEntries.push_back({ LOG_THREADPOOL_QUEUED, "" });
                }
                ImGui::SameLine();
                if (ImGui::Button(Tr(STR_EXPORT_REPORT), ImVec2(280, 42)))
                {
                    std::string repPath = "dist\\SysCore_SystemReport.json";
                    std::ofstream f(repPath);
                    if (f.is_open())
                    {
                        f << "{\n";
                        f << "  \"framework\": \"SysCore v1.0.0\",\n";
                        f << "  \"ram_load_percent\": " << memStatus.dwMemoryLoad << ",\n";
                        f << "  \"cpu_physical_cores\": " << cpuInfo.physicalCoreCount << ",\n";
                        f << "  \"gpu_adapter\": \"" << gpuInfo.adapterName << "\",\n";
                        f << "  \"autoruns_count\": " << autorunsList.size() << ",\n";
                        f << "  \"kernel_drivers_count\": " << driversList.size() << ",\n";
                        f << "  \"status\": \"SUCCESS\"\n";
                        f << "}\n";
                        f.close();
                        consoleEntries.push_back({ LOG_REPORT_EXPORTED, repPath });
                    }
                }

                ImGui::EndTabItem();
            }

            // TAB 2: Dynamic Module Manager & Hot-Reload
            if (ImGui::BeginTabItem(Tr(STR_TAB_MOD)))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_MOD_TITLE));
                ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "%s", Tr(STR_MOD_DESC));

                static int selectedModuleIdx = 0;
                const auto& loadedModules = core.GetModuleManager().GetLoadedModules();

                ImGui::Columns(2, "ModuleCols", true);
                ImGui::SetColumnWidth(0, 440);

                // Left Panel: List of Modules
                ImGui::BeginChild("ModuleListChild", ImVec2(0, -1), true);
                for (int i = 0; i < (int)loadedModules.size(); ++i)
                {
                    const auto& entry = loadedModules[i];
                    std::string modName = entry.moduleName;
                    std::string ver = entry.moduleInstance ? entry.moduleInstance->GetVersion() : "1.0.0";
                    static const LocString strModItem = { "Module: ", "Модуль: ", "模块: " };
                    std::string label = std::string(Tr(strModItem)) + modName + " (v" + ver + ")";
                    if (ImGui::Selectable(label.c_str(), selectedModuleIdx == i))
                    {
                        selectedModuleIdx = i;
                    }
                }
                ImGui::EndChild();

                ImGui::NextColumn();

                // Right Panel: Selected Module Control
                if (!loadedModules.empty() && selectedModuleIdx >= 0 && selectedModuleIdx < (int)loadedModules.size())
                {
                    const auto& mod = loadedModules[selectedModuleIdx];
                    ImGui::BeginChild("ModuleDetailChild", ImVec2(0, -1), true);
                    static const LocString strModHeader = { "Selected Module: ", "Выбранный модуль: ", "已选择模块: " };
                    static const LocString strVersion   = { "Version: ", "Версия: ", "版本: " };
                    static const LocString strModName   = { "Module Name: ", "Имя модуля: ", "模块名称: " };
                    static const LocString strLibPath   = { "Library Path: ", "Путь к библиотеке: ", "库路径: " };
                    static const LocString strLibHandle = { "Library Handle: ", "Хэндл библиотеки: ", "库句柄: " };

                    ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%s%s", Tr(strModHeader), mod.moduleName.c_str());
                    ImGui::Separator();
                    if (mod.moduleInstance)
                    {
                        ImGui::Text("%s%s", Tr(strVersion), mod.moduleInstance->GetVersion());
                        ImGui::Text("%s%s", Tr(strModName), mod.moduleInstance->GetName());
                    }
                    ImGui::Text("%s%s", Tr(strLibPath), mod.libraryPath.string().c_str());
                    ImGui::Text("%s0x%p", Tr(strLibHandle), (void*)mod.libraryHandle.Get());
                    ImGui::Spacing();

                    if (ImGui::Button(Tr(STR_EXEC_MODS), ImVec2(280, 44)))
                    {
                        core.GetModuleManager().ExecuteAllModules();
                        consoleEntries.push_back({ LOG_MODS_EXECUTED, "" });
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(Tr(STR_HOT_RELOAD), ImVec2(220, 44)))
                    {
                        core.GetModuleManager().ReloadModule(mod.moduleName);
                        static const LocString reloadLog = {
                            "[MODULE MANAGER] Reloaded module binary: ",
                            "[МЕНЕДЖЕР МОДУЛЕЙ] Перезагружен бинарный модуль: ",
                            "[模块管理器] 已重载模块二进制: "
                        };
                        consoleEntries.push_back({ reloadLog, mod.moduleName });
                    }
                    ImGui::EndChild();
                }
                ImGui::Columns(1);

                ImGui::EndTabItem();
            }

            // TAB 3: Process Tree & Memory Audit (WITH PROCESS DUMP & LOADED DLLS AUDIT!)
            if (ImGui::BeginTabItem(Tr(STR_TAB_PROC)))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_PROC_TREE_TITLE));
                
                ImGui::SetNextItemWidth(360);
                ImGui::InputText(Tr(STR_SEARCH_PROC), procFilterBuffer, sizeof(procFilterBuffer));
                ImGui::SameLine();
                if (ImGui::Button(Tr(STR_CLEAR_SEARCH))) { procFilterBuffer[0] = '\0'; }

                static auto processList = SysCore::Diagnostics::ProcessTreeManager::GetFlatProcessList();
                if (ImGui::Button(Tr(STR_REFRESH_PROC), ImVec2(240, 32)))
                {
                    processList = SysCore::Diagnostics::ProcessTreeManager::GetFlatProcessList();
                    static const LocString procRefLog = {
                        "[PROCESS MONITOR] Refreshed process snapshot. Total processes: ",
                        "[МОНИТОР ПРОЦЕССОВ] Снимок процессов обновлен. Всего процессов: ",
                        "[进程监视器] 已刷新进程快照。进程总数: "
                    };
                    consoleEntries.push_back({ procRefLog, std::to_string(processList.size()) });
                }

                // Process Table with Interactive Sorting!
                if (ImGui::BeginTable("ProcTreeTable", 6, 
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable, 
                    ImVec2(0, 240)))
                {
                    ImGui::TableSetupColumn(Tr(STR_COL_PID), ImGuiTableColumnFlags_DefaultSort, 0.0f, 0);
                    ImGui::TableSetupColumn(Tr(STR_COL_PNAME), ImGuiTableColumnFlags_None, 0.0f, 1);
                    ImGui::TableSetupColumn(Tr(STR_COL_PPID), ImGuiTableColumnFlags_None, 0.0f, 2);
                    ImGui::TableSetupColumn(Tr(STR_COL_THREADS), ImGuiTableColumnFlags_None, 0.0f, 3);
                    ImGui::TableSetupColumn(Tr(STR_COL_RAM), ImGuiTableColumnFlags_None, 0.0f, 4);
                    ImGui::TableSetupColumn(Tr(STR_COL_ACTION), ImGuiTableColumnFlags_NoSort, 0.0f, 5);
                    ImGui::TableHeadersRow();

                    if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs())
                    {
                        if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0)
                        {
                            const auto& spec = sort_specs->Specs[0];
                            std::sort(processList.begin(), processList.end(), [&spec](const auto& a, const auto& b) {
                                bool ascending = (spec.SortDirection == ImGuiSortDirection_Ascending);
                                switch (spec.ColumnUserID)
                                {
                                case 0: return ascending ? (a.processId < b.processId) : (a.processId > b.processId);
                                case 1: return ascending ? (a.processName < b.processName) : (a.processName > b.processName);
                                case 2: return ascending ? (a.parentProcessId < b.parentProcessId) : (a.parentProcessId > b.parentProcessId);
                                case 3: return ascending ? (a.threadCount < b.threadCount) : (a.threadCount > b.threadCount);
                                case 4: return ascending ? (a.workingSetSizeBytes < b.workingSetSizeBytes) : (a.workingSetSizeBytes > b.workingSetSizeBytes);
                                default: return false;
                                }
                            });
                            sort_specs->SpecsDirty = false;
                        }
                    }

                    std::string filterStr(procFilterBuffer);
                    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

                    for (const auto& proc : processList)
                    {
                        std::string pName = proc.processName;
                        std::string pNameLower = pName;
                        std::transform(pNameLower.begin(), pNameLower.end(), pNameLower.begin(), ::tolower);

                        if (!filterStr.empty() && pNameLower.find(filterStr) == std::string::npos)
                            continue;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%lu", proc.processId);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", proc.processName.c_str());

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%lu", proc.parentProcessId);

                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%lu", proc.threadCount);

                        ImGui::TableSetColumnIndex(4);
                        float ramMb = proc.workingSetSizeBytes / (1024.0f * 1024.0f);
                        if (ramMb > 200.0f)
                            ImGui::TextColored(ImVec4(1.00f, 0.30f, 0.30f, 1.00f), "%.2f MB", ramMb);
                        else if (ramMb > 50.0f)
                            ImGui::TextColored(ImVec4(1.00f, 0.80f, 0.20f, 1.00f), "%.2f MB", ramMb);
                        else
                            ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%.2f MB", ramMb);

                        ImGui::TableSetColumnIndex(5);
                        std::string auditBtnLabel = std::string(Tr(STR_AUDIT_PID_BTN)) + std::to_string(proc.processId);
                        if (ImGui::Button(auditBtnLabel.c_str()))
                        {
                            snprintf(pidInput, sizeof(pidInput), "%lu", proc.processId);
                            scannedRegions = ScanProcessMemoryRegions(proc.processId);
                            loadedDlls = GetProcessLoadedDlls(proc.processId);
                            hasScannedMemory = true;
                            static const LocString memScanLog = {
                                "[MEMORY AUDIT] Scanned regions for PID ",
                                "[АУДИТ ПАМЯТИ] Просканированы регионы для PID ",
                                "[内存审计] 已扫描进程区域，PID: "
                            };
                            consoleEntries.push_back({ memScanLog, std::to_string(proc.processId) });
                        }
                        ImGui::SameLine();
                        std::string dumpBtnLabel = std::string(Tr(STR_DUMP_PROC)) + " ##" + std::to_string(proc.processId);
                        if (ImGui::Button(dumpBtnLabel.c_str()))
                        {
                            std::string dumpPath = "dist\\ProcessDump_" + std::to_string(proc.processId) + ".dmp";
                            if (CreateProcessDump(proc.processId, dumpPath))
                            {
                                consoleEntries.push_back({ LOG_DUMP_CREATED, dumpPath });
                            }
                        }
                        ImGui::SameLine();
                        std::string killBtnLabel = std::string(Tr(STR_TERMINATE_PROC)) + " ##" + std::to_string(proc.processId);
                        if (ImGui::Button(killBtnLabel.c_str()))
                        {
                            if (TerminateProcessByPid(proc.processId))
                            {
                                consoleEntries.push_back({ LOG_PROC_KILLED, std::to_string(proc.processId) });
                                processList = SysCore::Diagnostics::ProcessTreeManager::GetFlatProcessList();
                            }
                        }
                    }
                    ImGui::EndTable();
                }

                ImGui::Separator();

                // Memory Scan Section
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_AUDIT_WX_TITLE));
                ImGui::InputText(Tr(STR_TARGET_PID), pidInput, sizeof(pidInput));
                ImGui::SameLine();
                if (ImGui::Button(Tr(STR_SCAN_VM), ImVec2(220, 32)))
                {
                    DWORD targetPid = static_cast<DWORD>(atoi(pidInput));
                    scannedRegions = ScanProcessMemoryRegions(targetPid);
                    loadedDlls = GetProcessLoadedDlls(targetPid);
                    hasScannedMemory = true;
                    static const LocString memScanLog = {
                        "[MEMORY AUDIT] Scanned regions for PID ",
                        "[АУДИТ ПАМЯТИ] Просканированы регионы для PID ",
                        "[内存审计] 已扫描进程区域，PID: "
                    };
                    consoleEntries.push_back({ memScanLog, std::to_string(targetPid) });
                }

                if (hasScannedMemory)
                {
                    size_t execCount = 0;
                    bool wxViolation = false;
                    for (const auto& r : scannedRegions) {
                        if (r.protection == PAGE_EXECUTE || r.protection == PAGE_EXECUTE_READ || r.protection == PAGE_EXECUTE_READWRITE || r.protection == PAGE_EXECUTE_WRITECOPY)
                            execCount++;
                        if (r.protection == PAGE_EXECUTE_READWRITE)
                            wxViolation = true;
                    }

                    ImGui::Text("Total Regions: %zu | Executable Regions: %zu | W^X Status: ", scannedRegions.size(), execCount);
                    ImGui::SameLine();
                    if (wxViolation)
                        ImGui::TextColored(ImVec4(1.00f, 0.30f, 0.30f, 1.00f), "%s", Tr(STR_WX_FAIL));
                    else
                        ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%s", Tr(STR_WX_PASS));

                    // Token Privileges Display for audited PID
                    DWORD targetPid = static_cast<DWORD>(atoi(pidInput));
                    auto privsList = GetProcessTokenPrivileges(targetPid);
                    if (!privsList.empty())
                    {
                        ImGui::TextColored(ImVec4(1.00f, 0.80f, 0.20f, 1.00f), "Token Privileges Count: %zu", privsList.size());
                        ImGui::SameLine();
                        std::string privSummary = "";
                        for (size_t k = 0; k < std::min(privsList.size(), (size_t)4); ++k)
                        {
                            if (k > 0) privSummary += ", ";
                            privSummary += privsList[k].name;
                        }
                        if (privsList.size() > 4) privSummary += "...";
                        ImGui::Text("(%s)", privSummary.c_str());
                    }

                    // Loaded DLL Modules Display
                    if (!loadedDlls.empty())
                    {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "Loaded DLL Modules (Count: %zu)", loadedDlls.size());
                        if (ImGui::BeginTable("LoadedDllsTable", 3, 
                            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, 
                            ImVec2(0, 140)))
                        {
                            ImGui::TableSetupColumn("DLL Name", ImGuiTableColumnFlags_None, 0.0f, 0);
                            ImGui::TableSetupColumn("Base Address", ImGuiTableColumnFlags_None, 0.0f, 1);
                            ImGui::TableSetupColumn("DLL Full Path", ImGuiTableColumnFlags_None, 0.0f, 2);
                            ImGui::TableHeadersRow();

                            for (const auto& dll : loadedDlls)
                            {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%s", dll.moduleName.c_str());
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("0x%p", (void*)dll.baseAddress);
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%s", dll.modulePath.c_str());
                            }
                            ImGui::EndTable();
                        }
                    }

                    // Memory Regions Table (NOW WITH DYNAMIC COLUMN SORTING BY ADDRESS, SIZE, TYPE, PROTECTION!)
                    if (ImGui::BeginTable("MemTable", 4, 
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable, 
                        ImVec2(0, 160)))
                    {
                        ImGui::TableSetupColumn(Tr(STR_COL_BASE_ADDR), ImGuiTableColumnFlags_DefaultSort, 0.0f, 0);
                        ImGui::TableSetupColumn(Tr(STR_COL_REG_SIZE), ImGuiTableColumnFlags_None, 0.0f, 1);
                        ImGui::TableSetupColumn(Tr(STR_COL_TYPE), ImGuiTableColumnFlags_None, 0.0f, 2);
                        ImGui::TableSetupColumn(Tr(STR_COL_PROTECT), ImGuiTableColumnFlags_None, 0.0f, 3);
                        ImGui::TableHeadersRow();

                        // Sorting logic for Memory Regions Table
                        if (ImGuiTableSortSpecs* memSortSpecs = ImGui::TableGetSortSpecs())
                        {
                            if (memSortSpecs->SpecsDirty && memSortSpecs->SpecsCount > 0)
                            {
                                const auto& spec = memSortSpecs->Specs[0];
                                std::sort(scannedRegions.begin(), scannedRegions.end(), [&spec](const auto& a, const auto& b) {
                                    bool ascending = (spec.SortDirection == ImGuiSortDirection_Ascending);
                                    switch (spec.ColumnUserID)
                                    {
                                    case 0: return ascending ? (a.baseAddress < b.baseAddress) : (a.baseAddress > b.baseAddress);
                                    case 1: return ascending ? (a.sizeBytes < b.sizeBytes) : (a.sizeBytes > b.sizeBytes);
                                    case 2: return ascending ? (a.type < b.type) : (a.type > b.type);
                                    case 3: return ascending ? (a.protection < b.protection) : (a.protection > b.protection);
                                    default: return false;
                                    }
                                });
                                memSortSpecs->SpecsDirty = false;
                            }
                        }

                        size_t maxCount = (scannedRegions.size() < 200) ? scannedRegions.size() : 200;
                        for (size_t i = 0; i < maxCount; ++i)
                        {
                            const auto& reg = scannedRegions[i];
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("0x%p", (void*)reg.baseAddress);
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%.2f KB", reg.sizeBytes / 1024.0f);
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%lu", reg.type);
                            ImGui::TableSetColumnIndex(3);
                            if (reg.protection == PAGE_EXECUTE_READWRITE)
                                ImGui::TextColored(ImVec4(1.00f, 0.30f, 0.30f, 1.00f), "0x%X (PAGE_EXECUTE_READWRITE)", reg.protection);
                            else
                                ImGui::Text("0x%X", reg.protection);
                        }
                        ImGui::EndTable();
                    }
                }

                ImGui::EndTabItem();
            }

            // TAB 4: Windows Kernel Drivers Auditor (NEW TAB!)
            if (ImGui::BeginTabItem(Tr(STR_TAB_DRV)))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_DRV_TITLE));

                if (ImGui::Button(Tr(STR_REFRESH_DRV), ImVec2(240, 32)))
                {
                    driversList = GetLoadedKernelDrivers();
                    consoleEntries.push_back({ LOG_DRIVERS_REFRESHED, std::to_string(driversList.size()) });
                }

                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "(Loaded Kernel Drivers: %zu)", driversList.size());

                if (ImGui::BeginTable("KernelDriversTable", 2, 
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, 
                    ImVec2(0, 320)))
                {
                    ImGui::TableSetupColumn("Kernel Driver Name", ImGuiTableColumnFlags_None, 0.0f, 0);
                    ImGui::TableSetupColumn("Kernel Base Address", ImGuiTableColumnFlags_None, 0.0f, 1);
                    ImGui::TableHeadersRow();

                    for (const auto& drv : driversList)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%s", drv.driverName.c_str());

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("0x%p", (void*)drv.baseAddress);
                    }
                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            // TAB 5: Windows Autoruns & Startup
            if (ImGui::BeginTabItem(Tr(STR_TAB_AUTO)))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_AUTO_TITLE));
                
                if (ImGui::Button(Tr(STR_REFRESH_AUTO), ImVec2(240, 32)))
                {
                    autorunsList = GetWindowsAutoruns();
                    consoleEntries.push_back({ LOG_AUTORUNS_REFRESHED, std::to_string(autorunsList.size()) });
                }

                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "(Discovered Items: %zu)", autorunsList.size());

                if (ImGui::BeginTable("AutorunsTable", 3, 
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, 
                    ImVec2(0, 320)))
                {
                    ImGui::TableSetupColumn(Tr(STR_COL_ITEM_NAME), ImGuiTableColumnFlags_None, 0.0f, 0);
                    ImGui::TableSetupColumn(Tr(STR_COL_ITEM_CMD), ImGuiTableColumnFlags_None, 0.0f, 1);
                    ImGui::TableSetupColumn(Tr(STR_COL_ITEM_LOC), ImGuiTableColumnFlags_None, 0.0f, 2);
                    ImGui::TableHeadersRow();

                    for (const auto& item : autorunsList)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%s", item.name.c_str());

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", item.command.c_str());

                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", item.location.c_str());
                    }
                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            // TAB 6: File Integrity & Crypto Hashing
            if (ImGui::BeginTabItem(Tr(STR_TAB_HASH)))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_HASH_TITLE));
                
                ImGui::SetNextItemWidth(500);
                ImGui::InputText(Tr(STR_FILE_PATH_INPUT), fileHashInput, sizeof(fileHashInput));
                ImGui::SameLine();
                if (ImGui::Button(Tr(STR_COMPUTE_HASHES), ImVec2(240, 32)))
                {
                    if (CalculateFileHashes(fileHashInput, md5Result, sha256Result))
                    {
                        static const LocString hashLog = {
                            "[CRYPTO HASH] Successfully computed MD5 & SHA-256 for: ",
                            "[КРИПТОХЕШ] Успешно вычислены MD5 и SHA-256 для: ",
                            "[密码哈希] 成功计算 MD5 和 SHA-256，文件: "
                        };
                        consoleEntries.push_back({ hashLog, fileHashInput });
                    }
                    else
                    {
                        md5Result = "Error: File not found or read access denied.";
                        sha256Result = "Error: File not found or read access denied.";
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "MD5 Hash:");
                ImGui::TextUnformatted(md5Result.c_str());

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "SHA-256 Hash:");
                ImGui::TextUnformatted(sha256Result.c_str());

                ImGui::EndTabItem();
            }

            // TAB 7: Lua Script Engine
            if (ImGui::BeginTabItem(Tr(STR_TAB_LUA)))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_LUA_TITLE));
                ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "%s", Tr(STR_LUA_DESC));

                ImGui::InputTextMultiline("##LuaEditor", luaEditorBuffer, sizeof(luaEditorBuffer), ImVec2(-1, 170));

                if (ImGui::Button(Tr(STR_EXEC_LUA), ImVec2(240, 40)))
                {
                    auto scriptRes = SysCore::Scripting::LuaScriptEngine::ExecuteScript(luaEditorBuffer);
                    luaOutputLog = scriptRes.outputLog;
                    static const LocString luaExecLog = {
                        "[LUA ENGINE] Executed script in ms: ",
                        "[ДВИЖОК LUA] Скрипт выполнен за мс: ",
                        "[LUA 引擎] 脚本执行耗时 (毫秒): "
                    };
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(2) << scriptRes.executionTimeMs;
                    consoleEntries.push_back({ luaExecLog, ss.str() });
                }

                ImGui::SameLine();
                if (ImGui::Button(Tr(STR_PRESET_MEM), ImVec2(220, 40)))
                {
                    auto templates = SysCore::Scripting::LuaScriptEngine::GetPresetTemplates();
                    snprintf(luaEditorBuffer, sizeof(luaEditorBuffer), "%s", templates[0].second.c_str());
                }

                ImGui::SameLine();
                if (ImGui::Button(Tr(STR_PRESET_BENCH), ImVec2(220, 40)))
                {
                    auto templates = SysCore::Scripting::LuaScriptEngine::GetPresetTemplates();
                    snprintf(luaEditorBuffer, sizeof(luaEditorBuffer), "%s", templates[1].second.c_str());
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%s", Tr(STR_LUA_STREAM));
                ImGui::BeginChild("LuaOutputChild", ImVec2(0, 130), true);
                ImGui::TextUnformatted(luaOutputLog.c_str());
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            // TAB 8: Network Socket Monitor (WITH DYNAMIC SORTING!)
            if (ImGui::BeginTabItem(Tr(STR_TAB_NET)))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_NET_TITLE));

                static auto socketConnections = SysCore::Network::NetworkMonitor::GetActiveSockets();
                if (ImGui::Button(Tr(STR_REFRESH_NET), ImVec2(320, 40)))
                {
                    socketConnections = SysCore::Network::NetworkMonitor::GetActiveSockets();
                    static const LocString netRefLog = {
                        "[NETWORK AUDIT] Refreshed sockets. Discovered total: ",
                        "[СЕТЕВОЙ АУДИТ] Сокеты обновлены. Всего обнаружено: ",
                        "[网络审计] 已刷新套接字。发现总数: "
                    };
                    consoleEntries.push_back({ netRefLog, std::to_string(socketConnections.size()) });
                }

                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "(Active Socket Connections: %zu)", socketConnections.size());

                if (ImGui::BeginTable("NetSocketTable", 6, 
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable, 
                    ImVec2(0, 300)))
                {
                    ImGui::TableSetupColumn(Tr(STR_COL_PROTO), ImGuiTableColumnFlags_None, 0.0f, 0);
                    ImGui::TableSetupColumn(Tr(STR_COL_LADDR), ImGuiTableColumnFlags_None, 0.0f, 1);
                    ImGui::TableSetupColumn(Tr(STR_COL_RADDR), ImGuiTableColumnFlags_None, 0.0f, 2);
                    ImGui::TableSetupColumn(Tr(STR_COL_STATE), ImGuiTableColumnFlags_None, 0.0f, 3);
                    ImGui::TableSetupColumn(Tr(STR_COL_PID), ImGuiTableColumnFlags_DefaultSort, 0.0f, 4);
                    ImGui::TableSetupColumn(Tr(STR_COL_PNAME), ImGuiTableColumnFlags_None, 0.0f, 5);
                    ImGui::TableHeadersRow();

                    // Apply Table Sorting Specs
                    if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs())
                    {
                        if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0)
                        {
                            const auto& spec = sort_specs->Specs[0];
                            std::sort(socketConnections.begin(), socketConnections.end(), [&spec](const auto& a, const auto& b) {
                                bool ascending = (spec.SortDirection == ImGuiSortDirection_Ascending);
                                switch (spec.ColumnUserID)
                                {
                                case 0: return ascending ? (a.protocol < b.protocol) : (a.protocol > b.protocol);
                                case 1: return ascending ? (a.localPort < b.localPort) : (a.localPort > b.localPort);
                                case 2: return ascending ? (a.remotePort < b.remotePort) : (a.remotePort > b.remotePort);
                                case 3: return ascending ? (a.state < b.state) : (a.state > b.state);
                                case 4: return ascending ? (a.processId < b.processId) : (a.processId > b.processId);
                                case 5: return ascending ? (a.processName < b.processName) : (a.processName > b.processName);
                                default: return false;
                                }
                            });
                            sort_specs->SpecsDirty = false;
                        }
                    }

                    for (size_t i = 0; i < std::min(socketConnections.size(), (size_t)150); ++i)
                    {
                        const auto& sock = socketConnections[i];
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        if (sock.protocol == "TCP")
                            ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%s", sock.protocol.c_str());
                        else
                            ImGui::TextColored(ImVec4(1.00f, 0.80f, 0.20f, 1.00f), "%s", sock.protocol.c_str());

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s:%u", sock.localAddress.c_str(), sock.localPort);

                        ImGui::TableSetColumnIndex(2);
                        if (sock.remotePort != 0)
                            ImGui::Text("%s:%u", sock.remoteAddress.c_str(), sock.remotePort);
                        else
                            ImGui::Text("%s", sock.remoteAddress.c_str());

                        ImGui::TableSetColumnIndex(3);
                        if (sock.state == "ESTABLISHED")
                            ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%s", sock.state.c_str());
                        else if (sock.state == "LISTENING")
                            ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", sock.state.c_str());
                        else
                            ImGui::Text("%s", sock.state.c_str());

                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("%lu", sock.processId);

                        ImGui::TableSetColumnIndex(5);
                        ImGui::Text("%s", sock.processName.c_str());
                    }
                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }

            // TAB 9: Directory Watcher & IPC Telemetry
            if (ImGui::BeginTabItem(Tr(STR_TAB_DIR)))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_DIR_TITLE));
                ImGui::Text("%s", Tr(STR_DIR_STATUS));
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.00f, 0.95f, 0.55f, 1.00f), "%s", Tr(STR_DIR_ACTIVE));

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.00f, 0.88f, 1.00f, 1.00f), "%s", Tr(STR_IPC_TITLE));
                if (ImGui::Button(Tr(STR_PUBLISH_IPC), ImVec2(380, 40)))
                {
                    core.GetIpcManager().SendTelemetry("GUI Telemetry Trigger Event #" + std::to_string(rand() % 100));
                    consoleEntries.push_back({ LOG_IPC_PUBLISHED, "" });
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // Bottom Console Log Area (WITH FULL MULTI-LANGUAGE DYNAMIC TRANSLATION!)
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.85f, 0.90f, 0.98f, 1.00f), "%s", Tr(STR_CONSOLE_LOG));
        ImGui::BeginChild("ConsoleLogChild", ImVec2(0, 110), true);
        for (const auto& entry : consoleEntries)
        {
            std::string line = std::string(Tr(entry.locStr)) + entry.extraData;
            ImGui::TextUnformatted(line.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();

        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.08f, 0.10f, 0.14f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }

    // Cleanup ImGui & DirectX 11
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper Functions for Direct3D 11 Initialization
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Win32 Message Handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
