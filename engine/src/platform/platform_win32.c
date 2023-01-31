#include "platform/platform.h"

#ifdef L_ISWIN

#include "core/event.h"
#include "core/input.h"
#include "core/logger.h"

#include <windows.h>
#include <windowsx.h>

#include "renderer/vulkan_platform.h"

static const char* window_class_name = "lise_window_class";

typedef struct internal_state
{
	HINSTANCE h_instance;
	HWND hwnd;
} internal_state;

// Clock
static double clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK win32_process_message(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param);

bool lise_platform_init(
	lise_platform_state* plat_state,
	const char* application_name,
	int32_t x, int32_t y,
	int32_t width, int32_t height)
{
	
	plat_state->internal_state = malloc(sizeof(internal_state));
	internal_state* state = (internal_state*) plat_state->internal_state;

	state->h_instance = GetModuleHandleA(0);

	HICON icon = LoadIcon(state->h_instance, IDI_APPLICATION);
	WNDCLASSA wc;
	memset(&wc, 0, sizeof(wc));
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = win32_process_message;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = state->h_instance;
	wc.hIcon = icon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = window_class_name;

	if (!RegisterClassA(&wc))
	{
		MessageBoxA(0, "Window registration failed.", "Error", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	uint32_t client_x = x;
	uint32_t client_y = y;
	uint32_t client_width = width;
	uint32_t client_height = height;

	uint32_t window_x = client_x;
	uint32_t window_y = client_y;
	uint32_t window_width = client_width;
	uint32_t window_height = client_height;

	uint32_t window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;
	uint32_t window_ex_style = WS_EX_APPWINDOW;

	RECT border_rect = {0, 0, 0, 0};
	AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);
	
	window_x += border_rect.left;
	window_y += border_rect.top;

	window_width += border_rect.right - border_rect.left;
	window_height += border_rect.bottom - border_rect.top;

	HWND handle = CreateWindowExA(
		window_ex_style, window_class_name, application_name,
		window_style, window_x, window_y, window_width, window_height,
		0, 0, state->h_instance, 0);

	if (handle == 0)
	{
		MessageBoxA(NULL, "Window creation failed.", "Error", MB_ICONEXCLAMATION | MB_OK);

		// TODO: Add logger fatal log.

		return false;
	}

	state->hwnd = handle;

	// Show the window
	int32_t should_activate = 1; // Whether the window should accept input or not.
	int32_t show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;

	ShowWindow(state->hwnd, show_window_command_flags);

	// Time setup
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	clock_frequency = 1.0 / (double) freq.QuadPart;
	QueryPerformanceCounter(&start_time);

	return true;
}

void lise_platform_shutdown(lise_platform_state* plat_state)
{
	internal_state* state = (internal_state*) plat_state->internal_state;

	if (state->hwnd)
	{
		DestroyWindow(state->hwnd);
		state->hwnd = 0;
	}
}

bool lise_platform_poll_messages(lise_platform_state* plat_state)
{
	MSG message;
	while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}

	return true;
}

void lise_platform_console_write(const char *message, uint8_t colour)
{
	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	// FATAL,ERROR,WARN,INFO,DEBUG,TRACE
	static uint8_t levels[6] = {64, 4, 6, 2, 1, 8};
	SetConsoleTextAttribute(console_handle, levels[colour]);
	OutputDebugStringA(message);
	uint64_t length = strlen(message);
	LPDWORD number_written = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, number_written, 0);
}

void lise_platform_console_write_error(const char *message, uint8_t colour)
{
	HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
	// FATAL,ERROR,WARN,INFO,DEBUG,TRACE
	static uint8_t levels[6] = {64, 4, 6, 2, 1, 8};
	SetConsoleTextAttribute(console_handle, levels[colour]);
	OutputDebugStringA(message);
	uint64_t length = strlen(message);
	LPDWORD number_written = 0;
	WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, number_written, 0);
}

double lise_platform_get_absolute_time()
{
	LARGE_INTEGER now_time;
	QueryPerformanceCounter(&now_time);
	return (double) now_time.QuadPart * clock_frequency;
}

void lise_platform_sleep(uint64_t ms)
{
	Sleep(ms);
}

LRESULT CALLBACK win32_process_message(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param)
{
	switch (msg)
	{
		case WM_ERASEBKGND:
			// Notify WM that erasing will be handled by us.
			return 1;
		case WM_CLOSE:
			// TODO: Fire an event for the application to quit.
			lise_event_fire(LISE_EVENT_ON_WINDOW_CLOSE, (lise_event_context) {});

			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_SIZE:
		{
//			RECT r;
//			GetClientRect(hwnd, &r);
//
//			uint32_t width = r.right - r.left;
//			uint32_t height = r.bottom - r.top;
//
//			// TODO: Fire an event for window resize. 
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			// Key pressed / released
			int pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
			lise_keys key = (lise_keys) w_param;

			lise_input_process_keys(key, pressed);
		} break;
		case WM_MOUSEMOVE:
		{
			// Mouse move
			int32_t x_position = GET_X_LPARAM(l_param);
			int32_t y_position = GET_Y_LPARAM(l_param);
		
			lise_input_process_mouse_move((lise_vector2i) { x_position, y_position });
		} break;
		case WM_MOUSEWHEEL:
		{
			int8_t dz = GET_WHEEL_DELTA_WPARAM(w_param);
			if (dz != 0)
			{
				// Normalize input in range [-1, 1]
				dz = (dz < 0) ? -1 : 1;
			
				lise_input_process_mouse_wheel(dz);
			}
		} break;
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		{
//			int pressed = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN);
			// TODO: Input processing
		} break;
	}

	return DefWindowProcA(hwnd, msg, w_param, l_param);
}

const char** lise_platform_get_required_instance_extensions(uint32_t* out_extension_count)
{
	static const char* required_instance_extensions[] = {
		"VK_KHR_surface", "VK_KHR_win32_surface"
	};

	*out_extension_count = 2;

	return required_instance_extensions;
}

bool lise_vulkan_platform_create_vulkan_surface(lise_platform_state* plat_state,
                                                VkInstance instance,
                                                VkSurfaceKHR* out_surface)
{
	internal_state* state = (internal_state*) plat_state->internal_state;

    VkWin32SurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	create_info.hinstance = state->h_instance;
	create_info.hwnd = state->hwnd;

	if (vkCreateWin32SurfaceKHR(instance, &create_info, NULL, out_surface) != VK_SUCCESS)
	{
		LFATAL("Failed to create a vulkan surface.");
		return false;
	}

	return true;
}

#endif
