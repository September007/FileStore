#include<context.h>

Context::Context() {
	//set by default, change in load
	m_WriteFile = ::stdio_WriteFile;
	m_ReadFile = ::stdio_ReadFile;
	m_GetReferedBlockStoragePath = ::GetReferedBlockStoragePath_deep2;
}