#include "websocket_client.h"
#include "build_info.h"

WebSocketClient::WebSocketClient(std::shared_ptr<Allxon::JsonValidator> json_validator, const std::string &url)
    : m_json_validator(json_validator), m_url(url), received_person_("nobody"), alert_enabled_(false), alert_trigger_(false)
{
    m_endpoint.set_reuse_addr(true);
    m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
    m_endpoint.clear_access_channels(websocketpp::log::elevel::all);
    m_endpoint.clear_error_channels(websocketpp::log::alevel::all);
    m_endpoint.clear_error_channels(websocketpp::log::elevel::all);
    m_endpoint.set_tls_init_handler(bind(&WebSocketClient::OnTLSInit, this, std::placeholders::_1));
    m_endpoint.set_open_handler(bind(&WebSocketClient::OnOpen, this, std::placeholders::_1));
    m_endpoint.set_fail_handler(bind(&WebSocketClient::OnFail, this, std::placeholders::_1));
    m_endpoint.set_message_handler(bind(&WebSocketClient::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    m_endpoint.set_close_handler(bind(&WebSocketClient::OnClose, this, std::placeholders::_1));
    m_endpoint.init_asio();
    m_endpoint.start_perpetual();
    m_run_thread.reset(new std::thread(&client::run, &m_endpoint));
}
WebSocketClient::~WebSocketClient()
{
}

void WebSocketClient::Connect(const std::string &url)
{
    websocketpp::lib::error_code ec;
    client::connection_ptr con = m_endpoint.get_connection(url, ec);
    if (ec)
    {
        std::cout << "Connect initialization error: " << ec.message() << std::endl;
        return;
    }
    m_hdl = con->get_handle();
    m_endpoint.connect(con);
}

void WebSocketClient::RunSendingLoop()
{
    Connect(m_url);
    int count = 0;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        SendPluginCommandAck(m_cmd_accept_queue);
        SendPluginCommandAck(m_cmd_ack_queue);
        if (++count == 60)
        {
            SendPluginStatesMetrics();
            count = 0;
        }
        if (alert_trigger()) {
            if (is_alert_enabled())
                SendPluginAlert();
            set_alert_trigger(false);
        }
    }
}
context_ptr WebSocketClient::OnTLSInit(websocketpp::connection_hdl hdl)
{
    std::cout << "OnTLSInit" << std::endl;
    context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12);

    ctx->set_options(asio::ssl::context::default_workarounds |
                     asio::ssl::context::no_sslv2 |
                     asio::ssl::context::no_sslv3 |
                     asio::ssl::context::single_dh_use);

    ctx->set_verify_mode(asio::ssl::verify_none);
    return ctx;
}

void WebSocketClient::OnOpen(websocketpp::connection_hdl hdl)
{
    std::cout << "OnOpen" << std::endl;
    SendNotifyPluginUpdate();
}
void WebSocketClient::OnClose(websocketpp::connection_hdl hdl)
{
    std::cout << "OnClose" << std::endl;
    m_endpoint.stop();
    exit(0);
}
void WebSocketClient::OnFail(websocketpp::connection_hdl hdl)
{
    std::cout << "OnFail" << std::endl;
    m_endpoint.get_alog().write(websocketpp::log::alevel::app, "Connection Failed");
    m_endpoint.close(hdl, websocketpp::close::status::normal, "Connection Failed.");
    m_endpoint.stop();
}
void WebSocketClient::OnMessage(websocketpp::connection_hdl hdl, client::message_ptr msg)
{
    const auto payload = msg->get_payload();
    std::cout << "OnMessage" << std::endl;
    std::cout << payload.c_str() << std::endl;
    std::string get_method;
    if (!m_json_validator->Verify(payload, get_method))
    {
        std::cout << m_json_validator->error_message() << std::endl;
        exit(0);
    }
    if (get_method == "v2/notifyPluginCommand")
    {
        std::cout << "Get Method:" << get_method << std::endl;
        auto cmd_cjson = cJSON_Parse(payload.c_str());
        auto params_cjson = cJSON_GetObjectItemCaseSensitive(cmd_cjson, "params");
        auto command_id_cjson = cJSON_GetObjectItemCaseSensitive(params_cjson, "commandId");
        std::cout << "get command id: " << command_id_cjson->valuestring << std::endl;
        auto module_name_cjson = cJSON_GetObjectItemCaseSensitive(params_cjson, "moduleName");
        auto commands_cjson = cJSON_GetObjectItemCaseSensitive(params_cjson, "commands");
        auto command_cjson = cJSON_GetArrayItem(commands_cjson, 0);
        auto cmd_params_cjson = cJSON_GetObjectItemCaseSensitive(command_cjson, "params");
        auto cmd_param_cjson = cJSON_GetArrayItem(cmd_params_cjson, 0);
        auto cmd_value_cjson = cJSON_GetObjectItemCaseSensitive(cmd_param_cjson, "value");
        set_received_person(cJSON_GetStringValue(cmd_value_cjson));

        auto cmd_accept = Util::getJsonFromFile(Util::plugin_install_dir + "/plugin_command_ack.json");
        auto cmd_accept_cjson = cJSON_Parse(cmd_accept.c_str());
        auto accept_params_cjson = cJSON_GetObjectItemCaseSensitive(cmd_accept_cjson, "params");
        auto accept_command_id_cjson = cJSON_GetObjectItemCaseSensitive(accept_params_cjson, "commandId");
        auto accept_command_state_cjson = cJSON_GetObjectItemCaseSensitive(accept_params_cjson, "commandState");
        cJSON_SetValuestring(accept_command_id_cjson, command_id_cjson->valuestring);
        cJSON_SetValuestring(accept_command_state_cjson, "ACCEPTED");
        auto accept_command_acks_cjson = cJSON_GetObjectItemCaseSensitive(accept_params_cjson, "commandAcks");
        auto accept_command_ack_cjson = cJSON_GetArrayItem(accept_command_acks_cjson, 0);
        auto accept_result_cjson = cJSON_GetObjectItemCaseSensitive(accept_command_ack_cjson, "result");
        cJSON_AddStringToObject(accept_result_cjson, "response", std::string("Hello " + received_person()).c_str());
        char *cmd_accept_str = cJSON_Print(cmd_accept_cjson);
        PushCommandQueue(m_cmd_accept_queue, std::string(cmd_accept_str));
        delete cmd_accept_str;

        cJSON_SetValuestring(accept_command_state_cjson, "ACKED");
        char *cmd_ack_str = cJSON_Print(cmd_accept_cjson);
        PushCommandQueue(m_cmd_ack_queue, std::string(cmd_ack_str));
        delete cmd_ack_str;

        cJSON_Delete(cmd_cjson);
        cJSON_Delete(cmd_accept_cjson);

        set_alert_trigger(true);
    }
    else if (get_method == "v2/notifyPluginAlarmUpdate")
    {
        std::cout << "Get Method:" << get_method << std::endl;
        auto np_alarm_cjson = cJSON_Parse(payload.c_str());
        auto params_cjson = cJSON_GetObjectItemCaseSensitive(np_alarm_cjson, "params");
        auto version_cjson = cJSON_GetObjectItemCaseSensitive(params_cjson, "version");
        std::string version_from_portal = cJSON_GetStringValue(version_cjson);
        if (version_from_portal != PLUGIN_VERSION)
        {
            std::cerr << "Error: version is not same with portal, plugIN: " << PLUGIN_VERSION << ", Portal: " << version_from_portal;
            cJSON_Delete(np_alarm_cjson);
            return;
        }
        auto modules_cjson = cJSON_GetObjectItemCaseSensitive(params_cjson, "modules");
        auto module_cjson = cJSON_GetArrayItem(modules_cjson, 0);
        auto alarms_cjson = cJSON_GetObjectItemCaseSensitive(module_cjson, "alarms");
        auto alarm_cjson = cJSON_GetArrayItem(alarms_cjson, 0);
        auto enabled_cjson = cJSON_GetObjectItemCaseSensitive(alarm_cjson, "enabled");
        set_alert_enabled(cJSON_IsTrue(enabled_cjson));
        cJSON_Delete(np_alarm_cjson);
    }
}
void WebSocketClient::SendNotifyPluginUpdate()
{
    std::cout << "SendNotifyPluginUpdate" << std::endl;
    std::string notify_plugin_update = Util::getJsonFromFile(Util::plugin_install_dir + "/plugin_update_template.json");
    auto np_update_cjson = cJSON_Parse(notify_plugin_update.c_str());
    auto params_cjson = cJSON_GetObjectItemCaseSensitive(np_update_cjson, "params");
    auto modules_cjson = cJSON_GetObjectItemCaseSensitive(params_cjson, "modules");
    auto module_cjson = cJSON_GetArrayItem(modules_cjson, 0);
    auto properties_cjson = cJSON_GetObjectItemCaseSensitive(module_cjson, "properties");
    auto property_cjson = cJSON_GetArrayItem(properties_cjson, 0);
    auto property_value_cjson = cJSON_GetObjectItemCaseSensitive(property_cjson, "value");
    cJSON_SetValuestring(property_value_cjson, Util::plugin_install_dir.c_str());
    auto output_char = cJSON_Print(np_update_cjson);
    std::string output_string(output_char);
    delete output_char;
    cJSON_Delete(np_update_cjson);
    if (!m_json_validator->Sign(output_string))
    {
        std::cout << m_json_validator->error_message().c_str() << std::endl;
        return;
    }
    m_endpoint.send(m_hdl, output_string.c_str(), websocketpp::frame::opcode::TEXT);
    std::cout << "Send:" << output_string << std::endl;
}

void WebSocketClient::SendPluginStatesMetrics()
{
    std::cout << "SendPluginStateMetrics" << std::endl;
    std::string notify_plugin_state = Util::getJsonFromFile(Util::plugin_install_dir + "/plugin_state.json");
    auto np_state_cjson = cJSON_Parse(notify_plugin_state.c_str());
    auto params_cjson = cJSON_GetObjectItemCaseSensitive(np_state_cjson, "params");
    auto states_cjson = cJSON_GetObjectItemCaseSensitive(params_cjson, "states");
    auto state_cjson = cJSON_GetArrayItem(states_cjson, 0);
    auto state_value_cjson = cJSON_GetObjectItemCaseSensitive(state_cjson, "value");
    cJSON_SetValuestring(state_value_cjson, std::string("Hello " + received_person() + " ~").c_str());
    auto output_char = cJSON_Print(np_state_cjson);
    std::string output_string(output_char);
    delete output_char;
    cJSON_Delete(np_state_cjson);
    if (!m_json_validator->Sign(output_string))
    {
        std::cout << m_json_validator->error_message().c_str() << std::endl;
        return;
    }

    m_endpoint.send(m_hdl, output_string.c_str(), websocketpp::frame::opcode::TEXT);
    std::cout << "Send:" << output_string << std::endl;
}

void WebSocketClient::SendPluginCommandAck(std::queue<std::string> &queue)
{
    if (queue.empty())
        return;
    std::cout << "SendPluginCommandAck" << std::endl;
    std::string np_cmd_ack_str;
    while (PopCommandQueue(queue, np_cmd_ack_str))
    {
        if (!m_json_validator->Sign(np_cmd_ack_str))
        {
            std::cout << m_json_validator->error_message().c_str() << std::endl;
            return;
        }
        m_endpoint.send(m_hdl, np_cmd_ack_str.c_str(), websocketpp::frame::opcode::TEXT);
        std::cout << "Send:" << np_cmd_ack_str << std::endl;
    }
}

void WebSocketClient::SendPluginAlert()
{
    std::cout << "SendPluginAlert" << std::endl;
    std::string notify_plugin_alert = Util::getJsonFromFile(Util::plugin_install_dir + "/plugin_alert.json");
    auto np_alert_cjson = cJSON_Parse(notify_plugin_alert.c_str());
    auto params_cjson = cJSON_GetObjectItemCaseSensitive(np_alert_cjson, "params");
    auto alarms_cjson = cJSON_GetObjectItemCaseSensitive(params_cjson, "alarms");
    auto alarm_cjson = cJSON_GetArrayItem(alarms_cjson, 0);
    auto alarm_msg_cjson = cJSON_GetObjectItemCaseSensitive(alarm_cjson, "message");
    cJSON_SetValuestring(alarm_msg_cjson, std::string("Hello " + received_person() + " ~").c_str());
    auto alarm_time_cjson = cJSON_GetObjectItemCaseSensitive(alarm_cjson, "time");
    cJSON_SetValuestring(alarm_time_cjson, std::to_string(time(NULL)).c_str());
    auto output_char = cJSON_Print(np_alert_cjson);
    std::string output_string(output_char);
    delete output_char;
    cJSON_Delete(np_alert_cjson);
    if (!m_json_validator->Sign(output_string))
    {
        std::cout << m_json_validator->error_message().c_str() << std::endl;
        set_alert_trigger(false);
        return;
    }

    m_endpoint.send(m_hdl, output_string.c_str(), websocketpp::frame::opcode::TEXT);
    std::cout << "Send:" << output_string << std::endl;
}

void WebSocketClient::PushCommandQueue(std::queue<std::string> &queue, std::string data)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    queue.push(data);
}

bool WebSocketClient::PopCommandQueue(std::queue<std::string> &queue, std::string &pop_data)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (queue.empty())
        return false;
    pop_data = queue.front();
    queue.pop();
    return true;
}

void WebSocketClient::set_alert_enabled(bool enabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    alert_enabled_ = enabled;
}

bool WebSocketClient::is_alert_enabled() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return alert_enabled_;
}

void WebSocketClient::set_received_person(const std::string &person)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    received_person_ = person;
}

std::string WebSocketClient::received_person() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return received_person_;
}

void WebSocketClient::set_alert_trigger(bool need_trigger)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    alert_trigger_ = need_trigger;
}

bool WebSocketClient::alert_trigger() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return alert_trigger_;
}