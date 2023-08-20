#include "cardinal_pch.h"
#include "cardinal.h"

#include "core.h"

int main() {

	EngineApplication* engine = new EngineApplication();

	engine->Init();
	
	while (engine->IsApplicationRunning())
	{
		engine->Update();
	}

	engine->Shutdown();

	delete engine;

	return EXIT_SUCCESS;
}