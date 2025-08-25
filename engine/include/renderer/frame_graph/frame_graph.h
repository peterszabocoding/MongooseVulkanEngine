#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>
#include "renderer/scene.h"
#include "renderer/vulkan/vulkan_renderpass.h"
#include "renderer/vulkan/vulkan_device.h"
#include "renderer/vulkan/vulkan_mesh.h"

namespace MongooseVK
{
    namespace FrameGraph
    {
        class FrameGraphRenderPass;

        typedef uint32_t FrameGraphHandle;

        struct FrameGraphResourceHandle {
            FrameGraphHandle index;
        };

        struct FrameGraphNodeHandle {
            FrameGraphHandle index;
        };

        struct ResourceUsage {
            enum class Access { Read, Write, ReadWrite };

            enum class Type { Texture, Buffer };

            enum class Usage { Sampled, Storage, ColorAttachment, DepthStencil, UniformBuffer };

            Access access;
            Type type;
            Usage usage;
        };

        struct FrameGraphBufferCreateInfo {
            uint64_t size;
            VkBufferUsageFlags usageFlags;
            VmaMemoryUsage memoryUsage;
        };

        struct FrameGraphResourceCreate {
            const char* name;
            ResourceUsage::Type type;
            FrameGraphBufferCreateInfo bufferCreateInfo{};

            TextureHandle textureHandle = INVALID_TEXTURE_HANDLE;
            TextureCreateInfo textureInfo{};
        };

        struct RenderPassContext {
            PipelineHandle pipeline = INVALID_PIPELINE_HANDLE;
            RenderPassHandle renderPass = INVALID_RENDER_PASS_HANDLE;
            std::vector<FramebufferHandle> framebuffers{};

            VkExtent2D renderArea{};

            VkRenderPassBeginInfo beginInfo{}; // prefilled for convenience
        };

        struct FrameGraphPassBase {
            friend class PassBuilder;

            std::string name;
            std::function<void(VkCommandBuffer)> executeFn; // generic placeholder

            virtual ~FrameGraphPassBase() = default;

            virtual void Setup(PassBuilder& builder) = 0;
            virtual void Execute(VkCommandBuffer cmd, const RenderPassContext& ctx) = 0;

            std::vector<const char*> writeResources;
            std::vector<const char*> readResources;
        };

        template<typename T>
        struct FrameGraphPass : FrameGraphPassBase {
            using SetupFunc = std::function<void(PassBuilder&, T&)>;
            using ExecuteFunc = std::function<void(VkCommandBuffer, const T&, const RenderPassContext&)>;

            void Setup(PassBuilder& builder) override
            {
                setup(builder, data);
            }

            void Execute(VkCommandBuffer cmd, const RenderPassContext& ctx) override
            {
                execute(cmd, data, ctx);
            }

            T data; // user-defined per-pass data
            SetupFunc setup;
            ExecuteFunc execute;
        };

        struct FrameGraphResource : PoolObject {
            const char* name;
            ResourceUsage::Type type;
            AllocatedBuffer allocatedBuffer;

            TextureHandle textureHandle = INVALID_TEXTURE_HANDLE;
            TextureCreateInfo textureInfo{};

            FrameGraphPassBase* producer;
            FrameGraphPassBase* lastWriter;
            uint32_t refCount = 0;
        };

        class PassBuilder {
            friend class FrameGraph;

        public:
            PassBuilder(FrameGraph& fg, FrameGraphPassBase* pass): frameGraph(fg), pass(pass) {}

            void CreateTexture(const char* name, TextureCreateInfo& info);
            void CreateBuffer(const char* name, FrameGraphBufferCreateInfo& info);

            void Write(const char* name);
            void Read(const char* name);

            VkExtent2D GetResolution() const;

        private:
            FrameGraph& frameGraph;
            FrameGraphPassBase* pass;
        };

        template<typename T>
        struct FrameGraphNode {
            std::string name;
            T* renderPass;

            std::vector<std::string> inputs;
            std::vector<std::pair<std::string, ResourceUsage>> outputs;
        };

        class FrameGraph {
            friend class PassBuilder;

        public:
            FrameGraph(VulkanDevice* device);
            ~FrameGraph() = default;

            void Compile(VkExtent2D _resolution);
            void Execute(VkCommandBuffer cmd, SceneGraph* scene);
            void Resize(VkExtent2D newResolution);
            void Cleanup();

            template<typename T>
            void AddRenderPass(const char* name)
            {
                renderPasses[name] = new T(device, resolution);
                renderPassList.push_back(renderPasses[name]);
            }

            template<typename T, typename SetupFunc, typename ExecuteFunc>
            FrameGraphPass<T>& AddPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute)
            {
                // Construct the pass
                auto pass = CreateScope<FrameGraphPass<T>>();
                pass->name = name;
                pass->setup = std::forward<SetupFunc>(setup);
                pass->execute = std::forward<ExecuteFunc>(execute);

                // Call setup with a PassBuilder so the user can declare resources
                PassBuilder builder(*this, pass.get());
                pass->setup(builder, pass->data);

                // Store pass in graph (ownership transfer)
                FrameGraphPass<T>& ref = *pass;
                passes.push_back(std::move(pass));

                return ref;
            }

            void AddExternalResource(const char* name, FrameGraphResource* resource);
            FrameGraphResource* GetResource(const char* name);

        private:
            FrameGraphResourceHandle CreateTextureResource(const char* resourceName, TextureCreateInfo& createInfo);
            FrameGraphResourceHandle CreateBufferResource(const char* resourceName, FrameGraphBufferCreateInfo& createInfo);

            void DestroyResources();
            void InitializeRenderPasses();
            void CreateFrameGraphOutputs();
            RenderPassHandle CreateRenderPass(const std::vector<std::pair<FrameGraphResource*, ResourceUsage>>& outputs);
            PipelineHandle CreatePipeline(PipelineCreateInfo pipelineCreate, RenderPassHandle renderPassHandle,
                                          const std::vector<std::pair<FrameGraphResource*, ResourceUsage>>& outputs);
            FramebufferHandle CreateFramebuffer(RenderPassHandle renderPassHandle,
                                                const std::vector<std::pair<FrameGraphResource*, ResourceUsage>>& outputs);

            void CreateFrameGraphTextureResource(const char* resourceName, const TextureCreateInfo& createInfo);
            void CreateFrameGraphBufferResource(const char* resourceName, FrameGraphBufferCreateInfo& createInfo);

        public:
            std::vector<FrameGraphRenderPass*> renderPassList{};
            std::unordered_map<std::string, FrameGraphRenderPass*> renderPasses;
            std::unordered_map<std::string, FrameGraphResourceHandle> resourceHandles;

            std::unordered_map<std::string, FrameGraphResource*> renderPassResourceMap;
            std::unordered_map<std::string, FrameGraphResource*> externalResources;

            std::unordered_map<std::string, RenderPassContext> renderContextMap;

            std::vector<Scope<FrameGraphPassBase>> passes;

        private:
            VulkanDevice* device;
            VkExtent2D resolution;

            ObjectResourcePool<FrameGraphResource> resourcePool;
        };
    }
}
