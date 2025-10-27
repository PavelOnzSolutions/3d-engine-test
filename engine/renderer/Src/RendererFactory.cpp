#include <Engine/Renderer/RendererFactory.h>
#include <Engine/Renderer/RenderContext.h>
#include <Engine/Renderer/Core/FrameGraph.h>
#include <Engine/Renderer/Core/Passes.h>
#include <Engine/Renderer/Scene.h>
#include <memory>
#include <string>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#define NOMINMAX 1
#include <windows.h>
#include <cstring>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <Engine/Assets/SceneLoader.h>
#include <Engine/Core/Logger.h>
#include <Engine/Core/Config.h>
#include <Engine/Assets/GlbLoader.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace Engine::Renderer {

namespace {

static Mat4 Identity4()
{
    Mat4 m{}; m.m[0]=1; m.m[5]=1; m.m[10]=1; m.m[15]=1; return m;
}

// Global scene path for stubs (placeholder until real integration)
static std::wstring g_scene_path;

// Helper: convert UTF-8 string to wide string for Win32 APIs
static std::wstring Utf8ToWide(const char* utf8)
{
    if (!utf8) return L"";
    int len = static_cast<int>(strlen(utf8));
    if (len <= 0) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, len, nullptr, 0);
    if (wlen <= 0) return L"";
    std::wstring wstr(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, len, wstr.data(), wlen);
    return wstr;
}

static std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string s(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), s.data(), len, nullptr, nullptr);
    return s;
}

// No CPU/GDI placeholders allowed: all drawing happens via GPU pipelines now.


class RendererDX12Stub : public IRenderer {
public:
    void SetScenePath(const char* scene_path_utf8) override { g_scene_path = Utf8ToWide(scene_path_utf8); }
    bool Initialize(void* windowHandle) override {
        // Initialize D3D12 device and swap chain for real GPU present
        m_hwnd = reinterpret_cast<HWND>(windowHandle);

        RECT rc{}; GetClientRect(m_hwnd, &rc);
        m_width  = std::max(1u, static_cast<unsigned>(rc.right - rc.left));
        m_height = std::max(1u, static_cast<unsigned>(rc.bottom - rc.top));

        UINT dxgi_factory_flags = 0;
#ifdef _DEBUG
        {
            Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
                debugController->EnableDebugLayer();
            dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
#endif
        if (FAILED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&m_factory)))) return false;
        if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) return false;

        // Command queue
        D3D12_COMMAND_QUEUE_DESC qdesc{}; qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        if (FAILED(m_device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&m_queue)))) return false;

        // Swap chain
        DXGI_SWAP_CHAIN_DESC1 scd{};
        scd.BufferCount = FrameCount;
        scd.Width = m_width;
        scd.Height = m_height;
        scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        scd.SampleDesc.Count = 1;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swap1;
        if (FAILED(m_factory->CreateSwapChainForHwnd(m_queue.Get(), m_hwnd, &scd, nullptr, nullptr, &swap1))) return false;
        if (FAILED(swap1.As(&m_swapchain))) return false;
        m_frame_index = m_swapchain->GetCurrentBackBufferIndex();

        // RTV heap and back buffers
        D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{}; rtvDesc.NumDescriptors = FrameCount; rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        if (FAILED(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtv_heap)))) return false;
        m_rtv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
        for (UINT n = 0; n < FrameCount; ++n)
        {
            if (FAILED(m_swapchain->GetBuffer(n, IID_PPV_ARGS(&m_render_targets[n])))) return false;
            m_device->CreateRenderTargetView(m_render_targets[n].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += m_rtv_descriptor_size;
            if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_allocators[n])))) return false;
        }

        // Create depth buffer
        if (!CreateDepthBuffer()) return false;

        if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_allocators[m_frame_index].Get(), nullptr, IID_PPV_ARGS(&m_cmd_list)))) return false;
        m_cmd_list->Close();

        if (FAILED(m_device->CreateFence(m_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) return false;
        ++m_fence_value;
        m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!m_fence_event) return false;

        // Create basic pipeline (root signature + PSO)
        if (!CreatePipelineState()) return false;

        // Attempt to load mesh from g_scene_path (GLB). If fails, we will draw nothing and rely on app overlay.
        LoadMeshFromScenePath();

        // Setup viewport/scissor
        m_viewport = { 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
        m_scissor = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
        return true;
    }
    void RenderFrame() override {
        if (!m_device) return;
        // Reset
        m_allocators[m_frame_index]->Reset();
        m_cmd_list->Reset(m_allocators[m_frame_index].Get(), m_pso.Get());

        // Transition to render target
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_render_targets[m_frame_index].Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        m_cmd_list->ResourceBarrier(1, &barrier);

        // Set RTV/DSV
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += static_cast<SIZE_T>(m_frame_index) * static_cast<SIZE_T>(m_rtv_descriptor_size);
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
        const float clearColor[4] = { 0.10f, 0.10f, 0.12f, 1.0f };
        m_cmd_list->RSSetViewports(1, &m_viewport);
        m_cmd_list->RSSetScissorRects(1, &m_scissor);
        m_cmd_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        m_cmd_list->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        m_cmd_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // Bind pipeline and draw if mesh is available
        if (m_vb.Get() && m_ib.Get() && m_index_count > 0)
        {
            m_cmd_list->SetGraphicsRootSignature(m_root_sig.Get());
            // Set a very basic MVP = identity (clip-space positions should be transformed in VS)
            float mvp[16] = { 1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1 };
            m_cmd_list->SetGraphicsRoot32BitConstants(0, 16, mvp, 0);

            D3D12_VERTEX_BUFFER_VIEW vbv{};
            vbv.BufferLocation = m_vb->GetGPUVirtualAddress();
            vbv.SizeInBytes = m_vb_size;
            vbv.StrideInBytes = sizeof(VertexPNC);

            D3D12_INDEX_BUFFER_VIEW ibv{};
            ibv.BufferLocation = m_ib->GetGPUVirtualAddress();
            ibv.Format = m_index_format;
            ibv.SizeInBytes = m_ib_size;

            m_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_cmd_list->IASetVertexBuffers(0, 1, &vbv);
            m_cmd_list->IASetIndexBuffer(&ibv);
            m_cmd_list->DrawIndexedInstanced(m_index_count, 1, 0, 0, 0);
        }

        // Transition back to present
        std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
        m_cmd_list->ResourceBarrier(1, &barrier);

        m_cmd_list->Close();
        ID3D12CommandList* lists[] = { m_cmd_list.Get() };
        m_queue->ExecuteCommandLists(1, lists);
        m_swapchain->Present(1, 0);
        MoveToNextFrame();
    }
    void Shutdown() override {
        if (m_queue && m_fence && m_fence_event) {
            UINT64 fenceToWaitFor = m_fence_value;
            if (SUCCEEDED(m_queue->Signal(m_fence.Get(), fenceToWaitFor))) {
                m_fence_value++;
                if (m_fence->GetCompletedValue() < fenceToWaitFor) {
                    m_fence->SetEventOnCompletion(fenceToWaitFor, m_fence_event);
                    WaitForSingleObject(m_fence_event, INFINITE);
                }
            }
        }
        if (m_fence_event) { CloseHandle(m_fence_event); m_fence_event = nullptr; }
        // Release COM via ComPtr destructors
        m_cb.Reset();
        m_ib.Reset();
        m_vb.Reset();
        m_pso.Reset();
        m_root_sig.Reset();
        m_dsv_heap.Reset();
        m_depth.Reset();
        m_cmd_list.Reset();
        for (auto& rt : m_render_targets) rt.Reset();
        for (auto& al : m_allocators) al.Reset();
        m_rtv_heap.Reset(); m_swapchain.Reset(); m_queue.Reset(); m_device.Reset(); m_factory.Reset();
    }
private:
    struct VertexPNC { float px,py,pz; float nx,ny,nz; float u,v; };

    bool CreateDepthBuffer()
    {
        // DSV heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{}; dsvDesc.NumDescriptors = 1; dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        if (FAILED(m_device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&m_dsv_heap)))) return false;

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment = 0;
        desc.Width = m_width;
        desc.Height = m_height;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc = { 1, 0 };
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clear{}; clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; clear.DepthStencil.Depth = 1.0f; clear.DepthStencil.Stencil = 0;
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        if (FAILED(m_device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clear,
                IID_PPV_ARGS(&m_depth))))
            return false;

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc2{}; dsvDesc2.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; dsvDesc2.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        m_device->CreateDepthStencilView(m_depth.Get(), &dsvDesc2, m_dsv_heap->GetCPUDescriptorHandleForHeapStart());
        return true;
    }

    bool CompileShader(const char* source, const char* entry, const char* target, Microsoft::WRL::ComPtr<ID3DBlob>& out)
    {
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        HRESULT hr = D3DCompile(source, strlen(source), nullptr, nullptr, nullptr, entry, target, flags, 0, &out, &errors);
        if (FAILED(hr)) {
            if (errors) OutputDebugStringA((const char*)errors->GetBufferPointer());
            return false;
        }
        return true;
    }

    bool CreatePipelineState()
    {
        static const char* vs = R"HLSL(
struct VSIn { float3 pos: POSITION; float3 n: NORMAL; float2 uv: TEXCOORD0; };
struct VSOut { float4 pos: SV_Position; float3 n: NORMAL; float2 uv: TEXCOORD0; };
cbuffer MVP : register(b0) { float4x4 mvp; };
VSOut main(VSIn i) {
    VSOut o; o.pos = mul(mvp, float4(i.pos,1)); o.n = i.n; o.uv = i.uv; return o; }
)HLSL";
        static const char* ps = R"HLSL(
struct VSOut { float4 pos: SV_Position; float3 n: NORMAL; float2 uv: TEXCOORD0; };
float4 main(VSOut i) : SV_Target { float3 c = 0.5 + 0.5*normalize(i.n); return float4(c,1); }
)HLSL";
        Microsoft::WRL::ComPtr<ID3DBlob> vsb, psb;
        if (!CompileShader(vs, "main", "vs_5_0", vsb)) return false;
        if (!CompileShader(ps, "main", "ps_5_0", psb)) return false;

        // Root signature: 1 CBV as root constants (we'll use 16*float constants)
        D3D12_ROOT_PARAMETER rp{};
        rp.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rp.Constants.Num32BitValues = 16;
        rp.Constants.RegisterSpace = 0;
        rp.Constants.ShaderRegister = 0;
        rp.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        D3D12_ROOT_SIGNATURE_DESC rsd{};
        rsd.NumParameters = 1;
        rsd.pParameters = &rp;
        rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Microsoft::WRL::ComPtr<ID3DBlob> sig, errs;
        if (FAILED(D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &errs))) {
            if (errs) OutputDebugStringA((const char*)errs->GetBufferPointer());
            return false;
        }
        if (FAILED(m_device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&m_root_sig)))) return false;

        D3D12_INPUT_ELEMENT_DESC il[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.pRootSignature = m_root_sig.Get();
        pso.VS = { vsb->GetBufferPointer(), vsb->GetBufferSize() };
        pso.PS = { psb->GetBufferPointer(), psb->GetBufferSize() };
        // Set default fixed-function state without d3dx12 helpers
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        pso.BlendState = blendDesc;
        pso.SampleMask = UINT_MAX;
        D3D12_RASTERIZER_DESC rast{};
        rast.FillMode = D3D12_FILL_MODE_SOLID;
        rast.CullMode = D3D12_CULL_MODE_BACK;
        rast.FrontCounterClockwise = FALSE;
        rast.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rast.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rast.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rast.DepthClipEnable = TRUE;
        rast.MultisampleEnable = FALSE;
        rast.AntialiasedLineEnable = FALSE;
        rast.ForcedSampleCount = 0;
        rast.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        pso.RasterizerState = rast;
        D3D12_DEPTH_STENCIL_DESC ds{};
        ds.DepthEnable = TRUE;
        ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        ds.StencilEnable = FALSE;
        pso.DepthStencilState = ds;
        pso.InputLayout = { il, _countof(il) };
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.NumRenderTargets = 1; pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        pso.SampleDesc.Count = 1;
        if (FAILED(m_device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&m_pso)))) return false;
        return true;
    }

    void LoadMeshFromScenePath()
    {
        if (g_scene_path.empty()) return;
        std::string scene_utf8 = WideToUtf8(g_scene_path);
        Engine::Assets::MeshData mesh{};
        std::vector<std::string> warns;
        if (!Engine::Assets::LoadFirstTriangleMesh(scene_utf8, mesh, warns))
        {
            for (const auto& w : warns) Engine::Core::Logger::Info(std::string("[GLB] ") + w);
            return;
        }
        // Log basic info
        Engine::Core::Logger::Info(std::string("[GLB] Mesh loaded: ") + std::to_string(mesh.vertices.size()) + " verts, " + std::to_string(mesh.indices.size()) + " indices");

        // Build GPU buffers in UPLOAD heap for simplicity
        m_vb_size = static_cast<UINT>(mesh.vertices.size() * sizeof(VertexPNC));
        m_ib_size = static_cast<UINT>(mesh.indices.size() * sizeof(uint32_t));
        if (m_vb_size == 0 || m_ib_size == 0) return;

        // Create VB
        {
            D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            D3D12_RESOURCE_DESC desc{};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; desc.Width = m_vb_size; desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN; desc.SampleDesc = {1,0}; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vb))))
                return;
            void* ptr = nullptr; m_vb->Map(0, nullptr, &ptr);
            if (ptr) {
                // Convert from Assets::MeshData::VertexPNC to local VertexPNC if layouts match
                memcpy(ptr, mesh.vertices.data(), m_vb_size);
                m_vb->Unmap(0, nullptr);
            }
        }
        // Create IB (choose 16-bit or 32-bit)
        bool use16 = true;
        for (uint32_t idx : mesh.indices) { if (idx > 0xFFFFu) { use16 = false; break; } }
        m_index_format = use16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        if (use16)
        {
            m_ib_size = static_cast<UINT>(mesh.indices.size() * sizeof(uint16_t));
            D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            D3D12_RESOURCE_DESC desc{};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; desc.Width = m_ib_size; desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN; desc.SampleDesc = {1,0}; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ib)))) return;
            void* ptr = nullptr; m_ib->Map(0, nullptr, &ptr);
            if (ptr) {
                std::vector<uint16_t> tmp; tmp.reserve(mesh.indices.size());
                for (uint32_t v : mesh.indices) tmp.push_back(static_cast<uint16_t>(v));
                memcpy(ptr, tmp.data(), m_ib_size);
                m_ib->Unmap(0, nullptr);
            }
        }
        else
        {
            D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            D3D12_RESOURCE_DESC desc{};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; desc.Width = m_ib_size; desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN; desc.SampleDesc = {1,0}; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ib)))) return;
            void* ptr = nullptr; m_ib->Map(0, nullptr, &ptr);
            if (ptr) {
                memcpy(ptr, mesh.indices.data(), m_ib_size);
                m_ib->Unmap(0, nullptr);
            }
        }
        m_index_count = static_cast<UINT>(mesh.indices.size());
    }

    // Helpers for resource creation
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(UINT64 size, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_STATES state)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> res;
        D3D12_HEAP_PROPERTIES heapProps{}; heapProps.Type = heap;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc = {1,0};
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (FAILED(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&res)))) return {};
        return res;
    }

    void MoveToNextFrame() {
        const UINT64 currentFence = m_fence_value;
        m_queue->Signal(m_fence.Get(), currentFence);
        ++m_fence_value;
        if (m_fence->GetCompletedValue() < currentFence) {
            m_fence->SetEventOnCompletion(currentFence, m_fence_event);
            WaitForSingleObject(m_fence_event, INFINITE);
        }
        m_frame_index = m_swapchain->GetCurrentBackBufferIndex();
    }

    static constexpr UINT FrameCount = 2;
    HWND m_hwnd{};
    UINT m_width{800}, m_height{600};

    Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapchain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
    UINT m_rtv_descriptor_size{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_render_targets[FrameCount];
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_allocators[FrameCount];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmd_list;
    UINT m_frame_index{};

    // Depth
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsv_heap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depth;

    // Pipeline
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_sig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT     m_scissor{};

    // Geometry
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vb;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_ib;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_cb; // reserved for future
    UINT m_vb_size{};
    UINT m_ib_size{};
    DXGI_FORMAT m_index_format{ DXGI_FORMAT_R16_UINT };
    UINT m_index_count{};

    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fence_value{0};
    HANDLE m_fence_event{};
};

class RendererVulkanStub : public IRenderer {
public:
    void SetScenePath(const char* scene_path_utf8) override { g_scene_path = Utf8ToWide(scene_path_utf8); }
    bool Initialize(void* windowHandle) override {
        m_ctx = std::make_unique<RenderContext>();
#if defined(ENGINE_USE_METHANEKIT)
        HWND hwnd = reinterpret_cast<HWND>(windowHandle);
        RECT rc{}; GetClientRect(hwnd, &rc);
        const uint32_t w = static_cast<uint32_t>(rc.right - rc.left);
        const uint32_t h = static_cast<uint32_t>(rc.bottom - rc.top);
        if (!m_ctx->InitializeFromWindowHandle(hwnd, w ? w : 800, h ? h : 600))
            return false;
#endif
        m_frame_graph = std::make_unique<FrameGraph>();

        m_scene = std::make_unique<Scene>();
        m_camera = std::make_unique<Camera>();
        m_culled = std::make_unique<std::vector<DrawCall>>();
        m_camera->SetViewProjection(Identity4(), Identity4());
        Renderable r; r.mesh_id = 0; r.material_id = 0; r.model = Identity4();
        r.world_bounds = { { -1,-1,-1 }, { 1,1,1 } };
        m_scene->renderables.push_back(r);
        m_frame_graph->SetResource(SceneResources::ScenePtr, m_scene.get());
        m_frame_graph->SetResource(SceneResources::CameraPtr, m_camera.get());
        m_frame_graph->SetResource(SceneResources::CulledDrawsPtr, m_culled.get());

        const bool use_forward_plus = []{
            if (const char* env = std::getenv("ENGINE_FORWARD_PLUS"))
                return env[0] == '1' || env[0] == 't' || env[0] == 'T' || env[0] == 'y' || env[0] == 'Y';
            return false;
        }();
        if (use_forward_plus)
            BuildForwardPlusPipeline(*m_frame_graph);
        else
            BuildDeferredPipeline(*m_frame_graph);
        m_frame_graph->Compile();
        return true;
    }
    void RenderFrame() override {
        if (!m_ctx) return;
        if (!m_ctx->IsFrameRecording()) m_ctx->BeginFrame();
        if (m_frame_graph) m_frame_graph->Execute(*m_ctx);
        m_ctx->EndFrame();
    }
    void Shutdown() override { m_frame_graph.reset(); m_ctx.reset(); m_scene.reset(); m_camera.reset(); m_culled.reset(); }
private:
    std::unique_ptr<RenderContext> m_ctx;
    std::unique_ptr<FrameGraph> m_frame_graph;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<std::vector<DrawCall>> m_culled;
};

class RendererMethaneStub : public IRenderer {
public:
    void SetScenePath(const char* scene_path_utf8) override { g_scene_path = Utf8ToWide(scene_path_utf8); }
    bool Initialize(void* windowHandle) override {
        m_ctx = std::make_unique<RenderContext>();
#if defined(ENGINE_USE_METHANEKIT)
        // Initialize Methane render context from provided window handle
        HWND hwnd = reinterpret_cast<HWND>(windowHandle);
        RECT rc{}; GetClientRect(hwnd, &rc);
        const uint32_t w = static_cast<uint32_t>(rc.right - rc.left);
        const uint32_t h = static_cast<uint32_t>(rc.bottom - rc.top);
        if (!m_ctx->InitializeFromWindowHandle(hwnd, w ? w : 800, h ? h : 600))
            return false;
#endif
        m_frame_graph = std::make_unique<FrameGraph>();

        m_scene = std::make_unique<Scene>();
        m_camera = std::make_unique<Camera>();
        m_culled = std::make_unique<std::vector<DrawCall>>();
        m_camera->SetViewProjection(Identity4(), Identity4());
        Renderable r; r.mesh_id = 0; r.material_id = 0; r.model = Identity4();
        r.world_bounds = { { -1,-1,-1 }, { 1,1,1 } };
        m_scene->renderables.push_back(r);
        m_frame_graph->SetResource(SceneResources::ScenePtr, m_scene.get());
        m_frame_graph->SetResource(SceneResources::CameraPtr, m_camera.get());
        m_frame_graph->SetResource(SceneResources::CulledDrawsPtr, m_culled.get());

        // Choose pipeline based on environment variable ENGINE_FORWARD_PLUS (1/true/yes to enable)
        const bool use_forward_plus = []{
            if (const char* env = std::getenv("ENGINE_FORWARD_PLUS"))
                return env[0] == '1' || env[0] == 't' || env[0] == 'T' || env[0] == 'y' || env[0] == 'Y';
            return false;
        }();
        if (use_forward_plus)
            BuildForwardPlusPipeline(*m_frame_graph);
        else
            BuildDeferredPipeline(*m_frame_graph);
        m_frame_graph->Compile();
        return true;
    }
    void RenderFrame() override {
        if (!m_ctx) return;
        if (!m_ctx->IsFrameRecording()) m_ctx->BeginFrame();
        if (m_frame_graph) m_frame_graph->Execute(*m_ctx);
        m_ctx->EndFrame();
    }
    void Shutdown() override { m_frame_graph.reset(); m_ctx.reset(); m_scene.reset(); m_camera.reset(); m_culled.reset(); }
private:
    std::unique_ptr<RenderContext> m_ctx;
    std::unique_ptr<FrameGraph> m_frame_graph;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<std::vector<DrawCall>> m_culled;
};

} // anonymous namespace

std::unique_ptr<IRenderer> CreateRenderer(const std::string& id)
{
    std::string lid;
    lid.reserve(id.size());
    for (char c : id) lid.push_back(static_cast<char>(::tolower(static_cast<unsigned char>(c))));

    if (lid == "direct3d" || lid == "dx12" || lid == "d3d12" || lid == "d3d" || lid == "directx")
        return std::make_unique<RendererDX12Stub>();
    if (lid == "vulkan" || lid == "vulcan" || lid == "vk")
        return std::make_unique<RendererVulkanStub>();
#if defined(ENGINE_USE_METHANEKIT)
    if (lid == "methane" || lid == "methanekit" || lid == "methane-kit" || lid == "mk")
        return std::make_unique<RendererMethaneStub>();
#else
    if (lid == "methane" || lid == "methanekit" || lid == "methane-kit" || lid == "mk")
        return std::make_unique<RendererDX12Stub>();
#endif

    return {};
}

} // namespace Engine::Renderer
