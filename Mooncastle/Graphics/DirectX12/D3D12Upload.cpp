#include "D3D12Upload.h"
#include "D3D12Core.h"

namespace mooncastle::graphics::d3D12::upload
{
	namespace
	{
		struct uploadFrame
		{
			ID3D12CommandAllocator*			commandAllocator{ nullptr };
			ID3D12GraphicsCommandList*	    commandList{ nullptr };
			ID3D12Resource*					uploadBuffer{ nullptr };
			void*							cpuAddress{ nullptr };
			u64								fenceValue{ 0 };

			void waitAndReset();

			void release()
			{
				waitAndReset();
				core::release(commandAllocator);
				core::release(commandList);
			}

			constexpr bool isReady() const { return uploadBuffer == nullptr; }
		};

		constexpr u32		noUploadFrame{ 4 };
		uploadFrame		    uploadFrames[noUploadFrame]{};
		ID3D12CommandQueue*	uploadCommandQueue{ nullptr };
		ID3D12Fence1*		uploadFence{ nullptr };
		u64					uploadFenceValue{ 0 };
		HANDLE				fenceEvent{};
		std::mutex			frameMutex{};
		std::mutex			queueMutex{};

		void uploadFrame::waitAndReset()
		{
			assert(uploadFence && fenceEvent);

			if (uploadFence->GetCompletedValue() < fenceValue)
			{
				DXCall(uploadFence->SetEventOnCompletion(fenceValue, fenceEvent));
				WaitForSingleObject(fenceEvent, INFINITE);
			}

			core::release(uploadBuffer);
			cpuAddress = nullptr;
		}

		//Frames should be locked before this function is called.
		u32 getAvailableUploadFrame()
		{
			u32 index{ u32_invalid_id };
			const u32 count{ noUploadFrame };
			uploadFrame* const frames{ &uploadFrames[0] };

			for (u32 i{ 0 }; i < count; ++i)
			{
				if (frames[i].isReady())
				{
					index = i;
					break;
				}
			}

			/*None of the frames were done uploading. 
			We're the only thread here, so we can iterate through the frames until a thread that is ready is found.*/
			if (index == u32_invalid_id)
			{
				index = 0;

				while (!frames[index].isReady())
				{
					index = (index + 1) % count;
					std::this_thread::yield();
				}
			}

			return index;
		}

		bool initFailed()
		{
			shutdown();

			return false;
		}

	}

	D3D12UploadContext::D3D12UploadContext(u32 alignedSize)
	{
		assert(uploadCommandQueue);
		{
			//We don't want to lock this function for longer than necessary.
			std::lock_guard lock{ frameMutex };
			frameIndex = getAvailableUploadFrame();
			assert(frameIndex != u32_invalid_id);

			//Before unlocking, we prevent other threads from picking this frame by making IsReady() return false.
			uploadFrames[frameIndex].uploadBuffer = (ID3D12Resource*)1;
		}

		uploadFrame& frame{ uploadFrames[frameIndex] };
		frame.uploadBuffer = d3DX::createBuffer(alignedSize, nullptr, true);

		NAME_D3D12_OBJECT_INDEXED(frame.uploadBuffer, alignedSize, L"Upload Buffer - Size");

		const D3D12_RANGE range{};
		DXCall(frame.uploadBuffer->Map(0, &range, reinterpret_cast<void**>(&frame.cpuAddress)));
		assert(frame.cpuAddress);

		cmdList = frame.commandList;
		uploadBuffer = frame.uploadBuffer;
		cpuAddress = frame.cpuAddress;
		assert(cmdList && uploadBuffer && cpuAddress);

		DXCall(frame.commandAllocator->Reset());
		DXCall(frame.commandList->Reset(frame.commandAllocator, nullptr));
	}

	void D3D12UploadContext::endUpload()
	{
		assert(frameIndex != u32_invalid_id);

		uploadFrame& frame{ uploadFrames[frameIndex] };
		ID3D12GraphicsCommandList* const commandList{ frame.commandList };
		DXCall(commandList->Close());

		std::lock_guard lock{ queueMutex };

		ID3D12CommandList* const commandLists[]{ commandList };
		ID3D12CommandQueue* const commandQueue{ uploadCommandQueue };
		commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		++uploadFenceValue;
		frame.fenceValue = uploadFenceValue;
		DXCall(commandQueue->Signal(uploadFence, frame.fenceValue));

		//Wait for the copy queue to finish and then release the upload buffer.
		frame.waitAndReset();

		//This instance of upload context is now expired.
		DEBUG_OP(new (this) D3D12UploadContext);
	}

	bool initialize()
	{
		ID3D12Device* const device{ core::device() };
		assert(device && !uploadCommandQueue);

		HRESULT hr{ S_OK };

		for (u32 i{ 0 }; i < noUploadFrame; ++i)
		{
			uploadFrame& frame{ uploadFrames[i] };

			DXCall(hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&frame.commandAllocator)));
			if (FAILED(hr)) return initFailed();

			DXCall(hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, frame.commandAllocator, nullptr, IID_PPV_ARGS(&frame.commandList)));
			if (FAILED(hr)) return initFailed();

			DXCall(frame.commandList->Close());

			NAME_D3D12_OBJECT_INDEXED(frame.commandAllocator, i, L"Upload Command Allocator");
			NAME_D3D12_OBJECT_INDEXED(frame.commandList, i, L"Upload Command List");
		}

		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

		DXCall(hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&uploadCommandQueue)));
		if (FAILED(hr)) return initFailed();
		NAME_D3D12_OBJECT(uploadCommandQueue, L"Upload Copy Queue");

		DXCall(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence)));
		if (FAILED(hr)) return initFailed();
		NAME_D3D12_OBJECT(uploadFence, L"Upload Fence");

		fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		assert(fenceEvent);

		if (!fenceEvent) return initFailed();

		return true;
	}

	void shutdown()
	{
		for (u32 i{ 0 }; i < noUploadFrame; ++i)
		{
			uploadFrames[i].release();
		}

		if (fenceEvent)
		{
			CloseHandle(fenceEvent);
			fenceEvent = nullptr;
		}

		core::release(uploadCommandQueue);
		core::release(uploadFence);
		uploadFenceValue = 0;
	}
}