#pragma once

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "baseband_api.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "transmitter_model.hpp"
#include "string_format.hpp"
#include "file.hpp"
#include "capture_thread.hpp"
#include "replay_thread.hpp"
#include "io_file.hpp"
#include "io_convert.hpp"
#include "portapack.hpp"
#include "message.hpp"
#include "event_m0.hpp"
#include "ui_fileman.hpp"

#include <array>
#include <memory>
#include <string>

namespace ui::external_app::c16blocker {

enum class AppState { IDLE, SCANNING, DETECTED, SENDING };

class C16BlockerView : public View {
public:
    explicit C16BlockerView(NavigationView& nav);
    ~C16BlockerView();
    void focus() override;
    std::string title() const override { return "C16 Blocker"; }

private:
    NavigationView& nav_;
    AppState state_{AppState::IDLE};
    bool file_ok_{false};
    bool sending_{false};
    int32_t rssi_val_{-120};
    uint32_t rec_secs_{0};
    uint32_t frame_cnt_{0};
    std::filesystem::path c16_path_{};
    std::string rec_name_{};

    static constexpr uint32_t SR = 500'000;
    static constexpr uint32_t BW = 1'750'000;
    static constexpr uint32_t REC_MAX = 30;
    static constexpr int RSSI_BAR_Y = UI_POS_Y(2) + 2;

    std::unique_ptr<CaptureThread> cap_thd_{};
    std::unique_ptr<ReplayThread>  rep_thd_{};
    bool ready_sig_{false};

    // Row 0
    Text txt_title_  {{UI_POS_X(0), UI_POS_Y(0), UI_POS_WIDTH(16), UI_POS_DEFAULT_HEIGHT}, "C16 BLOCKER+REC"};
    Text txt_badge_  {{UI_POS_X(16),UI_POS_Y(0), UI_POS_WIDTH(14), UI_POS_DEFAULT_HEIGHT}, "[IDLE]"};

    // Row 1 — RSSI
    Text lbl_rssi_   {{UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(5),  UI_POS_DEFAULT_HEIGHT}, "RSSI:"};
    Text txt_rssi_   {{UI_POS_X(5), UI_POS_Y(1), UI_POS_WIDTH(9),  UI_POS_DEFAULT_HEIGHT}, "---dBm"};
    Text lbl_thr_    {{UI_POS_X(14),UI_POS_Y(1), UI_POS_WIDTH(4),  UI_POS_DEFAULT_HEIGHT}, "THR:"};
    NumberField fld_thr_{{UI_POS_X(18),UI_POS_Y(1)}, 4, {-100,-10}, 1, ' '};

    // Row 2 — RSSI bar (painted)

    // Row 3 — Frequency as MHz * 1000 (kHz)
    Text lbl_freq_   {{UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(5),  UI_POS_DEFAULT_HEIGHT}, "FREQ:"};
    NumberField fld_freq_{{UI_POS_X(5), UI_POS_Y(3)}, 7, {1000, 6000000}, 25, '0'};

    // Row 4 — Gains
    Text lbl_lna_    {{UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(4),  UI_POS_DEFAULT_HEIGHT}, "LNA:"};
    NumberField fld_lna_{{UI_POS_X(4), UI_POS_Y(4)}, 2, {0, 40}, 8, '0'};
    Text lbl_vga_    {{UI_POS_X(8), UI_POS_Y(4), UI_POS_WIDTH(4),  UI_POS_DEFAULT_HEIGHT}, "VGA:"};
    NumberField fld_vga_{{UI_POS_X(12),UI_POS_Y(4)}, 2, {0, 62}, 2, '0'};
    Text lbl_txg_    {{UI_POS_X(18),UI_POS_Y(4), UI_POS_WIDTH(4),  UI_POS_DEFAULT_HEIGHT}, "TXG:"};
    NumberField fld_txg_{{UI_POS_X(22),UI_POS_Y(4)}, 2, {0, 47}, 1, '0'};

    // Row 5 — File
    Text lbl_file_   {{UI_POS_X(0), UI_POS_Y(5), UI_POS_WIDTH(5),  UI_POS_DEFAULT_HEIGHT}, "FILE:"};
    Text txt_file_   {{UI_POS_X(5), UI_POS_Y(5), UI_POS_WIDTH(25), UI_POS_DEFAULT_HEIGHT}, "<none>"};
    Button btn_brws_ {{UI_POS_X(0), UI_POS_Y(6), UI_POS_WIDTH(8),  UI_POS_DEFAULT_HEIGHT}, "BROWSE"};

    // Row 7 — Status
    Text txt_status_ {{UI_POS_X(0), UI_POS_Y(7), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, "Idle - press ARM"};

    // Row 8 — Rec info
    Text txt_rec_    {{UI_POS_X(0), UI_POS_Y(8), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, ""};

    // Row 9-10 — Buttons (2 rows tall)
    Button btn_arm_  {{UI_POS_X(0),  UI_POS_Y(9), UI_POS_WIDTH(10), UI_POS_DEFAULT_HEIGHT*2}, "SCAN+ARM"};
    Button btn_send_ {{UI_POS_X(10), UI_POS_Y(9), UI_POS_WIDTH(10), UI_POS_DEFAULT_HEIGHT*2}, "SEND C16"};
    Button btn_stop_ {{UI_POS_X(20), UI_POS_Y(9), UI_POS_WIDTH(10), UI_POS_DEFAULT_HEIGHT*2}, "STOP"};

    // Row 11-13 — Log
    Text txt_log0_   {{UI_POS_X(0), UI_POS_Y(11), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, ""};
    Text txt_log1_   {{UI_POS_X(0), UI_POS_Y(12), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, ""};
    Text txt_log2_   {{UI_POS_X(0), UI_POS_Y(13), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, ""};

    static constexpr int LOG_N = 3;
    std::array<std::string, LOG_N> logs_{};

    void paint(Painter& painter) override;
    void set_state(AppState s);
    void do_arm();
    void do_send();
    void do_stop();
    void start_rx();
    void start_tx();
    void start_capture();
    void stop_capture();
    void start_replay();
    void stop_replay();
    void on_detected();
    void push_log(const std::string& s);
    rf::Frequency freq_hz() const { return (rf::Frequency)fld_freq_.value() * 1000LL; }

    MessageHandlerRegistration msg_stats_{
        Message::ID::ChannelStatistics,
        [this](const Message* const p) {
            rssi_val_ = static_cast<const ChannelStatisticsMessage*>(p)->statistics.max_db;
            txt_rssi_.set(to_string_dec_int(rssi_val_) + "dBm");
            if (state_ == AppState::SCANNING && rssi_val_ > fld_thr_.value())
                on_detected();
            if (state_ == AppState::DETECTED && rssi_val_ < fld_thr_.value() - 10) {
                stop_capture(); stop_replay();
                set_state(AppState::SCANNING);
                start_rx();
                push_log("Signal gone, rescanning");
            }
            set_dirty();
        }};

    MessageHandlerRegistration msg_frame_{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            if (state_ != AppState::DETECTED || !cap_thd_) return;
            if (++frame_cnt_ < 60) return;
            frame_cnt_ = 0;
            txt_rec_.set("REC:" + to_string_dec_uint(++rec_secs_) + "s->" + rec_name_);
            if (rec_secs_ >= REC_MAX) {
                stop_capture(); stop_replay();
                set_state(AppState::SCANNING);
                start_rx();
                push_log("REC max, rescanning");
            }
        }};

    MessageHandlerRegistration msg_replay_{
        Message::ID::ReplayThreadDone,
        [this](const Message* const p) {
            const auto rc = static_cast<const ReplayThreadDoneMessage*>(p)->return_code;
            stop_replay();
            sending_ = false;
            push_log(rc == ReplayThread::END_OF_FILE ? "TX done" : "TX error");
            set_state(AppState::IDLE);
        }};
};

}  // namespace ui::external_app::c16blocker
