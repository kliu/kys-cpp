#pragma once
#include "TextBox.h"
#include "Head.h"

//�����ר������ʾ����Role�Ĳ�ͬ������������ҩ����ʾǰ��Ƚ�
//���������Ա仯ǰ����һ��ʱ�����¼���ٱȽ�ǰ��ı仯
class ShowRoleDifference : public TextBox
{
public:
    ShowRoleDifference();
    ~ShowRoleDifference();
    ShowRoleDifference(Role* r1, Role* r2) : ShowRoleDifference() { setTwinRole(r1, r2); }

    Role* role1_ = nullptr, *role2_ = nullptr;

    void setTwinRole(Role* r1, Role* r2) { role1_ = r1; role2_ = r2; }

    virtual void pressedOK() override { exitWithResult(0); }
    virtual void pressedCancel() override { exitWithResult(-1); }

    virtual void draw() override;

private:
    template<typename T> void showOneDifference(T& pro1, const std::string& format_str, int size, BP_Color c, int& x, int& y, int force = 0)
    {
        //ע�⣬���²���������ȫ�����Լ����Ű�
        auto diff = (char*)&pro1 - (char*)role1_;
        if (diff > sizeof(Role) || diff < 0) { return; }
        auto p1 = pro1;
        auto p2 = *(T*)(diff + (char*)role2_);
        if (p1 != p2 || force)
        {
            auto str = convert::formatString(format_str.c_str(), p1, p2);
            Font::getInstance()->draw(str, size, x, y, c);
            y += size + 5;
        }
    }
    Head* head1_ = nullptr;
    Head* head2_ = nullptr;

};


