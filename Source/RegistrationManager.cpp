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

void RegistrationManager::registerSerialNumberAsync(const juce::String& serialNumber,
                                                    std::function<void(bool)> callback)
{
    // Create a shared pointer to store the thread
    auto thread = std::make_shared<std::thread>([this, serialNumber, callback]()
    {
        const bool isValid = validateSerialNumber(serialNumber);

        if (isValid)
        {
            // Store registration info in settings if valid
            auto* settings = appProperties.getUserSettings();
            settings->setValue("isRegistered", true);
            settings->setValue("serialNumber", serialNumber);
            settings->saveIfNeeded();
        }

        // Call the callback on the message thread
        juce::MessageManager::callAsync([callback, isValid]()
        {
            if (callback)
                callback(isValid);
        });
    });

    // Detach the thread after creating it
    thread->detach();
}

bool RegistrationManager::validateSerialNumber(const juce::String& serialNumber)
{
    try
    {
        juce::URL url("https://www.faderkeys.com/api/auth/serial");

        juce::var jsonBody = juce::var(new juce::DynamicObject());
        jsonBody.getDynamicObject()->setProperty("serialNumber", serialNumber);

        auto opts = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                       .withExtraHeaders("Content-Type: application/json")
                       .withConnectionTimeoutMs(5000);

        url = url.withPOSTData(juce::JSON::toString(jsonBody));

        // Create the input stream and wait for response
        auto inputStream = url.createInputStream(opts);
        if (inputStream != nullptr)
        {
            if (auto* webStream = dynamic_cast<juce::WebInputStream*>(inputStream.get()))
            {
                // Wait for the full response
                auto response = inputStream->readEntireStreamAsString();
                return webStream->getStatusCode() == 200;
            }
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
