#pragma once

#include <Windows.h>
#include "Core/EngineString.h"

class FSplashScreen
{
public:
	static void Show(HINSTANCE hInstance, const FString& ImagePath);
	static void SetText(const FString& Text);
	static void SetText(const FWideString& Text);

	static void SetProgress(float Ratio);
	static void Hide();

};