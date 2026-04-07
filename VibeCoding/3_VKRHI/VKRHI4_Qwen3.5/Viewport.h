/**
 * Copyright (c) 2025. MIT License. All rights reserved.
 *
 *  Viewport Objects
 *
 *  Swapchain and presentation management using NVRHI and Vulkan.
 */

#pragma once

#include "Renderer/RHI/Common.h"
#include "Renderer/RHI/Object/Frambuffer.h"
#include "Renderer/RHI/Object/RenderTarget.h"

/*-----------------------------------------------------------------------------
	FViewportDesc - Viewport Configuration
-----------------------------------------------------------------------------*/

/**
 * Viewport initialization descriptor
 */
struct FViewportDesc
{
	uint32_t Width = 800;
	uint32_t Height = 600;
	bool bEnableVSync = true;
	bool bEnableValidation = false;
	uint32_t MaxFramesInFlight = 3;
	const TCHAR* DebugName = nullptr;

	FViewportDesc() = default;
};

/*-----------------------------------------------------------------------------
	FViewport - Swapchain and Presentation Management
-----------------------------------------------------------------------------*/

/**
 * Manages swapchain, framebuffers, and presentation for a window
 * 
 * Responsibilities:
 * - Swapchain creation and recreation
 * - Framebuffer management for swapchain images
 * - Synchronization (semaphores, fences)
 * - Frame indexing and triple buffering
 * - Present operations
 * - Resize handling
 * 
 * Usage:
 * ```cpp
 * FViewport Viewport;
 * FViewportDesc Desc;
 * Desc.Width = 1920;
 * Desc.Height = 1080;
 * Viewport.Initialize(Device, Instance, PhysicalDevice, VulkanDevice, Window, Desc);
 * 
 * // In render loop
 * uint32_t FrameIndex = Viewport.AcquireNextFrame(CommandList);
 * // ... render to Viewport.GetCurrentFramebuffer() ...
 * Viewport.Present(CommandList, FrameIndex);
 * ```
 */
class FViewport
{
public:
	NOCOPYMOVE(FViewport)

	FViewport();
	virtual ~FViewport();

	bool Initialize(
		nvrhi::IDevice* InNvrhiDevice,
		vk::Instance InInstance,
		vk::PhysicalDevice InPhysicalDevice,
		vk::Device InVulkanDevice,
		GLFWwindow* InWindow,
		const FViewportDesc& Desc);

	uint32_t AcquireNextFrame(nvrhi::ICommandList* CommandList);
	void Present(nvrhi::ICommandList* CommandList, uint32_t FrameIndex);

	[[nodiscard]] nvrhi::FramebufferHandle GetCurrentFramebuffer() const;
	[[nodiscard]] nvrhi::TextureHandle GetCurrentColorTarget() const;
	[[nodiscard]] uint32_t GetWidth() const { return Width; }
	[[nodiscard]] uint32_t GetHeight() const { return Height; }
	[[nodiscard]] FFramebuffer* GetFramebuffer(uint32_t Index) const;

	void OnWindowResized(int NewWidth, int NewHeight);
	[[nodiscard]] bool IsResizing() const { return bResizing; }

	vk::Semaphore GetRenderFinishedSemaphore(uint32_t FrameIndex) const;
	vk::Fence GetInFlightFence(uint32_t FrameIndex) const;

	void SetDebugName(const TCHAR* Name);

protected:
	bool CreateSurface();
	bool CreateSwapchain(const FViewportDesc& Desc);
	bool CreateSwapchainImageViews();
	bool CreateSwapchainTextures();
	bool CreateFramebuffers();
	bool CreateSyncObjects(const FViewportDesc& Desc);

	void DestroySwapchainResources();
	void WaitForGPU();

	nvrhi::IDevice* NvrhiDevice;
	vk::Instance Instance;
	vk::PhysicalDevice PhysicalDevice;
	vk::Device VulkanDevice;
	GLFWwindow* Window;

	vk::SurfaceKHR Surface;
	vk::SwapchainKHR Swapchain;
	vk::Format SwapchainFormat;
	vk::Extent2D SwapchainExtent;
	std::vector<vk::Image> SwapchainImages;
	std::vector<vk::UniqueImageView> SwapchainImageViews;

	std::vector<nvrhi::TextureHandle> SwapchainTextures;
	std::vector<TUniquePtr<FFramebuffer>> Framebuffers;

	std::vector<vk::UniqueSemaphore> ImageAvailableSemaphores;
	std::vector<vk::UniqueSemaphore> RenderFinishedSemaphores;
	std::vector<vk::UniqueFence> InFlightFences;
	std::vector<vk::Fence> ImagesInFlight;

	uint32_t CurrentFrame;
	uint32_t Width;
	uint32_t Height;
	bool bResizing;

	TCharArray<64> DebugName;
};

/*-----------------------------------------------------------------------------
	FViewportManager - Multi-Viewport Management
-----------------------------------------------------------------------------*/

/**
 * Manages multiple viewports for multi-window support
 */
class FViewportManager
{
public:
	NOCOPYMOVE(FViewportManager)

	FViewportManager() = default;
	~FViewportManager();

	void Initialize(nvrhi::IDevice* InDevice);

	FViewport* CreateViewport(
		GLFWwindow* Window,
		const FViewportDesc& Desc);

	void RemoveViewport(GLFWwindow* Window);
	void RemoveAllViewports();

	FViewport* FindViewport(GLFWwindow* Window);
	[[nodiscard]] nvrhi::IDevice* GetDevice() const { return Device; }

protected:
	nvrhi::IDevice* Device = nullptr;
	TMap<GLFWwindow*, TUniquePtr<FViewport>> ViewportPool;
};
