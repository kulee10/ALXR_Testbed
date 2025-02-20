#include "ClientConnection.h"
#include <mutex>
#include <string.h>
#include <iomanip>

#include "Statistics.h"
#include "Logger.h"
#include "bindings.h"
#include "Utils.h"
#include "Settings.h"
#include "alvr_server.h"

const int64_t STATISTICS_TIMEOUT_US = 1000 * 1000;

ClientConnection::ClientConnection() : m_LastStatisticsUpdate(0) {

	m_Statistics = std::make_shared<Statistics>();

	reed_solomon_init();
	
	videoPacketCounter = 0;
	m_fecPercentage = INITIAL_FEC_PERCENTAGE;
	memset(&m_reportedStatistics, 0, sizeof(m_reportedStatistics));
	m_Statistics->ResetAll();

	// [kyl begin]
	reset_log();
	// [kyl end]
}

// [kyl begin]
void ClientConnection::reset_log() {
	std::string config_dir;
	if_read.open("D:/kyl/ALXR_Testbed/alvr/config/testcase.txt");
	std::getline(if_read ,config_dir);
	filepath = config_dir + "/log.txt";
	// Info("ClientConnection target dir: %s\n", filepath);
	if_read.close();

	if(remove(filepath.c_str()) != 0)
		Info("Error deleting log file");
  	else
		Info("log file successfully deleted");

	fs.open(filepath, std::fstream::out | std::fstream::app);
}
// [kyl end]

void ClientConnection::FECSend(uint8_t *buf, int len, uint64_t targetTimestampNs, uint64_t videoFrameIndex) {
	int shardPackets = CalculateFECShardPackets(len, m_fecPercentage);

	int blockSize = shardPackets * ALVR_MAX_VIDEO_BUFFER_SIZE;

	int dataShards = (len + blockSize - 1) / blockSize;
	int totalParityShards = CalculateParityShards(dataShards, m_fecPercentage);
	int totalShards = dataShards + totalParityShards;

	assert(totalShards <= DATA_SHARDS_MAX);

	Debug("reed_solomon_new. dataShards=%d totalParityShards=%d totalShards=%d blockSize=%d shardPackets=%d\n"
		, dataShards, totalParityShards, totalShards, blockSize, shardPackets);

	reed_solomon *rs = reed_solomon_new(dataShards, totalParityShards);

	std::vector<uint8_t *> shards(totalShards);

	for (int i = 0; i < dataShards; i++) {
		shards[i] = buf + i * blockSize;
	}
	if (len % blockSize != 0) {
		// Padding
		shards[dataShards - 1] = new uint8_t[blockSize];
		memset(shards[dataShards - 1], 0, blockSize);
		memcpy(shards[dataShards - 1], buf + (dataShards - 1) * blockSize, len % blockSize);
	}
	for (int i = 0; i < totalParityShards; i++) {
		shards[dataShards + i] = new uint8_t[blockSize];
	}

	int ret = reed_solomon_encode(rs, &shards[0], totalShards, blockSize);
	assert(ret == 0);

	reed_solomon_release(rs);

	uint8_t packetBuffer[2000];
	VideoFrame *header = (VideoFrame *)packetBuffer;
	uint8_t *payload = packetBuffer + sizeof(VideoFrame);
	int dataRemain = len;

	header->type = ALVR_PACKET_TYPE_VIDEO_FRAME;
	header->trackingFrameIndex = targetTimestampNs;
	header->videoFrameIndex = videoFrameIndex;
	header->sentTime = GetTimestampUs();
	header->frameByteSize = len;
	header->fecIndex = 0;
	header->fecPercentage = (uint16_t)m_fecPercentage;
	for (int i = 0; i < dataShards; i++) {
		for (int j = 0; j < shardPackets; j++) {
			int copyLength = std::min(ALVR_MAX_VIDEO_BUFFER_SIZE, dataRemain);
			if (copyLength <= 0) {
				break;
			}
			memcpy(payload, shards[i] + j * ALVR_MAX_VIDEO_BUFFER_SIZE, copyLength);
			dataRemain -= ALVR_MAX_VIDEO_BUFFER_SIZE;

			header->packetCounter = videoPacketCounter;
			videoPacketCounter++;
			VideoSend(*header, (unsigned char *)packetBuffer + sizeof(VideoFrame), copyLength);
			m_Statistics->CountPacket(sizeof(VideoFrame) + copyLength);
			header->fecIndex++;
		}
	}
	header->fecIndex = dataShards * shardPackets;
	for (int i = 0; i < totalParityShards; i++) {
		for (int j = 0; j < shardPackets; j++) {
			int copyLength = ALVR_MAX_VIDEO_BUFFER_SIZE;
			memcpy(payload, shards[dataShards + i] + j * ALVR_MAX_VIDEO_BUFFER_SIZE, copyLength);

			header->packetCounter = videoPacketCounter;
			videoPacketCounter++;
			
			VideoSend(*header, (unsigned char *)packetBuffer + sizeof(VideoFrame), copyLength);
			m_Statistics->CountPacket(sizeof(VideoFrame) + copyLength);
			header->fecIndex++;
		}
	}

	if (len % blockSize != 0) {
		delete[] shards[dataShards - 1];
	}
	for (int i = 0; i < totalParityShards; i++) {
		delete[] shards[dataShards + i];
	}
}

void ClientConnection::SendVideo(uint8_t *buf, int len, uint64_t targetTimestampNs) {
	if (Settings::Instance().m_enableFec) {
		FECSend(buf, len, targetTimestampNs, mVideoFrameIndex);
	} else {
		VideoFrame header = {};
		header.packetCounter = this->videoPacketCounter;
		header.trackingFrameIndex = targetTimestampNs;
		header.videoFrameIndex = mVideoFrameIndex;
		header.sentTime = GetTimestampUs();
		header.frameByteSize = len;

		VideoSend(header, buf, len);

		m_Statistics->CountPacket(sizeof(VideoFrame) + len);

		this->videoPacketCounter++;
	}

	mVideoFrameIndex++;
}

void ClientConnection::ProcessTimeSync(TimeSync data) {
	m_Statistics->CountPacket(sizeof(TrackingInfo));

	TimeSync *timeSync = &data;
	uint64_t Current = GetTimestampUs();

	if (timeSync->mode == 0) {
		//timings might be a little incorrect since it is a mix from a previous sent frame and latest frame

		vr::Compositor_FrameTiming timing[2];
		timing[0].m_nSize = sizeof(vr::Compositor_FrameTiming);
		vr::VRServerDriverHost()->GetFrameTimings(&timing[0], 2);

		m_reportedStatistics = *timeSync;
		TimeSync sendBuf = *timeSync;
		sendBuf.mode = 1;
		sendBuf.serverTime = Current;
		sendBuf.serverTotalLatency = (int)(m_reportedStatistics.averageSendLatency + (timing[0].m_flPreSubmitGpuMs + timing[0].m_flPostSubmitGpuMs + timing[0].m_flTotalRenderGpuMs + timing[0].m_flCompositorRenderGpuMs + timing[0].m_flCompositorRenderCpuMs + timing[0].m_flCompositorIdleCpuMs + timing[0].m_flClientFrameIntervalMs + timing[0].m_flPresentCallCpuMs + timing[0].m_flWaitForPresentCpuMs + timing[0].m_flSubmitFrameMs) * 1000 + m_Statistics->GetEncodeLatencyAverage() + m_reportedStatistics.averageTransportLatency + m_reportedStatistics.averageDecodeLatency + m_reportedStatistics.idleTime);
		TimeSyncSend(sendBuf);

		m_Statistics->NetworkTotal(sendBuf.serverTotalLatency);
		m_Statistics->NetworkSend(m_reportedStatistics.averageTransportLatency);

		float renderTime = timing[0].m_flPreSubmitGpuMs + timing[0].m_flPostSubmitGpuMs + timing[0].m_flTotalRenderGpuMs + timing[0].m_flCompositorRenderGpuMs + timing[0].m_flCompositorRenderCpuMs;
		float idleTime = timing[0].m_flCompositorIdleCpuMs;
		float waitTime = timing[0].m_flClientFrameIntervalMs + timing[0].m_flPresentCallCpuMs + timing[0].m_flWaitForPresentCpuMs + timing[0].m_flSubmitFrameMs;

		if (timeSync->fecFailure) {
			OnFecFailure();
		}

		m_Statistics->Add(sendBuf.serverTotalLatency / 1000.0, 
			(double)(m_Statistics->GetEncodeLatencyAverage()) / US_TO_MS,
			m_reportedStatistics.averageTransportLatency / 1000.0,
			m_reportedStatistics.averageDecodeLatency / 1000.0,
			m_reportedStatistics.fps,
			m_RTT / 2. / 1000.);

		uint64_t now = GetTimestampUs();
		if (now - m_LastStatisticsUpdate > STATISTICS_TIMEOUT_US)
		{
			// Text statistics only, some values averaged
			// Info("#{ \"id\": \"Statistics\", \"data\": {"
			// 	"\"totalPackets\": %llu, "
			// 	"\"packetRate\": %llu, "
			// 	"\"packetsLostTotal\": %llu, "
			// 	"\"packetsLostPerSecond\": %llu, "
			// 	"\"totalSent\": %llu, "
			// 	"\"sentRate\": %.3f, "
			// 	"\"bitrate\": %llu, "
			// 	"\"ping\": %.3f, "
			// 	"\"totalLatency\": %.3f, "
			// 	"\"encodeLatency\": %.3f, "
			// 	"\"sendLatency\": %.3f, "
			// 	"\"decodeLatency\": %.3f, "
			// 	"\"fecPercentage\": %d, "
			// 	"\"fecFailureTotal\": %llu, "
			// 	"\"fecFailureInSecond\": %llu, "
			// 	"\"clientFPS\": %.3f, "
			// 	"\"serverFPS\": %.3f, "
			// 	"\"batteryHMD\": %d, "
			// 	"\"batteryLeft\": %d, "
			// 	"\"batteryRight\": %d"
			// 	"} }#\n",
			// 	m_Statistics->GetPacketsSentTotal(),
			// 	m_Statistics->GetPacketsSentInSecond(),
			// 	m_reportedStatistics.packetsLostTotal,
			// 	m_reportedStatistics.packetsLostInSecond,
			// 	m_Statistics->GetBitsSentTotal() / 8 / 1000 / 1000,
			// 	m_Statistics->GetBitsSentInSecond() / 1000. / 1000.0,
			// 	m_Statistics->GetBitrate(),
			// 	m_Statistics->Get(5),  //ping
			// 	m_Statistics->Get(0),  //totalLatency
			// 	m_Statistics->Get(1),  //encodeLatency
			// 	m_Statistics->Get(2),  //sendLatency
			// 	m_Statistics->Get(3),  //decodeLatency
			// 	m_fecPercentage,
			// 	m_reportedStatistics.fecFailureTotal,
			// 	m_reportedStatistics.fecFailureInSecond,
			// 	m_Statistics->Get(4),  //clientFPS
			// 	m_Statistics->GetFPS(),
			// 	(int)(m_Statistics->m_hmdBattery * 100),
			// 	(int)(m_Statistics->m_leftControllerBattery * 100),
			// 	(int)(m_Statistics->m_rightControllerBattery * 100));
			if (!resetConfigValue && resetCount == 0) {
				resetFlag = false;
			}
			reset_lock.lock();
			if (resetConfigValue && !resetFlag) {
				fs.close();
				reset_log();
				if (resetCount == 3) {
					resetCount = 0;
					resetConfigValue = false;
				} 
				else {
					resetCount += 1;
					resetFlag = true;
				}
				Info("ClientConnection reset count %d", resetCount);
				Info("ClientConnection reset flag %d", resetFlag);
			}
			reset_lock.unlock();
			m_Statistics->updateThroughput(m_reportedStatistics.bitsSentInSecond / 1000. / 1000.0);
			if (captureTriggerValue) {
				fs << "[kyl_instantaneousBitrate]: " << std::fixed << std::setprecision(3) << m_Statistics->GetBitsSentInSecond() / 1000. / 1000.0 << std::endl;
				fs << "[kyl_instantaneousThroughput]: " << std::fixed << std::setprecision(3) << m_reportedStatistics.bitsSentInSecond / 1000. / 1000.0 << std::endl;
				fs << "[kyl_ewmaThroughput]: " << std::fixed << std::setprecision(3) << m_Statistics->GetThroughput() << std::endl;
				fs << "[kyl_instantaneousTotalLatency]: " << std::fixed << std::setprecision(3) << sendBuf.serverTotalLatency / 1000.0 << std::endl;
				fs << "[kyl_instantaneousTransportLatency]: " << std::fixed << std::setprecision(3) << m_reportedStatistics.averageTransportLatency / 1000.0 << std::endl;
				// Info("[kyl_instantaneousBitrate]: %.3f", m_Statistics->GetBitsSentInSecond() / 1000. / 1000.0);
				// Info("[kyl_instantaneousThroughput]: %.3f", m_reportedStatistics.bitsSentInSecond / 1000. / 1000.0);
				// Info("[kyl_ewmaThroughput]: %f", m_Statistics->GetThroughput());
				// Info("[kyl_instantaneousTotalLatency]: %.3f", sendBuf.serverTotalLatency / 1000.0);
				// Info("[kyl_instantaneousTransportLatency]: %.3f", m_reportedStatistics.averageTransportLatency / 1000.0);
				if (m_Statistics->GetPacketsSentInSecond() != 0 && m_Statistics->GetPacketsSentInSecond() > m_reportedStatistics.packetsLostInSecond) {
					// Info("[kyl_PacketLossinSecond]: %d", m_reportedStatistics.packetsLostInSecond);
					// Info("[kyl_PacketinSecond]: %d", m_Statistics->GetPacketsSentInSecond());
					// Info("[kyl_PacketLossRate]: %.2f", (static_cast<double>(m_reportedStatistics.packetsLostInSecond) / static_cast<double>(m_Statistics->GetPacketsSentInSecond())) * 100);
					fs << "[kyl_PacketLossRate]: " << std::fixed << std::setprecision(2) << (static_cast<double>(m_reportedStatistics.packetsLostInSecond) / static_cast<double>(m_Statistics->GetPacketsSentInSecond())) * 100 << std::endl;
				}
			}

			m_LastStatisticsUpdate = now;
			m_Statistics->Reset();
		};

		// Continously send statistics info for updating graphs
		// Info("#{ \"id\": \"GraphStatistics\", \"data\": [%llu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f] }#\n",
		// 	Current / 1000,                                                //time
		// 	sendBuf.serverTotalLatency / 1000.0,                           //totalLatency
		// 	m_reportedStatistics.averageSendLatency / 1000.0,              //receiveLatency
		// 	renderTime,                                                    //renderTime
		// 	idleTime,                                                      //idleTime
		// 	waitTime,                                                      //waitTime
		// 	(double)(m_Statistics->GetEncodeLatencyAverage()) / US_TO_MS,  //encodeLatency
		// 	m_reportedStatistics.averageTransportLatency / 1000.0,         //sendLatency
		// 	m_reportedStatistics.averageDecodeLatency / 1000.0,            //decodeLatency
		// 	m_reportedStatistics.idleTime / 1000.0,                        //clientIdleTime
		// 	m_reportedStatistics.fps,                                      //clientFPS
		// 	m_Statistics->GetFPS());                                       //serverFPS

	}
	else if (timeSync->mode == 2) {
		// Calclate RTT
		uint64_t RTT = Current - timeSync->serverTime;
		m_RTT = RTT;
		// Estimated difference between server and client clock
		int64_t TimeDiff = Current - (timeSync->clientTime + RTT / 2);
		m_TimeDiff = TimeDiff;
		Debug("TimeSync: server - client = %lld us RTT = %lld us\n", TimeDiff, RTT);
	}
}

float ClientConnection::GetPoseTimeOffset() {
	return -(double)(m_Statistics->GetTotalLatencyAverage()) / 1000.0 / 1000.0;
}

void ClientConnection::OnFecFailure() {
	Debug("Listener::OnFecFailure()\n");
	// [kyl fec]
	// if (GetTimestampUs() - m_lastFecFailure < CONTINUOUS_FEC_FAILURE) {
	// 	if (m_fecPercentage < MAX_FEC_PERCENTAGE) {
	// 		m_fecPercentage += 5;
	// 	}
	// }
	// [kyl end]
	m_lastFecFailure = GetTimestampUs();
}

std::shared_ptr<Statistics> ClientConnection::GetStatistics() {
	return m_Statistics;
}
