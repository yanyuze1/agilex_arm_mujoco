// Copyright 2021 DeepMind Technologies Limited
//
// Licensed under the Apache License, Version 2.0.

#define private public
#include "glfw_adapter.h"
#undef private

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <thread>

#include <mujoco/mujoco.h>

#include "array_safety.h"
#include "param.h"
#include "simulate.h"
#include "plugin_manager.h"


#define MUJOCO_PLUGIN_DIR "mujoco_plugin"

extern "C" {
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#else
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#include <sys/errno.h>
#include <unistd.h>
#endif
}

namespace {
namespace mj = ::mujoco;
namespace mju = ::mujoco::sample_util;

constexpr double kSyncMisalign = 0.1;
constexpr double kSimRefreshFraction = 0.7;
constexpr int kErrorLength = 1024;

mjModel* m = nullptr;
mjData* d = nullptr;
mjtNum* ctrlnoise = nullptr;
mj::Simulate* g_sim = nullptr;

PluginManager g_plugin_manager;

using Seconds = std::chrono::duration<double>;

std::string GetExecutableDir() {
#if defined(_WIN32) || defined(__CYGWIN__)
  constexpr char kPathSep = '\\';
  std::string realpath = [&]() -> std::string {
    std::unique_ptr<char[]> realpath(nullptr);
    DWORD buf_size = 128;
    bool success = false;
    while (!success) {
      realpath.reset(new (std::nothrow) char[buf_size]);
      if (!realpath) {
        std::cerr << "cannot allocate memory to store executable path\n";
        return "";
      }

      DWORD written = GetModuleFileNameA(nullptr, realpath.get(), buf_size);
      if (written < buf_size) {
        success = true;
      } else if (written == buf_size) {
        buf_size *= 2;
      } else {
        std::cerr << "failed to retrieve executable path: " << GetLastError()
                  << "\n";
        return "";
      }
    }
    return realpath.get();
  }();
#else
  constexpr char kPathSep = '/';
#if defined(__APPLE__)
  std::unique_ptr<char[]> buf(nullptr);
  {
    std::uint32_t buf_size = 0;
    _NSGetExecutablePath(nullptr, &buf_size);
    buf.reset(new char[buf_size]);
    if (!buf) {
      std::cerr << "cannot allocate memory to store executable path\n";
      return "";
    }
    if (_NSGetExecutablePath(buf.get(), &buf_size)) {
      std::cerr << "unexpected error from _NSGetExecutablePath\n";
    }
  }
  const char* path = buf.get();
#else
  const char* path = "/proc/self/exe";
#endif
  std::string realpath = [&]() -> std::string {
    std::unique_ptr<char[]> realpath(nullptr);
    std::uint32_t buf_size = 128;
    bool success = false;
    while (!success) {
      realpath.reset(new (std::nothrow) char[buf_size]);
      if (!realpath) {
        std::cerr << "cannot allocate memory to store executable path\n";
        return "";
      }

      std::size_t written = readlink(path, realpath.get(), buf_size);
      if (written < buf_size) {
        realpath.get()[written] = '\0';
        success = true;
      } else if (written == static_cast<std::size_t>(-1)) {
        if (errno == EINVAL) {
          return path;
        }

        std::cerr << "error while resolving executable path: "
                  << strerror(errno) << "\n";
        return "";
      } else {
        buf_size *= 2;
      }
    }
    return realpath.get();
  }();
#endif

  if (realpath.empty()) {
    return "";
  }

  for (std::size_t i = realpath.size() - 1; i > 0; --i) {
    if (realpath.c_str()[i] == kPathSep) {
      return realpath.substr(0, i);
    }
  }

  return "";
}

std::filesystem::path FindAgilexMujocoRoot(
    const std::filesystem::path& start_dir) {
  std::filesystem::path path = std::filesystem::absolute(start_dir);

  for (int i = 0; i < 8 && !path.empty(); ++i) {
    if (std::filesystem::exists(path / "agilex_arm") &&
        std::filesystem::exists(path / "simulate" / "config.yaml")) {
      return path;
    }

    const auto parent = path.parent_path();
    if (parent == path) {
      break;
    }
    path = parent;
  }

  return {};
}

std::filesystem::path ResolveScenePath(
    const std::filesystem::path& agilex_mujoco_root) {
  std::filesystem::path scene = param::config.robot_scene;
  if (!scene.is_relative()) {
    return scene;
  }

  std::string robot_dir = param::config.robot;
  if (robot_dir.rfind("agilex_", 0) != 0) {
    robot_dir = "agilex_" + robot_dir;
  }

  return agilex_mujoco_root / "agilex_arm" / robot_dir / scene;
}

void ScanPluginLibraries() {
  const int nplugin = mjp_pluginCount();
  if (nplugin) {
    std::printf("Built-in plugins:\n");
    for (int i = 0; i < nplugin; ++i) {
      std::printf("    %s\n", mjp_getPluginAtSlot(i)->name);
    }
  }

#if defined(_WIN32) || defined(__CYGWIN__)
  const std::string sep = "\\";
#else
  const std::string sep = "/";
#endif

  const std::string executable_dir = GetExecutableDir();
  if (executable_dir.empty()) {
    return;
  }

  const std::string plugin_dir =
      executable_dir + sep + MUJOCO_PLUGIN_DIR;

  mj_loadAllPluginLibraries(
      plugin_dir.c_str(),
      +[](const char* filename, int first, int count) {
        std::printf("Plugins registered by library '%s':\n", filename);
        for (int i = first; i < first + count; ++i) {
          std::printf("    %s\n", mjp_getPluginAtSlot(i)->name);
        }
      });
}

void PrintSceneInformation(const mjModel* model) {
  if (!model) {
    return;
  }

  std::printf("\nScene information:\n");
  std::printf("  model: %s\n", model->names);
  std::printf("  nq: %d\n", model->nq);
  std::printf("  nv: %d\n", model->nv);
  std::printf("  nu: %d\n", model->nu);
  std::printf("  bodies: %d\n", model->nbody);
  std::printf("  joints: %d\n", model->njnt);
  std::printf("  actuators: %d\n", model->nu);
  std::printf("  sensors: %d\n", model->nsensor);

  if (model->njnt > 0) {
    std::printf("  joint names:\n");
    for (int i = 0; i < model->njnt; ++i) {
      const char* name = mj_id2name(model, mjOBJ_JOINT, i);
      std::printf("    [%d] %s\n", i, name ? name : "(unnamed)");
    }
  }

  if (model->nu > 0) {
    std::printf("  actuator names:\n");
    for (int i = 0; i < model->nu; ++i) {
      const char* name = mj_id2name(model, mjOBJ_ACTUATOR, i);
      std::printf("    [%d] %s\n", i, name ? name : "(unnamed)");
    }
  }

  std::printf("\n");
}

mjModel* LoadModel(const char* file, mj::Simulate& sim) {
  char filename[mj::Simulate::kMaxFilenameLength];
  mju::strcpy_arr(filename, file);

  if (!filename[0]) {
    return nullptr;
  }

  char load_error[kErrorLength] = "";
  mjModel* model_new = nullptr;

  if (mju::strlen_arr(filename) > 4 &&
      !std::strncmp(filename + mju::strlen_arr(filename) - 4, ".mjb",
                    mju::sizeof_arr(filename) - mju::strlen_arr(filename) + 4)) {
    model_new = mj_loadModel(filename, nullptr);
    if (!model_new) {
      mju::strcpy_arr(load_error, "could not load binary model");
    }
  } else {
    model_new = mj_loadXML(filename, nullptr, load_error, kErrorLength);
    if (load_error[0]) {
      int error_length = mju::strlen_arr(load_error);
      if (load_error[error_length - 1] == '\n') {
        load_error[error_length - 1] = '\0';
      }
    }
  }

  mju::strcpy_arr(sim.load_error, load_error);

  if (!model_new) {
    std::printf("%s\n", load_error);
    return nullptr;
  }

  if (load_error[0]) {
    std::printf("Model compiled, but simulation warning (paused):\n  %s\n",
                load_error);
    sim.run = 0;
  }

  return model_new;
}

void ReplaceModel(mj::Simulate& sim, mjModel* model_new, mjData* data_new,
                  const char* filename) {
  sim.Load(model_new, data_new, filename);

  mj_deleteData(d);
  mj_deleteModel(m);

  m = model_new;
  d = data_new;
  mj_forward(m, d);

  free(ctrlnoise);
  ctrlnoise = static_cast<mjtNum*>(malloc(sizeof(mjtNum) * m->nu));
  mju_zero(ctrlnoise, m->nu);

  if (param::config.print_scene_information) {
    PrintSceneInformation(m);
  }
  if (!g_plugin_manager.Configure(param::config, m, d)) {
    sim.exitrequest.store(true);
  }

}

void StepSimulation() {
  g_plugin_manager.PreStep(m, d);

  if (g_plugin_manager.RequiresSplitStep()) {
    mj_step1(m, d);
    g_plugin_manager.Update(m, d);
    mj_step2(m, d);
  } else {
    mj_step(m, d);
  }

  g_plugin_manager.PostStep(m, d);
}
void PhysicsLoop(mj::Simulate& sim) {
  std::chrono::time_point<mj::Simulate::Clock> sync_cpu;
  mjtNum sync_sim = 0;

  while (!sim.exitrequest.load()) {
    if (sim.droploadrequest.load()) {
      sim.LoadMessage(sim.dropfilename);
      mjModel* model_new = LoadModel(sim.dropfilename, sim);
      sim.droploadrequest.store(false);

      mjData* data_new = nullptr;
      if (model_new) {
        data_new = mj_makeData(model_new);
      }

      if (data_new) {
        ReplaceModel(sim, model_new, data_new, sim.dropfilename);
      } else {
        mj_deleteModel(model_new);
        sim.LoadMessageClear();
      }
    }

    if (sim.uiloadrequest.load()) {
      sim.uiloadrequest.fetch_sub(1);
      sim.LoadMessage(sim.filename);

      mjModel* model_new = LoadModel(sim.filename, sim);
      mjData* data_new = nullptr;
      if (model_new) {
        data_new = mj_makeData(model_new);
      }

      if (data_new) {
        ReplaceModel(sim, model_new, data_new, sim.filename);
      } else {
        mj_deleteModel(model_new);
        sim.LoadMessageClear();
      }
    }

    if (sim.run && sim.busywait) {
      std::this_thread::yield();
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    {
      const std::unique_lock<std::recursive_mutex> lock(sim.mtx);

      if (!m) {
        continue;
      }

      if (sim.run) {
        bool stepped = false;

        const auto start_cpu = mj::Simulate::Clock::now();
        const auto elapsed_cpu = start_cpu - sync_cpu;
        const double elapsed_sim = d->time - sync_sim;

        if (sim.ctrl_noise_std) {
          mjtNum rate =
              mju_exp(-m->opt.timestep / mju_max(sim.ctrl_noise_rate, mjMINVAL));
          mjtNum scale = sim.ctrl_noise_std * mju_sqrt(1 - rate * rate);

          for (int i = 0; i < m->nu; i++) {
            ctrlnoise[i] =
                rate * ctrlnoise[i] + scale * mju_standardNormal(nullptr);
            d->ctrl[i] = ctrlnoise[i];
          }
        }

        const double slowdown = 100 / sim.percentRealTime[sim.real_time_index];
        const bool misaligned =
            mju_abs(Seconds(elapsed_cpu).count() / slowdown - elapsed_sim) >
            kSyncMisalign;

        if (elapsed_sim < 0 || elapsed_cpu.count() < 0 ||
            sync_cpu.time_since_epoch().count() == 0 || misaligned ||
            sim.speed_changed) {
          sync_cpu = start_cpu;
          sync_sim = d->time;
          sim.speed_changed = false;

          StepSimulation();
          stepped = true;
        } else {
          bool measured = false;
          const mjtNum previous_sim_time = d->time;
          const double refresh_time = kSimRefreshFraction / sim.refresh_rate;

          while (Seconds((d->time - sync_sim) * slowdown) <
                     mj::Simulate::Clock::now() - sync_cpu &&
                 mj::Simulate::Clock::now() - start_cpu <
                     Seconds(refresh_time)) {
            if (!measured && elapsed_sim) {
              sim.measured_slowdown =
                  std::chrono::duration<double>(elapsed_cpu).count() /
                  elapsed_sim;
              measured = true;
            }

            StepSimulation();
            stepped = true;

            if (d->time < previous_sim_time) {
              break;
            }
          }
        }

        if (stepped) {
          sim.AddToHistory();
        }
      } else {
        mj_forward(m, d);
        sim.speed_changed = true;
      }
    }
  }
}

void PhysicsThread(mj::Simulate* sim, const char* filename) {
  if (filename != nullptr) {
    sim->LoadMessage(filename);

    m = LoadModel(filename, *sim);
    if (m) {
      d = mj_makeData(m);
    }

    if (d) {
      sim->Load(m, d, filename);
      mj_forward(m, d);

      free(ctrlnoise);
      ctrlnoise = static_cast<mjtNum*>(malloc(sizeof(mjtNum) * m->nu));
      mju_zero(ctrlnoise, m->nu);

      if (param::config.print_scene_information) {
        PrintSceneInformation(m);
      }
      if (!g_plugin_manager.Configure(param::config, m, d)) {
        sim->exitrequest.store(true);
      }
    } else {
      mj_deleteModel(m);
      m = nullptr;
      sim->LoadMessageClear();
    }
  }

  PhysicsLoop(*sim);

  free(ctrlnoise);
  ctrlnoise = nullptr;

  mj_deleteData(d);
  mj_deleteModel(m);
  d = nullptr;
  m = nullptr;
}

void UserKeyCallback(GLFWwindow* window, int key, int scancode, int act,
                     int mods) {
  (void)window;
  (void)scancode;
  (void)mods;

  if (act != GLFW_PRESS) {
    return;
  }

  if (key == GLFW_KEY_BACKSPACE && g_sim && m && d) {
    const std::unique_lock<std::recursive_mutex> lock(g_sim->mtx);
    mj_resetData(m, d);
    mj_forward(m, d);
    g_plugin_manager.Reset(m, d);
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::printf("MuJoCo version %s\n", mj_versionString());
  if (mjVERSION_HEADER != mj_version()) {
    mju_error("Headers and library have different versions");
  }

  ScanPluginLibraries();

  const std::filesystem::path executable_dir = GetExecutableDir();
  std::filesystem::path agilex_mujoco_root = FindAgilexMujocoRoot(executable_dir);

  if (agilex_mujoco_root.empty()) {
    agilex_mujoco_root = FindAgilexMujocoRoot(std::filesystem::current_path());
  }

  if (agilex_mujoco_root.empty()) {
    std::cerr << "Failed to locate agilex_mujoco root directory." << std::endl;
    return EXIT_FAILURE;
  }

  param::config.config_file = agilex_mujoco_root / "simulate" / "config.yaml";
  param::ParseConfigPathOnly(argc, argv);

  if (param::config.config_file.is_relative()) {
    param::config.config_file =
        std::filesystem::absolute(param::config.config_file);
  }

  param::config.LoadFromYaml(param::config.config_file);
  param::ParseCommandLine(argc, argv);
  if (!g_plugin_manager.CreatePlugins(param::config.plugins)) {
    return EXIT_FAILURE;
  }

  param::config.robot_scene = ResolveScenePath(agilex_mujoco_root);

  if (!std::filesystem::exists(param::config.robot_scene)) {
    std::cerr << "Scene file does not exist: "
              << param::config.robot_scene << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Agilex MuJoCo root: " << agilex_mujoco_root << std::endl;
  std::cout << "Config file: " << param::config.config_file << std::endl;
  std::cout << "Robot: " << param::config.robot << std::endl;
  std::cout << "Scene: " << param::config.robot_scene << std::endl;

  mjvCamera cam;
  mjv_defaultCamera(&cam);

  mjvOption opt;
  mjv_defaultOption(&opt);

  mjvPerturb pert;
  mjv_defaultPerturb(&pert);

  auto sim = std::make_unique<mj::Simulate>(
      std::make_unique<mj::GlfwAdapter>(), &cam, &opt, &pert,
      /*is_passive=*/false);

  g_sim = sim.get();

  std::thread physics_thread(
      &PhysicsThread, sim.get(), param::config.robot_scene.c_str());

  glfwSetKeyCallback(
      static_cast<mj::GlfwAdapter*>(sim->platform_ui.get())->window_,
      UserKeyCallback);

  sim->RenderLoop();

  physics_thread.join();
  g_sim = nullptr;

  return EXIT_SUCCESS;
}
