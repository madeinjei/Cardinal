#include "cardinal_pch.h"
#include "cardinal.h"

#include "core.h"

EngineApplication::EngineApplication()
{
	m_window = new EngineWindow(L"CARDINAL_WINDOW", 1280, 720);
	m_renderer = new EngineRenderer(m_window);
}

EngineApplication::~EngineApplication() 
{
	delete m_window;
	delete m_renderer;
}

void EngineApplication::Init()
{
	m_renderer->Init();

	this->m_isApplicationRunning = true;
}

void EngineApplication::Shutdown()
{
	vkDeviceWaitIdle(m_renderer->GetVkDevice());

	m_renderer->Destroy();
}

void EngineApplication::Update()
{
	PollEvents();

	m_renderer->DrawFrame();
}

void EngineApplication::PollEvents()
{
	if(GetMessage(&m_MSG, m_window->GetWindow(), 0, 0) > 0)
	{
		if (m_MSG.message == WM_QUIT)
		{
			this->m_isApplicationRunning = false;
			PostQuitMessage(0);
		}
		else
		{
			TranslateMessage(&m_MSG);
			DispatchMessage(&m_MSG);
		}
	}
}
