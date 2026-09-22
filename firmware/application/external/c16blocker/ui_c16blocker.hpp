/*
 * C16 Blocker + Recorder — Mayhem External App
 * Scans frequency, detects unknown signals, jams + records them
 */
#pragma once

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_spectrum.hpp"
#include "baseband_api.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "transmitter_model.hpp"
#include "string_format.hpp"
#include "file.hpp"
#include "capture_thread.hpp"
#include "replay_thread.hpp"
#include "app_settings.hpp"
#include "portapack_persistent_memory.hpp"
#include "message.hpp"
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
    AppState        state_     { AppState::IDLE };
    bool            file_ok_   { false };
    bool            sending_   { false };
    int32_t         rssi_val_  { -120 };
    uint32_t        rec_secs_  { 0 };
    uint32_t        frame_cnt_ { 0 };
    std::filesystem::path c16_path_{};
    std::string rec_name_{};

    static constexpr uint32_t SAMPLE_RATE  = 500'000;
    static constexpr uint32_t BW           = 1'750'000;
    static constexpr uint32_t REC_MAX_SEC  = 30;

    RxRadioState rx_state_{};

    std::unique_ptr<CaptureThread> cap_thd_{};
    std::unique_ptr<ReplayThread>  rep_thd_{};

    // ── Widgets ──────────────────────────────────────────────────────────────

    // Row 0: title + state badge
    Text txt_title_  {{UI_POS_X(0),  UI_POS_Y(0), UI_POS_WIDTH(16), UI_POS_DEFAULT_HEIGHT}, "C16 BLOCKER+REC"};
    Text txt_badge_  {{UI_POS_X(16), UI_POS_Y(0), UI_POS_WIDTH(14), UI_POS_DEFAULT_HEIGHT}, "[IDLE]"};

    // Row 1: RSSI live
    Text lbl_rssi_   {{UI_POS_X(0),  UI_POS_Y(1), UI_POS_WIDTH(5),  UI_POS_DEFAULT_HEIGHT}, "RSSI:"};
    Text txt_rssi_   {{UI_POS_X(5),  UI_POS_Y(1), UI_POS_WIDTH(9),  UI_POS_DEFAULT_HEIGHT}, "---dBm"};
    Text lbl_thr_    {{UI_POS_X(14), UI_POS_Y(1), UI_POS_WIDTH(4),  UI_POS_DEFAULT_HEIGHT}, "THR:"};
    NumberField fld_thr_{{UI_POS_X(18), UI_POS_Y(1)}, 4, {-100, -10}, 1, ' '};

    // Row 2: RSSI bar (drawn in paint)
    // Row 3: Frequency
    Text lbl_freq_   {{UI_POS_X(0),  UI_POS_Y(3), UI_POS_WIDTH(5),  UI_POS_DEFAULT_HEIGHT}, "FREQ:"};
    FrequencyField fld_freq_{{UI_POS_X(5), UI_POS_Y(3)}};

    // Row 4: Gain
    Text lbl_lna_    {{UI_POS_X(0),  UI_POS_Y(4), UI_POS_WIDTH(4),  UI_POS_DEFAULT_HEIGHT}, "LNA:"};
    LNAGainField  fld_lna_ {{UI_POS_X(4),  UI_POS_Y(4)}};
    Text lbl_vga_    {{UI_POS_X(8),  UI_POS_Y(4), UI_POS_WIDTH(4),  UI_POS_DEFAULT_HEIGHT}, "VGA:"};
    VGAGainField  fld_vga_ {{UI_POS_X(12), UI_POS_Y(4)}};
    Text lbl_txg_    {{UI_POS_X(18), UI_POS_Y(4), UI_POS_WIDTH(4),  UI_POS_DEFAULT_HEIGHT}, "TXG:"};
    NumberField fld_txg_{{UI_POS_X(22), UI_POS_Y(4)}, 2, {0, 47}, 1, '0'};

    // Row 5: File
    Text lbl_file_   {{UI_POS_X(0),  UI_POS_Y(5), UI_POS_WIDTH(5),  UI_POS_DEFAULT_HEIGHT}, "FILE:"};
    Text txt_file_   {{UI_POS_X(5),  UI_POS_Y(5), UI_POS_WIDTH(25), UI_POS_DEFAULT_HEIGHT}, "<none>"};
    Button btn_brws_ {{UI_POS_X(0),  UI_POS_Y(6), UI_POS_WIDTH(8),  UI_POS_DEFAULT_HEIGHT}, "BROWSE"};

    // Row 7: Status line
    Text txt_status_ {{UI_POS_X(0),  UI_POS_Y(7), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, "Idle — press ARM"};

    // Row 8: Record info
    Text txt_rec_    {{UI_POS_X(0),  UI_POS_Y(8), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, ""};

    // Row 9-10: Big buttons
    Button btn_arm_  {{UI_POS_X(0),  UI_POS_Y(9),  UI_POS_WIDTH(10), UI_POS_DEFAULT_HEIGHT*2}, "SCAN+ARM"};
    Button btn_send_ {{UI_POS_X(10), UI_POS_Y(9),  UI_POS_WIDTH(10), UI_POS_DEFAULT_HEIGHT*2}, "SEND C16"};
    Button btn_stop_ {{UI_POS_X(20), UI_POS_Y(9),  UI_POS_WIDTH(10), UI_POS_DEFAULT_HEIGHT*2}, "STOP"};

    // Row 11-13: Log
    Text txt_log0_   {{UI_POS_X(0), UI_POS_Y(11), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, ""};
    Text txt_log1_   {{UI_POS_X(0), UI_POS_Y(12), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, ""};
    Text txt_log2_   {{UI_POS_X(0), UI_POS_Y(13), UI_POS_WIDTH(30), UI_POS_DEFAULT_HEIGHT}, ""};

    static constexpr int LOG_N = 3;
    std::array<std::string, LOG_N> logs_{};

    // ── Private methods ──────────────────────────────────────────────────────
    void paint(Painter& painter) override;
    void set_state(AppState s);
    void do_arm();
    void do_send();
    void do_stop();
    void start_rx(rf::Frequency freq);
    void start_tx(rf::Frequency freq);
    void start_capture(rf::Frequency freq);
    void stop_capture();
    void start_replay(rf::Frequency freq);
    void stop_replay();
    void push_log(const std::string& s);

    // RSSI message handler
    MessageHandlerRegistration msg_stats_{
        Message::ID::ChannelStatistics,
        [this](const Message* const p) {
            const auto& stats = static_cast<const ChannelStatisticsMessage*>(p)->statistics;
            rssi_val_ = stats.max_db;

            // Update RSSI display
            txt_rssi_.set(to_string_dec_int(rssi_val_) + "dBm");

            // Check threshold
            if (state_ == AppState::SCANNING && rssi_val_ > fld_thr_.value()) {
                on_signal_detected();
            }

            // Auto-stop recording if signal gone
            if (state_ == AppState::DETECTED && rssi_val_ < fld_thr_.value() - 10) {
                push_log("Signal gone, rescan");
                stop_capture();
                stop_replay();
                set_state(AppState::SCANNING);
                start_rx(fld_freq_.value());
            }
            set_dirty();
        }
    };

    // Frame sync for rec timer
    MessageHandlerRegistration msg_frame_{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            if (state_ == AppState::DETECTED && cap_thd_) {
                frame_cnt_++;
                if (frame_cnt_ >= 60) {   // ~60 frames = 1 second
                    frame_cnt_ = 0;
                    rec_secs_++;
                    txt_rec_.set("REC: " + to_string_dec_uint(rec_secs_) + "s -> " + rec_name_);
                    if (rec_secs_ >= REC_MAX_SEC) {
                        push_log("REC max, back to scan");
                        stop_capture();
                        stop_replay();
                        set_state(AppState::SCANNING);
                        start_rx(fld_freq_.value());
                    }
                }
            }
        }
    };

    // Replay done handler
    MessageHandlerRegistration msg_replay_done_{
        Message::ID::ReplayThreadDone,
        [this](const Message* const p) {
            const auto msg = static_cast<const ReplayThreadDoneMessage*>(p);
            if (msg->return_code == ReplayThread::END_OF_FILE) {
                push_log("C16 TX done");
            } else {
                push_log("TX error");
            }
            stop_replay();
            sending_ = false;
            set_state(AppState::IDLE);
        }
    };

    void on_signal_detected();
};

}  // namespace ui::external_app::c16blocker
