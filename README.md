# LicheeRV-Nano-Build
board wiki: https://wiki.sipeed.com/hardware/zh/lichee/RV_Nano/1_intro.html

# download source

```
git clone https://github.com/sipeed/LicheeRV-Nano-Build --depth=1
cd LicheeRV-Nano-Build
git clone https://github.com/sophgo/host-tools --depth=1
```

## host environment

you can use container:

```
cd host/ubuntu
docker build -t licheervnano-build-ubuntu .
docker run --name licheervnano-build-ubuntu licheervnano-build-ubuntu
docker export licheervnano-build-ubuntu | sqfstar licheervnano-build-ubuntu.sqfs
singularity shell -e licheervnano-build-ubuntu.sqfs
```

如果执行 `singularity shell` 提示找不到 `singularity`，需要先在宿主机安装
容器运行工具。可以安装 Apptainer，然后使用 `apptainer shell` 进入上述镜像。

Deepin 25（amd64）可使用以下官方 Debian 安装包，安装时需要输入 sudo 密码：

```bash
wget -O /tmp/apptainer_1.5.4_amd64.deb https://github.com/apptainer/apptainer/releases/download/v1.5.4/apptainer_1.5.4_amd64.deb
sudo apt-get install -y /tmp/apptainer_1.5.4_amd64.deb
```

安装完成后，在仓库的 `host/ubuntu` 目录执行：

```bash
apptainer shell -e licheervnano-build-ubuntu.sqfs
```

其他宿主系统或架构的安装方式请参考
[Apptainer 官方安装文档](https://apptainer.org/docs/admin/main/installation.html)。

# build it

```
source build/cvisetup.sh
# C906:
defconfig sg2002_licheervnano_sd
# A53:
# defconfig sg2002_licheea53nano_sd
build_all
```

## LVGL framebuffer demos

SG200X 的两个 Buildroot 默认配置已启用 LVGL 8.3.11 和 framebuffer
测试 demo。源码在后续构建时由 Buildroot 下载；正常 `build_all` 会将
`lvgl_demo` 安装到根文件系统的 `/usr/bin/`。

也可在 Buildroot menuconfig 的 `Target packages` →
`Graphic libraries and applications (graphic/text)` → `Graphic libraries`
中选择 `lvgl` 和 `framebuffer test demos`。

配置好工程及交叉工具链后，先确认当前生效的 `buildroot/.config` 中有
以下选项（修改 defconfig 不会自动更新已有的 `.config`）：

```text
BR2_PACKAGE_LVGL=y
BR2_PACKAGE_LVGL_DEMO=y
```

如未启用，运行 `make -C buildroot menuconfig`，选择上述选项并保存。
首次构建软件包：

```sh
make -C buildroot lvgl
```

如果此前已编译 LVGL，之后才启用 demo，必须强制重新配置；普通 make
会沿用先前的 CMake 缓存和构建完成标记：

```sh
make -C buildroot lvgl-reconfigure
```

本工程启用了 Buildroot 的 per-package 目录，单独构建后的程序位于：

```text
buildroot/output/build/lvgl-8.3.11/lvgl_demo
buildroot/output/per-package/lvgl/target/usr/bin/lvgl_demo
```

再执行完整构建，汇总到 `output/target` 并更新 rootfs 镜像：

```sh
make -C buildroot
ls -l buildroot/output/target/usr/bin/lvgl_demo
```

在板端运行（默认使用 `/dev/fb0`）：

```sh
lvgl_demo widgets
lvgl_demo benchmark
lvgl_demo stress
lvgl_demo widgets /dev/fb1
```

按 Ctrl+C 退出。需要内核及显示驱动提供可 mmap 的 Linux framebuffer；
分辨率由设备读取，支持 packed truecolor 16/24/32 bpp。
此移植用于显示测试，暂未注册触摸、鼠标或键盘输入设备。
运行前请停止其他占用同一 framebuffer 的图形程序。
LVGL 的静态库及头文件安装到 Buildroot staging 目录，应用须使用同一份
`lv_conf.h`；配置位于 `buildroot/package/lvgl/lv_conf.h`。

# build fail

on some system, qt5svg or qt5base will build failed on first build, please retry command:

```
build_all
```

# how to modify image after build:

```
# first partition
touch wifi.sta
mcopy -i install/xxx/xxx.img@@1s wifi.sta ::/

# second partition
./host/mount_ext4.sh install/xxx/xxx.img mountpoint
cd mountpoint
touch xxx
```

# logo

```
./host/make_logo.sh input.jpeg logo.jpeg
mcopy -i install/xxx/xxx.img@@1s logo.jpeg ::/
```
