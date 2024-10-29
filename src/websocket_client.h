#ifndef _WEBSOCKET_CLIENT_H_
#define _WEBSOCKET_CLIENT_H_

#include <iostream>
#include <thread>
#include <fstream>

#include "octo/octo.h"

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
    WebSocketClient(std::shared_ptr<Allxon::Octo> octo);
    ~WebSocketClient();

    void run();

private:
    context_ptr on_tls_init(websocketpp::connection_hdl hdl);
    void connect(const std::string &url);
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_fail(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg);
    void send_np_update();
    void send_np_states_metrics();
    void run_command_and_ack(std::queue<std::string> &queue);
    void send_np_alert();
    bool verify_and_send(const std::string &json);
    void push_command_queue(std::queue<std::string> &queue, std::string data);
    bool pop_command_queue(std::queue<std::string> &queue, std::string &pop_data);

    void set_alert_enabled(bool enabled);
    bool is_alert_enabled() const;

    void set_received_person(const std::string &person);
    std::string received_person() const;

    void set_alert_trigger(bool need_trigger);
    bool alert_trigger() const;

    client endpoint_;
    websocketpp::connection_hdl hdl_;
    mutable std::mutex mutex_;
    websocketpp::lib::shared_ptr<std::thread> run_thread_;
    std::shared_ptr<Allxon::Octo> octo_;
    std::queue<std::string> cmd_queue_;
    std::string received_person_;
    bool alert_enabled_;
    bool alert_trigger_;
};
#endif
