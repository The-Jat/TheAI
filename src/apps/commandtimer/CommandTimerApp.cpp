/*
 * Copyright 2020. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Jason Scaroni, jscaroni@calpoly.edu
 *	Humdinger, humdingerb@gmail.com
 */

#include "CommandTimerApp.h"


void CommandTimerApp::ArgvReceived(int32 argc, char** argv)
{
debug_printf("CommandTimerApp:: ArgvReceived\n");

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


CommandTimerApp::CommandTimerApp()
	:
	BApplication("application/x-vnd.jas.CommandTimer")
{
	BRect cTWindowRect(100, 100, 525, 240);
	cTWindow = new CommandTimerWindow(cTWindowRect, fCommand);
	cTWindow->Show();
}
