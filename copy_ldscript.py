import shutil
import os

Import("env")

# 获取 ESP8266 Arduino 框架目录
framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif8266")
ld_dir = os.path.join(framework_dir, "tools", "sdk", "ld")

# 把项目中的自定义链接脚本复制到框架目录
src = os.path.join(env["PROJECT_DIR"], "eagle.flash.4m.2m_app.ld")
dst = os.path.join(ld_dir, "eagle.flash.4m.2m_app.ld")

if os.path.exists(src):
    shutil.copy(src, dst)
    print(f"[custom-ld] Copied custom ldscript to: {dst}")
else:
    print(f"[custom-ld] WARNING: Source ldscript not found: {src}")
