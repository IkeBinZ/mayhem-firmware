#include "ui.hpp"
#include "ui_c16blocker.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

namespace ui::external_app::c16blocker {
void initialize_app(ui::NavigationView& nav) {
    nav.push<C16BlockerView>();
}
}  // namespace ui::external_app::c16blocker

extern "C" {
__attribute__((section(".external_app.app_c16blocker.application_information"), used))
application_information_t _application_information_c16blocker = {
    /*.memory_location =*/ (uint8_t*)0x00000000,
    /*.externalAppEntry =*/ ui::external_app::c16blocker::initialize_app,
    /*.header_version =*/ CURRENT_HEADER_VERSION,
    /*.app_version =*/ VERSION_MD5,
    /*.app_name =*/ "C16 Blocker",
    /*.bitmap_data =*/ {
        0xF0, 0x0F, 0xFC, 0x3F, 0x0E, 0x70, 0x07, 0x60,
        0x03, 0xC0, 0x83, 0xC1, 0xC3, 0xC3, 0xE3, 0xC7,
        0xE3, 0xC7, 0xC3, 0xC3, 0x83, 0xC1, 0x03, 0xC0,
        0x07, 0x60, 0x0E, 0x70, 0xFC, 0x3F, 0xF0, 0x0F,
    },
    /*.icon_color =*/ ui::Color::red().v,
    /*.menu_location =*/ app_location_t::RX,
    /*.desired_menu_position =*/ -1,
    /*.m4_app_tag =*/ {0, 0, 0, 0},
    /*.m4_app_offset =*/ 0x00000000,
};
}
