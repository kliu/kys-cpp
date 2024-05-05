﻿#include "BattleSceneSekiro.h"
#include "Audio.h"
#include "Event.h"
#include "MainScene.h"
#include "TeamMenu.h"

BattleSceneSekiro::BattleSceneSekiro()
{
    keys_ = *UIKeyConfig::getKeyConfig();
    full_window_ = 1;
    COORD_COUNT = BATTLEMAP_COORD_COUNT;

    earth_layer_.resize(COORD_COUNT);
    building_layer_.resize(COORD_COUNT);

    heads_.resize(1);
    int i = 0;
    for (auto& h : heads_)
    {
        h = std::make_shared<Head>();
        h->setAlwaysLight(1);
        addChild(h, 10, 10 + (i++) * 80);
        h->setVisible(false);
    }
    heads_[0]->setVisible(true);
    heads_[0]->setRole(Save::getInstance()->getRole(0));

    heads_[0]->setPosition(10, Engine::getInstance()->getWindowHeight() - 100);

    head_boss_.resize(1);
    for (auto& h : head_boss_)
    {
        h = std::make_shared<Head>();
        h->setStyle(2);
        h->setVisible(false);
        addChild(h);
    }
}

void BattleSceneSekiro::setID(int id)
{
    battle_id_ = id;
    info_ = BattleMap::getInstance()->getBattleInfo(id);

    BattleMap::getInstance()->copyLayerData(info_->BattleFieldID, 0, &earth_layer_);
    BattleMap::getInstance()->copyLayerData(info_->BattleFieldID, 1, &building_layer_);
}

void BattleSceneSekiro::draw()
{
    //在这个模式下，使用的是直角坐标
    Engine::getInstance()->setRenderAssistTexture();
    Engine::getInstance()->fillColor({ 0, 0, 0, 255 }, 0, 0, render_center_x_ * 2, render_center_y_ * 2);

    //以下是计算出需要画的区域，先画到一个大图上，再转贴到窗口
    {
        auto p = pos90To45(pos_.x, pos_.y);
        man_x_ = p.x;
        man_y_ = p.y;
    }
    //一整块地面
    if (earth_texture_)
    {
        Engine::getInstance()->setRenderTarget(earth_texture_);
        Engine::getInstance()->fillColor({ 0, 0, 0, 255 }, 0, 0, COORD_COUNT * TILE_W * 2, COORD_COUNT * TILE_H * 2);
        for (int sum = -view_sum_region_; sum <= view_sum_region_ + 15; sum++)
        {
            for (int i = -view_width_region_; i <= view_width_region_; i++)
            {
                int ix = man_x_ + i + (sum / 2);
                int iy = man_y_ - i + (sum - sum / 2);
                auto p = pos45To90(ix, iy);
                if (!isOutLine(ix, iy))
                {
                    int num = earth_layer_.data(ix, iy) / 2;
                    BP_Color color = { 255, 255, 255, 255 };
                    bool need_draw = true;
                    if (need_draw && num > 0)
                    {
                        TextureManager::getInstance()->renderTexture("smap", num, p.x, p.y / 2, color);
                    }
                }
            }
        }

        struct DrawInfo
        {
            std::string path;
            int num;
            Pointf p;
            BP_Color color{ 255, 255, 255, 255 };
            uint8_t alpha = 255;
            int rot = 0;
            int shadow = 0;
            uint8_t white = 0;
        };
        std::vector<DrawInfo> draw_infos;
        draw_infos.reserve(10000);

        for (int sum = -view_sum_region_; sum <= view_sum_region_ + 15; sum++)
        {
            for (int i = -view_width_region_; i <= view_width_region_; i++)
            {
                int ix = man_x_ + i + (sum / 2);
                int iy = man_y_ - i + (sum - sum / 2);
                auto p = pos45To90(ix, iy);
                if (!isOutLine(ix, iy))
                {
                    int num = building_layer_.data(ix, iy) / 2;
                    if (num > 0)
                    {
                        //TextureManager::getInstance()->renderTexture("smap", num, p.x, p.y/2);
                        DrawInfo info;
                        info.path = "smap";
                        info.num = num;
                        info.p.x = p.x;
                        info.p.y = p.y;
                        info.shadow = 0;
                        draw_infos.emplace_back(std::move(info));
                    }
                }
            }
        }

        for (auto r : battle_roles_)
        {
            //if (r->Dead) { continue; }
            DrawInfo info;
            info.path = fmt1::format("fight/fight{:03}", r->HeadID);
            info.color = { 255, 255, 255, 255 };
            info.alpha = 255;
            info.white = 0;
            if (battle_cursor_->isRunning() && !acting_role_->isAuto())
            {
                info.color = { 128, 128, 128, 255 };
                if (inEffect(acting_role_, r))
                {
                    info.color = { 255, 255, 255, 255 };
                }
            }
            info.p = r->Pos;
            if (result_ == -1 && r->Shake)
            {
                info.p.x += -2.5 + rand_.rand() * 5;
            }
            r->FaceTowards = realTowardsToFaceTowards(r->RealTowards);
            info.num = calRolePic(r, r->ActType, r->ActFrame);
            //if (r->HurtFrame)
            //{
            //    if (r->HurtFrame % 2 == 1)
            //    {
            //        info.white = 128;
            //    }
            //}
            if (r->Dead)
            {
                //if (r->Frozen == 0)
                {
                    if (r->FaceTowards >= 2)
                    {
                        info.rot = 90;
                    }
                    else
                    {
                        info.rot = 270;
                    }
                }
            }
            if (r->Attention)
            {
                info.alpha = 255 - r->Attention * 4;
            }
            info.shadow = 1;
            //TextureManager::getInstance()->renderTexture(path, pic, r->X1, r->Y1, color, alpha);
            draw_infos.emplace_back(std::move(info));
        }

        //effects
        //for (auto& ae : attack_effects_)
        //{
        //    //for (auto r : ae.Defender)
        //    {
        //        DrawInfo info;
        //        info.path = ae.Path;
        //        if (ae.TotalEffectFrame > 0)
        //        {
        //            info.num = ae.Frame % ae.TotalEffectFrame;
        //        }
        //        else
        //        {
        //            info.num = 0;
        //        }
        //        info.p = ae.Pos;
        //        info.color = { 255, 255, 255, 255 };
        //        info.alpha = 192;
        //        if (ae.FollowRole)
        //        {
        //            info.p = ae.FollowRole->Pos;
        //        }
        //        info.shadow = 1;
        //        if (ae.Attacker && ae.Attacker->Team == 0)
        //        {
        //            info.shadow = 2;
        //        }
        //        info.alpha = 255 * (ae.TotalFrame * 1.25 - ae.Frame) / (ae.TotalFrame * 1.25);    //越来越透明
        //        draw_infos.emplace_back(std::move(info));
        //        //TextureManager::getInstance()->renderTexture(ae.Path, ae.Frame % ae.TotalEffectFrame, ae.X1, ae.Y1 / 2, { 255, 255, 255, 255 }, 192);
        //    }
        //}

        auto sort_building = [](DrawInfo& d1, DrawInfo& d2)
        {
            if (d1.p.y != d2.p.y)
            {
                return d1.p.y < d2.p.y;
            }
            else
            {
                return d1.p.x < d2.p.x;
            }
        };
        std::sort(draw_infos.begin(), draw_infos.end(), sort_building);

        //影子
        for (auto& d : draw_infos)
        {
            if (d.shadow)
            {
                auto tex = TextureManager::getInstance()->getTexture(d.path, d.num);
                if (tex)
                {
                    double scalex = 1, scaley = 0.3;
                    int yd = tex->dy * 0.7;
                    if (d.rot)
                    {
                        scalex = 0.3;
                        scaley = 1;
                        yd = tex->dy * 0.1;
                    }
                    if (d.shadow == 1)
                    {
                        TextureManager::getInstance()->renderTexture(tex, d.p.x, d.p.y / 2 + yd, { 32, 32, 32, 255 }, d.alpha / 2, scalex, scaley, d.rot);
                    }
                    if (d.shadow == 2)
                    {
                        TextureManager::getInstance()->renderTexture(tex, d.p.x, d.p.y / 2 + yd, { 128, 128, 128, 255 }, d.alpha / 2, scalex, scaley, d.rot, 128);
                    }
                }
            }
        }
        for (auto& d : draw_infos)
        {
            double scaley = 1;
            if (d.rot)
            {
                scaley = 0.5;
            }
            TextureManager::getInstance()->renderTexture(d.path, d.num, d.p.x, d.p.y / 2 - d.p.z, d.color, d.alpha, scaley, 1, d.rot, d.white);
        }

        for (auto r : battle_roles_)
        {
            renderExtraRoleInfo(r, r->Pos.x, r->Pos.y / 2);
        }

        BP_Color c = { 255, 255, 255, 255 };
        Engine::getInstance()->setColor(earth_texture_, c);
        int w = render_center_x_ * 2;
        int h = render_center_y_ * 2;
        //获取的是中心位置，如贴图应减掉屏幕尺寸的一半
        BP_Rect rect0 = { int(pos_.x - render_center_x_ - x_), int(pos_.y / 2 - render_center_y_ - y_), w, h };
        BP_Rect rect1 = { 0, 0, w, h };
        if (rect0.x < 0)
        {
            rect1.x = -rect0.x;
            rect0.x = 0;
            rect0.w = w - rect1.x;
        }
        if (rect0.y < 0)
        {
            rect1.y = -rect0.y;
            rect0.y = 0;
            rect0.h = h - rect1.y;
        }
        rect0.w = std::min(rect0.w, COORD_COUNT * TILE_W * 2 - rect0.x);
        rect0.h = std::min(rect0.h, COORD_COUNT * TILE_H * 2 - rect0.y);
        rect1.w = rect0.w;
        rect1.h = rect0.h;
        //for (auto& te : text_effects_)
        //{
        //    Font::getInstance()->draw(te.Text, te.Size, te.Pos.x, te.Pos.y / 2, te.Color, 255);
        //}

        Engine::getInstance()->setRenderAssistTexture();
        //if (close_up_)
        //{
        //    rect0.w /= 2;
        //    rect0.h /= 2;
        //    rect0.x += rect0.w / 2;
        //    rect0.y += rect0.h / 2 - 20;
        //}
        Engine::getInstance()->renderCopy(earth_texture_, &rect0, &rect1, 0);
    }

    Engine::getInstance()->renderAssistTextureToWindow();

    //if (result_ >= 0)
    //{
    //    Engine::getInstance()->fillColor({ 0, 0, 0, 128 }, 0, 0, -1, -1);
    //}
}

void BattleSceneSekiro::dealEvent(BP_Event& e)
{
    auto engine = Engine::getInstance();
    auto r = role_;

    pos_ = r->Pos;

    Pointf pos = r->Pos;
    double speed = std::min(4.0, r->Speed / 30.0);
    if (e.type == BP_KEYUP && e.key.keysym.sym == BPK_TAB
        || e.type == BP_CONTROLLERBUTTONUP && e.cbutton.button == BP_CONTROLLER_BUTTON_BACK)
    {
        if (r->Auto == 0) { r->Auto = 1; }
        else { r->Auto = 0; }
    }
    if (e.type == BP_KEYUP && e.key.keysym.sym == BPK_ESCAPE
        || e.type == BP_CONTROLLERBUTTONUP && e.cbutton.button == BP_CONTROLLER_BUTTON_START)
    {
        auto menu2 = std::make_shared<MenuText>();
        menu2->setStrings({ "確認（Y）", "取消（N）" });
        menu2->setPosition(400, 300);
        menu2->setFontSize(24);
        menu2->setHaveBox(true);
        menu2->setText("認輸？");
        menu2->arrange(0, 50, 150, 0);
        if (menu2->run() == 0)
        {
            result_ = 1;
            for (auto r : friends_)
            {
                r->ExpGot = 0;
            }
            setExit(true);
        }
    }
    if (r->Dead == 0)
    {
        if (r->Frozen == 0 && r->CoolDown == 0)
        {
            //if (current_frame_ % 3 == 0)
            {
                auto axis_x = engine->gameControllerGetAxis(BP_CONTROLLER_AXIS_LEFTX);
                auto axis_y = engine->gameControllerGetAxis(BP_CONTROLLER_AXIS_LEFTY);
                if (abs(axis_x) < 6000) { axis_x = 0; }
                if (abs(axis_y) < 6000) { axis_y = 0; }
                if (axis_x != 0 || axis_y != 0)
                {
                    //fmt1::print("{} {}, ", axis_x, axis_y);
                    axis_x = GameUtil::limit(axis_x, -30000, 30000);
                    axis_y = GameUtil::limit(axis_y, -30000, 30000);
                    Pointf axis{ double(axis_x), double(axis_y) };
                    axis *= 1.0 / 30000 / sqrt(2.0);
                    r->RealTowards = axis;
                    //r->FaceTowards = realTowardsToFaceTowards(r->RealTowards);
                    pos += speed * axis;
                }
                Pointf direct;
                if (engine->checkKeyPress(keys_.Left) || engine->checkKeyPress(BPK_LEFT))
                {
                    direct.x = -1;
                    r->FaceTowards = Towards_LeftDown;
                }
                if (engine->checkKeyPress(keys_.Right) || engine->checkKeyPress(BPK_RIGHT))
                {
                    direct.x = 1;
                    r->FaceTowards = Towards_RightUp;
                }
                if (engine->checkKeyPress(keys_.Up) || engine->checkKeyPress(BPK_UP))
                {
                    direct.y = -1;
                    r->FaceTowards = Towards_LeftUp;
                }
                if (engine->checkKeyPress(keys_.Down) || engine->checkKeyPress(BPK_DOWN))
                {
                    direct.y = 1;
                    r->FaceTowards = Towards_RightDown;
                }
                direct.normTo(speed);
                pos += direct;
                //这样来看同时用手柄和键盘会走得很快，就这样吧
            }
        }
        if (engine->checkKeyPress(keys_.Up) && engine->checkKeyPress(keys_.Right)
            || engine->checkKeyPress(BPK_UP) && engine->checkKeyPress(BPK_RIGHT))
        {
            r->FaceTowards = Towards_RightUp;
        }
        if (engine->checkKeyPress(keys_.Down) && engine->checkKeyPress(keys_.Left)
            || engine->checkKeyPress(BPK_DOWN) && engine->checkKeyPress(BPK_LEFT))
        {
            r->FaceTowards = Towards_LeftDown;
        }
        //实际的朝向可以不能走到
        if (pos.x != r->Pos.x || pos.y != r->Pos.y)
        {
            r->RealTowards = pos - r->Pos;
        }

        if (canWalk90(pos, r))
        {
            r->Pos = pos;
        }

        // 初始化武功
        std::vector<Magic*> magic(4);
        for (int i = 0; i < 4; i++)
        {
            magic[i] = Save::getInstance()->getMagic(r->EquipMagic[i]);
            if (magic[i] && r->getMagicOfRoleIndex(magic[i]) < 0) { magic[i] = nullptr; }
            //equip_magics_[i]->setState(NodeNormal);
        }

        if (r->Frozen == 0 && r->CoolDown == 0)
        {
            int index = -1;
            if (r->PhysicalPower >= 10
                && (engine->checkKeyPress(keys_.Light)
                    || engine->gameControllerGetButton(BP_CONTROLLER_BUTTON_X)
                    || (e.type == BP_MOUSEBUTTONDOWN && e.button.button == BP_BUTTON_LEFT)))
            {
                index = 0;
            }
            if (r->PhysicalPower >= 30
                && (engine->checkKeyPress(keys_.Heavy)
                    || engine->gameControllerGetButton(BP_CONTROLLER_BUTTON_Y)
                    || (e.type == BP_MOUSEWHEEL && e.wheel.y > 0)
                    || (e.type == BP_MOUSEBUTTONDOWN && e.button.button == BP_BUTTON_MIDDLE)))
            {
                index = 1;
            }
            if (r->PhysicalPower >= 20
                && (engine->checkKeyPress(keys_.Long)
                    || engine->gameControllerGetButton(BP_CONTROLLER_BUTTON_B)
                    || (e.type == BP_MOUSEBUTTONDOWN && e.button.button == BP_BUTTON_RIGHT)))
            {
                index = 2;
            }
            if (r->PhysicalPower >= 10
                && (engine->checkKeyPress(keys_.Slash)
                    || engine->gameControllerGetButton(BP_CONTROLLER_BUTTON_A)
                    || (e.type == BP_MOUSEWHEEL && e.wheel.y < 0)))
            {
                index = 3;
            }
            if (index >= 0)
            {
                r->Auto = 0;
            }
            if (r->OperationCount >= 3 || current_frame_ - r->PreActTimer > 60)
            {
                r->OperationCount = 0;
            }
            if (index >= 0 && index == r->OperationType)
            {
                r->OperationCount++;
            }

            if (index >= 0 && index < magic.size() && magic[index])
            {
                r->OperationType = index;
                //equip_magics_[index]->setState(NodePass);
                auto m = magic[index];
                r->ActType = m->MagicType;
                r->UsingMagic = m;
                r->UsingItem = nullptr;
                r->ActFrame = 0;
                r->HaveAction = 1;
                //r->Frozen = 5;
                if (index == 0)
                {
                    //点攻
                    //r->CoolDown = 10;
                }
                if (index == 1)
                {
                    //面攻
                    //r->CoolDown = 60;
                }
                if (index == 2)
                {
                    //远程
                    //r->CoolDown = 20;
                }
                if (index == 3)
                {
                    //闪身
                    //r->CoolDown = 10;    //冷却更长，有收招硬直
                }
                //r->CoolDown = calCoolDown(magic[index]->MagicType, index, r);
                if (r->OperationCount >= 3 && index == 0)
                {
                    r->CoolDown *= 2;
                }
            }
        }
    }
    backRun1();
}

void BattleSceneSekiro::dealEvent2(BP_Event& e)
{
}

void BattleSceneSekiro::onEntrance()
{
    calViewRegion();
    Audio::getInstance()->playMusic(info_->Music);
    //注意此时才能得到窗口的大小，用来设置头像的位置
    head_self_->setPosition(10, 10);
    int count = 0;
    for (auto& h : head_boss_)
    {
        h->setPosition(20, 20);
    }
    addChild(MainScene::getInstance()->getWeather());

    earth_texture_ = Engine::getInstance()->createARGBRenderedTexture(COORD_COUNT * TILE_W * 2, COORD_COUNT * TILE_H * 2);

    //首先设置位置和阵营，其他的后面统一处理
    //敌方
    for (int i = 0; i < BATTLE_ENEMY_COUNT; i++)
    {
        auto r_ptr = Save::getInstance()->getRole(info_->Enemy[i]);
        if (r_ptr)
        {
            enemies_obj_.push_back(*r_ptr);
            auto r = &enemies_obj_.back();
            r->resetBattleInfo();
            enemies_.push_back(r);
            r->setPositionOnly(info_->EnemyX[i], info_->EnemyY[i]);
            r->Team = 1;
            readFightFrame(r);
            r->FaceTowards = rand_.rand_int(4);
            man_x_ = r->X();
            man_y_ = r->Y();
        }
    }

    //初始状态
    for (auto r : enemies_)
    {
        setRoleInitState(r);
    }
    pos_ = enemies_[0]->Pos;

    //敌人按能力从低到高，依次出场
    std::sort(enemies_.begin(), enemies_.end(), [](Role* l, Role* r)
        {
            return l->MaxHP + l->Attack < r->MaxHP + r->Attack;
        });

    for (int i = 0; i < head_boss_.size(); i++)
    {
        bool is_boss = false;
        if (enemies_.size() >= i + 1)
        {
            auto r = enemies_[enemies_.size() - i - 1];
            if (is_boss || r->MaxHP >= 300 || r == enemies_.back())
            {
                is_boss = true;
                head_boss_[i]->setRole(r);
            }
        }
    }
    for (int i = 0; i < 1; i++)
    {
        if (!enemies_.empty())
        {
            battle_roles_.push_back(enemies_.front());
            enemies_.pop_front();
        }
    }

    //判断是不是有自动战斗人物
    if (info_->AutoTeamMate[0] >= 0)
    {
        for (int i = 0; i < TEAMMATE_COUNT; i++)
        {
            auto r = Save::getInstance()->getRole(info_->AutoTeamMate[i]);
            if (r)
            {
                friends_.push_back(r);
                r->Auto = 2;    //由AI控制
            }
        }
    }

    if (01)    //无队友出场
    {
        auto team_menu = std::make_shared<TeamMenu>();
        team_menu->setMode(1);
        team_menu->setForceMainRole(true);
        team_menu->run();

        for (auto r : team_menu->getRoles())
        {
            if (std::find(friends_.begin(), friends_.end(), r) == friends_.end())
            {
                friends_.push_back(r);
            }
        }
    }
    //队友
    role_ = Save::getInstance()->getRole(0);
    bool have_main = false;
    for (auto r : friends_)
    {
        if (r == role_) { have_main = true; }
    }
    if (!have_main)
    {
        friends_.insert(friends_.begin(), role_);
    }
    for (int i = 0; i < friends_.size(); i++)
    {
        auto r = friends_[i];
        if (r)
        {
            r->resetBattleInfo();
            battle_roles_.push_back(r);
            r->setPositionOnly(info_->TeamMateX[i], info_->TeamMateY[i]);
            r->Team = 0;
            setRoleInitState(r);
        }
    }
    int i = 0;
    for (auto r : friends_)
    {
        if (r && r != heads_[0]->getRole())
        {
            auto head = std::make_shared<Head>();
            head->setRole(r);
            head->setAlwaysLight(true);
            addChild(head, Engine::getInstance()->getWindowWidth() - 280, 10 + 80 * i++);
        }
    }

    //head_self_->setRole(role_);

    //for (int i = 0; i < equip_magics_.size(); i++)
    //{
    //    auto m = Save::getInstance()->getMagic(role_->EquipMagic[i]);
    //    if (m && role_->getMagicOfRoleIndex(m) < 0) { m = nullptr; }
    //    if (m)
    //    {
    //        std::string text = m->Name;
    //        text += std::string(10 - Font::getTextDrawSize(text), ' ');
    //        equip_magics_[i]->setText(text);
    //    }
    //    else
    //    {
    //        equip_magics_[i]->setText("__________");
    //    }
    //}
    //menu_->setVisible(true);
}

void BattleSceneSekiro::onExit()
{
}

void BattleSceneSekiro::backRun1()
{
    {
        int current_frame2 = current_frame_;
        for (auto r : battle_roles_)
        {
            //有行动
            Action(r);
            //ai策略
            AI(r);
        }
    }
    {
        //人物出场
        if (getTeamMateCount(1) < 2)
        {
            if (!enemies_.empty())
            {
                battle_roles_.push_back(enemies_.front());
                enemies_.pop_front();
                battle_roles_.back()->Attention = 30;
                battle_roles_.back()->CoolDown = 30;
                battle_roles_.back()->Invincible = 30;
            }
        }
        //亮血条
        if (enemies_.size() < head_boss_.size())
        {
            for (int i = 0; i < head_boss_.size(); i++)
            {
                if (i >= enemies_.size())
                {
                    head_boss_[i]->setVisible(true);
                }
            }
        }
        //检测战斗结果
        int battle_result = checkResult();

        //if (battle_result >= 0)
        //{
        //    if (result_ == -1)
        //    {
        //        //pos_ = dying_->Pos;
        //        close_up_ = 60;
        //        frozen_ = 60;
        //        slow_ = 40;
        //        shake_ = 60;
        //        result_ = battle_result;
        //    }
        //    if (slow_ == 0 && (result_ == 0 || result_ == 1))
        //    {
        //        menu_->setVisible(false);
        //        calExpGot();
        //        setExit(true);
        //    }
        //}
    }
}

void BattleSceneSekiro::Action(Role* r)
{
    if (r->HaveAction)
    {
        //音效和动画
        if (r->OperationType >= 0
            //&& r->ActFrame == r->FightFrame[r->ActType] - 3
            && r->ActFrame == calCast(r->ActType, r->OperationType, r))
        {
            //r->HaveAction = 0;
            r->PreActTimer = current_frame_;
            for (auto m : r->getLearnedMagics())
            {
                if (special_magic_effect_attack_.count(m->Name))
                {
                    special_magic_effect_attack_[m->Name](r);
                }
            }
            Magic* magic = nullptr;
            if (r->UsingMagic)
            {
                magic = r->UsingMagic;
            }
            else
            {
                std::vector<Magic*> v;
                for (int i = 0; i < ROLE_MAGIC_COUNT; i++)
                {
                    if (r->MagicID[i] > 0)
                    {
                        auto m = Save::getInstance()->getMagic(r->MagicID[i]);
                        if (m->MagicType == r->ActType)
                        {
                            v.push_back(m);
                        }
                    }
                }
                if (!v.empty())
                {
                    magic = v[rand_.rand() * v.size()];
                }
            }
            AttackEffect ae;
            if (magic)
            {
                Audio::getInstance()->playASound(magic->SoundID);
                ae.setEft(magic->EffectID);
                ae.UsingMagic = magic;
            }
            else
            {
                Audio::getInstance()->playESound(r->ActType);
                ae.setEft(11);
                magic = Save::getInstance()->getMagic(1);
                ae.UsingMagic = Save::getInstance()->getMagic(1);
            }
            r->PhysicalPower = GameUtil::limit(r->PhysicalPower - 3, 0, Role::getMaxValue()->PhysicalPower);
            int level_index = r->getMagicLevelIndex(magic->ID);
            int needMP = magic->calNeedMP(level_index);
            ae.TotalFrame = 30;
            //r->CoolDown += ae.TotalFrame;
            ae.Attacker = r;
            r->RealTowards.normTo(1);
            ae.Pos = r->Pos + TILE_W * 2.0 * r->RealTowards;
            ae.Frame = 0;
            if (r->Team == 0 && r == role_)
            {
                ae.OperationType = r->OperationType;
                //if (r->OperationType == 0 && ae.UsingMagic->AttackAreaType != 0)
                //{
                //    ae.OperationType = -1;
                //}
                //if (r->OperationType == 2 && (ae.UsingMagic->AttackAreaType != 1 && ae.UsingMagic->AttackAreaType != 2))
                //{
                //    ae.OperationType = -1;
                //}
                //if (r->OperationType == 1 && ae.UsingMagic->AttackAreaType != 3)
                //{
                //    ae.OperationType = -1;
                //}
            }
            else
            {
                ae.OperationType = r->OperationType;
                if (r->OperationType == -1)
                {
                    if (ae.UsingMagic->AttackAreaType == 0)
                    {
                        ae.OperationType = 0;
                    }
                    else if (ae.UsingMagic->AttackAreaType == 1 || ae.UsingMagic->AttackAreaType == 2)
                    {
                        ae.OperationType = 2;
                    }
                    else if (ae.UsingMagic->AttackAreaType == 3)
                    {
                        ae.OperationType = 1;
                    }
                }
            }

            int index = r->getMagicOfRoleIndex(ae.UsingMagic);
            if (index >= 0)
            {
                r->MagicLevel[index] = GameUtil::limit(r->MagicLevel[index] + rand_.rand() * 2 + 1, 0, 999);
            }
            //根据性质创造攻击效果
            if (ae.OperationType == 0)
            {
                ae.TotalFrame = 10;
                if (r->OperationCount == 3 && magic->AttackAreaType == 0)
                {
                    ae.TotalFrame = 30;
                    shake_ = 10;
                    ae.Strengthen = 2;
                    ae.Velocity = r->RealTowards;
                    ae.Velocity.normTo(magic->SelectDistance[level_index] / 2.0);
                    ae.Track = 1;
                }
                attack_effects_.push_back(std::move(ae));
                needMP *= 0.1;
            }
            else if (ae.OperationType == 1)
            {
                int range = 0;
                if (magic->AttackAreaType == 3)
                {
                    range = 1;
                    //range += magic->AttackDistance[level_index] + magic->SelectDistance[level_index] / 2;
                }
                int count = 1 + range;
                //count = 2;
                auto p = ae.Pos;
                ae.TotalFrame = 120;
                double angle = r->RealTowards.getAngle();
                for (int i = 0; i < count; i++)
                {
                    double a = angle - 5 / 180 * M_PI + rand_.rand() * 10 / 180 * M_PI;
                    ae.Pos = p;
                    ae.Velocity = { cos(a), sin(a) };
                    ae.Velocity.normTo(3);
                    ae.Frame = rand_.rand() * 10;
                    ae.Track = 1;
                    attack_effects_.push_back(ae);
                }
                needMP *= 0.4;
            }
            else if (ae.OperationType == 2)
            {
                auto r0 = findNearestEnemy(r->Team, r->Pos);
                if (r0)
                {
                    ae.Velocity = r0->Pos - r->Pos;
                    r->RealTowards = ae.Velocity;
                    //r->FaceTowards = realTowardsToFaceTowards(r->RealTowards);
                }
                else
                {
                    ae.Velocity = r->RealTowards;
                }
                ae.Velocity.normTo(5);
                ae.TotalFrame = 15 + magic->SelectDistance[level_index] * 5;
                if (magic->AttackAreaType == 1 || magic->AttackAreaType == 2)
                {
                    ae.Through = 1;
                }
                attack_effects_.push_back(ae);
                needMP *= 0.2;
                if (magic->AttackAreaType == 1 || magic->AttackAreaType == 2)
                {
                    double v = 5;
                    double angle = ae.Velocity.getAngle();
                    for (int i = 0; i < 2; i++)
                    {
                        v -= 0.5;
                        double a = angle - 15 / 180 * M_PI + rand_.rand() * 30 / 180 * M_PI;
                        ae.Velocity = { cos(a), sin(a) };
                        ae.Velocity.normTo(v);
                        //ae.TotalFrame = 150;
                        ae.Through = 1;
                        ae.Strengthen = 0.5;
                        attack_effects_.push_back(ae);
                        //ae.Pos = ae.Pos - ae.Velocity;
                    }
                }
            }
            else if (ae.OperationType == 3)
            {
                if (r->HeadID == 0)
                {
                    int i = 0;
                }
                auto acc = r->RealTowards;
                acc.normTo(std::min(4.0, r->Speed / 30.0) * 1.7);
                r->Velocity = acc;
                //r->Acceleration += acc;
                //r->VelocitytFrame = 10;
                r->ActType = 0;
                auto p = ae.Pos;
                int count = std::min(3, (r->Speed + r->getActProperty(ae.UsingMagic->MagicType)) / 60);
                for (int i = 0; i < count; i++)
                {
                    ae.Pos = p + r->Velocity * (i - 1) * 2;
                    ae.Frame += 3;
                    attack_effects_.push_back(ae);
                }
                needMP *= 0.05;
            }
            fmt1::print("{} use {} as {}\n", ae.Attacker->Name, ae.UsingMagic->Name, ae.OperationType);
            r->MP -= needMP;
            r->UsingMagic = nullptr;
        }

        if (r->UsingItem)
        {
            Item* item = r->UsingItem;
            if (item->ItemType == 3)
            {
                // 药品直接服用
                r->useItem(item);
                //TextEffect te;
                //BP_Color c = { 255, 255, 255, 255 };
                //if (r->Team == 0)
                //{
                //    c = { 255, 20, 220, 20 };
                //}
                //const int left = std::max(0, Save::getInstance()->getItemCountInBag(item->ID) - 1);
                //te.set(fmt1::format("服用{}，剩余{}", item->Name, left), c, r);
                //text_effects_.push_back(std::move(te));
            }
            else if (item->ItemType == 4)
            {
                // 暗器
                AttackEffect ae1;
                auto r0 = findFarthestEnemy(r->Team, r->Pos);
                if (r0)
                {
                    ae1.Velocity = r0->Pos - r->Pos;
                }
                else
                {
                    ae1.Velocity = r->RealTowards;
                }
                ae1.Velocity.normTo(10);
                ae1.Attacker = r;
                ae1.Pos = r->Pos;
                ae1.UsingHiddenWeapon = item;
                ae1.Through = 0;
                ae1.setEft(item->HiddenWeaponEffectID);
                ae1.TotalFrame = 100;
                ae1.Frame = 0;
                ae1.OperationType = 4;
                attack_effects_.push_back(std::move(ae1));
            }
            // 减少数量
            Event::getInstance()->addItemWithoutHint(item->ID, -1);
            r->UsingItem = nullptr;
        }

        if (r->OperationType == 1)
        {
            r->ActFrame++;
            if (r->ActFrame >= 7)
            {
                shake_ = 1;
            }
        }
        else
        {
            r->ActFrame++;
        }
    }
}

void BattleSceneSekiro::AI(Role* r)
{
    if ((r != role_ || r->Auto)
        && r->Dead == 0)
    {
        if (r->CoolDown == 0)
        {
            if (r->UsingMagic == nullptr)
            {
                if (r == role_)    //主角只能使用已装备的武学，几率相同
                {
                    std::vector<Magic*> v;
                    for (auto i : r->EquipMagic)
                    {
                        auto m = Save::getInstance()->getMagic(i);
                        if (m && r->getMagicOfRoleIndex(m) >= 0)
                        {
                            v.push_back(m);
                        }
                    }
                    if (!v.empty())
                    {
                        int index = rand_.rand() * v.size();
                        r->UsingMagic = v[index];
                    }
                }
                else
                {
                    //其他ai可以使用所有武学
                    auto v = r->getLearnedMagics();
                    if (v.size() == 1)
                    {
                        r->UsingMagic = v[0];
                    }
                    else if (v.size() >= 0)
                    {
                        std::vector<double> hurt;
                        double sum = 0;
                        for (auto m : v)
                        {
                            double h = m->Attack[r->getMagicLevelIndex(m)];
                            //h = exp(h / 500);    //几率正比于武功威力
                            hurt.push_back(sum + h);
                            sum += h;
                        }
                        double select = rand_.rand() * sum;
                        for (int i = 0; i < hurt.size(); i++)
                        {
                            if (select < hurt[i])
                            {
                                r->UsingMagic = v[i];
                                break;
                            }
                        }
                    }
                }
            }
            auto r0 = findNearestEnemy(r->Team, r->Pos);
            if (r0)
            {
                r->RealTowards = r0->Pos - r->Pos;
                //r->FaceTowards = realTowardsToFaceTowards(r->RealTowards);
                r->RealTowards.normTo(1);
                int dis = TILE_W * 3;
                if (r->UsingMagic)
                {
                    if (r->UsingMagic->AttackAreaType == 3) { dis = 180; }
                    if (r->UsingMagic->AttackAreaType == 1 || r->UsingMagic->AttackAreaType == 2) { dis = 300; }
                }
                double speed = r->Speed / 30.0;
                if (EuclidDis(r->Pos, r0->Pos) > dis)
                {
                    auto p = r->Pos + speed * r->RealTowards;
                    if (canWalk90(p, r) && r->FindingWay == 0)
                    {
                        //能否闪身的条件，似乎比较复杂
                        if (rand_.rand() < 0.25 && r->Speed >= 60
                            && (r != role_ && r->UsingMagic
                                || r == role_ && r->getEquipMagicOfRoleIndex(r->UsingMagic) == 3))
                        {
                            r->OperationType = 3;
                        }
                        else
                        {
                            r->OperationType = -1;
                        }
                        if (r->OperationType == 3)
                        {
                            r->CoolDown = calCoolDown(r->UsingMagic->MagicType, r->OperationType, r);
                            r->ActFrame = 0;
                            r->HaveAction = 1;
                        }
                        else
                        {
                            r->Pos = p;
                        }
                    }
                    else if (r->Velocity.norm() < 0.1)
                    {
                        //用复杂路径法查找一个目标并接近
                        MapSquareInt dis_layer;
                        dis_layer.resize(COORD_COUNT);
                        auto p_enemy45 = pos90To45(r0->Pos.x, r0->Pos.y);
                        calDistanceLayer(p_enemy45.x, p_enemy45.y, dis_layer, 64);
                        auto p_self45 = pos90To45(r->Pos.x, r->Pos.y);
                        int max_dis45 = 4096;
                        Pointf p_target = r->Pos;
                        for (int x = p_self45.x - 1; x <= p_self45.x + 1; x++)
                        {
                            for (int y = p_self45.y - 1; y <= p_self45.y + 1; y++)
                            {
                                if (calDistance(x, y, p_self45.x, p_self45.y) != 1)
                                {
                                    continue;
                                }
                                auto p1 = pos45To90(x, y);
                                double dis1 = dis_layer.data(x, y) + 1 * (rand_.rand() - rand_.rand());
                                if (canWalk90(p1, r) && dis1 < max_dis45)
                                {
                                    max_dis45 = dis1;
                                    p_target = p1;
                                }
                            }
                        }
                        r->FindingWay = 1;
                        r->RealTowards = p_target - r->Pos;
                        if (rand_.rand() < 0.25 && r->Speed >= 60
                            && (r != role_ && r->UsingMagic
                                || r == role_ && r->getEquipMagicOfRoleIndex(r->UsingMagic) == 3))
                        {
                            r->OperationType = 3;
                        }
                        else
                        {
                            r->OperationType = -1;
                        }
                        if (r->OperationType == 3)
                        {
                            r->CoolDown = calCoolDown(r->UsingMagic->MagicType, r->OperationType, r);
                            r->ActFrame = 0;
                            r->HaveAction = 1;
                        }
                        else
                        {
                            r->RealTowards = p_target - r->Pos;
                            //r->FaceTowards = realTowardsToFaceTowards(r->RealTowards);
                            auto distance = r->RealTowards.norm();
                            //r->FaceTowards = readTowardsToFaceTowards(r->RealTowards);
                            r->RealTowards.normTo(1);
                            //r->Pos = p2;
                            r->Velocity = r->RealTowards * speed;
                            //todo:r->VelocitytFrame = 3;
                        }
                    }
                }
                else
                {
                    r->FindingWay = 0;
                    if (r->PhysicalPower >= 30 && r->UsingMagic)
                    {
                        //点攻击疯狗咬即可
                        if (r->UsingMagic->AttackAreaType == 0 || rand_.rand() < 0.75 && r->UsingMagic->AttackAreaType != 0)
                        {
                            //attack
                            auto m = r->UsingMagic;
                            if (m)
                            {
                                if (r == role_)
                                {
                                    r->OperationType = r->getEquipMagicOfRoleIndex(m);
                                }
                                else
                                {
                                    if (m->AttackAreaType == 0)
                                    {
                                        r->OperationType = 0;
                                    }
                                    else if (m->AttackAreaType == 1 || m->AttackAreaType == 2)
                                    {
                                        r->OperationType = 2;
                                    }
                                    else if (m->AttackAreaType == 3)
                                    {
                                        r->OperationType = 1;
                                    }
                                }
                                r->CoolDown = calCoolDown(m->MagicType, r->OperationType, r);
                                r->ActFrame = 0;
                                r->ActType = m->MagicType;
                                r->HaveAction = 1;
                                //TextEffect te;
                                //te.Text = m->Name;
                                //te.Size = 15;
                                //te.Type = 1;
                                //te.Pos.x = r->Pos.x - 15 * te.Text.size() / 3;
                                //te.Pos.y = r->Pos.y;
                                //te.Color = { 255, 0, 0, 255 };
                                //te.Frame = 15;
                                //text_effects_.push_back(te);
                            }
                        }
                        else
                        {
                            if (r != role_ && rand_.rand() < 0.25)
                            {
                                r->OperationType = 3;
                            }
                            if (r->OperationType == 3)
                            {
                                //随机移动一下，增加一些变数
                                r->RealTowards.rotate(M_PI * 0.75 * (2 * rand_.rand() - 1));
                                r->OperationType = 3;
                                r->CoolDown = calCoolDown(r->UsingMagic->MagicType, r->OperationType, r);
                                r->ActFrame = 0;
                                r->HaveAction = 1;
                                //r->RealTowards *= 3;
                            }
                        }
                        if (!r->HaveAction)
                        {
                            //走两步
                            r->RealTowards.rotate(M_PI * 0.5 * (2 * rand_.rand() - 1));
                            //r->FaceTowards = realTowardsToFaceTowards(r->RealTowards);
                            r->Velocity = r->RealTowards;
                            r->Velocity.normTo(speed);
                            //todo:r->VelocitytFrame = 20;
                        }
                    }
                }
            }
        }
    }
}

int BattleSceneSekiro::checkResult()
{
    int team0 = getTeamMateCount(0);
    int team1 = enemies_.size() + getTeamMateCount(1);
    if (team0 > 0 && team1 == 0)
    {
        return 0;
    }
    if (team0 == 0 && team1 >= 0)
    {
        return 1;
    }
    return -1;
}

void BattleSceneSekiro::setRoleInitState(Role* r)
{
    BattleScene::setRoleInitState(r);
    if (r->Team == 0)
    {
        r->HP = r->MaxHP;
        r->MP = r->MaxMP;
        r->PhysicalPower = (std::max)(r->PhysicalPower, 90);
    }
    else
    {
        r->HP = r->MaxHP;
        r->MP = r->MaxMP;
        r->PhysicalPower = (std::max)(r->PhysicalPower, 90);
    }

    auto p = pos45To90(r->X(), r->Y());

    r->Pos.x = p.x;
    r->Pos.y = p.y;
    if (r->FaceTowards == Towards_RightDown)
    {
        r->RealTowards = { 1, 1 };
    }
    if (r->FaceTowards == Towards_RightUp)
    {
        r->RealTowards = { 1, -1 };
    }
    if (r->FaceTowards == Towards_LeftDown)
    {
        r->RealTowards = { -1, 1 };
    }
    if (r->FaceTowards == Towards_LeftUp)
    {
        r->RealTowards = { -1, -1 };
    }
    r->Acceleration = { 0, 0, gravity_ };
}

void BattleSceneSekiro::renderExtraRoleInfo(Role* r, double x, double y)
{
    if (r == nullptr || r->Dead)
    {
        return;
    }
    // 画个血条
    BP_Color outline_color = { 0, 0, 0, 128 };
    BP_Color background_color = { 0, 255, 0, 128 };    // 我方绿色
    if (r->Team == 1)
    {
        // 敌方红色
        background_color = { 255, 0, 0, 128 };
    }
    int hp_max_w = 24;
    int hp_x = x - hp_max_w / 2;
    int hp_y = y - 60;
    int hp_h = 3;
    double perc = ((double)r->HP / r->MaxHP);
    if (perc < 0)
    {
        perc = 0;
    }
    double alpha = 1;
    if (r->HP <= 0)
    {
        alpha = dead_alpha_ / 255.0;
    }
    BP_Rect r0 = { hp_x - 1, hp_y - 1, hp_max_w + 2, hp_h + 2 };
    Engine::getInstance()->renderSquareTexture(&r0, outline_color, 128 * alpha);
    BP_Rect r1 = { hp_x, hp_y, int(perc * hp_max_w), hp_h };
    Engine::getInstance()->renderSquareTexture(&r1, background_color, 192 * alpha);
}

Role* BattleSceneSekiro::findNearestEnemy(int team, Pointf p)
{
    double dis = 4096;
    Role* r0 = nullptr;
    for (auto r1 : battle_roles_)
    {
        if (r1->Dead == 0 && team != r1->Team)
        {
            auto dis1 = EuclidDis(p, r1->Pos);
            if (dis1 < dis)
            {
                dis = dis1;
                r0 = r1;
            }
        }
    }
    return r0;
}

Role* BattleSceneSekiro::findFarthestEnemy(int team, Pointf p)
{
    double dis = 0;
    Role* r0 = nullptr;
    for (auto r1 : battle_roles_)
    {
        if (r1->Dead == 0 && team != r1->Team)
        {
            auto dis1 = EuclidDis(p, r1->Pos);
            if (dis1 > dis)
            {
                dis = dis1;
                r0 = r1;
            }
        }
    }
    return r0;
}


//前摇
int BattleSceneSekiro::calCast(int act_type, int operation_type, Role* r)
{
    int v[4] = { 10, 20, 15, 5 };
    if (operation_type >= 0 && operation_type <= 3)
    {
        return v[operation_type];
    }
    return 0;
}

//冷却减去前摇就是后摇
//需注意攻击判定可能仍然存在，严格来说攻击判定存在的时间加上前摇应小于冷却
int BattleSceneSekiro::calCoolDown(int act_type, int operation_type, Role* r)
{
    int i = r->getActProperty(act_type);
    int v[4] = { 60 - i / 2, 160 - i, 70 - i / 2, 10 };
    int min_v[4] = { 20, 45, 30, 10 };
    if (operation_type >= 0 && operation_type <= 3)
    {
        int c = std::max(min_v[operation_type], v[operation_type]);
        if (r->AttackTwice > 0)
        {
            c *= 0.666;
            c = std::max(calCast(act_type, operation_type, r) + 2, c);
        }
        return c;
    }
    else
    {
        return 0;
    }
}

int BattleSceneSekiro::calRolePic(Role* r, int style, int frame)
{
    if (style < 0 || style >= 5)
    {
        for (int i = 0; i < 5; i++)
        {
            if (r->FightFrame[i] > 0)
            {
                return r->FightFrame[i] * r->FaceTowards;
            }
        }
    }
    if (r->FightFrame[style] <= 0)
    {
        //改为选一个存在的动作，否则看不出是在攻击
        for (int i = 0; i < 5; i++)
        {
            if (r->FightFrame[i] > 0)
            {
                style = i;
            }
        }
    }
    int total = 0;
    for (int i = 0; i < 5; i++)
    {
        if (i == style)
        {
            //停留在最后一帧
            if (frame < r->FightFrame[style] - 2)
            {
                return total + r->FightFrame[style] * r->FaceTowards + frame;
            }
            else
            {
                return total + r->FightFrame[style] * r->FaceTowards + r->FightFrame[style] - 2;
            }
        }
        total += r->FightFrame[i] * 4;
    }
    return r->FaceTowards;
}
