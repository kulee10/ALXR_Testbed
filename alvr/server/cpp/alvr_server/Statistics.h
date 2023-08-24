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
		idle_time = time(NULL);
		readTable();
		readConfig();
		// [kyl] end
	}

	// [kyl] begin
	void readConfig() {
		std::fstream file("D:/kyl/ALXR_Testbed/alvr/config/config.csv", std::ios::in);
		bool first = true;
		if (file.is_open()) {
			while (getline(file, line)) {
				if (!first) {
					row.clear();
					std::stringstream str(line);
					while(getline(str, word, ','))
						row.push_back(std::stof(word));
					vr_exp = row[0];
					gaming_freq = row[1];
					SI = row[2];
					TI = row[3];
					table_ID = row[4];
					algo_ID = row[5];

					// choose table
					if (table_ID == 0) {
						table = table_general;
						row_num = 14670;
						Info("Table: general");
						Info("entries: %d", table.size());
					}
					else if (table_ID == 1) {
						table = table_angryBird;
						row_num = 4890;
						Info("Table: angryBird");
						Info("entries: %d", table.size());
					}
					else if (table_ID == 2) {
						table = table_beatSaber;
						row_num = 4890;
						Info("Table: beatSaber");
						Info("entries: %d", table.size());
					}
					else if (table_ID == 3) {
						table = table_artPuzzle;
						row_num = 4890;
						Info("Table: artPuzzle");
						Info("entries: %d", table.size());
					}
				}
				else {
					first = false;
				}
			}
			file.close();
		}
	}

	void readTable() {
		// general table
		std::fstream file("D:/kyl/QoE_Modeling/lookupTable_interpolate.csv", std::ios::in);
		bool first = true;
		if (file.is_open()) {
			while (getline(file, line)) {
				if (!first) {
					row.clear();
					std::stringstream str(line);
					while(getline(str, word, ','))
						row.push_back(std::stof(word));
					table_general.push_back(row);
				}
				else {
					first = false;
				}
			}
			file.close();
		}

		// angryBird table
		std::fstream file_a("D:/kyl/QoE_Modeling/lookupTable_angryBird_interpolate.csv", std::ios::in);
		first = true;
		if (file_a.is_open()) {
			while (getline(file_a, line)) {
				if (!first) {
					row.clear();
					std::stringstream str(line);
					while(getline(str, word, ','))
						row.push_back(std::stof(word));
					table_angryBird.push_back(row);
				}
				else {
					first = false;
				}
			}
			file_a.close();
		}

		// beatSaber table
		std::fstream file_b("D:/kyl/QoE_Modeling/lookupTable_beatSaber_interpolate.csv", std::ios::in);
		first = true;
		if (file_b.is_open()) {
			while (getline(file_b, line)) {
				if (!first) {
					row.clear();
					std::stringstream str(line);
					while(getline(str, word, ','))
						row.push_back(std::stof(word));
					table_beatSaber.push_back(row);
				}
				else {
					first = false;
				}
			}
			file_b.close();
		}

		// artPuzzle table
		std::fstream file_p("D:/kyl/QoE_Modeling/lookupTable_artPuzzle_interpolate.csv", std::ios::in);
		first = true;
		if (file_p.is_open()) {
			while (getline(file_p, line)) {
				if (!first) {
					row.clear();
					std::stringstream str(line);
					while(getline(str, word, ','))
						row.push_back(std::stof(word));
					table_artPuzzle.push_back(row);
				}
				else {
					first = false;
				}
			}
			file_p.close();
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
			m_totalLatency = latencyUs * 0.5 + m_totalLatency * 0.5;
		}
	}

	void NetworkSend(uint64_t latencyUs) {
		if (latencyUs > 5e5)
			latencyUs = 5e5;
		if (m_sendLatency == 0) {
			m_sendLatency = latencyUs;
		} else {
			m_sendLatency = latencyUs * 0.5 + m_sendLatency * 0.5;
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

	uint64_t GetThroughput() {
		return m_throughput;
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
	bool checkUpdateFlag() {
		return updateFlag;	
	}

	void resetUpdateFlag() {
		updateFlag = false;
	}

	float getTableID() {
		return table_ID;
	}

	float getAlgoID() {
		return algo_ID;
	}

	void updateThroughput(uint64_t throughput) {
		m_throughput = alpha * throughput + (1-alpha) * m_throughput;
		// m_throughput = throughput;
	}

	bool CheckUpdate() {
		time_t cur_time = time(NULL);
		int interval = cur_time - idle_time;
		if (interval > 10) { // avoid idle time
			interval = cur_time - init_time_br;
		}
		else {
			interval = 0;
		}

		// reconfiguration periodically (5s)
		if (interval >= 5 && algo_ID == 2) {
			// match latency
			for (int i = 0; i < 5; i++) {
				latency_diff = (m_totalLatency / 1000) - latency[i];
				latency_diff = abs(latency_diff);
				// Info("latency_diff: %d", latency_diff);
				if (latency_diff <= min_latency_diff) {
					min_latency_diff = latency_diff;
					min_latency_diff_idx = i;
				}
			}

			// match throughput
			for (int i = 0; i < 7; i++) {
				throughput_diff = m_throughput - throughput[i]; 
				throughput_diff = abs(throughput_diff);
				// Info("throughput_diff: %d", throughput_diff);
				if (throughput_diff <= min_throughput_diff) {
					min_throughput_diff = throughput_diff;
					min_throughput_diff_idx = i;
				}
			}
			
			// match table
			for (int i = 0; i < row_num; i++) {
				if (table_ID == 0) {
					if (table[i][4] == latency[min_latency_diff_idx] && table[i][5] == throughput[min_throughput_diff_idx] && table[i][7] == vr_exp && table[i][8] == gaming_freq && table[i][9] == SI && table[i][10] == TI) {
						// float mos_diff = abs(table[i][11] - current_mos);
						if (table[i][11] >= current_mos) {
							current_mos = table[i][11];
							target_idx = i;
						}
					}
				}
				else {
					if (table[i][4] == latency[min_latency_diff_idx] && table[i][5] == throughput[min_throughput_diff_idx] && table[i][7] == vr_exp && table[i][8] == gaming_freq) {
						// float mos_diff = abs(table[i][11] - current_mos);
						if (table[i][9] >= current_mos) {
							current_mos = table[i][9];
							target_idx = i;
						}
					}
				}
			}

			Info("throughput: %d", throughput[min_throughput_diff_idx]);
			Info("latency: %d", latency[min_latency_diff_idx]);
			Info("target idx: %d", target_idx);
			Info("current_mos: %f", current_mos);

			// increase the bitrate if the parameter didn't update
			if (target_idx == prev_idx) {	
				Info("Increase bitrate!");
				if (m_bitrate <= 31) {
					m_bitrate += 1;
				}
				else {
					m_bitrate = 32;
				}

				// bitrate debug
				if (m_bitrate > 32) {
					Info("Exceed bitrate bound!");
				}
			}
			else {
				// update parameters
				m_bitrate = table[target_idx][0];
				m_refreshRate = table[target_idx][1];
				m_renderWidth = table[target_idx][2];
				m_renderHeight = table[target_idx][3];
			}

			// save previous parameters
			prev_idx = target_idx;

			// update and reset parameters
			init_time_br = cur_time;
			updateFlag = true;
			min_latency_diff = 10000;
			min_throughput_diff = 10000;
			min_throughput_diff_idx = 0;
			min_latency_diff_idx = 0;
			current_mos = 0;

			return true;
		}
		return false;
	}

	// bitrate update algorithm
	bool CheckBitrateUpdated() {
		Info("ALVR Adaptation");
		uint64_t latencyUs = m_sendLatency;
		if (latencyUs != 0) {
			if (latencyUs > m_adaptiveBitrateTarget + m_adaptiveBitrateThreshold) {
				if (m_bitrate < 2 + m_adaptiveBitrateDownRate)
					m_bitrate = 2;
				else
					m_bitrate -= m_adaptiveBitrateDownRate;
			} else if (latencyUs < m_adaptiveBitrateTarget - m_adaptiveBitrateThreshold) {
				if (m_bitrate > m_adaptiveBitrateMaximum - m_adaptiveBitrateUpRate)
					m_bitrate = m_adaptiveBitrateMaximum;
				else if (m_bitsSentInSecondPrev * 1e-6 > m_bitrate * m_adaptiveBitrateLightLoadThreshold * (m_framesPrevious == 0 ? m_refreshRate : m_framesPrevious) / m_refreshRate)
					m_bitrate += m_adaptiveBitrateUpRate;
			}
		}
		if (m_bitrateUpdated != m_bitrate) {
			m_bitrateUpdated = m_bitrate;
			return true;
		}
		else {
			return false;
		}
	}

	// framerate update algorithm
	bool CheckFramerateUpdated() {
		return false;
	}

	bool CheckResolutionUpdated() {
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

	// [kyl throughput]
	uint64_t m_throughput = 0;
	// [kyl end]

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
	time_t idle_time;

	std::vector<std::vector<float>> table;
	std::vector<std::vector<float>> table_general;
	std::vector<std::vector<float>> table_angryBird;
	std::vector<std::vector<float>> table_beatSaber;
	std::vector<std::vector<float>> table_artPuzzle;
	float table_ID = 0;
	int row_num = 14670;
	std::vector<float> row;
	std::string line, word;
	float algo_ID = 0;

	// quality modeling inputs
	float vr_exp = 0;
	float gaming_freq = 1;
	float SI = 35.72;
	float TI = 24.41;
	int latency[5] = {77, 149, 219, 384, 583};
	int throughput[7] = {4, 6, 18, 21, 26, 30, 36};
	int packetloss[1] = {0};
	int latency_diff = 0;
	int min_latency_diff = 10000;
	int min_latency_diff_idx = 0;
	int throughput_diff = 0;
	int min_throughput_diff = 10000;
	int min_throughput_diff_idx = 0;
	float current_mos = 0;
	int target_idx = 0;
	int prev_idx = 0;
	bool updateFlag = false;
	float alpha = 0.5;
	// [kyl] end
	
	// Total/Encode/Send/Decode/ClientFPS/Ping
	float m_statistics[6];
	uint64_t m_statisticsCount = 0;
};
