#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <WinUser.h>
#include <wingdi.h>

#include "../../nvapi/nvapi.h"
#include <vector>

NvAPI_Status AllocateAndGetDisplayConfig(NvU32* pathInfoCount, NV_DISPLAYCONFIG_PATH_INFO** pPathInfo);
NV_COLOR_DATA GetFirstMonitorColorData();
void ShowCurrentDisplayConfig(void);
NvAPI_Status SetMode(void);

struct Screens
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

    std::vector<Screens>* s = reinterpret_cast<std::vector<Screens>*>(dwData);
    Screens newScreen;
    newScreen.left = iMonitor.rcMonitor.left;
    newScreen.right = iMonitor.rcMonitor.right;
    newScreen.bottom = iMonitor.rcMonitor.bottom;
    newScreen.top = iMonitor.rcMonitor.top;

    s->push_back(newScreen);

    return true;
}

int main()
{
    //NvAPI_Status ret = NvAPI_Status::NVAPI_OK;
    //ret = NvAPI_Initialize();

    //if (ret != NVAPI_OK)
    //{
    //    printf("NvAPI_Initialize() failed = 0x%x", ret);
    //    return 1; // Initialization failed
    //}

    //NvU32 displayIds[NVAPI_MAX_DISPLAYS] = { 0 };
    //NvU32 noDisplays = 0;

    //NV_COLOR_DATA colorData = { 0 };
    //colorData.version = NV_COLOR_DATA_VER;
    //colorData.size = sizeof(NV_COLOR_DATA);
    //colorData.cmd = NV_COLOR_CMD_GET;

    //NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS] = { 0 };
    //NvU32 pGpuCount;
    //NvDisplayHandle hNvDisplay = { 0 };

    //_DISPLAY_DEVICEA displayTest;
    //displayTest.cb = sizeof(DISPLAY_DEVICE);
    //
    //PDISPLAY_DEVICEA testDev = &displayTest;

    //bool isEnumDisplayDevicesSuccessfull = EnumDisplayDevicesA(nullptr, 0, testDev, EDD_GET_DEVICE_INTERFACE_NAME);
    //std::string displayName = testDev->DeviceName;
    //const char* displayNameChar = displayName.c_str();

    //ret = NvAPI_GetAssociatedNvidiaDisplayHandle(displayNameChar, &hNvDisplay);
    //ret = NvAPI_GetPhysicalGPUsFromDisplay(hNvDisplay, nvGPUHandle, &pGpuCount);

    //NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS illumControl = { 0 };
    //illumControl.version = NV_GPU_CLIENT_ILLUM_ZONE_INFO_PARAMS_VER;

    //if (nvGPUHandle[0] != 0)
    //{
    //    ret = NvAPI_GPU_ClientIllumZonesGetInfo(nvGPUHandle[0], &illumControl);
    //}
    //ret = NvAPI_Disp_ColorControl(displayIds[0], &colorData);

    //TODO: Locking the pc breaks this.
    while (true)
    {
        HDC topWindow = GetWindowDC(NULL);
        tagMONITORINFO monitorInfo;
        tagPOINT currentCursorCoords;
        tagRECT topWindowRect;

        std::vector<Screens> scr;
        //bool hasCursorCoords = GetCursorPos(&currentCursorCoords);

        EnumDisplayMonitors(topWindow, NULL, MyInfoEnumProc, reinterpret_cast<LPARAM>(&scr));

        std::size_t scrSize = scr.size();
        std::vector<COLORREF> colors;
        if (scrSize > 0)
        {
            int right = scr[0].right;
            int bottom = scr[0].bottom;

            const int totalPixels = right * bottom;

            COLORREF pixel;

            for (int y = 0; y < bottom; y += 150)
            {
                for (int x = 0; x < right; x += 150)
                {
                    pixel = GetPixel(topWindow, x, y);
                    colors.push_back(pixel);
                }
            }
        }

        int averageRGB = 0;
        for (int i = 0; i != colors.size(); ++i)
        {
            averageRGB += colors.at(i);
        }

        averageRGB = averageRGB / colors.size();
        DWORD red = GetRValue(averageRGB);
        DWORD green = GetGValue(averageRGB);
        DWORD blue = GetBValue(averageRGB);

        std::cout << "red: " << red << " green: " << green << " blue: " << blue << std::endl;
        //if (isRectCalculated)
        //{
        //    //COLORREF pixelColor = GetPixel(topWindow, currentCursorCoords.x, currentCursorCoords.y);
        //    COLORREF pixelColor = GetPixel(topWindow, 1, 1);

        //    DWORD red = GetRValue(pixelColor);
        //    DWORD green = GetGValue(pixelColor);
        //    DWORD blue = GetBValue(pixelColor);

        //    std::cout << "red: " << red << " green: " << green << " blue: " << blue << std::endl;
        //}

        //Sleep(1000);
    }
}

NvAPI_Status AllocateAndGetDisplayConfig(NvU32* pathInfoCount, NV_DISPLAYCONFIG_PATH_INFO** pPathInfo)
{
    NvAPI_Status ret;

    // Retrieve the display path information
    NvU32 pathCount = 0;
    NV_DISPLAYCONFIG_PATH_INFO* pathInfo = NULL;

    ret = NvAPI_DISP_GetDisplayConfig(&pathCount, NULL);
    if (ret != NVAPI_OK)    return ret;

    pathInfo = (NV_DISPLAYCONFIG_PATH_INFO*)malloc(pathCount * sizeof(NV_DISPLAYCONFIG_PATH_INFO));
    if (!pathInfo)
    {
        return NVAPI_OUT_OF_MEMORY;
    }

    memset(pathInfo, 0, pathCount * sizeof(NV_DISPLAYCONFIG_PATH_INFO));
    for (NvU32 i = 0; i < pathCount; i++)
    {
        pathInfo[i].version = NV_DISPLAYCONFIG_PATH_INFO_VER;
    }

    // Retrieve the targetInfo counts
    ret = NvAPI_DISP_GetDisplayConfig(&pathCount, pathInfo);
    if (ret != NVAPI_OK)
    {
        return ret;
    }

    for (NvU32 i = 0; i < pathCount; i++)
    {
        // Allocate the source mode info

        if (pathInfo[i].version == NV_DISPLAYCONFIG_PATH_INFO_VER1 || pathInfo[i].version == NV_DISPLAYCONFIG_PATH_INFO_VER2)
        {
            pathInfo[i].sourceModeInfo = (NV_DISPLAYCONFIG_SOURCE_MODE_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
        }
        else
        {

#ifdef NV_DISPLAYCONFIG_PATH_INFO_VER3
            pathInfo[i].sourceModeInfo = (NV_DISPLAYCONFIG_SOURCE_MODE_INFO*)malloc(pathInfo[i].sourceModeInfoCount * sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
#endif

        }
        if (pathInfo[i].sourceModeInfo == NULL)
        {
            return NVAPI_OUT_OF_MEMORY;
        }
        memset(pathInfo[i].sourceModeInfo, 0, sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));

        // Allocate the target array
        pathInfo[i].targetInfo = (NV_DISPLAYCONFIG_PATH_TARGET_INFO*)malloc(pathInfo[i].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
        if (pathInfo[i].targetInfo == NULL)
        {
            return NVAPI_OUT_OF_MEMORY;
        }
        // Allocate the target details
        memset(pathInfo[i].targetInfo, 0, pathInfo[i].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
        for (NvU32 j = 0; j < pathInfo[i].targetInfoCount; j++)
        {
            pathInfo[i].targetInfo[j].details = (NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
            memset(pathInfo[i].targetInfo[j].details, 0, sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
            pathInfo[i].targetInfo[j].details->version = NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_VER;
        }
    }

    // Retrieve the full path info
    ret = NvAPI_DISP_GetDisplayConfig(&pathCount, pathInfo);
    if (ret != NVAPI_OK)
    {
        return ret;
    }

    *pathInfoCount = pathCount;
    *pPathInfo = pathInfo;
    return NVAPI_OK;
}

NV_COLOR_DATA GetFirstMonitorColorData()
{
    NV_COLOR_DATA result = { 0 };
    result.version = NV_COLOR_DATA_VER;
    result.size = sizeof(NV_COLOR_DATA);
    result.cmd = NV_COLOR_CMD_GET;

    NvU32 displayIds[NVAPI_MAX_DISPLAYS] = { 0 };

    NV_DISPLAYCONFIG_PATH_INFO *pathInfo = NULL;
    NvU32 pathCount = 0;

    NvAPI_Status ret = NvAPI_Status::NVAPI_OK;

    ret = AllocateAndGetDisplayConfig(&pathCount, &pathInfo);
    if (ret != NVAPI_OK)
    {
        printf("AllocateAndGetDisplayConfig failed!\n");
        getchar();
        exit(1);
    }

    //TODO: Get Display ID.
    ret = NvAPI_Disp_ColorControl(displayIds[0], &result);

    return result;
}