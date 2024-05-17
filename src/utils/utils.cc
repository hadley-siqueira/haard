// for mkdir
#include <cstdlib>
#include <unistd.h>
#include <map>

#include "utils/utils.h"

std::vector<std::string> split(std::string s, char sep) {
    std::string tmp;
    std::vector<std::string> r;

    for (int i = 0; i < s.size(); ++i) {
        if (s[i] == sep) {
            r.push_back(tmp);
            tmp = "";
        } else {
            tmp += s[i];
        }
    }

    r.push_back(tmp);
    return r;
}

std::string uppercase(std::string s) {
    std::string r;

    for (int i = 0; i < s.size(); ++i) {
        r += uppercase(s[i]);
    }

    return r;
}

char uppercase(char c) {
    if (c >= 'a' && c <= 'z') {
        return 'A' + (c - 'a');
    }

    return c;
}

char lowercase(char c) {
    if (c >= 'A' && c <= 'Z') {
        return 'a' + (c - 'A');
    }

    return c;
}

std::string lowercase(std::string s) {
    std::string r;

    for (int i = 0; i < s.size(); ++i) {
        r += lowercase(s[i]);
    }

    return r;
}

std::string join(std::vector<std::string> v, std::string sep) {
    int i;
    std::string r;

    if (v.size() > 0) {
        for (i = 0; i < v.size() - 1; ++i) {
            r += v[i] + sep;
        }

        r += v[i];
    }

    return r;
}

void mkdir(std::string path) {
    std::string cmd = "mkdir " + path;
    system(cmd.c_str());
}

void rmdir(std::string path) {
    std::string cmd = "rm -rf " + path;
    system(cmd.c_str());
}

std::string indent(std::string s, int ws) {
    std::string r;

    for (int i = 0; i < ws; ++i) {
        r += ' ';
    }

    for (int i = 0; i < s.size(); ++i) {
        r += s[i];

        if (s[i] == '\n' && i < s.size() - 1) {
            for (int j = 0; j < ws; ++j) {
                r += ' ';
            }
        }
    }

    return r;
}

std::string colorify(std::string msg, bool no_normal) {
    std::string r;
    std::map<std::string, std::string> color_map;

    color_map["normal"] = "\033[0m";
    color_map["black"] = "\033[30m";
    color_map["red"] = "\033[91m";
    color_map["green"] = "\033[92m";
    color_map["orange"] = "\033[93m";
    color_map["blue"] = "\033[94m";
    color_map["magenta"] = "\033[95m";
    color_map["cyan"] = "\033[96m";
    color_map["white"] = "\033[97m";

    for (int i = 0; i < msg.size(); ++i) {
        if (msg[i] == '<') {
            std::string color;

            ++i;
            while (msg[i] != '>') {
                color += msg[i];
                ++i;
            }

            r += color_map[color];
        } else {
            r += msg[i];
        }
    }

    if (!no_normal) {
        r += color_map["normal"];
    }

    return r;
}
