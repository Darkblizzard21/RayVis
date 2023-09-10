# IMGUI
add_library(imgui STATIC)
target_sources(imgui
    PRIVATE
    imgui/imconfig.h
    imgui/imgui.h
    imgui/backends/imgui_impl_dx12.h
    imgui/backends/imgui_impl_win32.h
    imgui/imgui_internal.h
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h
    helpers/imgui_helpers.h
    
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_dx12.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/imgui_widgets.cpp
    )
target_include_directories(imgui PUBLIC imgui helpers)