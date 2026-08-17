#include "DemoApp.h"

#include <stdexcept>
#include <algorithm>

#include "d3d12/d3dx12/d3dx12.h"

#include "Silica/backends/SilicaImplDX12.h"
#include "Silica/backends/SilicaImplWin32.h"
#include "Silica/include/Renderer.h"
#include "Silica/include/Theme.h"

#include "Silica/include/SButton.h"
#include "Silica/include/SHorizontalBox.h"
#include "Silica/include/SVerticalBox.h"
#include "Silica/include/SScissorBox.h"
#include "Silica/include/STextBlock.h"
#include "Silica/include/SEditableText.h"
#include "Silica/include/SScrollBox.h"
#include "Silica/include/SOverlay.h"
#include "Silica/include/SWindow.h"
#include "Silica/include/SSliderFloat.h"
#include "Silica/include/SSliderInt.h"
#include "Silica/include/SCheckbox.h"
#include "Silica/include/SWorkspace.h"
#include "Silica/include/SCollapsingHeader.h"
#include "Silica/include/SColorPicker.h"
#include "Silica/include/SMenuAnchor.h"
#include "Silica/include/SNodeEditor.h"
#include "Silica/include/InputCodes.h"
#include "Silica/include/SInputFieldFloat.h"
#include "Silica/include/SInputFieldInt.h"
#include "Silica/include/SInputFieldVec3Float.h"

class SBorderLayout : public Silica::SWidget {
public:
	Silica::WidgetPtr topBar;
	Silica::WidgetPtr contentArea;

	void construct(Silica::WidgetPtr tb, Silica::WidgetPtr ca) { topBar = tb; contentArea = ca; }
	void computeDesiredSize() override {
		if (topBar) topBar->computeDesiredSize();
		if (contentArea) contentArea->computeDesiredSize();
		m_desiredSize = Silica::Vec2::zero();
	}

	void arrangeChildren(const Silica::Geometry& geo) override {
		m_allocatedGeometry = geo;
		topBar->computeDesiredSize();
		float tbHeight = topBar->getDesiredSize().y;

		topBar->arrangeChildren({ geo.position, {geo.size.x, tbHeight} });
		contentArea->arrangeChildren({ {geo.position.x, geo.position.y + tbHeight}, {geo.size.x, geo.size.y - tbHeight} });
	}

	void onDraw(Silica::DrawList& dl, const Silica::Geometry& geo) const override {
		topBar->onDraw(dl, topBar->getAllocatedGeometry());
		contentArea->onDraw(dl, contentArea->getAllocatedGeometry());
	}

	Silica::EventReply onMouseMove(const Silica::Geometry& geo, const Silica::Vec2& m) override {
		if (topBar->onMouseMove(topBar->getAllocatedGeometry(), m).isHandled) return Silica::EventReply::handled();
		return contentArea->onMouseMove(contentArea->getAllocatedGeometry(), m);
	}
	Silica::EventReply onMouseButtonDown(const Silica::Geometry& geo, const Silica::Vec2& m, Silica::MouseButton button) override {
		if (topBar->onMouseButtonDown(topBar->getAllocatedGeometry(), m, button).isHandled) return Silica::EventReply::handled();
		return contentArea->onMouseButtonDown(contentArea->getAllocatedGeometry(), m, button);
	}
	Silica::EventReply onMouseButtonUp(const Silica::Geometry& geo, const Silica::Vec2& m, Silica::MouseButton button) override {
		if (topBar->onMouseButtonUp(topBar->getAllocatedGeometry(), m, button).isHandled) return Silica::EventReply::handled();
		return contentArea->onMouseButtonUp(contentArea->getAllocatedGeometry(), m, button);
	}
	Silica::EventReply onMouseWheel(const Silica::Geometry& geo, const Silica::Vec2& m, float s) override {
		if (topBar->onMouseWheel(topBar->getAllocatedGeometry(), m, s).isHandled) return Silica::EventReply::handled();
		return contentArea->onMouseWheel(contentArea->getAllocatedGeometry(), m, s);
	}
};

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
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	int trueWidth = clientRect.right - clientRect.left;
	int trueHeight = clientRect.bottom - clientRect.top;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = s_frameCount;
	swapChainDesc.Width = trueWidth;
	swapChainDesc.Height = trueHeight;
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

	// -- Hierarchy --
	auto hierarchyContent = Silica::MakeWidget<Silica::SScrollBox>({
		.child = Silica::MakeWidget<Silica::SVerticalBox>({
			.spacing = 2.0f,
			.slots = {
				{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "[+] Main Scene", .font = &m_font})}) },
				{ {20, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Directional Light", .font = &m_font})}) },
				{ {20, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Main Camera", .font = &m_font})}) },
				{ {20, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Player Model", .font = &m_font})}) },
				{ {20, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Terrain Setup", .font = &m_font})}) }
			}
		})
		});

	// -- Properties --
	auto propertiesContent = Silica::MakeWidget<Silica::SVerticalBox>({
		.spacing = 4.0f,
		.slots = {
			// SHOWCASING SCHECKBOX AND SEDITABLETEXT
			{ {5, 5}, Silica::MakeWidget<Silica::SHorizontalBox>({
				.spacing = 10.0f,
				.slots = {
					{ {0,0}, Silica::MakeWidget<Silica::STextBlock>({.text = "Entity:", .font = &m_font}) },
					{ {0,0}, Silica::MakeWidget<Silica::SEditableText>({.hintText = "Player Model", .font = &m_font}) },
					{ {0,0}, Silica::MakeWidget<Silica::SCheckBox>({.initialCheck = true}) }, // The Box
					{ {0,0}, Silica::MakeWidget<Silica::STextBlock>({.text = "Visible", .font = &m_font}) } // The Label
				}
			}) },

				// --- Transform Section ---
				{ {0, 0}, Silica::MakeWidget<Silica::SCollapsingHeader>({
					.title = "Transform",
					.initiallyOpen = true,
					.font = &m_font,
					.content = Silica::MakeWidget<Silica::SVerticalBox>({
						.spacing = 2.0f,
						.slots = {
							{ {10, 2}, Silica::MakeWidget<Silica::STextBlock>({.text = "Position: X: 0.0  Y: 10.0  Z: 5.0", .font = &m_font}) },
							{ {10, 2}, Silica::MakeWidget<Silica::STextBlock>({.text = "Rotation: X: 0.0  Y: 0.0  Z: 0.0", .font = &m_font}) },
							{ {10, 2}, Silica::MakeWidget<Silica::STextBlock>({.text = "Scale:    X: 1.0  Y: 1.0  Z: 1.0", .font = &m_font}) }
						}
					})
				}) },

				// --- Camera Settings Section ---
				{ {0, 0}, Silica::MakeWidget<Silica::SBox>({.backgroundColor = Silica::Color(55,55,55,255), .child = Silica::MakeWidget<Silica::STextBlock>({.text = "  Camera Settings", .font = &m_font})}) },
				{ {10, 2}, Silica::MakeWidget<Silica::SHorizontalBox>({
					.spacing = 15.0f,
					.slots = {
						{ {0, 0}, Silica::MakeWidget<Silica::STextBlock>({.text = "Field of View", .font = &m_font}) },
						{ {0, 0}, Silica::MakeWidget<Silica::SSliderFloat>({
							.initialValue = 90.0f,
							.minValue = 30.0f,
							.maxValue = 120.0f
						})},
						{ {0, 0}, Silica::MakeWidget<Silica::SColorPicker>({
							.initialColor = Silica::Color(255, 128, 0, 255)
						})}
					}
				}) },

				// --- Mesh Section ---
				{ {0, 0}, Silica::MakeWidget<Silica::SBox>({.backgroundColor = Silica::Color(55,55,55,255), .child = Silica::MakeWidget<Silica::STextBlock>({.text = "  Mesh Settings", .font = &m_font})}) },
				{ {10, 2}, Silica::MakeWidget<Silica::SHorizontalBox>({
					.spacing = 15.0f,
					.slots = {
						{ {0, 0}, Silica::MakeWidget<Silica::STextBlock>({.text = "Subdivisions ", .font = &m_font}) },
						{ {0, 0}, Silica::MakeWidget<Silica::SSliderInt>({
							.initialValue = 2,
							.minValue = 1,
							.maxValue = 8
						}) }
					}
				}) }
			}
		});

	// -- Editor Viewport --
	auto vpToolbar = Silica::MakeWidget<Silica::SBox>({
		.backgroundColor = Silica::Color(45, 45, 45, 255),
		.child = Silica::MakeWidget<Silica::SHorizontalBox>({
			.slots = {
				{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Play", .font = &m_font})}) },
				{ {2, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Simulate", .font = &m_font})}) },
				{ {2, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Stop", .font = &m_font})}) },
				{ {2, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Step", .font = &m_font})}) },
				{ {15, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Camera: 3D", .font = &m_font})}) },
				//{ {2, 2}, Silica::MakeWidget<Silica::SSliderFloat>({ .initialValue = 2.0f, .minValue = 1.0f, .maxValue = 10.0f, .snapStep = 0.5f, .font = &m_font }) },
				//{ {2, 2}, Silica::MakeWidget<Silica::SSliderInt>({ .initialValue = 2, .minValue = 1, .maxValue = 10, .snapStep = 2, .font = &m_font }) },
				//{ {2, 2}, Silica::MakeWidget<Silica::SInputFieldFloat>({ .initialValue = 2.0f, .font = &m_font }) },
				//{ {2, 2}, Silica::MakeWidget<Silica::SInputFieldInt>({ .initialValue = 2, .font = &m_font }) },
				{ {2, 2}, Silica::MakeWidget<Silica::SInputFieldVec3Float>({ .initialValue = {2.0f, 2.0f, 2.0f}, .font = &m_font})},
			}
		})
	});
	auto vpRenderArea = Silica::MakeWidget<Silica::SBox>({
		.backgroundColor = Silica::Color(15, 15, 15, 255),
		.child = Silica::MakeWidget<Silica::STextBlock>({.text = "\n\n   [ 3D Render Image Placeholder ]", .font = &m_font})
	});
	auto viewportContent = std::make_shared<SBorderLayout>();
	viewportContent->construct(vpToolbar, vpRenderArea);

	// -- Content Browser --
	auto cbToolbar = Silica::MakeWidget<Silica::SBox>({
		.backgroundColor = Silica::Color(45, 45, 45, 255),
		.child = Silica::MakeWidget<Silica::SHorizontalBox>({
			.slots = {
				{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "<", .font = &m_font})}) },
				{ {2, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = ">", .font = &m_font})}) },
				{ {10, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Refresh", .font = &m_font})}) },
				{ {2, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "+ Add Folder", .font = &m_font})}) },
				{ {20, 2}, Silica::MakeWidget<Silica::SEditableText>({
					.hintText = "Search Assets...",
					.font = &m_font
				}) }
			}
		})
		});
	auto cbFilesArea = Silica::MakeWidget<Silica::SBox>({
		.backgroundColor = Silica::Color(30, 30, 30, 255),
		.child = Silica::MakeWidget<Silica::SHorizontalBox>({
			.slots = {
				{ {10, 10}, Silica::MakeWidget<Silica::SVerticalBox>({.slots = { { {0,0}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "[DIR]\nModels", .font = &m_font})}) } }}) },
				{ {10, 10}, Silica::MakeWidget<Silica::SVerticalBox>({.slots = { { {0,0}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "[DIR]\nTextures", .font = &m_font})}) } }}) },
				{ {10, 10}, Silica::MakeWidget<Silica::SVerticalBox>({.slots = { { {0,0}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "[FILE]\nPlayer.fbx", .font = &m_font})}) } }}) },
			}
		})
		});
	auto contentBrowserContent = std::make_shared<SBorderLayout>();
	contentBrowserContent->construct(cbToolbar, cbFilesArea);

	// -- Node Editor --
	auto nodeEditor = Silica::MakeWidget<Silica::SNodeEditor>({
		.font = &m_font,
		.onBackgroundContextClick = [this](Silica::Vec2 screenPos) -> Silica::WidgetPtr {
			return Silica::MakeWidget<Silica::SBox>({
				.backgroundColor = Silica::Color(50, 50, 50, 255),
				.child = Silica::MakeWidget<Silica::SVerticalBox>({
					.spacing = 2.0f,
					.slots = {
						{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Add Math Node", .font = &m_font})}) },
						{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Add Float Node", .font = &m_font})}) },
						{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Cancel", .font = &m_font})}) }
					}
				})
			});
		},
		.onNodeContextClick = [this](Silica::NodeID nodeID, Silica::Vec2 screenPos) -> Silica::WidgetPtr {
			return Silica::MakeWidget<Silica::SBox>({
				.backgroundColor = Silica::Color(50, 50, 50, 255),
				.child = Silica::MakeWidget<Silica::SVerticalBox>({
					.slots = {
						{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Delete Node", .font = &m_font})}) },
						{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Duplicate", .font = &m_font})}) }
					}
				})
			});
		}
		});

	//nodeEditor->addNode({
	//	.id = 1,
	//	.title = "Delta Time",
	//	.headerColor = Silica::Color(40, 150, 80, 255),
	//	.position = { 50, 100 },
	//	.size = { 120, 80 },
	//	.inputs = {},
	//	.outputs = { { 101, "Float", Silica::PinType::Output, Silica::Color(150, 255, 150, 255) } }
	//	});
	//
	//nodeEditor->addNode({
	//	.id = 2,
	//	.title = "Multiply",
	//	.headerColor = Silica::Color(80, 80, 200, 255),
	//	.position = { 300, 80 },
	//	.size = { 120, 100 },
	//	.inputs = {
	//		{ 201, "A", Silica::PinType::Input, Silica::Color(150, 255, 150, 255) },
	//		{ 202, "B", Silica::PinType::Input, Silica::Color(150, 255, 150, 255) }
	//	},
	//	.outputs = { { 203, "Result", Silica::PinType::Output, Silica::Color(150, 255, 150, 255) } }
	//	});
	//
	//nodeEditor->addLink(1, 101, 201, Silica::Color(150, 255, 150, 255));

	// -- Docking Workspace --
	auto workspace = Silica::MakeWidget<Silica::SWorkspace>({
		.initialTitle = "Viewport",
		.font = &m_font,
		.initialContent = viewportContent,
	});

	auto dock = workspace->getDockSpace();

	dock->registerTab("Hierarchy", hierarchyContent);
	dock->registerTab("Properties", propertiesContent);
	dock->registerTab("Content Browser", contentBrowserContent);
	dock->registerTab("Blueprint Graph", nodeEditor);
	dock->registerTab("Viewport", viewportContent);

	dock->loadLayout("editor_layout.ini");

	if (!dock->getRootNode() || dock->getRootNode()->tabs.empty() && dock->getRootNode()->splitDirection == Silica::SplitDirection::None) {
		auto root = dock->getRootNode();
		dock->splitNode(root, Silica::SplitDirection::Horizontal, 0.2f, "Hierarchy", hierarchyContent, true);
		dock->splitNode(root->child[1], Silica::SplitDirection::Horizontal, 0.75f, "Properties", propertiesContent, false);
		dock->splitNode(root->child[1]->child[0], Silica::SplitDirection::Vertical, 0.7f, "Content Browser", contentBrowserContent, false);
		dock->splitNode(root->child[1]->child[0]->child[0], Silica::SplitDirection::Horizontal, 0.5f, "Blueprint Graph", nodeEditor, false);
	}

	// -- Menu Bar --
	auto exportSubMenu = Silica::MakeWidget<Silica::SMenuAnchor>({
		.openOnHover = true,
		.openToRight = true,
		.showArrow = true,
		.anchorContent = Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Export", .font = &m_font})}),
		.menuContent = Silica::MakeWidget<Silica::SBox>({
			.backgroundColor = Silica::Color(50, 50, 50, 255),
			.child = Silica::MakeWidget<Silica::SVerticalBox>({
				.spacing = 2.0f,
				.slots = {
					{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Windows (.exe)", .font = &m_font})}) },
					{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Linux (.elf)", .font = &m_font})}) },
					{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Web (HTML5)", .font = &m_font})}) }
				}
			})
		})
		});

	auto fileMenu = Silica::MakeWidget<Silica::SMenuAnchor>({
		.openOnHover = false,
		.openToRight = false,
		.anchorContent = Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "File", .font = &m_font})}),
		.menuContent = Silica::MakeWidget<Silica::SBox>({
			.backgroundColor = Silica::Color(45, 45, 45, 255),
			.child = Silica::MakeWidget<Silica::SVerticalBox>({
				.spacing = 2.0f,
				.slots = {
					{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "New Scene", .font = &m_font})}) },

					// WIRE UP LOAD!
					{ {5, 2}, Silica::MakeWidget<Silica::SButton>({
						.padding = { 10.0f, 10.0f },
						.onClick = [dock, nodeEditor]() {
							dock->loadLayout("editor_layout.ini");
							nodeEditor->loadGraph("blueprint_graph.ini");
							return Silica::EventReply::handled();
						},
						.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Load Layout", .font = &m_font}),
					}) },

				// WIRE UP SAVE!
				{ {5, 2}, Silica::MakeWidget<Silica::SButton>({
					
					.onClick = [dock, nodeEditor]() {
						dock->saveLayout("editor_layout.ini");
						nodeEditor->saveGraph("blueprint_graph.ini");
						OutputDebugStringA("Layout Saved Successfully!\n");
						return Silica::EventReply::handled();
					},
					.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Save Layout", .font = &m_font}),
				}) },

				{ {5, 2}, Silica::MakeWidget<Silica::SBox>({.backgroundColor = Silica::Color(60,60,60,255), .child = Silica::MakeWidget<Silica::STextBlock>({.text = "", .font = &m_font})}) },
				{ {5, 2}, exportSubMenu },
				{ {5, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Exit", .font = &m_font})}) }
			}
		})
	})
		});

	auto mainMenuBar = Silica::MakeWidget<Silica::SBox>({
		.backgroundColor = Silica::Color(40, 40, 40, 255),
		.child = Silica::MakeWidget<Silica::SHorizontalBox>({
			.slots = {
				{ {15, 6}, Silica::MakeWidget<Silica::STextBlock>({.text = "AXION STUDIO", .font = &m_font}) },
				{ {2, 2}, fileMenu },
				{ {2, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Edit", .font = &m_font})}) },
				{ {2, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "View", .font = &m_font})}) },
				{ {2, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Project", .font = &m_font})}) },
				{ {2, 2}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Help", .font = &m_font})}) }
			}
		})
		});


	// -- Assembly of Root and Floating Overlay Window --
	auto mainLayout = std::make_shared<SBorderLayout>();
	mainLayout->construct(mainMenuBar, workspace);

	// SHOWCASING SWINDOW (Free floating over the UI)
	auto floatingWindow = Silica::MakeWidget<Silica::SWindow>({
		.title = "Inspector Tool",
		.initialPosition = { 350.0f, 150.0f },
		.initialSize = { 250.0f, 150.0f },
		.font = &m_font,
		.content = Silica::MakeWidget<Silica::SBox>({
			.backgroundColor = Silica::Color(35, 35, 35, 255),
			.child = Silica::MakeWidget<Silica::SVerticalBox>({
				.spacing = 8.0f,
				.slots = {
					{ {10, 5}, Silica::MakeWidget<Silica::STextBlock>({.text = "Floating Window Test", .font = &m_font}) },

					// Compose the checkbox and label!
					{ {10, 0}, Silica::MakeWidget<Silica::SHorizontalBox>({
						.spacing = 6.0f,
						.slots = {
							{ {0,0}, Silica::MakeWidget<Silica::SCheckBox>({.initialCheck = false}) },
							{ {0,0}, Silica::MakeWidget<Silica::STextBlock>({.text = "Lock Layout", .font = &m_font}) }
						}
					}) },

					{ {10, 0}, Silica::MakeWidget<Silica::SButton>({.child = Silica::MakeWidget<Silica::STextBlock>({.text = "Apply Settings", .font = &m_font})}) }
				}
			})
		})
		});

	// SHOWCASING SOVERLAY (Stacking the floating window on top of the main layout)
	m_uiRoot = Silica::MakeWidget<Silica::SOverlay>({
		.children = { mainLayout, floatingWindow }
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
