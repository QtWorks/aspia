//
// Aspia Project
// Copyright (C) 2020 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "base/command_line.h"
#include "base/logging.h"
#include "base/files/base_paths.h"
#include "base/win/service_controller.h"
#include "proxy/win/service.h"
#include "proxy/win/service_constants.h"

#if defined(USE_TBB)
#include <tbb/tbbmalloc_proxy.h>
#endif // defined(USE_TBB)

#include <iostream>

namespace {

std::filesystem::path loggingDir()
{
    std::filesystem::path path;

    if (!base::BasePaths::commonAppData(&path))
        return std::filesystem::path();

    path.append("aspia/logs");
    return path;
}

void startService()
{
    base::win::ServiceController controller =
        base::win::ServiceController::open(proxy::kServiceName);
    if (!controller.isValid())
    {
        std::cout << "Failed to access the service. Not enough rights or service not installed."
                  << std::endl;
    }
    else
    {
        if (!controller.start())
        {
            std::cout << "Failed to start the service." << std::endl;
        }
        else
        {
            std::cout << "The service started successfully." << std::endl;
        }
    }
}

void stopService()
{
    base::win::ServiceController controller =
        base::win::ServiceController::open(proxy::kServiceName);
    if (!controller.isValid())
    {
        std::cout << "Failed to access the service. Not enough rights or service not installed."
                  << std::endl;
    }
    else
    {
        if (!controller.stop())
        {
            std::cout << "Failed to stop the service." << std::endl;
        }
        else
        {
            std::cout << "The service has stopped successfully." << std::endl;
        }
    }
}

void installService()
{
    std::filesystem::path file_path;

    if (!base::BasePaths::currentExecFile(&file_path))
    {
        std::cout << "Failed to get the path to the executable." << std::endl;
    }
    else
    {
        base::win::ServiceController controller = base::win::ServiceController::install(
            proxy::kServiceName, proxy::kServiceDisplayName, file_path);
        if (!controller.isValid())
        {
            std::cout << "Failed to install the service." << std::endl;
        }
        else
        {
            std::cout << "The service has been successfully installed." << std::endl;
        }
    }
}

void removeService()
{
    if (base::win::ServiceController::isRunning(proxy::kServiceName))
    {
        stopService();
    }

    if (!base::win::ServiceController::remove(proxy::kServiceName))
    {
        std::cout << "Failed to remove the service." << std::endl;
    }
    else
    {
        std::cout << "The service was successfully deleted." << std::endl;
    }
}

void showHelp()
{
    std::cout << "aspia_proxy [switch]" << std::endl
        << "Available switches:" << std::endl
        << '\t' << "--install" << '\t' << "Install service" << std::endl
        << '\t' << "--remove"  << '\t' << "Remove service"  << std::endl
        << '\t' << "--start"   << '\t' << "Start service"   << std::endl
        << '\t' << "--stop"    << '\t' << "Stop service"    << std::endl
        << '\t' << "--help"    << '\t' << "Show help"       << std::endl;
}

} // namespace

int main(int argc, char* argv[])
{
    base::LoggingSettings settings;
    settings.destination = base::LOG_TO_FILE;
    settings.log_dir = loggingDir();

    base::initLogging(settings);

    base::CommandLine command_line(argc, argv);

    if (command_line.hasSwitch(u"install"))
    {
        installService();
    }
    else if (command_line.hasSwitch(u"remove"))
    {
        removeService();
    }
    else if (command_line.hasSwitch(u"start"))
    {
        startService();
    }
    else if (command_line.hasSwitch(u"stop"))
    {
        stopService();
    }
    else if (command_line.hasSwitch(u"help"))
    {
        showHelp();
    }
    else
    {
        proxy::Service().exec();
    }

    base::shutdownLogging();
    return 0;
}
