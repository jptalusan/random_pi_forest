#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "mqtt/async_client.h"
#include <iostream>
#include <cstdlib>
#include <thread>	// For sleep
#include <atomic>
#include <chrono>
#include <cstring>
#include "mqtt/async_client.h"

namespace Utils{
    class callback : public virtual mqtt::callback {
    public:
        void connection_lost(const std::string& cause) override;
        void delivery_complete(mqtt::delivery_token_ptr tok) override;
    };

    class action_listener : public virtual mqtt::iaction_listener {
    protected:
        void on_failure(const mqtt::token& tok) override;
        void on_success(const mqtt::token& tok) override;
    };
    
    class delivery_action_listener : public action_listener {
        std::atomic<bool> done_;
        void on_failure(const mqtt::token& tok) override;
        void on_success(const mqtt::token& tok) override;
    public:
        bool is_done() const;
        delivery_action_listener();
    };
}

#endif
