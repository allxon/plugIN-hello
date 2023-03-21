#include <iostream>
#include "json_validator.h"
#include "build_info.h"
#include "websocket_client.h"

using namespace Allxon;

std::string Util::plugin_install_dir = "";

void RunSendingLoop(WebSocketClient &web_client, int seconds)
{
    web_client.Connect();
    int count = 0;
    while (true && seconds > 0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        web_client.SendPluginCommandAck();
        if (++count == 60)
        {
            web_client.SendPluginStatesMetrics();
            count = 0;
        }
        web_client.SendPluginAlert();
        --seconds;
    }
};

void RunPluginFromJsonInSeconds(const std::string& JsonPath, int seconds)
{
    auto np_update_json = Util::getJsonFromFile(JsonPath);
    auto json_validator = std::make_shared<JsonValidator>(PLUGIN_NAME, PLUGIN_APP_GUID,
                                                          PLUGIN_ACCESS_KEY, PLUGIN_VERSION,
                                                          np_update_json);
    std::cout << np_update_json << std::endl;
    WebSocketClient web_client(json_validator, np_update_json);
    RunSendingLoop(web_client, seconds);
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        std::cout << "Please provide a plugin install directory." << std::endl;
        return 1;
    }
    else if (argc > 2)
    {
        std::cout << "Wrong arguments. Usage: device_plugin [plugin install directory]" << std::endl;
        return 1;
    }
    Util::plugin_install_dir = std::string(argv[1]);
    RunPluginFromJsonInSeconds(Util::plugin_install_dir + "/plugin_update_template.json", 5);
    RunPluginFromJsonInSeconds(Util::plugin_install_dir + "/plugin_update_template_new.json", 300);
    return 0;

}
