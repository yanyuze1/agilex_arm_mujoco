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
          <img src="https://global.agilex.ai/cdn/shop/files/7_-_3.png?v=1765176593&amp;width=1946" alt="AgileX NERO" width="280" />
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
- [ ] Add support for the AgileX Nero robotic arm
- [ ] Complete ROS 2 control modules
- [ ] Add AgileX SDK control integration
- [ ] Advance reinforcement learning deployment tests
- [ ] Advance vision-based grasping tests
- [ ] Advance deployment tests for various VAs
- [ ] Advance deployment tests for various VLAs

# Quick Start

## Build agilex_arm_mujoco

```bash
git clone https://github.com/yanyuze1/agilex_arm_mujoco.git
cd agilex_arm_mujoco
mkdir build && cd build
cmake ..
make -j4
```

## Run agilex_arm_mujoco

```bash
./agilex_mujoco -r piper -s scene.xml
```

If the program runs successfully, it will launch a MuJoCo simulation environment and display the Piper robotic arm inside the simulation.

# References

- https://github.com/unitreerobotics/unitree_mujoco
