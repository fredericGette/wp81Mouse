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

using namespace concurrency;

ConnectionStatus connectionStatus;
BYTE mouseButtons;
int16_t mouseX;
int16_t mouseY;
int16_t mouseWheel; 
int16_t mouseHwheel;
HANDLE hEventSendNotification;

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


	ConnectAppBarButton->IsEnabled = TRUE;
	DisconnectAppBarButton->IsEnabled = FALSE;

	connectionStatus = NOT_CONNECTED;

	mouseButtons = 0;
	mouseX = -1;
	mouseY = 0;
	mouseWheel = 0;
	mouseHwheel = 0;
	hEventSendNotification = CreateEventW(
		NULL,
		TRUE,	// manually reset
		FALSE,	// initial state: nonsignaled
		L"WP81_SEND_NOTIFICATION"
	);
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
		ConnectAppBarButton->IsEnabled = TRUE;
		DisconnectAppBarButton->IsEnabled = FALSE;
		break;
	case STARTING:
		connectionStatusTextBlock->Text = "Starting...";
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = FALSE;
		break;
	case ADVERTISING:
		connectionStatusTextBlock->Text = "Advertising...";
		connectionStatusTextBlock->ClearValue(Windows::UI::Xaml::Controls::TextBlock::ForegroundProperty);
		connectionStatusTextBlock->FontWeight = Windows::UI::Text::FontWeights::Normal;
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = TRUE;
		break;
	case PAIRING:
		connectionStatusTextBlock->Text = "Pairing...";
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = FALSE;
		break;
	case SERVING_ATTRIBUTES:
		connectionStatusTextBlock->Text = "Serving attributes...";
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = TRUE;
		break;
	case SENDING_NOTIFICATIONS:
		connectionStatusTextBlock->Text = "Connected";
		connectionStatusTextBlock->Foreground = ref new SolidColorBrush(Windows::UI::Colors::LightGreen);
		connectionStatusTextBlock->FontWeight = Windows::UI::Text::FontWeights::Bold;
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = TRUE;
		break;
	case STOPPING:
		connectionStatusTextBlock->Text = "Stopping...";
		ConnectAppBarButton->IsEnabled = FALSE;
		DisconnectAppBarButton->IsEnabled = FALSE;
		break;
	default:
		connectionStatusTextBlock->Text = "Unkown";
		break;
	}
}