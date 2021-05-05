#include "radio_calibration_impl.h"

#include <functional>
#include <string>

#include "global_include.h"
#include "px4_custom_mode.h"
#include "system.h"

namespace mavsdk {

using namespace std::placeholders;

RadioCalibrationImpl::RadioCalibrationImpl(System& system) : PluginImplBase(system)
{
    _parent->register_plugin(this);
}

RadioCalibrationImpl::RadioCalibrationImpl(std::shared_ptr<System> system) : PluginImplBase(system)
{
    _parent->register_plugin(this);
}

RadioCalibrationImpl::~RadioCalibrationImpl()
{
    _parent->unregister_plugin(this);
}

void RadioCalibrationImpl::init() 
{
    _parent->register_mavlink_message_handler(
        MAVLINK_MSG_ID_STATUSTEXT, std::bind(&RadioCalibrationImpl::process_statustext, this, _1), this);
}

void RadioCalibrationImpl::deinit() 
{
    _parent->unregister_all_mavlink_message_handlers(this);
}

void RadioCalibrationImpl::enable() {}

void RadioCalibrationImpl::disable() {}



void RadioCalibrationImpl::calibrate_radio_async(RadioCalibration::CalibrateRadioCallback callback)
{
    std::lock_guard<std::mutex> lock(_calibration_mutex);

    if (_parent->is_armed()) {
        RadioCalibration::ProgressData progress_data;
        call_callback(callback, RadioCalibration::Result::FailedArmed, progress_data);
        return;
    }

    if (_state != State::None) {
        RadioCalibration::ProgressData progress_data;
        call_callback(callback, RadioCalibration::Result::Busy, progress_data);
        return;
    }

    _state = State::RadioControllerCalibration;
    _calibration_callback = callback;

    MavlinkCommandSender::CommandLong command{};
    command.command = MAV_CMD_PREFLIGHT_CALIBRATION;
    MavlinkCommandSender::CommandLong::set_as_reserved(command.params, 0.0f);
    command.params.param4 = 1.0f; // Remote Control
    command.target_component_id = MAV_COMP_ID_AUTOPILOT1;
    _parent->send_command_async(
        command, std::bind(&RadioCalibrationImpl::command_result_callback, this, _1, _2));
}

void RadioCalibrationImpl::cancel() const
{
    std::lock_guard<std::mutex> lock(_calibration_mutex);

    uint8_t target_component_id = MAV_COMP_ID_AUTOPILOT1;

    switch (_state) {
        case State::None:
            LogWarn() << "No calibration to cancel";
            return;
        case State::RadioControllerCalibration:
            break;
    }

    MavlinkCommandSender::CommandLong command{};
    command.command = MAV_CMD_PREFLIGHT_CALIBRATION;
    // All params 0 signal cancellation of a calibration.
    MavlinkCommandSender::CommandLong::set_as_reserved(command.params, 0.0f);
    command.target_component_id = target_component_id;
    // We don't care about the result, the initial callback should get notified about it.
    _parent->send_command_async(command, nullptr);
}

void RadioCalibrationImpl::call_callback(
    const RadioCalibrationCallback& callback,
    const RadioCalibration::Result& result,
    const RadioCalibration::ProgressData progress_data)
{
    if (callback) {
        _parent->call_user_callback(
            [callback, result, progress_data]() { callback(result, progress_data); });
    }
}

void RadioCalibrationImpl::command_result_callback(
    MavlinkCommandSender::Result command_result, float progress)
{
    std::lock_guard<std::mutex> lock(_calibration_mutex);

    if (_state == State::None) {
        // It might be someone else like a ground station trying to do a
        // calibration. We silently ignore it.
        return;
    }

    // If we get a progress result here, it is the new interface and we can
    // use the progress info. If we get an ack, we need to translate that to
    // a first progress update, and then parse the statustexts for progress.
    switch (command_result) {
        case MavlinkCommandSender::Result::Success:
            // Silently ignore.
            break;

        case MavlinkCommandSender::Result::NoSystem:
            // FALLTHROUGH
        case MavlinkCommandSender::Result::ConnectionError:
            // FALLTHROUGH
        case MavlinkCommandSender::Result::Busy:
            // FALLTHROUGH
        case MavlinkCommandSender::Result::CommandDenied:
            // FALLTHROUGH
        case MavlinkCommandSender::Result::Unsupported:
            // FALLTHROUGH
        case MavlinkCommandSender::Result::UnknownError:
            // FALLTHROUGH
        case MavlinkCommandSender::Result::Timeout: {
            // Report all error cases.
            const auto timeout_result = calibration_result_from_command_result(command_result);
            call_callback(_calibration_callback, timeout_result, RadioCalibration::ProgressData());
            _calibration_callback = nullptr;
            _state = State::None;
            break;
        }

        case MavlinkCommandSender::Result::InProgress: {
            const auto progress_result = calibration_result_from_command_result(command_result);
            RadioCalibration::ProgressData progress_data;
            progress_data.has_progress = true;
            progress_data.progress = progress;

            call_callback(_calibration_callback, progress_result, progress_data);
            break;
        }
    };
}

RadioCalibration::Result
RadioCalibrationImpl::calibration_result_from_command_result(MavlinkCommandSender::Result result)
{
    switch (result) {
        case MavlinkCommandSender::Result::Success:
            return RadioCalibration::Result::Success;
        case MavlinkCommandSender::Result::NoSystem:
            return RadioCalibration::Result::NoSystem;
        case MavlinkCommandSender::Result::ConnectionError:
            return RadioCalibration::Result::ConnectionError;
        case MavlinkCommandSender::Result::Busy:
            return RadioCalibration::Result::Busy;
        case MavlinkCommandSender::Result::CommandDenied:
            return RadioCalibration::Result::CommandDenied;
        case MavlinkCommandSender::Result::Timeout:
            return RadioCalibration::Result::Timeout;
        case MavlinkCommandSender::Result::InProgress:
            return RadioCalibration::Result::Next;
        default:
            return RadioCalibration::Result::Unknown;
    }
}

void RadioCalibrationImpl::process_statustext(const mavlink_message_t& message)
{
    std::lock_guard<std::mutex> lock(_calibration_mutex);
    if (_state == State::None) {
        return;
    }

    mavlink_statustext_t statustext;
    mavlink_msg_statustext_decode(&message, &statustext);

    _parser.reset();

    char text_with_null[sizeof(statustext.text) + 1]{};
    strncpy(text_with_null, statustext.text, sizeof(text_with_null) - 1);
    _parser.parse(text_with_null);

    switch (_parser.get_status()) {
        case RadioCalibrationStatustextParser::Status::None:
            // Ignore it.
            break;
        case RadioCalibrationStatustextParser::Status::Started:
            report_started();
            break;
        case RadioCalibrationStatustextParser::Status::Done:
            report_done();
            break;
        case RadioCalibrationStatustextParser::Status::Failed:
            report_failed(_parser.get_failed_message());
            break;
        case RadioCalibrationStatustextParser::Status::Cancelled:
            report_cancelled();
            break;
        case RadioCalibrationStatustextParser::Status::Progress:
            report_progress(_parser.get_progress());
            break;
        case RadioCalibrationStatustextParser::Status::Instruction:
            report_instruction(_parser.get_instruction());
            break;
    }

    // In case we succeed or fail we need to notify that params
    // might have changed.
    switch (_parser.get_status()) {
        case RadioCalibrationStatustextParser::Status::Done:
            // FALLTHROUGH
        case RadioCalibrationStatustextParser::Status::Failed:
            // FALLTHROUGH
        case RadioCalibrationStatustextParser::Status::Cancelled:
            switch (_state) {
                case State::None:
                    break;
                case State::RadioControllerCalibration:
                    _parent->param_changed("CAL_GYRO0_ID");
                    break;
            }

        default:
            break;
    }

    switch (_parser.get_status()) {
        case RadioCalibrationStatustextParser::Status::Done:
            // FALLTHROUGH
        case RadioCalibrationStatustextParser::Status::Failed:
            // FALLTHROUGH
        case RadioCalibrationStatustextParser::Status::Cancelled:
            _calibration_callback = nullptr;
            _state = State::None;
            break;
        default:
            break;
    }
}

void RadioCalibrationImpl::report_started()
{
    report_progress(0.0f);
}

void RadioCalibrationImpl::report_done()
{
    const RadioCalibration::ProgressData progress_data;
    call_callback(_calibration_callback, RadioCalibration::Result::Success, progress_data);
}

void RadioCalibrationImpl::report_failed(const std::string& failed)
{
    LogErr() << "Calibration failed: " << failed;
    const RadioCalibration::ProgressData progress_data;
    call_callback(_calibration_callback, RadioCalibration::Result::Failed, progress_data);
}

void RadioCalibrationImpl::report_cancelled()
{
    LogWarn() << "Calibration was cancelled";
    const RadioCalibration::ProgressData progress_data;
    call_callback(_calibration_callback, RadioCalibration::Result::Cancelled, progress_data);
}

void RadioCalibrationImpl::report_progress(float progress)
{
    RadioCalibration::ProgressData progress_data;
    progress_data.has_progress = true;
    progress_data.progress = progress;
    call_callback(_calibration_callback, RadioCalibration::Result::Next, progress_data);
}

void RadioCalibrationImpl::report_instruction(const std::string& instruction)
{
    RadioCalibration::ProgressData progress_data;
    progress_data.has_status_text = true;
    progress_data.status_text = instruction;
    call_callback(_calibration_callback, RadioCalibration::Result::Next, progress_data);
}

} // namespace mavsdk