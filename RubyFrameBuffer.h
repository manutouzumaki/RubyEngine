#pragma once

namespace Ruby
{
    class FrameBuffer
    {
    public:
        FrameBuffer(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format)
            : mWidth(width), mHeight(height), mFormat(format),
            mRenderTargetView(nullptr),
            mShaderResourceView(nullptr),
            mTexture2D(nullptr)
        {
            CreateTextureAndViews(device, mWidth, mHeight, mFormat);
        }

        ~FrameBuffer()
        {
            SAFE_RELEASE(mTexture2D);
            SAFE_RELEASE(mRenderTargetView);
            SAFE_RELEASE(mShaderResourceView);
        }

        ID3D11ShaderResourceView* GetShaderResourceView()
        {
            return mShaderResourceView;
        }

        ID3D11RenderTargetView* GetRenderTargetView()
        {
            return mRenderTargetView;
        }

        void Resize(ID3D11Device* device, UINT width, UINT height)
        {
            CreateTextureAndViews(device, width, height, mFormat);
        }

    private:

        void CreateTextureAndViews(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format)
        {
            SAFE_RELEASE(mTexture2D);
            SAFE_RELEASE(mRenderTargetView);
            SAFE_RELEASE(mShaderResourceView);

            mWidth = width;
            mHeight = height;

            D3D11_TEXTURE2D_DESC texDesc;
            texDesc.Width = mWidth;
            texDesc.Height = mHeight;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = mFormat;
            texDesc.SampleDesc.Count = 1;
            texDesc.SampleDesc.Quality = 0;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            texDesc.CPUAccessFlags = 0;
            texDesc.MiscFlags = 0;

            if (FAILED(device->CreateTexture2D(&texDesc, 0, &mTexture2D))) {
                return;
            }

            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = mFormat;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;
            if (FAILED(device->CreateRenderTargetView(mTexture2D, &rtvDesc, &mRenderTargetView)))
            {
                return;
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = mFormat;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            if (FAILED(device->CreateShaderResourceView(mTexture2D, &srvDesc, &mShaderResourceView)))
            {
                return;
            }
        }

    private:
        UINT mWidth;
        UINT mHeight;
        DXGI_FORMAT mFormat;

        ID3D11Texture2D* mTexture2D;
        ID3D11RenderTargetView* mRenderTargetView;
        ID3D11ShaderResourceView* mShaderResourceView;
    };
}


