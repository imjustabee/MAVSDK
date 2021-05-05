#pragma once

#include "plugins/radio_calibration/radio_calibration.h"
#include "radio_calibration_statustext_parser.h"
#include "mavlink_include.h"
#include "plugin_impl_base.h"

namespace mavsdk {

class RadioCalibrationImpl : public PluginImplBase {
public:
    explicit RadioCalibrationImpl(System& system);
    explicit RadioCalibrationImpl(std::shared_ptr<System> system);
    ~RadioCalibrationImpl();

    void init() override;
    void deinit() override;

    void enable() override;
    void disable() override;

    void cancel() const;



    void calibrate_radio_async(const RadioCalibration::CalibrateRadioCallback& callback);



private:
    typedef std::function<void(const RadioCalibration::Result result, const RadioCalibration::ProgressData)>
        RadioCalibrationCallback;

    void call_callback(
        const RadioCalibrationCallback& callback,
        const RadioCalibration::Result& result,
        const RadioCalibration::ProgressData progress_data);
    void process_statustext(const mavlink_message_t& message);

    void command_result_callback(MavlinkCommandSender::Result command_result, float progress);

    static RadioCalibration::Result
    calibration_result_from_command_result(MavlinkCommandSender::Result result);

    void report_started();
    void report_done();
    void report_warning(const std::string& warning);
    void report_failed(const std::string& failed);
    void report_cancelled();
    void report_progress(float progress);
    void report_instruction(const std::string& instruction);

    RadioCalibrationStatustextParser _parser{};

    mutable std::mutex _calibration_mutex{};

    enum class State {
        None,
        RadioControllerCalibration
    } _state{State::None};

    RadioCalibrationCallback _calibration_callback{nullptr};
};

} // namespace mavsdk