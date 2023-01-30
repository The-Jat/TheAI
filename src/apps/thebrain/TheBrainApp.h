/*
 * Copyright 2020. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Jason Scaroni, jscaroni@calpoly.edu
 *	Humdinger, humdingerb@gmail.com
 */

#ifndef THEBRAINAPP_H
#define THEBRAINAPP_H

#include "TheBrainWindow.h"
#include <Application.h>

class TheBrainApp : public BApplication
{
public:
	TheBrainApp();
	virtual void ArgvReceived(int32 argc, char** argv);


private:
	TheBrainWindow* cTWindow;
	 const char* fCommand;
};

#endif
