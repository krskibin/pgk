#include "Window.h"
#include "WindowInitContext.h"
#include "WindowListeners.h"
#include "resource.h"
#include <cassert>

const wchar_t Window::s_windowClassName[] = L"FirstEngineClass";
const wchar_t Window::s_windowName[] = L"Fist engine";


Window::Window( HINSTANCE hInstance, int cmdShow )
	: m_hInstance( hInstance )
	, m_hWnd( nullptr )
	, m_cmdShow( cmdShow )
	, m_activeAppListener( nullptr )
	, m_inputListener( nullptr )
{
	DeclareHighDpiAware();

	RegisterClass();
}

Window::~Window()
{
	UnregisterClass();

	assert( !m_activeAppListener );
	assert( !m_inputListener );
}

void Window::Create( const WindowInitContext& initContext )
{
	const DWORD dwStyle = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU /* | WS_VISIBLE*/;
	const DWORD dwExStyle = 0;

	RECT windowRect = {};
	windowRect.left = 0;
	windowRect.top = 0;
	windowRect.right = initContext.width;
	windowRect.bottom = initContext.height;

	AdjustWindowRect( &windowRect, dwStyle, FALSE );

	CreateWindowExW( dwExStyle, s_windowClassName, s_windowName, dwStyle,
					 CW_USEDEFAULT, CW_USEDEFAULT,
					 windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
					 nullptr, nullptr, m_hInstance, reinterpret_cast< void* >( this ) );

	assert( m_hWnd );
}

void Window::Show()
{
	ShowWindow( m_hWnd, m_cmdShow );
	UpdateWindow( m_hWnd );
}

void Window::SetActiveAppListener( IWindowAcitveAppListener* lisener )
{
	assert( lisener && !m_activeAppListener || !lisener && m_activeAppListener );
	m_activeAppListener = lisener;
}

void Window::SetInputListener( IWindowInputListener* lisener )
{
	assert( lisener && !m_inputListener || !lisener && m_inputListener );
	m_inputListener = lisener;
}

void Window::DeclareHighDpiAware()
{
	// Declare this process to be high DPI aware, and prevent automatic scaling
	// Warning: This is better done as a <dpiaware> manifest element to avoid
	//          problems based on code that has run before this point.
	HINSTANCE hUser32 = LoadLibrary( L"user32.dll" );
	if( hUser32 )
	{
		typedef BOOL( WINAPI * LPSetProcessDPIAware )( void );
		LPSetProcessDPIAware pSetProcessDPIAware = (LPSetProcessDPIAware)GetProcAddress( hUser32,
																						 "SetProcessDPIAware" );
		if( pSetProcessDPIAware )
		{
			pSetProcessDPIAware();
		}
		FreeLibrary( hUser32 );
	}
}

void Window::RegisterClass()
{
	WNDCLASSEXW wcex = {};

	wcex.cbSize = sizeof( WNDCLASSEX );

	wcex.style = CS_CLASSDC;
	wcex.lpfnWndProc = StaticWindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_hInstance;
	wcex.hIcon = LoadIcon( m_hInstance, MAKEINTRESOURCE( IDI_PLAYGAMES ) );
	wcex.hCursor = LoadCursor( m_hInstance, IDC_ARROW );
	wcex.hbrBackground = nullptr;
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = s_windowClassName;
	wcex.hIconSm = LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_PLAYGAMES ) );

	::RegisterClassExW( &wcex );
}

void Window::UnregisterClass()
{
	::UnregisterClass( s_windowClassName, m_hInstance );
}

LRESULT CALLBACK Window::StaticWindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	if( message == WM_CREATE )
	{
		LPCREATESTRUCT data = (LPCREATESTRUCT)lParam;
		::SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)data->lpCreateParams );
		auto window = reinterpret_cast< Window* >( data->lpCreateParams );
		window->m_hWnd = hWnd;
	}

	// Process messages by window message function
	auto window = reinterpret_cast< Window* >(::GetWindowLongPtr( hWnd, GWLP_USERDATA ) );
	if( window )
	{
		return window->WindowProc( message, wParam, lParam );
	}
	else
	{
		return DefWindowProc( hWnd, message, wParam, lParam );
	}
}

LRESULT CALLBACK Window::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	// Consolidate the keyboard messages and pass them to the app's keyboard callback
	if( message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP )
	{
		if( m_inputListener )
		{
			bool keyDown = ( message == WM_KEYDOWN || message == WM_SYSKEYDOWN );

			int keyCode = ( BYTE )( wParam & 0xFF );
			m_inputListener->OnKey( keyCode, keyDown );
		}
	}

	// Consolidate the mouse button messages and pass them to the app's mouse callback
	if( message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_LBUTTONDBLCLK || message == WM_MBUTTONDOWN || message == WM_MBUTTONUP || message == WM_MBUTTONDBLCLK || message == WM_RBUTTONDOWN || message == WM_RBUTTONUP || message == WM_RBUTTONDBLCLK || message == WM_XBUTTONDOWN || message == WM_XBUTTONUP || message == WM_XBUTTONDBLCLK || message == WM_MOUSEWHEEL || message == WM_MOUSEMOVE )
	{
		if( m_inputListener )
		{
			int posX = (short)LOWORD( lParam );
			int posY = (short)HIWORD( lParam );

			int nMouseButtonState = LOWORD( wParam );
			bool leftButton = ( ( nMouseButtonState & MK_LBUTTON ) != 0 );
			bool rightButton = ( ( nMouseButtonState & MK_RBUTTON ) != 0 );
			bool middleButton = ( ( nMouseButtonState & MK_MBUTTON ) != 0 );
			bool sideButton1 = ( ( nMouseButtonState & MK_XBUTTON1 ) != 0 );
			bool sideButton2 = ( ( nMouseButtonState & MK_XBUTTON2 ) != 0 );

			m_inputListener->OnKey( VK_LBUTTON, leftButton );
			m_inputListener->OnKey( VK_RBUTTON, rightButton );
			m_inputListener->OnKey( VK_MBUTTON, middleButton );
			m_inputListener->OnKey( VK_XBUTTON1, sideButton1 );
			m_inputListener->OnKey( VK_XBUTTON2, sideButton2 );

			m_inputListener->OnMouseMove( posX, posY );
		}
	}

	switch( message )
	{
	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONDBLCLK:
		{
			SetCapture( m_hWnd );

			RECT rect;
			GetClientRect( m_hWnd, &rect );
			ClientToScreen( m_hWnd, reinterpret_cast< POINT* >( &rect.left ) );
			ClientToScreen( m_hWnd, reinterpret_cast< POINT* >( &rect.right ) );
			ClipCursor( &rect );
		}
		return TRUE;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
		{
			ClipCursor( nullptr );

			ReleaseCapture();
		}
		return TRUE;

	case WM_CAPTURECHANGED:
		if( (HWND)lParam != m_hWnd )
		{
			ClipCursor( nullptr );

			ReleaseCapture();
		}
		return TRUE;

	case WM_ACTIVATEAPP:
		if( m_activeAppListener )
		{
			if( wParam == TRUE && !m_activeAppListener->IsAppActive() ) // Handle only if previously not active
			{
				m_activeAppListener->SetAppActive( true );

				//RECT rect;
				//GetClientRect( m_hWnd, &rect );
				//ClipCursor( &rect );
				//SetCapture( m_hWnd );
			}
			else if( wParam == FALSE && m_activeAppListener->IsAppActive() ) // Handle only if previously active
			{
				m_activeAppListener->SetAppActive( false );
			
				//ReleaseCapture();
				//ClipCursor( NULL );
			}
		}
		break;
	}

	return DefWindowProc( m_hWnd, message, wParam, lParam );
}
