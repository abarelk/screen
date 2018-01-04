/****
*
*   Screen
*
*/

#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <string>

#define LOG(fmt, ...)    LogMsg(fmt ## L"\n", __VA_ARGS__)

void LogMsg (wchar_t * fmt, ...) {
    va_list va;
    va_start(va, fmt);
    std::vwprintf(fmt, va);
    va_end(va);
}

bool SaveBitmap (const wchar_t * filePath, const BITMAPINFOHEADER& bi, const char * bitmap, size_t bytes) {
    BITMAPFILEHEADER   bmfHeader;    
    bool ret = false;

    // A file is created, this is where we will save the screen capture.
    HANDLE hFile = CreateFile(filePath,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);   
    
    if (hFile == INVALID_HANDLE_VALUE)
        return ret;

    // Add the size of the headers to the size of the bitmap to get the total file size
    DWORD dwSizeofDIB = bytes + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
 
    //Offset to where the actual bitmap bits start.
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER); 
    
    //Size of the file
    bmfHeader.bfSize = dwSizeofDIB; 
    
    //bfType must always be BM for Bitmaps
    bmfHeader.bfType = 0x4D42; //BM   
 
    DWORD dwBytesWritten = 0;
    if (   !WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL)
        || !WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL)
        || !WriteFile(hFile, (LPSTR)bitmap, bytes, &dwBytesWritten, NULL)) {
        ret = false;
        goto done;
    }

    ret = true;
done:
    //Close the handle for the file that was created
    CloseHandle(hFile);  

    return ret;
}

bool CaptureImage (HWND hWnd, BITMAPINFOHEADER * bi, char ** bitmap, size_t * bytes) {

    HDC hdcScreen;
    HDC hdcWindow;
    HDC hdcMemDC = NULL;
    HBITMAP hbmScreen = NULL;
    BITMAP bmpScreen;
    bool ret = false;

    hdcScreen = GetDC(NULL);
    hdcWindow = GetDC(hWnd);
    
    hdcMemDC = CreateCompatibleDC(hdcWindow);
    if (!hdcMemDC) {
        LOG(L"Failed to create compatible DC");
        goto done;
    }

    RECT  rcClient;
    GetClientRect(hWnd, &rcClient);

    hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
    if (!hbmScreen) {
        LOG(L"Failed to create compatible bitmap");
        goto done;
    }
    SelectObject(hdcMemDC, hbmScreen);

    if (!BitBlt(hdcMemDC, 0, 0, 
        rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
        hdcWindow,
        0, 0,
        SRCCOPY)) {
        LOG(L"Failed to copy bitmap");
        goto done;
    }

    GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

    bi->biSize = sizeof(BITMAPINFOHEADER);    
    bi->biWidth = bmpScreen.bmWidth;    
    bi->biHeight = bmpScreen.bmHeight;  
    bi->biPlanes = 1;    
    bi->biBitCount = 32;    
    bi->biCompression = BI_RGB;    
    bi->biSizeImage = 0;  
    bi->biXPelsPerMeter = 0;    
    bi->biYPelsPerMeter = 0;    
    bi->biClrUsed = 0;    
    bi->biClrImportant = 0;

    *bytes = ((bmpScreen.bmWidth * bi->biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

    *bitmap = (char *)HeapAlloc(GetProcessHeap(), 0, *bytes);

    // Gets the "bits" from the bitmap and copies them into a buffer 
    // which is pointed to by lpbitmap.
    GetDIBits(hdcWindow, hbmScreen, 0,
        (UINT)bmpScreen.bmHeight,
        *bitmap,
        (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    ret = true;

done:
    DeleteObject(hbmScreen);
    DeleteObject(hdcMemDC);
    ReleaseDC(NULL, hdcScreen);
    ReleaseDC(hWnd, hdcWindow);
    return ret;
}

int main (int argc, char ** argv) {

    std::wstring procName;
    std::wstring filepath;

    std::cout << "Enter process name to capture (default: nox): ";
    std::getline(std::wcin, procName);
    if (procName.size() == 0)
        procName = L"녹스 플레이어";

//    HWND hWnd = ::FindWindow(0, procName.c_str());
    HWND hWnd = GetDesktopWindow();
    if (!hWnd) {
        LOG(L"Cannot find window named [%s]", procName.c_str());
        goto done;
    }

    std::cout << "Enter filepath to save (default: capture): ";
    std::getline(std::wcin, filepath);
    if (filepath.size() == 0)
        filepath = L"capture";

    BITMAPINFOHEADER bi;
    char * bitmap;
    size_t bytes;

    for (int i = 0;; ++i) {
        std::cout << "Capture now? (quit: q) ";
        std::wstring enter;
        std::getline(std::wcin, enter);
        if (enter == L"q")
            break;

        if (!CaptureImage(hWnd, &bi, &bitmap, &bytes)) {
            LOG(L"Capture failed");
            goto done;
        }

        std::wstring fullpath = filepath;
        (i == 0 ? fullpath : fullpath.append(L"(").append(std::to_wstring(i)).append(L")"));
        fullpath.append(L".bmp");

        if (!SaveBitmap(fullpath.c_str(), bi, bitmap, bytes)) {
            LOG(L"Failed to save");
            goto done;
        }
     
        LOG(L"Successfully saved at [%s]", filepath.c_str());
    }

done:
    return 0;
}