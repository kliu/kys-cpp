
#include "filefunc.h"
#include <iostream>
#include <string>
#include <vector>

#include "GrpIdxFile.h"
#include "cmdline.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

void init_ins(std::string ini_file, std::string talkfile);
std::string transk(std::vector<int> e);
std::string trans50(std::string str);
void trans_talks(std::string talk_path, std::string coding);

int main(int argc, char* argv[])
{
    cmdline::parser cmd;
    cmd.add("talk", '\0', "trans talk to utf-8");
    cmd.add("kdef", '\0', "trans kdef directly to cifa (.c) event scripts");

    cmd.add<std::string>("in", 'i', "input path or file", false, ".");
    cmd.add<std::string>("out", 'o', "output path or file", false, ".");

    cmd.add<std::string>("talkfile", 't', "talk utf-8 file", false, "talkutf8.txt");
    cmd.add<std::string>("talkcoding", 'c', "talkcoding of grp", false, "cp950");

#ifdef _MSC_VER
    cmd.parse_check(GetCommandLineA());
#else
    cmd.parse_check(argc, argv);
#endif

    if (cmd.exist("talk"))
    {
        std::string talk_path = cmd.get<std::string>("in");
        trans_talks(talk_path, cmd.get<std::string>("talkcoding"));
    }

    if (cmd.exist("kdef"))
    {
        std::string path = cmd.get<std::string>("in");
        std::string path_out = cmd.get<std::string>("out");
        path_out += "/script/event-cifa";

        std::vector<int> offset, length;
        auto kdef_str = GrpIdxFile::getIdxContent(path + "/kdef.idx", path + "/kdef.grp", &offset, &length);
        std::vector<std::vector<int>> kdef_;
        kdef_.resize(length.size());
        for (int i = 0; i < length.size(); i++)
        {
            kdef_[i].resize(length[i] / sizeof(int16_t), -1);
            for (int k = 0; k < length[i] / sizeof(int16_t); k++)
            {
                kdef_[i][k] = *(int16_t*)(kdef_str.data() + offset[i] + k * 2);
            }
        }

        filefunc::makePath(path_out);
        init_ins("transk.ini", cmd.get<std::string>("talkfile"));
        for (int i = 0; i < kdef_.size(); i++)
        {
            auto str = trans50(transk(kdef_[i]));
            if (!str.empty())
            {
                filefunc::writeStringToFile(str, path_out + "/" + std::to_string(i) + ".c");
            }
            printf("%d.c\r", i);
        }
    }

    return 0;
}