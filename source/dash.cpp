#include <stdio.h>
#include <Hw.h>
#include <xenon_smc/xenon_smc.h>
extern "C" {
#include <ppc/timebase.h>
}

#include "dash.h"
#include "ftpd.h"

using namespace ZLX;

Dash App;

void Dash::UpdateStatus() {
    uint64_t now = mftb();
    if (statusTime && tb_diff_msec(now, statusTime) < 3000)
        return;
    statusTime = now;

    uint16_t t[4];
    xenon_smc_query_sensors(t);
    snprintf(status, sizeof status, "CPU %d C   GPU %d C   RAM %d C   Board %d C        IP %s  (link %s)        FTP %s",
            t[0] / 256, t[1] / 256, t[2] / 256, t[3] / 256, Hw::LocalIp(), Hw::LinkUp() ? "up" : "down",
            ftpd_running() ? "on" : "off");
}

void Dash::Idle() {
    UpdateStatus();
}

void Dash::Backdrop() {
    float x = 8;
    for (size_t i = 0; i < tabs.size(); i++) {
        bool sel = (int) i == current;
        float w = TextWidth(tabs[i]->name, 1.1f);
        Text(tabs[i]->name, sel ? C_TEXT : C_DIM, x, 0, 1.1f);
        if (sel)
            Rect(x, 34, w, 3, C_ACCENT);
        x += w + 50;
    }
    Text("LB/RB: tab", C_DIM, SAFE_W - 110, 2, 0.8f);

    Rect(-SAFE_X, SAFE_H - 8, SAFE_X * 2 + SAFE_W, 44, 0xFF000000);
    Rect(-SAFE_X, SAFE_H - 8, SAFE_X * 2 + SAFE_W, 1, 0xFF404040);
    Text(status, C_TEXT, 8, SAFE_H, 0.85f);
}

void Dash::Run(const char *resourcePath) {
    Open(resourcePath);

    tabs.push_back(NewHomebrewTab());
    tabs.push_back(NewLinuxTab());
    tabs.push_back(NewFilesTab());
    tabs.push_back(NewSystemTab());
    tabs.push_back(NewSettingsTab());
    current = 0;
    tabs[current]->Enter();

    for (;;) {
        Pump();
        if (!tabs[current]->Modal() && (ZLX_HIT(in, lb) || ZLX_HIT(in, rb))) {
            current = (current + (ZLX_HIT(in, rb) ? 1 : (int) tabs.size() - 1)) % tabs.size();
            tabs[current]->Enter();
        } else {
            tabs[current]->Update(*this, in);
        }
        BeginFrame();
        Backdrop();
        tabs[current]->Render(*this);
        EndFrame();
    }
}
