#include "utils.h"
#include <string>

namespace Utils {
    void callback::connection_lost(const std::string& cause) {
        std::cout << "\nConnection lost" << std::endl;
        if (!cause.empty()) {
            std::cout << "\tcause: " << cause << std::endl;
        }
    }

    void callback::delivery_complete(mqtt::delivery_token_ptr tok) {
        std::cout << "\tDelivery complete for token: " 
            << (tok ? tok->get_message_id() : -1) << std::endl;
    }

    void action_listener::on_failure(const mqtt::token& tok) {
        std::cout << "\tListener failure for token: "
            << tok.get_message_id() << std::endl;
    }

    void action_listener::on_success(const mqtt::token& tok) {
        std::cout << "\tListener success for token: "
            << tok.get_message_id() << std::endl;
    }

    std::atomic<bool> done_;

    void delivery_action_listener::on_failure(const mqtt::token& tok) {
        action_listener::on_failure(tok);
        done_ = true;
    }

    void delivery_action_listener::on_success(const mqtt::token& tok) {
        action_listener::on_success(tok);
        done_ = true;
    }

    delivery_action_listener::delivery_action_listener() : done_(false) {}
    bool delivery_action_listener::is_done() const { return done_; }

}

