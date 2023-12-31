#include "RubyApp.h"

#include <WindowsX.h>

namespace Ruby
{
    static App* gRubyApp;
    LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // Forward hwnd on because we can get messages (e.g., WM_CREATE)
        // before CreateWindow returns, and thus before mhMainWnd is valid.
        return gRubyApp->MsgProc(hwnd, msg, wParam, lParam);
    }

    App::App(HINSTANCE instance, UINT clientWidth, UINT clientHeight, const char* windowCaption, bool enable4xMsaa)
    :   mInstance(instance),
        mClientWidth(clientWidth),
        mClientHeight(clientHeight),
        mWindowCaption(windowCaption),
        mDriverType(D3D_DRIVER_TYPE_HARDWARE),
        mEnable4xMsaa(enable4xMsaa),
        mWindow(0),
        mRunning(false),
        mPause(false),
        mMinimized(false),
        mMaximized(false),
        mResizing(false),
        m4xMsaaQuality(0),
        mDevice(nullptr),
        mImmediateContext(nullptr),
        mSwapChain(nullptr),
        mDepthStencilBuffer(nullptr),
        mRenderTargetView(nullptr),
        mDepthStencilView(nullptr)
    {
        ZeroMemory(&mViewport, sizeof(D3D11_VIEWPORT));
        gRubyApp = this;
    }
    App::~App()
    {
        SAFE_RELEASE(mRenderTargetView);
        SAFE_RELEASE(mDepthStencilView);
        SAFE_RELEASE(mSwapChain);
        SAFE_RELEASE(mDepthStencilBuffer);

        // restore all default settingsqs
        if (mImmediateContext) mImmediateContext->ClearState();

        SAFE_RELEASE(mImmediateContext);
        SAFE_RELEASE(mDevice);
    }

    HINSTANCE App::Instance()
    {
        return mInstance;
    }

    HWND App::Window()
    {
        return mWindow;
    }

    float App::AspectRatio()
    {
        return (float)mClientWidth / (float)mClientHeight;
    }

    void App::FlushEvents()
    {
        MSG msg = {};
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
            case WM_QUIT: {
                mRunning = false;
            } break;

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP: {
                bool wasDown = ((msg.lParam & (1 << 30)) != 0);
                bool isDown = ((msg.lParam & (1 << 31)) == 0);
                if (isDown != wasDown) {
                    DWORD vkCode = (DWORD)msg.wParam;
                    mInput.mCurrent.keys[vkCode].isPress = isDown;
                }
            }break;
            case WM_MOUSEMOVE: {
                mInput.mCurrent.mouseX = (int)GET_X_LPARAM(msg.lParam);
                mInput.mCurrent.mouseY = (int)GET_Y_LPARAM(msg.lParam);
            }break;
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP: {
                mInput.mCurrent.mouseButtons[0].isPress = ((msg.wParam & MK_LBUTTON) != 0);
                mInput.mCurrent.mouseButtons[2].isPress = ((msg.wParam & MK_MBUTTON) != 0);
                mInput.mCurrent.mouseButtons[1].isPress = ((msg.wParam & MK_RBUTTON) != 0);
            }break;
            
            /*
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            {
                OnMouseDown(msg.wParam, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
            } break;
            case WM_LBUTTONUP:
            case WM_MBUTTONUP:
            case WM_RBUTTONUP:
            {
                OnMouseUp(msg.wParam, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
            } break;
            case WM_MOUSEMOVE:
            {
                OnMouseMove(msg.wParam, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
            } break;

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                bool wasDown = ((msg.lParam & (1 << 30)) != 0);
                bool isDown = ((msg.lParam & (1 << 31)) == 0);
                if (isDown != wasDown) OnKeyDown(msg.wParam);

            } break;
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                bool wasDown = ((msg.lParam & (1 << 30)) != 0);
                bool isDown = ((msg.lParam & (1 << 31)) == 0);
                if (isDown != wasDown) OnKeyUp(msg.wParam);
            } break;
            */
            default: {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }break;
            }
        }


        for (int i = 0; i < 349; ++i)
        {
            mInput.mCurrent.keys[i].wasPress = false;
            if (mInput.mLast.keys[i].isPress)
            {
                mInput.mCurrent.keys[i].wasPress = true;
            }
        }

        for (int i = 0; i < 3; ++i)
        {
            mInput.mCurrent.mouseButtons[i].wasPress = false;
            if (mInput.mLast.mouseButtons[i].isPress)
            {
                mInput.mCurrent.mouseButtons[i].wasPress = true;
            }
        }

    }

    int App::Run()
    {

        mTimer.Reset();

        mRunning = true;

        float accumulator = 0.0f;
        float dt = 1.0f / 120.0f;
        float targetFrameTime = 1.0f/120.0f;

        LARGE_INTEGER freq;
        LARGE_INTEGER lastTime;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&lastTime);

        while (mRunning)
        {
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);

            float frameTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / (float)freq.QuadPart;
            while (frameTime < targetFrameTime)
            {
                FlushEvents();
                QueryPerformanceCounter(&currentTime);
                frameTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / (float)freq.QuadPart;
            }

            lastTime = currentTime;

            FlushEvents();

            mTimer.Tick();

            if (!mPause)
            {
                char buffer[256];
                wsprintf(buffer, "%s FPS: %d\n", mWindowCaption, (int)(1.0f / mTimer.DeltaTime()));
                SetWindowText(mWindow, buffer);

                // Update
                UpdateScene();

                // Fix Update
                accumulator += mTimer.DeltaTime();
                int counter = 0;
                while (accumulator >= dt) {
                    counter++;
                    
                    FixUpdateScene(dt);
                    accumulator -= dt;
                }

                float t = accumulator / dt;
                PostUpdateScene(t); // NOTE: this is use for position interpolation before rendering

                DrawScene();
            }
            else
            {
                Sleep(100);
            }

            mInput.mLast = mInput.mCurrent;

        }
        return 0;
    }


    bool App::Init()
    {
        if (!InitMainWindow())
            return false;

        if (!InitDirect3D())
            return false;

        ShowWindow(mWindow, SW_SHOW);
        UpdateWindow(mWindow);

        return true;
    }

    void App::OnResize()
    {
        Assert(mImmediateContext);
        Assert(mDevice);
        Assert(mSwapChain);

        // Release the old views and the depth stencil buffer
        SAFE_RELEASE(mRenderTargetView);
        SAFE_RELEASE(mDepthStencilView);
        SAFE_RELEASE(mDepthStencilBuffer);

        // Resize the swapChain and recreate the render target view
        mSwapChain->ResizeBuffers(1, mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        ID3D11Texture2D* backBuffer = nullptr;
        mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        mDevice->CreateRenderTargetView(backBuffer, 0, &mRenderTargetView);
        SAFE_RELEASE(backBuffer);

        // Create the depth/stencil buffer and view
        D3D11_TEXTURE2D_DESC depthStencilDesc;
        depthStencilDesc.Width = mClientWidth;
        depthStencilDesc.Height = mClientHeight;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.ArraySize = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        if (mEnable4xMsaa)
        {
            depthStencilDesc.SampleDesc.Count = 4;
            depthStencilDesc.SampleDesc.Quality = m4xMsaaQuality - 1;
        }
        else
        {
            depthStencilDesc.SampleDesc.Count = 1;
            depthStencilDesc.SampleDesc.Quality = 0;
        }
        depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
        depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthStencilDesc.CPUAccessFlags = 0;
        depthStencilDesc.MiscFlags = 0;

        mDevice->CreateTexture2D(&depthStencilDesc, 0, &mDepthStencilBuffer);
        mDevice->CreateDepthStencilView(mDepthStencilBuffer, 0, &mDepthStencilView);
        // Bind the render target view and depth/stencil view to the pipeline.
        mImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);

        // Set the viewport transform.
        mViewport.TopLeftX = 0;
        mViewport.TopLeftY = 0;
        mViewport.Width = (float)mClientWidth;
        mViewport.Height = (float)mClientHeight;
        mViewport.MinDepth = 0.0f;
        mViewport.MaxDepth = 1.0f;
        mImmediateContext->RSSetViewports(1, &mViewport);

    }

    LRESULT App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CREATE:
        {
            CREATESTRUCTA* windowInfo = (CREATESTRUCTA*)lParam;
            mWindowX = windowInfo->x;
            mWindowY = windowInfo->y;
            return 0;
        }
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                mPause = true;
                mTimer.Stop();
            }
            else
            {
                mPause = false;
                mTimer.Start();
            }
            return 0;

        case WM_SIZE:
            mClientWidth = LOWORD(lParam);
            mClientHeight = HIWORD(lParam);
            if (mDevice)
            {
                if (wParam == SIZE_MINIMIZED)
                {
                    mPause = true;
                    mMinimized = true;
                    mMaximized = false;
                }
                else if (wParam == SIZE_MAXIMIZED)
                {
                    mPause = false;
                    mMinimized = false;
                    mMaximized = true;
                    OnResize();
                }
                else if (wParam == SIZE_RESTORED)
                {
                    if (mMinimized)
                    {
                        mPause = false;
                        mMinimized = false;
                        OnResize();
                    }
                    else if (mMaximized)
                    {
                        mPause = false;
                        mMaximized = false;
                        OnResize();
                    }
                    else if (mResizing)
                    {
                    }
                    else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
                    {
                        OnResize();
                    }
                }
            }
            return 0;
        case WM_ENTERSIZEMOVE:
            mPause = true;
            mResizing = true;
            mTimer.Stop();
            return 0;
        case WM_EXITSIZEMOVE:
            mPause = false;
            mResizing = false;
            mTimer.Start();
            OnResize();
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        // The WM_MENUCHAR message is sent when a menu is active and the user presses 
        // a key that does not correspond to any mnemonic or accelerator key. 
        case WM_MENUCHAR:
            // Don't beep when we alt-enter.
            return MAKELRESULT(0, MNC_CLOSE);

            // Catch this message so to prevent the window from becoming too small.
        case WM_GETMINMAXINFO:
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
            return 0;
        case WM_MOVE:
        {
            mWindowX = (UINT)(short)LOWORD(lParam);
            mWindowY = (UINT)(short)HIWORD(lParam);
            return 0;
        }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }


    bool App::InitMainWindow()
    {
        WNDCLASS wc;
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = MainWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = mInstance;
        wc.hIcon = LoadIcon(0, IDI_APPLICATION);
        wc.hCursor = LoadCursor(0, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wc.lpszMenuName = 0;
        wc.lpszClassName = "RubyEngineClassName";

        if (!RegisterClass(&wc))
        {
            MessageBoxA(0, "Register Class Failed.", 0, 0);
            return false;
        }

        RECT rect = {0, 0, mClientWidth, mClientHeight};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        mWindow = CreateWindowA("RubyEngineClassName", mWindowCaption,
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                width, height,
                                0, 0, mInstance, 0);
        if (!mWindow)
        {
            MessageBoxA(0, "Create Window Failed.", 0, 0);
            return false;
        }

        return true;

    }

    bool App::InitDirect3D()
    {
        UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        // Create the device and device context
        D3D_FEATURE_LEVEL featureLevel;
        HRESULT result = D3D11CreateDevice(0, mDriverType, 0, createDeviceFlags, 0, 0, D3D11_SDK_VERSION, &mDevice, &featureLevel, &mImmediateContext);
        if (FAILED(result))
        {
            MessageBoxA(0, "D3D11CreateDevice Failed.", 0, 0);
            return false;
        }
        if (featureLevel != D3D_FEATURE_LEVEL_11_0)
        {
            MessageBox(0, "Direct3D Feature Level 11 unsupported.", 0, 0);
            return false;
        }

        // Check for 4x MSAA quality support
        mDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality);
        Assert(m4xMsaaQuality > 0, "Error: DXGI_FORMAT_R8G8B8A8_UNORM not supported");

        // Create the swapChain
        DXGI_SWAP_CHAIN_DESC sd;
        sd.BufferDesc.Width = mClientWidth;
        sd.BufferDesc.Height = mClientHeight;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        if (mEnable4xMsaa)
        {
            sd.SampleDesc.Count = 4;
            sd.SampleDesc.Quality = m4xMsaaQuality - 1;
        }
        else
        {
            sd.SampleDesc.Count = 1;
            sd.SampleDesc.Quality = 0;
        }
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;
        sd.OutputWindow = mWindow;
        sd.Windowed = true;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sd.Flags = 0;

        // To correctly create the swap chain, we must use the IDXGIFactory that was
        // used to create the device.  If we tried to use a different IDXGIFactory instance
        // (by calling CreateDXGIFactory), we get an error: "IDXGIFactory::CreateSwapChain: 
        // This function is being called with a device from a different IDXGIFactory."
        IDXGIDevice* dxgiDevice = 0;
        mDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
        IDXGIAdapter* dxgiAdapter = 0;
        dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);
        IDXGIFactory* dxgiFactory = 0;
        dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
        dxgiFactory->CreateSwapChain(mDevice, &sd, &mSwapChain);
        SAFE_RELEASE(dxgiDevice);
        SAFE_RELEASE(dxgiAdapter);
        SAFE_RELEASE(dxgiFactory);

        // The remaining steps that need to be carried out for d3d creation
        // also need to be executed every time the window is resized.  So
        // just call the OnResize method here to avoid code duplication.
        OnResize();

        return true;

    }

}
