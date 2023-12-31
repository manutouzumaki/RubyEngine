#pragma once

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx11Effect.h>
#include <DirectXMath.h>
#include <vector>

#include "RubyDefines.h"
#include "RubyTimer.h"
#include "RubyInput.h"
#include "GeometryGenerator.h"

using namespace DirectX;

namespace Ruby
{
    class App
    {
    public:
        App(HINSTANCE instance, UINT clientWidth, UINT clientHeight, const char* windowCaption, bool enable4xMsaa);
        virtual ~App();


        HINSTANCE Instance();
        HWND Window();
        float AspectRatio();
        int Run();
        void FlushEvents();

        // framework methods. derived client class overrides this methos
        // to implement specifics application requirements
        virtual bool Init();
        virtual void OnResize();
        virtual void UpdateScene() = 0;
        virtual void FixUpdateScene(float dt) {};
        virtual void PostUpdateScene(float t) {};
        virtual void DrawScene() = 0;
       
        virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
        virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
        virtual void OnMouseMove(WPARAM btnState, int x, int y) { }
        virtual void OnKeyDown(WPARAM vkCode) { }
        virtual void OnKeyUp(WPARAM vkCode) { }

        ID3D11Device* mDevice;

    protected:
        bool InitMainWindow();
        bool InitDirect3D();

    protected:
        HINSTANCE mInstance;
        HWND      mWindow;
        bool      mPause;
        bool      mMinimized;
        bool      mMaximized;
        bool      mResizing;
        bool      mRunning;
        UINT      m4xMsaaQuality;

        Timer mTimer;
        Input mInput;

        ID3D11DeviceContext* mImmediateContext; // this is for single threading, try the deferred contex for multithreading
        IDXGISwapChain* mSwapChain;
        ID3D11Texture2D* mDepthStencilBuffer;
        ID3D11RenderTargetView* mRenderTargetView;
        ID3D11DepthStencilView* mDepthStencilView;
        D3D11_VIEWPORT mViewport;

        // starting values
        const char* mWindowCaption;
        D3D_DRIVER_TYPE mDriverType;
        UINT mClientWidth;
        UINT mClientHeight;
        bool mEnable4xMsaa;

        UINT mWindowX;
        UINT mWindowY;
    };


}



