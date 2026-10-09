#include <string.h>
#include <strings.h>
#include <ListView.h>
#include <Launcher.h>
#include <vfs.h>

#include "dash.h"

using namespace ZLX;

static bool IsElfName(const char *n) {
    size_t l = strlen(n);
    return (l > 4 && !strcasecmp(n + l - 4, ".elf")) || (l > 6 && !strcasecmp(n + l - 6, ".elf32"));
}

class HomebrewTab : public Tab {
    ListView list;
    bool scanned;

    // elf files of dir and of its sub directories (depth 1)
    void AddElfs(const std::string &dir, int depth) {
        vfs_dir d;
        char name[VFS_NAME_MAX];
        struct stat st;
        if (vfs_opendir(&d, dir.c_str()))
            return;
        std::vector<std::string> subdirs;
        while (!vfs_readdir(&d, name, &st)) {
            if (S_ISDIR(st.st_mode))
                subdirs.push_back(name);
            else if (IsElfName(name)) {
                Row r;
                r.path = r.name = dir + name;
                r.right = FormatSize(st.st_size);
                list.rows.push_back(r);
            }
        }
        vfs_closedir(&d);
        if (depth < 1)
            for (size_t i = 0; i < subdirs.size(); i++)
                AddElfs(dir + subdirs[i] + "/", depth + 1);
    }

    // sub directories of dir
    static std::vector<std::string> Dirs(const std::string &dir) {
        std::vector<std::string> dirs;
        vfs_dir d;
        char name[VFS_NAME_MAX];
        struct stat st;
        if (vfs_opendir(&d, dir.c_str()) == 0) {
            while (!vfs_readdir(&d, name, &st))
                if (S_ISDIR(st.st_mode))
                    dirs.push_back(name);
            vfs_closedir(&d);
        }
        return dirs;
    }

    // a folder called libxenon in the root of a device or directly below one of its folders
    void Scan() {
        list.rows.clear();
        list.sel = list.top = 0;
        for (int n = 0; vfs_device(n); n++) {
            std::string root = std::string(vfs_device(n)->name) + ":/";
            std::vector<std::string> dirs = Dirs(root);
            for (size_t i = 0; i < dirs.size(); i++) {
                if (!strcasecmp(dirs[i].c_str(), "libxenon"))
                    AddElfs(root + dirs[i] + "/", 0);
                else {
                    std::vector<std::string> sub = Dirs(root + dirs[i] + "/");
                    for (size_t k = 0; k < sub.size(); k++)
                        if (!strcasecmp(sub[k].c_str(), "libxenon"))
                            AddElfs(root + dirs[i] + "/" + sub[k] + "/", 0);
                }
            }
        }
        scanned = true;
    }

public:
    HomebrewTab() : Tab("Homebrew"), scanned(false) {}

    void Enter() { if (!scanned) Scan(); }

    void Update(Dash &d, Input &in) {
        if (in.up) list.Move(-1, VISIBLE_ROWS);
        if (in.down) list.Move(1, VISIBLE_ROWS);
        if (ZLX_HIT(in, x)) Scan();
        if (ZLX_HIT(in, a) && !list.rows.empty())
            Launcher::Launch(d, list.rows[list.sel].path.c_str());
    }

    void Render(Dash &d) {
        d.Text("libxenon folders on all filesystems", C_DIM, 8, 48, 0.85f);
        if (list.rows.empty())
            d.Text("No .elf found in any \"libxenon\" folder.", C_TEXT, 8, 95);
        list.Render(d, 0, 95, SAFE_W, VISIBLE_ROWS, true);
        d.Text("A: launch    X: rescan", C_DIM, 8, 575, 0.8f);
    }
};

Tab *NewHomebrewTab() { return new HomebrewTab(); }
