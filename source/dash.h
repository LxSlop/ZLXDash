#ifndef DASH_H
#define DASH_H

#include <stdint.h>
#include <Ui.h>

class Dash;

struct Tab {
    const char *name;
    Tab(const char *n) : name(n) {}
    virtual ~Tab() {}
    virtual void Enter() {}
    virtual bool Modal() { return false; }   // true: keep the focus (no tab switching)
    virtual void Update(Dash &d, ZLX::Input &in) = 0;
    virtual void Render(Dash &d) = 0;
};

// tabs plus the common frame (tab bar, status bar); everything else comes from ZLX::Ui
class Dash : public ZLX::Ui {
public:
    void Run(const char *resourcePath);

private:
    std::vector<Tab *> tabs;
    int current;
    char status[160];
    uint64_t statusTime;

    void Idle();
    void Backdrop();
    void UpdateStatus();
};

extern Dash App;

Tab *NewHomebrewTab();
Tab *NewLinuxTab();
Tab *NewFilesTab();
Tab *NewSystemTab();
Tab *NewSettingsTab();

// list based tabs
#define VISIBLE_ROWS 18

#endif
