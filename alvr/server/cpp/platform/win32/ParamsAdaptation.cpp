// [kyl] begin
#include "ParamsAdaptation.h"
#include "alvr_server/Statistics.h"

// using namespace std;
using namespace DirectX;

ParamsAdaptation::ParamsAdaptation() : m_bExiting(false) {}

ParamsAdaptation::~ParamsAdaptation() {}

void ParamsAdaptation::Initialize(std::shared_ptr<ClientConnection> listener) {
    m_Listener = listener;
}

void ParamsAdaptation::Run() {
    while (!m_bExiting) {
        if (m_Listener->GetStatistics()->CheckUpdate()) {
            Info("Encoding settings update");
        }
    }
}

void ParamsAdaptation::Stop() {
    m_bExiting = true;
    Info("stop ParamsAdaptation thread");
    Join();
}
// [kyl] end