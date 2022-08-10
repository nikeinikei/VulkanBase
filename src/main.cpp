#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <limits>
#include <optional>
#include <map>
#include <memory>
#include <iostream>
#include <vector>
#include <set>
#include <fstream>

static const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsQueue;
    std::optional<uint32_t> presentQueue;

    [[nodiscard]] bool complete() const {
        return graphicsQueue.has_value() && presentQueue.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), (long) fileSize);
    file.close();

    return buffer;
}

class Graphics {
public:
    Graphics();
    ~Graphics();

    void runMainLoop();    

private:
    void createVulkanInstance();
    static bool checkValidationSupport();
    void pickPhysicalDevice();
    void createDevice();
    void createSurface();
    void createSwapChain();
    void createImageViews();
    void createGraphicsPipeline();
    void createCommandPool();
    void createCommandBuffer();
    void createSyncObjects();
    void recordCommandBuffer(vk::CommandBuffer cmdBuffer, uint32_t imageIndex);
    void drawFrame();
    static void cmdTransitionImageLayout(vk::CommandBuffer cmdBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

    unsigned physicalDeviceRating(vk::PhysicalDevice);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice);
    static bool checkDeviceExtensionSupport(vk::PhysicalDevice);
    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice);
    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

    GLFWwindow* window;

    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
    vk::SurfaceKHR surface;
    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;
    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
    vk::Fence inFlightFence;

};

Graphics::Graphics() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(1024, 768, "vulkan test", nullptr, nullptr);

    try {
        createVulkanInstance();
        createSurface();
        pickPhysicalDevice();
        createDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
        createCommandPool();
        createCommandBuffer();
        createSyncObjects();
    } catch (std::exception const &e) {
        std::cerr << "something went wrong while initializing vulkan\n"
            << e.what() << std::endl; 
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "Simplify"
void Graphics::createVulkanInstance() {
    if (enableValidationLayers && !checkValidationSupport()) {
        throw std::runtime_error("validation layers requested but not available");
    }

    vk::ApplicationInfo appInfo {
        .pApplicationName = "",
        .applicationVersion = 1,
        .pEngineName = "",
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_3
    };

    uint32_t count;
    auto extensions = glfwGetRequiredInstanceExtensions(&count);

    vk::InstanceCreateInfo createInfo {
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = count,
        .ppEnabledExtensionNames = extensions
    };

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    instance = vk::createInstance(createInfo);
}
#pragma clang diagnostic pop

bool Graphics::checkValidationSupport() {
    auto availableLayers = vk::enumerateInstanceLayerProperties();

    for (auto layer : validationLayers) {
        bool layerFound = false;

        for (auto& availableLayer : availableLayers) {
            if (strcmp(layer, availableLayer.layerName)== 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

unsigned Graphics::physicalDeviceRating(vk::PhysicalDevice physDevice) {
    unsigned score;

    auto properties = physDevice.getProperties();
    switch (properties.deviceType) {
        case vk::PhysicalDeviceType::eDiscreteGpu:
            score = 10000;
            break;
        case vk::PhysicalDeviceType::eIntegratedGpu:
            score = 1000;
            break;
        case vk::PhysicalDeviceType::eVirtualGpu:
            score = 100;
            break;
        case vk::PhysicalDeviceType::eCpu:
            score = 10;
            break;
        default:
            score = 1;
            break;
    }

    auto indices = findQueueFamilies(physDevice);
    if (!indices.complete()) {
        score = 0;
    }

    auto extensionsSupported = checkDeviceExtensionSupport(physDevice);
    if (!extensionsSupported) {
        score = 0;
    }

    if (extensionsSupported) {
        auto swapChainSupport = querySwapChainSupport(physDevice);
        if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
            score = 0;
        }
    }

    return score;
}

void Graphics::pickPhysicalDevice() {
    auto physicalDevices = instance.enumeratePhysicalDevices();
    
    std::multimap<unsigned, vk::PhysicalDevice> candidates;
    for (const auto& physDevice : physicalDevices) {
        candidates.insert({physicalDeviceRating(physDevice), physDevice});
    }

    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("could not find a suitable device");
    }
}


QueueFamilyIndices Graphics::findQueueFamilies(vk::PhysicalDevice physDevice) {
    QueueFamilyIndices indices;

    auto properties = physDevice.getQueueFamilyProperties();

    uint32_t i = 0;
    for (const auto &queueFamily: properties) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsQueue = i;
        }

        if (physDevice.getSurfaceSupportKHR(i, surface)) {
            indices.presentQueue = i;
        }

        i++;
    }

    return indices;
}

void Graphics::runMainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    device.waitIdle();
}

Graphics::~Graphics() {
    device.destroySemaphore(imageAvailableSemaphore);
    device.destroySemaphore(renderFinishedSemaphore);
    device.destroyFence(inFlightFence);

    device.destroy(commandPool);

    device.destroy(graphicsPipeline);
    device.destroy(pipelineLayout);

    for (auto imageView: swapChainImageViews) {
        device.destroyImageView(imageView);
    }

    device.destroy(swapChain);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    device.destroy();
    instance.destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Graphics::createDevice() {
    auto indices = findQueueFamilies(physicalDevice);

    float priority = 1.0f;

    std::set<uint32_t> queueIndices = {indices.graphicsQueue.value(), indices.presentQueue.value()};
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    for (const uint32_t queueIndex : queueIndices) {
        queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = queueIndex,
                .queueCount = 1,
                .pQueuePriorities = &priority
        });
    }

    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{
        .dynamicRendering = VK_TRUE
    };

    vk::PhysicalDeviceFeatures deviceFeatures{};

    auto createInfo = vk::DeviceCreateInfo {
        .pNext = &dynamicRenderingFeature,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures,
    };

    device = physicalDevice.createDevice(createInfo);
    graphicsQueue = device.getQueue(indices.graphicsQueue.value(), 0);
    presentQueue = device.getQueue(indices.presentQueue.value(), 0);
}

void Graphics::createSurface() {
    VkSurfaceKHR windowSurface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &windowSurface) != VK_SUCCESS) {
        throw std::runtime_error("could not create window surface");
    }
    surface = windowSurface;
}

bool Graphics::checkDeviceExtensionSupport(vk::PhysicalDevice physDevice) {
    auto availableExtensions = physDevice.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &availableExtension: availableExtensions) {
        requiredExtensions.erase(availableExtension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails Graphics::querySwapChainSupport(vk::PhysicalDevice physDevice) {
    SwapChainSupportDetails details;

    details.capabilities = physDevice.getSurfaceCapabilitiesKHR(surface);
    details.formats = physDevice.getSurfaceFormatsKHR(surface);
    details.presentModes = physDevice.getSurfacePresentModesKHR(surface);

    return details;
}

vk::SurfaceFormatKHR Graphics::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR Graphics::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Graphics::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        vk::Extent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
        };

        actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height  = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Graphics::createSwapChain() {
    auto swapChainSupport = querySwapChainSupport(physicalDevice);

    auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    auto extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    auto createInfo = vk::SwapchainCreateInfoKHR {
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = nullptr
    };

    auto indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsQueue.value(), indices.presentQueue.value()};

    if (indices.graphicsQueue != indices.presentQueue) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    swapChain = device.createSwapchainKHR(createInfo);

    swapChainImages = device.getSwapchainImagesKHR(swapChain);
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void Graphics::createImageViews() {
    for (const auto& image : swapChainImages) {
        auto createInfo = vk::ImageViewCreateInfo  {
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = swapChainImageFormat,
            .components = {
                    .r = vk::ComponentSwizzle::eIdentity,
                    .g = vk::ComponentSwizzle::eIdentity,
                    .b = vk::ComponentSwizzle::eIdentity,
                    .a = vk::ComponentSwizzle::eIdentity,
            },
            .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
        };

        swapChainImageViews.push_back(device.createImageView(createInfo));
    }
}

void Graphics::createGraphicsPipeline() {
    auto vertexShaderCode = readFile("shaders/vert.spv");
    auto fragmentShaderCode = readFile("shaders/frag.spv");

    auto vertexShaderCreateInfo = vk::ShaderModuleCreateInfo{
            .codeSize = vertexShaderCode.size(),
            .pCode = reinterpret_cast<const uint32_t *>(vertexShaderCode.data())
    };
    auto vertexShaderModule = device.createShaderModule(vertexShaderCreateInfo);

    auto fragmentShaderCreateInfo = vk::ShaderModuleCreateInfo{
            .codeSize = fragmentShaderCode.size(),
            .pCode = reinterpret_cast<const uint32_t *>(fragmentShaderCode.data())
    };
    auto fragmentShaderModule = device.createShaderModule(fragmentShaderCreateInfo);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = vertexShaderModule,
            .pName = "main",
    };

    vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = fragmentShaderModule,
            .pName = "main",
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragmentShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
            .vertexBindingDescriptionCount = 0,
            .vertexAttributeDescriptionCount = 0,
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = VK_FALSE
    };

    vk::Viewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) swapChainExtent.width,
            .height = (float) swapChainExtent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };

    vk::Rect2D scissor{
            .offset = {0, 0},
            .extent = swapChainExtent,
    };

    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable = VK_FALSE,
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
            .blendEnable = VK_FALSE,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
            .logicOpEnable = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
    };

    pipelineLayout = device.createPipelineLayout({.setLayoutCount = 0});

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapChainImageFormat,
    };

    std::vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport
    };

    vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
        .renderPass = nullptr,
        .subpass = 0,
    };

    graphicsPipeline = device.createGraphicsPipeline(nullptr, pipelineInfo).value;

    device.destroyShaderModule(vertexShaderModule);
    device.destroyShaderModule(fragmentShaderModule);
}

void Graphics::createCommandPool() {
    auto queueFamilyIndices = findQueueFamilies(physicalDevice);

    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilyIndices.graphicsQueue.value(),
    };

    commandPool = device.createCommandPool(poolInfo);
}

void Graphics::createCommandBuffer() {
    auto commandBuffers = device.allocateCommandBuffers({
                                          .commandPool = commandPool,
                                          .level = vk::CommandBufferLevel::ePrimary,
                                          .commandBufferCount = 1,
                                  });

    commandBuffer = commandBuffers.at(0);
}

void Graphics::recordCommandBuffer(vk::CommandBuffer cmdBuffer, uint32_t imageIndex) {
    vk::CommandBufferBeginInfo beginInfo{};

    cmdBuffer.begin(beginInfo);

    vk::RenderingAttachmentInfo colorAttachmentInfo{
            .imageView = swapChainImageViews[imageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eDontCare,
            .storeOp = vk::AttachmentStoreOp::eStore,
    };

    vk::RenderingInfo renderingInfo{
            .renderArea = {
                    .extent = swapChainExtent,
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentInfo,
    };

    cmdTransitionImageLayout(cmdBuffer, swapChainImages[imageIndex], vk::ImageLayout::eUndefined,
                             vk::ImageLayout::eColorAttachmentOptimal);

    cmdBuffer.beginRendering(renderingInfo);

    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    std::array<vk::Viewport, 1> viewports = {
        vk::Viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(swapChainExtent.width),
            .height = static_cast<float>(swapChainExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        }
    };

    cmdBuffer.setViewport(0, viewports);

    cmdBuffer.draw(3, 1, 0, 0);

    cmdBuffer.endRendering();

    cmdTransitionImageLayout(cmdBuffer, swapChainImages[imageIndex], vk::ImageLayout::eColorAttachmentOptimal,
                             vk::ImageLayout::ePresentSrcKHR);

    commandBuffer.end();
}

void Graphics::drawFrame() {
    uint32_t imageIndex = device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE).value;

    device.waitForFences(1, &inFlightFence, VK_TRUE, UINT64_MAX);
    device.resetFences(1, &inFlightFence);

    commandBuffer.reset();
    recordCommandBuffer(commandBuffer, imageIndex);

    vk::Semaphore waitSemaphores[] = {imageAvailableSemaphore};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::Semaphore signalSemaphores[] = {renderFinishedSemaphore};
    vk::SubmitInfo submitInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };

    graphicsQueue.submit(1, &submitInfo, inFlightFence);

    vk::SwapchainKHR swapChains[] = {swapChain};
    vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores     = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,
    };

    presentQueue.presentKHR(presentInfo);
}

void Graphics::createSyncObjects() {
    imageAvailableSemaphore = device.createSemaphore({});
    renderFinishedSemaphore = device.createSemaphore({});
    inFlightFence = device.createFence({
        .flags = vk::FenceCreateFlagBits::eSignaled,
    });
}

void Graphics::cmdTransitionImageLayout(vk::CommandBuffer cmdBuffer, vk::Image image, vk::ImageLayout oldLayout,
                                        vk::ImageLayout newLayout) {
    vk::ImageMemoryBarrier barrier{
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image   = image,
            .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            }
    };

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    } else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR) {
        barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        barrier.dstAccessMask = {};

        sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
    } else {
        throw std::runtime_error("unknown layout transition");
    }

    cmdBuffer.pipelineBarrier(sourceStage, destinationStage, {},
                              0, nullptr,
                              0, nullptr,
                              1, &barrier);
}

int main() {
    try {
        std::unique_ptr<Graphics> graphics = std::make_unique<Graphics>();
        graphics->runMainLoop();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
