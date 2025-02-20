// [kyl] begin
#include "OutputFrame.h"

// using namespace std;
using namespace DirectX;
int frame_count = 0;
bool outputframeResetFlag = false;
std::string base_dir;
std::mutex lock;
std::mutex reset_lock;


		OutputFrame::OutputFrame()
			: m_bExiting(false)
			, m_targetTimestampNs(0)
		{
			m_encodeFinished.Set();
		}

		
			OutputFrame::~OutputFrame()
		{
			if (m_videoEncoder)
			{
				m_videoEncoder->Shutdown();
				m_videoEncoder.reset();
			}
		}

		void OutputFrame::Initialize(std::shared_ptr<CD3DRender> d3dRender, std::vector<ID3D11Texture2D*> *frames_vec, std::vector<uint64_t> *timeStamp) {
            frames_vec_ptr = frames_vec;
			timeStamp_ptr = timeStamp;
			m_pD3DRender = d3dRender;

			// read target file path
			// if_read.open("D:/kyl/ALXR_Testbed/alvr/config/testcase.txt");
			// std::getline(if_read ,base_dir);
			// // Info("target dir: %s\n", base_dir);
			// if_read.close();
			reset_log();
		}

		// [kyl] begin
		void OutputFrame::reset_log() {
			// read target file path
			if_read.open("D:/kyl/ALXR_Testbed/alvr/config/testcase.txt");
			std::getline(if_read ,base_dir);
			// Info("OutputFrame target dir: %s\n", base_dir);
			if_read.close();
		}
		// [kyl] end

		void OutputFrame::Run()
		{
			int now_frame = 0;
			int round = 0;
			int value = 0;
			// lock.lock();

            while (!clientShutDown || !(*frames_vec_ptr).empty())
            {
				// lock.unlock();
				if (!resetConfigValue && resetCount == 0) {
					outputframeResetFlag = false;
				}
				reset_lock.lock();
				if (resetConfigValue && !outputframeResetFlag) {
					reset_log();
					if (resetCount == 3) {
						resetCount = 0;
						resetConfigValue = false;
					} 
					else {
						resetCount += 1;
						outputframeResetFlag = true;
					}
					Info("outputframe reset count %d", resetCount);
					Info("outputframe reset flag %d", outputframeResetFlag);
				}
				reset_lock.unlock();
				lock.lock();
                if (!(*frames_vec_ptr).empty() && !(*timeStamp_ptr).empty()) {
					ID3D11Texture2D *texture = frames_vec_ptr->at(0);
					timeStamp = timeStamp_ptr->at(0);
					(*frames_vec_ptr).erase((*frames_vec_ptr).begin());
					(*timeStamp_ptr).erase((*timeStamp_ptr).begin());
					
					ScratchImage img;
					HRESULT hr = CaptureTexture(m_pD3DRender->GetDevice(), m_pD3DRender->GetContext(), texture, img);
					if (FAILED(hr)) {
						Info("[kyl] capture fail");
						lock.unlock();
					}
					else {
						round = (int)(timeStamp/1000);
						value = (int)(timeStamp%1000);
						now_frame = frame_count;
						frame_count++;
						// swprintf_s(filepath, L"frames/%lld_%lld.png", round, value);
						std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    					std::wstring wide_base_dir = converter.from_bytes(base_dir);
						swprintf_s(filepath, L"%s/frames/%d_%d.png", wide_base_dir.c_str(), round, value);
						lock.unlock();
						hr = SaveToWICFile(img.GetImages(), img.GetImageCount(), WIC_FLAGS_NONE, GUID_ContainerFormatPng, filepath, &GUID_WICPixelFormat24bppBGR);
						if (FAILED(hr)) {
							Info("[kyl] failed to save image");
						}	
					}
					texture->Release();
					img.Release();
                }
				else {
					lock.unlock();
				}
				// lock.lock();
            }

			Info("frame_count: %d.", frame_count);
			Info("Output done.");
		}

		void OutputFrame::Stop()
		{
			Info("stop OutputFrame thread");
			Join();
		}
// [kyl] end