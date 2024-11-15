#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <filesystem>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

#include "main.h"
#include "Theme/Theme.h"
#include "imgui/imgui_impl_win32.h"
#include "UbuntuMono-B.h"
#include "prepod/prepodgui.h"
#include "student/studentgui.h"
#include "json/json.hpp"
#include "Scheme/scheme.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "213.108.23.81"
#define SERVER_PORT 12345

struct ServerStatus {
    std::string status;
    std::string uptime;
};

ServerStatus serverStatus = {"disconnected", "N/A"};

namespace fs = std::filesystem;

// -------------------------- Global Variables --------------------------

bool reset_size = true;
bool ncmenu = true;
bool showConsole = false;
bool showDebugWindow = false;
bool showServerInfo = false;
int tabs = 1;  // Unused, but might be used later
Theme theme;

enum UserRole {
    NONE,
    TEACHER,
    STUDENT,
    DEBUG
};
UserRole user_role = NONE;

// -------------------------- Debugging & Build Information --------------------------

std::string generateBuildHash(const std::string& input) {
    std::hash<std::string> hasher;
    auto hashValue = hasher(input);

    std::stringstream ss;
    ss << std::hex << hashValue;
    std::string hashString = ss.str();

    if (hashString.length() > 5) {
        hashString = hashString.substr(0, 5);
    }

    return hashString;
}


const char* buildDate = __DATE__;
const char* buildTime = __TIME__;
std::string buildString = std::string(buildDate) + " " + buildTime;
std::string buildHash = generateBuildHash(buildString);
std::string str_window_title = "Kursach";
const char* window_title = str_window_title.c_str();
std::string consoleTitle = "Kursach | Build ID: " + buildHash + " | Build Date: " + buildDate + buildTime;


// -------------------------- Functions --------------------------

void CreateDataDirectories() {
    fs::path dataDir = fs::current_path() / "data";
    fs::path debugDir = fs::current_path() / "debug";
    fs::path studentDataDir = dataDir / "student_data";
    fs::path prepodDataDir = dataDir / "prepod_data";

    if (!fs::exists(dataDir)) fs::create_directory(dataDir);
    if (!fs::exists(studentDataDir)) fs::create_directory(studentDataDir);
    if (!fs::exists(prepodDataDir)) fs::create_directory(prepodDataDir);
    if (!fs::exists(debugDir)) fs::create_directory(debugDir);
}

// ------------------------- Server Logic --------------------------

void sendStatistics(SOCKET sock) {

    nlohmann::json statistics;
    statistics["build"] = "Stable";
    statistics["build_hash"] = buildHash;
    statistics["build_date"] = buildDate;
    statistics["build_time"] = buildTime;


    std::string statsString = statistics.dump();


    int result = send(sock, statsString.c_str(), statsString.size(), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Failed to send statistics" << std::endl;
    } else {
        std::cout << "Statistics sent successfully" << std::endl;
    }
}

void connectToServer() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in serverAddr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WinSock initialization failed!" << std::endl;
        return;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error creating socket" << std::endl;
        WSACleanup();
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed!" << std::endl;
        closesocket(sock);
        WSACleanup();
        return;
    }

    std::cout << "Connected to server!" << std::endl;


    sendStatistics(sock);

    closesocket(sock);
    WSACleanup();
}
// -------------------------- Main Program --------------------------


int main() {
    CreateDataDirectories();
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(console, 1); 

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ExternalProB1"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("ExternalProB1"), WS_OVERLAPPEDWINDOW, 0, 0, 50, 50, NULL, NULL, wc.hInstance, NULL);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(GetConsoleWindow(), SW_HIDE);
    UpdateWindow(hwnd);
    connectToServer();
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImFont* myFont = io.Fonts->AddFontFromMemoryTTF(UbuntuMono_B_ttf, UbuntuMono_B_ttf_len, 16.0f);
    if (myFont == nullptr) {
        std::cerr << "Failed to load font." << std::endl;
    } else {
        io.FontDefault = myFont;
    }
    
    theme.Register(io);

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 4.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);


    std::wstring wideConsoleTitle = std::wstring(consoleTitle.begin(), consoleTitle.end());
    SetConsoleTitle(wideConsoleTitle.c_str());

    bool done = true;
    while (done) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = false;
        }

        if (!done) break;

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();


        if (ncmenu) {
            if (reset_size) {
                ImGui::SetNextWindowSize(ImVec2(690, 450));
                reset_size = false;
            }

            ImGui::SetNextWindowBgAlpha(1.0f);
            ImGui::Begin(window_title, &ncmenu, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);


            if (user_role == TEACHER) {
                ShowPrepodGUI();
            } else if (user_role == STUDENT) {
                ShowStudentGUI();
            }

            if (user_role == NONE) {
                ImVec2 available = ImGui::GetContentRegionAvail();
                float buttonWidth = 120;
                float buttonHeight = 30;

                ImGui::SetCursorPosX((available.x - buttonWidth) / 2);
                ImGui::SetCursorPosY((available.y - (buttonHeight * 2 + 10)) / 2);

                if (ImGui::Button("Teacher", ImVec2(buttonWidth, buttonHeight))) {
                    user_role = TEACHER;
                }

                ImGui::Dummy(ImVec2(0.0f, 0.0f));

                ImGui::SetCursorPosX((available.x - buttonWidth) / 2);
                if (ImGui::Button("Student", ImVec2(buttonWidth, buttonHeight))) {
                    user_role = STUDENT;
                }
            }
            
            ImVec2 windowSize = ImGui::GetWindowSize();
            float buttonSize = 50.0f;

            ImGui::SetCursorPosX(windowSize.x - buttonSize - 100);
            ImGui::SetCursorPosY(25);
            ImGui::Checkbox("Console", &showConsole);

            if (showConsole) {
                ShowWindow(GetConsoleWindow(), SW_SHOW);
            } else {
                ShowWindow(GetConsoleWindow(), SW_HIDE);
            }

            // Debug window toggle
            ImGui::SetCursorPosX(windowSize.x - buttonSize - 225);
            ImGui::SetCursorPosY(25);
            ImGui::Checkbox("Debug Info", &showDebugWindow);
            
            ImGui::SetCursorPosX(windowSize.x - buttonSize - 350);
            ImGui::SetCursorPosY(25);
            ImGui::Checkbox("Server Info", &showServerInfo);


            ImGui::SetCursorPosX(windowSize.x - buttonSize - 5);
            ImGui::SetCursorPosY(25);
            if (ImGui::Button("Exit", ImVec2(buttonSize, 20))) {
                user_role = NONE;
            }
            if (showDebugWindow) {
                
                ImGui::Begin("Debug Window", &showDebugWindow, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
                ImGui::Text("Build: Stable");   
                ImGui::Text("Build Hash: %s", buildHash.c_str());
                ImGui::Text("Build Date: %s", buildDate);
                ImGui::Text("Build Time: %s", buildTime);
                ImGui::End();
            }
            
            if (showServerInfo)
            {
                
                ImGui::Begin("Server Info", &showServerInfo, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
                if (ImGui::Button("Send Statistics")) {
                    connectToServer();
                }
                ImGui::End();
            }


            ImGui::End();
        } else {
            exit(0);
        }

        ImGui::EndFrame();

        // Render
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, NULL, 1.0f, 0);

        if (g_pd3dDevice->BeginScene() >= 0) {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
            ResetDevice();
        }
    }

    // Cleanup
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
