#pragma once

#include <mujoco/mujoco.h>

#include "param.h"

class AgilexSimPlugin {
    public:
    virtual ~AgilexSimPlugin() = default;

    virtual bool Configure(const param::SimulationConfig& config,
                            mjModel* model,
                            mjData* data) {
        (void)config;
        (void)model;
        (void)data;
        return true;
    }

    virtual bool RequiresSplitStep() const {
        return false;
    }

    virtual void Reset(mjModel* model, mjData* data) {
        (void)model;
        (void)data;
    }

    virtual void PreStep(mjModel* model, mjData* data) {
        (void)model;
        (void)data;
    }

    virtual void Update(mjModel* model, mjData* data) {
        (void)model;
        (void)data;
    }

    virtual void PostStep(mjModel* model, mjData* data) {
        (void)model;
        (void)data;
    }
};
