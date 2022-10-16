/*
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Copyright 2011-2014, Rene Gollent, rene@gollent.com.
 * Copyright 2005-2009, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "nnWindow.h"
#include "nn.hpp"

#include <map>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include <AppMisc.h>
#include <AutoDeleter.h>
#include <Autolock.h>
#include <debug_support.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Invoker.h>
#include <Path.h>

#include <DriverSettings.h>
#include <MessengerPrivate.h>
#include <RegExp.h>
#include <RegistrarDefs.h>
#include <RosterPrivate.h>
#include <Server.h>
#include <StringList.h>

#include <util/DoublyLinkedList.h>

#include<wren.hpp>//wren language

#include <termios.h>
#include <File.h>
#include "DPath.h"

// command pipe include
#include <private/shared/CommandPipe.h>

class DPath;
class BFile;

static const char* kDebuggerSignature = "application/x-vnd.Haiku-nn";
static const int32 MSG_DEBUG_THIS_TEAM = 'dbtt';


//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TRACE(x) debug_printf x
#else
#	define TRACE(x) ;
#endif


using std::map;
using std::nothrow;


static const char *kSignature = "application/x-vnd.Haiku-nn";



static status_t
action_for_string(const char* action, int32& _action)
{
	if (strcmp(action, "kill") == 0)
		_action = kActionKillTeam;
	else if (strcmp(action, "debug") == 0)
		_action = kActionDebugTeam;
	else if (strcmp(action, "log") == 0
		|| strcmp(action, "report") == 0) {
		_action = kActionSaveReportTeam;
	} else if (strcasecmp(action, "core") == 0)
		_action = kActionWriteCoreFile;
	else if (strcasecmp(action, "user") == 0)
		_action = kActionPromptUser;
	else
		return B_BAD_VALUE;

	return B_OK;
}


static bool
match_team_name(const char* teamName, const char* parameterName)
{
	RegExp expressionMatcher;
	if (expressionMatcher.SetPattern(parameterName,
		RegExp::PATTERN_TYPE_WILDCARD)) {
		BString value = teamName;
		if (parameterName[0] != '/') {
			// the expression in question is a team name match only,
			// so we need to extract that.
			BPath path(teamName);
			if (path.InitCheck() == B_OK)
				value = path.Leaf();
		}

		RegExp::MatchResult match = expressionMatcher.Match(value);
		if (match.HasMatched())
			return true;
	}

	return false;
}


static status_t
action_for_team(const char* teamName, int32& _action,
	bool& _explicitActionFound)
{
	status_t error = B_OK;
	BPath path;
	error = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (error != B_OK)
		return error;

	path.Append("system/debug_server/settings");
	BDriverSettings settings;
	error = settings.Load(path.Path());
	if (error != B_OK)
		return error;

	int32 tempAction;
	if (action_for_string(settings.GetParameterValue("default_action",
		"user", "user"), tempAction) == B_OK) {
		_action = tempAction;
	} else
		_action = kActionPromptUser;
	_explicitActionFound = false;

	BDriverParameter parameter = settings.GetParameter("executable_actions");
	for (BDriverParameterIterator iterator = parameter.ParameterIterator();
		iterator.HasNext();) {
		BDriverParameter child = iterator.Next();
		if (!match_team_name(teamName, child.Name()))
			continue;

		if (child.CountValues() > 0) {
			if (action_for_string(child.ValueAt(0), tempAction) == B_OK) {
				_action = tempAction;
				_explicitActionFound = true;
			}
		}

		break;
	}

	return B_OK;
}


static void
KillTeam(team_id team, const char *appName = NULL)
{
	// get a team info to verify the team still lives
	team_info info;
	if (!appName) {
		status_t error = get_team_info(team, &info);
		if (error != B_OK) {
			debug_printf("debug_server: KillTeam(): Error getting info for "
				"team %" B_PRId32 ": %s\n", team, strerror(error));
			info.args[0] = '\0';
		}

		appName = info.args;
	}

	debug_printf("debug_server: Killing team %" B_PRId32 " (%s)\n", team,
		appName);

	kill_team(team);
}


// #pragma mark -


class DebugMessage : public DoublyLinkedListLinkImpl<DebugMessage> {
public:
	DebugMessage()
	{
	}

	void SetCode(debug_debugger_message code)		{ fCode = code; }
	debug_debugger_message Code() const				{ return fCode; }

	debug_debugger_message_data &Data()				{ return fData; }
	const debug_debugger_message_data &Data() const	{ return fData; }

private:
	debug_debugger_message		fCode;
	debug_debugger_message_data	fData;
};

typedef DoublyLinkedList<DebugMessage>	DebugMessageList;


class NNHandler : public BLocker {
public:
	NNHandler(team_id team);
	~NNHandler();

	status_t Init(port_id nubPort);

	team_id Team() const;

	status_t PushMessage(DebugMessage *message);

private:
	status_t _PopMessage(DebugMessage *&message);

	thread_id _EnterDebugger(bool saveReport);
	status_t _SetupGDBArguments(BStringList &arguments, bool usingConsoled);
	status_t _WriteCoreFile();
	void _KillTeam();

	int32 _HandleMessage(DebugMessage *message);

	void _LookupSymbolAddress(debug_symbol_lookup_context *lookupContext,
		const void *address, char *buffer, int32 bufferSize);
	void _PrintStackTrace(thread_id thread);
	void _NotifyAppServer(team_id team);
	void _NotifyRegistrar(team_id team, bool openAlert, bool stopShutdown);

	status_t _InitGUI();

	static status_t _HandlerThreadEntry(void *data);
	status_t _HandlerThread();

	bool _ExecutableNameEquals(const char *name) const;
	bool _IsAppServer() const;
	bool _IsInputServer() const;
	bool _IsRegistrar() const;
	bool _IsGUIServer() const;

	static const char *_LastPathComponent(const char *path);
	static team_id _FindTeam(const char *name);
	static bool _AreGUIServersAlive();

private:
	DebugMessageList		fMessages;
	sem_id					fMessageCountSem;
	team_id					fTeam;
	team_info				fTeamInfo;
	char					fExecutablePath[B_PATH_NAME_LENGTH];
	thread_id				fHandlerThread;
	debug_context			fDebugContext;
};


class NNHandlerRoster : public BLocker {
private:
	NNHandlerRoster()
		:
		BLocker("team debug handler roster")
	{
	}

public:
	static NNHandlerRoster *CreateDefault()
	{
		if (!sRoster)
			sRoster = new(nothrow) NNHandlerRoster;

		return sRoster;
	}

	static NNHandlerRoster *Default()
	{
		return sRoster;
	}

	bool AddHandler(NNHandler *handler)
	{
		if (!handler)
			return false;

		BAutolock _(this);

		fHandlers[handler->Team()] = handler;

		return true;
	}

	NNHandler *RemoveHandler(team_id team)
	{
		BAutolock _(this);

		NNHandler *handler = NULL;

		NNHandlerMap::iterator it = fHandlers.find(team);
		if (it != fHandlers.end()) {
			handler = it->second;
			fHandlers.erase(it);
		}

		return handler;
	}

	NNHandler *HandlerFor(team_id team)
	{
		BAutolock _(this);

		NNHandler *handler = NULL;

		NNHandlerMap::iterator it = fHandlers.find(team);
		if (it != fHandlers.end())
			handler = it->second;

		return handler;
	}

	status_t DispatchMessage(DebugMessage *message)
	{
		if (!message)
			return B_BAD_VALUE;

		ObjectDeleter<DebugMessage> messageDeleter(message);

		team_id team = message->Data().origin.team;

		// get the responsible team debug handler
		BAutolock _(this);

		NNHandler *handler = HandlerFor(team);
		if (!handler) {
			// no handler yet, we need to create one
			handler = new(nothrow) NNHandler(team);
			if (!handler) {
				KillTeam(team);
				return B_NO_MEMORY;
			}

			status_t error = handler->Init(message->Data().origin.nub_port);
			if (error != B_OK) {
				delete handler;
				KillTeam(team);
				return error;
			}

			if (!AddHandler(handler)) {
				delete handler;
				KillTeam(team);
				return B_NO_MEMORY;
			}
		}

		// hand over the message to it
		handler->PushMessage(message);
		messageDeleter.Detach();

		return B_OK;
	}

private:
	typedef map<team_id, NNHandler*>	NNHandlerMap;

	static NNHandlerRoster	*sRoster;

	NNHandlerMap				fHandlers;
};


NNHandlerRoster *NNHandlerRoster::sRoster = NULL;


class NN : public BServer {
public:
	NN(status_t &error);

	status_t Init();

	virtual bool QuitRequested();

private:
	static status_t _ListenerEntry(void *data);
	status_t _Listener();

	void _DeleteNNHandler(NNHandler *handler);

private:
	typedef map<team_id, NNHandler*>	NNHandlerMap;

	port_id				fListenerPort;
	thread_id			fListener;
	bool				fTerminating;
};


// #pragma mark -


NNHandler::NNHandler(team_id team)
	:
	BLocker("team debug handler"),
	fMessages(),
	fMessageCountSem(-1),
	fTeam(team),
	fHandlerThread(-1)
{
	fDebugContext.nub_port = -1;
	fDebugContext.reply_port = -1;

	fExecutablePath[0] = '\0';
}


NNHandler::~NNHandler()
{
	// delete the message count semaphore and wait for the thread to die
	if (fMessageCountSem >= 0)
		delete_sem(fMessageCountSem);

	if (fHandlerThread >= 0 && find_thread(NULL) != fHandlerThread) {
		status_t result;
		wait_for_thread(fHandlerThread, &result);
	}

	// destroy debug context
	if (fDebugContext.nub_port >= 0)
		destroy_debug_context(&fDebugContext);

	// delete the remaining messages
	while (DebugMessage *message = fMessages.Head()) {
		fMessages.Remove(message);
		delete message;
	}
}


status_t
NNHandler::Init(port_id nubPort)
{
debug_printf("NNHandler::Init\n");



	// get the team info for the team
	status_t error = get_team_info(fTeam, &fTeamInfo);
	if (error != B_OK) {
		debug_printf("debug_server: NNHandler::Init(): Failed to get "
			"info for team %" B_PRId32 ": %s\n", fTeam, strerror(error));
		return error;
	}

	// get the executable path
	error = BPrivate::get_app_path(fTeam, fExecutablePath);
	if (error != B_OK) {
		debug_printf("debug_server: NNHandler::Init(): Failed to get "
			"executable path of team %" B_PRId32 ": %s\n", fTeam,
			strerror(error));

		fExecutablePath[0] = '\0';
	}

	// init a debug context for the handler
	error = init_debug_context(&fDebugContext, fTeam, nubPort);
	if (error != B_OK) {
		debug_printf("debug_server: NNHandler::Init(): Failed to init "
			"debug context for team %" B_PRId32 ", port %" B_PRId32 ": %s\n",
			fTeam, nubPort, strerror(error));
		return error;
	}

	// set team flags
	debug_nub_set_team_flags message;
	message.flags = B_TEAM_DEBUG_PREVENT_EXIT;

	send_debug_message(&fDebugContext, B_DEBUG_MESSAGE_SET_TEAM_FLAGS, &message,
		sizeof(message), NULL, 0);

	// create the message count semaphore
	char name[B_OS_NAME_LENGTH];
	snprintf(name, sizeof(name), "team %" B_PRId32 " message count", fTeam);
	fMessageCountSem = create_sem(0, name);
	if (fMessageCountSem < 0) {
		debug_printf("debug_server: NNHandler::Init(): Failed to create "
			"message count semaphore: %s\n", strerror(fMessageCountSem));
		return fMessageCountSem;
	}

	// spawn the handler thread
	snprintf(name, sizeof(name), "team %" B_PRId32 " handler", fTeam);
	fHandlerThread = spawn_thread(&_HandlerThreadEntry, name, B_NORMAL_PRIORITY,
		this);
	if (fHandlerThread < 0) {
		debug_printf("debug_server: NNHandler::Init(): Failed to spawn "
			"handler thread: %s\n", strerror(fHandlerThread));
		return fHandlerThread;
	}

	resume_thread(fHandlerThread);

	return B_OK;
}


team_id
NNHandler::Team() const
{
	return fTeam;
}


status_t
NNHandler::PushMessage(DebugMessage *message)
{
	BAutolock _(this);

	fMessages.Add(message);
	release_sem(fMessageCountSem);

	return B_OK;
}


status_t
NNHandler::_PopMessage(DebugMessage *&message)
{
	// acquire the semaphore
	status_t error;
	do {
		error = acquire_sem(fMessageCountSem);
	} while (error == B_INTERRUPTED);

	if (error != B_OK)
		return error;

	// get the message
	BAutolock _(this);

	message = fMessages.Head();
	fMessages.Remove(message);

	return B_OK;
}


status_t
NNHandler::_SetupGDBArguments(BStringList &arguments, bool usingConsoled)
{
	// prepare the argument vector
	BString teamString;
	teamString.SetToFormat("--pid=%" B_PRId32, fTeam);

	status_t error;
	BPath terminalPath;
	if (usingConsoled) {
		error = find_directory(B_SYSTEM_BIN_DIRECTORY, &terminalPath);
		if (error != B_OK) {
			debug_printf("debug_server: can't find system-bin directory: %s\n",
				strerror(error));
			return error;
		}
		error = terminalPath.Append("consoled");
		if (error != B_OK) {
			debug_printf("debug_server: can't append to system-bin path: %s\n",
				strerror(error));
			return error;
		}
	} else {
		error = find_directory(B_SYSTEM_APPS_DIRECTORY, &terminalPath);
		if (error != B_OK) {
			debug_printf("debug_server: can't find system-apps directory: %s\n",
				strerror(error));
			return error;
		}
		error = terminalPath.Append("Terminal");
		if (error != B_OK) {
			debug_printf("debug_server: can't append to system-apps path: %s\n",
				strerror(error));
			return error;
		}
	}

	arguments.MakeEmpty();
	if (!arguments.Add(terminalPath.Path()))
		return B_NO_MEMORY;

	if (!usingConsoled) {
		BString windowTitle;
		windowTitle.SetToFormat("Debug of Team %" B_PRId32 ": %s", fTeam,
			_LastPathComponent(fExecutablePath));
		if (!arguments.Add("-t") || !arguments.Add(windowTitle))
			return B_NO_MEMORY;
	}

	BPath gdbPath;
	error = find_directory(B_SYSTEM_BIN_DIRECTORY, &gdbPath);
	if (error != B_OK) {
		debug_printf("debug_server: can't find system-bin directory: %s\n",
			strerror(error));
		return error;
	}
	error = gdbPath.Append("gdb");
	if (error != B_OK) {
		debug_printf("debug_server: can't append to system-bin path: %s\n",
			strerror(error));
		return error;
	}
	if (!arguments.Add(gdbPath.Path()) || !arguments.Add(teamString))
		return B_NO_MEMORY;

	if (strlen(fExecutablePath) > 0 && !arguments.Add(fExecutablePath))
		return B_NO_MEMORY;

	return B_OK;
}


thread_id
NNHandler::_EnterDebugger(bool saveReport)
{
debug_printf("NNHandler::_EnterDebugger\n");

	TRACE(("debug_server: NNHandler::_EnterDebugger(): team %" B_PRId32
		"\n", fTeam));

	// prepare a debugger handover
	TRACE(("debug_server: NNHandler::_EnterDebugger(): preparing "
		"debugger handover for team %" B_PRId32 "...\n", fTeam));

	status_t error = send_debug_message(&fDebugContext,
		B_DEBUG_MESSAGE_PREPARE_HANDOVER, NULL, 0, NULL, 0);
	if (error != B_OK) {
		debug_printf("debug_server: Failed to prepare debugger handover: %s\n",
			strerror(error));
		return error;
	}

	BStringList arguments;
	const char *argv[16];
	int argc = 0;

	bool debugInConsoled = _IsGUIServer() || !_AreGUIServersAlive();
#ifdef HANDOVER_USE_GDB

	error = _SetupGDBArguments(arguments, debugInConsoled);
	if (error != B_OK) {
		debug_printf("debug_server: Failed to set up gdb arguments: %s\n",
			strerror(error));
		return error;
	}

	// start the terminal
	TRACE(("debug_server: NNHandler::_EnterDebugger(): starting  "
		"terminal (debugger) for team %" B_PRId32 "...\n", fTeam));

#elif defined(HANDOVER_USE_DEBUGGER)
	if (!debugInConsoled && !saveReport
		&& be_roster->IsRunning(kDebuggerSignature)) {

		// for graphical handovers, check if Debugger is already running,
		// and if it is, simply send it a message to attach to the requested
		// team.
		BMessenger messenger(kDebuggerSignature);
		BMessage message(MSG_DEBUG_THIS_TEAM);
		if (message.AddInt32("team", fTeam) == B_OK
			&& messenger.SendMessage(&message) == B_OK) {
			return 0;
		}
	}

	// prepare the argument vector
	BPath debuggerPath;
	if (debugInConsoled) {
		error = find_directory(B_SYSTEM_BIN_DIRECTORY, &debuggerPath);
		if (error != B_OK) {
			debug_printf("debug_server: can't find system-bin directory: %s\n",
				strerror(error));
			return error;
		}
		error = debuggerPath.Append("consoled");
		if (error != B_OK) {
			debug_printf("debug_server: can't append to system-bin path: %s\n",
				strerror(error));
			return error;
		}

		if (!arguments.Add(debuggerPath.Path()))
			return B_NO_MEMORY;
	}

	error = find_directory(B_SYSTEM_APPS_DIRECTORY, &debuggerPath);
	if (error != B_OK) {
		debug_printf("debug_server: can't find system-apps directory: %s\n",
			strerror(error));
		return error;
	}
	error = debuggerPath.Append("Debugger");
	if (error != B_OK) {
		debug_printf("debug_server: can't append to system-apps path: %s\n",
			strerror(error));
		return error;
	}
	if (!arguments.Add(debuggerPath.Path()))
		return B_NO_MEMORY;

	if (debugInConsoled && !arguments.Add("--cli"))
		return B_NO_MEMORY;

	BString debuggerParam;
	debuggerParam.SetToFormat("%" B_PRId32, fTeam);
	if (saveReport) {
		if (!arguments.Add("--save-report"))
			return B_NO_MEMORY;
	}
	if (!arguments.Add("--team") || !arguments.Add(debuggerParam))
		return B_NO_MEMORY;

	// start the debugger
	TRACE(("debug_server: NNHandler::_EnterDebugger(): starting  "
		"%s debugger for team %" B_PRId32 "...\n",
			debugInConsoled ? "command line" : "graphical", fTeam));
#endif

	for (int32 i = 0; i < arguments.CountStrings(); i++)
		argv[argc++] = arguments.StringAt(i).String();
	argv[argc] = NULL;

	thread_id thread = load_image(argc, argv, (const char**)environ);
	if (thread < 0) {
		debug_printf("debug_server: Failed to start debugger: %s\n",
			strerror(thread));
		return thread;
	}
	resume_thread(thread);

	TRACE(("debug_server: NNHandler::_EnterDebugger(): debugger started "
		"for team %" B_PRId32 ": thread: %" B_PRId32 "\n", fTeam, thread));

	return thread;
}


void
NNHandler::_KillTeam()
{
	KillTeam(fTeam, fTeamInfo.args);
}


status_t
NNHandler::_WriteCoreFile()
{
	// get a usable path for the core file
	BPath directoryPath;
	status_t error = find_directory(B_DESKTOP_DIRECTORY, &directoryPath);
	if (error != B_OK) {
		debug_printf("debug_server: Couldn't get desktop directory: %s\n",
			strerror(error));
		return error;
	}

	const char* executableName = strrchr(fExecutablePath, '/');
	if (executableName == NULL)
		executableName = fExecutablePath;
	else
		executableName++;

	BString fileBaseName("core-");
	fileBaseName << executableName << '-' << fTeam;
	BPath filePath;

	for (int32 index = 0;; index++) {
		BString fileName(fileBaseName);
		if (index > 0)
			fileName << '-' << index;

		error = filePath.SetTo(directoryPath.Path(), fileName.String());
		if (error != B_OK) {
			debug_printf("debug_server: Couldn't get core file path for team %"
				B_PRId32 ": %s\n", fTeam, strerror(error));
			return error;
		}

		struct stat st;
		if (lstat(filePath.Path(), &st) != 0) {
			if (errno == B_ENTRY_NOT_FOUND)
				break;
		}

		if (index > 1000) {
			debug_printf("debug_server: Couldn't get usable core file path for "
				"team %" B_PRId32 "\n", fTeam);
			return B_ERROR;
		}
	}

	debug_nub_write_core_file message;
	message.reply_port = fDebugContext.reply_port;
	strlcpy(message.path, filePath.Path(), sizeof(message.path));

	debug_nub_write_core_file_reply reply;

	error = send_debug_message(&fDebugContext, B_DEBUG_WRITE_CORE_FILE,
			&message, sizeof(message), &reply, sizeof(reply));
	if (error == B_OK)
		error = reply.error;
	if (error != B_OK) {
		debug_printf("debug_server: Failed to write core file for team %"
			B_PRId32 ": %s\n", fTeam, strerror(error));
	}

	return error;
}


int32
NNHandler::_HandleMessage(DebugMessage *message)
{
	// This method is called only for the first message the debugger gets for
	// a team. That means only a few messages are actually possible, while
	// others wouldn't trigger the debugger in the first place. So we deal with
	// all of them the same way, by popping up an alert.
	TRACE(("debug_server: NNHandler::_HandleMessage(): team %" B_PRId32
		", code: %" B_PRId32 "\n", fTeam, (int32)message->Code()));

	thread_id thread = message->Data().origin.thread;

	// get some user-readable message
	char buffer[512];
	switch (message->Code()) {
		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
			// This shouldn't happen.
			debug_printf("debug_server: Got a spurious "
				"B_DEBUGGER_MESSAGE_TEAM_DELETED message for team %" B_PRId32
				"\n", fTeam);
			return true;

		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
			get_debug_exception_string(
				message->Data().exception_occurred.exception, buffer,
				sizeof(buffer));
			break;

		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
		{
			// get the debugger() message
			void *messageAddress = message->Data().debugger_call.message;
			char messageBuffer[128];
			status_t error = B_OK;
			ssize_t bytesRead = debug_read_string(&fDebugContext,
				messageAddress, messageBuffer, sizeof(messageBuffer));
			if (bytesRead < 0)
				error = bytesRead;

			if (error == B_OK) {
				sprintf(buffer, "Debugger call: `%s'", messageBuffer);
			} else {
				snprintf(buffer, sizeof(buffer), "Debugger call: %p "
					"(Failed to read message: %s)", messageAddress,
					strerror(error));
			}
			break;
		}

		default:
			get_debug_message_string(message->Code(), buffer, sizeof(buffer));
			break;
	}

	debug_printf("debug_server: Thread %" B_PRId32 " entered the debugger: %s\n",
		thread, buffer);

	_PrintStackTrace(thread);

	int32 debugAction = kActionPromptUser;
	bool explicitActionFound = false;
	if (action_for_team(fExecutablePath, debugAction, explicitActionFound)
			!= B_OK) {
		debugAction = kActionPromptUser;
		explicitActionFound = false;
	}

	// ask the user whether to debug or kill the team
	if (_IsGUIServer()) {
		// App server, input server, or registrar. We always debug those.
		// if not specifically overridden.
		if (!explicitActionFound)
			debugAction = kActionDebugTeam;
	} else if (debugAction == kActionPromptUser && USE_GUI
		&& _AreGUIServersAlive() && _InitGUI() == B_OK) {
		// normal app -- tell the user
		_NotifyAppServer(fTeam);
		_NotifyRegistrar(fTeam, true, false);

		nnWindow *alert = new nnWindow(fTeamInfo.args);

		// TODO: It would be nice if the alert would go away automatically
		// if someone else kills our teams.
		debugAction = alert->Go();
		if (debugAction < 0) {
			// Happens when closed by escape key
			debugAction = kActionKillTeam;
		}
		_NotifyRegistrar(fTeam, false, debugAction != kActionKillTeam);
	}

	return debugAction;
}


void
NNHandler::_LookupSymbolAddress(
	debug_symbol_lookup_context *lookupContext, const void *address,
	char *buffer, int32 bufferSize)
{
	// lookup the symbol
	void *baseAddress;
	char symbolName[1024];
	char imageName[B_PATH_NAME_LENGTH];
	bool exactMatch;
	bool lookupSucceeded = false;
	if (lookupContext) {
		status_t error = debug_lookup_symbol_address(lookupContext, address,
			&baseAddress, symbolName, sizeof(symbolName), imageName,
			sizeof(imageName), &exactMatch);
		lookupSucceeded = (error == B_OK);
	}

	if (lookupSucceeded) {
		// we were able to look something up
		if (strlen(symbolName) > 0) {
			// we even got a symbol
			snprintf(buffer, bufferSize, "%s + %#lx%s", symbolName,
				(addr_t)address - (addr_t)baseAddress,
				(exactMatch ? "" : " (closest symbol)"));

		} else {
			// no symbol: image relative address
			snprintf(buffer, bufferSize, "(%s + %#lx)", imageName,
				(addr_t)address - (addr_t)baseAddress);
		}

	} else {
		// lookup failed: find area containing the IP
		bool useAreaInfo = false;
		area_info info;
		ssize_t cookie = 0;
		while (get_next_area_info(fTeam, &cookie, &info) == B_OK) {
			if ((addr_t)info.address <= (addr_t)address
				&& (addr_t)info.address + info.size > (addr_t)address) {
				useAreaInfo = true;
				break;
			}
		}

		if (useAreaInfo) {
			snprintf(buffer, bufferSize, "(%s + %#lx)", info.name,
				(addr_t)address - (addr_t)info.address);
		} else if (bufferSize > 0)
			buffer[0] = '\0';
	}
}


void
NNHandler::_PrintStackTrace(thread_id thread)
{debug_printf("NNHandler::_PrintStackTrace\n");
	// print a stacktrace
	void *ip = NULL;
	void *stackFrameAddress = NULL;
	status_t error = debug_get_instruction_pointer(&fDebugContext, thread, &ip,
		&stackFrameAddress);

	if (error == B_OK) {
		// create a symbol lookup context
		debug_symbol_lookup_context *lookupContext = NULL;
		error = debug_create_symbol_lookup_context(fTeam, -1, &lookupContext);
		if (error != B_OK) {
			debug_printf("debug_server: Failed to create symbol lookup "
				"context: %s\n", strerror(error));
		}

		// lookup the IP
		char symbolBuffer[2048];
		_LookupSymbolAddress(lookupContext, ip, symbolBuffer,
			sizeof(symbolBuffer) - 1);

		debug_printf("stack trace, current PC %p  %s:\n", ip, symbolBuffer);

		for (int32 i = 0; i < 50; i++) {
			debug_stack_frame_info stackFrameInfo;

			error = debug_get_stack_frame(&fDebugContext, stackFrameAddress,
				&stackFrameInfo);
			if (error < B_OK || stackFrameInfo.parent_frame == NULL)
				break;

			// lookup the return address
			_LookupSymbolAddress(lookupContext, stackFrameInfo.return_address,
				symbolBuffer, sizeof(symbolBuffer) - 1);

			debug_printf("  (%p)  %p  %s\n", stackFrameInfo.frame,
				stackFrameInfo.return_address, symbolBuffer);

			stackFrameAddress = stackFrameInfo.parent_frame;
		}

		// delete the symbol lookup context
		if (lookupContext)
			debug_delete_symbol_lookup_context(lookupContext);
	}
}


void
NNHandler::_NotifyAppServer(team_id team)
{
	// This will remove any kWindowScreenFeels of the application, so that
	// the debugger alert is visible on screen
	BRoster::Private roster;
	roster.ApplicationCrashed(team);
}


void
NNHandler::_NotifyRegistrar(team_id team, bool openAlert,
	bool stopShutdown)
{
	BMessage notify(BPrivate::B_REG_TEAM_DEBUGGER_ALERT);
	notify.AddInt32("team", team);
	notify.AddBool("open", openAlert);
	notify.AddBool("stop shutdown", stopShutdown);

	BRoster::Private roster;
	BMessage reply;
	roster.SendTo(&notify, &reply, false);
}


status_t
NNHandler::_InitGUI()
{debug_printf("NNHandler::_InitGUI\n");
	NN *app = dynamic_cast<NN*>(be_app);
	BAutolock _(app);
	return app->InitGUIContext();
}


status_t
NNHandler::_HandlerThreadEntry(void *data)
{
	return ((NNHandler*)data)->_HandlerThread();
}


status_t
NNHandler::_HandlerThread()
{debug_printf("NNHandler::_HandleThread\n");
	TRACE(("debug_server: NNHandler::_HandlerThread(): team %" B_PRId32
		"\n", fTeam));

	// get initial message
	TRACE(("debug_server: NNHandler::_HandlerThread(): team %" B_PRId32
		": getting message...\n", fTeam));

	DebugMessage *message;
	status_t error = _PopMessage(message);
	int32 debugAction = kActionKillTeam;
	if (error == B_OK) {
		// handle the message
		debugAction = _HandleMessage(message);
		delete message;
	} else {
		debug_printf("NNHandler::_HandlerThread(): Failed to pop "
			"initial message: %s", strerror(error));
	}

	// kill the team or hand it over to the debugger
	thread_id debuggerThread = -1;
	if (debugAction == kActionKillTeam) {
		// The team shall be killed. Since that is also the handling in case
		// an error occurs while handing over the team to the debugger, we do
		// nothing here.
	} else if (debugAction == kActionWriteCoreFile) {
		_WriteCoreFile();
		debugAction = kActionKillTeam;
	} else if ((debuggerThread = _EnterDebugger(
			debugAction == kActionSaveReportTeam)) >= 0) {
		// wait for the "handed over" or a "team deleted" message
		bool terminate = false;
		do {
			error = _PopMessage(message);
			if (error != B_OK) {
				debug_printf("NNHandler::_HandlerThread(): Failed to "
					"pop message: %s", strerror(error));
				debugAction = kActionKillTeam;
				break;
			}

			if (message->Code() == B_DEBUGGER_MESSAGE_HANDED_OVER) {
				// The team has successfully been handed over to the debugger.
				// Nothing to do.
				terminate = true;
			} else if (message->Code() == B_DEBUGGER_MESSAGE_TEAM_DELETED) {
				// The team died. Nothing to do.
				terminate = true;
			} else {
				// Some message we can ignore. The debugger will take care of
				// it.

				// check whether the debugger thread still lives
				thread_info threadInfo;
				if (get_thread_info(debuggerThread, &threadInfo) != B_OK) {
					// the debugger is gone
					debug_printf("debug_server: The debugger for team %"
						B_PRId32 " seems to be gone.", fTeam);

					debugAction = kActionKillTeam;
					terminate = true;
				}
			}

			delete message;
		} while (!terminate);
	} else
		debugAction = kActionKillTeam;

	if (debugAction == kActionKillTeam) {
		// kill the team
		_KillTeam();
	}

	// remove this handler from the roster and delete it
	NNHandlerRoster::Default()->RemoveHandler(fTeam);

	delete this;

	return B_OK;
}


bool
NNHandler::_ExecutableNameEquals(const char *name) const
{
	return strcmp(_LastPathComponent(fExecutablePath), name) == 0;
}


bool
NNHandler::_IsAppServer() const
{
	return _ExecutableNameEquals("app_server");
}


bool
NNHandler::_IsInputServer() const
{
	return _ExecutableNameEquals("input_server");
}


bool
NNHandler::_IsRegistrar() const
{
	return _ExecutableNameEquals("registrar");
}


bool
NNHandler::_IsGUIServer() const
{
	// app or input server
	return _IsAppServer() || _IsInputServer() || _IsRegistrar();
}


const char *
NNHandler::_LastPathComponent(const char *path)
{
	const char *lastSlash = strrchr(path, '/');
	return lastSlash ? lastSlash + 1 : path;
}


team_id
NNHandler::_FindTeam(const char *name)
{
	// Iterate through all teams and check their executable name.
	int32 cookie = 0;
	team_info teamInfo;
	while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
		entry_ref ref;
		if (BPrivate::get_app_ref(teamInfo.team, &ref) == B_OK) {
			if (strcmp(ref.name, name) == 0)
				return teamInfo.team;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


bool
NNHandler::_AreGUIServersAlive()
{
	return _FindTeam("app_server") >= 0 && _FindTeam("input_server") >= 0
		&& _FindTeam("registrar");
}


// #pragma mark -


NN::NN(status_t &error)
	:
	BServer(kSignature, false, &error),
	fListenerPort(-1),
	fListener(-1),
	fTerminating(false)
{
}


typedef int (*Func)(void);
//testing wren
/*static void writeFn(WrenVM* vm, const char* text)
{
  debug_printf("%s", text);
}
void errorFn(WrenVM* vm, WrenErrorType errorType,
             const char* module, const int line,
             const char* msg)
{
  switch (errorType)
  {
    case WREN_ERROR_COMPILE:
    {
      debug_printf("[%s line %d] [Error] %s\n", module, line, msg);
    } break;
    case WREN_ERROR_STACK_TRACE:
    {
      debug_printf("[%s line %d] in %s\n", module, line, msg);
    } break;
    case WREN_ERROR_RUNTIME:
    {
      debug_printf("[Runtime Error] %s\n", msg);
    } break;
  }
}

*/
//done
extern "C" int max(int ,int);

/*
thread_id
PipeCommand(int argc, const char** argv, int& in, int& out,
	int& err, const char** envp = (const char**)environ)
{
	// This function written by Peter Folk <pfolk@uni.uiuc.edu>
	// and published in the BeDevTalk FAQ
	// http://www.abisoft.com/faq/BeDevTalk_FAQ.html#FAQ-209

	// Save current FDs
	int old_out = dup(1);
	int old_err = dup(2);

	int filedes[2];
	
	// create new pipe FDs as stdout, stderr
	pipe(filedes);  dup2(filedes[1], 1); close(filedes[1]);
	out = filedes[0]; // Read from out, taken from cmd's stdout
	pipe(filedes);  dup2(filedes[1], 2); close(filedes[1]);
	err = filedes[0]; // Read from err, taken from cmd's stderr

	// taken from pty.cpp
	// create a tty for stdin, as utilities don't generally use stdin
	int master = posix_openpt(O_RDWR);
	if (master < 0)
		return -1;

	int slave;
	const char* ttyName;
	if (grantpt(master) != 0 || unlockpt(master) != 0
		|| (ttyName = ptsname(master)) == NULL
		|| (slave = open(ttyName, O_RDWR | O_NOCTTY)) < 0) {
		close(master);
		return -1;
	}

	int pid = fork();
	if (pid < 0) {
		close(master);
		close(slave);
		return -1;
	}

	// child
	if (pid == 0) {
		close(master);

		setsid();
		if (ioctl(slave, TIOCSCTTY, NULL) != 0)
			return -1;

		dup2(slave, 0); 
		close(slave);

		// "load" command.
		execv(argv[0], (char *const *)argv);

		// shouldn't return
		return -1;
	}

	// parent
	close (slave);
	in = master;

	// Restore old FDs
	close(1); dup(old_out); close(old_out);
	close(2); dup(old_err); close(old_err);

	// Theoretically I should do loads of error checking, but
	// the calls aren't very likely to fail, and that would
	// muddy up the example quite a bit. YMMV.

	return pid;
}*/
/*
thread_id
PipeCommand(int argc, const char** argv, int& in, int& out, int& err,
	//const char** envp const char** envp = (const char**)environ)
{
	// This function written by Peter Folk <pfolk@uni.uiuc.edu>
	// and published in the BeDevTalk FAQ
	// http://www.abisoft.com/faq/BeDevTalk_FAQ.html#FAQ-209

	if (!envp)
		envp = (const char**)environ;

	// Save current FDs
	int old_in  =  dup(0);
	int old_out  =  dup(1);
	int old_err  =  dup(2);

	int filedes[2];

	// Create new pipe FDs as stdin, stdout, stderr
	pipe(filedes);  dup2(filedes[0], 0); close(filedes[0]);
	in = filedes[1];  // Write to in, appears on cmd's stdin
	pipe(filedes);  dup2(filedes[1], 1); close(filedes[1]);
	out = filedes[0]; // Read from out, taken from cmd's stdout
	pipe(filedes);  dup2(filedes[1], 2); close(filedes[1]);
	err = filedes[0]; // Read from err, taken from cmd's stderr

	// "load" command.
	thread_id ret  =  load_image(argc, argv, envp);
	if (ret < B_OK)
		goto cleanup;

	// thread ret is now suspended.

	setpgid(ret, ret);

cleanup:
	// Restore old FDs
	close(0); dup(old_in); close(old_in);
	close(1); dup(old_out); close(old_out);
	close(2); dup(old_err); close(old_err);

	//Theoretically I should do loads of error checking, but
	//   the calls aren't very likely to fail, and that would 
	//   muddy up the example quite a bit.  YMMV. 

	return ret;
}*/
// Option flags for SourceType::CreateSourceFile
enum
{
	SOURCEFILE_PAIR = 0x00000001	// Create a source file and a partner file, if appropriate
};
entry_ref
MakeProjectFile (DPath folder, const char *name, const char *data = NULL, const char *type =NULL)
{
	debug_printf("MakeProjectFile started\n");
	entry_ref ref;
	
	DPath path(folder);
	path.Append(name);
	debug_printf("MakeProjectFile %s\n",path.GetFullPath());

	BEntry entry(path.GetFullPath());
	
	if (entry.Exists())
	{
	debug_printf("MakeProjectFile file exists\n");
		//BString errstr = B_TRANSLATE("%filepath% already exists. Do you want to overwrite it?");
		//errstr.ReplaceFirst("%filepath%", path.GetFullPath());
		//int32 result = ShowAlert(errstr.String(),B_TRANSLATE("Overwrite"),B_TRANSLATE("Cancel"));
		//if (result == 1)
		//	return ref;
	}
	
	//BFile file(path.GetFullPath(),B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	//BFile file("boot/home/Projects/any.cpp", B_READ_WRITE);


	/*BPath path2;
	if (find_directory(JS_MAK_DIRECTORY, &path2) == B_OK) {
		
	}
	
	path2.Append("any.cpp");
debug_printf("MakeProjectFile path2 = %s \n",path2.Path());*/
	debug_printf("MakeProjectFile path.path() = %s\n",path.GetFullPath());
	BFile file(path.GetFullPath(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		//BFile fil2e(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		
	if (data && strlen(data) > 0)
		file.Write(data,strlen(data));
	
	BString fileType = (type && strlen(type) > 0) ? type : "text/x-source-code";
	file.WriteAttr("BEOS:TYPE",B_STRING_TYPE, 0, fileType.String(),
						fileType.Length() + 1);
	
	file.Unset();
	entry.GetRef(&ref);
	return ref;
}


BString
MakeHeaderGuard(const char *name)
{
	BString define(name);
	define.ReplaceSet(" .-","_");
	
	// Normally, we'd just put something like :
	// define[i] = toupper(define[i]);
	// in a loop, but the BString defines for Zeta are screwed up, so we're going to have to
	// work around them.
	char *buffer = define.LockBuffer(define.CountChars() + 1);
	for (int32 i = 0; i < define.CountChars(); i++)
		buffer[i] = toupper(buffer[i]);
	define.UnlockBuffer();
	
	BString guard;
	guard << "#ifndef " << define << "\n"
		<< "#define " << define << "\n"
			"\n"
			"\n"
			"\n"
			"#endif\n";
	return guard;
}


entry_ref
CreateSourceFile(const char *dir, const char *name, uint32 options, BString data2)
{
	if (!dir || !name)
		return entry_ref();
	
	BString folderstr(dir);
	if (folderstr.ByteAt(folderstr.CountChars() - 1) != '/')
		folderstr << "/";
	
	DPath folder(folderstr.String()), filename(name);
	
	bool is_cpp = false;
	bool is_header = false;
	bool create_pair = ((options & SOURCEFILE_PAIR) != 0);
	
	BString ext = filename.GetExtension();
	if ( (ext.ICompare("cpp") == 0) || (ext.ICompare("c") == 0) ||
		(ext.ICompare("cxx") == 0) || (ext.ICompare("cc") == 0) )
		is_cpp = true;
	else if ((ext.ICompare("h") == 0) || (ext.ICompare("hxx") == 0) ||
			(ext.ICompare("hpp") == 0) || (ext.ICompare("h++") == 0))
		is_header = true;
	
	if (!is_cpp && !is_header)
		return entry_ref();
	
	BString sourceName, headerName;
	if (is_cpp)
	{
		sourceName = name;//"newie.cpp";//filename.GetFileName();
		headerName = filename.GetBaseName();//"new";//filename.GetBaseName();
		headerName << ".h";
	}
	else
	{
		sourceName = "new";//filename.GetBaseName();
		sourceName << ".cpp";
		headerName = "newiee.cpp";//filename.GetFileName();
	}
		
	
	entry_ref sourceRef, headerRef;
	BString data;
	if (is_cpp || create_pair)
	{
		if (create_pair)
			data << "#include \"" << headerName << "\"\n\n";
			
		data.Append(data2);
		
		sourceRef = MakeProjectFile(folder.GetFullPath(),sourceName.String(),data.String());
	}
	
	if (is_header || create_pair)
	{
		data = MakeHeaderGuard(headerName.String());
		headerRef = MakeProjectFile(folder.GetFullPath(),headerName.String(),data.String());
	}
	
	return is_cpp ? sourceRef : headerRef;
}

static const uint32 kMsgCompile = 'DRCT';
status_t
NN::Init()
{
	debug_printf("NN::Init\n");

	debug_printf("Making the cpp file {started}...\n");
/*
entry_ref reff = CreateSourceFile("Haiku/system/","any.cpp", true);
entry_ref reff2 = CreateSourceFile("/","any2.cpp", true);
*/

	BString data;
	data << "#include <cstdio> \n"
	"int main(){ printf(\"output\"); return 0;} ";
	entry_ref reff3 = CreateSourceFile("/boot/home/","any2.cpp", true, data);



///BFile file("/boot/home/new.txt", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
//BFile file2("/boot/home/new2.txt", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
//const char* unknown = "mak-jaat";


		
//sourceRef = MakeProjectFile(folder.GetFullPath(),sourceName.String(),data.String());

//char unknown[10]={'m','a','k'};

///file.Write(data.String(), strlen(data));
//file.Write(unknown, sizeof(unknown));

	/*BPath pathii;
	if (find_directory(JS_MAK_DIRECTORY, &pathii) == B_OK) {
		pathii.Append("seeting.cpp");

		BFile file3(pathii.Path(), B_CREATE_FILE | B_READ_WRITE);
		//if (file.InitCheck() == B_OK)
		//	file.Read(&offset, sizeof(BPoint));
	}*/
	debug_printf("before launching the terminal...\n");


	BPath path;
	if(find_directory(B_SYSTEM_APPS_DIRECTORY, &path) != B_OK || path.Append("Terminal") != B_OK)
	{
	debug_printf("not foundd\n");
		path.SetTo("/boot/system/apps/Terminal");
	}
	

	BEntry entry(path.Path());
	entry_ref ref;
	
	///BString cmd;
	
	///cmd = "cd /boot/home; gcc any2.cpp";
	/*const char* arguments[] = {cmd.String(),"any2.cpp"};
	entry.GetRef( &ref);
	be_roster->Launch(&ref,2,arguments);
	*/
	
	//system(cmd);
	
	
	/*entry_ref pwd;
	int					fStdIn;
	int					fStdOut;
	int fStdErr;
	//BString cmd;
	int32 argc = 3;
	const char** argv = new const char * [argc + 1];

	argv[0] = strdup("/bin/sh");
	argv[1] = strdup("-c");
	argv[2] = strdup(cmd.String());
	argv[argc] = NULL;
	thread_id fThreadId;
	fThreadId = PipeCommand(argc, argv, fStdIn, fStdOut, fStdErr);
	set_thread_priority(fThreadId, B_LOW_PRIORITY);

	resume_thread(fThreadId);*/
	
	BPath targetPath;

	if (entry.GetPath(&targetPath) != B_OK)
	{debug_printf("error in terminal launching...");
	}

	// Launch the Terminal.
	const char* kTerminalSignature = "application/x-vnd.Haiku-Terminal";
	const char* argv[] = {"-w", "/boot/home", "-t", "AI", "/bin/sh", "-c", "gcc any2.cpp", NULL};
	be_roster->Launch(kTerminalSignature, 7, argv);
	
	// Launch the command without Terminal
/*	BString cd;
	//BString thePath;
	BString commandStr="gcc any2.cpp";
	cd.SetToFormat("cd '%s' &&","/boot/home");
	commandStr.Prepend(cd);
	
	BPrivate::BCommandPipe pipe;
	pipe.AddArg("/bin/sh");
	pipe.AddArg("-c");
	pipe.AddArg(commandStr);
	debug_printf("NNServer :: %s\n", commandStr.String());
	pipe.RunAsync();
*/	
	// Launch the Command Timer
/*	BMessage commandMsg('DRCT');//(kMsgCompile);
	const char* kCommandTimerSignature = "application/x-vnd.jas.CommandTimer";
	be_roster->Launch(kCommandTimerSignature, &commandMsg);
*/	
	//const char* argv[] = {"-w", "/boot/home", "-t", "AI", NULL};
	//const char* argv[] = {"-w", targetPath.Path(), "gcc", "any2.cpp", NULL};
	
	/*BPath rasta;
	BEntry entr(rasta.Path());
	entry_ref reference;
	BString cmdi;
	cmdi="gcc";
	const char* argvv[] = {cmdi.String()};
	if(entr.GetRef(&reference) != B_OK || be_roster->Launch(&reference,1,argvv) != B_OK)
	{
	debug_printf("unable to launch");
	}
	*/
	//static const uint32 FULLSCREEN							= 'fscr';
	
	///BMessage msg(FULLSCREEN);
	//msg->AddMessenger("fscr");

//	be_roster->Launch(kTerminalSignature, 4, argv);
	
	//BString command = "gcc input.cpp";
	//system(command);
	
	
	
	
	//be_roster->Launch(kTerminalSignature, &msg);
//	be_roster->Launch("application/x-vnd.Haiku-About");
	/*
	cmd="gcc --help";
	const char* argv[]={"gcc","--help"};//{"gcc --help"};//{cmd.String()};
	if(entry.GetRef(&ref) != B_OK || be_roster->Launch(&ref,2,argv) != B_OK)
	{
		debug_printf("unable to launch the terminal...\n");
	}*/

debug_printf("after launching the terminal...\n");



	
	// create listener port
	fListenerPort = create_port(10, "kernel listener");
	if (fListenerPort < 0)
		return fListenerPort;

	// spawn the listener thread
	fListener = spawn_thread(_ListenerEntry, "kernel listener",
		B_NORMAL_PRIORITY, this);
	if (fListener < 0)
		return fListener;

	// register as default debugger
	// TODO: could set default flags
	status_t error = install_default_debugger(fListenerPort);
	if (error != B_OK)
		return error;

	// resume the listener
	resume_thread(fListener);

	return B_OK;
}


bool
NN::QuitRequested()
{
	// Never give up, never surrender. ;-)
	return false;
}


status_t
NN::_ListenerEntry(void *data)
{
	return ((NN*)data)->_Listener();
}


status_t
NN::_Listener()
{
	while (!fTerminating) {
		// receive the next debug message
		DebugMessage *message = new DebugMessage;
		int32 code;
		ssize_t bytesRead;
		do {
			bytesRead = read_port(fListenerPort, &code, &message->Data(),
				sizeof(debug_debugger_message_data));
		} while (bytesRead == B_INTERRUPTED);

		if (bytesRead < 0) {
			debug_printf("debug_server: Failed to read from listener port: "
				"%s. Terminating!\n", strerror(bytesRead));
			exit(1);
		}
TRACE(("debug_server: Got debug message: team: %" B_PRId32 ", code: %" B_PRId32
	"\n", message->Data().origin.team, code));

		message->SetCode((debug_debugger_message)code);

		// dispatch the message
		NNHandlerRoster::Default()->DispatchMessage(message);
	}

	return B_OK;
}


// #pragma mark -


int
main()
{
debug_printf("hello hi , i m nn , born right now...\n");
	status_t error;


debug_printf("hello hi , i m nn , adult...\n");
	BPath path;
	error = find_directory(JS_MAK_DIRECTORY, &path, true);
	if (error != B_OK)
		debug_printf("failed in creation MaK directory.\n");

//testing wren it works man
/*debug_printf("wren testing.....\n");
 WrenConfiguration config;
  wrenInitConfiguration(&config);
    config.writeFn = &writeFn;
    config.errorFn = &errorFn;
  WrenVM* vm = wrenNewVM(&config);

  const char* module = "main";
  const char* script = "System.print(\"I am running in a VM!\")";

  WrenInterpretResult result2 = wrenInterpret(vm, module, script);

  switch (result2) {
    case WREN_RESULT_COMPILE_ERROR:
      { printf("Compile Error!\n"); } break;
    case WREN_RESULT_RUNTIME_ERROR:
      { printf("Runtime Error!\n"); } break;
    case WREN_RESULT_SUCCESS:
      { printf("Success!\n"); } break;
  }

  wrenFreeVM(vm);
  debug_printf("wren testing done.....\n");
*/
//debug_printf("before main2...\n");
//int ret=main2();
//debug_printf("ret =%d\n",ret);
//debug_printf("after main2...\n");


	// for the time being let the debug server print to the syslog
	int console = open("/dev/dprintf", O_RDONLY);
	if (console < 0) {
		debug_printf("debug_server: Failed to open console: %s\n",
			strerror(errno));
	}
	dup2(console, STDOUT_FILENO);
	dup2(console, STDERR_FILENO);
	close(console);

	// create the team debug handler roster
	if (!NNHandlerRoster::CreateDefault()) {
		debug_printf("debug_server: Failed to create team debug handler "
			"roster.\n");
		exit(1);
	}

	// create application
	NN server(error);
	if (error != B_OK) {
		debug_printf("debug_server: Failed to create BApplication: %s\n",
			strerror(error));
		exit(1);
	}

	// init application
	error = server.Init();
	if (error != B_OK) {
		debug_printf("debug_server: Failed to init application: %s\n",
			strerror(error));
		exit(1);
	}

	server.Run();

	return 0;
}
