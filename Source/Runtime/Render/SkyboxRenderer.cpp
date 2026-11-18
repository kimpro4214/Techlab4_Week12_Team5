#include "EnginePCH.h"
#include "SkyboxRenderer.h"

#include "RenderCommand.h"
#include "RenderResourceManager.h"
#include "Components/SkySphereComponent.h"

namespace
{
	// 샘플링 결과가 이미 선형인 포맷인지 판단한다.
	// - _SRGB 포맷: 하드웨어가 샘플링할 때 sRGB -> 선형으로 디코딩한다 (예: Sky.dds = BC7_UNORM_SRGB)
	// - float 포맷: .hdr처럼 원래 선형 값이다
	// 그 외 8bit/BC UNORM 포맷(.jpg/.png, 비-sRGB DDS)은 sRGB 인코딩 값이 그대로 나오므로 셰이더에서 디코딩한다.
	bool IsSampledAsLinear(DXGI_FORMAT Format)
	{
		switch (Format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
			return true;
		default:
			return false;
		}
	}
}

bool FSkyboxRenderer::Init()
{
	Shader = FRenderResourceManager::GetShaderProgram("Resources/Shader/SkyboxShader.hlsl");
	if (!Shader)
	{
		HTR_LOG(Error, "[Skybox] shader not found");
		return false;
	}

	ConstantBuffer = RenderCommand::CreateConstantBuffer(sizeof(FSkyboxConstants));
	if (!ConstantBuffer) return false;

	PipelineState.Shader = Shader;
	PipelineState.Topology = EPrimitiveTopology::TriangleList;
	PipelineState.RasterizerState = ERasterizerState::SolidNone;
	PipelineState.BlendState = EBlendState::Opaque;
	PipelineState.DepthStencilState = EDepthStencilState::Disabled;

	return true;
}

void FSkyboxRenderer::OnRender(const FMatrix& ViewProjection, const FVector& CameraPosition,
	const USkySphereComponent* SkySphere)
{
	FSkyboxConstants Constants;
	FTexture2D* Texture = nullptr;
	if (SkySphere)
	{
		if (const UMaterial* Material = SkySphere->GetBackgroundMaterial())
		{
			// 첫 번째 슬롯을 등장방형 파노라마로 사용하고 BaseColor로 색을 조절한다.
			Constants.BackgroundColor = Material->BaseColor;
			if (Material->Textures.Num() > 0 && Material->Textures[0])
				Texture = Material->Textures[0]->GetResource();
		}
		else
		{
			Constants.BackgroundColor = SkySphere->GetBackgroundColor().ToVector4();
		}
	}

	if (Texture)
	{
		Constants.InverseViewProjection = ViewProjection.Inverse();
		Constants.CameraPosition = CameraPosition;
		Constants.DecodeSRGB = IsSampledAsLinear(Texture->GetFormat()) ? 0.0f : 1.0f;
		Constants.HasPanorama = 1.0f;
	}

	RenderCommand::BindPipelineState(PipelineState);
	RenderCommand::UpdateBufferData(ConstantBuffer.get(), &Constants);
	RenderCommand::BindConstantBuffer(0, ConstantBuffer.get(), EShaderBindFlagBits::Pixel);
	RenderCommand::BindShaderResource(0, Texture, EShaderBindFlagBits::Pixel);
	// 경도(U)가 0<->1에서 감기므로 Clamp가 아니라 Wrap이어야 이음매가 안 보인다.
	RenderCommand::BindSamplerState(0, ESamplerState::LinearWrap, EShaderBindFlagBits::Pixel);

	// 정점 버퍼 없이 3개. VS가 SV_VertexID로 삼각형을 만든다.
	RenderCommand::Draw(3);
}                               
