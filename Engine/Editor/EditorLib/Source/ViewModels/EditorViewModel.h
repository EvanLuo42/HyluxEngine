/// @file
/// @brief Base class for editor ViewModels. Holds an EditorContext reference so
///        concrete VMs can access selection / engine command queue / dispatcher
///        without taking each dependency individually in their ctors.

#pragma once

#include <QObject>

namespace Hylux::Editor
{

class EditorContext;

class EditorViewModel : public QObject
{
    Q_OBJECT
public:
    explicit EditorViewModel(EditorContext& context, QObject* parent = nullptr);
    ~EditorViewModel() override = default;

    EditorViewModel(const EditorViewModel&)            = delete;
    EditorViewModel& operator=(const EditorViewModel&) = delete;

protected:
    [[nodiscard]] EditorContext& Context() noexcept { return context_; }

private:
    EditorContext& context_;
};

} // namespace Hylux::Editor
