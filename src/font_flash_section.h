#ifndef FONT_FLASH_SECTION_H
#define FONT_FLASH_SECTION_H

/*
 * 字体段补丁：让 U8g2 中文字体数组进入 flash，而不是 RAM 的 .rodata
 *
 * 问题：U8g2_for_Adafruit_GFX 的 u8g2_fonts.h 仅为 AVR 定义 U8X8_FONT_SECTION，
 *       在 ESP8266 上该宏为空，导致所有 const 字体数组被链接进 RAM 的 .rodata，
 *       吞噬宝贵的 80KB DRAM（wqy14_chinese2 约 16KB），导致启动崩溃反复复位。
 *
 * 修复：通过 -include 预定义该宏，使字体数组落入 flash 的 .irom0.text 段
 *       （该段映射到 0x40200000 可直接指针访问，u8x8_pgm_read 在非 AVR 上是直接指针读取）。
 */
#if defined(ESP8266) && !defined(U8X8_FONT_SECTION)
#define U8X8_FONT_SECTION(name) __attribute__((section(".irom0.text")))
#endif

#endif // FONT_FLASH_SECTION_H
