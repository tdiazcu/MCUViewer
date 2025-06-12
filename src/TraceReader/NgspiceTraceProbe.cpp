#include "NgspiceTraceProbe.hpp"

#include <cstring>
#include <random>

#include "logging.h"
#include "register.h"

NgspiceTraceProbe::NgspiceTraceProbe(spdlog::logger* logger) : logger(logger)
{
	//init_chipids(const_cast<char*>("./chips"));
}

bool NgspiceTraceProbe::stopTrace()
{
	if (ngspice == nullptr)
		return false;

	ngspice_exit_debug_mode(ngspice);
	ngspice_trace_disable(ngspice);
	ngspice_close(ngspice);

	logger->info("Trace stopped.");
	return true;
}

bool NgspiceTraceProbe::startTrace(const TraceProbeSettings& probeSettings, uint32_t coreFrequency, uint32_t tracePrescaler, uint32_t activeChannelMask, bool shouldReset)
{
	ngspice = ngspice_open_udp(probeSettings.ip4.address, probeSettings.ip4.port);

	if (ngspice == nullptr)
	{
		logger->error("Ngspice not found!");
		return false;
	}

	if (shouldReset)
		ngspice_reset(ngspice, RESET_SOFT);

	uint32_t traceFrequency = coreFrequency / (tracePrescaler + 1);

	logger->info("Trace frequency {}", traceFrequency);
	logger->info("Trace prescaler {}", tracePrescaler);
	logger->info("Trace channels mask {}", activeChannelMask);

	if (ngspice_trace_enable(ngspice, traceFrequency))
	{
		logger->error("Unable to turn on tracing in ngspice");
		return false;
	}

	if (ngspice_run(ngspice, RUN_NORMAL))
	{
		logger->error("Unable to run target device");
		return false;
	}

	logger->info("Starting reader thread!");

	return true;
}

int32_t NgspiceTraceProbe::readTraceBuffer(uint8_t* buffer, uint32_t size)
{
	if (ngspice == nullptr)
		return -1;

	return ngspice_trace_read(ngspice, buffer, size);
}

std::vector<std::string> NgspiceTraceProbe::getConnectedDevices()
{
	//ngspice_t** ngspice_devs = nullptr;
	//uint32_t size;

	//size = ngspice_probe_usb(&ngspice_devs, CONNECT_HOT_PLUG, 24000);

	std::vector<std::string> deviceIDs;

	//for (size_t i = 0; i < size; i++)
	//{
	//	std::string serialNumber{ngspice_devs[i]->serial};
//
	//	if (!serialNumber.empty())
	//	{
	//		logger->info("Ngspice serial number {}", serialNumber);
	//		deviceIDs.push_back(serialNumber);
	//	}
	//}

	//ngspice_probe_usb_free(&ngspice_devs, size);

	if(ngspice != nullptr)
	{
		deviceIDs.push_back("Ngspice connected");
		logger->info("Ngspice connected");
	}

	return deviceIDs;
}