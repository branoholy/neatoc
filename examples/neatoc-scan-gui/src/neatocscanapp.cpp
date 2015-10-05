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

#include "neatocscanapp.hpp"

#include <chrono>

NeatocScanApp::NeatocScanApp(neatoc::Controller &controller, bool useScanner, bool manualScanning) : wxApp(),
	controller(controller), useScanner(useScanner), manualScanning(manualScanning)
{
}

bool NeatocScanApp::OnInit()
{
	// Frame
	frame = new wxFrame(NULL, wxID_ANY, "NeatoC Scan", wxDefaultPosition, wxSize(600, 400));

	// Frame StatusBar
	wxStatusBar *statusBar = frame->CreateStatusBar(2);

	std::string endpoint;
	if(useScanner) endpoint = controller.getEndpoint().address().to_string() + ':' + std::to_string(controller.getEndpoint().port());
	else endpoint = controller.getLogPath();

	frame->SetStatusText("", 0);
	frame->SetStatusText("Connected to " + endpoint, 1);

	int sbWidths[] = { -1, 300 };
	statusBar->SetStatusWidths(2, sbWidths);

	// Panel
	panel = new wxPanel(frame);
	panel->GetEventHandler()->Bind(wxEVT_KEY_DOWN, &NeatocScanApp::setMotorByKey, this);
	panel->GetEventHandler()->Bind(wxEVT_PAINT, &NeatocScanApp::repaint, this);

	if(!manualScanning)
	{
		scanThreadRunning = true;
		scanThread = std::thread([this]()
		{
			while(scanThreadRunning)
			{
				scanAndShow();

				if(scanThreadRunning)
				{
					std::unique_lock<std::mutex> lock(scanThreadCVMutex);
					scanThreadCV.wait_for(lock, std::chrono::milliseconds(500));
				}
			}
		});
	}

	frame->Show(true);

	return true;
}

int NeatocScanApp::OnExit()
{
	stopScanThread();
	if(scanThread.joinable()) scanThread.join();

	return wxApp::OnExit();
}

void NeatocScanApp::setMotorByKey(wxKeyEvent& keyEvent)
{
	switch(keyEvent.GetKeyCode())
	{
		case WXK_UP:
			controllerMutex.lock();
			frame->SetStatusText("Going up...", 0);

			if(keyEvent.ControlDown()) controller.setMotor(500, 500, 100);
			else controller.setMotor(100, 100, 50);

			controllerMutex.unlock();
			break;

		case WXK_DOWN:
			controllerMutex.lock();
			frame->SetStatusText("Going down...", 0);
			controller.setMotor(-100, -100, 50);
			controllerMutex.unlock();
			break;

		case WXK_LEFT:
			controllerMutex.lock();
			frame->SetStatusText("Turning left...", 0);
			if(keyEvent.ControlDown()) controller.setMotor(-30, 30, 50);
			else controller.setMotor(20, 100, 50);
			controllerMutex.unlock();
			break;

		case WXK_RIGHT:
			controllerMutex.lock();
			frame->SetStatusText("Turning right...", 0);
			if(keyEvent.ControlDown()) controller.setMotor(30, -30, 50);
			else controller.setMotor(100, 20, 50);
			controllerMutex.unlock();
			break;

		case 'S':
			if(manualScanning)
			{
				frame->SetStatusText("Manual scanning...", 0);
				scanAndShow();
			}

		default:
			keyEvent.Skip();
	}
}

void NeatocScanApp::repaint(wxPaintEvent&)
{
	wxPaintDC dc(panel);

	int width, height;
	panel->GetSize(&width, &height);

	int width2 = width / 2;
	int height2 = height / 2;

	dc.DrawLine(0, height2, width, height2);
	dc.DrawLine(width2, 0, width2, height);

	controllerMutex.lock();

	for(const neatoc::ScanRecord& record : data)
	{
		double distance = record.distance / 10;
		double x = width2 + distance * std::cos(record.angle);
		double y = height2 - distance * std::sin(record.angle);

		dc.DrawRectangle(x, y, 2, 2);
	}

	controllerMutex.unlock();
}

void NeatocScanApp::stopScanThread()
{
	if(scanThreadRunning)
	{
		scanThreadRunning = false;
		scanThreadCV.notify_one();
	}
}

void NeatocScanApp::scanAndShow()
{
	controllerMutex.lock();

	data = controller.getLdsScan(useScanner);
	bool emptyData = data.empty();
	if(emptyData) stopScanThread();

	controllerMutex.unlock();

	this->GetTopWindow()->GetEventHandler()->CallAfter([this, emptyData]()
	{
		if(emptyData) frame->SetStatusText("No more scans to show (EOF).", 0);
		else frame->Refresh();
	});
}

void NeatocScanApp::Display(wxApp *app, int& argc, char **argv)
{
	wxApp::SetInstance(app);
	wxEntryStart(argc, argv);
	app->CallOnInit();
	app->OnRun();
	app->OnExit();
	wxEntryCleanup();
}
