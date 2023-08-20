#pragma once

class EngineApplication
{
public:
	EngineApplication();
	~EngineApplication();

public:
	void Init();
	void Shutdown();
	void Update();

	bool IsApplicationRunning() { return this->m_isApplicationRunning; }

private:

	bool m_isApplicationRunning = false;

	EngineWindow* m_window = nullptr;
	EngineRenderer* m_renderer = nullptr;

	MSG m_MSG = {};

private:
	void PollEvents();
};

