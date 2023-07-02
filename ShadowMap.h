#pragma once

class ShadowMap
{
public:
    ShadowMap(ID3D11Device* device, UINT width, UINT height)
        : mWidth(width), mHeight(height),
        mDepthMapSRV(nullptr),
        mDepthMapDSV(nullptr),
        mDepthMap(nullptr)
    {
        mViewport.TopLeftX = 0.0f;
        mViewport.TopLeftY = 0.0f;
        mViewport.Width = static_cast<float>(width);
        mViewport.Height = static_cast<float>(height);
        mViewport.MinDepth = 0.0f;
        mViewport.MaxDepth = 1.0f;

        D3D11_TEXTURE2D_DESC texDesc;
        texDesc.Width = mWidth;
        texDesc.Height = mHeight;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        device->CreateTexture2D(&texDesc, 0, &mDepthMap);

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        dsvDesc.Flags = 0;
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        device->CreateDepthStencilView(mDepthMap, &dsvDesc, &mDepthMapDSV);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        device->CreateShaderResourceView(mDepthMap, &srvDesc, &mDepthMapSRV);
    }
    ~ShadowMap() {
        SAFE_RELEASE(mDepthMapSRV);
        SAFE_RELEASE(mDepthMapDSV);
        SAFE_RELEASE(mDepthMap);
    }

    ID3D11ShaderResourceView* DepthMapSRV()
    {
        return mDepthMapSRV;
    }
    void BindDsvAndSetNullRenderTarget(ID3D11DeviceContext* dc)
    {
        dc->RSSetViewports(1, &mViewport);

        ID3D11RenderTargetView* renderTarget[1] = { 0 };
        dc->OMSetRenderTargets(1, renderTarget, mDepthMapDSV);
        dc->ClearDepthStencilView(mDepthMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
    }

    UINT mWidth;
    UINT mHeight;

    ID3D11ShaderResourceView* mDepthMapSRV;
    ID3D11DepthStencilView* mDepthMapDSV;

    ID3D11Texture2D* mDepthMap = nullptr;

    D3D11_VIEWPORT mViewport;

};



