// Main.cpp : Defines the entry point for the application.
#include "Demo/FPSDemo.h"
#include "Demo/PBRDemo.h"
#include "Demo/BoxDemo.h"

#include <crtdbg.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    OutputDebugString("Hello Ruby Engine\n");

    //BoxDemo* app = new BoxDemo(hInstance, 800, 600, "Ruby Engine: Box Demo", false);
    FPSDemo* app = new FPSDemo(hInstance, 1280, 720, "Ruby Engine: FPS Demo", false);
    //PBRDemo* app = new PBRDemo(hInstance, 1280, 720, "Ruby Engine: PBR Demo", false);


    if (!app->Init())
        return 0;

    app->Run();

    SAFE_DELETE(app);

    return 1;
}


