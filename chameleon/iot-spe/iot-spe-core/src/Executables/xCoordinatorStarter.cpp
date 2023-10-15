/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <Components/xCoordinator.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <GRPC/CoordinatorRPCServer.hpp>
#include <Util/Logger/Logger.hpp>
#include <Version/version.hpp>
#include <iostream>
#include <vector>

using namespace x;
using namespace std;

const string logo = "\n";

const string coordinator = "\n"
                           "▒█▀▀█ █▀▀█ █▀▀█ █▀▀█ █▀▀▄ ░▀░ █▀▀▄ █▀▀█ ▀▀█▀▀ █▀▀█ █▀▀█ \n"
                           "▒█░░░ █░░█ █░░█ █▄▄▀ █░░█ ▀█▀ █░░█ █▄▄█ ░░█░░ █░░█ █▄▄▀ \n"
                           "▒█▄▄█ ▀▀▀▀ ▀▀▀▀ ▀░▀▀ ▀▀▀░ ▀▀▀ ▀░░▀ ▀░░▀ ░░▀░░ ▀▀▀▀ ▀░▀▀";

int main(int argc, const char* argv[]) {
    try {
        std::cout << logo << std::endl;
        std::cout << coordinator << " v" << x_VERSION << std::endl;
        x::Logger::setupLogging("xCoordinatorStarter.log", x::LogLevel::LOG_DEBUG);
        CoordinatorConfigurationPtr coordinatorConfig = CoordinatorConfiguration::create(argc, argv);

        Logger::getInstance()->changeLogLevel(coordinatorConfig->logLevel.getValue());

        x_INFO("start coordinator with {}", coordinatorConfig->toString());

        x_INFO("creating coordinator");
        xCoordinatorPtr crd = std::make_shared<xCoordinator>(coordinatorConfig);

        x_INFO("Starting Coordinator");
        crd->startCoordinator(/**blocking**/ true);//This is a blocking call
        x_INFO("Stopping Coordinator");
        crd->stopCoordinator(true);
    } catch (std::exception& exp) {
        x_ERROR("Problem with coordinator: {}", exp.what());
        return 1;
    } catch (...) {
        x_ERROR("Unknown exception was thrown");
        try {
            std::rethrow_exception(std::current_exception());
        } catch (std::exception& ex) {
            x_ERROR("{}", ex.what());
        }
        return 1;
    }
}