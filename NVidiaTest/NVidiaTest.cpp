#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <WinUser.h>
#include <wingdi.h>
#include <vector>
#include <algorithm>
#include <highlevelmonitorconfigurationapi.h>
#include <lowlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>

#include "../../nvapi/nvapi.h"

#include "../../ChromaSDK/inc/RzChromaSDKDefines.h"
#include "../../ChromaSDK/inc/RzChromaSDKTypes.h"
#include "../../ChromaSDK/inc/RzErrors.h"

#include "Helper.h"

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

//TODO: Color from mouse cursor position.
//TODO: Error handling/logging.
//TODO: Move third party header files into base of this project.
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
            else
            {
                std::cout << "Chroma SDK Init returned: " << rzResult << ". Expected return value of: " << RZRESULT_SUCCESS << std::endl;
                return rzResult;
            }
        }
        else
        {
            std::cout << "Failed to retrieve Init procedure from: " << CHROMASDKDLL << std::endl;
            return 1;
        }
    }
    else
    {
        std::cout << "Failed to load required chroma SDK DLL: " << CHROMASDKDLL << std::endl;
        return 1;
    }

    RZRESULT result;
    RZEFFECTID effectID;

    ChromaSDK::Keyboard::BREATHING_EFFECT_TYPE breathingColor = {};
    ChromaSDK::Keyboard::STATIC_EFFECT_TYPE staticColor = {};
    ChromaSDK::Keyboard::v2::CUSTOM_EFFECT_TYPE customEffect = {};

    constexpr int maxKeyboardKeys = ChromaSDK::Keyboard::v2::MAX_COLUMN * ChromaSDK::Keyboard::v2::MAX_ROW;

    HDC topWindow = GetWindowDC(NULL);

    std::vector<Helper::ScreenBoundaries> screensBoundaries;
    EnumDisplayMonitors(topWindow, NULL, Helper::MonitorInfoEnumProc, reinterpret_cast<LPARAM>(&screensBoundaries));

    //This version gets a collection of colors discovered on the screen and applies a random color to a random key on the keyboard.
    //This one is my favourite so far.
    while (true)
    {
        std::vector<COLORREF> colors = Helper::GetCommonColorsFromScreenX(screensBoundaries, topWindow);

        std::size_t colorsSize = colors.size();
        if (colorsSize > 0)
        {
            DWORD red = 0;
            DWORD green = 0;
            DWORD blue = 0;

            int col = 0;
            int row = 0;
            for (int i = 0; i != maxKeyboardKeys; ++i)
            {
                COLORREF randomColor = Helper::getRandomColor(colors);

                red = GetRValue(randomColor);
                green = GetGValue(randomColor);
                blue = GetBValue(randomColor);

                while (customEffect.Color[row][col] != 0)
                {
                    row = std::rand() % ChromaSDK::Keyboard::v2::MAX_ROW;
                    col = std::rand() % ChromaSDK::Keyboard::v2::MAX_COLUMN;
                }

                customEffect.Color[row][col] = randomColor;

                std::cout << "red: " << red << " green: " << green << " blue: " << blue << std::endl;
            }

            result = CreateKeyboardEffect(ChromaSDK::Keyboard::CHROMA_CUSTOM2, &customEffect, &effectID);
            if (result != 0)
            {
                std::cout << "Ran into error applying custom keyboard effect. Error Code: " << result << std::endl;
            }

            result = SetEffect(effectID);
            if (result != 0)
            {
                std::cout << "Ran into error applying custom keyboard effect. Error Code: " << result << std::endl;
            }

            customEffect = {};
        }
        else
        {
            Sleep(1000);
        }
    }

    //This version collates the most commonly occuring colors from the screen and disperses those colours across our keys.
    //while (true)
    //{
    //    std::vector<COLORREF> commonColors = Helper::GetCommonColorsFromScreen();
    //    std::size_t commonColorsSize = commonColors.size();

    //    //size will be 0 if the user locks their screen.
    //    if (commonColorsSize == 0)
    //    {
    //        Sleep(1000);
    //        continue;
    //    }

    //    int colorIndex = 0;
    //    for (int i = 0; i != ChromaSDK::Keyboard::v2::MAX_ROW; ++i)
    //    {
    //        for (int x = 0; x != ChromaSDK::Keyboard::v2::MAX_COLUMN; ++x)
    //        {
    //            if (colorIndex == commonColorsSize) colorIndex = 0;

    //            Effect.Color[i][x] = commonColors[colorIndex];
    //            ++colorIndex;
    //        }
    //    }

    //    result = CreateKeyboardEffect(ChromaSDK::Keyboard::CHROMA_CUSTOM2, &Effect, &effectID);
    //    if (result != 0)
    //    {
    //        std::cout << "Ran into error applying custom keyboard effect. Error Code: " << result << std::endl;
    //    }

    //    result = SetEffect(effectID);
    //    if (result != 0)
    //    {
    //        std::cout << "Ran into error applying custom keyboard effect. Error Code: " << result << std::endl;
    //    }
    //}

    //This version gets a collection of colors discovered on the screen and applies randomly a color as a static color to the keyboard.
    //The pixels are scanned both horizontally and vertically across the monitor.
    //while (true)
    //{
    //    const int pixelOffset = 400;

    //    std::vector<COLORREF> colors = Helper::GetCommonColorsFromScreenVerticalHorizontal(pixelOffset);

    //    std::size_t colorsSize = colors.size();
    //    if (colorsSize > 0)
    //    {
    //        COLORREF randomColor = Helper::getRandomColor(colors);

    //        DWORD red = GetRValue(randomColor);
    //        DWORD green = GetGValue(randomColor);
    //        DWORD blue = GetBValue(randomColor);

    //        std::cout << "red: " << red << " green: " << green << " blue: " << blue << std::endl;
    //        staticColor.Color = randomColor;

    //        result = CreateKeyboardEffect(ChromaSDK::Keyboard::CHROMA_STATIC, &staticColor, &effectID);
    //        if (result != 0)
    //        {
    //            std::cout << "Ran into error applying custom keyboard effect. Error Code: " << result << std::endl;
    //        }

    //        result = SetEffect(effectID);
    //        if (result != 0)
    //        {
    //            std::cout << "Ran into error applying custom keyboard effect. Error Code: " << result << std::endl;
    //        }
    //    }
    //}

    //This version gets the mode from a collection of colors discovered on the screen and applies that as a static color to the keyboard.
    //The pixels are scanned both horizontally and vertically across the monitor.
    //while (true)
    //{
    //    const int pixelOffset = 400;

    //    std::vector<COLORREF> colors = Helper::GetCommonColorsFromScreenVerticalHorizontal(pixelOffset);

    //    std::size_t colorsSize = colors.size();
    //    if (colorsSize > 0)
    //    {
    //        COLORREF modeColor = Helper::getModeFromCollection(colors);

    //        DWORD red = GetRValue(modeColor);
    //        DWORD green = GetGValue(modeColor);
    //        DWORD blue = GetBValue(modeColor);

    //        std::cout << "red: " << red << " green: " << green << " blue: " << blue << std::endl;
    //        staticColor.Color = modeColor;

    //        result = CreateKeyboardEffect(ChromaSDK::Keyboard::CHROMA_STATIC, &staticColor, &effectID);
    //        if (result != 0)
    //        {
    //            std::cout << "Ran into error applying custom keyboard effect. Error Code: " << result << std::endl;
    //        }

    //        result = SetEffect(effectID);
    //        if (result != 0)
    //        {
    //            std::cout << "Ran into error applying custom keyboard effect. Error Code: " << result << std::endl;
    //        }
    //    }
    //}
}
