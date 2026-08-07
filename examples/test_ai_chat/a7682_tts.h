/**
 * @file      a7682_tts.h
 * @brief     A7682E text-to-speech helper for the Voice AI screen.
 */
#pragma once

bool a7682_tts_init();
bool a7682_tts_speak(const char *text);
void a7682_tts_stop();
bool a7682_tts_is_active();
