/** @file winservice.cc
 * @brief Windows service handling.
 */
/* Copyright (c) 2012 Richard Boulton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifdef __WIN32__

#include <config.h>
#define UNICODE 1

#include "cli/cli.h"
#include "httpserver/httpserver.h"
#include "logger/logger.h"
#include "rest/router.h"
#include "rest/routes.h"
#include "server/server.h"
#include "server/task_manager.h"
#include "utils/winservice.h"
#include <windows.h>
#include <xapian.h> // For UTF8 stuff.

using namespace RestPose;

// Methods passed to windows service controller API.
static void WINAPI run_service(DWORD argc, LPTSTR *argv);
static void WINAPI service_ctrl(DWORD ctrlCode);

// Globals, used to manage service state.
static std::wstring g_service_winname;
static SERVICE_STATUS_HANDLE g_status_handle;
static Server * g_server;
static const CliOptions * g_options;

static std::string
to_utf8_string(LPTSTR str)
{
    std::string result;
    for (LPTSTR pos = str; *pos != 0; ++pos) {
	Xapian::Unicode::append_utf8(result, *pos);
    }
    return result;
}

static std::string
get_win_err(DWORD errnum)
{
    LPTSTR err_msg;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		  NULL, errnum, 0, (LPTSTR)&err_msg, 0, NULL);
    std::string err_msg_utf8 = to_utf8_string(err_msg);
    LocalFree(err_msg);
    return err_msg_utf8;
}

static bool
install_service(const CliOptions & options)
{
    SC_HANDLE sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (sc_manager == NULL) {
	DWORD err = GetLastError();

	LOG_ERROR("Unable to connect to Service Control Manager: " +
		  get_win_err(err));
	return false;
    }

    std::wstring service_winname(options.service_name.begin(),
				 options.service_name.end());
 
    // Check for existing service with same name
    SC_HANDLE service_handle = OpenService(sc_manager, service_winname.c_str(),
					   SERVICE_ALL_ACCESS);
    if (service_handle != NULL) {
	LOG_ERROR("Found existing service named \"" + options.service_name +
		  "\", can't install");
	CloseServiceHandle(service_handle);
	CloseServiceHandle(sc_manager);
	return false;
    }

    // Calculate command line to use for service.
    char exe_path[1024];
    GetModuleFileNameA(NULL, exe_path, sizeof exe_path);
    std::string cli_buf("\"");
    cli_buf += exe_path;
    cli_buf += "\" ";
    cli_buf += options.service_command_opts();
    std::wstring command_line(cli_buf.begin(), cli_buf.end()); // FIXME - convert from UTF8

    // Create service.
    service_handle = CreateService(
				   sc_manager,
				   service_winname.c_str(),
				   service_winname.c_str(),
				   SERVICE_ALL_ACCESS,
				   SERVICE_WIN32_OWN_PROCESS,
				   SERVICE_AUTO_START,
				   SERVICE_ERROR_NORMAL,
				   command_line.c_str(),
				   NULL,
				   NULL,
				   L"\0\0",
				   NULL,
				   NULL);
    if (service_handle == NULL) {
	DWORD err = GetLastError();
	LOG_ERROR("Unable to create service: " + get_win_err(err));
	return false;
    }
    LOG_INFO("Service " + options.service_name +
	     " installed (with command line: " +
	     to_utf8_string(const_cast<LPTSTR>(command_line.c_str())) + ")");

    // Set a description for the service.
    SERVICE_DESCRIPTION service_description;
    service_description.lpDescription =
	    const_cast<LPTSTR>(service_winname.c_str());
    bool ok = ChangeServiceConfig2(service_handle, SERVICE_CONFIG_DESCRIPTION,
				   &service_description);
    if (!ok) {
	LOG_ERROR("Could not set service description.");
	CloseServiceHandle(service_handle);
	CloseServiceHandle(sc_manager);
	return false;
    }

    // Set the service to auto-restart.
    {
	SC_ACTION actions[3] = {
	    { SC_ACTION_RESTART, 0 },
	    { SC_ACTION_RESTART, 0 },
	    { SC_ACTION_RESTART, 0 }
	};

	SERVICE_FAILURE_ACTIONS failure;
	ZeroMemory(&failure, sizeof(SERVICE_FAILURE_ACTIONS));
	failure.cActions = 3;
	failure.lpsaActions = actions;

	if (!ChangeServiceConfig2(service_handle,
				  SERVICE_CONFIG_FAILURE_ACTIONS, &failure)) {
	    CloseServiceHandle(service_handle);
	    CloseServiceHandle(sc_manager);
	    return false;
	}
    }

    CloseServiceHandle(service_handle);
    CloseServiceHandle(sc_manager);
    return true;
}

static bool
remove_service(const CliOptions & options)
{
    SC_HANDLE sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (sc_manager == NULL) {
	DWORD err = GetLastError();
	LOG_ERROR("Unable to connect to Service Control Manager: " +
		  get_win_err(err));
	return false;
    }

    std::wstring service_winname(options.service_name.begin(),
				 options.service_name.end());

    SC_HANDLE service_handle = OpenService(sc_manager, service_winname.c_str(),
					   SERVICE_ALL_ACCESS);
    if (service_handle == NULL) {
	LOG_ERROR("Couldn't find service named \"" + options.service_name +
		  "\", can't remove");
	CloseServiceHandle(sc_manager);
	return false;
    }

    SERVICE_STATUS status;
    // stop service if its running
    if (ControlService(service_handle, SERVICE_CONTROL_STOP, &status)) {
	LOG_INFO("Stopping service " + options.service_name);

	// FIXME - can we wait for this, rather than polling?
	while (QueryServiceStatus(service_handle, &status)) {
	    if (status.dwCurrentState == SERVICE_STOP_PENDING) {
		Sleep(1000);
	    } else {
		break;
	    }
	}
	LOG_INFO("Service " + options.service_name + " stopped");
    }

    bool removed = DeleteService(service_handle);
    CloseServiceHandle(service_handle);
    CloseServiceHandle(sc_manager);

    if (!removed) {
	LOG_ERROR("Service " + options.service_name + " removed");
	return false;
    }

    LOG_INFO("Service " + options.service_name + " removed");
    return true;
}

static void
start_service(const CliOptions & options,
	      Server & server)
{
    g_service_winname = std::wstring(options.service_name.begin(),
				     options.service_name.end());
    g_status_handle = 0;
    g_server = &server;
    g_options = &options;

    SERVICE_TABLE_ENTRY dispatch_table[] = {
	{
	    const_cast<LPTSTR>(g_service_winname.c_str()),
	    (LPSERVICE_MAIN_FUNCTION)run_service
	},
	{ NULL, NULL }
    };

    LOG_INFO("Trying to start Windows service '" + options.service_name + "'");
    StartServiceCtrlDispatcher(dispatch_table);
}

static bool
report_status(DWORD current_state, DWORD wait_millis = 0) {
    if (g_status_handle == 0)
	return false;

    static DWORD next_check_point = 1;

    SERVICE_STATUS status;

    DWORD controls_accepted;
    DWORD check_point;

    if (current_state == SERVICE_START_PENDING ||
	current_state == SERVICE_STOP_PENDING ||
	current_state == SERVICE_STOPPED) {
	controls_accepted = 0;
    } else {
	controls_accepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    }

    if (current_state == SERVICE_RUNNING ||
	current_state == SERVICE_STOPPED) {
	check_point = 0;
    } else {
	check_point = next_check_point++;
    }

    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwServiceSpecificExitCode = 0;
    status.dwControlsAccepted = controls_accepted;
    status.dwCurrentState = current_state;
    status.dwWin32ExitCode = NO_ERROR;
    status.dwWaitHint = wait_millis;
    status.dwCheckPoint = check_point;

    return SetServiceStatus(g_status_handle, &status);
}

/** Function called by the windows service manager to run the service.
 */
static void WINAPI
run_service(DWORD, LPTSTR *)
{
    g_status_handle = RegisterServiceCtrlHandler(g_service_winname.c_str(),
						 service_ctrl);
    if (!g_status_handle)
	return;

    report_status(SERVICE_START_PENDING, 1000);

    CollectionPool pool(g_options->datadir);
    TaskManager * taskman = new TaskManager(pool);
    g_server->add("taskman", taskman);
    Router router(taskman, g_server);
    setup_routes(router);

    // Service setup and mainloop (could be moved to a separate function).
    g_server->add("httpserver",
		  new HTTPServer(g_options->port, g_options->pedantic, &router));
    report_status(SERVICE_RUNNING);
    g_server->run();

    // Service shutdown
    report_status(SERVICE_STOPPED);
}

static void WINAPI service_ctrl(DWORD code)
{
    switch (code) {
	case SERVICE_CONTROL_STOP:
	    // Service asked to stop.
	    report_status(SERVICE_STOP_PENDING);
	    g_server->shutdown();
	    break;
	case SERVICE_CONTROL_SHUTDOWN:
	    // System is shutting down
	    report_status(SERVICE_STOP_PENDING);
	    g_server->shutdown();
	    break;
    }
}

/** Perform a service task.
 *
 *  Returns true if a service task has been performed.
 */
bool
RestPose::winservice_handle(const CliOptions & options,
			    Server & server)
{
    switch (options.service_action) {
	case CliOptions::SRVACT_NONE:
	    return false;
	case CliOptions::SRVACT_INSTALL:
	    install_service(options);
	    return true;
	case CliOptions::SRVACT_REMOVE:
	    remove_service(options);
	    return true;
	case CliOptions::SRVACT_REINSTALL:
	    remove_service(options);
	    install_service(options);
	    return true;
	case CliOptions::SRVACT_RUN_SERVICE:
	    start_service(options, server);
	    return true;
    }
    return false;
}

#endif
