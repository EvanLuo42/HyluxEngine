# Stub port — installs nothing. Qt is supplied by a prebuilt Qt 6 install
# located via CMAKE_PREFIX_PATH (e.g. C:/Qt/6.10.3/msvc2022_64) so that
# find_package(Qt6 ...) resolves outside of vcpkg.
set(VCPKG_POLICY_EMPTY_PACKAGE enabled)

file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage"
"${PORT} is a Hylux stub. Qt is provided by a system / prebuilt Qt 6 install.
Ensure CMAKE_PREFIX_PATH (or Qt6_DIR) points at your Qt 6 install so that
find_package(Qt6 COMPONENTS Core Gui Widgets ...) succeeds.
")
