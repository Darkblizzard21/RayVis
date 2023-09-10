#pragma once

#include <imgui.h>

#include <format>
#include <string_view>

// Helper functions that extend ImGui functionality
namespace ImGui {

    inline void TextUnformatted(std::string_view text)
    {
        const auto* begin = text.data();
        const auto* end   = begin + text.size();

        TextUnformatted(begin, end);
    }

    template <class... T>
    inline void TextFormat(std::format_string<T...> fmt, T&&... args)
    {
        TextUnformatted(std::format(fmt, std::forward<T...>(args...)));
    }

    inline ImVec2 multiply(ImVec2 a, ImVec2 b)
    {
        return ImVec2(a.x * b.x, a.y * b.y);
    }

    inline ImVec4 multiply(ImVec4 a, ImVec4 b)
    {
        return ImVec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
    }

    class StyleColor {
        bool show;

    public:
        StyleColor(ImGuiCol idx, ImVec4 col, bool show = true) : show(show)
        {
            if (show) {
                ImGui::PushStyleColor(idx, col);
            }
        }

        StyleColor(ImGuiCol idx, ImVec4 col1, ImVec4 col2, bool show1, bool show2) : show(show1 || show2)
        {
            if (!show) {
                return;
            }

            ImVec4 col = ImVec4(1, 1, 1, 1);
            if (show1) {
                col = multiply(col, col1);
            }
            if (show2) {
                col = multiply(col, col2);
            }

            ImGui::PushStyleColor(idx, col);
        }

        ~StyleColor()
        {
            if (show) {
                ImGui::PopStyleColor();
            }
        }
    };

    inline StyleColor StyleColorBlend2(ImGuiCol idx, ImVec4 col1, ImVec4 col2, bool show1, bool show2)
    {
        return StyleColor(idx, col1, col2, show1, show2);
    }

    inline StyleColor StyleColorGrey(ImGuiCol idx, bool show = true)
    {
        return StyleColor(idx, ImVec4(0.7, 0.7, 0.7, 1), show);
    }
}  // namespace ImGui