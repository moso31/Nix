#pragma once
#include <string>

class NXRayTracePass
{
public:
	NXRayTracePass() {}
	~NXRayTracePass() {}

	std::string GetOutFilePath() { return m_outFilePath; }
	void SetOutFilePath(std::string path) { m_outFilePath = path; }
	virtual void Render() = 0;

protected:
	std::string m_outFilePath;
};
