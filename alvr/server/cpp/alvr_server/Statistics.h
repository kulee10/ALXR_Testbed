#pragma once

#include <algorithm>
#include <stdint.h>
#include <time.h>

#include "Utils.h"
#include "Settings.h"

// [kyl] begin
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cstdlib>
#include "Logger.h" 
// [kyl] end

class Statistics {
public:
	Statistics() {
		ResetAll();
		m_current = time(NULL);

		// [kyl] begin
		init_time_br = time(NULL);
		init_time_fr = time(NULL);
		init_time_re = time(NULL);
		// readcsv();
		// [kyl] end
	}

	// [kyl] begin
	void readcsv() {
		std::fstream file("D:/kyl/lookupTable.csv", std::ios::in);
		bool first = true;
		if (file.is_open()) {
			while (getline(file, line)) {
				if (!first) {
					row.clear();
					std::stringstream str(line);
					while(getline(str, word, ','))
						row.push_back(std::stof(word));
					table.push_back(row);
				}
				else {
					first = false;
				}
			}
			file.close();
		}
	}
	// [kyl] end

	void ResetAll() {
		m_packetsSentTotal = 0;
		m_packetsSentInSecond = 0;
		m_packetsSentInSecondPrev = 0;
		m_bitsSentTotal = 0;
		m_bitsSentInSecond = 0;
		m_bitsSentInSecondPrev = 0;

		m_framesInSecond = 0;
		m_framesPrevious = 0;

		m_totalLatency = 0;

		m_encodeLatencyTotalUs = 0;
		m_encodeLatencyMin = 0;
		m_encodeLatencyMax = 0;
		m_encodeSampleCount = 0;
		m_encodeLatencyAveragePrev = 0;
		m_encodeLatencyMinPrev = 0;
		m_encodeLatencyMaxPrev = 0;

		m_sendLatency = 0;
	}

	void CountPacket(int bytes) {
		CheckAndResetSecond();

		m_packetsSentTotal++;
		m_packetsSentInSecond++;
		m_bitsSentTotal += bytes * 8;
		m_bitsSentInSecond += bytes * 8;
	}

	void EncodeOutput(uint64_t latencyUs) {
		CheckAndResetSecond();

		m_framesInSecond++;
		m_encodeLatencyAveragePrev = latencyUs;
		m_encodeLatencyTotalUs += latencyUs;
		m_encodeLatencyMin = std::min(latencyUs, m_encodeLatencyMin);
		m_encodeLatencyMax = std::max(latencyUs, m_encodeLatencyMax);
		m_encodeSampleCount++;
	}

	void NetworkTotal(uint64_t latencyUs) {
		if (latencyUs > 5e5)
			latencyUs = 5e5;
		if (m_totalLatency == 0) {
			m_totalLatency = latencyUs;
		} else {
			m_totalLatency = latencyUs * 0.05 + m_totalLatency * 0.95;
		}
	}

	void NetworkSend(uint64_t latencyUs) {
		if (latencyUs > 5e5)
			latencyUs = 5e5;
		if (m_sendLatency == 0) {
			m_sendLatency = latencyUs;
		} else {
			m_sendLatency = latencyUs * 0.1 + m_sendLatency * 0.9;
		}
	}

	uint64_t GetPacketsSentTotal() {
		return m_packetsSentTotal;
	}
	uint64_t GetPacketsSentInSecond() {
		return m_packetsSentInSecondPrev;
	}

	// [kyl] begin
	// change bitrate
	uint64_t GetBitrate() {
		return m_bitrate;
	}

	// change framerate
	int64_t GetFramerate() {
		return m_refreshRate;
	}

	// change resolution scale
	uint32_t GetWidth() {
		return m_renderWidth;
	}
	uint32_t GetHeight() {
		return m_renderHeight;
	}
	// [kyl] end

	uint64_t GetBitsSentTotal() {
		return m_bitsSentTotal;
	}
	uint64_t GetBitsSentInSecond() {
		return m_bitsSentInSecondPrev;
	}
	float GetFPS() {
		return m_framesPrevious;
	}
	uint64_t GetTotalLatencyAverage() {
		return m_totalLatency;
	}
	uint64_t GetEncodeLatencyAverage() {
		return m_encodeLatencyAveragePrev;
	}
	uint64_t GetSendLatencyAverage() {
		return m_sendLatency;
	}

	// [kyl] begin
	bool CheckUpdate() {
		// bool state_br = CheckBitrateUpdated();
		// bool state_fr = CheckFramerateUpdated();
		// return state_br | state_fr;

		time_t cur_time = time(NULL);
		int interval = cur_time - init_time_br;

		if (interval >= 10) {
			// match latency
			for (int i = 0; i < 5; i++) {
				latency_diff = m_totalLatency - latency[i]; 
				latency_diff = abs(latency_diff);
				if (latency_diff < min_latency_diff) {
					min_latency_diff = latency_diff;
					min_latency_diff_idx = i;
				}
			}

			// match throughput
			for (int i = 0; i < 4; i++) {
				throughput_diff = m_bitsSentInSecondPrev * 1e-6 - throughput[i]; 
				throughput_diff = abs(throughput_diff);
				if (throughput_diff < min_throughput_diff) {
					min_throughput_diff = throughput_diff;
					min_throughput_diff_idx = i;
				}
			}
			
			// match table
			// for (int i = 0; i < 9000; i++) {
			// 	if (table[i][3] == latency[min_latency_diff_idx] && table[i][4] == throughput[min_throughput_diff_idx]) {
			// 		if (table[i][8] > current_mos) {
			// 			current_mos = table[i][8];
			// 			target_idx = i;
			// 		}
			// 	}
			// }
			// Info("target idx: %d", target_idx);
			// Info("current_mos: %f", current_mos);
			// update parameter
			// m_bitrate = table[target_idx][0];
			// m_refreshRate = table[target_idx][1];
			// if (table[target_idx][2] == 1) {
			// 	m_renderWidth = 1408;
			// 	m_renderHeight = 768;
			// }
			// else if (table[target_idx][2] == 1.5) {
			// 	m_renderWidth = 1760;
			// 	m_renderHeight = 960;
			// }
			// else if (table[target_idx][2] == 2) {
			// 	m_renderWidth = 2112;
			// 	m_renderHeight = 1184;
			// }
			// else if (table[target_idx][2] == 2.5) {
			// 	m_renderWidth = 2496;
			// 	m_renderHeight = 1344;
			// }
			// else if (table[target_idx][2] == 3) {
			// 	m_renderWidth = 2880;
			// 	m_renderHeight = 1568;
			// }

			// reset
			init_time_br = cur_time;
			min_latency_diff = 10000;
			min_throughput_diff = 10000;

			return true;
		}
		return false;
	}
	// bitrate update algorithm
	bool CheckBitrateUpdated() {
		// if (m_enableAdaptiveBitrate) {
		// 	uint64_t latencyUs = m_sendLatency;
		// 	if (latencyUs != 0) {
		// 		if (latencyUs > m_adaptiveBitrateTarget + m_adaptiveBitrateThreshold) {
		// 			if (m_bitrate < 5 + m_adaptiveBitrateDownRate)
		// 				m_bitrate = 5;
		// 			else
		// 				m_bitrate -= m_adaptiveBitrateDownRate;
		// 		} else if (latencyUs < m_adaptiveBitrateTarget - m_adaptiveBitrateThreshold) {
		// 			if (m_bitrate > m_adaptiveBitrateMaximum - m_adaptiveBitrateDownRate)
		// 				m_bitrate = m_adaptiveBitrateMaximum;
		// 			else if (m_bitsSentInSecondPrev * 1e-6 > m_bitrate * m_adaptiveBitrateLightLoadThreshold * (m_framesPrevious == 0 ? m_refreshRate : m_framesPrevious) / m_refreshRate)
		// 				m_bitrate += m_adaptiveBitrateUpRate;
		// 		}
		// 	}
		// 	if (m_bitrateUpdated != m_bitrate) {
		// 		m_bitrateUpdated = m_bitrate;
		// 		return true;
		// 	}
		// }

		// time_t cur_time = time(NULL);
		// int interval = cur_time - init_time_br;
		// // Info("interval: %d", interval);
		// if (interval >= 5 && m_bitrate >= 4) {
		// 	init_time_br = cur_time;
		// 	m_bitrate -= 2;
		// 	return true;
		// }
		return false;
	}

	// framerate update algorithm
	bool CheckFramerateUpdated() {
		// time_t cur_time = time(NULL);
		// int interval = cur_time - init_time_fr;
		// // Info("interval: %d", interval);
		// if (interval >= 5 && m_refreshRate >= 24) {
		// 	init_time_fr = cur_time;
		// 	m_refreshRate -= 12;
		// 	return true;
		// }
		return false;
	}

	bool CheckResolutionUpdated() {
		// time_t cur_time = time(NULL);
		// int interval = cur_time - init_time_re;
		// if (interval >= 5) {
		// 	if (m_renderWidth == 2880) {
		// 		m_renderWidth = 2112;
		// 		m_renderHeight = 1184;
		// 	}
		// 	else if (m_renderWidth == 2112) {
		// 		m_renderWidth = 1408;
		// 		m_renderHeight = 768;
		// 	}
		// 	else if (m_renderWidth == 1408){
		// 		m_renderWidth = 2880;
		// 		m_renderHeight = 1568;
		// 	}
		// 	init_time_re = cur_time;
		// 	return true;
		// }
		return false;
	}

	// [kyl] end

	void Reset() {
		for(int i = 0; i < 6; i++) {
			m_statistics[i] = 0;
		}
		m_statisticsCount = 0;
	}
	void Add(float total, float encode, float send, float decode, float fps, float ping) {
		m_statistics[0] += total;
		m_statistics[1] += encode;
		m_statistics[2] += send;
		m_statistics[3] += decode;
		m_statistics[4] += fps;
		m_statistics[5] += ping;
		m_statisticsCount++;
	}
	float Get(uint32_t i) {
		return (m_statistics[i] / m_statisticsCount);
	}

	float m_hmdBattery;
	bool m_hmdPlugged;
	float m_leftControllerBattery;
	float m_rightControllerBattery;

private:
	void ResetSecond() {
		m_packetsSentInSecondPrev = m_packetsSentInSecond;
		m_bitsSentInSecondPrev = m_bitsSentInSecond;
		m_packetsSentInSecond = 0;
		m_bitsSentInSecond = 0;

		m_framesPrevious = m_framesInSecond;
		m_framesInSecond = 0;

		m_encodeLatencyMinPrev = m_encodeLatencyMin;
		m_encodeLatencyMaxPrev = m_encodeLatencyMax;
		m_encodeLatencyTotalUs = 0;
		m_encodeSampleCount = 0;
		m_encodeLatencyMin = UINT64_MAX;
		m_encodeLatencyMax = 0;

		if (m_adaptiveBitrateUseFrametime) {
			if (m_framesPrevious > 0) {
				m_adaptiveBitrateTarget = 1e6 / m_framesPrevious + m_adaptiveBitrateTargetOffset;
			}
			if (m_adaptiveBitrateTarget > m_adaptiveBitrateTargetMaximum) {
				m_adaptiveBitrateTarget = m_adaptiveBitrateTargetMaximum;
			}
		}
	}

	void CheckAndResetSecond() {
		time_t current = time(NULL);
		if (m_current != current) {
			m_current = current;
			ResetSecond();
		}
	}

	uint64_t m_packetsSentTotal;
	uint64_t m_packetsSentInSecond;
	uint64_t m_packetsSentInSecondPrev;

	uint64_t m_bitsSentTotal;
	uint64_t m_bitsSentInSecond;
	uint64_t m_bitsSentInSecondPrev;

	uint32_t m_framesInSecond;
	uint32_t m_framesPrevious;

	uint64_t m_totalLatency = 0;

	uint64_t m_encodeLatencyTotalUs;
	uint64_t m_encodeLatencyMin;
	uint64_t m_encodeLatencyMax;
	uint64_t m_encodeSampleCount;
	uint64_t m_encodeLatencyAveragePrev = 0;
	uint64_t m_encodeLatencyMinPrev;
	uint64_t m_encodeLatencyMaxPrev;
	
	uint64_t m_sendLatency = 0;

	uint64_t m_bitrate = Settings::Instance().mEncodeBitrateMBs;
	uint64_t m_bitrateUpdated = Settings::Instance().mEncodeBitrateMBs;

	int64_t m_refreshRate = Settings::Instance().m_refreshRate;

	// [kyl] begin
	uint32_t m_renderWidth = Settings::Instance().m_renderWidth;
	uint32_t m_renderHeight = Settings::Instance().m_renderHeight;
	// [kyl] end

	bool m_enableAdaptiveBitrate = Settings::Instance().m_enableAdaptiveBitrate;
	uint64_t m_adaptiveBitrateMaximum = Settings::Instance().m_adaptiveBitrateMaximum;
	uint64_t m_adaptiveBitrateTarget = Settings::Instance().m_adaptiveBitrateTarget;
	bool m_adaptiveBitrateUseFrametime = Settings::Instance().m_adaptiveBitrateUseFrametime;
	uint64_t m_adaptiveBitrateTargetMaximum = Settings::Instance().m_adaptiveBitrateTargetMaximum;
	int32_t m_adaptiveBitrateTargetOffset = Settings::Instance().m_adaptiveBitrateTargetOffset;
	uint64_t m_adaptiveBitrateThreshold = Settings::Instance().m_adaptiveBitrateThreshold;

	uint64_t m_adaptiveBitrateUpRate = Settings::Instance().m_adaptiveBitrateUpRate;
	uint64_t m_adaptiveBitrateDownRate = Settings::Instance().m_adaptiveBitrateDownRate;

	float m_adaptiveBitrateLightLoadThreshold = Settings::Instance().m_adaptiveBitrateLightLoadThreshold;

	time_t m_current;

	// [kyl] begin
	time_t init_time_br;
	time_t init_time_fr;
	time_t init_time_re;

	std::vector<std::vector<float>> table;
	std::vector<float> row;
	std::string line, word;

	// quality modeling inputs
	int vr_exp = 0;
	int gaming_freq = 1;
	int latency[5] = {66, 96, 124, 187, 203};
	int throughput[4] = {5, 19, 26, 35};
	int packetloss[1] = {0};
	float latency_diff = 0;
	float min_latency_diff = 10000;
	int min_latency_diff_idx = 0;
	float throughput_diff = 0;
	float min_throughput_diff = 10000;
	int min_throughput_diff_idx = 0;
	float current_mos = 0;
	int target_idx = 0;
	// [kyl] end
	
	// Total/Encode/Send/Decode/ClientFPS/Ping
	float m_statistics[6];
	uint64_t m_statisticsCount = 0;
};
