#include "context.h"
#include "error.h"
#include "info.h"
#include "window.h"
#include <GLFW/glfw3.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

/// Queue family indices.
typedef struct QueueFamilyIndices {
    /// The compute queue index.
    u32 compute_index;

    /// The graphics queue index.
    u32 graphics_index;

    /// The present queue index.
    u32 present_index;

    /// Whether the queue family indices are all defined.
    /// If this is `false`, the whole struct should not be used.
    bool is_complete;
} QueueFamilyIndices;

/// Return value for `create_vk_instance`.
typedef struct InstanceInfo {
    /// The Vulkan instance.
    VkInstance instance;

    /// The (optional) debug messenger.
    VkDebugUtilsMessengerEXT debug_messenger;
} InstanceInfo;

/// Return value for `choose_physical_device`.
typedef struct PhysicalDeviceData {
    /// The chosen physical device.
    VkPhysicalDevice physical_device;

    /// The queue family indices.
    QueueFamilyIndices queue_family_indices;
} PhysicalDeviceData;

static const QueueFamilyIndices NULL_QUEUE_FAMILY_INDICES = {
    .compute_index = 0,
    .graphics_index = 0,
    .present_index = 0,
    .is_complete = false,
};

static const InstanceInfo NULL_INSTANCE_INFO = {
    .instance = VK_NULL_HANDLE,
    .debug_messenger = VK_NULL_HANDLE,
};

static const PhysicalDeviceData NULL_PHYSICAL_DEVICE_DATA = {
    .physical_device = VK_NULL_HANDLE,
    .queue_family_indices = NULL_QUEUE_FAMILY_INDICES,
};

/// The Vulkan validation layer name.
static const cstr VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";

/// Creates a Vulkan istance from the builder.
/// Also attempts to create a debug messenger if debug messaging is requested.
///
/// On failure, stores the error and returns `VK_NULL_HANDLE`.
///
/// @param builder The context builder.
/// @return A new Vulkan instance and debug messenger (if requested) or `NULL_INSTANCE_INFO` on
/// failure.
static InstanceInfo create_vk_instance(const ContextBuilder* builder);

/// Returns whether debugging features are supported for the system.
///
/// This checks for the `VK_EXT_debug_utils` extension and the `VK_LAYER_KHRONOS_validation` layer.
///
/// @return Whether debugging features are supported for the system.
static bool is_debugging_supported();

/// Vulkan debug messenger callback.
static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
               VkDebugUtilsMessageTypeFlagsEXT message_type,
               const VkDebugUtilsMessengerCallbackDataEXT* data, void*);

/// Creates a surface from the instance and window.
///
/// On failure, stores the error and returns `VK_NULL_HANDLE`.
///
/// @param instance The Vulkan instance.
/// @param window The window.
/// @return A new surface or `VK_NULL_HANDLE` on failure.
static VkSurfaceKHR create_surface(const VkInstance instance, const Window* window);

/// Chooses a Vulkan istance from the builder.
///
/// On failure, stores the error and returns `VK_NULL_HANDLE`.
///
/// @param instance The Vulkan instance.
/// @param surface The target surface.
/// @return A physical device or `VK_NULL_HANDLE` on failure.
static PhysicalDeviceData choose_physical_device(const VkInstance instance,
                                                 const VkSurfaceKHR surface);

/// Gets queue family indices from the given physical device.
///
/// @param physical_device The physical device.
/// @param surface The target surface.
/// @return The physical device's queue family indices.
static QueueFamilyIndices get_queue_family_indices(const VkPhysicalDevice physical_device,
                                                   const VkSurfaceKHR surface);

/// Returns a score for the physical device.
/// A negative score means the device is not usable.
///
/// @param physical_device The physical device to be graded.
/// @return The physical device's score (or a negative number if not usable).
static i32 grade_physical_device(const VkPhysicalDevice physical_device);

/// Creates a device from the given Vulkan instance and physical device.
///
/// On failure, stores the error and returns `VK_NULL_HANDLE`.
///
/// @param physical_device The physical device.
/// @param queue_family_indices The physical device's queue family indices.
/// @return A new device or `VK_NULL_HANDLE` on failure.
static VkDevice create_device(const VkPhysicalDevice physical_device,
                              const QueueFamilyIndices* queue_family_indices);

Context create_context(const ContextBuilder* builder, const Window* window) {
    Context context = NULL_CONTEXT;

    // Create Vulkan instance.
    InstanceInfo instance_info = create_vk_instance(builder);
    context.instance = instance_info.instance;
    context.debug_messenger = instance_info.debug_messenger;
    if (context.instance == VK_NULL_HANDLE) {
        goto FAIL;
    }

    // Create the window surface.
    context.surface = create_surface(context.instance, window);
    context.window = window;
    if (context.surface == VK_NULL_HANDLE) {
        goto FAIL;
    }

    // Get physical device and data.
    PhysicalDeviceData physical_device_data =
        choose_physical_device(context.instance, context.surface);
    context.physical_device = physical_device_data.physical_device;
    QueueFamilyIndices queue_family_indices = physical_device_data.queue_family_indices;
    if (context.physical_device == VK_NULL_HANDLE) {
        goto FAIL;
    }

    // Create device.
    context.device = create_device(context.physical_device, &queue_family_indices);
    if (context.device == VK_NULL_HANDLE) {
        goto FAIL;
    }

    // Get device queues.
    context.compute_queue_index = queue_family_indices.compute_index;
    context.graphics_queue_index = queue_family_indices.graphics_index;
    context.present_queue_index = queue_family_indices.present_index;
    vkGetDeviceQueue(context.device, context.compute_queue_index, 0, &context.compute_queue);
    vkGetDeviceQueue(context.device, context.graphics_queue_index, 0, &context.graphics_queue);
    vkGetDeviceQueue(context.device, context.present_queue_index, 0, &context.present_queue);

    return context;

    // Failure jump; cleans and returns a null context.
FAIL:
    destroy_context(&context);
    return NULL_CONTEXT;
}

void destroy_context(Context* context) {
    vkDeviceWaitIdle(context->device);

    if (context->device != VK_NULL_HANDLE) {
        vkDestroyDevice(context->device, NULL);
    }
    if (context->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(context->instance, context->surface, NULL);
    }
    if (context->debug_messenger != VK_NULL_HANDLE) {
        PFN_vkDestroyDebugUtilsMessengerEXT debug_messenger_destroy_fn =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                context->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (debug_messenger_destroy_fn != NULL) {
            debug_messenger_destroy_fn(context->instance, context->debug_messenger, NULL);
        }
    }
    if (context->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(context->instance, NULL);
    }

    *context = NULL_CONTEXT;
}

InstanceInfo create_vk_instance(const ContextBuilder* builder) {
    // Get extensions and layers.
    u32 num_extensions = 0;
    cstr extensions[16] = {NULL};
    u32 num_layers = 0;
    cstr layers[16] = {NULL};

    // Get GLFW extensions.
    cstr* glfw_extensions = glfwGetRequiredInstanceExtensions(&num_extensions);
    if (glfw_extensions != NULL) {
        memcpy(extensions, glfw_extensions, num_extensions * sizeof(cstr));
    }

    // Use portability for Mac.
#if defined(__APPLE__)
    extensions[num_extensions] = "VK_KHR_portability_enumeration";
    num_extensions++;
#endif

    // Add debug extension and layer if requested and supported.
    bool do_debug_messaging = false;
    if (builder->do_debug_messaging && is_debugging_supported()) {
        extensions[num_extensions++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        layers[num_layers++] = VALIDATION_LAYER_NAME;
        do_debug_messaging = true;
    }

    const u32 app_version = VK_MAKE_API_VERSION(
        0, builder->app_version.major, builder->app_version.minor, builder->app_version.patch);
    VkApplicationInfo application_info = (VkApplicationInfo){
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = builder->app_name,
        .applicationVersion = app_version,
        .pEngineName = "Ibis",
        .engineVersion = IBIS_VK_VERSION,
        .apiVersion = VK_API_VERSION_1_3,
    };
    VkDebugUtilsMessengerCreateInfoEXT debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData = NULL,
    };
    VkInstanceCreateInfo create_info = (VkInstanceCreateInfo){
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = do_debug_messaging ? &debug_info : NULL,
        .flags = 0,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = num_layers,
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = num_extensions,
        .ppEnabledExtensionNames = extensions,
    };

#if defined(__APPLE__)
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // Create instance and debug messenger (if requested).
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
    if (!query_vk_result(vkCreateInstance(&create_info, NULL, &instance))) {
        return NULL_INSTANCE_INFO;
    }
    if (do_debug_messaging) {
        PFN_vkCreateDebugUtilsMessengerEXT debug_messenger_create_fn =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                instance, "vkCreateDebugUtilsMessengerEXT");
        if (debug_messenger_create_fn != NULL) {
            debug_messenger_create_fn(instance, &debug_info, NULL, &debug_messenger);
        }
    }
    return (InstanceInfo){.instance = instance, .debug_messenger = debug_messenger};
}

bool is_debugging_supported() {
    cstr extension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    cstr layer = VALIDATION_LAYER_NAME;

    // Check for debug utils extension.
    u32 num_extensions = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, NULL);
    VkExtensionProperties* extensions = malloc(sizeof(VkExtensionProperties) * num_extensions);
    vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, extensions);
    bool has_debug_extension = false;
    for (u32 i = 0; i < num_extensions; i++) {
        if (strcmp(extensions[i].extensionName, extension) == 0) {
            has_debug_extension = true;
            break;
        }
    }
    free(extensions);
    if (!has_debug_extension) {
        return false;
    }

    // Check for validation layer.
    u32 num_layers = 0;
    vkEnumerateInstanceLayerProperties(&num_layers, NULL);
    VkLayerProperties* layers = malloc(sizeof(VkLayerProperties) * num_layers);
    vkEnumerateInstanceLayerProperties(&num_layers, layers);
    bool has_validation_layer = false;
    for (u32 i = 0; i < num_layers; i++) {
        if (strcmp(layers[i].layerName, layer) == 0) {
            has_validation_layer = true;
            break;
        }
    }
    free(layers);
    if (!has_validation_layer) {
        return false;
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT _message_severity,
               VkDebugUtilsMessageTypeFlagsEXT _message_type,
               const VkDebugUtilsMessengerCallbackDataEXT* data, void* _user_data) {
    (void)_message_severity;
    (void)_message_type;
    (void)_user_data;
    fprintf(stderr, "%s\n", data->pMessage);
    return VK_FALSE;
}

VkSurfaceKHR create_surface(const VkInstance instance, const Window* window) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult result = glfwCreateWindowSurface(instance, window->window, NULL, &surface);

    if (query_vk_result(result)) {
        return surface;
    } else {
        return VK_NULL_HANDLE;
    }
}

PhysicalDeviceData choose_physical_device(const VkInstance instance, const VkSurfaceKHR surface) {
    // Get physical devices.
    u32 num_physical_devices = 0;
    vkEnumeratePhysicalDevices(instance, &num_physical_devices, NULL);
    VkPhysicalDevice* physical_devices = malloc(num_physical_devices * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &num_physical_devices, physical_devices);

    // Iterate and choose the best.
    i32 best_score = INT32_MIN;
    PhysicalDeviceData best = NULL_PHYSICAL_DEVICE_DATA;
    for (u32 i = 0; i < num_physical_devices; i++) {
        // Get queue family indices.
        QueueFamilyIndices queue_family_indices =
            get_queue_family_indices(physical_devices[i], surface);
        if (!queue_family_indices.is_complete) {
            continue;
        }

        i32 score = grade_physical_device(physical_devices[i]);
        if (score > 0 && score > best_score) {
            best_score = score;
            best.physical_device = physical_devices[i];
            best.queue_family_indices = queue_family_indices;
        }
    }

    free(physical_devices);

    if (best_score < 0) {
        set_vk_error(VK_ERROR_INITIALIZATION_FAILED);
    }
    return best;
}

QueueFamilyIndices get_queue_family_indices(const VkPhysicalDevice physical_device,
                                            const VkSurfaceKHR surface) {
    QueueFamilyIndices result = {
        .graphics_index = 0,
        .present_index = 0,
        .compute_index = 0,
        .is_complete = false,
    };
    bool graphics_index_found = false;
    bool present_index_found = false;
    bool compute_index_found = false;

    // Get queue families.
    u32 num_queue_families = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, NULL);
    VkQueueFamilyProperties* queue_families =
        malloc(sizeof(VkQueueFamilyProperties) * num_queue_families);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, queue_families);

    // Choose the first queue family to meet each requirement.
    for (u32 i = 0; i < num_queue_families; i++) {
        // Check for graphics support.
        if (!graphics_index_found && (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            result.graphics_index = i;
            graphics_index_found = true;
        }

        // Check for compute support.
        if (!compute_index_found && (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            result.compute_index = i;
            compute_index_found = true;
        }

        // Check for present support.
        if (!present_index_found) {
            b32 supported = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &supported);
            if (supported) {
                result.present_index = i;
                present_index_found = true;
            }
        }
    }

    free(queue_families);

    result.is_complete = graphics_index_found && present_index_found && compute_index_found;
    return result;
}

i32 grade_physical_device(const VkPhysicalDevice physical_device) {
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceFeatures(physical_device, &features);
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    // Query swapchain support.
    u32 num_extensions = 0;
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_extensions, NULL);
    VkExtensionProperties* extensions = malloc(num_extensions * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_extensions, extensions);
    bool is_swapchain_supported = false;
    for (u32 i = 0; i < num_extensions; i++) {
        if (strcmp(extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            is_swapchain_supported = true;
            break;
        }
    }
    if (!is_swapchain_supported) {
        return INT32_MIN;
    }

    // Score the physical device.
    i32 score = 1;
    if (properties.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score |= 1 << 30;
    }

    return score;
}

VkDevice create_device(const VkPhysicalDevice physical_device,
                       const QueueFamilyIndices* queue_family_indices) {
    cstr extensions[16] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, NULL};
    u32 num_extensions = 1;

#if defined(__APPLE__)
    extensions[num_extensions++] = "VK_KHR_portability_subset";
#endif

    // Create queues.
    const f32 queue_priority = 1.0;
    VkDeviceQueueCreateInfo queue_create_infos[3];
    u32 queue_create_info_count = 0;
    queue_create_infos[queue_create_info_count++] = (VkDeviceQueueCreateInfo){
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = queue_family_indices->compute_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    if (queue_family_indices->graphics_index != queue_family_indices->compute_index) {
        queue_create_infos[queue_create_info_count++] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = queue_family_indices->graphics_index,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        };
    }
    if (queue_family_indices->present_index != queue_family_indices->compute_index &&
        queue_family_indices->present_index != queue_family_indices->graphics_index) {
        queue_create_infos[queue_create_info_count++] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = queue_family_indices->present_index,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        };
    }

    const VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = queue_create_info_count,
        .pQueueCreateInfos = queue_create_infos,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = num_extensions,
        .ppEnabledExtensionNames = extensions,
        .pEnabledFeatures = NULL,
    };

    VkDevice device = VK_NULL_HANDLE;
    if (query_vk_result(vkCreateDevice(physical_device, &create_info, NULL, &device))) {
        return device;
    } else {
        return VK_NULL_HANDLE;
    }
}
