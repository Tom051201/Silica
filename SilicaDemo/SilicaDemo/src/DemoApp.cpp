#include "DemoApp.h"

#include <stdexcept>

#include "d3d12/d3dx12/d3dx12.h"

#include "Silica/backends/SilicaImplDX12.h"
#include "Silica/backends/SilicaImplWin32.h"
#include "Silica/include/Renderer.h"

#include "Silica/include/SButton.h"
#include "Silica/include/SHorizontalBox.h"
#include "Silica/include/SVerticalBox.h"
#include "Silica/include/SScissorBox.h"
#include "Silica/include/STextBlock.h"
#include "Silica/include/SEditableText.h"
#include "Silica/include/SScrollBox.h"

inline void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr)) throw std::runtime_error("DX12 Error");
}

bool DemoApp::initialize(HWND hwnd, int width, int height) {
	m_width = width;
	m_height = height;
	m_frameIndex = 0;

	// -- Enable Debug Layer --
	UINT dxgiFactoryFlags = 0;
	#if defined SILICA_DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

	// -- Create Device --
	Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &hardwareAdapter); ++adapterIndex) {
		DXGI_ADAPTER_DESC1 desc;
		hardwareAdapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) {
			break;
		}
	}

	// -- Create Command Queue --
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// -- Create Swap Chain --
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = s_frameCount;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain));
	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// -- Create Descriptor Heaps (RTV) --
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = s_frameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// -- Create Render Targets --
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT n = 0; n < s_frameCount; n++) {
		ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
	}

	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
	ThrowIfFailed(m_commandList->Close());

	// -- Create Synchronization Objects --
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceValues[0] = 0;
	m_fenceValues[1] = 0;
	m_fenceValues[m_frameIndex] = 1;
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// -- Init Silica Backend --
	Silica::ImplDX12_init(m_device.Get(), s_frameCount, DXGI_FORMAT_R8G8B8A8_UNORM);
	Silica::ImplWin32_init(hwnd);

	if (!m_font.loadFromFile("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f)) {
		OutputDebugStringA("Failed to load font!\n");
	}

	ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));

	Silica::ImplDX12_uploadFontAtlas(m_commandList.Get(), m_font.getPixels(), m_font.getWidth(), m_font.getHeight());

	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* ppCommandListsInit[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandListsInit), ppCommandListsInit);
	waitForGpu();

	// -- Build UI Tree --
	m_uiRoot = Silica::MakeWidget<Silica::SBox>({
		.padding = { 20.0f, 20.0f },
		.backgroundColor = 0xFF1E1E1E,
		.child = Silica::MakeWidget<Silica::SVerticalBox>({
			.slots = {
				{
					.padding = { 0.0f, 0.0f },
					.child = Silica::MakeWidget<Silica::SHorizontalBox>({
						.slots = {
							{
								.padding = { 5.0f, 5.0f },
								.child = Silica::MakeWidget<Silica::SButton>({
									.padding = { 30.0f, 15.0f },
									.color = 0xFF3A3A3A,
									.hoverColor = 0xFF505050,
									.pressedColor = 0xFF2A2A2A,
									.onClick = []() {
										OutputDebugStringA("Save Clicked!\n");
										return Silica::EventReply::handled();
									},
									// NEW: Add the TextBlock as the Button's child!
									.child = Silica::MakeWidget<Silica::STextBlock>({
										.text = "Save",
										.font = &m_font
									})
								})
							},
							{
								.padding = { 5.0f, 5.0f },
								.child = Silica::MakeWidget<Silica::SButton>({
									.padding = { 30.0f, 15.0f },
									.color = 0xFF3A3A3A,
									.hoverColor = 0xFF505050,
									.pressedColor = 0xFF2A2A2A,
									.onClick = []() {
										OutputDebugStringA("Compile Clicked!\n");
										return Silica::EventReply::handled();
									},
									// NEW: Add the TextBlock as the Button's child!
									.child = Silica::MakeWidget<Silica::STextBlock>({
										.text = "Compile",
										.font = &m_font
									})
								})
							}
						}
					})
				},

			// ==========================================
			// ROW 2: MAIN CONTENT AREA (CLIPPED)
			// ==========================================
			{
				.padding = { 5.0f, 20.0f }, // Push it down slightly from the toolbar
				.child = Silica::MakeWidget<Silica::SScrollBox>({
					.child = Silica::MakeWidget<Silica::SBox>({
						// This box acts as the background for our scrollable list
						.padding = { 10.0f, 10.0f },
						.backgroundColor = 0xFF252526,
						.child = Silica::MakeWidget<Silica::SVerticalBox>({
							.slots = {
								// Fake "Properties" in the list
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SEditableText>({
										.hintText = "Type your name...",
										.font = &m_font
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
								{
									.padding = { 0.0f, 5.0f },
									.child = Silica::MakeWidget<Silica::SBox>({
										.padding = { 100.0f, 20.0f },
										.backgroundColor = 0xFF333333
									})
								},
							}
						})
					})
				})
			}
		}
	})
	});

	return true;
}

void DemoApp::render() {
	// -- Reset Command Allocator and List --
	ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));

	// -- Transition Back Buffer to Render Target --
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// -- Clear Screen --
	const float clearColor[] = { 0.15f, 0.15f, 0.15f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// -- Set Viewport and Scissor --
	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
	D3D12_RECT scissorRect = { 0, 0, m_width, m_height };
	m_commandList->RSSetViewports(1, &viewport);
	m_commandList->RSSetScissorRects(1, &scissorRect);

	// -- Render UI --
	Silica::ImplDX12_newFrame();
	Silica::Renderer::render(m_uiRoot, (float)m_width, (float)m_height);

	const Silica::DrawList* drawData = Silica::Renderer::getDrawData();
	Silica::ImplDX12_renderDrawData(drawData, m_commandList.Get(), (float)m_width, (float)m_height);

	// -- Transition to Present and Execute --
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &barrier);
	ThrowIfFailed(m_commandList->Close());

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailed(m_swapChain->Present(1, 0));
	moveToNextFrame();
}

void DemoApp::moveToNextFrame() {
	const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex]) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

void DemoApp::cleanup() {
	waitForGpu();

	Silica::ImplDX12_shutdown();
	Silica::ImplWin32_shutdown();

	CloseHandle(m_fenceEvent);
}

void DemoApp::waitForGpu() {
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));
	ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	m_fenceValues[m_frameIndex]++;
}

void DemoApp::resize(int width, int height) {
	if (width == 0 || height == 0) return;

	m_width = width;
	m_height = height;

	waitForGpu();

	for (UINT n = 0; n < s_frameCount; n++) {
		m_renderTargets[n].Reset();
		m_fenceValues[n] = m_fenceValues[m_frameIndex];
	}

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	m_swapChain->GetDesc(&swapChainDesc);
	ThrowIfFailed(m_swapChain->ResizeBuffers(
		s_frameCount,
		width,
		height,
		swapChainDesc.BufferDesc.Format,
		swapChainDesc.Flags
	));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT n = 0; n < s_frameCount; n++) {
		ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}
}
