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

template <typename T>
static std::vector<T> getUniqueCollection(std::vector<T> collection)
{
    std::sort(collection.begin(), collection.end());

    int numbersSize = collection.size();

    std::vector<T> collectionsUnique = collection;
    std::vector<int> collectionsCount;

    //As to why we need to prefix typename here, see:
    //https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords
    typename std::vector<T>::iterator it;
    it = std::unique(collectionsUnique.begin(), collectionsUnique.end());

    collectionsUnique.resize(std::distance(collectionsUnique.begin(), it));
    std::sort(collectionsUnique.begin(), collectionsUnique.end());

    return collectionsUnique;
}

template <typename T>
static T getModeFromCollection(std::vector<T> collection)
{
    //This whole function feels haphazard, any cleaner way to do this?
    std::sort(collection.begin(), collection.end());

    int numbersSize = collection.size();

    std::vector<T> collectionsUnique = collection;
    std::vector<int> collectionsCount;

    //As to why we need to prefix typename here, see:
    //https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords
    typename std::vector<T>::iterator it;
    it = std::unique(collectionsUnique.begin(), collectionsUnique.end());

    collectionsUnique.resize(std::distance(collectionsUnique.begin(), it));
    std::sort(collectionsUnique.begin(), collectionsUnique.end());

    int numbersUniqueSize = collectionsUnique.size();

    //Only 1 or 0 items in the collection, nothing to do..
    if (numbersUniqueSize == 0) return 0;
    if (numbersUniqueSize == 1) return collectionsUnique[0];

    int i = 0;
    for (int x = 0; x != numbersUniqueSize; ++x)
    {
        int numbersSum = 0;
        for (; i != numbersSize; ++i)
        {
            if (collection[i] != collectionsUnique[x]) break;
            ++numbersSum;
        }

        collectionsCount.push_back(numbersSum);
    }

    int indexWithLargestNumber = 0;
    int numberToCompare = 0;
    int numberCountSize = collectionsCount.size();
    for (int a = 0; a != numberCountSize; ++a)
    {
        if (collectionsCount[a] > numberToCompare)
        {
            numberToCompare = collectionsCount[a];
            indexWithLargestNumber = a;
        }
    }

    return collectionsUnique[indexWithLargestNumber];
}

std::vector<COLORREF> GetCommonColorsFromScreen()
{
    std::vector<COLORREF> result;

    HDC topWindow = GetWindowDC(NULL);
    
    std::vector<ScreensBoundaries> scr;
    EnumDisplayMonitors(topWindow, NULL, MyInfoEnumProc, reinterpret_cast<LPARAM>(&scr));

    const int pixelOffset = 400;

    std::size_t scrSize = scr.size();
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
                result.push_back(pixel);
            }
        }
    }

    return result;
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

    RZRESULT result;
    RZEFFECTID Frame1;

    ChromaSDK::Keyboard::BREATHING_EFFECT_TYPE breathingColor = {};
    ChromaSDK::Keyboard::STATIC_EFFECT_TYPE staticColor = {};

    while (true)
    {
        std::vector<COLORREF> commonColors = GetCommonColorsFromScreen();
        std::size_t commonColorsSize = commonColors.size();

        ChromaSDK::Keyboard::v2::CUSTOM_EFFECT_TYPE Effect = {};
        int colorIndex = 0;
        for (int i = 0; i != 8; ++i)
        {
            for (int x = 0; x != 24; ++x)
            {
                if (colorIndex == commonColorsSize) colorIndex = 0;

                Effect.Color[i][x] = commonColors[colorIndex];
                ++colorIndex;
            }
        }

        result = CreateKeyboardEffect(ChromaSDK::Keyboard::CHROMA_CUSTOM2, &Effect, &Frame1);
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

    while (true)
    {
        const int pixelOffset = 600;

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

        COLORREF commonOccuringColor = getModeFromCollection(colors);
        std::vector<COLORREF> uniqueColors = getUniqueCollection(colors);

        int averageRGB = 0;
        std::size_t colorsSize = colors.size();
        if (colorsSize > 0)
        {
            DWORD red = GetRValue(commonOccuringColor);
            DWORD green = GetGValue(commonOccuringColor);
            DWORD blue = GetBValue(commonOccuringColor);

            std::cout << "red: " << red << " green: " << green << " blue: " << blue << std::endl;

            staticColor.Color = commonOccuringColor;

            result = CreateKeyboardEffect(ChromaSDK::Keyboard::CHROMA_STATIC, &staticColor, &Frame1);
            //result = CreateKeyboardEffect(ChromaSDK::Keyboard::CHROMA_BREATHING, &breathingColor, &Frame1);
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
