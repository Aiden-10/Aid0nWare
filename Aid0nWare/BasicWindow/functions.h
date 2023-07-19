#pragma once
#include "globals.h"

uintptr_t GetModuleBaseAddress(DWORD dwPid, const wchar_t* moduleName) {
    uintptr_t dwBase = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwPid);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W ModuleEntry32;
        ModuleEntry32.dwSize = sizeof(MODULEENTRY32W);
        if (Module32FirstW(hSnapshot, &ModuleEntry32)) {
            do {
                if (!wcscmp(ModuleEntry32.szModule, moduleName)) {
                    dwBase = (uintptr_t)ModuleEntry32.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnapshot, &ModuleEntry32));
        }
        CloseHandle(hSnapshot);
    }
    return dwBase;
}

void DrawUpsideDownTriangle(HDC hdc, int x, int y, int size, COLORREF color) {
    HPEN hPen = CreatePen(PS_SOLID, 2, color);
    HBRUSH hBrush = CreateSolidBrush(color);

    HGDIOBJ hOldPen = SelectObject(hdc, hPen);
    HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);

    POINT points[3] = { {x - size / 2, y}, {x + size / 2, y}, {x, y + size} };
    Polygon(hdc, points, 3);

    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);

    DeleteObject(hPen);
    DeleteObject(hBrush);
}

void DrawTriangle(HDC hdc, int x, int y, int size, COLORREF color) {
    HPEN hPen = CreatePen(PS_SOLID, 2, color);
    HBRUSH hBrush = CreateSolidBrush(color);

    HGDIOBJ hOldPen = SelectObject(hdc, hPen);
    HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);

    POINT points[3] = { {x, y + size}, {x + size, y + size}, {x + size / 2, y} };
    Polygon(hdc, points, 3);

    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);

    DeleteObject(hPen);
    DeleteObject(hBrush);
}

void DrawLine(HDC hdc, POINT startPoint, POINT endPoint) {
    HPEN hLinePen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0)); // Green color for line
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hLinePen));

    MoveToEx(hdc, startPoint.x, startPoint.y, nullptr);
    LineTo(hdc, endPoint.x, endPoint.y);

    SelectObject(hdc, hOldPen);
    DeleteObject(hLinePen);
}

void updateCirclePositions() {
    for (auto i = 0; i <= 32; ++i) {
        const auto localPlayer = RPM<std::uintptr_t>(moduleBase + dwLocalPlayer);
        const auto localTeam = RPM<std::int32_t>(localPlayer + m_iTeamNum);
        const auto player = RPM<std::uintptr_t>(moduleBase + dwEntityList + i * 0x10);
        const auto boneMatrix = RPM<std::uintptr_t>(player + m_dwBoneMatrix);

        if (RPM<std::int32_t>(player + m_lifeState))
            continue;

        if (RPM<std::int32_t>(player + m_bDormant))
            continue;

        Vector3 position{
            RPM<float>(boneMatrix + 0x30 * 8 + 0x0C),
            RPM<float>(boneMatrix + 0x30 * 8 + 0x1C),
            RPM<float>(boneMatrix + 0x30 * 8 + 0x2C)
        };

        circlePositions.push_back(position);

        COLORREF circleColor = RGB(255, 255, 255);

        if (RPM<std::int32_t>(player + m_iTeamNum) == localTeam) {
            if (RPM<bool>(player + m_bIsDefusing) != 0) {
                circleColor = RGB(255, 165, 0);
            }
            else {
                circleColor = RGB(0, 0, 255);
            }
        }
        else if (RPM<std::int32_t>(player + m_iTeamNum) != localTeam) {
            if (RPM<bool>(player + m_bIsDefusing) != 0) {
                circleColor = RGB(255, 165, 0);
            }
            else if (RPM<bool>(player + m_bIsScoped) != 0) {
                circleColor = RGB(255, 16, 240);
            }
            else {
                circleColor = RGB(255, 0, 0);
            }
        }

        circleColors.push_back(circleColor);
    }
}

float localEyePos() {
    const auto localPlayer = RPM<std::uintptr_t>(moduleBase + dwLocalPlayer);

    const auto localEyePosition = RPM<Vector3>(localPlayer + m_vecOrigin) + RPM<Vector3>(localPlayer + m_vecViewOffset);

    return localEyePosition.z;
}

std::string ReadString(uintptr_t address) {
    const int maxLength = 64;  // Maximum length of the player name

    char buffer[maxLength] = { 0 };
    ReadProcessMemory(TargetProcess, (LPCVOID)address, buffer, sizeof(buffer), NULL);

    // Find the null-terminator in the buffer
    int stringLength = 0;
    for (; stringLength < maxLength; ++stringLength) {
        if (buffer[stringLength] == '\0')
            break;
    }

    return std::string(buffer, stringLength);
}

void DrawPlayerViewLines(HDC hdc, uintptr_t moduleBase, uintptr_t engine, int windowWidth, int windowHeight) {
    const auto clientState = RPM<uintptr_t>(engine + dwClientState);
    const auto localPlayerIndex = RPM<int>(clientState + dwClientState_GetLocalPlayer);

    for (auto i = 0; i <= 32; ++i) {
        const auto player = RPM<uintptr_t>(moduleBase + dwEntityList + i * 0x10);

        if (i == 0)
        {
            continue;
        }


        const auto dormant = RPM<bool>(player + m_bDormant);
        if (dormant)
            continue;

        const auto lifeState = RPM<int>(player + m_lifeState);
        if (lifeState != 0)
            continue;

        const auto viewAnglesX = RPM<float>(player + m_angEyeAnglesX);
        const auto viewAnglesY = RPM<float>(player + m_angEyeAnglesY);



        float convertedAngle = viewAnglesY >= 0 ? viewAnglesY : 180 + (180 + viewAnglesY);


        Vector3 scopedForwardVector{
            cosf(convertedAngle * 3.14159265f / 180.0f) * 2000.f,
            sinf(convertedAngle * 3.14159265f / 180.0f) * 2000.f,
            0.0f
        };


        Vector3 forwardVector{
            cosf(convertedAngle * 3.14159265f / 180.0f) * 400.f,
            sinf(convertedAngle * 3.14159265f / 180.0f) * 400.f,
            0.0f
        };

        Vector3 playerPosition{
            RPM<float>(player + m_vecOrigin),
            RPM<float>(player + m_vecOrigin + 0x4),
            RPM<float>(player + m_vecOrigin + 0x8)
        };

        Vector3 endPointPosition = playerPosition + forwardVector;

        Vector3 scopedEndPointPosition = playerPosition + scopedForwardVector;

        POINT startPoint{
            static_cast<int>((playerPosition.x) * (static_cast<float>(windowWidth) / mapWidth)) + offsetX,
            windowHeight - static_cast<int>((playerPosition.y) * (static_cast<float>(windowHeight) / mapHeight)) - offsetY
        };

        POINT endPoint{
            static_cast<LONG>((endPointPosition.x) * (static_cast<float>(windowWidth) / mapWidth)) + offsetX,
            windowHeight - static_cast<int>((endPointPosition.y) * (static_cast<float>(windowHeight) / mapHeight)) - offsetY
        };

        POINT scopedStartPoint{
            static_cast<int>((playerPosition.x) * (static_cast<float>(windowWidth) / mapWidth)) + offsetX,
            windowHeight - static_cast<int>((playerPosition.y) * (static_cast<float>(windowHeight) / mapHeight)) - offsetY
        };

        POINT scopedEndPoint{
            static_cast<LONG>((scopedEndPointPosition.x) * (static_cast<float>(windowWidth) / mapWidth)) + offsetX,
            windowHeight - static_cast<int>((scopedEndPointPosition.y) * (static_cast<float>(windowHeight) / mapHeight)) - offsetY
        };

        const auto armorValue = RPM<int>(player + m_ArmorValue);
        const auto health = RPM<int>(player + m_iHealth);

        std::string armorText = "" + std::to_string(armorValue);
        std::string healthText = "" + std::to_string(health);

        HFONT hFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");

        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

        // Set text colors
        SetTextColor(hdc, RGB(0, 0, 255)); // Blue color for armor
        SetBkMode(hdc, TRANSPARENT); // Set background mode to transparent
        TextOutA(hdc, startPoint.x -15, startPoint.y + 5, armorText.c_str(), armorText.length());


        SetTextColor(hdc, RGB(255, 0, 0)); // Red color for health
        TextOutA(hdc, startPoint.x + 5, startPoint.y + 5, healthText.c_str(), healthText.length());

        SelectObject(hdc, hOldFont); // Restore the original font
        DeleteObject(hFont); // Delete the created font handle

        if (RPM<bool>(player + m_bIsScoped) != 0) {
            HPEN hLinePen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255)); // White color for line
            HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hLinePen));

            MoveToEx(hdc, startPoint.x, startPoint.y, nullptr);
            LineTo(hdc, scopedEndPoint.x, scopedEndPoint.y);

            SelectObject(hdc, hOldPen);
            DeleteObject(hLinePen);
        }
        else {
            HPEN hLinePen = CreatePen(PS_SOLID, 2, RGB(0, 125, 0)); // Green color for line
            HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hLinePen));

            MoveToEx(hdc, startPoint.x, startPoint.y, nullptr);
            LineTo(hdc, endPoint.x, endPoint.y);

            SelectObject(hdc, hOldPen);
            DeleteObject(hLinePen);
        }

        /*MoveToEx(hdc, startPoint.x, startPoint.y, nullptr);
        LineTo(hdc, endPoint.x, endPoint.y);

        SelectObject(hdc, hOldPen);
        DeleteObject(hLinePen);*/
    }
}


void LeftClick()
{
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));

    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; // Press left mouse button
    SendInput(1, &input, sizeof(INPUT));

    // Optionally, you can add a small delay between pressing and releasing the button
    Sleep(10); // Adjust the duration as needed

    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP; // Release left mouse button
    SendInput(1, &input, sizeof(INPUT));
}

void CreateControls(HWND hwnd) {
    // Create the checkbox
    hCheckBox = CreateWindowW(
        L"BUTTON",                          // Predefined class name for a button control
        L"Trigger Bot | L ALT ( ! ) Will Shoot Teammates", // Text to display on the checkbox
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, // Checkbox styles
        130, 10, 325, 20,                    // Position and size of the checkbox
        hwnd,                               // Parent window handle
        reinterpret_cast<HMENU>(1),         // Identifier of the checkbox control
        nullptr,                            // Instance handle
        nullptr);                           // Pointer to user-defined data

    // Create the combo box
    hComboBox = CreateWindowW(
        L"COMBOBOX",                        // Predefined class name for a combo box control
        nullptr,                            // No initial text
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, // Combo box styles
        475, 10, 100, 200,                   // Position and size of the combo box
        hwnd,                               // Parent window handle
        reinterpret_cast<HMENU>(2),         // Identifier of the combo box control
        nullptr,                            // Instance handle
        nullptr);                           // Pointer to user-defined data

    // Add map options to the ComboBox
    SendMessageW(hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Dust 2"));
    SendMessageW(hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mirage"));
    SendMessageW(hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Inferno"));
    // Add more map options as needed

    // Select the first map option as the default
    SendMessageW(hComboBox, CB_SETCURSEL, 0, 0);
}

void DrawControls(HWND hwnd) {
    // Get the checkbox's DC
    HDC checkboxDC = GetDC(hCheckBox);

    // Draw the checkbox
    RECT checkboxRect;
    GetClientRect(hCheckBox, &checkboxRect);
    DrawFrameControl(checkboxDC, &checkboxRect, DFC_BUTTON, DFCS_BUTTONCHECK | (IsDlgButtonChecked(hwnd, 1) ? DFCS_CHECKED : 0));

    // Release the checkbox's DC
    ReleaseDC(hCheckBox, checkboxDC);

    // Get the combo box's DC
    HDC comboBoxDC = GetDC(hComboBox);

    // Draw the combo box
    RECT comboBoxRect;
    GetClientRect(hComboBox, &comboBoxRect);
    DrawFrameControl(comboBoxDC, &comboBoxRect, DFC_SCROLL, DFCS_SCROLLCOMBOBOX);

    // Release the combo box's DC
    ReleaseDC(hComboBox, comboBoxDC);
}

void UpdateMapVariables(const std::wstring& mapName) {
    // Update the map-related variables based on the map name
    if (mapName == L"Dust 2") {
        mapWidth = 4400;
        mapHeight = 4250;
        offsetX = 555;
        offsetY = 265;
    }
    else if (mapName == L"Mirage") {
        mapWidth = 5000;
        mapHeight = 4900;
        offsetX = 635;
        offsetY = 660;
    }
    else if (mapName == L"Inferno") {
        mapWidth = 4890;
        mapHeight = 4850;
        offsetX = 415;
        offsetY = 212;
    }
    // Add more conditions for other map options
}

void DrawImageOnDC(HDC hdc, const wchar_t* imagePath, int targetWidth, int targetHeight) {
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Image image(imagePath);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    float aspectRatio = static_cast<float>(image.GetWidth()) / image.GetHeight();

    if (targetWidth / aspectRatio > targetHeight) {
        targetWidth = static_cast<int>(targetHeight * aspectRatio);
    }
    else {
        targetHeight = static_cast<int>(targetWidth / aspectRatio);
    }

    int targetX = (clientRect.right - clientRect.left - targetWidth) / 2;
    int targetY = (clientRect.bottom - clientRect.top - targetHeight) / 2;

    graphics.DrawImage(&image, targetX, targetY, targetWidth, targetHeight);
}
