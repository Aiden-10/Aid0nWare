#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <cstddef>
#include <vector>
#include <string>
#include <gdiplus.h>
#include <codecvt> 
#include"functions.h"

#pragma comment(lib, "gdiplus.lib")

uintptr_t moduleBase;
uintptr_t engine;
HANDLE TargetProcess;
HDC hdc;
HWND hCheckBox; // Declare a global variable to store the handle of the checkbox
HWND hwnd;
HWND hComboBox;
HDC memDC = nullptr;
HBITMAP memBitmap = nullptr;
HBITMAP oldBitmap = nullptr; // Move this line here


extern Gdiplus::Image* dust;
extern Gdiplus::Image* mirage;
extern Gdiplus::Image* inferno;


int windowWidth = 0;                  // Store the window width
int windowHeight = 0;

bool TriggerOn = false;



const int   screenSize_X = 1024;
const int   screenSize_Y = 1024;

int radius = 5;



// !!!! IMPORTANT
//float scaleX = static_cast<float>(windowWidth) / dust2Width;
//float scaleY = static_cast<float>(windowHeight) / dust2Height;


// !!!!!!!!!!!  DO NOT CHANGE THIS
int mapWidth = 0;
int mapHeight = 0;
int offsetX = 0;
int offsetY = 0;




const float initialZoomLevel = 1.0f; // Initial zoom level
float zoomLevel = initialZoomLevel;

const int fontSize = 2; // Set the desired font size
// Declare the image variable

HFONT hFont = CreateFont(
    -MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72), // Calculate the font height based on the desired size
    0,
    0,
    0,
    FW_NORMAL,
    FALSE,
    FALSE,
    FALSE,
    DEFAULT_CHARSET,
    OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS,
    DEFAULT_QUALITY,
    DEFAULT_PITCH | FF_DONTCARE,
    "Arial" // Specify the desired font face
);

struct Vector3 {
    float x, y, z;

    Vector3 operator+(const Vector3& other) const {
        return { x + other.x, y + other.y, z + other.z };
    }
};

static std::vector<Vector3> circlePositions; // Store the circle positions
static std::vector<COLORREF> circleColors;
Vector3 localEyePosition;

constexpr ::std::ptrdiff_t m_iItemDefinitionIndex = 0x2FBA;
constexpr ::std::ptrdiff_t dwEntityList = 0x4DFFF7C;
constexpr ::std::ptrdiff_t m_iTeamNum = 0xF4;
constexpr ::std::ptrdiff_t m_iHealth = 0x100;
constexpr ::std::ptrdiff_t m_vecOrigin = 0x138;
constexpr ::std::ptrdiff_t m_vecViewOffset = 0x108;
constexpr ::std::ptrdiff_t dwLocalPlayer = 0xDEA98C;
constexpr ::std::ptrdiff_t m_dwBoneMatrix = 0x26A8;
constexpr ::std::ptrdiff_t m_lifeState = 0x25F;
constexpr ::std::ptrdiff_t dwClientState = 0x59F19C;
constexpr ::std::ptrdiff_t dwClientState_ViewAngles = 0x4D90;
constexpr ::std::ptrdiff_t dwClientState_GetLocalPlayer = 0x180;
constexpr ::std::ptrdiff_t m_bDormant = 0xED;
constexpr ::std::ptrdiff_t m_bIsDefusing = 0x997C;
constexpr ::std::ptrdiff_t m_bIsScoped = 0x9974;
constexpr ::std::ptrdiff_t m_angEyeAnglesX = 0x117D0;
constexpr ::std::ptrdiff_t m_angEyeAnglesY = 0x117D4;
constexpr ::std::ptrdiff_t m_ArmorValue = 0x117CC;
constexpr ::std::ptrdiff_t m_iCrosshairId = 0x11838;


template <typename T>
T RPM(SIZE_T address) {
    T buffer;
    ReadProcessMemory(TargetProcess, (LPCVOID)address, &buffer, sizeof(T), NULL);
    return buffer;
}