// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/common.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>
#ifdef _WIN32
#include <spdlog/details/tcp_client-windows.h>
#else
#include <spdlog/details/tcp_client.h>
#endif

#include <mutex>
#include <string>
// #include <chrono>
// #include <functional>
#include <list>

#pragma once

// Simple tcp client sink
// Connects to remote address and send the formatted log.
// Will attempt to reconnect if connection drops.
// If more complicated behaviour is needed (i.e get responses), you can inherit it and override the sink_it_ method.

namespace spdlog {
namespace sinks {

struct tcp_sink_config
{
    std::string server_host;
    int server_port;
    bool lazy_connect = false; // if true connect on first log call instead of on construction
    bool force_flush_ = false;
    unsigned short buffers_size_ = 64;

    tcp_sink_config(std::string host, int port)
        : server_host{std::move(host)}
        , server_port{port}
    {}
};

template<typename Mutex>
class tcp_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
    // connect to tcp host/port or throw if failed
    // host can be hostname or ip address

    explicit tcp_sink(tcp_sink_config sink_config)
        : config_{std::move(sink_config)}
    {
        if (!config_.lazy_connect)
        {
            this->client_.connect(config_.server_host, config_.server_port);
        }
    }

    ~tcp_sink() override = default;

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override
    {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        if(config_.force_flush_){
            if (!client_.is_connected())
            {
                connect();
            }
            send(formatted);
        }else{
            buffers_.emplace_back(std::move(formatted));
        }
    }

    void flush_() override {
        if (!client_.is_connected())
        {
            connect();
        }
        while(!buffers_.empty() && client_.is_connected()){
            send();
        }
    }

    inline void connect(){ 
        try{
            if (!client_.is_connected())
            {
                client_.connect(config_.server_host, config_.server_port);
            }
        }catch(std::exception& e){

        }catch(...){ 

        }
    }
    inline void send(spdlog::memory_buf_t& formatted){ 
        if (client_.is_connected()){
            try{
                client_.send(formatted.data(), formatted.size());
            }catch(std::exception& e){
            }catch(...){ 
            }
        }else{
            if(buffers_.size() < config_.buffers_size_)
                buffers_.push_back(std::move(formatted));
            else{
                auto msg = fmt::format("tcp_sink buffers({}) full", buffers_.size());
                throw_spdlog_ex(msg);
            }
        }
    }
    inline void send(){ 
        try{
            auto&  formatted = buffers_.front();
            client_.send(formatted.data(), formatted.size());
            buffers_.pop_front();
        }catch(std::exception& e){
        }catch(...){ 
        }
    }
    tcp_sink_config config_;
    details::tcp_client client_;
    std::list<spdlog::memory_buf_t> buffers_;
};

using tcp_sink_mt = tcp_sink<std::mutex>;
using tcp_sink_st = tcp_sink<spdlog::details::null_mutex>;

} // namespace sinks
} // namespace spdlog
