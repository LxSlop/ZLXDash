#include <strings.h>
#include <ListView.h>
#include <Kboot.h>
#include <vfs.h>

#include "dash.h"

using namespace ZLX;

class LinuxTab : public Tab {
    ListView list;
    std::vector<Kboot> confs;
    std::vector<std::pair<int, int> > rowEntry;   // row -> (conf, entry)
    bool scanned;

    // linux/kboot.conf (or xell.conf) on every device
    void Scan() {
        static const char *names[] = {"linux/kboot.conf", "linux/xell.conf"};
        list.rows.clear();
        rowEntry.clear();
        confs.clear();
        list.sel = list.top = 0;
        for (int n = 0; vfs_device(n); n++)
            for (int k = 0; k < 2; k++) {
                Kboot c;
                if (!c.Load((std::string(vfs_device(n)->name) + ":/" + names[k]).c_str()))
                    continue;
                confs.push_back(c);
                for (size_t i = 0; i < c.entries.size(); i++) {
                    Row r;
                    r.name = (int) i == c.defaultIdx ? "* " : "  ";
                    r.name += c.entries[i].label;
                    r.right = c.path;
                    list.rows.push_back(r);
                    rowEntry.push_back(std::make_pair((int) confs.size() - 1, (int) i));
                }
            }
        scanned = true;
    }

public:
    LinuxTab() : Tab("Linux"), scanned(false) {}

    void Enter() { if (!scanned) Scan(); }

    void Update(Dash &d, Input &in) {
        if (in.up) list.Move(-1, VISIBLE_ROWS);
        if (in.down) list.Move(1, VISIBLE_ROWS);
        if (ZLX_HIT(in, x)) Scan();
        if (ZLX_HIT(in, a) && !list.rows.empty() && d.Warning("Boot this entry?")) {
            const Kboot &c = confs[rowEntry[list.sel].first];
            c.Boot(d, c.entries[rowEntry[list.sel].second]);
        }
    }

    void Render(Dash &d) {
        d.Text("kboot.conf entries on all filesystems", C_DIM, 8, 48, 0.85f);
        if (list.rows.empty())
            d.Text("No linux/kboot.conf found.", C_TEXT, 8, 95);
        else {
            const Kboot &c = confs[rowEntry[list.sel].first];
            const KbootEntry &e = c.entries[rowEntry[list.sel].second];
            d.Text("kernel: " + FitText(d, e.kernel, SAFE_W - 90, 0.8f), C_TEXT, 8, 500, 0.8f);
            d.Text("initrd: " + FitText(d, e.initrd, SAFE_W - 90, 0.8f), C_TEXT, 8, 522, 0.8f);
            d.Text("args:   " + FitText(d, (e.root.empty() ? "" : "root=" + e.root + " ") + e.params, SAFE_W - 90, 0.8f), C_TEXT, 8, 544, 0.8f);
        }
        list.Render(d, 0, 95, SAFE_W, VISIBLE_ROWS - 4, true);
        d.Text("A: boot    X: rescan    *: default", C_DIM, 8, 575, 0.8f);
    }
};

Tab *NewLinuxTab() { return new LinuxTab(); }
