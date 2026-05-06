#pragma once

#include <memory>
#include <string>
#include <vector>

#include <mujoco/mujoco.h>

#include "param.h"
#include "sim_plugin.h"

class PluginManager {
    public:
        bool CreatePlugins(const std::vector<std::string>& plugin_names);
        bool Configure(const param::SimulationConfig& config,
                        mjModel* model,
                        mjData* data);
        bool RequiresSplitStep() const;

        void Reset(mjModel* model, mjData* data);
        void PreStep(mjModel* model, mjData* data);
        void Update(mjModel* model, mjData* data);
        void PostStep(mjModel* model, mjData* data);

        bool empty() const { return plugins_.empty(); }
        const std::vector<std::string>& plugin_names() const {
            return plugin_names_;
        }

    private:
        std::vector<std::string> plugin_names_;
        std::vector<std::unique_ptr<AgilexSimPlugin>> plugins_;
};
