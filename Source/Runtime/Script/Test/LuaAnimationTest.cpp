#include "EnginePCH.h"
#include "LuaAnimationTest.h"
#include "Script/LuaBindings.h"
#include "Script/LuaActorRef.h"
#include "Engine/World.h"
#include "Engine/Classes/Animation/AnimStateMachine.h"
#include <sol/sol.hpp>

#if defined(ENGINE_DEBUG)
namespace
{
	bool RunLuaCheck(sol::state& Lua, sol::environment& Env, const char* Code)
	{
		auto Result = Lua.safe_script(Code, Env, sol::script_pass_on_error);
		if (Result.valid()) return true;
		sol::error Error = Result;
		HTR_LOG(Error, "[LuaAnimTest] {}", Error.what());
		return false;
	}

	struct FTestCleanup
	{
		UWorld* World;
		AActor*& First;
		AActor*& Second;
		sol::state& Lua;
		~FTestCleanup()
		{
			FLuaBindings::ReleaseAnimationStateMachines(Lua);
			if (First) World->DestroyActor(First);
			if (Second) World->DestroyActor(Second);
		}
	};
}

bool AnimTest::VerifyLuaAnimationBindings(UWorld* World, bool bCheckRuleErrors)
{
	if (!World) return false;
	sol::state Lua;
	Lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string);
	FLuaBindings::RegisterAnimation(Lua);
	AActor* First = World->SpawnActor<AActor>(FName("LuaAnimationTestFirst"));
	AActor* Second = World->SpawnActor<AActor>(FName("LuaAnimationTestSecond"));
	FTestCleanup Cleanup{ World, First, Second, Lua };
	if (!First || !Second) return false;
	sol::environment Env(Lua, sol::create, Lua.globals());
	sol::environment OtherEnv(Lua, sol::create, Lua.globals());
	Env["obj"] = FLuaActorRef::Make(First);
	OtherEnv["obj"] = FLuaActorRef::Make(Second);
	const char* BuildCode = R"lua(
attackRequested = false
hitRequested = false
function Build()
    sm = CreateAnimStateMachine()
    locomotion = sm:AddState("Locomotion")
    attack = sm:AddState("Attack")
    hit = sm:AddState("Hit")
    -- 코루틴이 수거된 뒤에도 등록된 조건을 메인 스레드에서 호출할 수 있어야 한다.
    local co = coroutine.create(function()
        assert(sm:AddTransition(locomotion, attack, function() return attackRequested end))
    end)
    assert(coroutine.resume(co))
    assert(sm:AddTransition(attack, locomotion, function() return sm:GetStateElapsedTime() >= 1 end))
    assert(sm:AddAnyStateTransition(hit, function() return hitRequested end))
    assert(sm:AddTransition(hit, locomotion, function() return sm:GetStateElapsedTime() >= 1 end))
    assert(sm:Start(locomotion))
end
function Step(dt)
    sm:Update(dt)
    attackRequested = false
    hitRequested = false
    return sm:GetCurrentStateName(), sm:GetStateElapsedTime()
end
Build()
collectgarbage("collect")
)lua";
	if (!RunLuaCheck(Lua, Env, BuildCode) || !RunLuaCheck(Lua, OtherEnv, BuildCode)) return false;
	bool bAttack = false, bHit = false;
	FAnimStateMachine Native;
	const int32 Locomotion = Native.AddState(FName("Locomotion"));
	const int32 Attack = Native.AddState(FName("Attack"));
	const int32 Hit = Native.AddState(FName("Hit"));
	Native.AddTransition(Locomotion, Attack, [&bAttack]() { return bAttack; });
	Native.AddTransition(Attack, Locomotion, [&Native]() { return Native.GetStateElapsedTime() >= 1.0f; });
	Native.AddAnyStateTransition(Hit, [&bHit]() { return bHit; });
	Native.AddTransition(Hit, Locomotion, [&Native]() { return Native.GetStateElapsedTime() >= 1.0f; });
	Native.Start(Locomotion);
	sol::protected_function Step = Env["Step"];
	auto CompareStep = [&](float Dt, bool bAttackInput, bool bHitInput)
	{
		bAttack = bAttackInput;
		bHit = bHitInput;
		Env["attackRequested"] = bAttackInput;
		Env["hitRequested"] = bHitInput;
		Native.Update(Dt);
		auto Result = Step(Dt);
		const bool bMatches = Result.valid()
			&& Result.get<FString>(0) == Native.GetCurrentStateName().ToString()
			&& Result.get<float>(1) == Native.GetStateElapsedTime();
		if (!bMatches) HTR_LOG(Error, "[LuaAnimTest] Native/Lua transition mismatch");
		return bMatches;
	};
	if (!CompareStep(0.25f, false, false) || !CompareStep(0.25f, true, false)
		|| !CompareStep(0.5f, false, false) || !CompareStep(0.5f, false, false)
		|| !CompareStep(0.25f, true, true) || !CompareStep(0.25f, false, true)
		|| !CompareStep(0.75f, false, false) || !CompareStep(0.25f, true, false)
		|| !CompareStep(0.25f, false, true)) return false;
	Native.SetStateRestartOnReentry(Hit, true);
	if (!RunLuaCheck(Lua, Env, "assert(sm:SetStateRestartOnReentry(hit, true))")
		|| !CompareStep(0.25f, false, true) || !CompareStep(0.5f, false, false)
		|| !CompareStep(0.5f, false, false)) return false;
	if (!RunLuaCheck(Lua, OtherEnv, "assert(sm:GetCurrentStateName() == 'Locomotion' and sm:GetStateElapsedTime() == 0)")) return false;

	if (!RunLuaCheck(Lua, Env, R"lua(
old = sm
Build()
assert(not old:IsValid() and sm:IsValid())
assert(not pcall(function() old:Update(0.1) end))
sm = CreateAnimStateMachine()
local a = sm:AddState("A")
local b = sm:AddState("B")
assert(sm:AddTransition(a, b, function()
    assert(not pcall(function() sm:Update(0.1) end))
    local ok, message = pcall(function() return CreateAnimStateMachine() end)
    assert(not ok and string.find(message, "Cannot replace", 1, true))
    return true
end))
assert(not pcall(function() sm:AddTransition(a, b, nil) end))
assert(sm:Start(a) and sm:Update(0.1) and sm:GetCurrentState() == b)
assert(not pcall(function() sm:Update(-0.1) end))
assert(not pcall(function() sm:Update(0 / 0) end))
)lua")) return false;

	if (bCheckRuleErrors && !RunLuaCheck(Lua, Env, R"lua(
for _, rule in ipairs({function() error("intentional rule failure") end, function() return "invalid" end}) do
    sm = CreateAnimStateMachine()
    local a = sm:AddState("A")
    local b = sm:AddState("B")
    assert(sm:AddTransition(a, b, rule) and sm:Start(a))
    assert(not sm:Update(0.1) and sm:GetCurrentState() == a)
end
)lua")) return false;
	FLuaBindings::ReleaseCallbacks(First);
	if (!RunLuaCheck(Lua, Env, "assert(not sm:IsValid()); Build(); assert(sm:IsValid())")
		|| !RunLuaCheck(Lua, OtherEnv, "assert(sm:IsValid())")) return false;
	World->DestroyActor(First);
	First = nullptr;
	if (!RunLuaCheck(Lua, Env, "assert(not sm:IsValid()); assert(not pcall(function() sm:GetCurrentState() end))")) return false;
	FLuaBindings::ReleaseAnimationStateMachines(Lua);
	if (!RunLuaCheck(Lua, OtherEnv, "assert(not sm:IsValid())")) return false;
	HTR_LOG(Info, "[LuaAnimTest] Native/Lua transitions, lifetime and reentry checks passed");
	return true;
}
#endif
