#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/radio_calibration/radio_calibration.h>
#include <future>
#include <iostream>

using namespace mavsdk;

bool are_arguments_valid(int argc, char** argv);
void print_usage(const std::string&);
void wait_until_discover(Mavsdk&);
std::function<void(RadioCalibration::Result, RadioCalibration::ProgressData)>
create_calibration_callback(std::promise<void>&);
void calibrate_radio(RadioCalibration&);

int main(int argc, char** argv)
{
    if (!are_arguments_valid(argc, argv)) {
        const auto binary_name = argv[0];
        print_usage(binary_name);
        return 1;
    }

    Mavsdk mavsdk;

    const auto connection_url = argv[1];
    const auto connection_result = mavsdk.add_any_connection(connection_url);

    if (connection_result != ConnectionResult::Success) {
        std::cout << "Connection failed: " << connection_result << std::endl;
        return 1;
    }

    wait_until_discover(mavsdk);

    Calibration calibration(mavsdk.systems().at(0));
    calibrate_radio(calibration);

    return 0;
}

bool are_arguments_valid(int argc, char** /* argv */)
{
    return argc == 2;
}

void print_usage(const std::string& bin_name)
{
    std::cout << "Usage : " << bin_name << " <connection_url>" << std::endl
              << "Connection URL format should be :" << std::endl
              << " For TCP : tcp://[server_host][:server_port]" << std::endl
              << " For UDP : udp://[bind_host][:bind_port]" << std::endl
              << " For Serial : serial:///path/to/serial/dev[:baudrate]" << std::endl
              << "For example, to connect to the simulator use URL: udp://:14540" << std::endl;
}

void wait_until_discover(Mavsdk& mavsdk)
{
    std::cout << "Waiting to discover system..." << std::endl;
    std::promise<void> discover_promise;
    auto discover_future = discover_promise.get_future();

    mavsdk.subscribe_on_new_system([&mavsdk, &discover_promise]() {
        const auto system = mavsdk.systems().at(0);

        if (system->is_connected()) {
            std::cout << "Discovered system" << std::endl;
            discover_promise.set_value();
        }
    });

    discover_future.wait();
}

void calibrate_radio(RadioCalibration& calibration)
{
    std::cout << "Calibrating radio..." << std::endl;

    std::promise<void> calibration_promise;
    auto calibration_future = calibration_promise.get_future();

    calibration.calibrate_radio_async(create_calibration_callback(calibration_promise));

    calibration_future.wait();
}

std::function<void(RadioCalibration::Result, RadioCalibration::ProgressData)>
create_calibration_callback(std::promise<void>& calibration_promise)
{
    return [&calibration_promise](
               const RadioCalibration::Result result, const RadioCalibration::ProgressData progress_data) {
        switch (result) {
            case RadioCalibration::Result::Success:
                std::cout << "--- Calibration succeeded!" << std::endl;
                calibration_promise.set_value();
                break;
            case RadioCalibration::Result::Next:
                if (progress_data.has_progress) {
                    std::cout << "    Progress: " << progress_data.progress << std::endl;
                }
                if (progress_data.has_status_text) {
                    std::cout << "    Instruction: " << progress_data.status_text << std::endl;
                }
                break;
            default:
                std::cout << "--- Calibration failed with message: " << result << std::endl;
                calibration_promise.set_value();
                break;
        }
    };
}
