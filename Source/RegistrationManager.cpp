#include "RegistrationManager.h"

RegistrationManager::RegistrationManager(juce::ApplicationProperties& properties)
    : appProperties(properties)
{
}

bool RegistrationManager::isRegistered() const
{
    auto* settings = appProperties.getUserSettings();
    return settings->getBoolValue("isRegistered", false);
}

bool RegistrationManager::registerSerialNumber(const juce::String& serialNumber)
{
    const bool isValid = validateSerialNumber(serialNumber);
    if (isValid)
    {
        auto* settings = appProperties.getUserSettings();
        settings->setValue("isRegistered", true);
        settings->setValue("serialNumber", serialNumber);
        settings->saveIfNeeded();
    }
    return isValid;
}

bool RegistrationManager::validateSerialNumber(const juce::String& serialNumber)
{
    try
    {
        juce::URL url("https://www.faderkeys.com/api/auth/serial");

        juce::var jsonBody = juce::var(new juce::DynamicObject());
        jsonBody.getDynamicObject()->setProperty("serialNumber", serialNumber);

        juce::URL::InputStreamOptions opts(juce::URL::ParameterHandling::inPostData);
        opts.withExtraHeaders("Content-Type: application/json")
            .withConnectionTimeoutMs(5000);

        url = url.withPOSTData(juce::JSON::toString(jsonBody));

        if (auto inputStream = url.createInputStream(opts))
        {
            if (auto* webStream = dynamic_cast<juce::WebInputStream*>(inputStream.get()))
                return webStream->getStatusCode() == 200;
        }
    }
    catch (const std::exception& e)
    {
        DBG("Network error: " << e.what());
    }

    // For development, return true to bypass validation
    #if JUCE_DEBUG
    return true;
    #else
    return false;
    #endif
}
