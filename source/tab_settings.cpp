#include <stdio.h>
#include <stdlib.h>
#include <Hw.h>
#include <Video.h>
#include <xenon_smc/xenon_smc.h>
extern "C" {
#include <ppc/timebase.h>
}

#include "dash.h"
#include "ftpd.h"

using namespace ZLX;

#define CONFIRM_SECONDS 10

enum Item { RESOLUTION, BACKGROUND, FTP, XELL, RESTART, SHUTDOWN, NB_ITEMS };

class SettingsTab : public Tab {
    int sel;
    int mode;                   // mode shown in the list
    int active;                 // mode the display is running
    bool pending;               // new mode waiting for confirmation
    uint64_t deadline;

    // label (left column) and value (right column)
    void Label(int i, std::string &label, std::string &value) {
        char b[96];
        value.clear();
        switch (i) {
        case RESOLUTION:
            snprintf(b, sizeof b, "%s  (%dx%d)", Video::Name(mode), Video::Width(mode), Video::Height(mode));
            label = "Resolution";
            value = b;
            break;
        case BACKGROUND: label = "Background"; value = Background::Name(); break;
        case FTP:
            label = "FTP server";
            value = ftpd_running() ? std::string("on   ") + Hw::LocalIp() + "  (A: stop)" : "off  (A: start)";
            break;
        case XELL: label = "Return to xell"; break;
        case RESTART: label = "Restart"; break;
        default: label = "Shutdown"; break;
        }
    }

    void Apply() {
        if (mode == active)
            return;
        Video::Set(mode);
        pending = true;
        deadline = mftb();
    }

    void Activate(Dash &d, int i) {
        switch (i) {
        case RESOLUTION: Apply(); break;
        case BACKGROUND: Background::Next(); break;
        case FTP:
            if (ftpd_running()) ftpd_stop();
            else if (Hw::NetworkStart() == 0) ftpd_start();
            break;
        case XELL: exit(0);
        case RESTART: if (d.Warning("Restart the console?")) xenon_smc_power_reboot(); break;
        case SHUTDOWN: if (d.Warning("Shut the console down?")) xenon_smc_power_shutdown(); break;
        }
    }

public:
    SettingsTab() : Tab("Settings"), sel(0), pending(false) {
        active = mode = Video::Guess();
    }

    bool Modal() { return pending; }

    void Update(Dash &d, Input &in) {
        if (pending) {
            if (in.c.start && in.c.back) {                // confirmed: keep the new mode
                active = mode;
                pending = false;
            } else if (tb_diff_msec(mftb(), deadline) >= CONFIRM_SECONDS * 1000) {
                Video::Set(active);                        // nobody confirmed: go back
                mode = active;
                pending = false;
            }
            return;
        }
        if (in.up) sel = (sel + NB_ITEMS - 1) % NB_ITEMS;
        if (in.down) sel = (sel + 1) % NB_ITEMS;
        int dir = ZLX_HIT(in, right) - ZLX_HIT(in, left);
        if (dir && sel == RESOLUTION) {
            int n = Video::Count();
            do mode = (mode + dir + n) % n; while (!Video::Available(mode) && mode != active);
        }
        if (dir && sel == BACKGROUND)
            Background::Next();
        if (ZLX_HIT(in, a))
            Activate(d, sel);
    }

    void Render(Dash &d) {
        for (int i = 0; i < NB_ITEMS; i++) {
            float y = 60 + i * 36;
            if (i == sel)
                d.Rect(0, y - 4, SAFE_W, 34, C_SELBG);
            std::string label, value;
            Label(i, label, value);
            d.Text(label, i == sel ? C_TEXT : C_DIM, 16, y);
            d.Text(value, i == sel ? C_TEXT : C_DIM, 300, y);
        }
        d.Text("UP/DOWN: select   LEFT/RIGHT: change   A: apply/run", C_DIM, 8, 575, 0.8f);

        if (pending) {
            int left = CONFIRM_SECONDS - (int) (tb_diff_msec(mftb(), deadline) / 1000);
            char b[96];
            d.Rect(150, 180, 800, 220, 0xFF000000);
            d.Text("Keep this resolution?", C_TEXT, 180, 210);
            d.Text("Hold START + BACK to keep it.", C_TEXT, 180, 270, 0.9f);
            snprintf(b, sizeof b, "Reverting to %s in %d s", Video::Name(active), left);
            d.Text(b, C_ACCENT, 180, 330, 0.9f);
        }
    }
};

Tab *NewSettingsTab() { return new SettingsTab(); }
