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
      <strong><kbd>中文</kbd></strong>
      <a href="./README_EN.md"><kbd>English</kbd></a>
    </p>
</div>

# AgileX MuJoCo 仿真基座描述
本项目是AgileX机械臂系列的MuJoCo仿真基座，会伴随项目的不断开发逐步兼容ROS2控制、SDK控制、强化学习控制，完成sim2sim的顺利过渡，最后完成sim2real。

# TODO

- [x] 基础环境构建，支持 Piper 机械臂
- [x] 对基础环境进行重构，提供一个更简洁的仿真环境基座
- [ ] 添加python版本仿真环境基座
- [ ] 添加 Agilex Nero 机械臂支持
- [x] 完成ros2功能测试模块控制
- [ ] 对ros2功能模块进行重构,将功能进行解耦
- [ ] 添加agilex sdk控制集成测试
- [ ] 推进强化学习部署测试
- [ ] 推进视觉抓取测试
- [ ] 推进各类 VA 的部署测试
- [ ] 推进各类 VLA 的部署测试

# 快速开始
## 编译agilex_arm_mujoco
```bash
git clone https://github.com/yanyuze1/agilex_arm_mujoco.git
cd agilex_arm_mujoco/simulate
mkdir build && cd build
cmake ..
make -j4
```

## ros2 control测试插件

对simulate/CMakeLists.txt进行修改
```bash
option(AGILEX_ENABLE_ROS2_CONTROL "Build ROS2 control plugin" OFF)
#改为
option(AGILEX_ENABLE_ROS2_CONTROL "Build ROS2 control plugin" ON)
```
进行编译
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

## 运行agilex_arm_mujoco
```bash
./agilex_mujoco -h
./agilex_mujoco -r piper -s scene.xml
./agilex_mujoco -r piper -s scene.xml -p ros2_control
```
此时运行成功，会启动一个MuJoCo仿真环境，并显示Piper机械臂在仿真环境当中。

![alt text](images/mujoco_piper.png)

# 参考与感谢
- https://github.com/unitreerobotics/unitree_mujoco
