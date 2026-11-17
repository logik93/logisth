#pragma once
#include "EqSpectrum.h"
namespace logisth_ui {
class EqView : public juce::Component, public juce::SettableTooltipClient {
    LogisthProcessor &processor;
    const LogisthColours &theme;
    std::function<int()> mode;
    int dragging = -1, selected = 0;
    float dragY = 0, dragX = 0, dragValue = 0, dragFrequency = 1000;
    juce::RangedAudioParameter *gesture = nullptr, *frequencyGesture = nullptr;
    logisth::EqSpectrum spectrum;
    int analysisMode = 3, analysisTilt = 2;
    void drawSpectrum(juce::Graphics &g, juce::Rectangle<float> area) const {
        const double rate = std::max(8000., processor.getSampleRate());
        for (int stream = 0; stream < 2; ++stream) {
            if (!(analysisMode & (1 << stream)))
                continue;
            juce::Path path;
            bool started = false;
            for (int i = 0; i < 384; ++i) {
                const double hz = 20 * std::pow(1000., i / 383.);
                if (hz >= rate * .5)
                    break;
                const double bin = hz * logisth::EqSpectrum::size / rate;
                const auto at = std::min(size_t(bin), logisth::EqSpectrum::bins - 2);
                const float fraction = float(bin - at);
                const auto &values = spectrum.db[size_t(stream)];
                const auto level = values[at] + fraction * (values[at + 1] - values[at]) +
                                   logisth::EqSpectrum::tiltDb(hz, analysisTilt);
                const auto yy = area.getBottom() - float((level + 108) / 120) * area.getHeight();
                if (!started) {
                    path.startNewSubPath(x(hz), yy);
                    started = true;
                } else
                    path.lineTo(x(hz), yy);
            }
            g.setColour((stream == 0 ? theme.mint : theme.screenAccent).withAlpha(.65f));
            g.strokePath(path, juce::PathStrokeType(1.2f));
        }
    }
    float value(const juce::String &id) const {
        return processor.state.getRawParameterValue(id)->load();
    }
    logisth::EqualizerParams settings() const {
        logisth::EqualizerParams p;
        p.eqMode = mode();
        p.eqAutoGain = value("eqautogain") > 0;
        p.eqOutput = value("eqoutput");
        for (int i = 0; i < logisth::eqMaxBands; ++i) {
            auto n = size_t(i);
            auto prefix = "eqb" + juce::String(i);
            p.eqBands[n] = value("eqband" + juce::String(i));
            p.eqTypes[n] = int(value(prefix + "type"));
            p.eqFrequencies[n] = value(prefix + "freq");
            p.eqQ[n] = value(prefix + "q");
            p.eqEnabled[n] = value(prefix + "on") > 0;
        }
        return p;
    }
    double response(const logisth::EqualizerParams &p, double hz, bool output = true) const {
        return logisth::Equalizer::response(p, value("eqbass"), value("eqbassfreq"),
                                            value("eqtreble"), value("eqtreblefreq"), hz,
                                            std::max(44100., processor.getSampleRate()), output);
    }
    juce::Rectangle<float> plot() const {
        return {30, 5, float(getWidth() - 74), float(getHeight() - 26)};
    }
    float x(double hz) const {
        return plot().getX() + float(std::log(hz / 20) / std::log(1000.)) * plot().getWidth();
    }
    float y(double db) const {
        return plot().getCentreY() - float(db) / 48 * plot().getHeight();
    }
    int nearest(float px) const {
        auto p = settings();
        int best = -1;
        for (int i = 0; i < logisth::eqMaxBands; ++i)
            if (p.eqEnabled[size_t(i)] &&
                (best < 0 || std::abs(px - x(p.eqFrequencies[size_t(i)])) <
                                 std::abs(px - x(p.eqFrequencies[size_t(best)]))))
                best = i;
        return best;
    }
    void finish() {
        if (gesture)
            gesture->endChangeGesture();
        if (frequencyGesture)
            frequencyGesture->endChangeGesture();
        gesture = frequencyGesture = nullptr;
        dragging = -1;
    }

  public:
    void setAnalyzer(int selected, int tilt) {
        analysisMode = selected;
        analysisTilt = tilt;
        repaint();
    }
    void updateSpectrum() {
        if (analysisMode &&
            spectrum.update(processor.eqSpectrum, std::max(8000., processor.getSampleRate()),
                            analysisMode))
            repaint();
    }
    std::function<void(int)> onSelectionChanged;
    EqView(LogisthProcessor &p, const LogisthColours &c, std::function<int()> m)
        : processor(p), theme(c), mode(std::move(m)) {
        setComponentID("eq.graph");
        setTooltip("Select a point; drag frequency/gain, wheel for Q, double-click for 0 dB. Add "
                   "bands below. Left: EQ response dB. Right: stereo-power dBFS with the selected "
                   "display tilt.");
    }
    ~EqView() override {
        finish();
    }
    void selectBand(int band) {
        selected = juce::jlimit(0, logisth::eqMaxBands - 1, band);
        repaint();
    }
    void paint(juce::Graphics &g) override {
        g.setColour(theme.screen);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        auto area = plot();
        g.setColour(theme.screenLine);
        for (double hz : {100., 1000., 10000.})
            g.drawVerticalLine(int(x(hz)), area.getY(), area.getBottom());
        for (double db : {-12., 0., 12.})
            g.drawHorizontalLine(int(y(db)), area.getX(), area.getRight());
        auto p = settings();
        double compensation = 0;
        if (p.eqAutoGain)
            for (int i = 0; i < 96; ++i)
                compensation =
                    std::max(compensation, response(p, 20 * std::pow(1000., i / 95.), false));
        {
            juce::Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(area.toNearestInt());
            drawSpectrum(g, area);
            juce::Path curve;
            for (int i = 0; i < 256; ++i) {
                double hz = 20 * std::pow(1000., i / 255.);
                float xx = x(hz), yy = y(response(p, hz) - compensation);
                if (i == 0)
                    curve.startNewSubPath(xx, yy);
                else
                    curve.lineTo(xx, yy);
            }
            g.setColour(theme.screenText);
            g.strokePath(curve, juce::PathStrokeType(1.8f));
            if (p.eqMode == 1)
                for (int i = 0; i < logisth::eqMaxBands; ++i)
                    if (p.eqEnabled[size_t(i)]) {
                        auto n = size_t(i);
                        float yy = juce::jlimit(
                            area.getY() + 4, area.getBottom() - 4,
                            y((p.eqTypes[n] < 3 ? p.eqBands[n] : 0) + p.eqOutput - compensation));
                        g.setColour(i == selected ? theme.mint : theme.screenAccent.withAlpha(.7f));
                        g.fillEllipse(x(p.eqFrequencies[n]) - 3, yy - 3, 6, 6);
                        if (i == selected) {
                            g.drawEllipse(x(p.eqFrequencies[n]) - 6, yy - 6, 12, 12, 1.1f);
                        }
                    }
        }
        theme.drawCaption(g, "+24", {1, 2, 26, 13}, 8, theme.muted);
        theme.drawCaption(g, "-24", {1, int(area.getBottom() - 13), 26, 13}, 8, theme.muted);
        if (analysisMode) {
            const int right = int(area.getRight()) + 3;
            theme.drawCaption(g, "0dBFS",
                              {right, int(area.getY() + area.getHeight() * .1f) - 6, 37, 13}, 7,
                              theme.muted);
            theme.drawCaption(g, "-96",
                              {right, int(area.getY() + area.getHeight() * .9f) - 6, 37, 13}, 7,
                              theme.muted);
        }
        const int axisY = getHeight() - 18;
        theme.drawCaption(g, "20 Hz", {30, axisY, 58, 15}, 8, theme.muted);
        theme.drawCaption(g, "1 kHz", {int(x(1000)) - 30, axisY, 60, 15}, 8, theme.muted,
                          juce::Justification::centred);
        theme.drawCaption(g, "20 kHz", {int(area.getRight()) - 72, axisY, 72, 15}, 8, theme.muted,
                          juce::Justification::centredRight);
    }
    void mouseDown(const juce::MouseEvent &e) override {
        finish();
        if (mode() != 1)
            return;
        dragging = nearest(e.position.x);
        if (dragging < 0)
            return;
        selected = dragging;
        if (onSelectionChanged)
            onSelectionChanged(selected);
        gesture = processor.state.getParameter("eqband" + juce::String(dragging));
        frequencyGesture = processor.state.getParameter("eqb" + juce::String(dragging) + "freq");
        dragY = e.position.y;
        dragX = e.position.x;
        dragValue = value("eqband" + juce::String(dragging));
        dragFrequency = value("eqb" + juce::String(dragging) + "freq");
        gesture->beginChangeGesture();
        frequencyGesture->beginChangeGesture();
    }
    void mouseDrag(const juce::MouseEvent &e) override {
        if (!gesture)
            return;
        float db =
            juce::jlimit(-18.f, 18.f, dragValue + (dragY - e.position.y) * 48 / plot().getHeight());
        if (value("eqb" + juce::String(dragging) + "type") < 3)
            gesture->setValueNotifyingHost(gesture->convertTo0to1(db));
        float hz = juce::jlimit(20.f, 20000.f,
                                dragFrequency *
                                    std::pow(1000.f, (e.position.x - dragX) / plot().getWidth()));
        frequencyGesture->setValueNotifyingHost(frequencyGesture->convertTo0to1(hz));
        repaint();
    }
    void mouseUp(const juce::MouseEvent &) override {
        finish();
    }
    void mouseDoubleClick(const juce::MouseEvent &e) override {
        finish();
        if (mode() != 1)
            return;
        int index = nearest(e.position.x);
        if (index < 0)
            return;
        auto *p = processor.state.getParameter("eqband" + juce::String(index));
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(0));
        p->endChangeGesture();
        repaint();
    }
    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &w) override {
        if (mode() != 1)
            return;
        int index = nearest(e.position.x);
        if (index < 0)
            return;
        auto *p = processor.state.getParameter("eqb" + juce::String(index) + "q");
        float q =
            juce::jlimit(.1f, 20.f, p->convertFrom0to1(p->getValue()) * std::exp(w.deltaY * 2));
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(q));
        p->endChangeGesture();
        repaint();
    }
};
} // namespace logisth_ui
