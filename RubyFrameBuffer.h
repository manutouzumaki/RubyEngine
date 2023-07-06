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

    struct CubeRTVs
    {
        ID3D11RenderTargetView* mRenderTargetViews[6];
        ID3D11DepthStencilView* mDepthStencilView;
    };

    class CubeFrameBuffer
    {
    public:
        CubeFrameBuffer(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format, UINT mipCount)
            : mWidth(width), mHeight(height), mFormat(format), mMipCount(mipCount),
            mCubeRTVs(),
            mShaderResourceView(nullptr),
            mTexture2D(nullptr)
        {
            CreateTextureAndViews(device, mWidth, mHeight, mFormat);
        }

        ~CubeFrameBuffer()
        {
            SAFE_RELEASE(mTexture2D);
            for (int j = 0; j < mCubeRTVs.size(); ++j)
            {
                for (int i = 0; i < 6; ++i)
                {
                    SAFE_RELEASE(mCubeRTVs[j].mRenderTargetViews[i]);
                }
                SAFE_RELEASE(mCubeRTVs[j].mDepthStencilView);
            }
            SAFE_RELEASE(mShaderResourceView);
            SAFE_RELEASE(mDepthStencilBuffer);
        }

        ID3D11ShaderResourceView* GetShaderResourceView()
        {
            return mShaderResourceView;
        }

        ID3D11DepthStencilView* GetDepthStencilView(UINT mipLevel)
        {
            return mCubeRTVs[mipLevel].mDepthStencilView;
        }

        ID3D11RenderTargetView* GetRenderTargetView(UINT index, UINT mipLevel)
        {
            return mCubeRTVs[mipLevel].mRenderTargetViews[index];
        }

        UINT GetMipCount()
        {
            return mMipCount;
        }

        void Resize(ID3D11Device* device, UINT width, UINT height)
        {
            CreateTextureAndViews(device, width, height, mFormat);
        }

    private:

        void CreateTextureAndViews(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format)
        {
            SAFE_RELEASE(mTexture2D);
            for (int j = 0; j < mCubeRTVs.size(); ++j)
            {
                for (int i = 0; i < 6; ++i)
                {
                    SAFE_RELEASE(mCubeRTVs[j].mRenderTargetViews[i]);
                }
                SAFE_RELEASE(mCubeRTVs[j].mDepthStencilView);
            }            
            SAFE_RELEASE(mShaderResourceView);
            SAFE_RELEASE(mDepthStencilBuffer);

            mWidth = width;
            mHeight = height;

            D3D11_TEXTURE2D_DESC texDesc;
            texDesc.Width = mWidth;
            texDesc.Height = mHeight;
            texDesc.MipLevels = mMipCount;
            texDesc.ArraySize = 6;
            texDesc.Format = mFormat;
            texDesc.SampleDesc.Count = 1;
            texDesc.SampleDesc.Quality = 0;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            texDesc.CPUAccessFlags = 0;
            texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

            if (FAILED(device->CreateTexture2D(&texDesc, 0, &mTexture2D))) {
                return;
            }

            for (UINT mipIndex = 0; mipIndex < mMipCount; ++mipIndex)
            {
                CubeRTVs cubeRtvs{};
                for (int i = 0; i < 6; ++i)
                {
                    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                    rtvDesc.Format = mFormat;
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtvDesc.Texture2DArray.ArraySize = 1;
                    rtvDesc.Texture2DArray.FirstArraySlice = D3D11CalcSubresource(0, i, 1);
                    rtvDesc.Texture2DArray.MipSlice = mipIndex;
                    if (FAILED(device->CreateRenderTargetView(mTexture2D, &rtvDesc, &cubeRtvs.mRenderTargetViews[i])))
                    {
                        return;
                    }
                }
                mCubeRTVs.push_back(cubeRtvs);
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = mFormat;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MipLevels = texDesc.MipLevels;
            srvDesc.TextureCube.MostDetailedMip = 0;
            if (FAILED(device->CreateShaderResourceView(mTexture2D, &srvDesc, &mShaderResourceView)))
            {
                return;
            }

            // Create the depth/stencil buffer and view
            D3D11_TEXTURE2D_DESC depthStencilDesc;
            depthStencilDesc.Width = mWidth;
            depthStencilDesc.Height = mHeight;
            depthStencilDesc.MipLevels = mMipCount;
            depthStencilDesc.ArraySize = 1;
            depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            depthStencilDesc.SampleDesc.Count = 1;
            depthStencilDesc.SampleDesc.Quality = 0;
            depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
            depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            depthStencilDesc.CPUAccessFlags = 0;
            depthStencilDesc.MiscFlags = 0;

            device->CreateTexture2D(&depthStencilDesc, 0, &mDepthStencilBuffer);

            for (UINT mipIndex = 0; mipIndex < mMipCount; ++mipIndex)
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC dsvDecs{};
                dsvDecs.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                dsvDecs.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsvDecs.Texture2D.MipSlice = mipIndex;
                ID3D11DepthStencilView* depthStencilView = nullptr;
                device->CreateDepthStencilView(mDepthStencilBuffer, &dsvDecs, &depthStencilView);
                mCubeRTVs[mipIndex].mDepthStencilView = depthStencilView;
            }
        }

    private:
        UINT mWidth;
        UINT mHeight;
        DXGI_FORMAT mFormat;
        UINT mMipCount;

        ID3D11Texture2D* mDepthStencilBuffer;
       
        ID3D11Texture2D* mTexture2D;

        std::vector<CubeRTVs> mCubeRTVs;

        ID3D11ShaderResourceView* mShaderResourceView;
    };

}


