 
/*
 *
 *	This init find initializes all of the gui!
 *	Without this, gui elements may not function.
 *		-Onee
 *
 */

// Stuff to init with
#include "../framework/drawmgr.hpp"		// So we can get drawmgr to draw our stuff

// Stuff to init
#include "gui.hpp"
#include "hudstrings/sidestrings.hpp"

#include "init.hpp"

namespace gui {
	
void Init() {
	
	// Setup the draw manager to run gui
	drawmgr::RequestDrawOnDraw(g_pGUI()->Update());
	
	// Other gui elements
	sidestrings::Init();	
}
	
}