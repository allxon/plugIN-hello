#include <iostream>
#include "octo/octo.h"
#include "websocket_client.h"

using namespace Allxon;

std::string Util::plugin_install_dir = "";

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
    WebSocketClient web_client(std::make_shared<Octo>(
        PLUGIN_NAME, PLUGIN_APP_GUID,
        PLUGIN_ACCESS_KEY, PLUGIN_VERSION,
        Util::getJsonFromFile(Util::plugin_install_dir + "/plugin_update_template.json")));
    web_client.run();
    return 0;
}
