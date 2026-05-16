#include "SilicaImplDX12.h"

#include <d3dcompiler.h>
#include <wrl/client.h>
#include <stdexcept>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "d3d12/d3dx12/d3dx12.h"

using Microsoft::WRL::ComPtr;

namespace Silica {

	struct BackendStateDX12 {
		ID3D12Device* device = nullptr;
		UINT numFramesInFlight = 0;
		UINT frameIndex = 0;

		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12PipelineState> pipelineState;

		ComPtr<ID3D12Resource> vertexBuffer;
		ComPtr<ID3D12Resource> indexBuffer;

		Vertex* vertexData = nullptr;
		uint32_t* indexData = nullptr;

		uint32_t maxVertices = 10000;
		uint32_t maxIndices = maxVertices * 3;
	};

	static BackendStateDX12 g_state;



	// ----- Helper To Compile Shader -----
	static ComPtr<ID3DBlob> compileShader(const char* shaderCode, const char* entryPoint, const char* target) {
		ComPtr<ID3DBlob> shaderBlob;
		ComPtr<ID3DBlob> errorBlob;
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

		HRESULT hr = D3DCompile(
			shaderCode,
			strlen(shaderCode),
			nullptr, nullptr, nullptr,
			entryPoint,
			target,
			flags,
			0,
			&shaderBlob,
			&errorBlob
		);

		if (FAILED(hr)) {
			if (errorBlob) {
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			}
			throw std::runtime_error("Failed to compile Silica UI Shader!");
		}

		return shaderBlob;
	}



	// ----- Init Silica DX12 Backend -----
	bool ImplDX12_init(ID3D12Device* device, UINT numFramesInFlight, DXGI_FORMAT rtvFormat) {
		g_state.device = device;
		g_state.numFramesInFlight = numFramesInFlight;
		g_state.frameIndex = 0;


		// -- Create Root Signature --
		CD3DX12_ROOT_PARAMETER rootParameters[1];
		rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.NumParameters = _countof(rootParameters);
		rootSigDesc.pParameters = rootParameters;
		rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&g_state.rootSignature));


		// -- Create HLSL Shader Code --
		const char* shaderSource = R"(
			cbuffer RootConstants : register(b0) {
				float4x4 ProjectionMatrix;
			};
			struct VS_INPUT {
				float2 pos : POSITION;
				float2 uv  : TEXCOORD0;
				uint color : COLOR0;
			};
			struct PS_INPUT {
				float4 pos : SV_POSITION;
				float4 col : COLOR0;
			};
			PS_INPUT VSMain(VS_INPUT input) {
				PS_INPUT output;
				output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.0f, 1.0f));
				
				output.col = float4((input.color & 0xFF) / 255.0f,
									((input.color >> 8) & 0xFF) / 255.0f,
									((input.color >> 16) & 0xFF) / 255.0f,
									((input.color >> 24) & 0xFF) / 255.0f);
				return output;
			}

			float4 PSMain(PS_INPUT input) : SV_Target {
				return input.col;
			}
		)";

		ComPtr<ID3DBlob> vsBlob = compileShader(shaderSource, "VSMain", "vs_5_0");
		ComPtr<ID3DBlob> psBlob = compileShader(shaderSource, "PSMain", "ps_5_0");


		// -- Input Layout --
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32_UINT,     0, offsetof(Vertex, color), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};


		// -- Create Pipeline State Object (PSO) --
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = g_state.rootSignature.Get();
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = rtvFormat;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DepthStencilState.DepthEnable = FALSE;

		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_state.pipelineState));


		// -- Create Dynamic Upload Buffers --
		uint32_t vbSize = g_state.maxVertices * sizeof(Vertex) * numFramesInFlight;
		uint32_t ibSize = g_state.maxIndices * sizeof(uint32_t) * numFramesInFlight;

		D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
		D3D12_RESOURCE_DESC vbDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, vbSize, 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
		D3D12_RESOURCE_DESC ibDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, ibSize, 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

		device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_state.vertexBuffer));
		device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &ibDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_state.indexBuffer));

		g_state.vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&g_state.vertexData));
		g_state.indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&g_state.indexData));


		return true;
	}



	// ----- Shutdown Silica DX12 Backend -----
	void ImplDX12_shutdown() {
		if (g_state.vertexBuffer) g_state.vertexBuffer->Unmap(0, nullptr);
		if (g_state.indexBuffer) g_state.indexBuffer->Unmap(0, nullptr);

		g_state.vertexBuffer.Reset();
		g_state.indexBuffer.Reset();
		g_state.pipelineState.Reset();
		g_state.rootSignature.Reset();
		g_state.device = nullptr;
	}



	// ----- Advance Frame Index --
	void ImplDX12_newFrame() {
		g_state.frameIndex = (g_state.frameIndex + 1) % g_state.numFramesInFlight;
	}



	// ----- Do The Rendering -----
	void ImplDX12_renderDrawData(const DrawList* drawData, ID3D12GraphicsCommandList* commandList, float screenWidth, float screenHeight) {
		if (!drawData || drawData->vertices.empty() || drawData->indices.empty()) return;

		size_t vertexCount = drawData->vertices.size();
		size_t indexCount = drawData->indices.size();

		// -- Calculate Offset for this Frame --
		uint32_t vbOffsetBytes = g_state.frameIndex * g_state.maxVertices * sizeof(Vertex);
		uint32_t ibOffsetBytes = g_state.frameIndex * g_state.maxIndices * sizeof(uint32_t);

		// -- Copy Data --
		memcpy((uint8_t*)g_state.vertexData + vbOffsetBytes, drawData->vertices.data(), vertexCount * sizeof(Vertex));
		memcpy((uint8_t*)g_state.indexData + ibOffsetBytes, drawData->indices.data(), indexCount * sizeof(uint32_t));

		// -- Set Pipeline and Root Signature --
		commandList->SetPipelineState(g_state.pipelineState.Get());
		commandList->SetGraphicsRootSignature(g_state.rootSignature.Get());

		// -- Calculate Orthographic Projection --
		float L = 0.0f;
		float R = screenWidth;
		float T = 0.0f;
		float B = screenHeight;
		float mvp[4][4] = {
			{ 2.0f / (R - L),		0.0f,						0.0f,			0.0f },
			{ 0.0f,					2.0f / (T - B),				0.0f,			0.0f },
			{ 0.0f,					0.0f,						0.5f,			0.0f },
			{ (L + R) / (L - R),	(T + B) / (B - T),			0.5f,			1.0f },
		};

		commandList->SetGraphicsRoot32BitConstants(0, 16, &mvp[0][0], 0);

		// -- Bind Vertex And Index Buffers --
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = g_state.vertexBuffer->GetGPUVirtualAddress() + vbOffsetBytes;
		vbv.SizeInBytes = (UINT)(vertexCount * sizeof(Vertex));
		vbv.StrideInBytes = sizeof(Vertex);

		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = g_state.indexBuffer->GetGPUVirtualAddress() + ibOffsetBytes;
		ibv.SizeInBytes = (UINT)(indexCount * sizeof(uint32_t));
		ibv.Format = DXGI_FORMAT_R32_UINT;

		commandList->IASetVertexBuffers(0, 1, &vbv);
		commandList->IASetIndexBuffer(&ibv);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// -- Draw Commands --
		for (const auto& cmd : drawData->commands) {
			commandList->DrawIndexedInstanced(
				cmd.indexCount,
				1,
				cmd.startIndex,
				cmd.vertexOffset,
				0
			);
		}
	}

}
