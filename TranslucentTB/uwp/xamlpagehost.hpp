#pragma once
#include "basexamlpagehost.hpp"

#include <limits>
#include <string>

#include "winrt.hpp"
#include "undefgetcurrenttime.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/TranslucentTB.Xaml.Pages.h>
#include "redefgetcurrenttime.h"

#include "util/strings.hpp"
#include "../ProgramLog/error/win32.hpp"

template<typename T>
#ifdef __cpp_concepts // MIGRATION: IDE concepts support
	requires std::derived_from<T, winrt::impl::base_one<T, winrt::TranslucentTB::Xaml::Pages::FramelessPage>>
#endif
class XamlPageHost final : public BaseXamlPageHost {
private:
	T m_content;
	winrt::event_token m_TitleChangedToken;

	static constexpr std::wstring_view ExtractTypename()
	{
		constexpr std::wstring_view prefix = L"XamlPageHost<struct winrt::";
		constexpr std::wstring_view suffix = L">::ExtractTypename";

		std::wstring_view funcsig = UTIL_WIDEN(__FUNCSIG__);
		funcsig.remove_prefix(funcsig.find(prefix) + prefix.length());
		funcsig.remove_suffix(funcsig.length() - funcsig.rfind(suffix));

		return funcsig;
	}

	inline static const std::wstring &GetClassName()
	{
		static constexpr std::wstring_view type_name = ExtractTypename();
		static const std::wstring class_name(type_name);

		return class_name;
	}

	inline void SetTitle(...)
	{
		if (!SetWindowText(m_WindowHandle, m_content.Title().c_str()))
		{
			LastErrorHandle(spdlog::level::warn, L"Failed to set window title");
		}
	}

	inline void ArrangeContent(winrt::Windows::Foundation::Rect area) override
	{
		m_content.Arrange(area);
	}

public:
	template<typename... Args>
	inline XamlPageHost(HINSTANCE hInst, Args&&... args) :
		BaseXamlPageHost(GetClassName(), hInst),
		m_content(std::forward<Args>(args)...)
	{
		source().Content(m_content);

		SetTitle();
		m_TitleChangedToken.value = m_content.RegisterPropertyChangedCallback(winrt::TranslucentTB::Xaml::Pages::FramelessPage::TitleProperty(), { this, &XamlPageHost::SetTitle });

		// Magic that gives us shadows
		const MARGINS margins = { 1 };
		HresultVerify(DwmExtendFrameIntoClientArea(m_WindowHandle, &margins), spdlog::level::warn, L"Failed to extend frame into client area");

		m_content.Measure(winrt::Windows::UI::Xaml::SizeHelper::FromDimensions(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()));
		PositionWindow(CalculateWindowPosition(m_content.DesiredSize()), true);

		if (!UpdateWindow(m_WindowHandle))
		{
			MessagePrint(spdlog::level::warn, L"Failed to update window");
		}

		SetFocus(m_WindowHandle);
	}

	inline constexpr T &content() noexcept
	{
		return m_content;
	}

	inline ~XamlPageHost() override
	{
		m_content.UnregisterPropertyChangedCallback(winrt::TranslucentTB::Xaml::Pages::FramelessPage::TitleProperty(), std::exchange(m_TitleChangedToken.value, 0));
		m_content = nullptr;
	}

	XamlPageHost(const XamlPageHost &) = delete;
	XamlPageHost &operator =(const XamlPageHost &) = delete;
};
