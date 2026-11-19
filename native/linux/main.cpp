#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include "frame_filter.hpp"

namespace {
volatile std::sig_atomic_t stopping = 0;
void stop(int) { stopping = 1; }
constexpr const char* virtualPrefix = "Scroll Wheel Fix:";

struct Device {
    int fd = -1;
    libevdev* input = nullptr;
    libevdev_uinput* output = nullptr;
    bool grabbed = false;
    explicit Device(const std::string& path) {
        fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) throw std::runtime_error("Cannot open selected device: " + std::string(std::strerror(errno)));
        const int result = libevdev_new_from_fd(fd, &input);
        if (result < 0) { close(fd); fd = -1; throw std::runtime_error("Cannot inspect selected input device"); }
    }
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    ~Device() {
        if (grabbed) libevdev_grab(input, LIBEVDEV_UNGRAB);
        if (output) libevdev_uinput_destroy(output);
        if (input) libevdev_free(input);
        if (fd >= 0) close(fd);
    }
    void write(const input_event& event) {
        const int result = libevdev_uinput_write_event(output, event.type, event.code, event.value);
        if (result < 0) throw std::runtime_error("Virtual mouse write failed; releasing physical mouse");
    }
    void report(const swf::Frame& frame) {
        if (frame.empty()) return;
        for (const auto& event : frame) write(event);
        input_event sync{}; sync.type = EV_SYN; sync.code = SYN_REPORT;
        write(sync);
    }
};
bool eligible(const libevdev* input) {
    const char* name = libevdev_get_name(input);
    if (name && std::string(name).rfind(virtualPrefix, 0) == 0) return false;
    if (!libevdev_has_event_code(input, EV_REL, REL_X) ||
        !libevdev_has_event_code(input, EV_REL, REL_Y) ||
        !libevdev_has_event_code(input, EV_KEY, BTN_LEFT) ||
        (!libevdev_has_event_code(input, EV_REL, REL_WHEEL) &&
         !libevdev_has_event_code(input, EV_REL, REL_WHEEL_HI_RES))) return false;
    if (libevdev_has_event_type(input, EV_ABS)) return false;
    // Refuse keyboard/composite event nodes rather than proxying typed keys.
    for (unsigned code = 0; code <= KEY_MAX; ++code)
        if ((code < BTN_MISC || code >= BTN_JOYSTICK) &&
            libevdev_has_event_code(input, EV_KEY, code)) return false;
    return true;
}
int listDevices() {
    std::vector<std::filesystem::path> paths;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator("/dev/input", error))
        if (entry.path().filename().string().rfind("event", 0) == 0) paths.push_back(entry.path());
    std::sort(paths.begin(), paths.end());
    unsigned found = 0;
    for (const auto& path : paths) {
        try {
            Device device(path.string());
            if (eligible(device.input)) {
                const char* name = libevdev_get_name(device.input);
                std::cout << path.string() << "  " << (name ? name : "Unnamed mouse") << '\n';
                ++found;
            }
        } catch (const std::runtime_error&) { /* inaccessible node: never read its events */ }
    }
    if (!found) std::cerr << "No accessible mouse-only relative wheel devices. Check permissions or run --list with sudo.\n";
    return found ? 0 : 1;
}
void usage() {
    std::cout << "Scroll Wheel Fix " << swf::version << "\n"
        "Usage: scroll-wheel-fix --device /dev/input/by-id/...-event-mouse [--mode balanced|strong]\n"
        "       scroll-wheel-fix --list | --help | --version\n\n"
        "Filters one explicitly selected relative mouse using evdev + uinput (Wayland or X11).\n"
        "Requires read access to that device and write access to /dev/uinput, usually sudo.\n"
        "No permission rules, services, or startup entries are installed. Ctrl+C stops and releases the mouse.\n"
        "Keyboard/composite nodes and absolute devices are rejected. This masks noisy input, not hardware damage.\n";
}
int run(const std::string& path, double window) {
    Device device(path);
    if (!eligible(device.input)) throw std::runtime_error("Selected node is not an eligible mouse-only relative wheel device");
    for (unsigned code = BTN_MISC; code < BTN_JOYSTICK; ++code)
        if (libevdev_get_event_value(device.input, EV_KEY, code) != 0)
            throw std::runtime_error("Release all mouse buttons before starting the filter");
    if (libevdev_set_clock_id(device.input, CLOCK_MONOTONIC) < 0)
        throw std::runtime_error("Could not select a monotonic input clock; no device was grabbed");
    const char* original = libevdev_get_name(device.input);
    const std::string name = std::string(virtualPrefix) + " " + (original ? original : "Mouse");
    libevdev_set_name(device.input, name.c_str());
    libevdev_set_phys(device.input, "scroll-wheel-fix");
    libevdev_set_uniq(device.input, "");
    libevdev_set_id_bustype(device.input, BUS_VIRTUAL);
    libevdev_set_id_vendor(device.input, 0);
    libevdev_set_id_product(device.input, 0);
    const int created = libevdev_uinput_create_from_device(device.input, LIBEVDEV_UINPUT_OPEN_MANAGED, &device.output);
    if (created < 0) throw std::runtime_error("Cannot create virtual mouse through /dev/uinput; check uinput and permissions");
    if (libevdev_grab(device.input, LIBEVDEV_GRAB) < 0)
        throw std::runtime_error("Cannot grab selected mouse (already in use?). No filtering started");
    device.grabbed = true;
    swf::FrameFilter filter(window);
    swf::Frame frame;
    std::cout << "Filtering selected mouse with " << static_cast<int>(window * 1000)
        << " ms confirmation. Ctrl+C stops and releases it.\n" << std::flush;
    pollfd descriptor{device.fd, POLLIN, 0};
    while (!stopping) {
        const int ready = poll(&descriptor, 1, 100);
        if (ready < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("Polling selected mouse failed");
        }
        if (!ready) continue;
        if (descriptor.revents & (POLLHUP | POLLERR | POLLNVAL))
            throw std::runtime_error("Selected mouse disconnected; filter stopped");
        input_event event{};
        int status;
        while (!stopping && (status = libevdev_next_event(device.input, LIBEVDEV_READ_FLAG_NORMAL, &event)) != -EAGAIN) {
            if (status == -EINTR) { if (stopping) break; else continue; }
            if (status == LIBEVDEV_READ_STATUS_SYNC) {
                frame.clear(); filter.bursts.reset();
                swf::Frame sync;
                while (libevdev_next_event(device.input, LIBEVDEV_READ_FLAG_SYNC, &event) == LIBEVDEV_READ_STATUS_SYNC)
                    if (event.type != EV_SYN) sync.push_back(event);
                device.report(sync);
                continue;
            }
            if (status < 0) throw std::runtime_error("Reading selected mouse failed; releasing it");
            if (event.type == EV_SYN && event.code == SYN_REPORT) {
                const double time = static_cast<double>(event.time.tv_sec) + event.time.tv_usec / 1e6;
                auto output = filter.process(frame, time);
                device.report(output.immediate);
                for (const auto& wheel : output.wheel) device.report(wheel);
                frame.clear();
            } else {
                frame.push_back(event);
                if (frame.size() > 4096) throw std::runtime_error("Malformed input frame; releasing selected mouse");
            }
        }
    }
    filter.bursts.reset();
    std::cout << "Stopped. Wheel frames: " << filter.bursts.seen
        << "; suppressed: " << filter.bursts.suppressed << ".\n";
    return 0;
}
} // namespace

int main(int argc, char** argv) {
    std::string device;
    double window = .080;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--help") { usage(); return 0; }
        if (argument == "--version") { std::cout << swf::version << '\n'; return 0; }
        if (argument == "--list" && argc == 2) return listDevices();
        if (argument == "--device" && i + 1 < argc) { device = argv[++i]; continue; }
        if (argument == "--mode" && i + 1 < argc) {
            const std::string mode = argv[++i];
            if (mode == "balanced") window = .080;
            else if (mode == "strong") window = .140;
            else { std::cerr << "Mode must be balanced or strong\n"; return 2; }
            continue;
        }
        std::cerr << "Unknown or incomplete argument: " << argument << '\n';
        usage(); return 2;
    }
    if (device.empty()) { usage(); return 2; }
    std::signal(SIGINT, stop); std::signal(SIGTERM, stop); std::signal(SIGHUP, stop);
    try { return run(device, window); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
