#ifndef _WEBSOCKET_CLIENT_H_
#define _WEBSOCKET_CLIENT_H_

#include <iostream>
#include <thread>
#include <fstream>

#include "cJSON.h"
#include "json_validator.h"

#define ASIO_STANDALONE
#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"

using namespace Allxon;
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

class Util
{
public:
    static std::string getJsonFromFile(const std::string &path)
    {
        std::string output;
        std::ifstream myfile(path);
        if (myfile.is_open())
        {
            std::string line;
            while (std::getline(myfile, line))
                output.append(line + "\n");
            myfile.close();
        }
        else
            std::cout << "Unable to open file";
        return output;
    }

    static std::string plugin_install_dir;
};

class WebSocketClient
{
public:
    WebSocketClient(std::shared_ptr<Allxon::JsonValidator> json_validator, const std::string &url);
    ~WebSocketClient();

    void RunSendingLoop();

private:
    context_ptr OnTLSInit(websocketpp::connection_hdl hdl);
    void Connect(const std::string &url);
    void OnOpen(websocketpp::connection_hdl hdl);
    void OnClose(websocketpp::connection_hdl hdl);
    void OnFail(websocketpp::connection_hdl hdl);
    void OnMessage(websocketpp::connection_hdl hdl, client::message_ptr msg);
    void SendNotifyPluginUpdate();
    void SendPluginStatesMetrics();
    void SendPluginCommandAck(std::queue<std::string> &queue);
    void SendPluginAlert();
    void PushCommandQueue(std::queue<std::string> &queue, std::string data);
    bool PopCommandQueue(std::queue<std::string> &queue, std::string &pop_data);

    void set_alert_enabled(bool enabled);
    bool is_alert_enabled() const;

    void set_received_person(const std::string &person);
    std::string received_person() const;

    void set_alert_trigger(bool need_trigger);
    bool alert_trigger() const;

    client m_endpoint;
    websocketpp::connection_hdl m_hdl;
    mutable std::mutex m_mutex;
    websocketpp::lib::shared_ptr<std::thread> m_run_thread;
    websocketpp::lib::shared_ptr<std::thread> m_send_thread;
    std::shared_ptr<Allxon::JsonValidator> m_json_validator;
    std::queue<std::string> m_cmd_accept_queue;
    std::queue<std::string> m_cmd_ack_queue;
    std::string m_url;
    std::string received_person_;
    bool alert_enabled_;
    bool alert_trigger_;
};
#endif
