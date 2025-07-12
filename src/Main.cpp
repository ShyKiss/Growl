#include "PCH.h"
#include "GUI/GUI.h"

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
	GUI::Init();
	
	return 0;
}