#include <filesystem>
#include "websocket_client.h"
#include "nlohmann/json.hpp"

namespace nl = nlohmann;

WebSocketClient::WebSocketClient(std::shared_ptr<Allxon::Octo> octo)
    : octo_(octo), received_person_("nobody"), alert_enabled_(false), alert_trigger_(false)
{
    endpoint_.set_reuse_addr(true);
    endpoint_.clear_access_channels(websocketpp::log::alevel::all);
    endpoint_.clear_access_channels(websocketpp::log::elevel::all);
    endpoint_.clear_error_channels(websocketpp::log::alevel::all);
    endpoint_.clear_error_channels(websocketpp::log::elevel::all);
    endpoint_.set_tls_init_handler(bind(&WebSocketClient::on_tls_init, this, std::placeholders::_1));
    endpoint_.set_open_handler(bind(&WebSocketClient::on_open, this, std::placeholders::_1));
    endpoint_.set_fail_handler(bind(&WebSocketClient::on_fail, this, std::placeholders::_1));
    endpoint_.set_message_handler(bind(&WebSocketClient::on_message, this, std::placeholders::_1, std::placeholders::_2));
    endpoint_.set_close_handler(bind(&WebSocketClient::on_close, this, std::placeholders::_1));
    endpoint_.init_asio();
    endpoint_.start_perpetual();
    run_thread_.reset(new std::thread(&client::run, &endpoint_));
}
WebSocketClient::~WebSocketClient()
{
}

void WebSocketClient::connect(const std::string &url)
{
    websocketpp::lib::error_code ec;
    client::connection_ptr con = endpoint_.get_connection(url, ec);
    if (ec)
    {
        std::cout << "Connect initialization error: " << ec.message() << std::endl;
        return;
    }
    hdl_ = con->get_handle();
    endpoint_.connect(con);
}

void WebSocketClient::run()
{
    connect(octo_->get_websocket_url());
    int count = 0;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        send_np_command_ack(cmd_accept_queue_);
        send_np_command_ack(cmd_ack_queue_);
        if (++count == 60)
        {
            send_np_states_metrics();
            count = 0;
        }
        if (alert_trigger())
        {
            if (is_alert_enabled())
                send_np_alert();
            set_alert_trigger(false);
        }
    }
}
context_ptr WebSocketClient::on_tls_init(websocketpp::connection_hdl hdl)
{
    (void)hdl;
    std::cout << "on_tls_init" << std::endl;
    context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12);

    ctx->set_options(asio::ssl::context::default_workarounds |
                     asio::ssl::context::no_sslv2 |
                     asio::ssl::context::no_sslv3 |
                     asio::ssl::context::single_dh_use);

    ctx->set_verify_mode(asio::ssl::verify_none);
    return ctx;
}

void WebSocketClient::on_open(websocketpp::connection_hdl hdl)
{
    (void)hdl;
    std::cout << "on_open" << std::endl;
    send_np_update();
}
void WebSocketClient::on_close(websocketpp::connection_hdl hdl)
{
    (void)hdl;
    std::cout << "on_close" << std::endl;
    exit(0);
}
void WebSocketClient::on_fail(websocketpp::connection_hdl hdl)
{
    (void)hdl;
    std::cout << "on_fail" << std::endl;
    endpoint_.get_alog().write(websocketpp::log::alevel::app, "Connection Failed");
    exit(1);
}
void WebSocketClient::on_message(websocketpp::connection_hdl hdl, client::message_ptr msg)
{
    (void)hdl;
    const auto payload = nl::json::parse(msg->get_payload());
    std::cout << "on_message" << std::endl;
    std::cout << payload.dump() << std::endl;

    // Handle JSON-RPC error object
    if (payload.contains("error"))
    {
        auto error = payload.at("error");
        printf("Received JSON-RPC error object, error code: %s, error_message: %s\n",
               error.at("code").template get<std::string>().c_str(), error.at("message").template get<std::string>().c_str());
        return;
    }

    // Verify payload
    std::string get_method;
    if (!octo_->verify(payload.dump(), get_method))
    {
        std::cout << octo_->error_message() << std::endl;
        printf("Received data verify failed: %s\n", octo_->error_message().c_str());
        return;
    }

    std::cout << "Get Method:" << get_method << std::endl;
    if (get_method == "v2/notifyPluginCommand")
    {
        auto cmd_id = payload.at("params").at("commandId").template get<std::string>();
        std::cout << "get command id: " << cmd_id << std::endl;
        set_received_person(payload.at("params").at("commands").at(0).at("params").at(0).at("value").template get<std::string>());

        auto cmd_accept = nl::json::parse(Util::getJsonFromFile(Util::plugin_install_dir + "/plugin_command_ack.json"));
        cmd_accept["params"]["commandId"] = cmd_id;
        cmd_accept["params"]["commandState"] = "ACCEPTED";
        cmd_accept["params"]["commandAcks"][0]["result"]["response"] = "Hello " + received_person();

        auto np_state = nl::json::parse(Util::getJsonFromFile(Util::plugin_install_dir + "/plugin_state.json"));
        np_state["params"]["states"][0]["value"] = "Hello " + received_person();
        cmd_accept["params"]["states"] = np_state.at("params").at("states");

        push_command_queue(cmd_accept_queue_, cmd_accept.dump());

        cmd_accept["params"]["commandState"] = "ACKED";
        push_command_queue(cmd_ack_queue_, cmd_accept.dump());
        set_alert_trigger(true);
    }
    else if (get_method == "v2/notifyPluginAlarmUpdate")
    {
        // deprecated case
        if (!payload.at("params").contains("modules"))
        {
            set_alert_enabled(false);
            return;
        }

        auto alarms = payload.at("params").at("modules").at(0).at("alarms");
        
        // turn off all alerts
        if (alarms.size() == 0)
        {
            set_alert_enabled(false);
            return;
        }

        // turn on/off specific alert
        set_alert_enabled(alarms.at(0).at("enabled").template get<bool>());
    }
}
void WebSocketClient::send_np_update()
{
    std::cout << "send_np_update" << std::endl;
    auto np_update = nl::json::parse(Util::getJsonFromFile(Util::plugin_install_dir + "/plugin_update_template.json"));
    np_update["params"]["modules"][0]["properties"][0]["value"] = std::filesystem::canonical(Util::plugin_install_dir).string();
    verify_and_send(np_update.dump());
}

void WebSocketClient::send_np_states_metrics()
{
    std::cout << "SendPluginStateMetrics" << std::endl;
    auto np_state = nl::json::parse(Util::getJsonFromFile(Util::plugin_install_dir + "/plugin_state.json"));
    np_state["params"]["states"][0]["value"] = "Hello " + received_person();
    verify_and_send(np_state.dump());
}

void WebSocketClient::send_np_command_ack(std::queue<std::string> &queue)
{
    if (queue.empty())
        return;
    std::cout << "send_np_command_ack" << std::endl;
    std::string np_cmd_ack_str;
    while (pop_command_queue(queue, np_cmd_ack_str) && verify_and_send(np_cmd_ack_str))
    {
    }
}

void WebSocketClient::send_np_alert()
{
    std::cout << "send_np_alert" << std::endl;
    auto np_alert = nl::json::parse(Util::getJsonFromFile(Util::plugin_install_dir + "/plugin_alert.json"));
    np_alert["params"]["alarms"][0]["message"] = "Hello " + received_person() + " ~";
    np_alert["params"]["alarms"][0]["time"] = std::to_string(time(NULL));
    verify_and_send(np_alert.dump());
}

bool WebSocketClient::verify_and_send(const std::string &json)
{
    auto send_json = json;
    if (!octo_->sign(send_json))
    {
        std::cout << octo_->error_message().c_str() << std::endl;
        set_alert_trigger(false);
        return false;
    }

    endpoint_.send(hdl_, send_json.c_str(), websocketpp::frame::opcode::TEXT);
    std::cout << "Send:" << send_json << std::endl;
    return true;
}

void WebSocketClient::push_command_queue(std::queue<std::string> &queue, std::string data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    queue.push(data);
}

bool WebSocketClient::pop_command_queue(std::queue<std::string> &queue, std::string &pop_data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue.empty())
        return false;
    pop_data = queue.front();
    queue.pop();
    return true;
}

void WebSocketClient::set_alert_enabled(bool enabled)
{
    std::lock_guard<std::mutex> lock(mutex_);
    alert_enabled_ = enabled;
}

bool WebSocketClient::is_alert_enabled() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return alert_enabled_;
}

void WebSocketClient::set_received_person(const std::string &person)
{
    std::lock_guard<std::mutex> lock(mutex_);
    received_person_ = person;
}

std::string WebSocketClient::received_person() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return received_person_;
}

void WebSocketClient::set_alert_trigger(bool need_trigger)
{
    std::lock_guard<std::mutex> lock(mutex_);
    alert_trigger_ = need_trigger;
}

bool WebSocketClient::alert_trigger() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return alert_trigger_;
}