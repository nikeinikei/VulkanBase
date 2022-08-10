#ifndef VULKAN_MEMORY_ALLOCATOR_HPP
#define VULKAN_MEMORY_ALLOCATOR_HPP

#if !defined(AMD_VULKAN_MEMORY_ALLOCATOR_H)
#include <vk_mem_alloc.h>
#endif

#include <vulkan/vulkan.hpp>

#if !defined(VMA_HPP_NAMESPACE)
#define VMA_HPP_NAMESPACE vma
#endif

#define VMA_HPP_NAMESPACE_STRING VULKAN_HPP_STRINGIFY(VMA_HPP_NAMESPACE)

#include "vk_mem_alloc_enums.hpp"
#include "vk_mem_alloc_handles.hpp"
#include "vk_mem_alloc_structs.hpp"
#include "vk_mem_alloc_funcs.hpp"

#endif
