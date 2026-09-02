#include <haard/resolution_table/resolution_table.h>

using namespace haard;

ResolutionTable::ResolutionTable() {
    resolutions.push_back(Resolution{0, 0, 0});
}

void ResolutionTable::set_type(u32 node, u32 type) {
    if (node == 0) {
        return;
    }

    reach(node);
    resolutions[node].type = type;
}

void ResolutionTable::set_declaration(u32 node, u32 module, u32 candidate) {
    if (node == 0) {
        return;
    }

    reach(node);
    resolutions[node].module = module;
    resolutions[node].candidate = candidate;
}

Resolution* ResolutionTable::get(u32 node) {
    if (node >= resolutions.size()) {
        return &resolutions[0];
    }

    return &resolutions[node];
}

u32 ResolutionTable::get_count() {
    return (u32) resolutions.size();
}

void ResolutionTable::reach(u32 node) {
    if (node >= resolutions.size()) {
        resolutions.resize(node + 1, Resolution{0, 0, 0});
    }
}
