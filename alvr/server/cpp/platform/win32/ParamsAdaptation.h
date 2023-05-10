#pragma once
#include "shared/d3drender.h"

#include "shared/threadtools.h"

#include <d3d11.h>
#include <wrl.h>
#include <map>
#include <d3d11_1.h>
#include <wincodec.h>
#include <wincodecsdk.h>
#include "DirectXTex/DirectXTexP.h"
#include "DirectXTex/DirectXTex.h"
#include "alvr_server/ClientConnection.h"
#include "alvr_server/Utils.h"
#include "FrameRender.h"
#include "VideoEncoder.h"
#include "VideoEncoderNVENC.h"
#include "VideoEncoderVCE.h"
#ifdef ALVR_GPL
	#include "VideoEncoderSW.h"
#endif
#include "alvr_server/IDRScheduler.h"

// [kyl] begin
#include "alvr_server/alvr_server.h"
// [kyl] end


	using Microsoft::WRL::ComPtr;

	//----------------------------------------------------------------------------
	// Blocks on reading backbuffer from gpu, so WaitForPresent can return
	// as soon as we know rendering made it this frame.  This step of the pipeline
	// should run about 3ms per frame.
	//----------------------------------------------------------------------------
	class ParamsAdaptation : public CThread
	{
	public:
		ParamsAdaptation();
		~ParamsAdaptation();

		void Initialize(std::shared_ptr<ClientConnection> listener);

		virtual void Run();

		virtual void Stop();

	private:
		CThreadEvent m_newFrameReady, m_encodeFinished;
		std::shared_ptr<ClientConnection> m_Listener;

		bool m_bExiting;
		uint64_t m_targetTimestampNs;
	};

