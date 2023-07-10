#include "FPSDemo.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static XMMATRIX InverseTranspose(CXMMATRIX M)
{
    // Inverse-transpose is just applied to normals.  So zero out 
    // translation row so that it doesn't get into our inverse-transpose
    // calculation--we don't want the inverse-transpose of the translation.
    XMMATRIX A = M;
    A.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    XMVECTOR det = XMMatrixDeterminant(A);
    return XMMatrixTranspose(XMMatrixInverse(&det, A));
}


void FPSDemo::OnMouseDown(WPARAM btnState, int x, int y)
{

}
void FPSDemo::OnMouseUp(WPARAM btnState, int x, int y)
{

}
void FPSDemo::OnMouseMove(WPARAM btnState, int x, int y)
{

}
void FPSDemo::OnKeyDown(WPARAM vkCode)
{

}
void FPSDemo::OnKeyUp(WPARAM vkCode)
{

}

FPSDemo::FPSDemo(HINSTANCE instance,
    UINT clientWidth, 
    UINT clientHeight,
    const char* windowCaption,
    bool enable4xMsaa)
    : Ruby::App(instance, clientWidth, clientHeight, windowCaption, enable4xMsaa),
    mEffect(nullptr),
    mTechnique(nullptr),
    mFxWorldViewProj(nullptr),
    mInputLayout(nullptr),
    mMesh()
{
    XMMATRIX identity = XMMatrixIdentity();
    XMStoreFloat4x4(&mWorld, identity);
    XMStoreFloat4x4(&mView, identity);
    XMStoreFloat4x4(&mProj, identity);
}

FPSDemo::~FPSDemo()
{
    SAFE_DELETE(mEnviromentMap);
    SAFE_DELETE(mIrradianceMap);

    SAFE_DELETE(mPinPongFrameBuffers[1]);
    SAFE_DELETE(mPinPongFrameBuffers[0]);
    SAFE_DELETE(mFrameBuffers[1]);
    SAFE_DELETE(mFrameBuffers[0]);
    SAFE_DELETE(mShadowMap);
    SAFE_DELETE(mBrdfMap);

    SAFE_DELETE(mMesh[0]);
    SAFE_DELETE(mMesh[1]);
    SAFE_DELETE(mMesh[2]);
    SAFE_DELETE(mMesh[3]);
    SAFE_DELETE(mMesh[4]);
    SAFE_DELETE(mMesh[5]);
    SAFE_DELETE(mMesh[6]);


    SAFE_RELEASE(mHdrSkyTexture2D);
    SAFE_RELEASE(mHdrSkySRV);

    SAFE_RELEASE(mCubeMapSRV);
    SAFE_RELEASE(mBrdfEffect);
    SAFE_RELEASE(mSkyEffect);
    SAFE_RELEASE(mConvoluteEffect);
    SAFE_RELEASE(mCubemapEffect);
    SAFE_RELEASE(mEffect);
    SAFE_RELEASE(mDepthEffect);
    SAFE_RELEASE(mHdrEffect);
    SAFE_RELEASE(mBlurEffect);
    SAFE_RELEASE(mInputLayout);

    SAFE_RELEASE(mRasterizerStateBackCull);
    SAFE_RELEASE(mRasterizerStateFrontCull);

}

static float angle = 0.0f;
static XMVECTOR lightPos;
static float lightDist = 8.0f;
static float lightHeight = 4.0f;

#include <algorithm>

static const int gNrRows = 7;
static const int gNrCols = 7;
static const float gSpacing = 1.2f;


bool FPSDemo::Init()
{
    if (!Ruby::App::Init())
        return false;


    D3D11_RASTERIZER_DESC fillRasterizerNoneDesc = {};
    fillRasterizerNoneDesc.FillMode = D3D11_FILL_SOLID;
    fillRasterizerNoneDesc.CullMode = D3D11_CULL_BACK;
    fillRasterizerNoneDesc.DepthClipEnable = true;
    mDevice->CreateRasterizerState(&fillRasterizerNoneDesc, &mRasterizerStateBackCull);

    fillRasterizerNoneDesc.FillMode = D3D11_FILL_SOLID;
    fillRasterizerNoneDesc.CullMode = D3D11_CULL_FRONT;
    fillRasterizerNoneDesc.DepthClipEnable = true;
    mDevice->CreateRasterizerState(&fillRasterizerNoneDesc, &mRasterizerStateFrontCull);

    mMesh[0] = new Ruby::Mesh(mDevice, "./assets/level2.gltf", "./assets/level2.bin", "./");
#if 0
    mMesh[1] = new Ruby::Mesh(mDevice, "./assets/sphere.gltf",   "./assets/sphere.bin",  "./");
    mMesh[2] = new Ruby::Mesh(mDevice, "./assets/mono.gltf",   "./assets/mono.bin",  "./");
    mMesh[3] = new Ruby::Mesh(mDevice, "./assets/level.gltf",  "./assets/level.bin", "./");
    mMesh[4] = new Ruby::Mesh(mDevice, "./assets/cube.gltf", "./assets/cube.bin", "./");
    mMesh[5] = new Ruby::Mesh(mDevice, "./assets/castel.gltf", "./assets/castel.bin", "./");
#endif
#if 1
    // try to split the mesh

    float meshWidth = 25.5f;
    float meshDepth = 25.7f;

    float stepZ = meshDepth / 3.0f;

    float centerZ = -6.4f;
    float centerX = 2.6f;

    // 1
    Ruby::Plane plane;

    plane.d = centerX;
    plane.n = XMFLOAT3(1.0f, 0.0f, 0.0f);
    mMesh[1] = mMesh[0]->Clip(mDevice, plane);

    Ruby::Mesh* tmp = mMesh[1];
    plane.d = (centerZ + (meshDepth*0.5f)) - stepZ;
    plane.n = XMFLOAT3(0.0f, 0.0f, 1.0f);
    mMesh[1] = mMesh[1]->Clip(mDevice, plane);
    SAFE_DELETE(tmp);


    // 2
    plane.d = centerX;
    plane.n = XMFLOAT3(1.0f, 0.0f, 0.0f);
    mMesh[2] = mMesh[0]->Clip(mDevice, plane);

    tmp = mMesh[2];
    plane.d = -((centerZ + (meshDepth * 0.5f)) - stepZ);
    plane.n = XMFLOAT3(0.0f, 0.0f, -1.0f);
    mMesh[2] = mMesh[2]->Clip(mDevice, plane);
    SAFE_DELETE(tmp);

    tmp = mMesh[2];
    plane.d = (centerZ + (meshDepth * 0.5f) - stepZ*2.0f);
    plane.n = XMFLOAT3(0.0f, 0.0f, 1.0f);
    mMesh[2] = mMesh[2]->Clip(mDevice, plane);
    SAFE_DELETE(tmp);

    // 3
    plane.d = centerX;
    plane.n = XMFLOAT3(1.0f, 0.0f, 0.0f);
    mMesh[3] = mMesh[0]->Clip(mDevice, plane);

    tmp = mMesh[3];
    plane.d = -((centerZ + (meshDepth * 0.5f) - stepZ * 2.0f));
    plane.n = XMFLOAT3(0.0f, 0.0f, -1.0f);
    mMesh[3] = mMesh[3]->Clip(mDevice, plane);
    SAFE_DELETE(tmp);

    // 4
    plane.d = -centerX;
    plane.n = XMFLOAT3(-1.0f, 0.0f, 0.0f);
    mMesh[4] = mMesh[0]->Clip(mDevice, plane);

    tmp = mMesh[4];
    plane.d = (centerZ + (meshDepth * 0.5f)) - stepZ;
    plane.n = XMFLOAT3(0.0f, 0.0f, 1.0f);
    mMesh[4] = mMesh[4]->Clip(mDevice, plane);
    SAFE_DELETE(tmp);


    // 5
    plane.d = -centerX;
    plane.n = XMFLOAT3(-1.0f, 0.0f, 0.0f);
    mMesh[5] = mMesh[0]->Clip(mDevice, plane);

    tmp = mMesh[5];
    plane.d = -((centerZ + (meshDepth * 0.5f)) - stepZ);
    plane.n = XMFLOAT3(0.0f, 0.0f, -1.0f);
    mMesh[5] = mMesh[5]->Clip(mDevice, plane);
    SAFE_DELETE(tmp);

    tmp = mMesh[5];
    plane.d = (centerZ + (meshDepth * 0.5f) - stepZ * 2.0f);
    plane.n = XMFLOAT3(0.0f, 0.0f, 1.0f);
    mMesh[5] = mMesh[5]->Clip(mDevice, plane);
    SAFE_DELETE(tmp);

    // 6
    plane.d = -centerX;
    plane.n = XMFLOAT3(-1.0f, 0.0f, 0.0f);
    mMesh[6] = mMesh[0]->Clip(mDevice, plane);

    tmp = mMesh[6];
    plane.d = -((centerZ + (meshDepth * 0.5f) - stepZ * 2.0f));
    plane.n = XMFLOAT3(0.0f, 0.0f, -1.0f);
    mMesh[6] = mMesh[6]->Clip(mDevice, plane);
    SAFE_DELETE(tmp);

#endif


    for (int row = 0; row < gNrRows; ++row)
    {
        float matallic = (float)row / (float)gNrRows;
        for (int col = 0; col < gNrCols; ++col)
        {
            Ruby::Pbr::Material material{};
            material.Albedo = XMFLOAT4(0.5f, 0.0f, 0.0f, 1.0f);
            material.Metallic = matallic;
            material.Roughness = std::clamp((float)col / (float)gNrCols, 0.05f, 1.0f);
            material.Ao = 1.0f;
            mMaterials[row * gNrRows + col] = material;

        }
    }

    // Load HDR Texture
    {
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        //float* data = stbi_loadf("./assets/newport_loft.hdr", &width, &height, &nrComponents, 0);
        float* data = stbi_loadf("./assets/sky.hdr", &width, &height, &nrComponents, 0);
        if (data)
        {
            // Create HDR Texture2D
            D3D11_TEXTURE2D_DESC texDesc;
            texDesc.Width = width;
            texDesc.Height = height;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            texDesc.SampleDesc.Count = 1;
            texDesc.SampleDesc.Quality = 0;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            texDesc.CPUAccessFlags = 0;
            texDesc.MiscFlags = 0;

            std::vector<XMFLOAT4> colors;
            for (int i = 0; i < width * height * 3; i += 3)
            {
                XMFLOAT4 color{ data[i + 0], data[i + 1], data[i + 2], 1.0f };
                colors.push_back(color);
            }

            D3D11_SUBRESOURCE_DATA initData{};
            initData.pSysMem = colors.data();
            initData.SysMemPitch = width * (sizeof(float) * 4);
            if (FAILED(mDevice->CreateTexture2D(&texDesc, &initData, &mHdrSkyTexture2D)))
            {
                OutputDebugString("Error Creating HDR Texture2D\n");
            }

            // Create Shader Resource View For HDR Texture
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            if (FAILED(mDevice->CreateShaderResourceView(mHdrSkyTexture2D, &srvDesc, &mHdrSkySRV)))
            {
                OutputDebugString("Error Creating HDR Shader Resource View\n");
            }

            OutputDebugStringA("HDR Texture Loaded\n");

            stbi_image_free(data);
        }
    }

    // Load Cube map
    {
        HRESULT result = D3DX11CreateShaderResourceViewFromFileA(mDevice, "./assets/cubemap.dds", 0, 0, &mCubeMapSRV, 0);
        if (SUCCEEDED(result))
        {
            OutputDebugStringA("CubeMap Load succeeded\n");
        }
    }

    // Load the geometry for the sky (cubemap)
    {
        Ruby::MeshData box;
        Ruby::GeometryGenerator geoGen;
        geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

        std::vector<Ruby::MeshGeometry::Subset> subsetTable;
        Ruby::MeshGeometry::Subset subset{};
        subset.IndexStart = 0;
        subset.IndexCount = box.Indices.size();
        subsetTable.push_back(subset);

        mSky.SetVertices(mDevice, box.Vertices.data(), box.Vertices.size());
        mSky.SetIndices(mDevice, box.Indices.data(), box.Indices.size());
        mSky.SetSubsetTable(subsetTable);
    }
    
    mShadowMap = new ShadowMap(mDevice, 1024, 1024);

    mFrameBuffers[0] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mFrameBuffers[1] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mPinPongFrameBuffers[0] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mPinPongFrameBuffers[1] = new Ruby::FrameBuffer(mDevice, mClientWidth, mClientHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

    
    mBrdfMap = new Ruby::FrameBuffer(mDevice, 512, 512, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mEnviromentMap = new Ruby::CubeFrameBuffer(mDevice, 512, 512, DXGI_FORMAT_R32G32B32A32_FLOAT, 5);
    mIrradianceMap = new Ruby::CubeFrameBuffer(mDevice, 512, 512, DXGI_FORMAT_R32G32B32A32_FLOAT, 1);
    



    // create the shaders and fx
    DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
    shaderFlags |= D3D10_SHADER_DEBUG;
#endif
    shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
    
    // Create Color Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/pbrColor.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mTechnique = mEffect->GetTechniqueByName("ColorTech");
        mFxWorld = mEffect->GetVariableByName("gWorld")->AsMatrix();
        mFxWorldInvTranspose = mEffect->GetVariableByName("gWorldInvTranspose")->AsMatrix();
        mFxWorldViewProj = mEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mFxMaterial = mEffect->GetVariableByName("gMaterial");
        mFxDirLight = mEffect->GetVariableByName("gDirLight");
        mFxPointLight = mEffect->GetVariableByName("gPointLight");
        mFxEyePosW = mEffect->GetVariableByName("gEyePosW")->AsVector();
        mFxLightSpaceMatrix = mEffect->GetVariableByName("gLightSpaceMatrix")->AsMatrix();
        mFxShadowMap = mEffect->GetVariableByName("gShadowMap")->AsShaderResource();
        mFxIrradianceMap = mEffect->GetVariableByName("gIrradianceMap")->AsShaderResource();
        mFxPrefilteredColor = mEffect->GetVariableByName("gPrefilteredColor")->AsShaderResource();
        mFxBrdfLUT = mEffect->GetVariableByName("gBrdfLUT")->AsShaderResource();
    }
    // Create Depth Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/depth.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mDepthEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mDepthTechnique = mDepthEffect->GetTechniqueByName("DepthTech");
        mDepthFxWorld = mDepthEffect->GetVariableByName("gWorld")->AsMatrix();
        mDepthFxLightSpaceMatrix = mDepthEffect->GetVariableByName("gLightSpaceMatrix")->AsMatrix();
;
    }
    // Create HDR Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/hdr.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mHdrEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mHdrTechnique = mHdrEffect->GetTechniqueByName("HdrTech");
        mHdrBackBuffer = mHdrEffect->GetVariableByName("gBackBuffer")->AsShaderResource();
        mBloomBuffer = mHdrEffect->GetVariableByName("gBloomBuffer")->AsShaderResource();
    }
    // Create Blur Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/blur.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mBlurEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mBlurTechnique = mBlurEffect->GetTechniqueByName("BlurTech");
        mBlurFxHorizontal = mBlurEffect->GetVariableByName("horizontal");
        mBlurFxImage = mBlurEffect->GetVariableByName("gImage")->AsShaderResource();
    }
    // Create Cubemap Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/cubemap.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mCubemapEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mCubemapTechnique = mCubemapEffect->GetTechniqueByName("CubeMapTech");
        mCubemapWorldViewProj = mCubemapEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mCubemapRoughness = mCubemapEffect->GetVariableByName("gRoughness");
        mCubeMap = mCubemapEffect->GetVariableByName("gCubeMap")->AsShaderResource();
    }
    // Create Convolute Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/convolute.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mConvoluteEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mConvoluteTechnique = mConvoluteEffect->GetTechniqueByName("ConvoluteTech");
        mConvoluteWorldViewProj = mConvoluteEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mConvoluteCubeMap = mConvoluteEffect->GetVariableByName("gCubeMap")->AsShaderResource();
    }
    // Create Sky Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/sky.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mSkyEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mSkyTechnique = mSkyEffect->GetTechniqueByName("SkyTech");
        mSkyWorldViewProj = mSkyEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
        mSkyCubeMap = mSkyEffect->GetVariableByName("gCubeMap")->AsShaderResource();
        mSkyTimer = mSkyEffect->GetVariableByName("gTimer");
    }
    // Create BRDF Effect
    {
        ID3D10Blob* compiledShader = 0;
        ID3D10Blob* compilationMsgs = 0;
        HRESULT result = D3DX11CompileFromFile("./FX/brdf.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

        if (compilationMsgs != 0)
        {
            MessageBox(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
            SAFE_RELEASE(compilationMsgs);
        }

        if (FAILED(result))
        {
            MessageBox(0, "Error: compiling fx ...", 0, 0);
        }

        result = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 0, mDevice, &mBrdfEffect);

        if (FAILED(result))
        {
            MessageBox(0, "Error: creating fx ...", 0, 0);
        }

        SAFE_RELEASE(compiledShader);

        mBrdfTechnique = mBrdfEffect->GetTechniqueByName("BrdfTech");

    }

    // set the vertex Layout
    D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    D3DX11_PASS_DESC passDesc;
    mTechnique->GetPassByIndex(0)->GetDesc(&passDesc);
    HRESULT result = mDevice->CreateInputLayout(vertexDesc, 4, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayout);

    // set proj matrices
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);

    // Build the view matrix.
    XMFLOAT3 eyePos = XMFLOAT3(10.0f, 5.0f, -10.0f);
    //XMFLOAT3 eyePos = XMFLOAT3(0, 0.0f, 6.0f);
    XMVECTOR pos = XMVectorSet(eyePos.x, eyePos.y, eyePos.z, 0.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, V);

    mFxEyePosW->SetRawValue(&eyePos, 0, sizeof(XMFLOAT3));

    lightPos = XMVectorSet(0.0f, lightHeight, lightDist, 1.0f);

    mDirLight.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    XMVECTOR lightDir = XMVector3Normalize(lightPos);
    XMStoreFloat3(&mDirLight.Direction, lightDir);
    mFxDirLight->SetRawValue(&mDirLight, 0, sizeof(Ruby::Pbr::DirectionalLight));

    // Point light--position is changed every frame to animate in UpdateScene function.
    mPointLight.Color = XMFLOAT4(100.0f, 100.0f, 100.0f, 1.0f);
    mPointLight.Position = XMFLOAT3(4.0f, 6.0f, -10.0f);
    mFxPointLight->SetRawValue(&mPointLight, 0, sizeof(Ruby::Pbr::PointLight));

    // Render CubeMap to Frame Buffer, we to this just one time at startup
    {
        XMMATRIX captureProj = XMMatrixPerspectiveFovLH((90.0f/180.0f) * XM_PI, 1.0f, 0.1f, 10.0f);
        XMMATRIX captureViews[] = { 
            XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet( 1.0f,  0.0f,  0.0f, 0.0f), XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f)),
            XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet(-1.0f,  0.0f,  0.0f, 0.0f), XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f)),
            XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet( 0.0f,  1.0f,  0.0f, 0.0f), XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)),
            XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet( 0.0f, -1.0f,  0.0f, 0.0f), XMVectorSet(0.0f, 0.0f,  1.0f, 0.0f)),
            XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet( 0.0f,  0.0f,  1.0f, 0.0f), XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f)),
            XMMatrixLookAtLH(XMVectorSet(0.0f,  0.0f,  0.0f, 1.0f), XMVectorSet( 0.0f,  0.0f, -1.0f, 0.0f), XMVectorSet(0.0f, 1.0f,  0.0f, 0.0f))
        };

        mCubeMap->SetResource(mHdrSkySRV);
        mConvoluteCubeMap->SetResource(mHdrSkySRV);

        mImmediateContext->IASetInputLayout(mInputLayout);
        mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        mImmediateContext->RSSetViewports(1, &mViewport);
        mImmediateContext->RSSetState(mRasterizerStateFrontCull);

        // create enviroment map
        {
            // Set the viewport transform.
            float CurrentWidht = 512.0f;
            float CurrentHeight = 512.0f;

            for (int mipIndex = 0; mipIndex < mEnviromentMap->GetMipCount(); ++mipIndex)
            {
                mViewport.TopLeftX = 0;
                mViewport.TopLeftY = 0;
                mViewport.Width = CurrentWidht;
                mViewport.Height = CurrentHeight;
                mViewport.MinDepth = 0.0f;
                mViewport.MaxDepth = 1.0f;
                mImmediateContext->RSSetViewports(1, &mViewport);


                float roughness = (float)mipIndex / (float)(mEnviromentMap->GetMipCount() - 1);
                mCubemapRoughness->SetRawValue(&roughness, 0, sizeof(float));

                CurrentWidht *= 0.5f;
                CurrentHeight *= 0.5f;

                for (int i = 0; i < 6; ++i)
                {
                    ID3D11RenderTargetView* renderTarget[1] = { mEnviromentMap->GetRenderTargetView(i, mipIndex) };
                    mImmediateContext->OMSetRenderTargets(1, renderTarget, mEnviromentMap->GetDepthStencilView(mipIndex));

                    XMVECTORF32 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
                    mImmediateContext->ClearRenderTargetView(mEnviromentMap->GetRenderTargetView(i, mipIndex), (float*)&clearColor);
                    mImmediateContext->ClearDepthStencilView(mEnviromentMap->GetDepthStencilView(mipIndex), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

                    XMMATRIX viewProj = captureViews[i] * captureProj;

                    D3DX11_TECHNIQUE_DESC techDesc;
                    mCubemapTechnique->GetDesc(&techDesc);
                    for (UINT p = 0; p < techDesc.Passes; ++p)
                    {
                        mCubemapWorldViewProj->SetMatrix(reinterpret_cast<float*>(&viewProj));
                        mCubemapTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                        mSky.Draw(mImmediateContext, 0);
                    }
                }
            }
        }


        // create irradiance map
        {
            mViewport.TopLeftX = 0;
            mViewport.TopLeftY = 0;
            mViewport.Width = 512.0f;
            mViewport.Height = 512.0f;
            mViewport.MinDepth = 0.0f;
            mViewport.MaxDepth = 1.0f;
            mImmediateContext->RSSetViewports(1, &mViewport);

            for (int i = 0; i < 6; ++i)
            {
                ID3D11RenderTargetView* renderTarget[1] = { mIrradianceMap->GetRenderTargetView(i, 0) };
                mImmediateContext->OMSetRenderTargets(1, renderTarget, mIrradianceMap->GetDepthStencilView(0));

                XMVECTORF32 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
                mImmediateContext->ClearRenderTargetView(mIrradianceMap->GetRenderTargetView(i, 0), (float*)&clearColor);
                mImmediateContext->ClearDepthStencilView(mIrradianceMap->GetDepthStencilView(0), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

                XMMATRIX viewProj = captureViews[i] * captureProj;

                D3DX11_TECHNIQUE_DESC techDesc;
                mConvoluteTechnique->GetDesc(&techDesc);
                for (UINT p = 0; p < techDesc.Passes; ++p)
                {
                    mConvoluteWorldViewProj->SetMatrix(reinterpret_cast<float*>(&viewProj));
                    mConvoluteTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mSky.Draw(mImmediateContext, 0);
                }
            }
        }

        mImmediateContext->RSSetState(mRasterizerStateBackCull);

        mViewport.TopLeftX = 0;
        mViewport.TopLeftY = 0;
        mViewport.Width = mClientWidth;
        mViewport.Height = mClientHeight;
        mViewport.MinDepth = 0.0f;
        mViewport.MaxDepth = 1.0f;
        mImmediateContext->RSSetViewports(1, &mViewport);
    }

    // Brdf Texture Generation
    {
        mViewport.TopLeftX = 0;
        mViewport.TopLeftY = 0;
        mViewport.Width = 512.0f;
        mViewport.Height = 512.0f;
        mViewport.MinDepth = 0.0f;
        mViewport.MaxDepth = 1.0f;
        mImmediateContext->RSSetViewports(1, &mViewport);
        ID3D11RenderTargetView* renderTarget[1] = { mBrdfMap->GetRenderTargetView() };
        mImmediateContext->OMSetRenderTargets(1, renderTarget, 0);

        XMVECTORF32 clearColor = { 0.0f, 1.0f, 1.0f, 1.0f };
        mImmediateContext->ClearRenderTargetView(mBrdfMap->GetRenderTargetView(), (float*)&clearColor);
        {
            D3DX11_TECHNIQUE_DESC techDesc;
            mBrdfTechnique->GetDesc(&techDesc);
            for (UINT p = 0; p < techDesc.Passes; ++p)
            {
                mBrdfTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                mImmediateContext->Draw(6, 0);
            }
        }
        mViewport.TopLeftX = 0;
        mViewport.TopLeftY = 0;
        mViewport.Width = mClientWidth;
        mViewport.Height = mClientHeight;
        mViewport.MinDepth = 0.0f;
        mViewport.MaxDepth = 1.0f;
        mImmediateContext->RSSetViewports(1, &mViewport);
    }

    // Set the cubemap to the shader
    mFxIrradianceMap->SetResource(mIrradianceMap->GetShaderResourceView());
    mFxPrefilteredColor->SetResource(mEnviromentMap->GetShaderResourceView());
    mFxBrdfLUT->SetResource(mBrdfMap->GetShaderResourceView());

    mSkyCubeMap->SetResource(mEnviromentMap->GetShaderResourceView());


    return true;
}

void FPSDemo::OnResize()
{
    Ruby::App::OnResize();
    // set proj matrices
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);

    if(mFrameBuffers[0])
        mFrameBuffers[0]->Resize(mDevice, mClientWidth, mClientHeight);

    if (mFrameBuffers[1])
        mFrameBuffers[1]->Resize(mDevice, mClientWidth, mClientHeight);

    if (mPinPongFrameBuffers[0])
        mPinPongFrameBuffers[0]->Resize(mDevice, mClientWidth, mClientHeight);

    if (mPinPongFrameBuffers[1])
        mPinPongFrameBuffers[1]->Resize(mDevice, mClientWidth, mClientHeight);
}

void FPSDemo::UpdateScene(float dt)
{
    angle += 0.5f * dt;

    XMMATRIX world = XMMatrixRotationY(angle) * XMMatrixRotationX(angle * 2.0f);
    XMStoreFloat4x4(&mWorld, world);

    XMFLOAT3 targetDir = XMFLOAT3(0, 0, 0);

    // Build the view matrix.
    XMFLOAT3 eyePos = XMFLOAT3(cosf(angle) * 20.0f, 5.0f, sinf(angle) * 20.0f);
    XMVECTOR pos = XMVectorSet(eyePos.x, eyePos.y, eyePos.z, 1.0f);
    XMVECTOR target = XMVectorSet(targetDir.x, targetDir.y, targetDir.z, 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, V);
    mFxEyePosW->SetRawValue(&eyePos, 0, sizeof(XMFLOAT3));

    static float timer = 0.0f;

    timer += dt;
    if (timer >= 4.0f)
    {
        timer = 0.0f;
    }
    mSkyTimer->SetRawValue(&timer, 0, sizeof(float));

#if 0
    char buffer[256];
    wsprintf(buffer, "FPS: %d\n", (DWORD)(1.0f / dt));
    OutputDebugStringA(buffer);
#endif
    //mPointLight.Position = XMFLOAT3(sinf(angle*2) * 5.0f, 6, cosf(angle) * 5.0f);
    //mFxPointLight->SetRawValue(&mPointLight, 0, sizeof(Ruby::Pbr::PointLight));

}



void FPSDemo::DrawScene()
{
#if 1
    mImmediateContext->IASetInputLayout(mInputLayout);
    mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mShadowMap->BindDsvAndSetNullRenderTarget(mImmediateContext);
    
    // render the scene to the depth buffer only for shadow calculations
    {
        XMVECTOR targetPos = XMVectorZero();
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);


        XMMATRIX V = XMMatrixLookAtLH(lightPos, targetPos, up);

        XMMATRIX P = XMMatrixOrthographicOffCenterLH(-20, 20, -20, 20, 0.0001f, 45.0f);

        XMMATRIX lightSpaceMatrix = V * P;
        mDepthFxLightSpaceMatrix->SetMatrix(reinterpret_cast<float*>(&lightSpaceMatrix));
        mFxLightSpaceMatrix->SetMatrix(reinterpret_cast<float*>(&lightSpaceMatrix));

        D3DX11_TECHNIQUE_DESC techDesc;
        mDepthTechnique->GetDesc(&techDesc);
        for (UINT p = 0; p < techDesc.Passes; ++p)
        {
#if 0
            // draw mesh 0
            {
                XMMATRIX world = XMMatrixTranslation(-2.0f, 3.99f, -2.0f);
                mDepthFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                Ruby::Mesh* mesh = mMesh[0];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mDepthTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 2
            {
                XMMATRIX world = XMMatrixTranslation(-1, 7, -2);
                mDepthFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                Ruby::Mesh* mesh = mMesh[2];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mDepthTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 3
            {
                XMMATRIX world = XMMatrixRotationY(XM_PIDIV4) * XMMatrixTranslation(8, 0.127f, -10.0f);
                mDepthFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                Ruby::Mesh* mesh = mMesh[3];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mDepthTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 4
            {
                XMMATRIX world = XMMatrixScaling(20.0f, 0.0000000001f, 40.0f) * XMMatrixTranslation(0, 0, 0);
                mDepthFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                Ruby::Mesh* mesh = mMesh[4];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mDepthTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 5
            {
                XMMATRIX world = XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-10, 4.7, -25);
                mDepthFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                Ruby::Mesh* mesh = mMesh[5];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mDepthTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
        
#endif
            float width = 0.0f;
            for (int indexX = 0; indexX < 2; ++indexX)
            {
                float depth = 0.0f;
                for (int indexZ = 0; indexZ < 3; ++indexZ)
                {

                    int index = (indexX * 3 + indexZ) + 1;
                    XMMATRIX world = XMMatrixTranslation(width, 0, depth);
                    mDepthFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                    Ruby::Mesh* mesh = mMesh[index];
                    for (UINT i = 0; i < mesh->Mat.size(); ++i)
                    {
                        mDepthTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                        mesh->ModelMesh.Draw(mImmediateContext, i);
                    }
                    depth -= 1.0f;
                }
                width -= 1.0f;
            }
            

        }
    }
    

    ID3D11RenderTargetView* frameBuffers[2] = { mFrameBuffers[0]->GetRenderTargetView(), mFrameBuffers[1]->GetRenderTargetView()};
    mImmediateContext->OMSetRenderTargets(2, frameBuffers, mDepthStencilView);

    mImmediateContext->RSSetViewports(1, &mViewport);

    mFxShadowMap->SetResource(mShadowMap->mDepthMapSRV);

    XMVECTORF32 clearColor = { 0.0f, 0.0f, 0.001f, 1.0f };
    mImmediateContext->ClearRenderTargetView(mFrameBuffers[0]->GetRenderTargetView(), (float*)&clearColor);
    mImmediateContext->ClearRenderTargetView(mFrameBuffers[1]->GetRenderTargetView(), (float*)&clearColor);
    mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    {
        // Build the view matrix.

        // Set constants
        XMMATRIX viewProj = XMLoadFloat4x4(&mView) * XMLoadFloat4x4(&mProj);
        D3DX11_TECHNIQUE_DESC techDesc;
        mTechnique->GetDesc(&techDesc);
        for (UINT p = 0; p < techDesc.Passes; ++p)
        {
#if 0
            // draw mesh 3
            {
                XMMATRIX world = XMMatrixRotationY(XM_PIDIV4) * XMMatrixTranslation(8, 0.127f, -10.0f);
                XMMATRIX worldInvTranspose = InverseTranspose(world);
                XMMATRIX worldViewProj = world * viewProj;
                mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                Ruby::Mesh* mesh = mMesh[3];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mFxMaterial->SetRawValue(&mesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                    mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 0
            {
                XMMATRIX world = XMMatrixTranslation(-2.0f, 3.99f, -2.0f);
                XMMATRIX worldInvTranspose = InverseTranspose(world);
                XMMATRIX worldViewProj = (world * XMMatrixTranslation(0, -2, 0)) * viewProj;
                mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                Ruby::Mesh* mesh = mMesh[0];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mFxMaterial->SetRawValue(&mesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                    mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 1
            {
                for (int row = 0; row < gNrRows; ++row)
                {
                    for (int col = 0; col < gNrCols; ++col)
                    {
                        XMMATRIX world = XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation((col - (gNrCols / 2)) * gSpacing,
                            (row - (gNrRows / 2)) * gSpacing + 4, 4.0f);
                        XMMATRIX worldInvTranspose = InverseTranspose(world);
                        XMMATRIX worldViewProj = world * viewProj;
                        mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                        mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                        mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                        Ruby::Mesh* mesh = mMesh[1];
                        for (UINT i = 0; i < mesh->Mat.size(); ++i)
                        {
                            mFxMaterial->SetRawValue(&mMaterials[row * gNrRows + col], 0, sizeof(Ruby::Pbr::Material));
                            mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                            mesh->ModelMesh.Draw(mImmediateContext, i);
                        }
                    }
                }
 
            }

            // draw mesh 2
            {
                XMMATRIX world = XMMatrixTranslation(-1, 7, -2);
                XMMATRIX worldInvTranspose = InverseTranspose(world);
                XMMATRIX worldViewProj = world * viewProj;
                mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                Ruby::Mesh* mesh = mMesh[2];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mFxMaterial->SetRawValue(&mesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                    mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 4
            {
                //XMMATRIX world = XMMatrixScaling(10.0f, 0.0000000001f, 10.0f) * XMMatrixTranslation(0, 0, 0);
                XMMATRIX world = XMMatrixScaling(20.0f, 0.0000000001f, 40.0f) *  XMMatrixTranslation(0, 0, 0);
                XMMATRIX worldInvTranspose = InverseTranspose(world);
                XMMATRIX worldViewProj = world * viewProj;
                mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                Ruby::Mesh* mesh = mMesh[4];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mFxMaterial->SetRawValue(&mesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                    mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
            // draw mesh 5
            {
                XMMATRIX world = XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-10, 4.7, -25);
                XMMATRIX worldInvTranspose = InverseTranspose(world);
                XMMATRIX worldViewProj = world * viewProj;
                mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                Ruby::Mesh* mesh = mMesh[5];
                for (UINT i = 0; i < mesh->Mat.size(); ++i)
                {
                    mFxMaterial->SetRawValue(&mesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                    mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                    mesh->ModelMesh.Draw(mImmediateContext, i);
                }
            }
#endif
            float width = 0.0f;
            for (int indexX = 0; indexX < 2; ++indexX)
            {
                float depth = 0.0f;
                for (int indexZ = 0; indexZ < 3; ++indexZ)
                {

                    int index = (indexX * 3 + indexZ) + 1;

                    XMMATRIX world = XMMatrixTranslation(width, 0, depth);
                    XMMATRIX worldInvTranspose = InverseTranspose(world);
                    XMMATRIX worldViewProj = world * viewProj;
                    mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
                    mFxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
                    mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
                    Ruby::Mesh* mesh = mMesh[index];
                    for (UINT i = 0; i < mesh->Mat.size(); ++i)
                    {
                        mFxMaterial->SetRawValue(&mesh->Mat[i], 0, sizeof(Ruby::Pbr::Material));
                        mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                        mesh->ModelMesh.Draw(mImmediateContext, i);
                    }
                    depth -= 1.0f;
                }
                width -= 1.0f;
            }
            

        }

        mSkyTechnique->GetDesc(&techDesc);
        for (UINT p = 0; p < techDesc.Passes; ++p)
        {
            XMFLOAT3 eyePos = XMFLOAT3(cosf(angle) * 20.0f, 5.0f, sinf(angle) * 20.0f);
            XMMATRIX world = XMMatrixTranslation(eyePos.x, eyePos.y, eyePos.z);
            XMMATRIX worldViewProj = world * viewProj;
            mSkyWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
            mSkyTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
            mSky.Draw(mImmediateContext, 0);
        }
    }

    mImmediateContext->RSSetState(0);

    // Blur pass
    {
        bool horizontal = true, firstIteration = true;
        float horizontalFloat = 1.0;
        int amount = 10;
        ID3D11RenderTargetView* renderTargets[2] = { mPinPongFrameBuffers[0]->GetRenderTargetView(),
                                                     mPinPongFrameBuffers[1]->GetRenderTargetView()};

        ID3D11ShaderResourceView* resourceViews[2] = { mPinPongFrameBuffers[0]->GetShaderResourceView(),
                                                       mPinPongFrameBuffers[1]->GetShaderResourceView() };
        for (unsigned int i = 0; i < amount; i++) {
            
            mImmediateContext->OMSetRenderTargets(1, &renderTargets[horizontal], 0);

            mBlurFxHorizontal->SetRawValue(&horizontal, 0, sizeof(bool));
            mBlurFxImage->SetResource(firstIteration ? mFrameBuffers[1]->GetShaderResourceView() : resourceViews[!horizontal]);

            D3DX11_TECHNIQUE_DESC techDesc;
            mBlurTechnique->GetDesc(&techDesc);
            for (UINT p = 0; p < techDesc.Passes; ++p)
            {


                mBlurTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                mImmediateContext->Draw(6, 0);

            }

            horizontal = !horizontal;
            if (firstIteration)
                firstIteration = false;

            ID3D11ShaderResourceView* nullSRV[16] = { 0 };
            mImmediateContext->PSSetShaderResources(0, 16, nullSRV);
        }
    }


    mHdrBackBuffer->SetResource(mFrameBuffers[0]->GetShaderResourceView());
    mBloomBuffer->SetResource(mPinPongFrameBuffers[1]->GetShaderResourceView());
    // Final render to quad2d
    {
        ID3D11RenderTargetView* renderTarget[1] = { mRenderTargetView };
        mImmediateContext->OMSetRenderTargets(1, renderTarget, 0);

        clearColor = { 0.69f, 0.34f, 0.34f, 1.0f };
        mImmediateContext->ClearRenderTargetView(mRenderTargetView, (float*)&clearColor);
        mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        {
            D3DX11_TECHNIQUE_DESC techDesc;
            mHdrTechnique->GetDesc(&techDesc);
            for (UINT p = 0; p < techDesc.Passes; ++p)
            {
                mHdrTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
                mImmediateContext->Draw(6, 0);
            }
        }
    }


    // Unbind shadow map as a shader input because we are going to render to it next frame.
    // The shadow might might be at any slot, so clear all slots.
    ID3D11ShaderResourceView* nullSRV[16] = { 0 };
    mImmediateContext->PSSetShaderResources(0, 16, nullSRV);
#endif
    mSwapChain->Present(1, 0);


}