#pragma once

#include "SampleBase.hpp"
#include "BasicMath.hpp"

namespace Diligent
{

class Tutorial03_Texturing final : public SampleBase
{
public:
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void Update(double CurrTime, double ElapsedTime, bool DoUpdateUI) override final;

    virtual const Char* GetSampleName() const override final { return "Tutorial03: Texturing"; }

private:
    void CreatePipelineState();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void LoadTexture();

    RefCntAutoPtr<IPipelineState>         m_pPSO;
    RefCntAutoPtr<IBuffer>                m_CubeVertexBuffer;
    RefCntAutoPtr<IBuffer>                m_CubeIndexBuffer;
    RefCntAutoPtr<IBuffer>                m_VSConstants;
    RefCntAutoPtr<ITextureView>           m_TextureSRV;
    RefCntAutoPtr<IShaderResourceBinding> m_SRB;
    float4x4                              m_WorldViewProjMatrix;


};

} // namespace Diligent