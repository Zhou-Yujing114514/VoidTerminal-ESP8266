#ifndef FONT_FLASH_SECTION_H
#define FONT_FLASH_SECTION_H

/*
 * 字体段补丁：让 U8g2 中文字体数组进入 flash（irom0），而不是 RAM 的 .rodata。
 *
 * 问题：U8g2_for_Adafruit_GFX 的 u8g2_fonts.h 仅为 AVR 定义 U8X8_FONT_SECTION，
 *       在 ESP8266 上该宏为空，const 字体数组被链接进 RAM 的 .rodata，吞噬 80KB DRAM。
 *
 * 修复：复刻 AVR 的做法，利用 name 参数给每个字体生成独立的 section 名
 *       （如 .irom0.text.u8g2_font_wqy14_t_gb2312a）。
 *       配合 -fdata-sections 与链接器 --gc-sections，只有被引用的字体才会被链接，
 *       其余字体会被丢弃，不会导致段溢出。
 *
 * 注意：不要写成固定段名（如 section(".irom0.text")），否则所有字体挤进同一段，
 *       --gc-sections 无法单独丢弃，会重蹈 v2.3.0 段溢出覆辙。
 */
#if defined(ESP8266) && !defined(U8X8_FONT_SECTION)
#define U8X8_FONT_SECTION(name) __attribute__((section(".irom0.text." name)))
#endif

#endif // FONT_FLASH_SECTION_H
