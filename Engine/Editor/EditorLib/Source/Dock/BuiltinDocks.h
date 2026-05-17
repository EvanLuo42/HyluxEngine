/// @file
/// @brief Stage-1 built-in dock factories: Log (functional), Outliner / Details /
///        ContentBrowser (placeholders). EditorApp calls RegisterBuiltinDocks during
///        bootstrap to populate the DockRegistry.

#pragma once

namespace Hylux::Editor
{

class DockRegistry;
class QmlLogSink;

/// @brief Registers Log, Outliner, Details, ContentBrowser factories on the registry.
///        The QmlLogSink pointer must outlive the LogViewModel (which is owned by
///        DockHost); the editor keeps the sink alive via LogSystemConfig::extraSinks.
void RegisterBuiltinDocks(DockRegistry& registry, QmlLogSink* logSink);

} // namespace Hylux::Editor
