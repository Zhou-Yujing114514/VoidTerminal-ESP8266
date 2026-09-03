#ifndef FONT_FLASH_SECTION_H
#define FONT_FLASH_SECTION_H

/*
 * 字体段补丁：让 U8g2 中文字体数组进入 flash（irom 数据段），而不是 RAM 的 .rodata。
 *
 * 问题：U8g2_for_Adafruit_GFX 的 u8g2_fonts.h 仅为 AVR 定义 U8X8_FONT_SECTION
 *       与 u8x8_pgm_read，在 ESP8266 上两个宏都不生效：
 *         1. U8X8_FONT_SECTION 为空 -> const 字体数组进 RAM 的 .rodata，吞噬 80KB DRAM
 *         2. u8x8_pgm_read 为直接指针解引用 -> ESP8266 上直接读 flash 未对齐字节会崩溃
 *
 * 修复（复刻原作者甘草酸不酸对 U8g2 库的两处改动）：
 *   1. 用 name 参数给每个字体生成独立 section 名（.irom.text.<name>，ESP8266 官方
 *      PROGMEM 数据段），配合 --gc-sections 只保留被引用的字体。
 *   2. u8x8_pgm_read 改用 pgm_read_byte（ESP8266 官方 32 位对齐读取）。
 *
 * 注意：
 *   - 段名必须是 .irom.text（数据段，支持字节读取），不能用 .irom0.text（代码段）。
 *   - 不要写成固定段名（如 section(".irom0.text")），否则所有字体挤进同一段，
 *     --gc-sections 无法单独丢弃，会重蹈 v2.3.0 段溢出覆辙。
 */
#if defined(ESP8266)
#include <sys/pgmspace.h>

#ifndef U8X8_FONT_SECTION
#define U8X8_FONT_SECTION(name) __attribute__((section(".irom.text." name)))
#endif

#ifndef u8x8_pgm_read
#define u8x8_pgm_read(adr) pgm_read_byte(adr)
#endif
#endif

#endif // FONT_FLASH_SECTION_H
