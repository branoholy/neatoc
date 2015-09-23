/*
 * NeatoC
 * Copyright (C) 2015  Branislav Holý <branoholy@gmail.com>
 *
 * This file is part of NeatoC.
 *
 * NeatoC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NeatoC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NeatoC.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <iostream>

#include <neatoc/controller.hpp>

#include "neatocscanapp.hpp"

void printHelp()
{
	std::cout << "Usage: neatoc-scan-gui [options]" << std::endl
			  << "Options:" << std::endl
			  << "  -e <endpoint>             The IP address and port that is used to connect to" << std::endl
			  << "                            the Neato robot (default: 10.0.0.1:12345)." << std::endl
			  << "                            Use string \"log\" to load a log file." << std::endl
			  << "  -l <file>                 The path to the log file." << std::endl
			  << "  -m                        Turn on manual scanning (by pressing key S)." << std::endl
			  << "  -h                        Show this help." << std::endl;
}

int main(int argc, char** argv)
{
	std::cout.setf(std::ios_base::boolalpha);

	std::string logPath;
	std::string endpoint = "10.0.0.1:12345";
	bool manualScanning = false;
	for(int i = 1; i < argc; i++)
	{
		std::string arg(argv[i]);
		if(arg == "-e") endpoint = std::string(argv[++i]);
		else if(arg == "-l") logPath = std::string(argv[++i]);
		else if(arg == "-m") manualScanning = true;
		else
		{
			printHelp();
			return 0;
		}
	}
	bool useScanner = (endpoint != "log");

	neatoc::Controller controller(logPath);
	std::cout << "Hello NeatoC!" << std::endl;

	std::cout << "Connecting to " << endpoint << std::endl;
	if(useScanner)
	{
		controller.connect(endpoint);

		controller.setTestMode(true);
		std::cout << "Test mode: " << controller.getTestMode() << std::endl;

		controller.setLdsRotation(true);
		std::cout << "LDS rotation: " << controller.getLdsRotation() << std::endl;
	}

	NeatocScanApp *app = new NeatocScanApp(controller, useScanner, manualScanning);
	NeatocScanApp::Display(app, argc, argv);

	if(useScanner)
	{
		controller.setLdsRotation(false);
		std::cout << "LDS rotation: " << controller.getLdsRotation() << std::endl;

		controller.setTestMode(false);
		std::cout << "Test mode: " << controller.getTestMode() << std::endl;
	}

	return 0;
}
