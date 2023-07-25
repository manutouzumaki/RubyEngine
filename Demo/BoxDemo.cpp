#include "BoxDemo.h"

using namespace DirectX;

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
    for (int i = 0; i < PARTICLE_COUNT; ++i)
    {
        XMStoreFloat4x4(&mWorld[i], identity);
    }
    XMStoreFloat4x4(&mView, identity);
    XMStoreFloat4x4(&mProj, identity);
}

BoxDemo::~BoxDemo()
{
    SAFE_DELETE(mCamera);

    for (int i = 0; i < PARTICLE_COUNT; ++i)
    {
        SAFE_DELETE(mParticle[i]);
    }

    SAFE_DELETE(mGravityFG);
    SAFE_DELETE(mDragFG);
    SAFE_DELETE(mForceRegistry);

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
    //generator.CreateBox(2, 2, 2, mCubeData);
    generator.CreateSphere(0.5f, 16, 16, mCubeData);

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

    mCamera = new Ruby::FPSCamera(XMFLOAT3(0, 0, -10), XMFLOAT3(0, 0, 0), 4);

    mParticle[0] = new Ruby::Physics::Particle();
    mParticle[0]->SetPosition(1, 0, 0);
    mParticle[0]->SetMass(1.0);
    mParticle[0]->SetDamping(0.999);

    mParticle[1] = new Ruby::Physics::Particle();
    mParticle[1]->SetPosition(-1, 0, 0);
    mParticle[1]->SetMass(1.0);
    mParticle[1]->SetDamping(0.3);

    Ruby::Physics::Vector3 gravity(0.0, -10.0, 0.0);
    mGravityFG = new Ruby::Physics::ParticleGravity(gravity);
    mDragFG = new Ruby::Physics::ParticleDrag(0.5, 0.5);
    mForceRegistry = new Ruby::Physics::ParticleForceRegistry();

    mForceRegistry->Add(mParticle[0], mGravityFG);
    mForceRegistry->Add(mParticle[0], mDragFG);

    mForceRegistry->Add(mParticle[1], mGravityFG);
    mForceRegistry->Add(mParticle[1], mDragFG);

    for (int i = 0; i < PARTICLE_COUNT; ++i)
    {
        Ruby::Physics::Vector3 pos = mParticle[i]->GetPosition();
        XMMATRIX world = XMMatrixTranslation(pos.x, pos.y, pos.z);
        XMStoreFloat4x4(&mWorld[i], world);
    }

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
    // Camera Update code ...
    {
        float dt = mTimer.DeltaTime();

        if (mInput.KeyIsDown('W'))
        {
            mCamera->MoveForward(dt);
        }
        if (mInput.KeyIsDown('S'))
        {
            mCamera->MoveBackward(dt);
        }
        if (mInput.KeyIsDown('A'))
        {
            mCamera->MoveLeft(dt);
        }
        if (mInput.KeyIsDown('D'))
        {
            mCamera->MoveRight(dt);
        }
        if (mInput.KeyIsDown('R'))
        {
            mCamera->MoveUp(dt);
        }
        if (mInput.KeyIsDown('F'))
        {
            mCamera->MoveDown(dt);
        }

        if (mInput.MouseButtonJustDown(1))
        {
            ShowCursor(false);
        }
        if (mInput.MouseButtonJustUp(1))
        {
            ShowCursor(true);
        }

        if (mInput.MouseButtonIsDown(1))
        {
            int deltaX = mInput.MousePosX() - mInput.MouseLastPosX();
            int deltaY = mInput.MousePosY() - mInput.MouseLastPosY();

            mCamera->MouseMove((float)deltaX * 0.001f, (float)deltaY * 0.001f);

            SetCursorPos(mWindowX + mClientWidth / 2, mWindowY + mClientHeight / 2);
            mInput.mCurrent.mouseX = mClientWidth / 2;
            mInput.mCurrent.mouseY = mClientHeight / 2;
            mInput.mLast.mouseX = mClientWidth / 2;
            mInput.mLast.mouseY = mClientHeight / 2;

        }

        mCamera->Update(dt);
        XMStoreFloat4x4(&mView, mCamera->GetView());

    }

    if (mInput.KeyIsDown(VK_SPACE))
    {
        mForceRegistry->UpdateForces(mTimer.DeltaTime());
        for (int i = 0; i < PARTICLE_COUNT; ++i)
        {
            mParticle[i]->Integrate(mTimer.DeltaTime());

            Ruby::Physics::Vector3 pos = mParticle[i]->GetPosition();
            XMMATRIX world = XMMatrixTranslation(pos.x, pos.y, pos.z);
            XMStoreFloat4x4(&mWorld[i], world);
        }
    }



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

    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    D3DX11_TECHNIQUE_DESC techDesc;
    mTechnique->GetDesc(&techDesc);
    for (UINT p = 0; p < techDesc.Passes; ++p)
    {
        for (int i = 0; i < PARTICLE_COUNT; ++i)
        {
            // Set constants
            XMMATRIX world = XMLoadFloat4x4(&mWorld[i]);
            XMMATRIX worldViewProj = world * view * proj;

            mFxWorld->SetMatrix(reinterpret_cast<float*>(&world));
            mFxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));

            mTechnique->GetPassByIndex(p)->Apply(0, mImmediateContext);
            mImmediateContext->DrawIndexed(mCubeData.Indices.size(), 0, 0);
        }
    }

    mSwapChain->Present(1, 0);
}