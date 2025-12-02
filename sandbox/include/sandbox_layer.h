#pragma once

#include "Sapfire.h"

class SandboxLayer final : public sf::Layer {
public:
    SandboxLayer();
    ~SandboxLayer() final = default;
    void on_attach() final;
    void on_detach() final;
    void on_update(sf::f32 delta_time) final;
    void on_event(sf::Event& e) final;
};
