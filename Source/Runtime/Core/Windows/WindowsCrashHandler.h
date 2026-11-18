#pragma once

struct FCrashHandler
{
	static void Install();

	// 콘솔 CauseCrash 명령용. 호출한 자리에서 Access Violation을 일으켜 필터와 덤프를 시험한다
	static void CauseCrash();
};