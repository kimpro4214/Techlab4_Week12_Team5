#pragma once

#include "Core/EngineString.h"

namespace StringUtils
{
	// UTF-8과 Windows UTF-16을 변환한다. 빈 입력은 성공하며 출력도 비운다.
	// 잘못된 인코딩이나 길이 초과 시 false를 반환하고 기존 출력은 유지한다.
	// 중간 NUL은 보존한다. 파일 경로에서의 허용 여부는 호출부가 별도로 검사한다.
	bool TryUtf8ToWide(const FString& Input, FWideString& Output);
	bool TryWideToUtf8(const FWideString& Input, FString& Output);
}
