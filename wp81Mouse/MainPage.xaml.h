//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace wp81Mouse
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		// Declare the DispatcherTimer member
		Windows::UI::Xaml::DispatcherTimer^ _checkTimer;

		// Declare the Tick event handler
		void CheckCondition_Tick(Platform::Object^ sender, Platform::Object^ e);

		void AppBarButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ConnectMouse();
		void DisconnectMouse();
	};
}
