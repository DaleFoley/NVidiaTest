#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <WinUser.h>
#include <wingdi.h>
#include <vector>
#include <highlevelmonitorconfigurationapi.h>
#include <lowlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>

#include "../../nvapi/nvapi.h"

#include "../../ChromaSDK/inc/RzChromaSDKDefines.h"
#include "../../ChromaSDK/inc/RzChromaSDKTypes.h"
#include "../../ChromaSDK/inc/RzErrors.h"

#ifdef _WIN64
#define CHROMASDKDLL        _T("RzChromaSDK64.dll")
#else
#define CHROMASDKDLL        _T("RzChromaSDK.dll")
#endif

typedef RZRESULT(*INIT)(void);
typedef RZRESULT(*UNINIT)(void);
typedef RZRESULT(*CREATEEFFECT)(RZDEVICEID DeviceId, ChromaSDK::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID* pEffectId);
typedef RZRESULT(*CREATEKEYBOARDEFFECT)(ChromaSDK::Keyboard::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID* pEffectId);
typedef RZRESULT(*CREATEHEADSETEFFECT)(ChromaSDK::Headset::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID* pEffectId);
typedef RZRESULT(*CREATEMOUSEPADEFFECT)(ChromaSDK::Mousepad::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID* pEffectId);
typedef RZRESULT(*CREATEMOUSEEFFECT)(ChromaSDK::Mouse::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID* pEffectId);
typedef RZRESULT(*CREATEKEYPADEFFECT)(ChromaSDK::Keypad::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID* pEffectId);
typedef RZRESULT(*CREATECHROMALINKEFFECT)(ChromaSDK::ChromaLink::EFFECT_TYPE Effect, PRZPARAM pParam, RZEFFECTID* pEffectId);
typedef RZRESULT(*SETEFFECT)(RZEFFECTID EffectId);
typedef RZRESULT(*DELETEEFFECT)(RZEFFECTID EffectId);
typedef RZRESULT(*REGISTEREVENTNOTIFICATION)(HWND hWnd);
typedef RZRESULT(*UNREGISTEREVENTNOTIFICATION)(void);
typedef RZRESULT(*QUERYDEVICE)(RZDEVICEID DeviceId, ChromaSDK::DEVICE_INFO_TYPE& DeviceInfo);

INIT Init = NULL;
UNINIT UnInit = NULL;
CREATEEFFECT CreateEffect = NULL;
CREATEKEYBOARDEFFECT CreateKeyboardEffect = NULL;
CREATEMOUSEEFFECT CreateMouseEffect = NULL;
CREATEHEADSETEFFECT CreateHeadsetEffect = NULL;
CREATEMOUSEPADEFFECT CreateMousematEffect = NULL;
CREATEKEYPADEFFECT CreateKeypadEffect = NULL;
CREATECHROMALINKEFFECT CreateChromaLinkEffect = NULL;
SETEFFECT SetEffect = NULL;
DELETEEFFECT DeleteEffect = NULL;
QUERYDEVICE QueryDevice = NULL;

struct ScreensBoundaries
{
    int left;
    int top;
    int right;
    int bottom;
};

BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    MONITORINFOEX iMonitor;
    iMonitor.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &iMonitor);

    std::vector<ScreensBoundaries>* s = reinterpret_cast<std::vector<ScreensBoundaries>*>(dwData);
    ScreensBoundaries newScreen;
    newScreen.left = iMonitor.rcMonitor.left;
    newScreen.right = iMonitor.rcMonitor.right;
    newScreen.bottom = iMonitor.rcMonitor.bottom;
    newScreen.top = iMonitor.rcMonitor.top;

    s->push_back(newScreen);

    return true;
}

int main()
{
    HMODULE razerModule;
    razerModule = LoadLibrary(CHROMASDKDLL);

    if (razerModule != NULL)
    {
        INIT Init = (INIT)GetProcAddress(razerModule, "Init");
        if (Init != NULL)
        {
            RZRESULT rzResult = Init();
            if (rzResult == RZRESULT_SUCCESS)
            {
                CreateEffect = (CREATEEFFECT)GetProcAddress(razerModule, "CreateEffect");
                CreateKeyboardEffect = (CREATEKEYBOARDEFFECT)GetProcAddress(razerModule, "CreateKeyboardEffect");
                CreateMouseEffect = (CREATEMOUSEEFFECT)GetProcAddress(razerModule, "CreateMouseEffect");
                CreateMousematEffect = (CREATEMOUSEPADEFFECT)GetProcAddress(razerModule, "CreateMousepadEffect");
                CreateKeypadEffect = (CREATEKEYPADEFFECT)GetProcAddress(razerModule, "CreateKeypadEffect");
                CreateHeadsetEffect = (CREATEHEADSETEFFECT)GetProcAddress(razerModule, "CreateHeadsetEffect");
                CreateChromaLinkEffect = (CREATECHROMALINKEFFECT)GetProcAddress(razerModule, "CreateChromaLinkEffect");
                SetEffect = (SETEFFECT)GetProcAddress(razerModule, "SetEffect");
                DeleteEffect = (DELETEEFFECT)GetProcAddress(razerModule, "DeleteEffect");
            }
        }
    }

    HDC topWindow = GetWindowDC(NULL);

    tagMONITORINFO monitorInfo;
    tagPOINT currentCursorCoords;
    tagRECT topWindowRect;

    std::vector<ScreensBoundaries> scr;

    EnumDisplayMonitors(topWindow, NULL, MyInfoEnumProc, reinterpret_cast<LPARAM>(&scr));

    while (true)
    {
        const int pixelOffset = 100;

        std::size_t scrSize = scr.size();
        std::vector<COLORREF> colors;
        if (scrSize > 0)
        {
            int right = scr[0].right;
            int bottom = scr[0].bottom;

            COLORREF pixel;

            for (int y = 0; y < bottom; y += pixelOffset)
            {
                for (int x = 0; x < right; x += pixelOffset)
                {
                    pixel = GetPixel(topWindow, x, y);
                    colors.push_back(pixel);
                }
            }
        }

        int averageRGB = 0;
        std::size_t colorsSize = colors.size();
        if (colorsSize > 0)
        {
            for (int i = 0; i != colorsSize; ++i)
            {
                averageRGB += colors.at(i);
            }

            averageRGB = averageRGB / colorsSize;

            DWORD red = GetRValue(averageRGB);
            DWORD green = GetGValue(averageRGB);
            DWORD blue = GetBValue(averageRGB);

            std::cout << "red: " << red << " green: " << green << " blue: " << blue << std::endl;

            RZEFFECTID Frame1;

            ChromaSDK::Keyboard::STATIC_EFFECT_TYPE staticColor = {};
            staticColor.Color = averageRGB;

            RZRESULT result;
            result = CreateKeyboardEffect(ChromaSDK::Keyboard::CHROMA_STATIC, &staticColor, &Frame1);
            if (result != 0)
            {
                std::cout << "Ran into error applying custom keyboard effect. Error Code: " << result << std::endl;
            }

            result = SetEffect(Frame1);
            if (result != 0)
            {
                std::cout << "Ran into error applying custom keyboard effect. Error Code: " << result << std::endl;
            }
        }
    }
}
