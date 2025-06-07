#include "ConfigHandler.hpp"

#include <memory>
#include <random>
#include <variant>

#include "IDebugProbe.hpp"
#include "ITraceProbe.hpp"

/* TODO refactor whole config and persistent storage handling */
ConfigHandler::ConfigHandler(
	const std::string&  configFilePath,
	PlotHandler*        plotHandler,
	PlotHandler*        tracePlotHandler,
	PlotGroupHandler*   plotGroupHandler,
	VariableHandler*    variableHandler,
	ViewerDataHandler*  viewerDataHandler,
	TraceDataHandler*   traceDataHandler,
	spdlog::logger*     logger
) : configFilePath(configFilePath),
	plotHandler(plotHandler),
	tracePlotHandler(tracePlotHandler),
	plotGroupHandler(plotGroupHandler),
	variableHandler(variableHandler),
	viewerDataHandler(viewerDataHandler),
	traceDataHandler(traceDataHandler),
	logger(logger)
{
	ini = std::make_unique<mINI::INIStructure>();
	file = std::make_unique<mINI::INIFile>(configFilePath);
}

bool ConfigHandler::changeConfigFile(const std::string& newConfigFilePath)
{
	configFilePath = newConfigFilePath;
	file.reset();
	file = std::make_unique<mINI::INIFile>(configFilePath);
	return true;
}

void ConfigHandler::loadVariables()
{
	uint32_t varId = 0;
	std::string name = "xxx";

	/* needed for fractional varaibles postprocessing */
	std::unordered_map<std::string, std::string> fractionalBaseVariableNames;

	auto varFieldFromID = [](uint32_t id)
	{ return std::string("var" + std::to_string(id)); };

	while (!name.empty())
	{
		name = ini->get(varFieldFromID(varId)).get("name");

		std::shared_ptr<Variable> newVar = std::make_shared<Variable>(name);
		std::string trackedName = ini->get(varFieldFromID(varId)).get("tracked_name");
		if (trackedName.empty())
			trackedName = name;
		newVar->setTrackedName(trackedName);

		if (trackedName != name)
			newVar->setIsTrackedNameDifferent(true);

		newVar->setAddress(atoi(ini->get(varFieldFromID(varId)).get("address").c_str()));
		newVar->setType(static_cast<Variable::Type>(atoi(ini->get(varFieldFromID(varId)).get("type").c_str())));
		newVar->setColor(static_cast<uint32_t>(atol(ini->get(varFieldFromID(varId)).get("color").c_str())));
		newVar->setShift(atoi(ini->get(varFieldFromID(varId)).get("shift").c_str()));

		Variable::HighLevelType highLevelType = static_cast<Variable::HighLevelType>(atoi(ini->get(varFieldFromID(varId)).get("high_level_type").c_str()));
		newVar->setHighLevelType(highLevelType);
		if (newVar->isFractional())
		{
			fractionalBaseVariableNames[name] = ini->get(varFieldFromID(varId)).get("base_variable");

			Variable::Fractional frac = {.fractionalBits = static_cast<uint32_t>(atoi(ini->get(varFieldFromID(varId)).get("frac").c_str())),
										 .base = atof(ini->get(varFieldFromID(varId)).get("base").c_str()),
										 .baseVariable = nullptr};
			newVar->setFractional(frac);
		}
		uint32_t mask = atoi(ini->get(varFieldFromID(varId)).get("mask").c_str());
		if (mask == 0)
			mask = 0xFFFFFFFF;
		newVar->setMask(mask);

		std::string shouldUpdateFromElf = ini->get(varFieldFromID(varId)).get("should_update_from_elf");
		if (shouldUpdateFromElf.empty())
			shouldUpdateFromElf = "true";
		newVar->setShouldUpdateFromElf(shouldUpdateFromElf == "true" ? true : false);
		varId++;

		if (newVar->getAddress() % 4 != 0)
			logger->warn("--------- Unaligned variable address! ----------");

		if (!newVar->getName().empty())
		{
			variableHandler->addVariable(newVar);
			newVar->setIsFound(true);
			logger->info("Adding variable: {}", newVar->getName());
		}
	}

	/* varaible bases have to be handled after all variables are loaded */
	for (auto& [varName, baseVarName] : fractionalBaseVariableNames)
	{
		Variable* baseVariable = nullptr;
		auto variable = variableHandler->getVariable(varName);

		if (variableHandler->contains(baseVarName))
			baseVariable = variableHandler->getVariable(baseVarName).get();
		else
			logger->error("Fractional variable {} has no base variable {}", varName, baseVarName);

		Variable::Fractional frac = variable->getFractional();
		frac.baseVariable = baseVariable;
		variable->setFractional(frac);
	}
}

void ConfigHandler::loadPlots()
{
	uint32_t plotNumber = 0;
	std::string plotName("xxx");

	auto plotSeriesFieldFromID = [](uint32_t plotId, uint32_t seriesId)
	{ return std::string("plot" + std::to_string(plotId) + "-" + "series" + std::to_string(seriesId)); };

	while (!plotName.empty())
	{
		std::string sectionName("plot" + std::to_string(plotNumber));
		plotName = ini->get(sectionName).get("name");
		Plot::Type type = static_cast<Plot::Type>(atoi(ini->get(sectionName).get("type").c_str()));

		if (!plotName.empty())
		{
			plotHandler->addPlot(plotName);
			auto plot = plotHandler->getPlot(plotName);
			plot->setType(type);
			if (type == Plot::Type::XY)
			{
				std::string xAxisVariable = ini->get(sectionName).get("x_axis_variable");
				if (variableHandler->contains(xAxisVariable))
					plot->setXAxisVariable(variableHandler->getVariable(xAxisVariable).get());
			}

			logger->info("Adding plot: {}", plotName);
			uint32_t seriesNumber = 0;
			std::string varName = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("name");

			while (varName != "")
			{
				plot->addSeries(variableHandler->getVariable(varName).get());
				bool visible = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("visibility") == "true" ? true : false;
				plot->getSeries(varName)->visible = visible;
				std::string displayFormat = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("format");
				if (displayFormat == "")
					displayFormat = "DEC";
				plot->getSeries(varName)->format = displayFormatMap.at(displayFormat);
				logger->info("Adding series: {}", varName);
				seriesNumber++;
				varName = ini->get(plotSeriesFieldFromID(plotNumber, seriesNumber)).get("name");
			}
		}
		plotNumber++;
	}
}
void ConfigHandler::loadTracePlots()
{
	std::string plotName = "xxx";
	uint32_t plotNumber = 0;
	const uint32_t colors[] = {4294967040, 4294960666, 4294954035, 4294947661, 4294941030, 4294934656, 4294928025, 4294921651, 4294915020, 4294908646, 4294902015};
	const uint32_t colormapSize = sizeof(colors) / sizeof(colors[0]);

	while (!plotName.empty())
	{
		std::string sectionName("trace_plot" + std::to_string(plotNumber));
		plotName = ini->get(sectionName).get("name");
		bool visibility = ini->get(sectionName).get("visibility") == "true" ? true : false;
		Plot::Domain domain = static_cast<Plot::Domain>(atoi(ini->get(sectionName).get("domain").c_str()));
		Plot::TraceVarType traceVarType = static_cast<Plot::TraceVarType>(atoi(ini->get(sectionName).get("type").c_str()));
		std::string alias = ini->get(sectionName).get("alias");

		if (!plotName.empty())
		{
			tracePlotHandler->addPlot(plotName);
			auto plot = tracePlotHandler->getPlot(plotName);
			plot->setVisibility(visibility);
			plot->setDomain(domain);
			if (domain == Plot::Domain::ANALOG)
				plot->setTraceVarType(traceVarType);
			plot->setAlias(alias);
			logger->info("Adding trace plot: {}", plotName);

			auto newVar = std::make_shared<Variable>(plotName);
			newVar->setColor(colors[(colormapSize - 1) - (plotNumber % colormapSize)]);
			traceDataHandler->traceVars[plotName] = newVar;
			plot->addSeries(newVar.get());
			plot->getSeries(plotName)->visible = true;
		}
		plotNumber++;
	}
}

void ConfigHandler::loadPlotGroups()
{
	uint32_t groupNumber = 0;
	std::string groupName("xxx");

	auto plotGroupFieldFromID = [](uint32_t groupId, uint32_t plotId, const std::string& prefix = "")
	{ return std::string(prefix + "group" + std::to_string(groupId) + "-" + "plot" + std::to_string(plotId)); };

	plotGroupHandler->removeAllGroups();

	while (!groupName.empty())
	{
		std::string sectionName("group" + std::to_string(groupNumber));
		groupName = ini->get(sectionName).get("name");

		if (!groupName.empty())
		{
			auto group = plotGroupHandler->addGroup(groupName);
			logger->info("Adding group: {}", groupName);
			uint32_t plotNumber = 0;
			std::string plotName = ini->get(plotGroupFieldFromID(groupNumber, plotNumber)).get("name");
			std::string visibilityStr = ini->get(plotGroupFieldFromID(groupNumber, plotNumber)).get("visibility");
			bool visibility = (visibilityStr == "true" || visibilityStr.empty()) ? true : false;

			while (plotName != "")
			{
				group->addPlot(plotHandler->getPlot(plotName), visibility);
				logger->info("Adding plot {} to group {}", plotName, groupName);

				plotNumber++;
				plotName = ini->get(plotGroupFieldFromID(groupNumber, plotNumber)).get("name");
				visibilityStr = ini->get(plotGroupFieldFromID(groupNumber, plotNumber)).get("visibility");
				visibility = (visibilityStr == "true" || visibilityStr.empty()) ? true : false;
			}
		}
		groupNumber++;
	}
	/* Add all plots to the first group if there are no groups */
	if (plotGroupHandler->getGroupCount() == 0)
	{
		std::string groupName = "default group";
		auto group = plotGroupHandler->addGroup(groupName);
		plotGroupHandler->setActiveGroup(groupName);
		logger->info("Adding group: {}", groupName);

		for (std::shared_ptr<Plot> plot : *plotHandler)
			group->addPlot(plot);
	}
}

bool ConfigHandler::readConfigFile(std::string& elfPath)
{
	ViewerDataHandler::Settings viewerSettings{};
	TraceDataHandler::Settings traceSettings{};
	IDebugProbe::DebugProbeSettings debugProbeSettings{};
	ITraceProbe::TraceProbeSettings traceProbeSettings{};

	if (!file->read(*ini))
		return false;

	auto getValue = [&](std::string&& category, std::string&& field, auto&& result)
	{
		try
		{
			std::string value = ini->get(category).get(field);
			parseValue(value, result);
		}
		catch (const std::exception& ex)
		{
			logger->error("config parsing exception {}", ex.what());
		}
	};

	elfPath = ini->get("elf").get("file_path");

	getValue("settings", "version", globalSettings.version);
	getValue("settings", "sample_frequency_Hz", viewerSettings.sampleFrequencyHz);
	getValue("settings", "max_points", viewerSettings.maxPoints);
	getValue("settings", "max_viewport_points", viewerSettings.maxViewportPoints);
	getValue("settings", "stop_acq_on_elf_change", viewerSettings.stopAcqusitionOnElfChange);
	getValue("settings", "refresh_on_elf_change", viewerSettings.refreshAddressesOnElfChange);
	getValue("settings", "probe_type", debugProbeSettings.debugProbe);
	debugProbeSettings.device = ini->get("settings").get("target_name");
	getValue("settings", "probe_mode", debugProbeSettings.mode);
	getValue("settings", "probe_speed_kHz", debugProbeSettings.speedkHz);
	debugProbeSettings.serialNumber = ini->get("settings").get("probe_SN");
	getValue("settings", "should_log", viewerSettings.shouldLog);
	viewerSettings.logFilePath = ini->get("settings").get("log_directory");
	viewerSettings.gdbCommand = ini->get("settings").get("gdb_command");

	if (viewerSettings.gdbCommand.empty())
		viewerSettings.gdbCommand = "gdb";

	getValue("trace_settings", "core_frequency", traceSettings.coreFrequency);
	getValue("trace_settings", "trace_prescaler", traceSettings.tracePrescaler);
	getValue("trace_settings", "max_points", traceSettings.maxPoints);
	getValue("trace_settings", "max_viewport_points_percent", traceSettings.maxViewportPointsPercent);
	getValue("trace_settings", "trigger_channel", traceSettings.triggerChannel);
	getValue("trace_settings", "trigger_level", traceSettings.triggerLevel);
	getValue("trace_settings", "timeout", traceSettings.timeout);
	getValue("trace_settings", "probe_type", traceProbeSettings.debugProbe);
	traceProbeSettings.device = ini->get("trace_settings").get("target_name");
	getValue("trace_settings", "probe_speed_kHz", traceProbeSettings.speedkHz);
	traceProbeSettings.serialNumber = ini->get("trace_settings").get("probe_SN");
	getValue("trace_settings", "should_log", traceSettings.shouldLog);
	traceSettings.logFilePath = ini->get("trace_settings").get("log_directory");

	/* TODO magic numbers (lots of them)! */
	if (traceSettings.timeout == 0)
		traceSettings.timeout = 2;

	if (traceSettings.maxViewportPointsPercent == 0 && traceSettings.maxPoints == 0)
		traceSettings.triggerChannel = -1;

	if (traceSettings.maxViewportPointsPercent == 0)
		traceSettings.maxViewportPointsPercent = 50;

	if (traceSettings.maxPoints == 0)
		traceSettings.maxPoints = 10000;

	if (viewerSettings.sampleFrequencyHz == 0)
		viewerSettings.sampleFrequencyHz = 100;

	if (viewerSettings.maxPoints == 0)
		viewerSettings.maxPoints = 1000;

	if (viewerSettings.maxViewportPoints == 0)
		viewerSettings.maxViewportPoints = viewerSettings.maxPoints;

	if (debugProbeSettings.speedkHz == 0)
		debugProbeSettings.speedkHz = 100;

	loadVariables();
	loadPlots();
	loadTracePlots();
	loadPlotGroups();

	viewerDataHandler->setSettings(viewerSettings);
	viewerDataHandler->setProbeSettings(debugProbeSettings);

	traceDataHandler->setSettings(traceSettings);
	traceDataHandler->setProbeSettings(traceProbeSettings);

	return true;
}

bool ConfigHandler::saveConfigFile(const std::string& elfPath, const std::string& newSavePath)
{
	*ini = prepareSaveConfigFile(elfPath);

	if (newSavePath != "")
	{
		file.reset();
		file = std::make_unique<mINI::INIFile>(newSavePath);
	}

	return file->generate(*ini, true);
}

mINI::INIStructure ConfigHandler::prepareSaveConfigFile(const std::string& elfPath)
{
	mINI::INIStructure configIni;

	ViewerDataHandler::Settings viewerSettings = viewerDataHandler->getSettings();
	TraceDataHandler::Settings traceSettings = traceDataHandler->getSettings();
	IDebugProbe::DebugProbeSettings debugProbeSettings = viewerDataHandler->getProbeSettings();
	ITraceProbe::TraceProbeSettings traceProbeSettings = traceDataHandler->getProbeSettings();

	(configIni).clear();

	(configIni)["elf"]["file_path"] = elfPath;

	auto varFieldFromID = [](uint32_t id)
	{ return std::string("var" + std::to_string(id)); };

	auto plotFieldFromID = [](uint32_t id, const std::string& prefix = "")
	{ return std::string(prefix + "plot" + std::to_string(id)); };

	auto groupFieldFromID = [](uint32_t id, const std::string& prefix = "")
	{ return std::string(prefix + "group" + std::to_string(id)); };

	auto plotGroupFieldFromID = [](uint32_t groupId, uint32_t plotId, const std::string& prefix = "")
	{ return std::string(prefix + "group" + std::to_string(groupId) + "-" + "plot" + std::to_string(plotId)); };

	auto plotSeriesFieldFromID = [](uint32_t plotId, uint32_t seriesId, const std::string& prefix = "")
	{ return std::string(prefix + "plot" + std::to_string(plotId) + "-" + "series" + std::to_string(seriesId)); };

	(configIni)["settings"]["version"] = std::to_string(globalSettings.version);

	(configIni)["settings"]["sample_frequency_hz"] = std::to_string(viewerSettings.sampleFrequencyHz);
	(configIni)["settings"]["max_points"] = std::to_string(viewerSettings.maxPoints);
	(configIni)["settings"]["max_viewport_points"] = std::to_string(viewerSettings.maxViewportPoints);
	(configIni)["settings"]["refresh_on_elf_change"] = viewerSettings.refreshAddressesOnElfChange ? std::string("true") : std::string("false");
	(configIni)["settings"]["stop_acq_on_elf_change"] = viewerSettings.stopAcqusitionOnElfChange ? std::string("true") : std::string("false");
	(configIni)["settings"]["probe_type"] = std::to_string(debugProbeSettings.debugProbe);
	(configIni)["settings"]["target_name"] = debugProbeSettings.device;
	(configIni)["settings"]["probe_mode"] = std::to_string(debugProbeSettings.mode);
	(configIni)["settings"]["probe_speed_kHz"] = std::to_string(debugProbeSettings.speedkHz);
	(configIni)["settings"]["probe_SN"] = debugProbeSettings.serialNumber;
	(configIni)["settings"]["should_log"] = viewerSettings.shouldLog ? std::string("true") : std::string("false");
	(configIni)["settings"]["log_directory"] = viewerSettings.logFilePath;
	(configIni)["settings"]["gdb_command"] = viewerSettings.gdbCommand;

	(configIni)["trace_settings"]["core_frequency"] = std::to_string(traceSettings.coreFrequency);
	(configIni)["trace_settings"]["trace_prescaler"] = std::to_string(traceSettings.tracePrescaler);
	(configIni)["trace_settings"]["max_points"] = std::to_string(traceSettings.maxPoints);
	(configIni)["trace_settings"]["max_viewport_points_percent"] = std::to_string(traceSettings.maxViewportPointsPercent);
	(configIni)["trace_settings"]["trigger_channel"] = std::to_string(traceSettings.triggerChannel);
	(configIni)["trace_settings"]["trigger_level"] = std::to_string(traceSettings.triggerLevel);
	(configIni)["trace_settings"]["timeout"] = std::to_string(traceSettings.timeout);
	(configIni)["trace_settings"]["probe_type"] = std::to_string(traceProbeSettings.debugProbe);
	(configIni)["trace_settings"]["target_name"] = traceProbeSettings.device;
	(configIni)["trace_settings"]["probe_speed_kHz"] = std::to_string(traceProbeSettings.speedkHz);
	(configIni)["trace_settings"]["probe_SN"] = traceProbeSettings.serialNumber;
	(configIni)["trace_settings"]["should_log"] = traceSettings.shouldLog ? std::string("true") : std::string("false");
	(configIni)["trace_settings"]["log_directory"] = traceSettings.logFilePath;

	uint32_t varId = 0;
	for (std::shared_ptr<Variable> var : *variableHandler)
	{
		(configIni)[varFieldFromID(varId)]["name"] = var->getName();
		(configIni)[varFieldFromID(varId)]["tracked_name"] = var->getTrackedName();
		(configIni)[varFieldFromID(varId)]["address"] = std::to_string(var->getAddress());
		(configIni)[varFieldFromID(varId)]["type"] = std::to_string(static_cast<uint8_t>(var->getType()));
		(configIni)[varFieldFromID(varId)]["color"] = std::to_string(static_cast<uint32_t>(var->getColorU32()));
		(configIni)[varFieldFromID(varId)]["should_update_from_elf"] = var->getShouldUpdateFromElf() ? "true" : "false";
		(configIni)[varFieldFromID(varId)]["shift"] = std::to_string(var->getShift());
		(configIni)[varFieldFromID(varId)]["mask"] = std::to_string(var->getMask());
		(configIni)[varFieldFromID(varId)]["high_level_type"] = std::to_string(static_cast<uint8_t>(var->getHighLevelType()));

		if (var->isFractional())
		{
			auto fractional = var->getFractional();
			(configIni)[varFieldFromID(varId)]["frac"] = std::to_string(fractional.fractionalBits);
			(configIni)[varFieldFromID(varId)]["base"] = std::to_string(fractional.base);
			(configIni)[varFieldFromID(varId)]["base_variable"] = fractional.baseVariable != nullptr ? fractional.baseVariable->getName() : "";
		}

		varId++;
	}

	uint32_t plotId = 0;
	for (std::shared_ptr<Plot> plt : *plotHandler)
	{
		(configIni)[plotFieldFromID(plotId)]["name"] = plt->getName();
		(configIni)[plotFieldFromID(plotId)]["type"] = std::to_string(static_cast<uint8_t>(plt->getType()));

		if (plt->getType() == Plot::Type::XY)
			(configIni)[plotFieldFromID(plotId)]["x_axis_variable"] = plt->getXAxisVariable() != nullptr ? plt->getXAxisVariable()->getName() : "";

		uint32_t serId = 0;

		for (auto& [key, ser] : plt->getSeriesMap())
		{
			(configIni)[plotSeriesFieldFromID(plotId, serId)]["name"] = ser->var->getName();
			(configIni)[plotSeriesFieldFromID(plotId, serId)]["visibility"] = ser->visible ? "true" : "false";

			std::string displayFormat = "DEC";
			for (auto [format, value] : displayFormatMap)
			{
				if (value == ser->format)
				{
					displayFormat = format;
					break;
				}
			}

			(configIni)[plotSeriesFieldFromID(plotId, serId)]["format"] = displayFormat;
			serId++;
		}

		plotId++;
	}

	uint32_t groupId = 0;
	for (auto& [name, group] : *plotGroupHandler)
	{
		(configIni)[groupFieldFromID(groupId)]["name"] = group->getName();

		uint32_t plotId = 0;
		for (auto& [name, plotElem] : *group)
		{
			auto plot = plotElem.plot;
			bool visibility = plotElem.visibility;
			(configIni)[plotGroupFieldFromID(groupId, plotId)]["name"] = plot->getName();
			(configIni)[plotGroupFieldFromID(groupId, plotId)]["visibility"] = visibility ? "true" : "false";
			plotId++;
		}
		groupId++;
	}

	plotId = 0;
	for (std::shared_ptr<Plot> plt : *tracePlotHandler)
	{
		const std::string plotName = plotFieldFromID(plotId, "trace_");

		(configIni)[plotName]["name"] = plt->getName();
		(configIni)[plotName]["alias"] = plt->getAlias();
		(configIni)[plotName]["visibility"] = plt->getVisibility() ? "true" : "false";
		(configIni)[plotName]["domain"] = std::to_string(static_cast<uint8_t>(plt->getDomain()));
		if (plt->getDomain() == Plot::Domain::ANALOG)
			(configIni)[plotName]["type"] = std::to_string(static_cast<uint8_t>(plt->getTraceVarType()));
		plotId++;
	}

	return configIni;
}

bool ConfigHandler::isSavingRequired(const std::string& elfPath)
{
	mINI::INIStructure configIniFromApp = prepareSaveConfigFile(elfPath);
	mINI::INIStructure configIniFromFile{};

	if (!file->read(configIniFromFile))
		return true;

	return !(configIniFromApp == configIniFromFile);
}