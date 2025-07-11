﻿#include "Console.h"
#include "BattleNetwork.h"
#include "BattleScene.h"
#include "DrawableOnCall.h"
#include "Event.h"
#include "Font.h"
#include "InputBox.h"
#include "MainScene.h"
#include "PotConv.h"
#include "Save.h"
#include "SuperMenuText.h"
#include "TextureManager.h"
#include "strfunc.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

Console::Console()
{
    std::string code;
    {
        auto input = std::make_shared<InputBox>("神秘代碼：", 30);
        input->setInputPosition(350, 300);
        input->run();
        if (input->getResult() >= 0)
        {
            code = input->getText();
        }
        else
        {
            return;
        }
    }
    // 捂脸
    auto splits = strfunc::splitString(code, " ");
    if (splits.empty())
    {
        splits.push_back("");
    }
    if (code == "menutest")
    {
        std::vector<std::pair<int, std::string>> generated;
        for (int i = 0; i < 450; i++)
        {
            generated.emplace_back(i, "a" + std::to_string(i));
        }
        auto smt = std::make_shared<SuperMenuText>("少废话", 28, generated, 10);
        smt->setInputPosition(180, 80);
        smt->run();
        int id = smt->getResult();
        LOG("result %d\n", id);
    }
    else if (RunNode::getPointerFromRoot<SubScene>() == nullptr
        && (code == "chuansong" || code == "teleport" || code == "mache" || code == ""))
    {
        std::vector<std::pair<int, std::string>> locs;
        for (const auto& info : Save::getInstance()->getSubMapInfos())
        {
            // 还有其他要求 这里作为一个demo就意思意思
            if (info->MainEntranceX1 != 0 && info->MainEntranceY1 != 0)
            {
                std::string name(info->Name);
                // 有空格方便完成双击确认
                locs.emplace_back(info->ID, name + std::to_string(info->ID));
            }
        }
        int dx = 180;
        int dy = 80;
        auto drawScene = [dx, dy](DrawableOnCall* d)
        {
            if (d->getID() == -1)
            {
                return;
            }
            int id = d->getID();
            auto scene = Save::getInstance()->getSubMapInfos()[id];
            int nx = dx + 350;
            int ny = dy + 100;
            int fontSize = 28;
            TextureManager::getInstance()->renderTexture("title", 126, { nx, ny, 400, 400 }, { 192, 192, 192, 255 }, 255);
            //Engine::getInstance()->fillColor({ 0, 0, 0, 192 }, nx, ny, 400, 400);
            Font::getInstance()->draw(std::format("{}，{}", scene->Name, scene->ID), fontSize, nx + 20, ny + 20);
            Font::getInstance()->draw(std::format("（{}，{}）", scene->MainEntranceX1, scene->MainEntranceY1),
                fontSize, nx + 20, ny + 20 + fontSize * 1.5);

            int man_x_ = scene->MainEntranceX1;
            int man_y_ = scene->MainEntranceY1;
            auto mainScene = MainScene::getInstance();

            if (man_x_ == 0 && man_y_ == 0)
            {
                return;
            }
            // 不会画场景，需要慢慢学习，不行我复制个代码 强行搞

            struct DrawInfo
            {
                int index;
                TextureWarpper* t;
                Point p;
            };

            std::vector<DrawInfo> building_vec(1000);
            int building_count = 0;

            int hw = 2;
            for (int sum = -hw; sum <= hw + 10; sum++)
            {
                for (int i = -hw; i <= hw; i++)
                {
                    int ix = man_x_ + i + (sum / 2);
                    int iy = man_y_ - i + (sum - sum / 2);
                    auto p = mainScene->getPositionOnRender(ix, iy, man_x_, man_y_);
                    p.x += nx - 160;
                    p.y += ny;
                    if (mainScene->building_layer_.data(ix, iy).getTexture())
                    {
                        //根据图片的宽度计算图的中点, 为避免出现小数, 实际是中点坐标的2倍
                        //次要排序依据是y坐标
                        //直接设置z轴
                        auto tex = mainScene->building_layer_.data(ix, iy).getTexture();
                        if (tex == nullptr)
                        {
                            continue;
                        }
                        auto w = tex->w;
                        auto h = tex->h;
                        auto dy = tex->dy;
                        int c = ((ix + iy) - (w + 35) / 36 - (dy - h + 1) / 9) * 1024 + ix;
                        //map[2 * c + 1] = { 2*c+1, t, p };
                        building_vec[building_count++] = { 2 * c + 1, tex, p };
                    }

                    auto sort_building = [](DrawInfo& d1, DrawInfo& d2)
                    {
                        return d1.index < d2.index;
                    };
                    std::sort(building_vec.begin(), building_vec.begin() + building_count, sort_building);
                    for (int i = 0; i < building_count; i++)
                    {
                        auto& d = building_vec[i];
                        TextureManager::getInstance()->renderTexture(d.t, d.p.x, d.p.y);
                    }
                }
            }
        };
        std::shared_ptr<DrawableOnCall> doc = std::make_shared<DrawableOnCall>(drawScene);
        auto smt = std::make_shared<SuperMenuText>("可輸入傳送地名，編號或拼音搜索：", 28, locs, 15);
        smt->setInputPosition(dx, dy);
        smt->addDrawableOnCall(doc);

        smt->run();
        int id = smt->getResult();
        if (id != -1)
        {
            auto scene = Save::getInstance()->getSubMapInfos()[id];
            MainScene::getInstance()->forceEnterSubScene(id, scene->EntranceX, scene->EntranceY);
            MainScene::getInstance()->setManPosition(scene->MainEntranceX1, scene->MainEntranceY1);
            LOG("傳送到{}\n", id);
        }
    }
    /*
    else if (splits[0] == "newsave" && splits.size() >= 2)
    {
        int rec;
        try
        {
            rec = std::stoi(splits[1]);
        }
        catch (...)
        {
            return;
        }
        auto save = Save::getInstance();
        auto main_scene = MainScene::getInstance();
        main_scene->getManPosition(save->MainMapX, save->MainMapY);
        save->InSubMap = -1;
        Save::getInstance()->saveRToCSV(rec);
        Save::getInstance()->saveSD(rec);
    }
    else if (splits[0] == "newload" && splits.size() >= 2)
    {
        int rec;
        try
        {
            rec = std::stoi(splits[1]);
        }
        catch (...)
        {
            return;
        }
        auto save = Save::getInstance();
        auto main_scene = MainScene::getInstance();
        Save::getInstance()->loadRFromCSV(rec);
        Save::getInstance()->loadSD(rec);
        main_scene->setManPosition(save->MainMapX, save->MainMapY);
        if (save->InSubMap >= 0)
        {
            main_scene->forceEnterSubScene(save->InSubMap, save->SubMapX, save->SubMapY);
        }
    }
    else if (splits[0] == "rinsert" && splits.size() >= 3)
    {
        int idx;
        try
        {
            idx = std::stoi(splits[2]);
        }
        catch (...)
        {
            return;
        }
        Save::getInstance()->insertAt(splits[1], idx);
    }*/
    else if (splits.size() > 1 && splits[0] == "host")
    {
        Save::getInstance()->save(11);

        auto host = BattleNetworkFactory::MakeHost(splits[1]);
        if (!host)
        {
            return;
        }

        auto battle = std::make_shared<BattleScene>();
        battle->setupNetwork(std::move(host));
        battle->run();

        Save::getInstance()->load(11);
    }
    else if (splits.size() > 1 && splits[0] == "client")
    {
        Save::getInstance()->save(11);

        auto client = BattleNetworkFactory::MakeClient(splits[1]);
        if (!client)
        {
            return;
        }

        auto battle = std::make_shared<BattleScene>();
        battle->setupNetwork(std::move(client));
        battle->run();

        Save::getInstance()->load(11);
    }
    else if (splits.size() > 1 && (splits[0] == "battle" || splits[0] == "b"))
    {
        Save::getInstance()->save(11);
        int k = atoi(splits[1].c_str());
        Event::getInstance()->tryBattle(k, 0);
        //Save::getInstance()->load(11);
    }
}
