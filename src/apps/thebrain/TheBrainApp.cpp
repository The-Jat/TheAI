/*
 * Copyright 2020. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Jason Scaroni, jscaroni@calpoly.edu
 *	Humdinger, humdingerb@gmail.com
 */

#include "TheBrainApp.h"


void TheBrainApp::ArgvReceived(int32 argc, char** argv)
{
debug_printf("TheBrainApp:: ArgvReceived\n");

	for(int i = 1; i<argc; i++){
		debug_printf("received = %s \n",argv[i]);
		debug_printf("received second arg = %s \n",argv[i++]);
		const char* arg = argv[i];
		
		if(*arg == '-'){
			if( strcmp(arg, "-r") == 0 || strcmp(arg, "--run") == 0 )
			{
				fCommand = argv[++i];
			}
		}
	}
}


TheBrainApp::TheBrainApp()
	:
	BApplication("application/x-vnd.jas.TheBrain")
{
	BRect cTWindowRect(100, 100, 525, 240);
	cTWindow = new TheBrainWindow(cTWindowRect, fCommand);
	cTWindow->Show();
}
