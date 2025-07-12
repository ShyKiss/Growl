#pragma once
#include "PCH.h"

namespace GUI {
	extern bool dragging;
	extern POINT dragStartMousePos;
	extern int dragStartWindowX;
	extern int dragStartWindowY;

	void HandleWindowDrag(GLFWwindow* window, const ImVec2& dragAreaPos, const ImVec2& dragAreaSize);
	void RenderLoop();
	void Init();
}