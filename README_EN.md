<div align="center">
  <h1>Agilex_Mujoco</h1>
  <table>
    <tr>
      <td align="center">
        <a href="https://global.agilex.ai/products/piper">
          <img src="images/piper.png" alt="AgileX PiPER" width="280" />
        </a>
        <br />
        <sub><strong>AgileX PiPER</strong></sub>
      </td>
      <td align="center">
        <a href="https://global.agilex.ai/products/nero">
          <img src="images/nero.png" alt="AgileX NERO" width="280" />
        </a>
        <br />
        <sub><strong>AgileX NERO</strong></sub>
      </td>
    </tr>
  </table>
    <p>
      <a href="./readme.md"><kbd>中文</kbd></a>
      <strong><kbd>English</kbd></strong>
    </p>
</div>

# AgileX MuJoCo Simulation Base Description

This project provides the MuJoCo simulation base for the AgileX robotic arm series. As development continues, it will gradually support ROS 2 control, SDK control, and reinforcement learning control, enabling a smooth sim-to-sim transition and eventually sim-to-real deployment.

# TODO

- [x] Build the base environment with Piper robotic arm support
- [x] Refactor the base environment to provide a cleaner simulation base
- [ ] Add a Python version of the simulation base
- [x] Add support for the AgileX Nero robotic arm
- [x] Complete the ROS 2 function test module control
- [ ] Refactor the ROS 2 function modules and decouple their functionality
- [ ] Add AgileX SDK control integration tests
- [ ] Advance reinforcement learning deployment tests
- [ ] Advance vision-based grasping tests
- [ ] Advance deployment tests for various VAs
- [ ] Advance deployment tests for various VLAs

# Quick Start
## Build agilex_arm_mujoco
```bash
git clone https://github.com/yanyuze1/agilex_arm_mujoco.git
cd agilex_arm_mujoco/simulate
mkdir build && cd build
cmake ..
make -j4
```

## ROS 2 Control Test Plugin

Modify `simulate/CMakeLists.txt`:
```bash
option(AGILEX_ENABLE_ROS2_CONTROL "Build ROS2 control plugin" OFF)
# Change to
option(AGILEX_ENABLE_ROS2_CONTROL "Build ROS2 control plugin" ON)
```

Then build:
```bash
git clone https://github.com/yanyuze1/agilex_ws.git
cd agilex_ws
colcon build --symlink-install --packages-up-to mujoco_ros2_control agilex_piper_mujoco
source install/setup.bash
cd agilex_mujoco/simulate
mkdir build && cd build
cmake ..
make -j4
```

## Run agilex_arm_mujoco
```bash
./agilex_mujoco -h
./agilex_mujoco -r piper -s scene.xml
./agilex_mujoco -r nero -s scene.xml
./agilex_mujoco -r piper -s piper_slope_demo.xml -p ros2_control
```

If the program runs successfully, it will launch a MuJoCo simulation environment and display the robotic arm inside the simulation.

![alt text](images/mujoco_piper.png)

![alt text](images/mujoco_nero.png)

# References and Acknowledgements

- https://github.com/unitreerobotics/unitree_mujoco
