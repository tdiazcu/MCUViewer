#include "NgspiceDebugProbe.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>

NgspiceDebugProbe::NgspiceDebugProbe(spdlog::logger* logger) : logger(logger)
{
}

bool NgspiceDebugProbe::startAcqusition(const DebugProbeSettings& probeSettings, std::vector<std::pair<uint32_t, uint8_t>>& addressSizeVector, uint32_t samplingFreqency)
{
	std::lock_guard<std::mutex> lock(mtx);
	int serialNumberInt = std::atoi(probeSettings.serialNumber.c_str());
	lastErrorMsg = "";
	isRunning = false;
	emptyMessageErrorCnt = 0;

	if (JLINKARM_EMU_SelectByUSBSN(serialNumberInt) < 0)
	{
		lastErrorMsg = "Could not connect to the selected probe";
		return false;
	}

	const char* error = JLINKARM_Open();

	if (error != 0)
		logger->error(error);

	JLINKARM_SetSpeed(probeSettings.speedkHz > maxSpeedkHz ? maxSpeedkHz : probeSettings.speedkHz);
	logger->info("J-Link speed set to: {}", JLINKARM_GetSpeed());

	/* select interface - SWD only for now */
	JLINKARM_TIF_Select(JLINKARM_TIF_SWD);

	/* set the desired target */
	char acOut[256];
	auto deviceCmd = "Device = " + probeSettings.device;
	JLINKARM_ExecCommand(deviceCmd.c_str(), acOut, sizeof(acOut));

	if (acOut[0] != 0)
		logger->error(acOut);

	/* try to connect to target */
	if (JLINKARM_Connect() < 0)
	{
		lastErrorMsg = "Could not connect to the target!";
		logger->error(lastErrorMsg);
		JLINKARM_Close();
		return isRunning;
	}

	if (probeSettings.mode == IDebugProbe::Mode::NORMAL)
	{
		isRunning = true;
		return true;
	}

	if (varTable.size() > 0)
		varTable.clear();

	trackedVarsCount = 0;

	/* account for timestamp in size */
	trackedVarsTotalSize = sizeof(uint32_t);
	for (auto [address, size] : addressSizeVector)
	{
		auto& desc = variableDesc[trackedVarsCount++];
		desc.Addr = address;
		desc.NumBytes = size;
		desc.Flags = 0;
		desc.Dummy = 0;
		addressSizeMap[address] = size;
		trackedVarsTotalSize += size;
	}

	if (samplingFreqency < 1)
		samplingFreqency = 1;

	uint32_t samplePeriodUs = 1.0 / (samplingFreqency * timestampResolution);
	int32_t result = JLINK_HSS_Start(variableDesc, trackedVarsCount, samplePeriodUs, JLINK_HSS_FLAG_TIMESTAMP_US);
	/* 1M is arbitraty value that works across all sampling frequencies */
	emptyMessageErrorThreshold = 1.0 / (timestampResolution * samplingFreqency) * 1000000;

	if (result >= 0)
		isRunning = true;
	else
	{
		JLINKARM_Close();
		isRunning = false;
	}

	if (result == -1)
		logger->error("Unspecified J-Link error!");
	else if (result == -2)
		logger->error("Failed to allocate memory for one or more buffers on the debug probe side!");
	else if (result == -3)
	{
		lastErrorMsg = "Too many memory variables specified!";
		logger->error(lastErrorMsg);
	}
	else if (result == -4)
	{
		lastErrorMsg = "Target does not support HSS mode!";
		logger->error(lastErrorMsg);
	}

	return isRunning;
}

bool NgspiceDebugProbe::stopAcqusition()
{
	std::lock_guard<std::mutex> lock(mtx);
	JLINKARM_Close();
	return true;
}

bool NgspiceDebugProbe::isValid() const
{
	return isRunning;
}

std::string NgspiceDebugProbe::getTargetName()
{
	std::lock_guard<std::mutex> lock(mtx);
	JLINKARM_DEVICE_SELECT_INFO info;
	info.SizeOfStruct = sizeof(JLINKARM_DEVICE_SELECT_INFO);
	int32_t index = JLINKARM_DEVICE_SelectDialog(NULL, 0, &info);

	JLINKARM_DEVICE_INFO devInfo{};
	devInfo.SizeOfStruct = sizeof(JLINKARM_DEVICE_INFO);
	JLINKARM_DEVICE_GetInfo(index, &devInfo);

	return devInfo.sName ? std::string(devInfo.sName) : std::string();
}

std::optional<IDebugProbe::varEntryType> NgspiceDebugProbe::readSingleEntry()
{
	std::lock_guard<std::mutex> lock(mtx);
	uint8_t rawBuffer[bufferSizeHSS]{};

	int32_t readSize = JLINK_HSS_Read(rawBuffer, sizeof(rawBuffer));

	if (readSize <= 0)
	{
		emptyMessageErrorCnt++;
		if (emptyMessageErrorCnt > emptyMessageErrorThreshold)
		{
			lastErrorMsg = "Error reading HSS data!";
			logger->error(lastErrorMsg);
			isRunning = false;
		}
		return varTable.pop();
	}

	emptyMessageErrorCnt = 0;

	for (int32_t i = 0; i < readSize; i += trackedVarsTotalSize)
	{
		varEntryType entry{};

		/* timestamp */
		entry.first = (*(uint32_t*)&rawBuffer[i]) * timestampResolution;

		int32_t k = i + 4;
		for (size_t j = 0; j < trackedVarsCount; j++)
		{
			uint32_t address = variableDesc[j].Addr;
			entry.second[address] = *(uint32_t*)&rawBuffer[k];
			k += addressSizeMap[address];
		}

		if (!varTable.push(entry))
		{
			lastErrorMsg = "HSS FIFO overflow!";
			logger->error(lastErrorMsg);
			isRunning = false;
		}
	}

	return varTable.pop();
}

bool NgspiceDebugProbe::readMemory(uint32_t address, uint8_t* buf, uint32_t size)
{
	std::lock_guard<std::mutex> lock(mtx);
	return (isRunning && JLINKARM_ReadMemEx(address, size, buf, 0) >= 0);
}

bool NgspiceDebugProbe::writeMemory(uint32_t address, uint8_t* buf, uint32_t size)
{
	std::lock_guard<std::mutex> lock(mtx);
	return (isRunning && JLINKARM_WriteMemEx(address, size, buf, 0) >= 0);
}

std::string NgspiceDebugProbe::getLastErrorMsg() const
{
	return lastErrorMsg;
}

std::vector<std::string> NgspiceDebugProbe::getConnectedDevices()
{
	std::lock_guard<std::mutex> lock(mtx);
	std::vector<std::string> deviceIDs{};

	JLINKARM_EMU_CONNECT_INFO connectInfo[maxDevices]{};
	int32_t result = JLINKARM_EMU_GetList(1, (JLINKARM_EMU_CONNECT_INFO*)connectInfo, maxDevices);

	if (result < 0)
	{
		logger->error("Error reading JLink devices list. Error code {}", result);
		return std::vector<std::string>{};
	}

	for (size_t i = 0; i < maxDevices; i++)
	{
		auto serialNumber = connectInfo[i].SerialNumber;

		if (serialNumber != 0)
		{
			logger->info("JLink serial number {}", serialNumber);
			deviceIDs.push_back(std::to_string(serialNumber));
		}
	}

	return deviceIDs;
}