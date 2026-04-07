/**
 * Copyright (c) 2025. MIT License. All rights reserved.
 *
 *  Viewport Implementation
 */

#include "Renderer/RHI/Object/Viewport.h"

#if USE_VK_BACKEND

FViewport::FViewport()
	: NvrhiDevice(nullptr)
	, Window(nullptr)
	, Width(0)
	, Height(0)
	, bResizing(false)
	, CurrentFrame(0)
{
}

FViewport::~FViewport()
{
	if (NvrhiDevice && VulkanDevice)
	{
		WaitForGPU();
	}
	DestroySwapchainResources();
	
	if (Surface && Instance)
	{
		Instance.destroySurfaceKHR(Surface);
	}
}

bool FViewport::Initialize(
	nvrhi::IDevice* InNvrhiDevice,
	vk::Instance InInstance,
	vk::PhysicalDevice InPhysicalDevice,
	vk::Device InVulkanDevice,
	GLFWwindow* InWindow,
	const FViewportDesc& Desc)
{
	HLVM_ENSURE_F(InNvrhiDevice, TXT("NVRHI device is null"));
	HLVM_ENSURE_F(InInstance, TXT("Vulkan instance is null"));
	HLVM_ENSURE_F(InVulkanDevice, TXT("Vulkan device is null"));
	HLVM_ENSURE_F(InWindow, TXT("Window is null"));

	NvrhiDevice = InNvrhiDevice;
	Instance = InInstance;
	PhysicalDevice = InPhysicalDevice;
	VulkanDevice = InVulkanDevice;
	Window = InWindow;

	if (!CreateSurface())
	{
		return false;
	}

	if (!CreateSwapchain(Desc))
	{
		return false;
	}

	if (!CreateSwapchainImageViews())
	{
		return false;
	}

	if (!CreateSwapchainTextures())
	{
		return false;
	}

	if (!CreateFramebuffers())
	{
		return false;
	}

	if (!CreateSyncObjects(Desc))
	{
		return false;
	}

	Width = Desc.Width;
	Height = Desc.Height;

	if (Desc.DebugName)
	{
		SetDebugName(Desc.DebugName);
	}

	return true;
}

uint32_t FViewport::AcquireNextFrame(nvrhi::ICommandList* CommandList)
{
	HLVM_ENSURE_F(!bResizing, TXT("Viewport is resizing"));

	vk::Fence InFlightFence = InFlightFences[CurrentFrame];
	VulkanDevice.waitForFences(InFlightFence, VK_TRUE, UINT64_MAX);

	vk::Result result;
	uint32_t ImageIndex = 0;
	try
	{
		result = VulkanDevice.acquireNextImageKHR(
			Swapchain,
			UINT64_MAX,
			ImageAvailableSemaphores[CurrentFrame].get(),
			nullptr,
			&ImageIndex);
	}
	catch (const vk::OutOfDateKHRError&)
	{
		result = vk::Result::eOutOfDateKHR;
	}

	if (result == vk::Result::eOutOfDateKHR)
	{
		int Width, Height;
		glfwGetFramebufferSize(Window, &Width, &Height);
		if (Width > 0 && Height > 0)
		{
			OnWindowResized(Width, Height);
		}
		return CurrentFrame;
	}
	else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
	{
		HLVM_LOG(LogRHI, err, TXT("Failed to acquire swap chain image"));
		return 0;
	}

	if (ImagesInFlight[ImageIndex] != nullptr)
	{
		VulkanDevice.waitForFences(ImagesInFlight[ImageIndex], VK_TRUE, UINT64_MAX);
	}
	ImagesInFlight[ImageIndex] = InFlightFence;

	VulkanDevice.resetFences(InFlightFence);

	return CurrentFrame;
}

void FViewport::Present(nvrhi::ICommandList* CommandList, uint32_t FrameIndex)
{
	HLVM_ENSURE_F(CommandList, TXT("CommandList is null"));

	NvrhiDevice->executeCommandList(CommandList);

	vk::Semaphore RenderFinishedSemaphore = RenderFinishedSemaphores[FrameIndex].get();
	vk::Semaphore ImageAvailableSemaphore = ImageAvailableSemaphores[FrameIndex].get();
	vk::Fence InFlightFence = InFlightFences[FrameIndex].get();

	vk::PipelineStageFlags WaitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo SubmitInfo;
	SubmitInfo.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&ImageAvailableSemaphore)
		.setPWaitDstStageMask(&WaitStages)
		.setCommandBufferCount(1)
		.setPCommandBuffers(&CommandList->getVulkanCommandBuffer())
		.setSignalSemaphoreCount(1)
		.setPSignalSemaphores(&RenderFinishedSemaphore);

	NvrhiDevice->getVulkanQueue()->submit(SubmitInfo, InFlightFence);

	vk::PresentInfoKHR PresentInfo;
	PresentInfo.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&RenderFinishedSemaphore)
		.setSwapchainCount(1)
		.setPSwapchains(&Swapchain)
		.setPImageIndices(&FrameIndex);

	vk::Result result;
	try
	{
		result = NvrhiDevice->getVulkanQueue()->presentKHR(PresentInfo);
	}
	catch (const vk::OutOfDateKHRError&)
	{
		result = vk::Result::eOutOfDateKHR;
	}

	if (result == vk::Result::eOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
	{
		int Width, Height;
		glfwGetFramebufferSize(Window, &Width, &Height);
		if (Width > 0 && Height > 0)
		{
			OnWindowResized(Width, Height);
		}
	}
	else if (result != vk::Result::eSuccess)
	{
		HLVM_LOG(LogRHI, err, TXT("Failed to present swap chain image"));
	}

	CurrentFrame = (CurrentFrame + 1) % RHI::MAX_FRAMES_IN_FLIGHT;
}

nvrhi::FramebufferHandle FViewport::GetCurrentFramebuffer() const
{
	if (Framebuffers.empty() || CurrentFrame >= Framebuffers.size())
	{
		return nullptr;
	}
	return Framebuffers[CurrentFrame]->GetFramebufferHandle();
}

nvrhi::TextureHandle FViewport::GetCurrentColorTarget() const
{
	if (SwapchainTextures.empty() || CurrentFrame >= SwapchainTextures.size())
	{
		return nullptr;
	}
	return SwapchainTextures[CurrentFrame];
}

FFramebuffer* FViewport::GetFramebuffer(uint32_t Index) const
{
	if (Index >= Framebuffers.size())
	{
		return nullptr;
	}
	return Framebuffers[Index].Get();
}

void FViewport::OnWindowResized(int NewWidth, int NewHeight)
{
	if (bResizing)
	{
		return;
	}

	bResizing = true;

	WaitForGPU();
	DestroySwapchainResources();

	SwapchainExtent.width = static_cast<uint32_t>(NewWidth);
	SwapchainExtent.height = static_cast<uint32_t>(NewHeight);

	FViewportDesc Desc;
	Desc.Width = NewWidth;
	Desc.Height = NewHeight;
	CreateSwapchain(Desc);
	CreateSwapchainImageViews();
	CreateSwapchainTextures();
	CreateFramebuffers();

	Width = NewWidth;
	Height = NewHeight;
	bResizing = false;

	HLVM_LOG(LogRHI, info, TXT("Viewport resized to {0}x{1}"), Width, Height);
}

vk::Semaphore FViewport::GetRenderFinishedSemaphore(uint32_t FrameIndex) const
{
	if (FrameIndex >= RenderFinishedSemaphores.size())
	{
		return nullptr;
	}
	return RenderFinishedSemaphores[FrameIndex].get();
}

vk::Fence FViewport::GetInFlightFence(uint32_t FrameIndex) const
{
	if (FrameIndex >= InFlightFences.size())
	{
		return nullptr;
	}
	return InFlightFences[FrameIndex].get();
}

void FViewport::SetDebugName(const TCHAR* Name)
{
	DebugName = Name;
}

bool FViewport::CreateSurface()
{
	if (glfwCreateWindowSurface(Instance, Window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&Surface)) != VK_SUCCESS)
	{
		HLVM_LOG(LogRHI, err, TXT("Failed to create window surface"));
		return false;
	}
	return true;
}

bool FViewport::CreateSwapchain(const FViewportDesc& Desc)
{
	vk::SurfaceCapabilitiesKHR Caps = PhysicalDevice.getSurfaceCapabilitiesKHR(Surface);

	vk::SurfaceFormatKHR SurfaceFormat;
	auto Formats = PhysicalDevice.getSurfaceFormatsKHR(Surface);
	SurfaceFormat = Formats[0];
	for (const auto& Fmt : Formats)
	{
		if (Fmt.format == vk::Format::eB8G8R8A8Srgb)
		{
			SurfaceFormat = Fmt;
			break;
		}
	}

	vk::PresentModeKHR PresentMode = vk::PresentModeKHR::eFifo;
	if (!Desc.bEnableVSync)
	{
		auto PresentModes = PhysicalDevice.getSurfacePresentModesKHR(Surface);
		for (const auto& Mode : PresentModes)
		{
			if (Mode == vk::PresentModeKHR::eMailbox)
			{
				PresentMode = Mode;
				break;
			}
		}
	}

	vk::Extent2D Extent = { Desc.Width, Desc.Height };
	Extent.width = std::clamp(Extent.width, Caps.minImageExtent.width, Caps.maxImageExtent.width);
	Extent.height = std::clamp(Extent.height, Caps.minImageExtent.height, Caps.maxImageExtent.height);

	vk::SwapchainCreateInfoKHR CreateInfo;
	CreateInfo.setSurface(Surface)
		.setMinImageCount(Desc.MaxFramesInFlight)
		.setImageFormat(SurfaceFormat.format)
		.setImageColorSpace(SurfaceFormat.colorSpace)
		.setImageExtent(Extent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setPreTransform(Caps.currentTransform)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(PresentMode)
		.setClipped(true);

	Swapchain = VulkanDevice.createSwapchainKHR(CreateInfo);
	SwapchainFormat = SurfaceFormat.format;
	SwapchainExtent = Extent;

	SwapchainImages = VulkanDevice.getSwapchainImagesKHR(Swapchain);

	return true;
}

bool FViewport::CreateSwapchainImageViews()
{
	SwapchainImageViews.resize(SwapchainImages.size());

	for (size_t i = 0; i < SwapchainImages.size(); i++)
	{
		vk::ImageViewCreateInfo CreateInfo;
		CreateInfo.setImage(SwapchainImages[i])
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(SwapchainFormat)
			.setSubresourceRange(vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

		VULKAN_HPP_TRY(
			auto ImageView = VulkanDevice.createImageViewUnique(CreateInfo);
			HLVM_ASSERT(ImageView);
			SwapchainImageViews[i] = MoveTemp(ImageView););
	}

	return true;
}

bool FViewport::CreateSwapchainTextures()
{
	SwapchainTextures.resize(SwapchainImages.size());

	for (size_t i = 0; i < SwapchainImages.size(); i++)
	{
		nvrhi::vulkan::TextureDesc TextureDesc;
		TextureDesc.setVulkanImage(SwapchainImages[i])
			.setFormat(nvrhi::Format::RGBA8)
			.setExtent(nvrhi::Extent3D(SwapchainExtent.width, SwapchainExtent.height, 1))
			.setInitialState(nvrhi::ResourceStates::Present)
			.setIsRenderTarget(true);

		SwapchainTextures[i] = NvrhiDevice->createTexture(TextureDesc);
		HLVM_ENSURE_F(SwapchainTextures[i], TXT("Failed to create swapchain texture"));
	}

	return true;
}

bool FViewport::CreateFramebuffers()
{
	Framebuffers.resize(SwapchainTextures.size());

	for (size_t i = 0; i < SwapchainTextures.size(); i++)
	{
		Framebuffers[i] = MakeUnique<FFramebuffer>();
		Framebuffers[i]->Initialize(NvrhiDevice);
		Framebuffers[i]->AddColorAttachment(SwapchainTextures[i]);
		Framebuffers[i]->CreateFramebuffer();
	}

	return true;
}

bool FViewport::CreateSyncObjects(const FViewportDesc& Desc)
{
	ImageAvailableSemaphores.resize(Desc.MaxFramesInFlight);
	RenderFinishedSemaphores.resize(Desc.MaxFramesInFlight);
	InFlightFences.resize(Desc.MaxFramesInFlight);
	ImagesInFlight.resize(SwapchainImages.size(), nullptr);

	vk::SemaphoreCreateInfo SemaphoreInfo;
	vk::FenceCreateInfo FenceInfo(vk::FenceCreateFlagBits::eSignaled);

	for (size_t i = 0; i < Desc.MaxFramesInFlight; i++)
	{
		VULKAN_HPP_TRY(
			auto ImageAvailableSemaphore = VulkanDevice.createSemaphoreUnique(SemaphoreInfo);
			HLVM_ASSERT(ImageAvailableSemaphore);
			auto RenderFinishedSemaphore = VulkanDevice.createSemaphoreUnique(SemaphoreInfo);
			HLVM_ASSERT(RenderFinishedSemaphore);
			auto Fence = VulkanDevice.createFenceUnique(FenceInfo);
			HLVM_ASSERT(Fence);
			ImageAvailableSemaphores[i] = MoveTemp(ImageAvailableSemaphore);
			RenderFinishedSemaphores[i] = MoveTemp(RenderFinishedSemaphore);
			InFlightFences[i] = MoveTemp(Fence););
	}

	return true;
}

void FViewport::DestroySwapchainResources()
{
	Framebuffers.clear();
	SwapchainTextures.clear();
	SwapchainImageViews.clear();

	if (Swapchain && VulkanDevice)
	{
		VulkanDevice.destroySwapchainKHR(Swapchain);
		Swapchain = nullptr;
	}
}

void FViewport::WaitForGPU()
{
	if (VulkanDevice)
	{
		VulkanDevice.waitIdle();
	}
}

/*-----------------------------------------------------------------------------
	FViewportManager Implementation
-----------------------------------------------------------------------------*/

FViewportManager::~FViewportManager()
{
	RemoveAllViewports();
}

void FViewportManager::Initialize(nvrhi::IDevice* InDevice)
{
	Device = InDevice;
}

FViewport* FViewportManager::CreateViewport(
	GLFWwindow* Window,
	const FViewportDesc& Desc)
{
	HLVM_ENSURE_F(Device, TXT("ViewportManager not initialized"));
	HLVM_ENSURE_F(Window, TXT("Window is null"));

	TUniquePtr<FViewport> Viewport = MakeUnique<FViewport>();
	
	vk::Instance Instance = nullptr;
	vk::PhysicalDevice PhysicalDevice = nullptr;
	vk::Device VulkanDevice = nullptr;
	
	Viewports.Add(MoveTemp(Viewport));
	
	return Viewport;
}

void FViewportManager::RemoveViewport(GLFWwindow* Window)
{
	for (auto It = Viewports.CreateIterator(); It; ++It)
	{
		if (It->Get())
		{
			Viewports.Remove(It);
			return;
		}
	}
}

void FViewportManager::RemoveAllViewports()
{
	Viewports.Empty();
}

FViewport* FViewportManager::FindViewport(GLFWwindow* Window)
{
	for (auto& Viewport : Viewports)
	{
		if (Viewport)
		{
			return Viewport.Get();
		}
	}
	return nullptr;
}

#endif
