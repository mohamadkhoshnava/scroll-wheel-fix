#pragma once
#include <cstddef>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace swf {
inline constexpr const char* version = "0.2.0";
enum class Decision { pass, hold, confirm };

// Keep the same thresholds/semantics as Sources/DirectionGate.swift.
class DirectionGate {
public:
    explicit DirectionGate(double window = 0.080) : window_(window) {}
    Decision process(int sign, double time) {
        if (sign == 0) return Decision::pass;
        if (direction_ == 0 || time - lastAccepted_ > 0.60) {
            direction_ = sign;
            lastAccepted_ = time;
            candidateStart_.reset();
            candidateCount_ = 0;
            return Decision::pass;
        }
        if (sign == direction_) {
            lastAccepted_ = time;
            candidateStart_.reset();
            candidateCount_ = 0;
            return Decision::pass;
        }
        if (!candidateStart_) candidateStart_ = time;
        ++candidateCount_;
        if (candidateCount_ >= 2 && time - *candidateStart_ >= window_) {
            direction_ = sign;
            lastAccepted_ = time;
            candidateStart_.reset();
            candidateCount_ = 0;
            return Decision::confirm;
        }
        return Decision::hold;
    }
    void reset() {
        direction_ = 0;
        lastAccepted_ = -std::numeric_limits<double>::infinity();
        candidateStart_.reset();
        candidateCount_ = 0;
    }
    void setWindow(double window) { reset(); window_ = window; }
    double window() const { return window_; }
private:
    int direction_ = 0;
    double lastAccepted_ = -std::numeric_limits<double>::infinity();
    std::optional<double> candidateStart_;
    std::size_t candidateCount_ = 0;
    double window_;
};

// Buffer only wheel payloads, never pointer movement or button events.
template<class Payload> class BurstFilter {
public:
    explicit BurstFilter(double window = 0.080) : gate_(window) { pending_.reserve(128); }
    std::vector<Payload> process(int sign, double time, Payload payload) {
        ++seen;
        switch (gate_.process(sign, time)) {
        case Decision::pass:
            discardPending();
            return {std::move(payload)};
        case Decision::hold:
            if (pending_.size() < 128) pending_.push_back(std::move(payload));
            else ++suppressed;
            return {};
        case Decision::confirm: {
            auto result = std::move(pending_);
            pending_.clear();
            pending_.reserve(128);
            result.push_back(std::move(payload));
            return result;
        }
        }
        return {};
    }
    void reset() { discardPending(); gate_.reset(); }
    void setWindow(double window) { discardPending(); gate_.setWindow(window); }
    double window() const { return gate_.window(); }
    std::size_t pendingCount() const { return pending_.size(); }
    std::size_t seen = 0;
    std::size_t suppressed = 0;
private:
    void discardPending() { suppressed += pending_.size(); pending_.clear(); }
    DirectionGate gate_;
    std::vector<Payload> pending_;
};
} // namespace swf
