Here is the adapted code. I have applied your coding style (macros like `VULKAN_HPP_TRY`, `HLVM_LOG`, `HLVM_ASSERT`, `TXT`, member naming conventions like `DeviceParams`, and window class `FGLFW3VulkanWindow`) to the remaining sections of the `FDeviceManagerVk` implementation, including Physical Device selection, Logical Device creation, SwapChain management, and the Render Loop.

```cpp
// =============================================================================
// PHYSICAL DEVICE SELECTION
// =============================================================================

bool FDeviceManagerVk::PickPhysicalDevice()
{
	auto devices = instance->enumeratePhysicalDevices();
	if (devices.empty())
	{
		HLVM_LOG(LogRHI, critical, TXT("No Vulkan-compatible GPUs found"));
		return false;
	}

	std::stringstream errorStream;
	errorStream << "Cannot find suitable Vulkan device:";

	std::vector<vk::PhysicalDevice> discreteGPUs;
	std::vector<vk::PhysicalDevice> otherGPUs;

	for (const auto& dev : devices)
	{
		auto props = dev.getProperties();
		errorStream << "\n" << props.deviceName.data() << ":";

		if (!FindQueueFamilies(dev))
		{
			errorStream << "\n  - missing required queue families";
			continue;
		}

		if (!CheckDeviceExtensionSupport(dev))
		{
			errorStream << "\n  - missing required extensions";
			continue;
		}

		auto features = dev.getFeatures();
		if (!features.samplerAnisotropy)
		{
			errorStream << "\n  - no sampler anisotropy";
			continue;
		}
		if (!features.textureCompressionBC)
		{
			errorStream << "\n  - no BC texture compression";
			continue;
		}

		// Check swapchain support
		auto swapChainSupport = QuerySwapChainSupport(dev);
		if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
		{
			errorStream << "\n  - inadequate swapchain support";
			continue;
		}

		// Check presentation support
		vk::Bool32 canPresent = dev.getSurfaceSupportKHR(m_GraphicsQueueFamily, *surface);
		if (!canPresent)
		{
			errorStream << "\n  - cannot present to surface";
			continue;
		}

		// Clamp swapchain buffer count
		auto surfaceCaps = dev.getSurfaceCapabilitiesKHR(*surface);
		DeviceParams.SwapChainBufferCount = Max(surfaceCaps.minImageCount, DeviceParams.SwapChainBufferCount);
		if (surfaceCaps.maxImageCount > 0)
		{
			DeviceParams.SwapChainBufferCount = Min(DeviceParams.SwapChainBufferCount, surfaceCaps.maxImageCount);
		}

		if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			discreteGPUs.push_back(dev);
		}
		else
		{
			otherGPUs.push_back(dev);
		}
	}

	if (!discreteGPUs.empty())
	{
		// glConfig.vendor = getGPUVendor(discreteGPUs[0].getProperties().vendorID); // Uncomment if glConfig exists
		// glConfig.gpuType = GPU_TYPE_DISCRETE;
		physicalDevice = discreteGPUs[0];
	}
	else if (!otherGPUs.empty())
	{
		// glConfig.vendor = getGPUVendor(otherGPUs[0].getProperties().vendorID);
		// glConfig.gpuType = GPU_TYPE_OTHER;
		physicalDevice = otherGPUs[0];
	}
	else
	{
		HLVM_LOG(LogRHI, critical, TO_TCHAR_CSTR(errorStream.str().c_str()));
		return false;
	}

	return true;
}

bool FDeviceManagerVk::FindQueueFamilies(vk::PhysicalDevice device)
{
	auto queueFamilies = device.getQueueFamilyProperties();

	m_GraphicsQueueFamily = -1;
	m_PresentQueueFamily = -1;
	m_ComputeQueueFamily = -1;
	m_TransferQueueFamily = -1;

	for (uint32_t i = 0; i < queueFamilies.size(); i++)
	{
		const auto& queueFamily = queueFamilies[i];

		if (m_GraphicsQueueFamily == -1 &&
		    queueFamily.queueCount > 0 &&
		    (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
		{
			m_GraphicsQueueFamily = i;
		}

		if (m_ComputeQueueFamily == -1 &&
		    queueFamily.queueCount > 0 &&
		    (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
		    !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
		{
			m_ComputeQueueFamily = i;
		}

		if (m_TransferQueueFamily == -1 &&
		    queueFamily.queueCount > 0 &&
		    (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) &&
		    !(queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
		    !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
		{
			m_TransferQueueFamily = i;
		}

		if (m_PresentQueueFamily == -1 && queueFamily.queueCount > 0)
		{
			vk::Bool32 presentSupported = device.getSurfaceSupportKHR(i, *surface);
			if (presentSupported)
			{
				m_PresentQueueFamily = i;
			}
		}
	}

	return m_GraphicsQueueFamily != -1 && m_PresentQueueFamily != -1;
}

bool FDeviceManagerVk::CheckDeviceExtensionSupport(vk::PhysicalDevice device)
{
	auto availableExtensions = device.enumerateDeviceExtensionProperties();

	std::set<std::string> required(enabledExtensions.device.begin(), enabledExtensions.device.end());

	for (const auto& extension : availableExtensions)
	{
		required.erase(extension.extensionName);
	}

	return required.empty();
}

SwapChainSupportDetails FDeviceManagerVk::QuerySwapChainSupport(vk::PhysicalDevice device)
{
	SwapChainSupportDetails details;
	details.capabilities	= device.getSurfaceCapabilitiesKHR(*surface);
	details.formats			= device.getSurfaceFormatsKHR(*surface);
	details.presentModes	= device.getSurfacePresentModesKHR(*surface);
	return details;
}

// =============================================================================
// LOGICAL DEVICE CREATION
// =============================================================================

bool FDeviceManagerVk::CreateLogicalDevice()
{
	// Enable optional extensions
	auto deviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
	for (const auto& ext : deviceExtensions)
	{
		const std::string name = ext.extensionName;
		if (optionalExtensions.device.find(name) != optionalExtensions.device.end())
		{
			enabledExtensions.device.insert(name);
		}
		if (DeviceParams.bEnableRayTracingExtensions && m_RayTracingExtensions.find(name) != m_RayTracingExtensions.end())
		{
			enabledExtensions.device.insert(name);
		}
	}

	// Log enabled extensions
	HLVM_LOG(LogRHI, info, TXT("Enabled Vulkan device extensions:"));
	for (const auto& ext : enabledExtensions.device)
	{
		HLVM_LOG(LogRHI, info, TXT("    {}"), TO_TCHAR_CSTR(ext.c_str()));
	}

	// Collect unique queue families
	std::set<uint32_t> uniqueQueueFamilies = {
		static_cast<uint32_t>(m_GraphicsQueueFamily),
		static_cast<uint32_t>(m_PresentQueueFamily)
	};

	if (DeviceParams.bEnableComputeQueue && m_ComputeQueueFamily != -1)
	{
		uniqueQueueFamilies.insert(m_ComputeQueueFamily);
	}
	if (DeviceParams.bEnableCopyQueue && m_TransferQueueFamily != -1)
	{
		uniqueQueueFamilies.insert(m_TransferQueueFamily);
	}

	float queuePriority = 1.0f;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		queueCreateInfos.push_back(
			vk::DeviceQueueCreateInfo()
				.setQueueFamilyIndex(queueFamily)
				.setQueueCount(1)
				.setPQueuePriorities(&queuePriority)
		);
	}

	// Device features
	vk::PhysicalDeviceFeatures deviceFeatures;
	deviceFeatures
		.setShaderImageGatherExtended(true)
		.setSamplerAnisotropy(true)
		.setTessellationShader(true)
		.setTextureCompressionBC(true)
		.setGeometryShader(true)
		.setFillModeNonSolid(true)
		.setImageCubeArray(true)
		.setDualSrcBlend(true);

	// Vulkan 1.2 features
	vk::PhysicalDeviceVulkan12Features vulkan12Features;
	vulkan12Features
		.setDescriptorIndexing(true)
		.setRuntimeDescriptorArray(true)
		.setDescriptorBindingPartiallyBound(true)
		.setDescriptorBindingVariableDescriptorCount(true)
		.setTimelineSemaphore(true)
		.setShaderSampledImageArrayNonUniformIndexing(true)
		.setBufferDeviceAddress(IsVulkanDeviceExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME));

	auto extensionsVec = StringSetToVector(enabledExtensions.device);

	vk::DeviceCreateInfo createInfo;
	createInfo
		.setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
		.setPQueueCreateInfos(queueCreateInfos.data())
		.setPEnabledFeatures(&deviceFeatures)
		.setEnabledExtensionCount(static_cast<uint32_t>(extensionsVec.size()))
		.setPpEnabledExtensionNames(extensionsVec.data())
		.setPNext(&vulkan12Features);

	VULKAN_HPP_TRY(
		auto dev = physicalDevice.createDeviceUnique(createInfo);
		HLVM_ASSERT(dev);
		device = std::move(dev););

	VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

	// Get queues
	graphicsQueue = device->getQueue(m_GraphicsQueueFamily, 0);
	presentQueue  = device->getQueue(m_PresentQueueFamily, 0);
	
	if (DeviceParams.bEnableComputeQueue && m_ComputeQueueFamily != -1)
	{
		computeQueue = device->getQueue(m_ComputeQueueFamily, 0);
	}
	if (DeviceParams.bEnableCopyQueue && m_TransferQueueFamily != -1)
	{
		transferQueue = device->getQueue(m_TransferQueueFamily, 0);
	}

	// Check D24S8 format support
	vk::ImageFormatProperties imageFormatProperties;
	auto formatResult = physicalDevice.getImageFormatProperties(
		vk::Format::eD24UnormS8Uint,
		vk::ImageType::e2D,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		{},
		&imageFormatProperties
	);
	DeviceParams.bEnableImageFormatD24S8 = (formatResult == vk::Result::eSuccess);

	// Query present modes
	auto surfacePModes = physicalDevice.getSurfacePresentModesKHR(*surface);
	enablePModeMailbox = std::find(surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eMailbox) != surfacePModes.end();
	enablePModeImmediate = std::find(surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eImmediate) != surfacePModes.end();
	enablePModeFifoRelaxed = std::find(surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eFifoRelaxed) != surfacePModes.end();

	// Store device info
	auto props = physicalDevice.getProperties();
	m_RendererString = std::string(props.deviceName.data());
	m_DeviceApiVersion = props.apiVersion;

	// Check timestamp query support (Optional: Uncomment if glConfig available)
	/*
	auto queueProps = physicalDevice.getQueueFamilyProperties();
	glConfig.timerQueryAvailable = (props.limits.timestampPeriod > 0.0) &&
	                               (props.limits.timestampComputeAndGraphics || queueProps[m_GraphicsQueueFamily].timestampValidBits > 0);
	*/

	HLVM_LOG(LogRHI, info, TXT("Created Vulkan device: {}"), TO_TCHAR_CSTR(m_RendererString.c_str()));

	return true;
}

// =============================================================================
// SWAPCHAIN CREATION
// =============================================================================

bool FDeviceManagerVk::CreateSwapChain()
{
	auto swapChainSupport = QuerySwapChainSupport(physicalDevice);

	vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR   presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	vk::Extent2D		   extent = ChooseSwapExtent(swapChainSupport.capabilities);

	// Clamp buffer count
	uint32_t imageCount = DeviceParams.SwapChainBufferCount;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	imageCount = Max(imageCount, swapChainSupport.capabilities.minImageCount);

	// Update stored format
	if (DeviceParams.SwapChainFormat == nvrhi::Format::SRGBA8_UNORM)
	{
		DeviceParams.SwapChainFormat = nvrhi::Format::SBGRA8_UNORM;
	}
	else if (DeviceParams.SwapChainFormat == nvrhi::Format::RGBA8_UNORM)
	{
		DeviceParams.SwapChainFormat = nvrhi::Format::BGRA8_UNORM;
	}

	swapChainImageFormat = vk::Format(nvrhi::vulkan::convertFormat(DeviceParams.SwapChainFormat));
	swapChainExtent = extent;

	uint32_t queueFamilyIndices[] = {
		static_cast<uint32_t>(m_GraphicsQueueFamily),
		static_cast<uint32_t>(m_PresentQueueFamily)
	};

	bool concurrentSharing = m_GraphicsQueueFamily != m_PresentQueueFamily;

	vk::SwapchainCreateInfoKHR createInfo;
	createInfo
		.setSurface(*surface)
		.setMinImageCount(imageCount)
		.setImageFormat(swapChainImageFormat)
		.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
		.setImageExtent(extent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
		.setImageSharingMode(concurrentSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive)
		.setQueueFamilyIndexCount(concurrentSharing ? 2u : 0u)
		.setPQueueFamilyIndices(concurrentSharing ? queueFamilyIndices : nullptr)
		.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(presentMode)
		.setClipped(true)
		.setOldSwapchain(nullptr);

	VULKAN_HPP_TRY(
		auto sc = device->createSwapchainKHRUnique(createInfo);
		HLVM_ASSERT(sc);
		swapChain = std::move(sc););

	// Get swapchain images
	auto images = device->getSwapchainImagesKHR(*swapChain);
	for (auto image : images)
	{
		SwapChainImage sci;
		sci.image = image;

		nvrhi::TextureDesc textureDesc;
		textureDesc.width = extent.width;
		textureDesc.height = extent.height;
		textureDesc.format = DeviceParams.SwapChainFormat;
		textureDesc.debugName = "Swap chain image";
		textureDesc.initialState = nvrhi::ResourceStates::Present;
		textureDesc.keepInitialState = true;
		textureDesc.isRenderTarget = true;

		sci.rhiHandle = m_NvrhiDevice->createHandleForNativeTexture(
			nvrhi::ObjectTypes::VK_Image,
			nvrhi::Object(sci.image),
			textureDesc
		);
		m_SwapChainImages.push_back(sci);
	}

	m_SwapChainIndex = 0;

	return true;
}

void FDeviceManagerVk::DestroySwapChain()
{
	if (device)
	{
		device->waitIdle();
	}

	while (!m_SwapChainImages.empty())
	{
		auto sci = m_SwapChainImages.back();
		m_SwapChainImages.pop_back();
		sci.rhiHandle = nullptr;
	}

	swapChain.reset();
}

vk::SurfaceFormatKHR FDeviceManagerVk::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return availableFormat;
		}
	}
	return availableFormats[0];
}

vk::PresentModeKHR FDeviceManagerVk::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
	switch (DeviceParams.VsyncEnabled)
	{
		case 0:
			if (enablePModeMailbox && r_vkPreferFastSync.GetBool())
				return vk::PresentModeKHR::eMailbox;
			if (enablePModeImmediate)
				return vk::PresentModeKHR::eImmediate;
			return vk::PresentModeKHR::eFifo;
		case 1:
			if (enablePModeFifoRelaxed)
				return vk::PresentModeKHR::eFifoRelaxed;
			return vk::PresentModeKHR::eFifo;
		case 2:
		default:
			return vk::PresentModeKHR::eFifo;
	}
}

vk::Extent2D FDeviceManagerVk::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}

	vk::Extent2D actualExtent = {
		DeviceParams.BackBufferWidth,
		DeviceParams.BackBufferHeight
	};

	actualExtent.width = std::clamp(
		actualExtent.width,
		capabilities.minImageExtent.width,
		capabilities.maxImageExtent.width
	);
	actualExtent.height = std::clamp(
		actualExtent.height,
		capabilities.minImageExtent.height,
		capabilities.maxImageExtent.height
	);

	return actualExtent;
}

// =============================================================================
// SYNCHRONIZATION
// =============================================================================

void FDeviceManagerVk::CreateSyncObjects()
{
	// Create semaphores for each swapchain image
	for (size_t i = 0; i < m_SwapChainImages.size(); i++)
	{
		VULKAN_HPP_TRY(
			auto sem = device->createSemaphoreUnique(vk::SemaphoreCreateInfo());
			HLVM_ASSERT(sem);
			m_PresentSemaphoreQueue.push(std::move(sem)););
	}

	// Get front semaphore for use
	m_PresentSemaphore = *m_PresentSemaphoreQueue.front();

	m_FrameWaitQuery = m_NvrhiDevice->createEventQuery();
	m_NvrhiDevice->setEventQuery(m_FrameWaitQuery, nvrhi::CommandQueue::Graphics);
}

// =============================================================================
// MAIN CREATION / DESTRUCTION
// =============================================================================

bool FDeviceManagerVk::CreateDeviceAndSwapChain()
{
	DeviceParams.bEnableNvrhiValidationLayer = r_useValidationLayers.GetInteger() > 0;
	DeviceParams.bEnableDebugRuntime = r_useValidationLayers.GetInteger() > 1;

	// Initialize dynamic loader
#if VK_HEADER_VERSION >= 301
	using VulkanDynamicLoader = vk::detail::DynamicLoader;
#else
	using VulkanDynamicLoader = vk::DynamicLoader;
#endif

	static const VulkanDynamicLoader dl;
	auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	#define CHECK(a) if (!(a)) { return false; }

	CHECK(CreateInstance());

	if (DeviceParams.bEnableDebugRuntime)
	{
		SetupDebugMessenger();
	}

	// Add user-requested device extensions
	for (const std::string& name : DeviceParams.RequiredVulkanDeviceExtensions)
	{
		enabledExtensions.device.insert(name);
	}
	for (const std::string& name : DeviceParams.OptionalVulkanDeviceExtensions)
	{
		optionalExtensions.device.insert(name);
	}

	CHECK(CreateWindowSurface());
	CHECK(PickPhysicalDevice());
	CHECK(FindQueueFamilies(physicalDevice));
	CHECK(CreateLogicalDevice());

	// Create NVRHI device
	auto vecInstanceExt = StringSetToVector(enabledExtensions.instance);
	auto vecLayers = StringSetToVector(enabledExtensions.layers);
	auto vecDeviceExt = StringSetToVector(enabledExtensions.device);

	nvrhi::vulkan::DeviceDesc deviceDesc;
	deviceDesc.errorCB = &DefaultMessageCallback::GetInstance();
	deviceDesc.instance = *instance;
	deviceDesc.physicalDevice = physicalDevice;
	deviceDesc.device = *device;
	deviceDesc.graphicsQueue = graphicsQueue;
	deviceDesc.graphicsQueueIndex = m_GraphicsQueueFamily;
	
	if (DeviceParams.bEnableComputeQueue && m_ComputeQueueFamily != -1)
	{
		deviceDesc.computeQueue = computeQueue;
		deviceDesc.computeQueueIndex = m_ComputeQueueFamily;
	}
	if (DeviceParams.bEnableCopyQueue && m_TransferQueueFamily != -1)
	{
		deviceDesc.transferQueue = transferQueue;
		deviceDesc.transferQueueIndex = m_TransferQueueFamily;
	}
	
	deviceDesc.instanceExtensions = vecInstanceExt.data();
	deviceDesc.numInstanceExtensions = vecInstanceExt.size();
	deviceDesc.deviceExtensions = vecDeviceExt.data();
	deviceDesc.numDeviceExtensions = vecDeviceExt.size();

	m_NvrhiDevice = nvrhi::vulkan::createDevice(deviceDesc);

	if (DeviceParams.bEnableNvrhiValidationLayer)
	{
		m_ValidationLayer = nvrhi::validation::createValidationLayer(m_NvrhiDevice);
	}

	// Determine max push constant size
	if (r_vkUsePushConstants.GetBool())
	{
		auto deviceProperties = physicalDevice.getProperties();
		DeviceParams.MaxPushConstantSize = Min(
			static_cast<uint32_t>(deviceProperties.limits.maxPushConstantsSize),
			nvrhi::c_MaxPushConstantSize
		);
	}

	CHECK(CreateSwapChain());
	CreateSyncObjects();

	#undef CHECK

	return true;
}

void FDeviceManagerVk::DestroyDeviceAndSwapChain()
{
	if (device)
	{
		device->waitIdle();
	}

	m_FrameWaitQuery = nullptr;

	while (!m_PresentSemaphoreQueue.empty())
	{
		m_PresentSemaphoreQueue.pop();
	}
	m_PresentSemaphore = nullptr;

	DestroySwapChain();

	m_NvrhiDevice = nullptr;
	m_ValidationLayer = nullptr;
	m_RendererString.clear();

	debugMessenger.reset();
	device.reset();
	surface.reset();
	instance.reset();
}

// =============================================================================
// FRAME RENDERING
// =============================================================================

void FDeviceManagerVk::BeginFrame()
{
	// Get GPU memory stats (Optional: requires extension support)
	/*
	vk::PhysicalDeviceMemoryProperties2 memoryProperties2;
	vk::PhysicalDeviceMemoryBudgetPropertiesEXT memoryBudget;
	memoryProperties2.pNext = &memoryBudget;
	physicalDevice.getMemoryProperties2(&memoryProperties2);

	VkDeviceSize gpuMemoryAllocated = 0;
	for (uint32_t i = 0; i < memoryProperties2.memoryProperties.memoryHeapCount; i++)
	{
		gpuMemoryAllocated += memoryBudget.heapUsage[i];
	}
	// Update memory stats...
	*/

	// Acquire next image
	VULKAN_HPP_TRY(
		auto [result, imageIndex] = device->acquireNextImageKHR(
			*swapChain,
			std::numeric_limits<uint64_t>::max(),
			m_PresentSemaphore,
			nullptr
		);

		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			HLVM_LOG(LogRHI, warning, TXT("Failed to acquire swapchain image"));
			return;
		}

		m_SwapChainIndex = imageIndex;
		m_NvrhiDevice->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, m_PresentSemaphore, 0);
	);
}

void FDeviceManagerVk::EndFrame()
{
	m_NvrhiDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, m_PresentSemaphore, 0);
}

void FDeviceManagerVk::Present()
{
	vk::PresentInfoKHR presentInfo;
	presentInfo
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&m_PresentSemaphore)
		.setSwapchainCount(1)
		.setPSwapchains(&*swapChain)
		.setPImageIndices(&m_SwapChainIndex);

	VULKAN_HPP_TRY(
		auto result = presentQueue.presentKHR(presentInfo);
		if (result != vk::Result::eSuccess && result != vk::Result::eErrorOutOfDateKHR && result != vk::Result::eSuboptimalKHR)
		{
			HLVM_LOG(LogRHI, warning, TXT("Failed to present swapchain image"));
		}
	);

	// Cycle semaphore queue
	auto front = std::move(const_cast<vk::UniqueSemaphore&>(m_PresentSemaphoreQueue.front()));
	m_PresentSemaphoreQueue.pop();
	m_PresentSemaphoreQueue.push(std::move(front));
	m_PresentSemaphore = *m_PresentSemaphoreQueue.front();

	// Frame synchronization
	if constexpr (NUM_FRAME_DATA > 2)
	{
		m_NvrhiDevice->waitEventQuery(m_FrameWaitQuery);
	}

	m_NvrhiDevice->resetEventQuery(m_FrameWaitQuery);
	m_NvrhiDevice->setEventQuery(m_FrameWaitQuery, nvrhi::CommandQueue::Graphics);

	if constexpr (NUM_FRAME_DATA < 3)
	{
		m_NvrhiDevice->waitEventQuery(m_FrameWaitQuery);
	}
}

// =============================================================================
// DEBUG CALLBACK
// =============================================================================

VKAPI_ATTR vk::Bool32 VKAPI_CALL FDeviceManagerVk::DebugCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	vk::DebugUtilsMessageTypeFlagsEXT /*messageType*/,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	const FDeviceManagerVk* manager = static_cast<const FDeviceManagerVk*>(pUserData);

	if (manager)
	{
		const auto& ignored = manager->DeviceParams.IgnoredVulkanValidationMessageLocations;
		// Note: location not available in DebugUtils, would need to parse message or use DebugReport
	}

	if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
	{
		HLVM_LOG(LogRHI, error, TXT("[Vulkan] ERROR: {}"), TO_TCHAR_CSTR(pCallbackData->pMessage));
	}
	else if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
	{
		HLVM_LOG(LogRHI, warning, TXT("[Vulkan] WARNING: {}"), TO_TCHAR_CSTR(pCallbackData->pMessage));
	}
	else if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
	{
		HLVM_LOG(LogRHI, info, TXT("[Vulkan] INFO: {}"), TO_TCHAR_CSTR(pCallbackData->pMessage));
	}

	return VK_FALSE;
}

// =============================================================================
// STUBS / EMPTY IMPLEMENTATIONS FOR INTERFACE
// =============================================================================

bool FDeviceManagerVk::CreateWindowDeviceAndSwapChain(const IWindow::Properties& Params)
{
    // Typically handled by CreateDeviceAndSwapChain in this implementation
    return CreateDeviceAndSwapChain();
}

void FDeviceManagerVk::Shutdown()
{
    DestroyDeviceAndSwapChain();
}

void FDeviceManagerVk::GetDPIScaleInfo(float& OutScaleX, float& OutScaleY) const
{
    // TODO: Implement DPI scaling query
    OutScaleX = 1.0f;
    OutScaleY = 1.0f;
}

void FDeviceManagerVk::UpdateWindowSize(const FUInt2& Params)
{
    DeviceParams.BackBufferWidth = Params.x;
    DeviceParams.BackBufferHeight = Params.y;
    ResizeSwapChain();
}

void FDeviceManagerVk::SetWindowTitle(const FString& Title)
{
    if (WindowHandle)
    {
        // Assuming FGLFW3VulkanWindow has a SetTitle method or raw handle access
        // static_cast<FGLFW3VulkanWindow*>(WindowHandle.get())->SetTitle(Title);
    }
}

void FDeviceManagerVk::SetVsyncEnabled(TINT32 VSyncMode)
{
    DeviceParams.VsyncEnabled = VSyncMode;
    // Requires swapchain recreationation to apply new present mode
    ResizeSwapChain();
}

void FDeviceManagerVk::ReportLiveObjects()
{
    if (m_NvrhiDevice)
    {
        m_NvrhiDevice->reportLiveObjects();
    }
}
```