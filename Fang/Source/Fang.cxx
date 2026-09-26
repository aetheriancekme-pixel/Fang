#include <iostream>
#include <thread>

#include "Driver/Driver.h"
#include "Globals.hxx"
#include "Miscellaneous/Output/Output.h"
#include "Core/Graphics/Graphics.h"
#include "Engine/Engine.h"
#include "Core/Features/Cache/Cache.h"
#include "Core/Features/Cheats/Misc/Misc.h"
#include "Core/Features/Cheats/World/World.h"
#include "Core/Features/Cheats/Aimbot/Aimbot.h"
#include "Core/Features/Cheats/Aimbot/Silent/Silent.h"
#include "Miscellaneous/Protection/FakeStrings.h"
#include "Miscellaneous/Protection/AntiInjection.h"
#include "Miscellaneous/Protection/External/oxorany_include.h"

#include "Auth/skStr.h"


#include <ShlObj.h>   
#pragma comment(lib, "Shell32.lib")

std::string tm_to_readable_time(tm ctx);
static std::time_t string_to_timet(std::string timestamp);
static std::tm timet_to_tm(time_t timestamp);
const std::string compilation_date = (std::string)skCrypt(__DATE__);
const std::string compilation_time = (std::string)skCrypt(__TIME__);
void sessionStatus();

// add ur own keyauth
std::int32_t main(std::int32_t argc, char** argv[])
{


    SetConsoleTitleA(WRAPPER_MARCO("Fang.wtf"));


    static constexpr const char* BINARY_NAME = { "RobloxPlayerBeta.exe" };

    Driver->Find_Process(BINARY_NAME);
    Driver->Attach_Process(BINARY_NAME);
    Driver->Find_Module(BINARY_NAME);

    auto FakeDataModel = Driver->Read<std::uint64_t>(Driver->Get_Module() + Offsets::FakeDataModel::Pointer);
    Globals::Datamodel.Address = Driver->Read<std::uint64_t>(FakeDataModel + Offsets::FakeDataModel::RealDataModel);
    Globals::VisualEngine.Address = Driver->Read<std::uint64_t>(Driver->Get_Module() + Offsets::VisualEngine::Pointer);
    Globals::Players.Address = Globals::Datamodel.Find_First_Child_Of_Class("Players").Address;
	Globals::Workspace.Address = Globals::Datamodel.Find_First_Child_Of_Class("Workspace").Address;
	Globals::Camera.Address = Globals::Workspace.Find_First_Child_Of_Class("Camera").Address;
    auto Lightin = Globals::Datamodel.Find_First_Child_Of_Class("Lighting");
    Globals::Lighting = SDK::Lighting(Lightin.Address);
    // i got lazy and cba to do all of them

    Output::Success(WRAPPER_MARCO("Process Found -> %u", Driver->Get_Process()));
    Output::Success(WRAPPER_MARCO("Attached To Process -> 0x%llx", Driver->Get_Handle()));
    Output::Success("Base Module Address -> 0x%llx", Driver->Get_Module());
	Output::Fang("Datamodel Address -> 0x%llx", Globals::Datamodel);
    Output::Fang("VisualEngine Address -> 0x%llx", Globals::VisualEngine);
    Output::Fang("Players Address -> 0x%llx", Globals::Players);
    Output::Fang("LocalPlayer Address -> 0x%llx", Globals::LocalPlayer);
    Output::Fang("Workspace Address -> 0x%llx", Globals::Workspace);
    Output::Fang("Camera Address -> 0x%llx", Globals::Camera);
    Output::Fang("Lighting Address -> 0x%llx", Globals::Lighting);

	std::thread(Cache::RunService).detach();
    std::thread(World::RunService).detach();
	std::thread(Aimbot::RunService).detach();
	std::thread(Silent::RunService).detach();
 
    auto workspacetoworld = Driver->Read<uintptr_t>(Globals::Datamodel.Find_First_Child_Of_Class("Workspace").Address + 0x3D8);
    Driver->Write<float>(workspacetoworld + 0x658, 200 * 4.f);

    Graphic->Create_Window();
    Graphic->Create_Device();
    Graphic->Create_Imgui();


    for (;;) {

        
        Graphic->Start_Render();
        
        Graphic->Render_Visuals();

        Graphic->Running ? Graphic->Render_Menu() : void();

        Graphic->End_Render();
    }
    std::cout << "aaaa\n";
    std::cin.get();

    return 0;

}


std::string tm_to_readable_time(tm ctx) {
    char buffer[80];

    strftime(buffer, sizeof(buffer), "%a %m/%d/%y %H:%M:%S %Z", &ctx);

    return std::string(buffer);
}

static std::time_t string_to_timet(std::string timestamp) {
    auto cv = strtol(timestamp.c_str(), NULL, 10); // long

    return (time_t)cv;
}

static std::tm timet_to_tm(time_t timestamp) {
    std::tm context;

    localtime_s(&context, &timestamp);

    return context;
}
