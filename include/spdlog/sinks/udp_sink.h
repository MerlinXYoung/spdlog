// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/common.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>
#ifdef _WIN32
#include <spdlog/details/udp_client-windows.h>
#else
#include <spdlog/details/udp_client.h>
#endif

#include <mutex>
#include <string>
#include <chrono>
#include <functional>

#pragma once

// Simple tcp client sink
// Connects to remote address and send the formatted log.
// Will attempt to reconnect if connection drops.
// If more complicated behaviour is needed (i.e get responses), you can inherit it and override the sink_it_ method.

namespace spdlog {
namespace sinks {

struct udp_sink_config
{
    std::string server_host;
    int server_port;
    // bool lazy_connect = false; // if true connect on first log call instead of on construction

    udp_sink_config(std::string host, int port)
        : server_host{std::move(host)}
        , server_port{port}
    {}
};

template<typename Mutex>
class udp_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
    // connect to tcp host/port or throw if failed
    // host can be hostname or ip address

    explicit udp_sink(udp_sink_config sink_config)
        : config_{std::move(sink_config)}
    {
        this->client_.connect(config_.server_host, config_.server_port);
    }

    ~udp_sink() = default;

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override
    {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

        client_.send(formatted.data(), formatted.size());
    }

    void flush_() override {}
    udp_sink_config config_;
    details::udp_client client_;
};

using udp_sink_mt = udp_sink<std::mutex>;
using udp_sink_st = udp_sink<spdlog::details::null_mutex>;

} // namespace sinks
} // namespace spdlog
