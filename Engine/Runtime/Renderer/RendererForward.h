/// @file
/// @brief Forward declarations for the Renderer module's public types. Include this in
///        headers that need to refer to Renderer types without pulling in their full
///        definitions.

#pragma once

#include <cstdint>

namespace Hylux::Renderer
{

// Subsystem
class RenderSubsystem;
struct RendererConfig;
class RendererFrame;
struct RendererStats;

// View
struct SceneViewRequest;
class SceneView;
class ViewFamily;

// Proxy
enum class ProxyId : std::uint32_t;
class ProxyRegistry;
struct PrimitiveProxy;
struct PrimitiveBounds;

// DrawList
enum class DrawBucket : std::uint8_t;
enum class SortMode : std::uint8_t;
struct DrawListDesc;
struct InstanceDataLayout;
struct DrawListHandle;
class DrawList;
class DrawListBuilder;

// Path
class IRenderPath;
class RenderContext;
class RenderResources;
class RendererRasterPass;
class RendererRasterBuilder;
class RendererPassContext;

// Pso
class PsoCache;
class PipelineLayoutCache;
class PsoBlobStore;
struct PsoKey;
struct PsoRequest;
struct PsoHandle;
struct PsoFormatKey;
struct PipelineRenderState;
struct PsoCacheStats;

// Thread
class RenderThread;
class FrameFenceTimeline;
class StructuralCommandQueue;
class TransformDoubleBuffer;
struct PrimitiveSpawnDesc;
struct TransformGpuRecord;
// NOTE: TransformMat3x4 is an alias for Math::Mat3x4; consumers include
//       Renderer/Thread/TransformDoubleBuffer.h (or Core/Math/Mat3x4.h) directly.

// Material
struct MaterialSnapshot;
class MaterialInstance;
class MaterialProxy;
class MaterialProxyCache;

// Upload
class UploadHeapManager;
class BindlessResidency;

} // namespace Hylux::Renderer
