#pragma once
#include "PCH.h"
#include "BNKManager/BNKManager.h"
#include <BNKManager/Searcher.h>

namespace Menu {
	extern std::string StatusMessage;
	extern BNKManager* CurrentBnk;
	extern Searcher* CurrentSearcher;

	extern std::vector<int> selected_indices;
	extern int selected_index;
	extern int last_selected_index;

	void DrawMainWindow(GLFWwindow* window);
}