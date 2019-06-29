#include "MarioMaker.hpp"
#include <decrypt.h>
#include <iostream>
#include <sys/stat.h>
#include <string.h>
#include <iomanip>

u128 askForUser()
{
    AppletHolder aph;
    AppletStorage hast1;
    LibAppletArgs args;
    appletCreateLibraryApplet(&aph, AppletId_playerSelect, LibAppletMode_AllForeground);
    libappletArgsCreate(&args, 0);
    libappletArgsPush(&args, &aph);
    appletCreateStorage(&hast1, 0xa0);
    u8 indata[0xa0] = { 0 };
    indata[0x96] = 1;
    appletStorageWrite(&hast1, 0, indata, 0xa0);
    appletHolderPushInData(&aph, &hast1);
    appletHolderStart(&aph);
    appletStorageClose(&hast1);
    while(appletHolderWaitInteractiveOut(&aph));
    appletHolderJoin(&aph);
    AppletStorage ost;
    appletHolderPopOutData(&aph, &ost);
    u8 out[24] = { 0 };
    appletStorageRead(&ost, 0, out, 24);
    appletStorageClose(&ost);
    appletHolderClose(&aph);
    return *(u128*)&out[8];
}

MarioMaker::MarioMaker()
{
    u128 userid = askForUser();
    Result rc = accountInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);
    
    AccountProfile accP;
    AccountProfileBase userProfile;
    accountGetProfile(&accP, userid);
    accountProfileGet(&accP, NULL, &userProfile);
    accountProfileLoadImage(&accP, &this->profileIcon, 64000, &this->profileSize);
    this->username = userProfile.username;

    if(userid == 0)
    {
        this->failResult = -1;
        return;
    }
    rc = fsMount_SaveData(&smm2fs, 0x01009B90006DC000, userid);
    if (R_FAILED(rc))
    {
        this->failResult = rc;
        return;
    }
    fsdevMountDevice("smm2save", smm2fs);
}

MarioMaker::~MarioMaker()
{
    fsdevUnmountDevice("smm2save");
}

void MarioMaker::dumpLevels()
{
    for(u32 i = 0; i < 0x100; i++)
    {
        MarioMakerLevel level;
        std::stringstream name;
        name << "smm2save:/course_data_" << std::setw(3) << std::setfill('0') << i << ".bcd";
        struct stat st;
        if(stat(name.str().c_str(), &st) == 0)
        {
            std::string data = name.str();
            name.str("");
            name << "smm2save:/course_thumb_" << std::setw(3) << std::setfill('0') << i << ".btl";
            std::string thumb = name.str();
            decrypt(data.c_str(), level.course);
            decrypt(thumb.c_str(), level.thumb);
            u16 *txt16 = (u16*)&level.course[0xf4];
            u8 lname[0x20 + 1] = {0};
            utf16_to_utf8(lname, txt16, 0x20);
            level.levelName = std::string((char*)lname);
            level.index = i;
            this->levels.push_back(level);
        }
    }
}

std::vector<MarioMakerLevel> MarioMaker::listLevels()
{
    dumpLevels();
    return levels;
}