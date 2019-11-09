#pragma once
#include "../Dear ImGui/imgui.h"
#include <better_enums.h>

//Add-ons to the Dear ImGUI library.

namespace ImGui
{
    //Shows a ComboBox for selecting an enum value.
    //The enum must be created with the better_enums library.
    template<typename BetterEnum_t>
    bool EnumCombo(const char* label, BetterEnum_t& currentItem,
                   int popupMaxHeightInItems = -1)
    {
        int currentIndex = (int)currentItem._to_index();
        bool result = ImGui::Combo(label, &currentIndex,
                                   [](void*, int index, const char** outTxt)
                                   {
                                       *outTxt = BetterEnum_t::_from_index(index)._to_string();
                                       return true;
                                   },
                                   nullptr, (int)BetterEnum_t::_size_constant,
                                   popupMaxHeightInItems);

        currentItem = BetterEnum_t::_from_index(currentIndex);
        return result;
    }
}