//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace wp81Mouse;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Graphics::Display;

using namespace concurrency;

ConnectionStatus connectionStatus;
BYTE mouseButtons;
int16_t mouseX;
int16_t mouseY;
int16_t mouseWheel; 
int16_t mouseHwheel;
HANDLE hEventSendNotification;
double dPreviousX;
double dPreviousY;
unsigned int previousXYpointerId;

double dPreviousVwheel;
unsigned int previousVwheelPointerId;

double dPreviousHwheel;
unsigned int previousHwheelPointerId;

BOOL jiggler;
int16_t jigglerX[88] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,0,0,-1,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,-1,0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
int16_t jigglerY[88] = {1,1,1,1,1,1,1,0,1,0,0,0,0,-1,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,-1,0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,0,0,-1,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,-1,0,0,0,0,1,0,1,1,1,1,1,1,1};
unsigned int jigglerIndex;

MainPage::MainPage()
{
	InitializeComponent();

	// 1. Create the DispatcherTimer instance
	_checkTimer = ref new DispatcherTimer();

	// 2. Set the interval (TimeSpan is in 100-nanosecond units)
	// 1 second = 10,000,000 (10^7) units
	_checkTimer->Interval = TimeSpan{ 10000000 }; // 1 second

	// 3. Attach the Tick event handler
	_checkTimer->Tick += ref new Windows::Foundation::EventHandler<Platform::Object^>(this, &MainPage::CheckCondition_Tick);

	// Start the timer immediately here:
	_checkTimer->Start();

	JigglerAppBarButton->IsEnabled = FALSE;
	ConnectAppBarButton->IsEnabled = TRUE;
	DisconnectAppBarButton->IsEnabled = FALSE;

	connectionStatus = NOT_CONNECTED;

	mouseButtons = 0;
	mouseX = 0;
	mouseY = 0;
	mouseWheel = 0;
	mouseHwheel = 0;
	hEventSendNotification = CreateEventW(
		NULL,
		TRUE,	// manually reset
		FALSE,	// initial state: nonsignaled
		L"WP81_SEND_NOTIFICATION"
	);

	previousXYpointerId = 0;

	dPreviousVwheel = 0.0;
	previousVwheelPointerId = 0;

	dPreviousHwheel = 0.0;
	previousHwheelPointerId = 0;

	jiggler = FALSE;
	jigglerIndex = 0;
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter

	// TODO: Prepare page for display here.

	// TODO: If your application contains multiple pages, ensure that you are
	// handling the hardware Back button by registering for the
	// Windows::Phone::UI::Input::HardwareButtons.BackPressed event.
	// If you are using the NavigationHelper provided by some templates,
	// this event is handled for you.

	// Lock the display orientation to Portrait
	DisplayInformation::AutoRotationPreferences = DisplayOrientations::Portrait;
}

void MainPage::AppBarButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Button^ b = (Button^)sender;
	if (b->Tag->ToString() == "Connect")
	{
		ConnectMouse();
	}
	else if (b->Tag->ToString() == "Disconnect")
	{
		DisconnectMouse();
	}
}

void MainPage::ConnectMouse()
{
	connectionStatusTextBlock->Text = "Starting...";
	JigglerAppBarButton->IsEnabled = FALSE;
	ConnectAppBarButton->IsEnabled = FALSE;
	DisconnectAppBarButton->IsEnabled = FALSE;

	connectionStatus = STARTING;
	create_task([]()
	{
		if (ChangeRadioState(TRUE) == EXIT_FAILURE)
		{
			throw std::runtime_error("Cannot activate Bluetooth radio controller.");
		}

		if (startService() == EXIT_FAILURE)
		{
			throw std::runtime_error("Cannot start the device controller service.");
		}

		if (bleConnectionStart(&connectionStatus, 
			&mouseButtons, 
			&mouseX, 
			&mouseY, 
			&mouseWheel, 
			&mouseHwheel, 
			hEventSendNotification) == EXIT_FAILURE)
		{
			throw std::runtime_error("Cannot start the connection.");
		}
	}).then([](concurrency::task<void> previousTask) // Handle potential exceptions anywhere in the chain
	{
		try
		{
			previousTask.get();
		}
		catch (const std::exception& e)
		{
			connectionStatus = NOT_CONNECTED;
		}
	});
}


void MainPage::DisconnectMouse()
{
	connectionStatusTextBlock->Text = "Stopping...";
	connectionStatusTextBlock->ClearValue(Windows::UI::Xaml::Controls::TextBlock::ForegroundProperty);
	connectionStatusTextBlock->FontWeight = Windows::UI::Text::FontWeights::Normal;
	connectionStatus = STOPPING;
	JigglerAppBarButton->IsEnabled = FALSE;
	ConnectAppBarButton->IsEnabled = FALSE;
	DisconnectAppBarButton->IsEnabled = FALSE;

	create_task([this]()
	{
		bleConnectionStop();
		// We have to stop/start the Bluetooth in order to cancel the pending Ioctls.
		ChangeRadioState(FALSE);
		Sleep(1000); // Give some time to cancel the pending Ioctls.
		ChangeRadioState(TRUE);
		connectionStatus = NOT_CONNECTED;
	});
}



void MainPage::CheckCondition_Tick(Platform::Object^ sender, Platform::Object^ e)
{
	// This code runs on the UI thread, safe to update UI elements.

	switch (connectionStatus)
	{
	case NOT_CONNECTED:
		connectionStatusTextBlock->Text = "Not connected";
		JigglerAppBarButton->IsEnabled = FALSE;
		ConnectAppBarButton->IsEnabled = TRUE;
		DisconnectAppBarButton->IsEnabled = FALSE;
		break;
	case STARTING:
		connectionStatusTextBlock->Text = "Starting...";
		JigglerAppBarButton->IsEnabled = FALSE;
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = FALSE;
		break;
	case ADVERTISING:
		connectionStatusTextBlock->Text = "Advertising...";
		connectionStatusTextBlock->ClearValue(Windows::UI::Xaml::Controls::TextBlock::ForegroundProperty);
		connectionStatusTextBlock->FontWeight = Windows::UI::Text::FontWeights::Normal;
		JigglerAppBarButton->IsEnabled = FALSE;
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = TRUE;
		break;
	case PAIRING:
		connectionStatusTextBlock->Text = "Pairing...";
		JigglerAppBarButton->IsEnabled = FALSE;
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = FALSE;
		break;
	case SERVING_ATTRIBUTES:
		connectionStatusTextBlock->Text = "Serving attributes...";
		JigglerAppBarButton->IsEnabled = FALSE;
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = TRUE;
		break;
	case SENDING_NOTIFICATIONS:
		connectionStatusTextBlock->Text = "Connected";
		connectionStatusTextBlock->Foreground = ref new SolidColorBrush(Windows::UI::Colors::LightGreen);
		connectionStatusTextBlock->FontWeight = Windows::UI::Text::FontWeights::Bold;
		JigglerAppBarButton->IsEnabled = TRUE;
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = TRUE;
		break;
	case STOPPING:
		connectionStatusTextBlock->Text = "Stopping...";
		JigglerAppBarButton->IsEnabled = FALSE;
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = FALSE;
		break;
	default:
		connectionStatusTextBlock->Text = "Unkown";
		break;
	}

	if (jiggler)
	{
		mouseX = jigglerX[jigglerIndex];
		mouseY = jigglerY[jigglerIndex];

		if (connectionStatus == SENDING_NOTIFICATIONS)
		{
			SetEvent(hEventSendNotification);
		}

		jigglerIndex++;
		if (jigglerIndex == 88)
		{
			jigglerIndex = 0;
		}
	}
}

void MainPage::Trackpad_Moved(Platform::Object ^ sender, PointerRoutedEventArgs ^ e)
{
	// Get the pointer position
	Windows::UI::Input::PointerPoint^ pointerPoint = e->GetCurrentPoint(dynamic_cast<UIElement^>(sender));

	// Get the unique ID for this pointer
	unsigned int pointerId = pointerPoint->PointerId;

	// We manage only one touche pointer: it's not possible to move the mouse with 2 fingers.
	if (previousXYpointerId == pointerId && dPreviousX != 0.0 && dPreviousY != 0.0)
	{
		mouseX = static_cast<int16_t>(round(pointerPoint->Position.X - dPreviousX));
		mouseY = static_cast<int16_t>(round(pointerPoint->Position.Y - dPreviousY));

		String^ stringValue = "ID="+ pointerId +" X=" + mouseX + " Y=" + mouseY + "\n";

		OutputDebugStringW(stringValue->Data());

		if (connectionStatus == SENDING_NOTIFICATIONS)
		{
			SetEvent(hEventSendNotification);
		}
	}

	dPreviousX = pointerPoint->Position.X;
	dPreviousY = pointerPoint->Position.Y;
	previousXYpointerId = pointerId;
}

void MainPage::Trackpad_Released(Platform::Object ^ sender, PointerRoutedEventArgs ^ e)
{
	dPreviousX = 0.0;
	dPreviousY = 0.0;
	previousXYpointerId = 0;
	mouseX = 0;
	mouseY = 0;
	OutputDebugStringW(L"Trackpad: Touch released or exited\n");
}

void MainPage::AppBarToggleButton_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	jiggler = TRUE;
	jigglerIndex = 0;
	OutputDebugStringW(L"Checked\n");
}

void MainPage::AppBarToggleButton_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	jiggler = FALSE;
	mouseX = 0;
	mouseY = 0;
	OutputDebugStringW(L"Unchecked\n");
}

void MainPage::Button_Pressed(Platform::Object ^ sender, PointerRoutedEventArgs ^ e)
{
	Border^ b = (Border^)sender;
	OutputDebugStringW(b->Tag->ToString()->Data());
	OutputDebugStringW(L" Pressed\n");
	if (b->Tag->ToString() == "Button1")
	{
		mouseButtons |= 0x01;
	}
	else if (b->Tag->ToString() == "Button2")
	{
		mouseButtons |= 0x02;
	}
	else if (b->Tag->ToString() == "Button3")
	{
		mouseButtons |= 0x04;
	}
	else if (b->Tag->ToString() == "Button4")
	{
		mouseButtons |= 0x08;
	}
	else if (b->Tag->ToString() == "Button5")
	{
		mouseButtons |= 0x10;
	}

	if (connectionStatus == SENDING_NOTIFICATIONS)
	{
		SetEvent(hEventSendNotification);
	}
}

void MainPage::Button_Released(Platform::Object ^ sender, PointerRoutedEventArgs ^ e)
{
	Border^ b = (Border^)sender;
	OutputDebugStringW(b->Tag->ToString()->Data());
	OutputDebugStringW(L" Released\n");
	if (b->Tag->ToString() == "Button1")
	{
		mouseButtons &= 0xFE;
	}
	else if (b->Tag->ToString() == "Button2")
	{
		mouseButtons &= 0xFD;
	}
	else if (b->Tag->ToString() == "Button3")
	{
		mouseButtons &= 0xFB;
	}
	else if (b->Tag->ToString() == "Button4")
	{
		mouseButtons &= 0xF7;
	}
	else if (b->Tag->ToString() == "Button5")
	{
		mouseButtons &= 0xEF;
	}

	if (connectionStatus == SENDING_NOTIFICATIONS)
	{
		SetEvent(hEventSendNotification);
	}
}

void MainPage::Vwheel_Moved(Platform::Object ^ sender, PointerRoutedEventArgs ^ e)
{
	// Get the pointer position
	Windows::UI::Input::PointerPoint^ pointerPoint = e->GetCurrentPoint(dynamic_cast<UIElement^>(sender));

	// Get the unique ID for this pointer
	unsigned int pointerId = pointerPoint->PointerId;

	// We manage only one touche pointer
	if (previousVwheelPointerId == pointerId && dPreviousVwheel != 0.0)
	{
		mouseWheel = static_cast<int16_t>(round(pointerPoint->Position.Y - dPreviousVwheel));

		String^ stringValue = "ID=" + pointerId + " Wheel=" + mouseWheel + "\n";

		OutputDebugStringW(stringValue->Data());

		if (connectionStatus == SENDING_NOTIFICATIONS)
		{
			SetEvent(hEventSendNotification);
		}
	}

	dPreviousVwheel = pointerPoint->Position.Y;
	previousVwheelPointerId = pointerId;
}

void MainPage::Vwheel_Released(Platform::Object ^ sender, PointerRoutedEventArgs ^ e)
{
	dPreviousVwheel = 0.0;
	previousVwheelPointerId = 0;
	mouseWheel = 0;
	OutputDebugStringW(L"V-Wheel: Touch released or exited\n");
}

void MainPage::Hwheel_Moved(Platform::Object ^ sender, PointerRoutedEventArgs ^ e)
{
	// Get the pointer position
	Windows::UI::Input::PointerPoint^ pointerPoint = e->GetCurrentPoint(dynamic_cast<UIElement^>(sender));

	// Get the unique ID for this pointer
	unsigned int pointerId = pointerPoint->PointerId;

	// We manage only one touche pointer
	if (previousHwheelPointerId == pointerId && dPreviousHwheel != 0.0)
	{
		mouseHwheel = static_cast<int16_t>(round(pointerPoint->Position.X - dPreviousHwheel));

		String^ stringValue = "ID=" + pointerId + " Hwheel=" + mouseHwheel + "\n";

		OutputDebugStringW(stringValue->Data());

		if (connectionStatus == SENDING_NOTIFICATIONS)
		{
			SetEvent(hEventSendNotification);
		}
	}

	dPreviousHwheel = pointerPoint->Position.X;
	previousHwheelPointerId = pointerId;
}

void MainPage::Hwheel_Released(Platform::Object ^ sender, PointerRoutedEventArgs ^ e)
{
	dPreviousHwheel = 0.0;
	previousHwheelPointerId = 0;
	mouseHwheel = 0;
	OutputDebugStringW(L"H-Wheel: Touch released or exited\n");
}