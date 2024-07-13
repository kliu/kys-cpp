#include "VirtualStick.h"
#include "Button.h"
#include "TextureManager.h"

#include "BattleSceneHades.h"
#include "BattleSceneSekiro.h"

VirtualStick::VirtualStick()
{
    Engine::getInstance()->getStartWindowSize(w_, h_);

    auto addButton = [this](std::shared_ptr<Button>& b, int id, int x, int y, int buttonid)
    {
        b = std::make_shared<Button>("title", id);
        b->setAlpha(128);
        b->button_id_ = buttonid;
        addChild(b, x, y);
    };

    addButton(button_up_, 312, w_ * 0.3, h_ * 0.6, SDL_CONTROLLER_BUTTON_DPAD_UP);
    addButton(button_down_, 312, w_ * 0.3, h_ * 0.8, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    addButton(button_left_, 312, w_ * 0.3 - h_ * 0.1, h_ * 0.7, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    addButton(button_right_, 312, w_ * 0.3 + h_ * 0.1, h_ * 0.7, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

    addButton(button_a_, 300, w_ * 0.8, h_ * 0.8, BP_CONTROLLER_BUTTON_A);
    addButton(button_b_, 302, w_ * 0.8 + h_ * 0.15, h_ * 0.65, BP_CONTROLLER_BUTTON_B);
    addButton(button_x_, 304, w_ * 0.8 - h_ * 0.15, h_ * 0.65, BP_CONTROLLER_BUTTON_X);
    addButton(button_y_, 306, w_ * 0.8, h_ * 0.5, BP_CONTROLLER_BUTTON_Y);

    //addButton(button_lb_, 300, w_ * 0.1, h_ * 0.5, BP_CONTROLLER_BUTTON_LEFTSHOULDER);
    //addButton(button_rb_, 302, w_ * 0.1, h_ * 0.8, BP_CONTROLLER_BUTTON_RIGHTSHOULDER);
    addButton(button_left_axis_, 320, w_ * 0.04, h_ * 0.5, BP_CONTROLLER_BUTTON_LEFTSTICK);

    addButton(button_menu_, 308, w_ * 0.5 - 20, h_ * 0.9, BP_CONTROLLER_BUTTON_START);
}

void VirtualStick::dealEvent(BP_Event& e)
{
    bool is_real = RunNode::getPointerFromRoot<BattleSceneHades>() != nullptr
        || RunNode::getPointerFromRoot<BattleSceneSekiro>() != nullptr;
    int num = SDL_GetNumTouchDevices();
    //fmt1::print("{}", num);
    auto engine = Engine::getInstance();
    engine->clearGameControllerButton();
    engine->clearGameControllerAxis();
    axis_x_ = 0;
    axis_y_ = 0;
    if (axis_radius_ == 0)
    {
        auto t = TextureManager::getInstance()->getTexture("title", 320);
        axis_center_x_ = t->w / 2 + w_ * 0.04;
        axis_center_y_ = t->h / 2 + h_ * 0.5;
        axis_radius_ = t->w / 2;
    }
    //fmt1::print("{}", "clear button");
    for (auto c : childs_)
    {
        auto b = std::dynamic_pointer_cast<Button>(c);
        b->state_ = NodeNormal;
    }

    for (int i_device = 0; i_device < num; i_device++)
    {
        auto touch = SDL_GetTouchDevice(i_device);
        if (!touch)
        {
            continue;
        }
        int fingers = SDL_GetNumTouchFingers(touch);
        bool is_press = false;
        for (int i = 0; i < fingers; i++)
        {
            auto s = SDL_GetTouchFinger(touch, i);
            //fmt1::print("{}: {} {} ", i, s->x * w_, s->y * h_);
            int x = s->x * w_;
            int y = s->y * h_;
            for (auto c : childs_)
            {
                auto b = std::dynamic_pointer_cast<Button>(c);
                if (b != button_left_axis_)
                {
                    b->state_ = NodeNormal;
                    engine->setGameControllerButton(b->button_id_, 0);
                    if (b->inSideInStartWindow(x, y))
                    {
                        auto& intval = button_interval_[b];
                        if (engine->getTicks() - intval.prev_press > intval.interval)
                        {
                            b->state_ = NodePress;
                            engine->setGameControllerButton(b->button_id_, 1);
                            intval.prev_press = engine->getTicks();
                            is_press = true;
                            intval.interval = 20;
                            if (is_real)
                            {
                                intval.interval = 0;
                            }
                        }
                    }
                }
                else
                {
                    if (b->inSideInStartWindow(x, y))
                    {
                        axis_x_ = x;
                        axis_y_ = y;
                        double r = sqrt((x - axis_center_x_) * (x - axis_center_x_) + (y - axis_center_y_) * (y - axis_center_y_));
                        //fmt1::print("{}", r);
                        auto& intval = button_interval_[b];
                        if (r < axis_radius_)
                        {
                            engine->setGameControllerAxis(SDL_CONTROLLER_AXIS_LEFTX, (x - axis_center_x_) * 30000 / axis_radius_);
                            engine->setGameControllerAxis(SDL_CONTROLLER_AXIS_LEFTY, (y - axis_center_y_) * 30000 / axis_radius_);
                            button_interval_[b].prev_press = engine->getTicks();
                            is_press = true;
                            b->state_ = NodePress;
                            intval.interval = 0;
                            if (is_real)
                            {
                                intval.interval = 0;
                            }
                        }
                    }
                }
            }
        }
        if (is_press)
        {
            prev_press_ = engine->getTicks();
        }
        if (is_press && button_a_->state_ == NodePress)
        {
            //fmt1::print("{}", "press a");
            e.type = BP_KEYUP;
            e.key.keysym.sym = BPK_RETURN;
            if (!is_real)
            {
                button_interval_[button_a_].interval = 100;
            }
        }
        if (is_press && button_b_->state_ == NodePress)
        {
            e.type = BP_KEYUP;
            e.key.keysym.sym = BPK_ESCAPE;
            if (!is_real)
            {
                button_interval_[button_b_].interval = 100;
            }
        }
        if (is_press && button_menu_->state_ == NodePress)
        {
            e.type = BP_KEYUP;
            e.key.keysym.sym = BPK_ESCAPE;
            if (!is_real)
            {
                button_interval_[button_menu_].interval = 100;
            }
        }
        if ((e.type == BP_MOUSEBUTTONUP || e.type == BP_MOUSEBUTTONDOWN || e.type == BP_MOUSEMOTION)
            && engine->getTicks() - prev_press_ < 500)
        {
            e.type = BP_FIRSTEVENT;
        }
        if (is_press)
        {
            return;
        }
    }
}

void VirtualStick::draw()
{
    RunNode::draw();
    if (axis_x_ > 0 && axis_y_ > 0)
    {
        auto t = TextureManager::getInstance()->getTexture("title", 312);
        TextureManager::getInstance()->renderTexture("title", 312, axis_x_ - t->w / 2, axis_y_ - t->h / 2, { 255, 255, 255, 255 }, 128);
    }
}
