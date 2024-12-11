#ifndef GAMEPLATFORM_H
#define GAMEPLATFORM_H
#include <string>

class GamePlatform {
public:
    // Constructeur
    GamePlatform(
        const std::string& name,
        const std::string& user_agent,
        const std::string& device_name,
        const std::string& device_model,
        const std::string& os_version
        );

    // Attributs
    std::string name;
    std::string user_agent;
    std::string device_name;
    std::string device_model;
    std::string os_version;
};

#endif // GAMEPLATFORM_H
