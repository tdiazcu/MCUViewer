#ifndef _NGSPICETRACEPROBE_HPP
#define _NGSPICETRACEPROBE_HPP

#include <memory>

#include "ITraceProbe.hpp"
#include "spdlog/spdlog.h"
#include "stlink.h"

class NgspiceTraceProbe : public ITraceProbe
{
   public:
	explicit NgspiceTraceProbe(spdlog::logger* logger);
	bool startTrace(const TraceProbeSettings& probeSettings, uint32_t coreFrequency, uint32_t tracePrescaler, uint32_t activeChannelMask, bool shouldReset) override;
	bool stopTrace() override;
	int32_t readTraceBuffer(uint8_t* buffer, uint32_t size) override;

	std::string getTargetName() override { return std::string(); }
	std::vector<std::string> getConnectedDevices() override;

   private:
	ngspice_t* ng = nullptr;
	spdlog::logger* logger;
};
#endif