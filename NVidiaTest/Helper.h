#pragma once

#include <windows.h>
#include <vector>
#include <algorithm>
class Helper
{
public:
    struct ScreenBoundaries
    {
        int left;
        int top;
        int right;
        int bottom;
    };

/**
    Intended to be used with EnumDisplayMonitors. Gets the monitor screen dimensions and sets appropriate class members to dwData.
    @return BOOL on success.
*/
    static BOOL CALLBACK MonitorInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

    static COLORREF getRandomColor(std::vector<COLORREF> v);

/**
    Gets pixels across the primary monitor display scanning both vertically and horizontally.
    @param pixelOffset the offset number of pixels to apply. The higher the number, the faster the execution time.
    The lower the number, the more accurate it is, but slower execution time.
    @return a vector of COLORREF of all identified pixels.
*/
    static std::vector<COLORREF> GetCommonColorsFromScreenVerticalHorizontal(int pixelOffset = 500);

/**
    Gets pixels across the primary monitor display in an 'X' pattern.
    @return a vector of COLORREF of all identified pixels.
*/
    static std::vector<COLORREF> GetCommonColorsFromScreenX(std::vector<ScreenBoundaries> screensBoundaries, HDC topWindow);

//Template functions

/**
    Gets a unique set of values from within collection.
    @return a vector of T of unique items.
*/
    template <typename T>
    static std::vector<T> getUniqueCollection(std::vector<T> collection)
    {
        std::sort(collection.begin(), collection.end());

        int numbersSize = collection.size();
        std::vector<T> collectionsUnique = collection;

        //As to why we need to prefix typename here, see:
        //https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords
        typename std::vector<T>::iterator it;
        it = std::unique(collectionsUnique.begin(), collectionsUnique.end());

        collectionsUnique.resize(std::distance(collectionsUnique.begin(), it));
        std::sort(collectionsUnique.begin(), collectionsUnique.end());

        return collectionsUnique;
    }

/**
    Gets the most commonly occurring item from collection. This does not handle bimodal, multimodal, it will simply return the first modal value.
    @return returns T where T is the most occuring item in collection.
*/
    template <typename T>
    static T getModeFromCollection(std::vector<T> collection)
    {
        //This whole function feels haphazard, any cleaner way to do this?
        std::sort(collection.begin(), collection.end());

        int collectionSize = collection.size();

        std::vector<T> collectionsUnique = getUniqueCollection(collection);
        int collectionUniqueSize = collectionsUnique.size();

        //Only 1 or 0 items in the collection, nothing to do..
        if (collectionUniqueSize == 0) return 0;
        if (collectionUniqueSize == 1) return collectionsUnique[0];

        std::vector<int> collectionsCount;

        int i = 0;
        for (int x = 0; x != collectionUniqueSize; ++x)
        {
            int numbersSum = 0;
            for (; i != collectionSize; ++i)
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
};

