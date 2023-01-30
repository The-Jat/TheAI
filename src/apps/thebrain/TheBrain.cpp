/*
 * Copyright 2020. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Jason Scaroni, jscaroni@calpoly.edu
 *	Humdinger, humdingerb@gmail.com
 */

#include "TheBrainApp.h"


int
main()
{
	TheBrainApp* cTApp;

	cTApp = new TheBrainApp();
	cTApp->Run();
	cTApp->PostMessage(B_QUIT_REQUESTED);
	delete cTApp;

	return 0;
}
