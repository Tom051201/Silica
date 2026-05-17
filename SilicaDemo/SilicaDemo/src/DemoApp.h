#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "Silica/include/SBox.h"
#include "Silica/include/FontAtlas.h"

class DemoApp {
public:

	bool initialize(HWND hWnd, int width, int height);
	void render();
	void cleanup();
	void resize(int width, int height);

	ID3D12Device* getDevice() const { return m_device.Get(); }
	int getFrameIndex() const { return m_frameIndex; }

	const Silica::WidgetPtr& getUIRoot() const { return m_uiRoot; }

private:

	static const UINT s_frameCount = 2;

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[s_frameCount];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocators[s_frameCount];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValues[s_frameCount] = { 0, 0 };

	UINT m_rtvDescriptorSize;
	int m_width;
	int m_height;

	Silica::WidgetPtr m_uiRoot;
	Silica::FontAtlas m_font;

	void waitForGpu();
	void moveToNextFrame();

};
