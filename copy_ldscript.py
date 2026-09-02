import os

Import("env")

# 获取 ESP8266 Arduino 框架目录
framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif8266")
ld_file = os.path.join(framework_dir, "tools", "sdk", "ld", "eagle.flash.4m.ld")

# 读取默认链接脚本
with open(ld_file, "r") as f:
    content = f.read()

# 备份原始文件（只备份一次）
backup_file = ld_file + ".bak"
if not os.path.exists(backup_file):
    with open(backup_file, "w") as f:
        f.write(content)
    print(f"[custom-ld] Backup original ldscript to: {backup_file}")

# 修改 irom0_0_seg 的大小，从 0xfeff0(约1MB) 改成 0x1fcff0(约2MB)
# 容纳中文字体(16KB)从RAM移到Flash
if "len = 0xfeff0" in content:
    content = content.replace("len = 0xfeff0", "len = 0x1fcff0")
    print(f"[custom-ld] Modified irom0_0_seg size: 0xfeff0 -> 0x1fcff0")
else:
    print(f"[custom-ld] WARNING: Could not find 'len = 0xfeff0' in ldscript")

# 写回文件
with open(ld_file, "w") as f:
    f.write(content)

print(f"[custom-ld] Modified default ldscript: {ld_file}")
