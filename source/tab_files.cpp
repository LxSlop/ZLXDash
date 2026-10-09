#include <string.h>
#include <strings.h>
#include <algorithm>

#include <ListView.h>
#include <Launcher.h>
#include <vfs.h>

#include "dash.h"

using namespace ZLX;

// two independent panes over the whole VFS; an empty path lists the devices
struct Pane {
    std::string path;
    ListView list;

    static bool Less(const Row &a, const Row &b) {
        if (a.dir != b.dir)
            return a.dir;
        return strcasecmp(a.name.c_str(), b.name.c_str()) < 0;
    }

    void Load() {
        list.rows.clear();
        list.sel = list.top = 0;
        if (path.empty()) {
            for (int n = 0; vfs_device(n); n++) {
                Row r;
                r.name = vfs_device(n)->name;
                r.right = "<DEV>";
                r.dir = true;
                list.rows.push_back(r);
            }
            return;
        }
        vfs_dir d;
        char name[VFS_NAME_MAX];
        struct stat st;
        if (vfs_opendir(&d, path.c_str()))
            return;
        while (!vfs_readdir(&d, name, &st)) {
            Row r;
            r.name = name;
            r.dir = S_ISDIR(st.st_mode);
            r.right = r.dir ? "<DIR>" : FormatSize(st.st_size);
            list.rows.push_back(r);
        }
        vfs_closedir(&d);
        std::sort(list.rows.begin(), list.rows.end(), Less);
    }

    void Enter(const std::string &name) {
        path = path.empty() ? name + ":/" : path + name + "/";
        Load();
    }

    void Up() {
        if (path.empty())
            return;
        size_t colon = path.find(":/");
        std::string p = path.substr(0, path.size() - 1);       // without the trailing '/'
        size_t slash = p.rfind('/');
        path = (slash == std::string::npos || slash <= colon + 1) ? "" : p.substr(0, slash + 1);
        Load();
    }
};

class FilesTab : public Tab {
    Pane pane[2];
    int active;
    bool loaded;

public:
    FilesTab() : Tab("Files"), active(0), loaded(false) {}

    void Enter() {
        if (!loaded) {
            pane[0].Load();
            pane[1].Load();
            loaded = true;
        }
    }

    void Update(Dash &d, Input &in) {
        Pane &p = pane[active];
        if (in.up) p.list.Move(-1, VISIBLE_ROWS);
        if (in.down) p.list.Move(1, VISIBLE_ROWS);
        if (ZLX_HIT(in, left)) active = 0;
        if (ZLX_HIT(in, right)) active = 1;
        if (ZLX_HIT(in, b)) p.Up();
        if (ZLX_HIT(in, y)) p.Load();
        if (ZLX_HIT(in, x)) {                       // show the other pane here
            pane[1 - active].path = p.path;
            pane[1 - active].Load();
        }
        if (ZLX_HIT(in, a) && !p.list.rows.empty()) {
            const Row &r = p.list.rows[p.list.sel];
            if (r.dir)
                p.Enter(r.name);
            else {
                Launcher::Launch(d, (p.path + r.name).c_str());
            }
        }
    }

    void Render(Dash &d) {
        for (int i = 0; i < 2; i++) {
            float w = (SAFE_W - 20) / 2, x = i ? w + 20 : 0;
            std::string title = pane[i].path.empty() ? "Devices" : pane[i].path;
            d.Text(FitText(d, title, w, 0.85f), i == active ? C_ACCENT : C_DIM, x + 8, 48, 0.85f);
            pane[i].list.Render(d, x, 95, w, VISIBLE_ROWS, i == active);
        }
        d.Text("A: open/launch   B: up   LEFT/RIGHT: pane   X: other pane here   Y: refresh", C_DIM, 8, 575, 0.8f);
    }
};

Tab *NewFilesTab() { return new FilesTab(); }
