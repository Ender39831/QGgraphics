#include "GameApp.h"
#include <XUtil.h>
#include <DXTrace.h>
using namespace DirectX;

int i = 0; // 全局变量，表示创建了几个立方体

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
    if (!D3DApp::Init())
        return false;

    m_TextureManager.Init(m_pd3dDevice.Get());
    m_ModelManager.Init(m_pd3dDevice.Get());

    // 务必先初始化所有渲染状态，以供下面的特效使用
    RenderStates::InitAll(m_pd3dDevice.Get());

    if (!m_BasicEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_SkyboxEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!InitResource())
        return false;

    return true;
}

void GameApp::OnResize()
{

    D3DApp::OnResize();
    
    m_pDepthTexture = std::make_unique<Depth2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight);
    m_pDepthTexture->SetDebugObjectName("DepthTexture");

    // 摄像机变更显示
    if (m_pCamera != nullptr)
    {
        m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
        m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        m_BasicEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
        m_SkyboxEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
    }
}

void GameApp::UpdateScene(float dt)
{
    m_CameraController.Update(dt);

    // 拾取
    // 获取键鼠输入(一开始以为用的mouse和keyboard，弄了半天搞不明白才反应过来这里用的ImGui)
    ImGuiIO& io = ImGui::GetIO();
    // 获取鼠标在屏幕上位置
    ImVec2 mousePos = ImGui::GetMousePos();
    mousePos.x = std::clamp(mousePos.x, 0.0f, m_ClientWidth - 1.0f);
    mousePos.y = std::clamp(mousePos.y, 0.0f, m_ClientHeight - 1.0f);
    // 生成射线
    Ray ray = Ray::ScreenToRay(*m_pCamera, mousePos.x, mousePos.y);

    // 判断是否碰撞
    bool hitObject = false;
    std::string pick = "None";
    if (ray.Hit(m_Sphere.GetBoundingOrientedBox()))
    {
        pick = "Sphere";
        hitObject = true;
    }
    else if (ray.Hit(m_Ground.GetBoundingOrientedBox()))
    {
        pick = "Ground";
        hitObject = true;
    }
    else if (ray.Hit(m_Cylinder.GetBoundingOrientedBox()))
    {
        pick = "Cylinder";
        hitObject = true;
    }

    for (int m = 0; m < i; m++)
    {
        if (ray.Hit(m_Box[m].GetBoundingOrientedBox()))
        {
            pick = "Cube";
            hitObject = true;
        }
    }

    // 放置物体
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        // 获取鼠标的指向位置
        XMFLOAT3 putPos = m_pCamera->GetPosition();
        XMFLOAT3 putDir = m_pCamera->GetLookAxis();

        // 各种设置
        Model* createModel = m_ModelManager.CreateFromGeometry("box", Geometry::CreateBox());
        createModel->SetDebugObjectName("createModel");
        m_TextureManager.CreateFromFile("..\\Texture\\bricks.dds");
        createModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\bricks.dds");
        createModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
        createModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f));
        createModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        createModel->materials[0].Set<float>("$SpecularPower", 12.0f);
        createModel->materials[0].Set<XMFLOAT4>("$ReflectColor", XMFLOAT4());
        m_Box[i].SetModel(createModel);
        m_Box[i].GetTransform().SetPosition(putDir.x * 5.0f + putPos.x, putDir.y * 5.0f + putPos.y, putDir.z * 5.0f + putPos.z);

        i++;
        if (i >= 39)//判断是否超出最大值
            i = 0;
    }

    // F销毁方块，不知道为啥不太灵敏
    //if (ImGui::IsKeyPressed(ImGuiKey_F))
    // 还是用右键吧
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        for (int k = 0; k < i; k++)
        {
            if (ray.Hit(m_Box[k].GetBoundingOrientedBox()) && k != i)
            {
                m_Box[k] = m_Box[k + 1]; //把删除正方体后的正方体往前移
                i--;
            }
        }
    }

    // 更新观察矩阵
    m_BasicEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_BasicEffect.SetEyePos(m_pCamera->GetPosition());

    m_SkyboxEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());

    if (ImGui::Begin("Static Cube Mapping"))
    {
        ImGui::Text("Current Object: %s", pick.c_str());

        static int skybox_item = 0;
        static const char* skybox_strs[] = {
            "Daylight",
            "Sunset",
            "illness"
        };
        if (ImGui::Combo("Skybox", &skybox_item, skybox_strs, ARRAYSIZE(skybox_strs)))
        {
            Model* pModel = m_ModelManager.GetModel("Skybox");
            switch (skybox_item)
            {
            case 0: 
                m_BasicEffect.SetTextureCube(m_TextureManager.GetTexture("Daylight"));
                pModel->materials[0].Set<std::string>("$Skybox", "Daylight");
                break;
            case 1: 
                m_BasicEffect.SetTextureCube(m_TextureManager.GetTexture("Sunset"));
                pModel->materials[0].Set<std::string>("$Skybox", "Sunset");
                break;
            case 2: 
                m_BasicEffect.SetTextureCube(m_TextureManager.GetTexture("illness")); 
                pModel->materials[0].Set<std::string>("$Skybox", "illness");
                break;
            }
        }
    }
    ImGui::End();
    ImGui::Render();
}

void GameApp::DrawScene()
{
    

    // 创建后备缓冲区的渲染目标视图
    if (m_FrameCount < m_BackBufferCount)
    {
        ComPtr<ID3D11Texture2D> pBackBuffer;
        m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(pBackBuffer.GetAddressOf()));
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), &rtvDesc, m_pRenderTargetViews[m_FrameCount].ReleaseAndGetAddressOf());
    }

    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pd3dImmediateContext->ClearRenderTargetView(GetBackBufferRTV(), black);
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthTexture->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    ID3D11RenderTargetView* pRTVs[1] = { GetBackBufferRTV() };
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthTexture->GetDepthStencil());
    D3D11_VIEWPORT viewport = m_pCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);

    // 绘制模型
    m_BasicEffect.SetRenderDefault();
    m_BasicEffect.SetReflectionEnabled(true);
    m_Sphere.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    m_BasicEffect.SetReflectionEnabled(false);
    m_Ground.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    m_Cylinder.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    // 绘制创建的立方体
    for (int temp = 0; temp < i; temp++)
    {
        m_Box[temp].Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    }

    // 绘制天空盒
    m_SkyboxEffect.SetRenderDefault();
    m_Skybox.Draw(m_pd3dImmediateContext.Get(), m_SkyboxEffect);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));

    
}

bool GameApp::InitResource()
{

    // ******************
    // 初始化天空盒相关
    
    ComPtr<ID3D11Texture2D> pTex;
    D3D11_TEXTURE2D_DESC texDesc;
    std::string filenameStr;
    std::vector<ID3D11ShaderResourceView*> pCubeTextures;
    std::unique_ptr<TextureCube> pTexCube;
    // Daylight
    {
        filenameStr = "..\\Texture\\daylight0.png";
        for (size_t i = 0; i < 6; ++i)
        {
            filenameStr[19] = '0' + (char)i;
            pCubeTextures.push_back(m_TextureManager.CreateFromFile(filenameStr));
        }

        pCubeTextures[0]->GetResource(reinterpret_cast<ID3D11Resource**>(pTex.ReleaseAndGetAddressOf()));
        pTex->GetDesc(&texDesc);
        pTexCube = std::make_unique<TextureCube>(m_pd3dDevice.Get(), texDesc.Width, texDesc.Height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        pTexCube->SetDebugObjectName("Daylight");
        for (uint32_t i = 0; i < 6; ++i)
        {
            pCubeTextures[i]->GetResource(reinterpret_cast<ID3D11Resource**>(pTex.ReleaseAndGetAddressOf()));
            m_pd3dImmediateContext->CopySubresourceRegion(pTexCube->GetTexture(), D3D11CalcSubresource(0, i, 1), 0, 0, 0, pTex.Get(), 0, nullptr);
        }
        m_TextureManager.AddTexture("Daylight", pTexCube->GetShaderResource());
    }
    
    // Sunset
    {
        filenameStr = "..\\Texture\\sunset0.bmp";
        pCubeTextures.clear();
        for (size_t i = 0; i < 6; ++i)
        {
            filenameStr[17] = '0' + (char)i;
            pCubeTextures.push_back(m_TextureManager.CreateFromFile(filenameStr));
        }
        pCubeTextures[0]->GetResource(reinterpret_cast<ID3D11Resource**>(pTex.ReleaseAndGetAddressOf()));
        pTex->GetDesc(&texDesc);
        pTexCube = std::make_unique<TextureCube>(m_pd3dDevice.Get(), texDesc.Width, texDesc.Height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        pTexCube->SetDebugObjectName("Sunset");
        for (uint32_t i = 0; i < 6; ++i)
        {
            pCubeTextures[i]->GetResource(reinterpret_cast<ID3D11Resource**>(pTex.ReleaseAndGetAddressOf()));
            m_pd3dImmediateContext->CopySubresourceRegion(pTexCube->GetTexture(), D3D11CalcSubresource(0, i, 1), 0, 0, 0, pTex.Get(), 0, nullptr);
        }
        m_TextureManager.AddTexture("Sunset", pTexCube->GetShaderResource());
    }
    
    // Desert
    //m_TextureManager.AddTexture("Desert", m_TextureManager.CreateFromFile("..\\Texture\\desertcube1024.dds", false, true));
    
    // illness
    m_TextureManager.AddTexture("illness", m_TextureManager.CreateFromFile("..\\Texture\\Texture_illness.dds",false,true));

    m_BasicEffect.SetTextureCube(m_TextureManager.GetTexture("Daylight"));
    
    // ******************
    // 初始化游戏对象
    //
    
    // 球体
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Sphere", Geometry::CreateSphere());
        pModel->SetDebugObjectName("Sphere");
        m_TextureManager.CreateFromFile("..\\Texture\\stone.dds");
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\stone.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);
        pModel->materials[0].Set<XMFLOAT4>("$ReflectColor", XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
        m_Sphere.SetModel(pModel);
    }
    // 地面
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Ground", Geometry::CreatePlane(XMFLOAT2(10.0f, 10.0f), XMFLOAT2(5.0f, 5.0f)));
        pModel->SetDebugObjectName("Ground");
        m_TextureManager.CreateFromFile("..\\Texture\\floor.dds");
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\floor.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);
        pModel->materials[0].Set<XMFLOAT4>("$ReflectColor", XMFLOAT4());
        m_Ground.SetModel(pModel);
        m_Ground.GetTransform().SetPosition(0.0f, -3.0f, 0.0f);
    }	
    // 柱体
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Cylinder", Geometry::CreateCylinder(0.5f, 2.0f));
        pModel->SetDebugObjectName("Cylinder");
        m_TextureManager.CreateFromFile("..\\Texture\\bricks.dds");
        pModel->materials[0].Set<std::string>("$Diffuse", "..\\Texture\\bricks.dds");
        pModel->materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f));
        pModel->materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
        pModel->materials[0].Set<float>("$SpecularPower", 16.0f);
        pModel->materials[0].Set<XMFLOAT4>("$ReflectColor", XMFLOAT4());
        m_Cylinder.SetModel(pModel);
        m_Cylinder.GetTransform().SetPosition(0.0f, -1.99f, 0.0f);
    }
    // 天空盒立方体
    Model* pModel = m_ModelManager.CreateFromGeometry("Skybox", Geometry::CreateBox());
    pModel->SetDebugObjectName("Skybox");
    pModel->materials[0].Set<std::string>("$Skybox", "Daylight");
    m_Skybox.SetModel(pModel);
    // ******************
    // 初始化摄像机
    //
    auto camera = std::make_shared<FirstPersonCamera>();
    m_pCamera = camera;
    m_CameraController.InitCamera(camera.get());
    camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
    camera->LookTo(XMFLOAT3(0.0f, 0.0f, -10.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

    m_BasicEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_BasicEffect.SetProjMatrix(camera->GetProjMatrixXM());
    m_SkyboxEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_SkyboxEffect.SetProjMatrix(camera->GetProjMatrixXM());



    // ******************
    // 初始化不会变化的值
    //

    // 方向光
    DirectionalLight dirLight[4]{};
    dirLight[0].ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
    dirLight[0].diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    dirLight[0].specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
    dirLight[0].direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
    dirLight[1] = dirLight[0];
    dirLight[1].direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
    dirLight[2] = dirLight[0];
    dirLight[2].direction = XMFLOAT3(0.577f, -0.577f, -0.577f);
    dirLight[3] = dirLight[0];
    dirLight[3].direction = XMFLOAT3(-0.577f, -0.577f, -0.577f);
    for (int i = 0; i < 4; ++i)
        m_BasicEffect.SetDirLight(i, dirLight[i]);

    return true;
}

