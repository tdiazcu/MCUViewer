#include "NgspiceDebugProbe.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>

#include "logging.h"

NgspiceDebugProbe::NgspiceDebugProbe(spdlog::logger* logger) : logger(logger)
{
	//init_chipids(const_cast<char*>("./chips"));
}

bool NgspiceDebugProbe::startAcqusition(const DebugProbeSettings& probeSettings, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, uint32_t samplingFreqency)
{
	std::lock_guard<std::mutex> lock(mtx);

	ngspice = ngspice_open_udp(probeSettings.ip4.address, probeSettings.ip4.port);
	isRunning = false;

	if (ngspice != nullptr)
	{
		if (ngspice_enter_swd_mode(ngspice) != 0)
		{
			isRunning = false;
			stlink_close(sl);
			lastErrorMsg = "STM32 target not found!";
			return false;
		}

		isRunning = true;
		lastErrorMsg = "";
		return true;
	}
	lastErrorMsg = "STLink not found!";
    */

    return false;
}
bool NgspiceDebugProbe::stopAcqusition()
{
	std::lock_guard<std::mutex> lock(mtx);
	isRunning = false;
	stlink_close(sl);
	return true;
}
bool NgspiceDebugProbe::isValid() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return isRunning;
}

std::optional<IDebugProbe::varEntryType> NgspiceDebugProbe::readSingleEntry()
{
	return std::nullopt;
}

bool NgspiceDebugProbe::readMemory(uint32_t address, uint8_t* buf, uint32_t size)
{
	std::lock_guard<std::mutex> lock(mtx);
	if (!isRunning)
		return false;

	uint32_t valueRaw = 0;
	uint8_t shouldShift = address % 4;

	bool result = stlink_read_debug32(sl, address, &valueRaw) == 0;

	*(uint32_t*)buf = valueRaw;

	if (size == 1)
	{
		if (shouldShift == 0)
			*(uint32_t*)buf = (valueRaw & 0x000000ff);
		else if (shouldShift == 1)
			*(uint32_t*)buf = (valueRaw & 0x0000ff00) >> 8;
		else if (shouldShift == 2)
			*(uint32_t*)buf = (valueRaw & 0x00ff0000) >> 16;
		else if (shouldShift == 3)
			*(uint32_t*)buf = (valueRaw & 0xff000000) >> 24;
	}
	else if (size == 2)
	{
		if (shouldShift == 0)
			*(uint32_t*)buf = (valueRaw & 0x0000ffff);
		else if (shouldShift == 1)
			*(uint32_t*)buf = (valueRaw & 0x00ffff00) >> 8;
		else if (shouldShift == 2)
			*(uint32_t*)buf = (valueRaw & 0xffff0000) >> 16;
		else if (shouldShift == 3)
			*(uint32_t*)buf = (valueRaw & 0x000000ff) << 8 | (valueRaw & 0xff000000) >> 24;
	}
	else if (size == 4)
	{
		if (shouldShift == 1)
			*(uint32_t*)buf = (valueRaw & 0x000000ff) << 24 | (valueRaw & 0xffffff00) >> 8;
		else if (shouldShift == 2)
			*(uint32_t*)buf = (valueRaw & 0x0000ffff) << 16 | (valueRaw & 0xffff0000) >> 16;
		else if (shouldShift == 3)
			*(uint32_t*)buf = (valueRaw & 0x00ffffff) << 24 | (valueRaw & 0xff000000) >> 8;
	}
	return result;
}
bool NgspiceDebugProbe::writeMemory(uint32_t address, uint8_t* buf, uint32_t size)
{
	std::lock_guard<std::mutex> lock(mtx);
	if (!isRunning)
		return false;
	std::copy(buf, buf + size, sl->q_buf);
	return stlink_write_mem8(sl, address, size) == 0;
}

std::string NgspiceDebugProbe::getLastErrorMsg() const
{
	return lastErrorMsg;
}

std::vector<std::string> NgspiceDebugProbe::getConnectedDevices()
{
	std::lock_guard<std::mutex> lock(mtx);
	stlink_t** stdevs;
	uint32_t size;

	size = stlink_probe_usb(&stdevs, CONNECT_HOT_PLUG, 24000);

	std::vector<std::string> deviceIDs;

	for (size_t i = 0; i < size; i++)
	{
		std::string serialNumber{stdevs[i]->serial};

		if (!serialNumber.empty())
		{
			logger->info("STLink serial number {}", serialNumber);
			deviceIDs.push_back(serialNumber);
		}
	}

	stlink_probe_usb_free(&stdevs, size);

	return deviceIDs;
}