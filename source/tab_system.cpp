#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
extern "C" {
#include <ppc/timebase.h>
#include <xb360/xb360.h>
#include <xenon_nand/xenon_sfcx.h>
#include <xenon_nand/xenon_config.h>
#include <xenon_smc/xenon_smc.h>
#include <diskio/ata.h>
}

#include <Hw.h>
#include <ListView.h>
#include <Video.h>
#include <vfs.h>

#include "dash.h"

using namespace ZLX;

class SystemTab : public Tab {
    std::vector<std::string> lines;
    uint64_t start, refreshed;

    void Add(const char *label, const char *fmt, ...) {
        char v[96];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(v, sizeof v, fmt, ap);
        va_end(ap);
        lines.push_back(std::string(label) + "\t" + v);
    }

    void Collect() {
        static const char *types[] = {"Xenon", "Zephyr", "Falcon", "Jasper", "Trinity", "Corona", "Winchester"};
        int t = xenon_get_console_type();
        uint8_t mac[6] = {0};
        xenon_config_get_mac_addr(mac);
        int w, h;
        Video::Size(&w, &h);
        unsigned long up = tb_diff_sec(mftb(), start);

        lines.clear();
        Add("Console", "%s (type %d)", t >= 0 && t < 7 ? types[t] : "unknown", t);
        Add("CPU PVR", "%08x", xenon_get_CPU_PVR());
        Add("GPU", "%04x", xenon_get_XenosID());
        Add("RAM", "%u MB", xenon_get_ram_size() / (1024 * 1024));
        Add("Video", "%s, %dx%d", Video::Name(Video::Guess()), w, h);
        Add("AV pack", "%02x", xenon_smc_read_avpack());
        Add("NAND", "%u MB", sfc.size_bytes / (1024 * 1024));
        Add("HDD", "%.1f GB", ata.size / (2.0 * 1024 * 1024));
        Add("MAC", "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Add("IP", "%s", Hw::LocalIp());
        std::string devs;
        for (int n = 0; vfs_device(n); n++)
            devs += std::string(n ? " " : "") + vfs_device(n)->name;
        Add("Devices", "%s", devs.c_str());
        Add("Heap in use", "%s", FormatSize(mallinfo().uordblks).c_str());
        Add("Uptime", "%lu:%02lu:%02lu", up / 3600, up / 60 % 60, up % 60);
        refreshed = mftb();
    }

public:
    SystemTab() : Tab("System"), refreshed(0) { start = mftb(); }

    void Enter() { Collect(); }

    void Update(Dash &d, Input &in) {
        if (tb_diff_msec(mftb(), refreshed) > 1000)
            Collect();
    }

    void Render(Dash &d) {
        float y = 60;
        for (size_t i = 0; i < lines.size(); i++, y += 32) {
            size_t tab = lines[i].find('\t');
            d.Text(lines[i].substr(0, tab), C_DIM, 8, y);
            d.Text(lines[i].substr(tab + 1), C_TEXT, 260, y);
        }
    }
};

Tab *NewSystemTab() { return new SystemTab(); }
