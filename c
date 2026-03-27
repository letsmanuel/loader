   -- =====================
-- INSTANT PATCH - absolute first thing, no yields
-- =====================
local Players = game:GetService("Players")
local RS = game:GetService("ReplicatedStorage")

-- =====================
-- CONFIG FLAGS - edit before deploying
-- =====================
local AUTO_START = true
local INSTANT_ACTIVATE = true  -- skip GUI entirely, just boot

local ALWAYS_DISABLED = {
    "antiCheatController"
    -- "phone",
    -- "flashbang",
}

-- =====================
local alwaysDisabledSet = {}
for _, name in ipairs(ALWAYS_DISABLED) do
    alwaysDisabledSet[name] = true
end

local targets = {"main", "preloader", "flamework"}
local patched = {}

local function tryPatch(s)
    for _, name in ipairs(targets) do
        if s.Name == name and not patched[name] then
            s.Disabled = true
            patched[name] = true
            print("[Patcher] Patched:", name)
        end
    end
end

local function onLocalPlayer(LocalPlayer)
    local function onPlayerScripts(PlayerScripts)
        local function onCode(Code)
            local function onScripts(scriptsFolder)
                for _, s in ipairs(scriptsFolder:GetChildren()) do tryPatch(s) end
                scriptsFolder.ChildAdded:Connect(tryPatch)
            end
            local scripts = Code:FindFirstChild("scripts")
            if scripts then onScripts(scripts)
            else Code.ChildAdded:Connect(function(c)
                if c.Name == "scripts" then onScripts(c) end
            end) end
        end
        local code = PlayerScripts:FindFirstChild("Code")
        if code then onCode(code)
        else PlayerScripts.ChildAdded:Connect(function(c)
            if c.Name == "Code" then onCode(c) end
        end) end
    end
    local ps = LocalPlayer:FindFirstChild("PlayerScripts")
    if ps then onPlayerScripts(ps)
    else LocalPlayer.ChildAdded:Connect(function(c)
        if c.Name == "PlayerScripts" then onPlayerScripts(c) end
    end) end
end

local lp = Players.LocalPlayer
if lp then onLocalPlayer(lp)
else Players:GetPropertyChangedSignal("LocalPlayer"):Connect(function()
    onLocalPlayer(Players.LocalPlayer)
end) end

if not game:IsLoaded() then game.Loaded:Wait() end

-- =====================
-- SETUP
-- =====================
local LocalPlayer = Players.LocalPlayer
local psCode = LocalPlayer:WaitForChild("PlayerScripts"):WaitForChild("Code")
local scriptsPath = psCode:WaitForChild("scripts")

print("[Boot] Patched:", patched.main, patched.preloader, patched.flamework)

if INSTANT_ACTIVATE then
    print("[Boot] INSTANT_ACTIVATE — skipping GUI")

    local stashed = {}
    local function stashModule(ms)
        local originalParent = ms.Parent
        ms.Parent = nil
        table.insert(stashed, {ms, originalParent})
        print("[INSTANT] Stashed:", ms.Name)
    end

    local function findByName(parent, name)
        for _, child in ipairs(parent:GetDescendants()) do
            if child.Name == name then return child end
        end
        return nil
    end

    local function bootDirect()
        for _, name in ipairs(ALWAYS_DISABLED) do
            local s = findByName(psCode, name)
            if s then
                if s:IsA("ModuleScript") then
                    stashModule(s)
                elseif s:IsA("LocalScript") then
                    s.Disabled = true
                    print("[INSTANT] Disabled LocalScript:", name)
                end
            else
                print("[INSTANT] Not found:", name)
            end
        end

        local preloader = scriptsPath:WaitForChild("preloader")
        local main = scriptsPath:WaitForChild("main")
        local flamework = scriptsPath:WaitForChild("flamework")
        preloader.Disabled = false task.wait()
        main.Disabled = false task.wait()
        flamework.Disabled = false
        print("[INSTANT] Flamework ignited ✅")
        game:GetService("StarterGui"):SetCore("SendNotification", {
            Title = "Vortex",
            Text = "Launched game without local Anticheat.",
            Duration = 5,
            Icon = "rbxassetid://4483345998"
        })
    end

    local ok, err = pcall(bootDirect)
    if not ok then print("[INSTANT ERROR]", err) end
    return
end

local UI = loadstring(game:HttpGet("https://api.getvortex.vip/scripts/OrionLib"))()

local Window = UI:MakeWindow({
    Name = "Vortex Testing - LETSMANUEL",
    HidePremium = false,
    SaveConfig = false,
    ConfigFolder = "vortex-testing.taser.faafo3"
})

local FlameworkStarted = false
local FlameworkRef = nil
local coreNames = {main = true, preloader = true, flamework = true}

local blacklist = {}
local stashed = {}

local function stashModule(ms)
    local originalParent = ms.Parent
    ms.Parent = nil
    table.insert(stashed, {ms, originalParent})
    print("[Vortex] Stashed:", ms.Name)
end


local function restoreAll()
    for _, entry in ipairs(stashed) do
        entry[1].Parent = entry[2]
    end
    stashed = {}
    print("[Vortex] All stashed modules restored")
end

local function bootFlamework()
    local ok, err = pcall(function()
        local preloader = scriptsPath:FindFirstChild("preloader")
        local main = scriptsPath:FindFirstChild("main")
        local flamework = scriptsPath:FindFirstChild("flamework")
        if not preloader or not main or not flamework then
            error("One or more core scripts not found")
        end

        for s, shouldDisable in pairs(blacklist) do
            if shouldDisable then
                if s:IsA("ModuleScript") then
                    stashModule(s)
                elseif s:IsA("LocalScript") then
                    s.Disabled = true
                    print("[Vortex] Disabled LocalScript:", s.Name)
                end
            end
        end

        preloader.Disabled = false
        task.wait()
        main.Disabled = false
        task.wait()
        flamework.Disabled = false
        FlameworkStarted = true
    end)
    return ok, err
end

-- =====================
-- COLLECT ALL SCRIPTS
-- =====================
local function collectAll(folder)
    local results = {}
    local function recurse(f, path)
        for _, child in ipairs(f:GetChildren()) do
            local childPath = path .. "/" .. child.Name
            if child:IsA("LocalScript") or child:IsA("ModuleScript") then
                if not (child.Parent == scriptsPath and coreNames[child.Name]) then
                    table.insert(results, {
                        script = child,
                        path = childPath,
                        folder = child.Parent.Name
                    })
                end
            end
            recurse(child, childPath)
        end
    end
    recurse(folder, "")
    return results
end

local allScripts = collectAll(psCode)

-- Apply ALWAYS_DISABLED defaults
for _, entry in ipairs(allScripts) do
    local s = entry.script
    if alwaysDisabledSet[s.Name] then
        blacklist[s] = true
        print("[Config] Always disabled:", s.Name)
    else
        blacklist[s] = false
    end
end

-- Collect unique folder names for filter toggles
local folderNames = {}
local folderSet = {}
for _, entry in ipairs(allScripts) do
    if not folderSet[entry.folder] then
        folderSet[entry.folder] = true
        table.insert(folderNames, entry.folder)
    end
end
table.sort(folderNames)

-- =====================
-- TABS
-- =====================
local TabPatch     = Window:MakeTab({Name = "Patcher",   Icon = "rbxassetid://4483345998", PremiumOnly = false})
local TabScripts   = Window:MakeTab({Name = "Scripts",   Icon = "rbxassetid://4483345998", PremiumOnly = false})
local TabFlamework = Window:MakeTab({Name = "Flamework", Icon = "rbxassetid://4483345998", PremiumOnly = false})
local TabInspect   = Window:MakeTab({Name = "Inspector", Icon = "rbxassetid://4483345998", PremiumOnly = false})

-- =====================
-- PATCHER TAB
-- =====================
TabPatch:AddSection({Name = "Core Script Status"})
for _, name in ipairs(targets) do
    TabPatch:AddLabel(name .. ": " .. (patched[name] and "✅ Disabled" or "❌ Not Found"))
end
TabPatch:AddSection({Name = "Actions"})
TabPatch:AddButton({
    Name = "Re-Patch All",
    Callback = function()
        local allOk = true
        for _, name in ipairs(targets) do
            local s = scriptsPath:FindFirstChild(name)
            if s then s.Disabled = true patched[name] = true
            else allOk = false end
        end
        UI:MakeNotification({
            Name = "Patcher",
            Content = allOk and "All 3 scripts disabled." or "Some scripts were not found.",
            Image = "rbxassetid://4483345998",
            Time = 3
        })
    end
})
TabPatch:AddButton({
    Name = "Check Script States",
    Callback = function()
        for _, name in ipairs(targets) do
            local s = scriptsPath:FindFirstChild(name)
            if s then
                print("[Patcher]", name, "| Disabled:", s.Disabled, "| Class:", s.ClassName)
            else
                print("[Patcher]", name, "| NOT FOUND")
            end
        end
        UI:MakeNotification({Name = "Patcher", Content = "Printed to console.", Image = "rbxassetid://4483345998", Time = 2})
    end
})

-- =====================
-- SCRIPTS TAB
-- =====================

-- State
local searchQuery = ""
local visibleFolders = {}
for _, name in ipairs(folderNames) do
    visibleFolders[name] = true -- all shown by default
end

-- We'll store toggle UI objects so we can show/hide them
-- Orion doesn't support hiding elements, so we track visibility via search/folder filter
-- and print a filtered list on demand instead — toggles are always shown but labeled clearly

TabScripts:AddSection({Name = "Search"})
TabScripts:AddTextbox({
    Name = "Search scripts",
    Default = "",
    TextDisappear = false,
    Callback = function(val)
        searchQuery = val:lower()
        print("[Scripts] Search:", searchQuery)
        -- Print filtered list to console since Orion can't hide toggles
        print("[Scripts] Matching scripts:")
        for _, entry in ipairs(allScripts) do
            if entry.script.Name:lower():find(searchQuery, 1, true) then
                local state = blacklist[entry.script] and "❌ disabled" or "✅ enabled"
                print("  -", entry.script.Name, "|", entry.folder, "|", state)
            end
        end
    end
})

TabScripts:AddSection({Name = "Folder Filter"})
for _, folderName in ipairs(folderNames) do
    TabScripts:AddToggle({
        Name = "Show: " .. folderName,
        Default = true,
        Callback = function(val)
            visibleFolders[folderName] = val
            -- Print current filtered view
            print("[Scripts] Folder filter updated. Visible scripts:")
            for _, entry in ipairs(allScripts) do
                if visibleFolders[entry.folder] then
                    local state = blacklist[entry.script] and "❌ disabled" or "✅ enabled"
                    print("  -", entry.script.Name, "|", entry.folder, "|", state)
                end
            end
        end
    })
end

TabScripts:AddSection({Name = "Script Toggles"})
TabScripts:AddLabel("Toggle OFF to blacklist. [M] = ModuleScript  [L] = LocalScript")
TabScripts:AddLabel("Always-disabled scripts (from config) start toggled OFF.")

local sections = {}
for _, entry in ipairs(allScripts) do
    local parentName = entry.folder
    if not sections[parentName] then
        sections[parentName] = true
        TabScripts:AddSection({Name = parentName})
    end
    local tag = entry.script:IsA("ModuleScript") and " [M]" or " [L]"
    local alwaysOff = alwaysDisabledSet[entry.script.Name] == true
    TabScripts:AddToggle({
        Name = entry.script.Name .. tag .. (alwaysOff and " ⚠️" or ""),
        Default = not alwaysOff,
        Callback = function(val)
            blacklist[entry.script] = not val
            print("[Scripts]", entry.script.Name, val and "→ enabled" or "→ blacklisted")
        end
    })
end

TabScripts:AddSection({Name = "Search Result Actions"})
TabScripts:AddButton({
    Name = "Disable All Matching Search",
    Callback = function()
        if searchQuery == "" then
            UI:MakeNotification({Name = "Scripts", Content = "No search query set.", Image = "rbxassetid://4483345998", Time = 2})
            return
        end
        local count = 0
        for _, entry in ipairs(allScripts) do
            if entry.script.Name:lower():find(searchQuery, 1, true) then
                blacklist[entry.script] = true
                count += 1
            end
        end
        UI:MakeNotification({Name = "Scripts", Content = count .. " scripts blacklisted.", Image = "rbxassetid://4483345998", Time = 3})
        print("[Scripts] Bulk disabled", count, "scripts matching:", searchQuery)
    end
})
TabScripts:AddButton({
    Name = "Enable All Matching Search",
    Callback = function()
        if searchQuery == "" then
            UI:MakeNotification({Name = "Scripts", Content = "No search query set.", Image = "rbxassetid://4483345998", Time = 2})
            return
        end
        local count = 0
        for _, entry in ipairs(allScripts) do
            if entry.script.Name:lower():find(searchQuery, 1, true) then
                blacklist[entry.script] = false
                count += 1
            end
        end
        UI:MakeNotification({Name = "Scripts", Content = count .. " scripts enabled.", Image = "rbxassetid://4483345998", Time = 3})
        print("[Scripts] Bulk enabled", count, "scripts matching:", searchQuery)
    end
})

TabScripts:AddSection({Name = "Launch"})
TabScripts:AddButton({
    Name = "🚀 Launch Flamework with these settings",
    Callback = function()
        if FlameworkStarted then
            UI:MakeNotification({Name = "Scripts", Content = "Already running!", Image = "rbxassetid://4483345998", Time = 2})
            return
        end
        local count = 0
        for _, v in pairs(blacklist) do if v then count += 1 end end
        local ok, err = bootFlamework()
        if ok then
            UI:MakeNotification({
                Name = "Flamework",
                Content = "Ignited! " .. count .. " scripts disabled.",
                Image = "rbxassetid://4483345998",
                Time = 4
            })
        else
            UI:MakeNotification({
                Name = "Flamework",
                Content = "Error: " .. tostring(err),
                Image = "rbxassetid://4483345998",
                Time = 6
            })
            print("[Flamework Boot Error]", err)
        end
    end
})

-- =====================
-- FLAMEWORK TAB
-- =====================
TabFlamework:AddSection({Name = "Boot Control"})
TabFlamework:AddLabel("Boot without any blacklist.")
TabFlamework:AddButton({
    Name = "🚀 Start Flamework (no blacklist)",
    Callback = function()
        if FlameworkStarted then
            UI:MakeNotification({Name = "Flamework", Content = "Already running!", Image = "rbxassetid://4483345998", Time = 2})
            return
        end
        for k in pairs(blacklist) do blacklist[k] = false end
        local ok, err = bootFlamework()
        if ok then
            UI:MakeNotification({Name = "Flamework", Content = "Ignited successfully ✅", Image = "rbxassetid://4483345998", Time = 3})
        else
            UI:MakeNotification({Name = "Flamework", Content = "Error: " .. tostring(err), Image = "rbxassetid://4483345998", Time = 6})
            print("[Flamework Boot Error]", err)
        end
    end
})
TabFlamework:AddSection({Name = "Registered Paths"})
TabFlamework:AddLabel("controllers  →  StarterPlayerScripts/Code/controllers")
TabFlamework:AddLabel("components  →  StarterPlayerScripts/Code/components")
TabFlamework:AddLabel("weather       →  StarterPlayerScripts/Code/modules/weather")
TabFlamework:AddLabel("RS comps    →  ReplicatedStorage/Code/components")

-- =====================
-- INSPECTOR TAB
-- =====================
TabInspect:AddSection({Name = "Flamework"})
TabInspect:AddButton({
    Name = "Dump Flamework Keys",
    Callback = function()
        if not FlameworkRef then
            UI:MakeNotification({Name = "Inspector", Content = "Flamework not started yet.", Image = "rbxassetid://4483345998", Time = 3})
            return
        end
        local keys = {}
        for k in pairs(FlameworkRef) do table.insert(keys, tostring(k)) end
        table.sort(keys)
        print("[Inspector] Flamework keys (" .. #keys .. "):")
        for _, k in ipairs(keys) do print("  -", k, ":", type(FlameworkRef[k])) end
        UI:MakeNotification({Name = "Inspector", Content = #keys .. " keys printed.", Image = "rbxassetid://4483345998", Time = 3})
    end
})
TabInspect:AddButton({
    Name = "Dump _lifecycleHooks",
    Callback = function()
        if not FlameworkRef then
            UI:MakeNotification({Name = "Inspector", Content = "Flamework not started.", Image = "rbxassetid://4483345998", Time = 2})
            return
        end
        local hooks = FlameworkRef._lifecycleHooks or FlameworkRef.lifecycleHooks
        if hooks then
            for k, v in pairs(hooks) do print("[LifecycleHook]", k, "->", v) end
        else
            print("[Inspector] No _lifecycleHooks found")
        end
        UI:MakeNotification({Name = "Inspector", Content = "Printed to console.", Image = "rbxassetid://4483345998", Time = 2})
    end
})
TabInspect:AddSection({Name = "Stash"})
TabInspect:AddButton({
    Name = "Restore All Stashed Modules",
    Callback = function()
        restoreAll()
        UI:MakeNotification({Name = "Inspector", Content = "All stashed modules restored.", Image = "rbxassetid://4483345998", Time = 3})
    end
})
TabInspect:AddButton({
    Name = "List Stashed Modules",
    Callback = function()
        if #stashed == 0 then
            print("[Inspector] No stashed modules.")
        else
            print("[Inspector] Stashed modules (" .. #stashed .. "):")
            for _, entry in ipairs(stashed) do
                print("  -", entry[1].Name, "| original parent:", entry[2] and entry[2]:GetFullName() or "nil")
            end
        end
        UI:MakeNotification({Name = "Inspector", Content = #stashed .. " stashed.", Image = "rbxassetid://4483345998", Time = 2})
    end
})
TabInspect:AddSection({Name = "Player"})
TabInspect:AddButton({
    Name = "Print LocalPlayer Info",
    Callback = function()
        print("[Inspector] Name:", LocalPlayer.Name)
        print("[Inspector] UserId:", LocalPlayer.UserId)
        print("[Inspector] Team:", LocalPlayer.Team and LocalPlayer.Team.Name or "None")
        print("[Inspector] Character:", LocalPlayer.Character and LocalPlayer.Character.Name or "None")
        UI:MakeNotification({Name = "Inspector", Content = "Player info printed.", Image = "rbxassetid://4483345998", Time = 2})
    end
})
TabInspect:AddSection({Name = "ReplicatedStorage"})
TabInspect:AddButton({
    Name = "Dump RS/Code Children",
    Callback = function()
        print("[Inspector] ReplicatedStorage.Code children:")
        for _, child in ipairs(RS.Code:GetChildren()) do
            print("  -", child.Name, "|", child.ClassName)
        end
        UI:MakeNotification({Name = "Inspector", Content = "Dumped to console.", Image = "rbxassetid://4483345998", Time = 2})
    end
})
TabInspect:AddButton({
    Name = "Dump node_modules",
    Callback = function()
        local nm = RS.Code.rbxts_include.node_modules
        print("[Inspector] node_modules:")
        for _, child in ipairs(nm:GetChildren()) do
            print("  -", child.Name)
            for _, sub in ipairs(child:GetChildren()) do
                print("      -", sub.Name, "|", sub.ClassName)
            end
        end
        UI:MakeNotification({Name = "Inspector", Content = "Dumped to console.", Image = "rbxassetid://4483345998", Time = 2})
    end
})

-- =====================
-- INIT NOTIFICATION + AUTO START
-- =====================
local patchSummary = ""
for _, name in ipairs(targets) do
    patchSummary = patchSummary .. name .. ": " .. (patched[name] and "✅" or "❌") .. "  "
end

local alwaysCount = #ALWAYS_DISABLED
UI:MakeNotification({
    Name = "Vortex Ready",
    Content = patchSummary .. "| " .. #allScripts .. " scripts | " .. alwaysCount .. " auto-disabled",
    Image = "rbxassetid://4483345998",
    Time = 6
})

if AUTO_START then
    print("[Boot] AUTO_START enabled, igniting...")
    local ok, err = bootFlamework()
    if ok then
        UI:MakeNotification({Name = "Flamework", Content = "Auto-ignited ✅", Image = "rbxassetid://4483345998", Time = 3})
        UI:Destroy()
    else
        UI:MakeNotification({Name = "Flamework", Content = "Auto-ignite failed: " .. tostring(err), Image = "rbxassetid://4483345998", Time = 6})
        print("[Auto-Start Error]", err)
    end
end
