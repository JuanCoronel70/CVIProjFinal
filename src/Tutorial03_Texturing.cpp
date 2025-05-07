#include "Tutorial03_Texturing.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "ColorConversion.h"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new Tutorial03_Texturing();
}

void Tutorial03_Texturing::CreatePipelineState()
{
    // Pipeline state object encompasses configuration of all GPU stages

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSOCreateInfo.PSODesc.Name = "Cube PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off
    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // Cull back faces
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;
    // Enable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    // clang-format on

    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.Desc.UseCombinedTextureSamplers = true;

    // Pack matrices in row-major order
    ShaderCI.CompileFlags = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    // Presentation engine always expects input in gamma space. Normally, pixel shader output is
    // converted from linear to gamma space by the GPU. However, some platforms (e.g. Android in GLES mode,
    // or Emscripten in WebGL mode) do not support gamma-correction. In this case the application
    // has to do the conversion manually.
    ShaderMacro Macros[] = {{"CONVERT_PS_OUTPUT_TO_GAMMA", m_ConvertPSOutputToGamma ? "1" : "0"}};
    ShaderCI.Macros      = {Macros, _countof(Macros)};

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube VS";
        ShaderCI.FilePath        = "water.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVS);
        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU
        struct VSConstants
        {
            float4x4 WorldViewProj;
            float    Time;
            float    WaveHeight;
            float    WaveFreq;
            float    WaveSpeed;
        };
        CreateUniformBuffer(m_pDevice, sizeof(VSConstants), "VS constants CB", &m_VSConstants);
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube PS";
        ShaderCI.FilePath        = "water.psh";
        m_pDevice->CreateShader(ShaderCI, &pPS);
    }

    // clang-format off
    // Define vertex shader input layout
    LayoutElement LayoutElems[] =
    {
        // Attribute 0 - vertex position
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        // Attribute 1 - texture coordinates
        LayoutElement{1, 0, 2, VT_FLOAT32, False}
    };
    // clang-format on

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // clang-format off
    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    // clang-format off
    // Define immutable sampler for g_Texture. Immutable samplers should be used whenever possible
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Texture", SamLinearClampDesc}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    // Since we did not explicitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables
    // never change and are bound directly through the pipeline state object.
    m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);

    // Since we are using mutable variable, we must create a shader resource binding object
    // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
    m_pPSO->CreateShaderResourceBinding(&m_SRB, true);
}

void Tutorial03_Texturing::CreateVertexBuffer()
{
    // Layout of this structure matches the one we defined in the pipeline state
    struct Vertex
    {
        float3 pos;
        float2 uv;
    };

    // Definimos las dimensiones del plano
    const float PlaneWidth  = 12.0f; // Ancho total del plano (-1 a +1 en X)
    const float PlaneLength = 12.0f; // Largo total del plano (-1 a +1 en Z)

    // Definimos la resolución del plano (número de divisiones)
    const Uint32 VertexCountX = 100; // Número de vértices en el eje X
    const Uint32 VertexCountZ = 100; // Número de vértices en el eje Z

    // Calculamos el número total de vértices
    const Uint32 TotalVertexCount = VertexCountX * VertexCountZ;

    // Creamos un arreglo dinámico para almacenar los vértices
    std::vector<Vertex> PlaneVerts(TotalVertexCount);

    // Calculamos el paso entre vértices
    const float StepX = PlaneWidth / (VertexCountX - 1);
    const float StepZ = PlaneLength / (VertexCountZ - 1);

    // Generamos los vértices del plano
    for (Uint32 z = 0; z < VertexCountZ; ++z)
    {
        for (Uint32 x = 0; x < VertexCountX; ++x)
        {
            Uint32 index = z * VertexCountX + x;

            // Posición del vértice
            float xPos = -PlaneWidth / 2.0f + x * StepX;
            float zPos = -PlaneLength / 2.0f + z * StepZ;

            // Coordenadas de textura
            float u = static_cast<float>(x) / (VertexCountX - 1);
            float v = static_cast<float>(z) / (VertexCountZ - 1);

            // Almacenamos el vértice
            PlaneVerts[index].pos = float3{xPos, 0.0f, zPos}; // Y = 0 para un plano horizontal
            PlaneVerts[index].uv  = float2{u, v};
        }
    }

    // Creamos el buffer de vértices
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name      = "Plane vertex buffer";
    VertBuffDesc.Usage     = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size      = TotalVertexCount * sizeof(Vertex);

    BufferData VBData;
    VBData.pData    = PlaneVerts.data();
    VBData.DataSize = TotalVertexCount * sizeof(Vertex);

    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &m_CubeVertexBuffer);
}


void Tutorial03_Texturing::CreateIndexBuffer()
{
    // Definimos la resolución del plano
    const Uint32 VertexCountX = 100;
    const Uint32 VertexCountZ = 100;

    // Calculamos el número de celdas en cada dirección
    const Uint32 CellCountX = VertexCountX - 1;
    const Uint32 CellCountZ = VertexCountZ - 1;

    // Cada celda requiere 2 triángulos, cada triángulo tiene 3 índices
    const Uint32 TotalIndexCount = CellCountX * CellCountZ * 6;

    // Creamos un arreglo dinámico para almacenar los índices
    std::vector<Uint32> Indices(TotalIndexCount);

    // Generamos los índices para los triángulos del plano
    Uint32 IndexOffset = 0;

    for (Uint32 z = 0; z < CellCountZ; ++z)
    {
        for (Uint32 x = 0; x < CellCountX; ++x)
        {
            // Calculamos los índices de los cuatro vértices de la celda
            Uint32 BottomLeft  = z * VertexCountX + x;
            Uint32 BottomRight = BottomLeft + 1;
            Uint32 TopLeft     = (z + 1) * VertexCountX + x;
            Uint32 TopRight    = TopLeft + 1;

            // Primer triángulo: BottomLeft -> TopLeft -> TopRight
            Indices[IndexOffset++] = BottomLeft;
            Indices[IndexOffset++] = TopLeft;
            Indices[IndexOffset++] = TopRight;

            // Segundo triángulo: BottomLeft -> TopRight -> BottomRight
            Indices[IndexOffset++] = BottomLeft;
            Indices[IndexOffset++] = TopRight;
            Indices[IndexOffset++] = BottomRight;
        }
    }

    // Creamos el buffer de índices
    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = "Plane index buffer";
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = TotalIndexCount * sizeof(Uint32);

    BufferData IBData;
    IBData.pData    = Indices.data();
    IBData.DataSize = TotalIndexCount * sizeof(Uint32);

    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_CubeIndexBuffer);
}


void Tutorial03_Texturing::LoadTexture()
{
    TextureLoadInfo loadInfo;
    loadInfo.IsSRGB = true;
    RefCntAutoPtr<ITexture> Tex;
    CreateTextureFromFile("water.png", loadInfo, m_pDevice, &Tex);
    // Get shader resource view from the texture
    m_TextureSRV = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    // Set texture SRV in the SRB
    m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TextureSRV);
}


void Tutorial03_Texturing::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    CreatePipelineState();
    CreateVertexBuffer();
    CreateIndexBuffer();
    LoadTexture();
}

// Render a frame
void Tutorial03_Texturing::Render()
{
    ITextureView* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    ITextureView* pDSV = m_pSwapChain->GetDepthBufferDSV();
    // Clear the back buffer
    float4 ClearColor = {0.350f, 0.350f, 0.350f, 1.0f};
    if (m_ConvertPSOutputToGamma)
    {
        // If manual gamma correction is required, we need to clear the render target with sRGB color
        ClearColor = LinearToSRGB(ClearColor);
    }
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor.Data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    {
        // Map the buffer and write current world-view-projection matrix and wave animation parameters
        struct VSConstants
        {
            float4x4 WorldViewProj;
            float    Time;
            float    WaveHeight;
            float    WaveFreq;
            float    WaveSpeed;
        };

        MapHelper<VSConstants> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        CBConstants->WorldViewProj = m_WorldViewProjMatrix;
        CBConstants->Time          = m_Time; // Tiempo actualizado en método Update()
        CBConstants->WaveHeight    = 0.3f;   // Altura de las ondas
        CBConstants->WaveFreq      = 0.4f;   // Frecuencia de las ondas
        CBConstants->WaveSpeed     = 1.5f;   // Velocidad de las ondas
    }

    // Bind vertex and index buffers
    const Uint64 offset   = 0;
    IBuffer*     pBuffs[] = {m_CubeVertexBuffer};
    m_pImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    m_pImmediateContext->SetIndexBuffer(m_CubeIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // Set the pipeline state
    m_pImmediateContext->SetPipelineState(m_pPSO);
    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.
    m_pImmediateContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawIndexedAttribs DrawAttrs;    // This is an indexed draw call
    DrawAttrs.IndexType = VT_UINT32; // Index type

    // Actualizar el número de índices
    // Para un plano de resolución 100x100, tenemos 99*99*2 triángulos (99*99*6 índices)
    const Uint32 VertexCountX = 100;
    const Uint32 VertexCountZ = 100;
    const Uint32 CellCountX   = VertexCountX - 1;
    const Uint32 CellCountZ   = VertexCountZ - 1;
    DrawAttrs.NumIndices      = CellCountX * CellCountZ * 6;

    // Verify the state of vertex and index buffers
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    m_pImmediateContext->DrawIndexed(DrawAttrs);
}

void Tutorial03_Texturing::Update(double CurrTime, double ElapsedTime, bool DoUpdateUI)
{
    SampleBase::Update(CurrTime, ElapsedTime, DoUpdateUI);

    // Actualizar el tiempo para la animación del líquido
    m_Time += static_cast<float>(ElapsedTime);

    float4x4 PlaneModelTransform = float4x4::RotationX(-PI_F * 0.15f); // Rotar para ver mejor el plano

    // Ajustar la cámara para ver el plano desde arriba
    float4x4 View = float4x4::Translation(0.f, 2.0f, 20.0f);

    // Get pretransform matrix that rotates the scene according the surface orientation
    float4x4 SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

    // Get projection matrix adjusted to the current screen orientation
    float4x4 Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);

    // Compute world-view-projection matrix
    m_WorldViewProjMatrix = PlaneModelTransform * View * SrfPreTransform * Proj;
}
} // namespace Diligent
