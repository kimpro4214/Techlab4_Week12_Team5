-- T1Zeus.scene의 Cinematic(ALevelSequenceActor)에 연결한다. obj는 시퀀스 액터 자신.
-- 번개가 치는 순간마다 카메라를 흔든다. 시간은 T1Zeus.sequence의 Bolt 켜짐 키와 맞춘다.

-- { 시퀀스 시간(초), 세기 }
local STRIKES <const> = {
    { 0.90, 0.25 }, { 1.80, 0.2 }, { 2.40, 0.3 }, { 3.30, 0.2 },
    { 10.90, 0.8 },   -- Zeus 뒤로 떨어지는 마지막 번개
}

local function StrikeShake()
    local now = 0.0
    for _, strike in ipairs(STRIKES) do
        wait(strike[1] - now)
        StartCameraShake(strike[2])
        now = strike[1]
    end
end

-- Zeus가 화면에 다 들어온 뒤(카메라가 멈추는 11.6초 무렵) 하단 레터박스에 환영 문구를 띄운다
local WELCOME_TIME <const> = 11.8

local function ShowWelcome()
    wait(WELCOME_TIME)
    local widget = CreateWidget("Assets/UI/T1Zeus.widget")
    if widget then widget:AddToViewport(10) end
end

function BeginPlay()
    StartCoroutine(StrikeShake)
    StartCoroutine(ShowWelcome)
end
