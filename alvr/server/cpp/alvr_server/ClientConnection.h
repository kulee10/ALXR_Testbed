#pragma once

#include <functional>
#include <memory>
#include <fstream>
#include <mutex>

#include "ALVR-common/packet_types.h"
#include "Settings.h"

#include "openvr_driver.h"

// [kyl] begin
#include "platform/win32/mutex.h"
// [kyl] end

class Statistics;

class ClientConnection {
public:

	ClientConnection();

	void FECSend(uint8_t *buf, int len, uint64_t targetTimestampNs, uint64_t videoFrameIndex);
	void SendVideo(uint8_t *buf, int len, uint64_t targetTimestampNs);
 	void ProcessTimeSync(TimeSync data);
	float GetPoseTimeOffset();
	void OnFecFailure();

	// [kyl] begin
	void reset_log();
	// [kyl] end

	std::shared_ptr<Statistics> GetStatistics();

	std::shared_ptr<Statistics> m_Statistics;

	uint32_t videoPacketCounter = 0;

	uint64_t m_RTT = 0;
	int64_t m_TimeDiff = 0;

	TimeSync m_reportedStatistics;
	uint64_t m_lastFecFailure = 0;
	// [kyl fec]
	static const uint64_t CONTINUOUS_FEC_FAILURE = 60 * 1000 * 1000;
	static const int INITIAL_FEC_PERCENTAGE = 1;
	static const int MAX_FEC_PERCENTAGE = 1;
	int m_fecPercentage = INITIAL_FEC_PERCENTAGE;
	// [kyl end]

	uint64_t mVideoFrameIndex = 1;

	uint64_t m_LastStatisticsUpdate;

	// [kyl] begin
	std::fstream fs;
	std::ifstream if_read;
	// std::string base_dir;
	std::string filepath;
	bool resetFlag = false;
	// [kyl] end
};
