#include "GamePlatform.h"

// Définition du constructeur
GamePlatform::GamePlatform(
    const std::string& name,
    const std::string& user_agent,
    const std::string& device_name,
    const std::string& device_model,
    const std::string& os_version
    )
    : name(name),
    user_agent(user_agent),
    device_name(device_name),
    device_model(device_model),
    os_version(os_version)
{}
