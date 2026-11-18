#include "EnginePCH.h"
#include "SoundWave.h"
#include "Audio/MiniaudioInclude.h"
#include "Core/StringUtils.h"

bool USoundWave::LoadFromFile(const FString& Path)
{
	FWideString FilePath;
	if (!StringUtils::TryUtf8ToWide(Path, FilePath))
	{
		HTR_LOG(Error, "[Audio] Invalid UTF-8 path: {}", Path);
		return false;
	}

	// 채널 0 = 원본 유지. 3채널 이상이면 스테레오로 다시 연다.
	ma_decoder_config Config = ma_decoder_config_init(ma_format_f32, 0, 0);
	ma_decoder Decoder;
	if (ma_decoder_init_file_w(FilePath.c_str(), &Config, &Decoder) != MA_SUCCESS)
	{
		HTR_LOG(Error, "[Audio] Cannot decode {}", Path);
		return false;
	}
	if (Decoder.outputChannels > 2)
	{
		ma_decoder_uninit(&Decoder);
		Config = ma_decoder_config_init(ma_format_f32, 2, 0);
		if (ma_decoder_init_file_w(FilePath.c_str(), &Config, &Decoder) != MA_SUCCESS) return false;
	}

	NumChannels = Decoder.outputChannels;
	SampleRate = Decoder.outputSampleRate;

	// MP3 등은 길이가 추정치라 청크로 끝까지 읽는다.
	constexpr ma_uint64 ChunkFrames = 16384;
	while(true)
	{
		const int32 OldNum = PCMData.Num();
		PCMData.SetNum(OldNum + static_cast<int32>(ChunkFrames * NumChannels));
		ma_uint64 Read = 0;
		ma_decoder_read_pcm_frames(&Decoder, PCMData.GetData() + OldNum, ChunkFrames, &Read);
		PCMData.SetNum(OldNum + static_cast<int32>(Read * NumChannels));
		if (Read < ChunkFrames) break;
	}
	ma_decoder_uninit(&Decoder);

	NumFrames = PCMData.Num() / NumChannels;
	Duration = static_cast<float>(NumFrames) / static_cast<float>(SampleRate);
	SourcePath = Path;
	return NumFrames > 0;
}
