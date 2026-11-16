#pragma once
#include "ProcessorPhaseTests.h"
namespace features0961_test {
inline void run() {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool ok, const char *m) {
        if (!ok)
            throw std::runtime_error(m);
    };
    auto contract = JSON::parse(
        File(String(LOGISTH_SOURCE_DIR)).getChildFile("tests/fixtures/parameters-0.9.6.1.json"));
    check(contract.isArray() && contract.size() == 1055 && parameterCount >= 1055,
          "complete v0.9.6.1 compatibility baseline");
    auto instance = std::make_unique<LogisthProcessor>();
    auto &p = *instance;
    for (int i = 0; i < contract.size(); ++i) {
        auto row = contract[i];
        const auto &s = parameterSpecs[size_t(i)];
        check(row["id"].toString() == s.id && int(row["index"]) == i &&
                  row["choices"].toString() == s.choices &&
                  std::abs(double(row["low"]) - s.low) < 1e-6 &&
                  std::abs(double(row["high"]) - s.high) < 1e-6 &&
                  std::abs(double(row["initial"]) - s.initial) < 1e-6 &&
                  std::abs(double(row["skew"]) - s.skew) < 1e-6 &&
                  bool(row["integer"]) == s.integer &&
                  p.getParameters()[i] == p.state.getParameter(s.id),
              "new frozen host contract retains every field/index");
    }
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    editor->snapshotPage(23);
    auto *console = ui->getChildComponent(0);
    for (int macro = 1; macro <= 4; ++macro) {
        String id = "m" + String(macro) + "target1";
        ComboBox *menu = nullptr;
        for (auto *c : console->findChildWithID(id)->getChildren())
            if (auto *m = dynamic_cast<ComboBox *>(c))
                menu = m;
        check(menu && menu->getNumItems() == 50, "all macros offer symmetric A/B/C destinations");
        for (int target = 0; target < 50; ++target) {
            menu->setSelectedId(target + 1, sendNotificationSync);
            check(p.effectiveValue(Param(size_t(Param::m1target1) + (macro - 1) * 13)) == target,
                  "every grouped macro leaf reaches its semantic target");
        }
    }
    for (int macro = 1; macro <= 4; ++macro) {
        gain_test::set(p, "m" + String(macro) + "target1", 0);
        gain_test::set(p, "m" + String(macro) + "target1cx", 0);
    }
    // A/B/C local filters and mixer targets have identical macro scaling.
    for (int index : {41, 42, 43, 44, 45, 46, 47, 48, 49}) {
        gain_test::set(p, "m1target1", index <= 30 ? float(index) : 0.f);
        gain_test::set(p, "m1target1cx", float(index - 30));
        gain_test::set(p, "m1low1", 0);
        gain_test::set(p, "m1high1", 1);
        gain_test::set(p, "macro1", .5f);
        auto target = macroTargets[size_t(index)];
        const auto &s = parameterSpecs[size_t(target)];
        float expected = s.low + (s.high - s.low) * std::pow(.5f, 1.f / s.skew);
        check(std::abs(p.effectiveValue(target) - expected) < 1e-3,
              "part-local C/A/B macro targets use the same host scaling");
    }
    std::cout << "PASS v0.9.6.1 baseline: 1055 frozen definitions, 200 grouped macro choices and "
                 "symmetric A/B/C mappings\n";
}
} // namespace features0961_test
