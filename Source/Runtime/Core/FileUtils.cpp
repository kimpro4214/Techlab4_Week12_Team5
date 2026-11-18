#include "EnginePCH.h"
#include "Core/FileUtils.h"

std::filesystem::path FFileUtils::ToNativePath(FStringView Path)
{
	return std::filesystem::path(std::u8string(Path.begin(), Path.end()));
}

FString FFileUtils::ToUTF8String(const std::filesystem::path& Path)
{
	const std::u8string UTF8Path = Path.generic_u8string();
	return FString(UTF8Path.begin(), UTF8Path.end());
}

FString FFileUtils::NormalizePath(const FString& Path)
{
	std::filesystem::path Normalized = ToNativePath(Path).lexically_normal();
	if (Normalized.is_absolute())
	{
		const auto Relative = Normalized.lexically_relative(std::filesystem::current_path());
		if (!Relative.empty() && *Relative.begin() != "..")
		{
			Normalized = Relative;
		}
	}

	return ToUTF8String(Normalized);
}

FString FFileUtils::GetExtension(const FString& Path)
{
	return ToUTF8String(ToNativePath(Path).extension());
}

bool FFileUtils::ReadAllBytes(const FString& Path, TArray<uint8>& OutBytes, FString& OutError, int64 MaxSize) try
{
	OutError.clear();
	std::ifstream File(ToNativePath(Path), std::ios::binary | std::ios::ate);
	const std::streamoff Size = File ? static_cast<std::streamoff>(File.tellg()) : -1;
	if (Size < 0 || Size > MaxSize || Size > std::numeric_limits<int32>::max())
	{
		OutError = "Cannot read file or file exceeds the size limit: " + Path;
		return false;
	}

	TArray<uint8> Bytes;
	Bytes.SetNum(static_cast<int32>(Size));
	File.seekg(0);
	if (Size > 0)
	{
		File.read(reinterpret_cast<char*>(Bytes.GetData()), Size);
	}

	if (!File)
	{
		OutError = "Incomplete file: " + Path;
		return false;
	}

	OutBytes = std::move(Bytes);
	return true;
}
catch (const std::exception& Error)
{
	OutError = FString("Cannot read file: ") + Error.what();
	return false;
}

bool FFileUtils::WriteAllBytesAtomic(const FString& Path, const TArray<uint8>& Bytes, FString& OutError) try
{
	OutError.clear();
	const std::filesystem::path Target = ToNativePath(Path);
	const std::filesystem::path Temporary = Target.wstring() + L".tmp";

	std::ofstream File(Temporary, std::ios::binary | std::ios::trunc);
	if (!Bytes.IsEmpty())
	{
		File.write(reinterpret_cast<const char*>(Bytes.GetData()), Bytes.Num());
	}
	File.close();

	// 쓰기 실패로 기존 파일이 손상되지 않도록 임시 파일을 완성한 뒤 교체한다.
	if (File && MoveFileExW(Temporary.c_str(), Target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
	{
		return true;
	}

	std::error_code Error;
	std::filesystem::remove(Temporary, Error);
	OutError = "Cannot write file: " + Path;
	return false;
}
catch (const std::exception& Error)
{
	OutError = FString("Cannot write file: ") + Error.what();
	return false;
}
