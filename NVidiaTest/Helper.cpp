#include "Helper.h"

BOOL Helper::MonitorInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    MONITORINFOEX iMonitor;
    iMonitor.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &iMonitor);

    std::vector<ScreenBoundaries>* screensBoundaries = reinterpret_cast<std::vector<ScreenBoundaries>*>(dwData);
    ScreenBoundaries newScreen;
    newScreen.left = iMonitor.rcMonitor.left;
    newScreen.right = iMonitor.rcMonitor.right;
    newScreen.bottom = iMonitor.rcMonitor.bottom;
    newScreen.top = iMonitor.rcMonitor.top;

    screensBoundaries->push_back(newScreen);

    return true;
}

COLORREF Helper::getRandomColor(std::vector<COLORREF> v)
{
    COLORREF result = 0;

    int max = v.size();
    int randomIndex = std::rand() % max;

    result = v[randomIndex];

    return result;
}

std::vector<COLORREF> Helper::GetCommonColorsFromScreenVerticalHorizontal(int pixelOffset)
{
    std::vector<COLORREF> result;

    HDC topWindow = GetWindowDC(NULL);

    std::vector<ScreenBoundaries> screensBoundaries;
    EnumDisplayMonitors(topWindow, NULL, MonitorInfoEnumProc, reinterpret_cast<LPARAM>(&screensBoundaries));

    std::size_t scrSize = screensBoundaries.size();
    if (scrSize > 0)
    {
        int right = screensBoundaries[0].right;
        int bottom = screensBoundaries[0].bottom;

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

std::vector<COLORREF> Helper::GetCommonColorsFromScreenX(std::vector<ScreenBoundaries> screensBoundaries, HDC topWindow)
{
    //TODO: Another way to possibly make this faster is by taking a snapshot of the current
    //      frame on the monitor, resizing the image and getting the RGB values from that.
    std::vector<COLORREF> result;

    std::size_t scrSize = screensBoundaries.size();
    if (scrSize > 0)
    {
        const int exponent = 2;
        int exponentResult = 2;

        int right = screensBoundaries[0].right;
        int bottom = screensBoundaries[0].bottom;

        COLORREF pixel;

        int x = exponentResult;
        int y = exponentResult;

        //Creates a diagonal line from top left to bottom right.
        while (x < right && y < bottom)
        {
            pixel = GetPixel(topWindow, x, y);
            result.push_back(pixel);

            exponentResult = exponentResult * exponent;

            x = exponentResult;
            y = exponentResult;
        }

        exponentResult = 2;

        x = right - exponentResult;
        y = 0 + exponentResult;

        //Creates a diagonal line from top right to bottom left.
        while (x > 0 && y < bottom)
        {
            pixel = GetPixel(topWindow, x, y);
            result.push_back(pixel);

            x = x - exponentResult;
            y = y + exponentResult;

            exponentResult = exponentResult * exponent;
        }
    }

    return result;
}
