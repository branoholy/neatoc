/*
 * Regilo
 * Copyright (C) 2015-2016  Branislav Holý <branoholy@gmail.com>
 *
 * This file is part of Regilo.
 *
 * Regilo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Regilo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Regilo.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "regilovisual.hpp"

#include <chrono>

#include <wx/dcgraph.h>

#include <regilo/neatocontroller.hpp>
#include <regilo/serialcontroller.hpp>
#include <regilo/socketcontroller.hpp>

RegiloVisual::RegiloVisual(regilo::Controller *controller, bool useScanner, bool manualScanning, bool moveScanning) : wxApp(),
	controller(controller), useScanner(useScanner), manualScanning(manualScanning), moveScanning(moveScanning),
	radarColor(1, 204, 0), pointColor(200, 200, 200), radarAngle(0), radarLength(400)
{
}

bool RegiloVisual::OnInit()
{
	wxInitAllImageHandlers();
	radarGradient.LoadFile("/home/brano/prog/regilo/examples/regilo-visual/images/radar-gradient.png");

	// Frame
	frame = new wxFrame(NULL, wxID_ANY, "Regilo Visual", wxDefaultPosition, wxSize(600, 400));

	// Frame StatusBar
	wxStatusBar *statusBar = frame->CreateStatusBar(2);

	std::string endpoint;
	if(useScanner) endpoint = controller->getEndpoint();
	else endpoint = controller->getLogPath();

	frame->SetStatusText("", 0);
	frame->SetStatusText("Connected to " + endpoint, 1);

	int sbWidths[] = { -1, 300 };
	statusBar->SetStatusWidths(2, sbWidths);

	// Panel
	panel = new wxPanel(frame);
	panel->GetEventHandler()->Bind(wxEVT_KEY_DOWN, &RegiloVisual::setMotorByKey, this);
	panel->GetEventHandler()->Bind(wxEVT_PAINT, &RegiloVisual::repaint, this);

	if(!manualScanning && !moveScanning)
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

	std::size_t fps = 24;
	radarThread = std::thread([this, fps]()
	{
		while(scanThreadRunning)
		{
			radarAngle += M_PI / fps / 2;
			this->GetTopWindow()->GetEventHandler()->CallAfter([this]()
			{
				frame->Refresh();
			});

			if(scanThreadRunning)
			{
				std::unique_lock<std::mutex> lock(radarThreadCVMutex);
				radarThreadCV.wait_for(lock, std::chrono::milliseconds(1000 / fps));
			}
		}
	});

	frame->Show(true);

	return true;
}

int RegiloVisual::OnExit()
{
	stopScanThread();
	if(scanThread.joinable()) scanThread.join();
	if(radarThread.joinable()) radarThread.join();

	return wxApp::OnExit();
}

void RegiloVisual::setMotorByKey(wxKeyEvent& keyEvent)
{
	if(keyEvent.GetKeyCode() == WXK_UP || keyEvent.GetKeyCode() == WXK_DOWN || keyEvent.GetKeyCode() == WXK_LEFT || keyEvent.GetKeyCode() == WXK_RIGHT)
	{
		if(moveScanning)
		{
			frame->SetStatusText("Move scanning...", 0);
			scanAndShow();
		}
	}

	regilo::BaseNeatoController *neatoController = dynamic_cast<regilo::NeatoController<regilo::SocketController>*>(controller);
	if(neatoController == nullptr) neatoController = dynamic_cast<regilo::NeatoController<regilo::SerialController>*>(controller);

	switch(keyEvent.GetKeyCode())
	{
		case WXK_UP:
			if(neatoController != nullptr)
			{
				controllerMutex.lock();
				frame->SetStatusText("Going up...", 0);

				if(keyEvent.ControlDown()) neatoController->setMotor(500, 500, 100);
				else neatoController->setMotor(100, 100, 50);

				controllerMutex.unlock();
			}
			break;

		case WXK_DOWN:
			if(neatoController != nullptr)
			{
				controllerMutex.lock();
				frame->SetStatusText("Going down...", 0);
				neatoController->setMotor(-100, -100, 50);
				controllerMutex.unlock();
			}
			break;

		case WXK_LEFT:
			if(neatoController != nullptr)
			{
				controllerMutex.lock();
				frame->SetStatusText("Turning left...", 0);
				if(keyEvent.ControlDown()) neatoController->setMotor(-30, 30, 50);
				else neatoController->setMotor(20, 100, 50);
				controllerMutex.unlock();
			}
			break;

		case WXK_RIGHT:
			if(neatoController != nullptr)
			{
				controllerMutex.lock();
				frame->SetStatusText("Turning right...", 0);
				if(keyEvent.ControlDown()) neatoController->setMotor(30, -30, 50);
				else neatoController->setMotor(100, 20, 50);
				controllerMutex.unlock();
			}
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

wxRect RegiloVisual::getRotatedBoundingBox(const wxRect& rect, double angle)
{
	double c = std::cos(angle);
	double s = std::sin(angle);

	wxPoint minBound(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
	wxPoint maxBound(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());

	wxPoint points[] = {rect.GetLeftTop(), rect.GetLeftBottom(), rect.GetRightTop(), rect.GetRightBottom()};
	for(wxPoint& point : points)
	{
		int x = std::ceil(c * point.x - s * point.y);
		int y = std::ceil(s * point.x + c * point.y);

		if(x < minBound.x) minBound.x = x;
		if(y < minBound.y) minBound.y = y;
		if(x > maxBound.x) maxBound.x = x;
		if(y > maxBound.y) maxBound.y = y;

		point.x = x;
		point.y = y;
	}

	wxRect box;
	box.SetLeftTop(minBound);
	box.SetRightBottom(maxBound);

	return box;
}

void RegiloVisual::drawRadarGradient(wxGCDC& gcdc, int width2, int height2)
{
	double radarLineX = width2 + radarLength * std::cos(radarAngle);
	double radarLineY = height2 - radarLength * std::sin(radarAngle);
	gcdc.DrawLine(width2, height2, radarLineX, radarLineY);

	wxImage rotatedImage = radarGradient.Rotate(radarAngle, wxPoint());

	wxRect boundingBox = getRotatedBoundingBox(wxRect(radarGradient.GetSize()), -radarAngle);

	wxPoint offset = boundingBox.GetLeftTop();
	offset.x += width2 - 1;
	offset.y += height2 - 1;

	gcdc.DrawBitmap(wxBitmap(rotatedImage), offset);
}

void RegiloVisual::repaint(wxPaintEvent&)
{
	wxPaintDC dc(panel);
	wxGCDC gcdc(dc);

	// Draw backgroud
	gcdc.SetBrush(*wxBLACK_BRUSH);
	gcdc.DrawRectangle(panel->GetSize());

	int width, height;
	panel->GetSize(&width, &height);

	int width2 = width / 2;
	int height2 = height / 2;

	// Draw axis
	gcdc.SetPen(*wxThePenList->FindOrCreatePen(radarColor, 2));
	gcdc.DrawLine(0, height2, width, height2);
	gcdc.DrawLine(width2, 0, width2, height);

	// Draw circles
	gcdc.SetBrush(*wxTRANSPARENT_BRUSH);
	for(std::size_t radius = 100; radius <= 400; radius += 100)
	{
		gcdc.DrawCircle(width2, height2, radius);
	}

	drawRadarGradient(gcdc, width2, height2);

	controllerMutex.lock();

	gcdc.SetPen(*wxThePenList->FindOrCreatePen(pointColor));
	for(const regilo::ScanRecord& record : data)
	{
		if(record.error) continue;

		double distance = record.distance / 10;
		double x = width2 + distance * std::cos(record.angle);
		double y = height2 - distance * std::sin(record.angle);

		gcdc.DrawRectangle(x, y, 2, 2);
	}

	controllerMutex.unlock();
}

void RegiloVisual::stopScanThread()
{
	if(scanThreadRunning)
	{
		scanThreadRunning = false;
		scanThreadCV.notify_one();
		radarThreadCV.notify_one();
	}
}

void RegiloVisual::scanAndShow()
{
	controllerMutex.lock();

	data = controller->getScan(useScanner);
	bool emptyData = data.empty();
	if(emptyData) stopScanThread();

	controllerMutex.unlock();

	this->GetTopWindow()->GetEventHandler()->CallAfter([this, emptyData]()
	{
		if(emptyData) frame->SetStatusText("No more scans to show (EOF).", 0);
		else frame->Refresh();
	});
}

void RegiloVisual::Display(wxApp *app, int& argc, char **argv)
{
	wxApp::SetInstance(app);
	wxEntryStart(argc, argv);
	app->CallOnInit();
	app->OnRun();
	app->OnExit();
	wxEntryCleanup();
}
