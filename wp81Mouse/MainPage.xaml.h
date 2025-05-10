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
		void AppBarToggleButton_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void AppBarToggleButton_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		void ConnectMouse();
		void DisconnectMouse();

		void Trackpad_Moved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
		void Trackpad_Released(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);

		void Button_Pressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
		void Button_Released(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);

		void Vwheel_Moved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
		void Vwheel_Released(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);

		void Hwheel_Moved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
		void Hwheel_Released(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
	};
}
