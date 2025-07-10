#pragma once

#include "application/application.h"

class DemoApplication final : public MongooseVK::Application {
public:
    explicit DemoApplication(const ::MongooseVK::ApplicationConfig& config)
        : Application(config) {}

    virtual ::MongooseVK::Window* CreateWindow(::MongooseVK::WindowParams params) override;
};
