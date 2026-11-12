#pragma once

class UWorld;

namespace AnimTest
{
	bool VerifyLuaAnimationBindings(UWorld* World, bool bCheckRuleErrors = false);
}
