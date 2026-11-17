#pragma once

#include <imgui.h>
#include <imgui_internal.h>

class IEditorPanel
{
public:
	virtual ~IEditorPanel() = default;
	virtual bool Init() = 0;
	virtual void Tick(float DeltaTime) = 0;
	virtual void OnRender() = 0;
	// 표시되지 않는 프레임에 이전 입력 상태를 해제한다.
	virtual void OnHidden() {}

	bool IsOpen() const { return bIsOpen; }
	void SetOpen(bool bOpen) { bIsOpen = bOpen; }
	virtual const char* GetPanelName() const = 0;

private:
	bool bIsOpen = true;
};