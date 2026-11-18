#pragma once
#include "SpectrumPlot.h"
namespace logisth_ui {
class DistortionView : public juce::Component, public juce::SettableTooltipClient {
    LogisthProcessor &processor;
    const LogisthColours &theme;
    std::function<int()> count;
    logisth::EqSpectrum spectrum;
    juce::RangedAudioParameter *gesture = nullptr;
    int selected = 0, divider = 0, dragging = -1, analysis = 3, tilt = 2;
    juce::Rectangle<float> plot() const {
        return {6, 7, float(getWidth() - 12), float(getHeight() - 25)};
    }
    double sampleRate() const {
        const double rate = processor.getSampleRate();
        return rate > 0 ? std::max(1000., rate) : 44100.;
    }
    std::array<float, 3> raw() const {
        std::array<float, 3> r;
        for (int i = 0; i < 3; ++i)
            r[size_t(i)] =
                processor.state.getRawParameterValue("distcross" + juce::String(i + 1))->load();
        return r;
    }
    float x(double hz) const {
        return plot().getX() + float(std::log(hz / 20) / std::log(1000.)) * plot().getWidth();
    }
    int nearest(float px) const {
        const auto cuts = logisth::DistortionCrossover::cutoffs(raw(), count(), sampleRate());
        int found = -1;
        for (int i = 0; i < count() - 1; ++i)
            if (std::abs(px - x(cuts[size_t(i)])) < 11 &&
                (found < 0 ||
                 std::abs(px - x(cuts[size_t(i)])) < std::abs(px - x(cuts[size_t(found)]))))
                found = i;
        return found;
    }
    void finish() {
        if (gesture)
            gesture->endChangeGesture();
        gesture = nullptr;
        dragging = -1;
    }
    void change(int cut, double hz) {
        const auto cuts = logisth::DistortionCrossover::cutoffs(raw(), count(), sampleRate());
        const double low = cut == 0 ? 20 : cuts[size_t(cut - 1)] * 1.03;
        const double high = cut == count() - 2 ? std::min(20000., sampleRate() * .45)
                                               : cuts[size_t(cut + 1)] / 1.03;
        auto *parameter = processor.state.getParameter("distcross" + juce::String(cut + 1));
        parameter->setValueNotifyingHost(
            parameter->convertTo0to1(float(std::clamp(hz, low, high))));
        repaint();
    }

  public:
    std::function<void(int)> onSelectionChanged;
    DistortionView(LogisthProcessor &p, const LogisthColours &c, std::function<int()> number)
        : processor(p), theme(c), count(std::move(number)) {
        setComponentID("distortion.graph");
        setTooltip("Click a band to edit its module. Drag a vertical crossover; wheel adjusts the "
                   "selected divider. Double-click resets that divider. Pre/post are stereo-power "
                   "spectra; crossover curves use a separate relative scale.");
    }
    ~DistortionView() override {
        finish();
    }
    void selectBand(int b) {
        selected = juce::jlimit(0, count() - 1, b);
        repaint();
    }
    void setAnalyzer(int mode, int slope) {
        analysis = mode;
        tilt = slope;
    }
    void updateSpectrum() {
        if (analysis && spectrum.update(processor.distortionSpectrum, sampleRate(), analysis))
            repaint();
    }
    void paint(juce::Graphics &g) override {
        g.setColour(theme.screen);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        const auto area = plot();
        const auto input = raw();
        const auto cuts = logisth::DistortionCrossover::cutoffs(input, count(), sampleRate());
        const float lo = selected == 0 ? area.getX() : x(cuts[size_t(selected - 1)]);
        const float hi = selected == count() - 1 ? area.getRight() : x(cuts[size_t(selected)]);
        g.setColour(theme.screenAccent.withAlpha(.09f));
        g.fillRect(juce::Rectangle<float>(lo, area.getY(), hi - lo, area.getHeight()));
        g.setColour(theme.screenLine.withAlpha(.55f));
        for (double hz : {100., 1000., 10000.})
            g.drawVerticalLine(int(x(hz)), area.getY(), area.getBottom());
        {
            juce::Graphics::ScopedSaveState scope(g);
            g.reduceClipRegion(area.toNearestInt());
            drawStereoSpectrum(g, area, theme, spectrum, analysis, tilt, sampleRate());
            std::array<juce::Path, 4> paths;
            for (int point = 0; point < 192; ++point) {
                const double hz = 20 * std::pow(1000., point / 191.);
                if (hz >= sampleRate() * .5)
                    break;
                const auto response =
                    logisth::DistortionCrossover::response(hz, count(), input, sampleRate());
                for (int band = 0; band < count(); ++band) {
                    const float db = float(std::max(
                        -60., 20 * std::log10(std::max(1e-8, std::abs(response[size_t(band)])))));
                    const float yy = area.getY() + 4 - db / 60 * (area.getHeight() - 7);
                    if (point == 0)
                        paths[size_t(band)].startNewSubPath(x(hz), yy);
                    else
                        paths[size_t(band)].lineTo(x(hz), yy);
                }
            }
            for (int band = 0; band < count(); ++band) {
                g.setColour((band == selected ? theme.screenText : theme.muted)
                                .withAlpha(band == selected ? .9f : .5f));
                g.strokePath(paths[size_t(band)],
                             juce::PathStrokeType(band == selected ? 1.4f : 1.f));
            }
        }
        for (int i = 0; i < count() - 1; ++i) {
            g.setColour(i == divider ? theme.screenAccent : theme.mint.withAlpha(.7f));
            g.drawLine(x(cuts[size_t(i)]), area.getY(), x(cuts[size_t(i)]), area.getBottom(), 1.2f);
            g.fillEllipse(x(cuts[size_t(i)]) - 3, area.getY() + 1, 6, 6);
        }
        theme.drawCaption(g, "20", {4, getHeight() - 17, 40, 15}, 8, theme.muted);
        theme.drawCaption(g, "1k", {int(x(1000)) - 18, getHeight() - 17, 36, 15}, 8, theme.muted,
                          juce::Justification::centred);
        theme.drawCaption(g, "20k Hz", {getWidth() - 65, getHeight() - 17, 60, 15}, 8, theme.muted,
                          juce::Justification::centredRight);
        if (count() > 1)
            theme.drawCaption(
                g,
                "C" + juce::String(divider + 1) + "  " +
                    juce::String(cuts[size_t(juce::jlimit(0, count() - 2, divider))], 0) + " Hz",
                {60, getHeight() - 17, 160, 15}, 8, theme.screenText);
    }
    void mouseDown(const juce::MouseEvent &e) override {
        finish();
        dragging = nearest(e.position.x);
        if (dragging >= 0) {
            divider = dragging;
            gesture = processor.state.getParameter("distcross" + juce::String(dragging + 1));
            gesture->beginChangeGesture();
        } else {
            const auto cuts = logisth::DistortionCrossover::cutoffs(raw(), count(), sampleRate());
            int band = 0;
            while (band < count() - 1 && e.position.x > x(cuts[size_t(band)]))
                ++band;
            selected = band;
            if (onSelectionChanged)
                onSelectionChanged(band);
        }
        repaint();
    }
    void mouseDrag(const juce::MouseEvent &e) override {
        if (dragging >= 0)
            change(dragging,
                   20 * std::pow(1000., (e.position.x - plot().getX()) / plot().getWidth()));
    }
    void mouseUp(const juce::MouseEvent &) override {
        finish();
    }
    void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel) override {
        if (count() < 2)
            return;
        divider = juce::jlimit(0, count() - 2, divider);
        auto *p = processor.state.getParameter("distcross" + juce::String(divider + 1));
        p->beginChangeGesture();
        change(divider,
               raw()[size_t(divider)] * std::pow(2., wheel.deltaY * (wheel.isReversed ? -1 : 1)));
        p->endChangeGesture();
    }
    void mouseDoubleClick(const juce::MouseEvent &e) override {
        const int near = nearest(e.position.x);
        if (near < 0)
            return;
        auto *p = processor.state.getParameter("distcross" + juce::String(near + 1));
        p->beginChangeGesture();
        change(near, std::array<double, 3>{200, 2000, 8000}[size_t(near)]);
        p->endChangeGesture();
    }
};
} // namespace logisth_ui
