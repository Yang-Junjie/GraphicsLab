#pragma once

#include "Application.hpp"

class FluidSimApp : public flux::Application {
public:
    explicit FluidSimApp(const flux::ApplicationSpecification& spec);
    ~FluidSimApp() override = default;
};
