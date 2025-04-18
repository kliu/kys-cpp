﻿#pragma once
#include "Button.h"
#include "Menu.h"
#include "Types.h"

class UIStatus : public RunNode
{
public:
    UIStatus();
    ~UIStatus();

protected:
    std::shared_ptr<Button> button_medicine_, button_detoxification_, button_leave_;
    std::shared_ptr<Button> button_equip0_, button_equip1_, button_learning_book_;
    std::shared_ptr<Menu> menu_;    //, menu_equip_magic_, menu_equip_item_;
    std::vector<std::shared_ptr<Button>> equip_magics_;
    std::shared_ptr<Button> equip_item_;

    bool show_button_ = true;
    Role* role_ = nullptr;

public:
    virtual void draw() override;
    virtual void dealEvent(EngineEvent& e) override;
    virtual void onPressedOK() override;

    void setShowButton(bool b) { show_button_ = b; }

    void setRole(Role* r) { role_ = r; }

    void setRoleName(const std::string& name);

    Role* getRole() { return role_; }

    void setExtentionVisible(bool b);

    std::shared_ptr<Menu>& getMenu() { return menu_; }
};
