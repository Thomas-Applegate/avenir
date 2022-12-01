#include "Future.h"

using namespace avenir;

Future<void>::Future(std::shared_ptr<State>& statePtr)
	: m_statePtr(statePtr) {}
