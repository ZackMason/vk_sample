#ifndef DESCRIPTOR_ALLOCATOR_HPP
#define DESCRIPTOR_ALLOCATOR_HPP

#include "core.hpp"

namespace gfx::vul {

static constexpr u64 MAX_DESCRIPTOR_POOLS = 10;

struct descriptor_allocator_t {
    explicit descriptor_allocator_t(VkDevice device_) : device{device_} {}
    virtual ~descriptor_allocator_t() {
        cleanup();
    }

    struct pool_size_t {
        VkDescriptorType type{};
        float            count{};
    };

    void reset_pools();
    void cleanup() {
        range_u64(i, 0, array_count(used_pools)) {
            if (used_pools[i] != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(device, used_pools[i], nullptr);
            }
        }
        range_u64(i, 0, array_count(free_pools)) {
            if (free_pools[i] != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(device, free_pools[i], nullptr);
            }
        }
    }
    bool allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);
    VkDescriptorPool get_pool();

    VkDevice device;

    VkDescriptorPool current_pool{VK_NULL_HANDLE};
    size_t used_pool_count{0};
    size_t free_pool_count{0};
    VkDescriptorPool used_pools[MAX_DESCRIPTOR_POOLS];
    VkDescriptorPool free_pools[MAX_DESCRIPTOR_POOLS];

    pool_size_t descriptor_sizes[11]{
        { VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
    };
};

inline static VkDescriptorPool 
create_pool(
    VkDevice device, 
    const descriptor_allocator_t::pool_size_t* pool_sizes,
    size_t pool_size_count,
    int count, VkDescriptorPoolCreateFlags flags
) {
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(pool_size_count);
    for (auto sz : std::span{pool_sizes, pool_size_count}) {
        sizes.push_back({ sz.type, uint32_t(sz.count * count) });
    }
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = flags;
    pool_info.maxSets = count;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes = sizes.data();

    VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

    return descriptorPool;
}

VkDescriptorPool 
descriptor_allocator_t::get_pool() {
    if (free_pool_count > 0) {
        return free_pools[--free_pool_count];
    } else {
        return create_pool(device, descriptor_sizes, array_count(descriptor_sizes), 1000, 0);
    }
}

bool
descriptor_allocator_t::allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout) {
    //initialize the currentPool handle if it's null
    if (current_pool == VK_NULL_HANDLE){
        current_pool = get_pool();
        used_pools[used_pool_count++] = current_pool;
        assert(used_pool_count < MAX_DESCRIPTOR_POOLS);
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;

    allocInfo.pSetLayouts = &layout;
    allocInfo.descriptorPool = current_pool;
    allocInfo.descriptorSetCount = 1;

    //try to allocate the descriptor set
    VkResult allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);
    bool needReallocate = false;

    switch (allocResult) {
    case VK_SUCCESS:
        //all good, return
        return true;
    case VK_ERROR_FRAGMENTED_POOL:
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        //reallocate pool
        needReallocate = true;
        break;
    default:
        //unrecoverable error
        return false;
    }

    if (needReallocate){
        //allocate a new pool and retry
        current_pool = get_pool();
        used_pools[used_pool_count++] = current_pool;
        assert(used_pool_count < MAX_DESCRIPTOR_POOLS);

        allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);

        //if it still fails then we have big issues
        if (allocResult == VK_SUCCESS){
            return true;
        }
    }

    return false;
}

void
descriptor_allocator_t::reset_pools() {
    for (auto p : std::span{used_pools, used_pool_count}) {
        vkResetDescriptorPool(device, p, 0);
        free_pools[free_pool_count++] = p;
        assert(free_pool_count < MAX_DESCRIPTOR_POOLS);
    }
    used_pool_count = 0;
    current_pool = VK_NULL_HANDLE;
}

static constexpr u64 MAX_DESCRIPTOR_LAYOUT_BINDINGS = 32;
static constexpr u64 MAX_DESCRIPTOR_LAYOUT_CACHE_SIZE = 512;

struct descriptor_layout_cache_t {
    explicit descriptor_layout_cache_t(VkDevice device_) : device{device_} {
        std::memset(cache, (int)VK_NULL_HANDLE, sizeof(size_t) * array_count(cache));
        std::memset(meta, 0, sizeof(size_t) * array_count(meta));
    }
    virtual ~descriptor_layout_cache_t() {
        cleanup();
    }

    void cleanup() {
        range_u64(i, 0, MAX_DESCRIPTOR_LAYOUT_CACHE_SIZE) {
            if (cache[i] != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(device, cache[i], nullptr);
            }
        }
    }

    VkDescriptorSetLayout create_descriptor_set_layout(VkDescriptorSetLayoutCreateInfo* info);

    struct descriptor_layout_info_t {
        size_t binding_count{0};
        VkDescriptorSetLayoutBinding bindings[MAX_DESCRIPTOR_LAYOUT_BINDINGS];

        bool operator==(const descriptor_layout_info_t& o) const;
        size_t hash() const;
    };

    struct descriptor_layout_hash_t {
        std::size_t operator()(const descriptor_layout_info_t& info) const {
            return info.hash();
        }
    };

    size_t meta[MAX_DESCRIPTOR_LAYOUT_CACHE_SIZE];
    VkDescriptorSetLayout cache[MAX_DESCRIPTOR_LAYOUT_CACHE_SIZE];
    VkDevice device;
};

VkDescriptorSetLayout 
descriptor_layout_cache_t::create_descriptor_set_layout(VkDescriptorSetLayoutCreateInfo* info){
    descriptor_layout_cache_t::descriptor_layout_info_t layout_info;
    layout_info.binding_count = info->bindingCount;
    bool isSorted = true;
    int lastBinding = -1;

    //copy from the direct info struct into our own one
    for (size_t i = 0; i < info->bindingCount; i++) {
        layout_info.bindings[i] = info->pBindings[i];

        //check that the bindings are in strict increasing order
        if ((int)info->pBindings[i].binding > lastBinding) {
            lastBinding = info->pBindings[i].binding;
        } else {
            isSorted = false;
        }
    }

    //sort the bindings if they aren't in order
    if (!isSorted){
        range_u64(w, 0, info->bindingCount-1) {
            size_t min{w};

            range_u64(r, w + 1, info->bindingCount) {
                if (layout_info.bindings[r].binding < layout_info.bindings[min].binding) {
                    min = r;
                }
            }

            if (min != w) {
                std::swap(layout_info.bindings[w], layout_info.bindings[min]);
            }
        }
    }

    //try to grab from cache
    const size_t hash = layout_info.hash();
    size_t bucket = hash % MAX_DESCRIPTOR_LAYOUT_CACHE_SIZE;
    
    while (cache[bucket] != VK_NULL_HANDLE && meta[bucket] != hash) {
        bucket = (bucket + 1) % MAX_DESCRIPTOR_LAYOUT_CACHE_SIZE;
        gen_warn(__FUNCTION__, "probe: {}", hash);
    }

    if (meta[bucket] == hash) {
        return cache[bucket];
    } else {
        VkDescriptorSetLayout layout;
        vkCreateDescriptorSetLayout(device, info, nullptr, &layout);

        cache[bucket] = layout;
        meta[bucket] = hash;
        return layout;
    }
}

struct descriptor_builder_t {
    static descriptor_builder_t begin(descriptor_layout_cache_t* layout_cache, descriptor_allocator_t* allocator) {
        return descriptor_builder_t{
            .cache = layout_cache,
            .alloc = allocator
        };
    }

    descriptor_builder_t& bind_buffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
    descriptor_builder_t& bind_image(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

    bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
    bool build(VkDescriptorSet& set);

    descriptor_layout_cache_t* cache;
    descriptor_allocator_t* alloc;

    std::vector<VkWriteDescriptorSet> writes;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

descriptor_builder_t& descriptor_builder_t::bind_buffer(
    uint32_t binding, 
    VkDescriptorBufferInfo* bufferInfo, 
    VkDescriptorType type, 
    VkShaderStageFlags stageFlags
) {
    //create the descriptor binding for the layout
    VkDescriptorSetLayoutBinding newBinding{};

    newBinding.descriptorCount = 1;
    newBinding.descriptorType = type;
    newBinding.pImmutableSamplers = nullptr;
    newBinding.stageFlags = stageFlags;
    newBinding.binding = binding;

    bindings.push_back(newBinding);

    //create the descriptor write
    VkWriteDescriptorSet newWrite{};
    newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    newWrite.pNext = nullptr;

    newWrite.descriptorCount = 1;
    newWrite.descriptorType = type;
    newWrite.pBufferInfo = bufferInfo;
    newWrite.dstBinding = binding;

    writes.push_back(newWrite);
    return *this;
}

descriptor_builder_t& descriptor_builder_t::bind_image(
    uint32_t binding, 
    VkDescriptorImageInfo* imageInfo,
    VkDescriptorType type, 
    VkShaderStageFlags stageFlags
) {
    //create the descriptor binding for the layout
    VkDescriptorSetLayoutBinding newBinding{};

    newBinding.descriptorCount = 1;
    newBinding.descriptorType = type;
    newBinding.pImmutableSamplers = nullptr;
    newBinding.stageFlags = stageFlags;
    newBinding.binding = binding;

    bindings.push_back(newBinding);

    //create the descriptor write
    VkWriteDescriptorSet newWrite{};
    newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    newWrite.pNext = nullptr;

    newWrite.descriptorCount = 1;
    newWrite.descriptorType = type;
    newWrite.pImageInfo = imageInfo;
    newWrite.dstBinding = binding;

    writes.push_back(newWrite);
    return *this;
}

bool descriptor_builder_t::build(VkDescriptorSet& set, VkDescriptorSetLayout& layout){
	//build layout first
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;

	layoutInfo.pBindings = bindings.data();
	layoutInfo.bindingCount = safe_truncate_u64(bindings.size());

	layout = cache->create_descriptor_set_layout(&layoutInfo);

	//allocate descriptor
	bool success = alloc->allocate(&set, layout);
	if (!success) { 
        gen_warn(__FUNCTION__, "Failed to build descriptor set");
        return false; 
    };

	//write descriptor
	for (VkWriteDescriptorSet& w : writes) {
		w.dstSet = set;
	}

	vkUpdateDescriptorSets(alloc->device, safe_truncate_u64(writes.size()), writes.data(), 0, nullptr);

    writes.clear();
    bindings.clear();

	return true;
}

bool descriptor_builder_t::build(VkDescriptorSet& set){
    VkDescriptorSetLayout layout;
	return build(set, layout);
}

size_t descriptor_layout_cache_t::descriptor_layout_info_t::hash() const{
    using std::size_t;
    using std::hash;

    size_t result = hash<size_t>()(binding_count);

    for (const VkDescriptorSetLayoutBinding& b : std::span{bindings, binding_count})
    {
        //pack the binding data into a single int64. Not fully correct but it's ok
        size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

        //shuffle the packed binding data and xor it with the main hash
        result ^= hash<size_t>()(binding_hash);
    }

    return result;
}

};

#endif