#pragma once
#include <Arduino.h>

void ScreenShares_Enter();
void ScreenShares_Draw();
bool ScreenShares_Tick();
bool ScreenShares_HandleTap(int x, int y);
void ScreenShares_Invalidate();
