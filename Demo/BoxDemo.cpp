#include "BoxDemo.h"

BoxDemo::BoxDemo(HINSTANCE instance, UINT clientWidth, UINT clientHeight, const char* windowCaption, bool enable4xMsaa)
    : Ruby::App(instance, clientWidth, clientHeight, windowCaption, enable4xMsaa),
    mVertexBuffer(nullptr),
    mIndexBuffer(nullptr),
    mEffect(nullptr),
    mTechnique(nullptr),
    mFxWorldViewProj(nullptr),
    mInputLayout(nullptr)
{
    XMMATRIX identity = XMMatrixIdentity();
    XMStoreFloat4x4(&mWorld, identity);
    XMStoreFloat4x4(&mView, identity);
    XMStoreFloat4x4(&mProj, identity);
}

BoxDemo::~BoxDemo()
{
    SAFE_RELEASE(mVertexBuffer);
    SAFE_RELEASE(mIndexBuffer);
    SAFE_RELEASE(mEffect);
    SAFE_RELEASE(mInputLayout);
}

bool BoxDemo::Init()
{
    if (!Ruby::App::Init())
        return false;

    // Create the data for the cube
    Ruby::GeometryGenerator generator;
    generator.CreateBox(2, 2, 2, mCubeData);

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Ruby::Vertex) * mCubeData.Vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    vbd.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = mCubeData.Vertices.data();
    HRESULT result = mDevice->CreateBuffer(&vbd, &vinitData, &mVertexBuffer);
    if (FAILED(result))
    {
        MessageBox(0, "Error: failed loading vertex data", 0, 0);
        return false;
    }

    D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(USHORT) * mCubeData.Indices.size();
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    ibd.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = mCubeData.Indices.data();
    result = mDevice->CreateBuffer(&ibd, &iinitData, &mIndexBuffer);
    if (FAILED(result))
    {
        MessageBox(0, "Error: falied loading index data", 0, 0);
        return false;
    }

    // create the shaders and fx
    DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
    shaderFlags |= D3D10_SHADER_DEBUG;
    shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

    ID3D10Blob* compiledShader = 0;
    ID3D10Blob* compilationMsgs = 0;
    result = D3DX11CompileFromFile("./FX/color.fx", 0, 0, 0, "fx_5_0", shaderFlags, 0, 0, &compiledShader, &compilationMsgs, 0);

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
    mFxWorldViewProj = mEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
    mFxWorld = mEffect->GetVariableByName("gWorld")->AsMatrix();


    // set the vertex Layout
    D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    D3DX11_PASS_DESC passDesc;
    mTechnique->GetPassByIndex(0)->GetDesc(&passDesc);
    result = mDevice->CreateInputLayout(vertexDesc, 4, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayout);

    // set proj matrices
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);

    // Build the view matrix.
    XMVECTOR pos = XMVectorSet(0, 0, -10.0f, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, V);

    return true;
}

void BoxDemo::OnResize()
{
    Ruby::App::OnResize();
    // set proj matrices
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 0.01f, 100.0f);
    XMStoreFloat4x4(&mProj, P);
}

void BoxDemo::UpdateScene()
{
    static float angle = 0.0f;
    XMMATRIX world = XMMatrixRotationY(angle) * XMMatrixRotationX(angle * 2.0f);
    XMStoreFloat4x4(&mWorld, world);
    angle += 1.0f * mTimer.DeltaTime();
}

void BoxDemo::DrawScene()
{
    XMVECTORF32 clearColor = { 0.69f, 0.77f, 0.87f, 1.0f };
    mImmediateContext->ClearRenderTargetView(mRenderTargetView, (float*)&clearColor);
    mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    mImmediateContext->IASetInputLayout(mInputLayout);
    mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = sizeof(Ruby::Vertex);
    UINT offet = 0;
    mImmediateContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offet);
    mImmediateContext->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    // Set constants
    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world * view * proj;

    mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
    mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));

    D3DX11_TECHNIQUE_DESC techDesc;
    mTechnique->GetDesc(&techDesc);
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
        mImmediateContext->DrawIndexed(mCubeData.Indices.size(), 0, 0);
    }

    mSwapChain->Present(1, 0);
}