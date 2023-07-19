#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <cstddef>
#include <vector>
#include <string>
#include"functions.h"


const char g_szClassName[] = "myWindowClass";

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static POINT startPoint{};
    static POINT endPoint{};
    static float lineLength = 1000.0f;
    static UINT_PTR timer = 0;                   // Declare the timer variable
    static int windowWidth = 0;                  // Store the window width
    static int windowHeight = 0;                 // Store the window height
    static ULONG_PTR gdiplusToken;
    static Gdiplus::Image* image = nullptr;



    switch (msg) {

    case WM_CREATE: {
        // Initialize GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

        // Load the image
        image = new Gdiplus::Image(L"mirage.png");

        CreateControls(hwnd);

        timer = SetTimer(hwnd, 1, 0, nullptr); // Start the timer to update the circle positions every second
        HWND GameHWND = FindWindowW(NULL, L"Counter-Strike: Global Offensive - Direct3D 9");
        DWORD dwPid;
        GetWindowThreadProcessId(GameHWND, &dwPid);
        TargetProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
        moduleBase = GetModuleBaseAddress(dwPid, L"client.dll");
        engine = GetModuleBaseAddress(dwPid, L"engine.dll");

        // Initialize circle colors and positions
        for (auto i = 1; i <= 32; ++i) {
            circlePositions.push_back(Vector3());
            const auto localPlayer = RPM<std::uintptr_t>(moduleBase + dwLocalPlayer);
            const auto localTeam = RPM<std::int32_t>(localPlayer + m_iTeamNum);
            const auto player = RPM<std::uintptr_t>(moduleBase + dwEntityList + i * 0x10);
            const auto onTeam = RPM<std::int32_t>(player + m_iTeamNum) == localTeam;
            const auto defusing = RPM<std::int32_t>(player + m_bIsDefusing);


            if (onTeam == localTeam) {
                if (defusing != 0) {
                    circleColors.push_back(RGB(255, 165, 0));
                }
                else {
                    circleColors.push_back(RGB(0, 0, 255));
                }
            }
            else if (onTeam != localTeam) {
                if (defusing != 0) {
                    circleColors.push_back(RGB(255, 165, 0));
                }
                else {
                    circleColors.push_back((255, 0, 0));
                }
            }
        }
        break;
    }

    case WM_CLOSE:
        //close window
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        // Clean up resources
        delete image;
        Gdiplus::GdiplusShutdown(gdiplusToken);
        PostQuitMessage(0);
        return 0;
        KillTimer(hwnd, timer);// Kill the timer when the window is destroyed
        PostQuitMessage(0);
        CloseHandle(TargetProcess);
        break;
    case WM_SIZE:
        windowWidth = LOWORD(lParam);// Update the window width
        windowHeight = HIWORD(lParam);// Update the window height
        break;
    case WM_ERASEBKGND:
        return FALSE;
    case WM_TIMER: {
        if (wParam == 1) {
            // Clear the circle positions and colors
            circlePositions.clear();
            circleColors.clear();

            // Update the line start and end points
            const auto clientState = RPM<std::uintptr_t>(engine + dwClientState);

            const auto localPlayer = RPM<std::uintptr_t>(moduleBase + dwLocalPlayer);

            const auto localEyePosition = RPM<Vector3>(localPlayer + m_vecOrigin) + RPM<Vector3>(localPlayer + m_vecViewOffset);

            const auto viewAngles = RPM<Vector3>(clientState + dwClientState_ViewAngles);

            float convertedAngle = viewAngles.y >= 0 ? viewAngles.y : 180 + (180 + viewAngles.y);

            Vector3 forwardVector{
                cosf(convertedAngle * 3.14159265f / 180.0f) * lineLength,
                sinf(convertedAngle * 3.14159265f / 180.0f) * lineLength,
                0.0f
            };

            Vector3 endPointPosition = localEyePosition + forwardVector;


            startPoint.x = static_cast<LONG>((localEyePosition.x) * (static_cast<float>(windowWidth) / mapWidth)) + offsetX;
            startPoint.y = windowHeight - static_cast<int>((localEyePosition.y) * (static_cast<float>(windowHeight) / mapHeight)) - offsetY;

            endPoint.x = static_cast<LONG>((endPointPosition.x) * (static_cast<float>(windowWidth) / mapWidth)) + offsetX;
            endPoint.y = windowHeight - static_cast<int>((endPointPosition.y) * (static_cast<float>(windowHeight) / mapHeight)) - offsetY;


            // Update circle positions
            updateCirclePositions();

            // Redraw the window    
            InvalidateRect(hwnd, NULL, TRUE);

        }
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1 && HIWORD(wParam) == BN_CLICKED) {
            // The checkbox was clicked
            HWND hCheckBox = GetDlgItem(hwnd, 1);
            BOOL isChecked = SendMessage(hCheckBox, BM_GETCHECK, 0, 0);

            if (isChecked == BST_CHECKED) {
                TriggerOn = true;
            }
            else {
                TriggerOn = false;
            }
        }
        else if (LOWORD(wParam) == 2 && HIWORD(wParam) == CBN_SELCHANGE) {
            // Combo box selection has changed
            int selectedIndex = SendMessageW(hComboBox, CB_GETCURSEL, 0, 0);
            wchar_t mapName[256];
            SendMessageW(hComboBox, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(mapName));
            UpdateMapVariables(mapName);

            // Load the corresponding image based on the selected map
            std::wstring imagePath;
            if (wcscmp(mapName, L"Dust 2") == 0) {
                imagePath = L"dust.png";
            }
            else if (wcscmp(mapName, L"Mirage") == 0) {
                imagePath = L"mirage.png";
            }
            else if (wcscmp(mapName, L"Inferno") == 0) {
                imagePath = L"inferno.png";
            }
            // Add more conditions for other map options

            // Load the image
            delete image;
            image = new Gdiplus::Image(imagePath.c_str());

            // Redraw the window to show the new image
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    }

    case WM_PAINT: {

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (!memDC) {
            memDC = CreateCompatibleDC(hdc);
            memBitmap = CreateCompatibleBitmap(hdc, windowWidth, windowHeight);
            oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
        }

        HDC memDC = CreateCompatibleDC(hdc);

        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, windowWidth, windowHeight);

        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        Gdiplus::Graphics graphics(memDC);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int targetWidth = 1024;
        int targetHeight = 1024;
        float aspectRatio = static_cast<float>(image->GetWidth()) / image->GetHeight();
        if (targetWidth / aspectRatio > targetHeight)
        {
            targetWidth = static_cast<int>(targetHeight * aspectRatio);
        }
        else
        {
            targetHeight = static_cast<int>(targetWidth / aspectRatio);
        }

        int targetX = (clientRect.right - clientRect.left - targetWidth) / 2;
        int targetY = (clientRect.bottom - clientRect.top - targetHeight) / 2;

        graphics.DrawImage(image, targetX, targetY, targetWidth, targetHeight);

        // draw controls
        DrawControls(hwnd);

        ////////////////////////////////////////////////////////////////////////////////////////////////////

        const auto localPlayer = RPM<std::uintptr_t>(moduleBase + dwLocalPlayer);

        if (TriggerOn) {
            auto crossId = RPM<int>(localPlayer + m_iCrosshairId);
            if (crossId > 0 && crossId < 64 && (GetAsyncKeyState(VK_MENU) & 0x8000))
            {
                const auto localPlayerTeam = RPM<int>(localPlayer + m_iTeamNum);
                const auto targetPlayerTeam = RPM<int>(moduleBase + dwEntityList + (crossId - 1) * 0x10 + m_iTeamNum);

                if (localPlayerTeam != targetPlayerTeam)
                {
                    LeftClick();
                    Sleep(10);
                }
            }
        }


        SetTextColor(memDC, RGB(255, 255, 255)); // White font color

        // local player line
        DrawLine(memDC, startPoint, endPoint);
        // player lines
        DrawPlayerViewLines(memDC, moduleBase, engine, windowWidth, windowHeight);



        // GRABBING LOCALPLAYER POSITION TO SUBTRACT FROM PLAYER POSITION TO CALC TRIANGLES
        const auto localEyePosition = RPM<Vector3>(localPlayer + m_vecOrigin) + RPM<Vector3>(localPlayer + m_vecViewOffset);

        // Draw circles players
        for (size_t i = 0; i < circlePositions.size(); ++i) {
            const Vector3& playerPosition = circlePositions[i];
            const COLORREF circleColor = circleColors[i];

            // Determine the scaling factors and offsets
            float scaleX = static_cast<float>(windowWidth) / mapWidth;
            float scaleY = static_cast<float>(windowHeight) / mapHeight;

            // Map player positions to window coordinates
            int x = static_cast<int>((playerPosition.x) * scaleX) + offsetX;
            int y = windowHeight - static_cast<int>((playerPosition.y) * scaleY) - offsetY;
            int z = playerPosition.z;

            // draw circles on the memory DC
            HBRUSH hCircleBrush = CreateSolidBrush(circleColor);
            HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(memDC, hCircleBrush));

            int left = x - radius;
            int top = y - radius;
            int right = x + radius;
            int bottom = y + radius;

            Ellipse(memDC, left, top, right, bottom);

            SelectObject(memDC, hOldBrush);
            DeleteObject(hCircleBrush);

            if ((localEyePosition.z - playerPosition.z) > 25) {
                //below
                int triangleX = x + 10; // Example X coordinate
                int triangleY = y + 5; // Example Y coordinate
                int triangleSize = 8; // Example size
                COLORREF triangleColor = RGB(251, 72, 196); // Example color
                DrawUpsideDownTriangle(memDC, triangleX, triangleY, triangleSize, triangleColor);
            }
            else if ((localEyePosition.z - playerPosition.z) < -25) {
                // above
                int triangleX = x + 10; // Example X coordinate
                int triangleY = y + 5; // Example Y coordinate
                int triangleSize = 8; // Example size
                COLORREF triangleColor = RGB(0, 255, 0); // Example color

                DrawTriangle(memDC, triangleX, triangleY, triangleSize, triangleColor);
            }
        }

        // Copy the contents of the memory DC to the window's DC
        BitBlt(hdc, 0, 0, windowWidth, windowHeight, memDC, 0, 0, SRCCOPY);

        // Clean up
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


//  entry point for a Windows application

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    //Step 1: Registering the Window Class
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);


    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TOPMOST, g_szClassName, "Aid0nWare", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, screenSize_X, screenSize_Y, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Step 3: The Message Loop
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Msg.wParam;
}
