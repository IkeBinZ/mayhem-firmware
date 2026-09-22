#include "ui_c16blocker.hpp"
using namespace portapack;

namespace ui::external_app::c16blocker {

C16BlockerView::C16BlockerView(NavigationView& nav) : nav_{nav} {
    add_children({
        &txt_title_, &txt_badge_,
        &lbl_rssi_,  &txt_rssi_, &lbl_thr_, &fld_thr_,
        &lbl_freq_,  &fld_freq_,
        &lbl_lna_,   &fld_lna_,
        &lbl_vga_,   &fld_vga_,
        &lbl_txg_,   &fld_txg_,
        &lbl_file_,  &txt_file_, &btn_brws_,
        &txt_status_, &txt_rec_,
        &btn_arm_, &btn_send_, &btn_stop_,
        &txt_log0_, &txt_log1_, &txt_log2_,
    });

    fld_freq_.set_value(433920);  // 433.920 MHz in kHz
    fld_lna_.set_value(32);
    fld_vga_.set_value(16);
    fld_txg_.set_value(40);
    fld_thr_.set_value(-60);

    btn_brws_.on_select = [this](Button&) {
        auto* v = nav_.push<FileLoadView>(".C16");
        v->on_changed = [this](std::filesystem::path p) {
            c16_path_ = p;
            auto n = p.filename().string();
            if (n.size() > 22) n = n.substr(0, 19) + "...";
            txt_file_.set(n);
            File f;
            auto err = f.open(p);
            file_ok_ = !err.is_valid();
            push_log(file_ok_ ? "Loaded: " + n : "ERR: open failed");
            set_dirty();
        };
    };

    btn_arm_.on_select  = [this](Button&) { do_arm();  };
    btn_send_.on_select = [this](Button&) { do_send(); };
    btn_stop_.on_select = [this](Button&) { do_stop(); };

    baseband::run_image(portapack::spi_flash::image_tag_capture);
    push_log("C16 Blocker+Rec");
    push_log("Set freq+thr, press ARM");
}

C16BlockerView::~C16BlockerView() {
    do_stop();
    receiver_model.disable();
    transmitter_model.disable();
}

void C16BlockerView::focus() { btn_arm_.focus(); }

void C16BlockerView::paint(Painter& painter) {
    View::paint(painter);
    // Separator lines
    painter.draw_hline({0, UI_POS_Y(2)},   240, Theme::getInstance()->bg_darkest->foreground);
    painter.draw_hline({0, UI_POS_Y(7)},   240, Theme::getInstance()->bg_darkest->foreground);
    painter.draw_hline({0, UI_POS_Y(9)},   240, Theme::getInstance()->bg_darkest->foreground);
    painter.draw_hline({0, UI_POS_Y(11)},  240, Theme::getInstance()->bg_darkest->foreground);

    // RSSI bar
    painter.fill_rectangle({0, RSSI_BAR_Y, 240, 6},
        Theme::getInstance()->bg_darkest->foreground);
    int norm = rssi_val_ + 120;
    if (norm < 0)   norm = 0;
    if (norm > 120) norm = 120;
    int bw = norm * 240 / 120;
    if (bw > 0) {
        Color c = (state_ == AppState::DETECTED)
            ? Theme::getInstance()->fg_red->foreground
            : (state_ == AppState::SCANNING)
            ? Theme::getInstance()->fg_green->foreground
            : Theme::getInstance()->fg_yellow->foreground;
        painter.fill_rectangle({0, RSSI_BAR_Y, bw, 6}, c);
    }
}

void C16BlockerView::set_state(AppState s) {
    state_ = s;
    switch (s) {
    case AppState::IDLE:
        txt_badge_.set("[IDLE]");
        txt_badge_.set_style(Theme::getInstance()->fg_medium);
        txt_status_.set("Idle - press SCAN+ARM");
        txt_status_.set_style(Theme::getInstance()->fg_medium);
        btn_arm_.set_text("SCAN+ARM");
        break;
    case AppState::SCANNING:
        txt_badge_.set("[SCAN]");
        txt_badge_.set_style(Theme::getInstance()->fg_green);
        txt_status_.set("Scanning...");
        txt_status_.set_style(Theme::getInstance()->fg_green);
        btn_arm_.set_text("[ARMED]");
        break;
    case AppState::DETECTED:
        txt_badge_.set("[JAM+REC]");
        txt_badge_.set_style(Theme::getInstance()->fg_red);
        txt_status_.set("DETECTED! JAM+REC");
        txt_status_.set_style(Theme::getInstance()->fg_red);
        break;
    case AppState::SENDING:
        txt_badge_.set("[TX C16]");
        txt_badge_.set_style(Theme::getInstance()->fg_yellow);
        txt_status_.set("Sending C16...");
        txt_status_.set_style(Theme::getInstance()->fg_yellow);
        break;
    }
    set_dirty();
}

void C16BlockerView::do_arm() {
    if (state_ == AppState::SCANNING || state_ == AppState::DETECTED) {
        do_stop(); return;
    }
    start_rx();
    set_state(AppState::SCANNING);
    push_log("Armed @ " + to_string_dec_uint((uint32_t)(freq_hz() / 1000000)) + "MHz");
}

void C16BlockerView::do_send() {
    if (!file_ok_) { push_log("No file loaded!"); return; }
    if (sending_)  { stop_replay(); sending_ = false; set_state(AppState::IDLE); return; }
    if (state_ == AppState::DETECTED) return;
    sending_ = true;
    start_tx();
    start_replay();
    set_state(AppState::SENDING);
}

void C16BlockerView::do_stop() {
    stop_capture();
    stop_replay();
    receiver_model.disable();
    transmitter_model.disable();
    rec_secs_ = 0; frame_cnt_ = 0; sending_ = false;
    txt_rec_.set("");
    set_state(AppState::IDLE);
    push_log("Stopped.");
}

void C16BlockerView::start_rx() {
    transmitter_model.disable();
    baseband::run_image(portapack::spi_flash::image_tag_capture);
    receiver_model.set_target_frequency(freq_hz());
    receiver_model.set_lna(fld_lna_.value());
    receiver_model.set_vga(fld_vga_.value());
    receiver_model.set_rf_amp(false);
    receiver_model.set_sampling_rate(SR);
    receiver_model.set_baseband_bandwidth(BW);
    receiver_model.enable();
}

void C16BlockerView::start_tx() {
    receiver_model.disable();
    baseband::run_image(portapack::spi_flash::image_tag_replay);
    transmitter_model.set_target_frequency(freq_hz());
    transmitter_model.set_tx_gain(fld_txg_.value());
    transmitter_model.set_rf_amp(true);
    transmitter_model.set_sampling_rate(SR);
    transmitter_model.set_channel_bandwidth(BW);
    transmitter_model.enable();
}

void C16BlockerView::start_capture() {
    rec_name_ = "det_" + to_string_dec_uint((uint32_t)(freq_hz() / 1000)) + ".C16";
    auto path = std::filesystem::path(u"/CAPTURES") / rec_name_;
    auto writer = std::make_unique<FileWriter>();
    if (writer->create(path).is_valid()) { push_log("SD err!"); return; }
    cap_thd_ = std::make_unique<CaptureThread>(
        std::move(writer), 0x4000, 4, [](){}, [](File::Error){});
    rec_secs_ = 0; frame_cnt_ = 0;
    txt_rec_.set("REC:0s->" + rec_name_);
    push_log("REC->" + rec_name_);
}

void C16BlockerView::stop_capture() {
    if (cap_thd_) {
        cap_thd_.reset();
        if (!rec_name_.empty()) push_log("Saved:" + rec_name_);
    }
    txt_rec_.set("");
    rec_name_.clear();
}

void C16BlockerView::start_replay() {
    if (!file_ok_) return;
    // Try jam_noise.C16 first
    std::filesystem::path jam{u"/CAPTURES/jam_noise.C16"};
    File chk;
    auto& path = chk.open(jam).is_valid() ? c16_path_ : jam;
    auto reader = std::make_unique<FileReader>();
    if (reader->open(path).is_valid()) { push_log("ERR:open replay"); return; }
    ready_sig_ = false;
    rep_thd_ = std::make_unique<ReplayThread>(
        std::move(reader), 0x4000, 3, &ready_sig_,
        [](uint32_t rc) {
            ReplayThreadDoneMessage msg{rc};
            EventDispatcher::send_message(msg);
        });
    push_log("JAM/TX active");
}

void C16BlockerView::stop_replay() {
    if (rep_thd_) rep_thd_.reset();
}

void C16BlockerView::on_detected() {
    set_state(AppState::DETECTED);
    push_log("DETECT " +
        to_string_dec_uint((uint32_t)(freq_hz()/1000000)) + "." +
        to_string_dec_uint((uint32_t)((freq_hz()%1000000)/1000)) +
        "MHz " + to_string_dec_int(rssi_val_) + "dBm");
    start_capture();
    start_tx();
    start_replay();
}

void C16BlockerView::push_log(const std::string& s) {
    for (int i = LOG_N-1; i > 0; --i) logs_[i] = logs_[i-1];
    logs_[0] = s.size() > 29 ? s.substr(0,29) : s;
    txt_log0_.set(logs_[0]);
    txt_log1_.set(logs_[1]);
    txt_log2_.set(logs_[2]);
}

}  // namespace ui::external_app::c16blocker
