#pragma once
#include "StudioViews.h"
namespace logisth_ui {
class ImpulseView : public Component, public FileDragAndDropTarget {
    LogisthProcessor &p;
    LogisthColours &c;
    FontMaker font;
    bool over = false;

  public:
    ImpulseView(LogisthProcessor &processor, LogisthColours &colours, FontMaker f)
        : p(processor), c(colours), font(std::move(f)) {
        setComponentID("reverb.ir.waveform");
    }
    bool isInterestedInFileDrag(const StringArray &files) override {
        return files.size() == 1 && File(files[0]).hasFileExtension("wav;aif;aiff;flac");
    }
    void fileDragEnter(const StringArray &, int, int) override {
        over = true;
        repaint();
    }
    void fileDragExit(const StringArray &) override {
        over = false;
        repaint();
    }
    void filesDropped(const StringArray &files, int, int) override {
        over = false;
        if (isInterestedInFileDrag(files))
            p.loadImpulseAsync(File(files[0]));
        repaint();
    }
    void paint(Graphics &g) override {
        g.setColour(c.screen);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        auto data = p.impulseData();
        if (!data)
            return;
        auto area = getLocalBounds().reduced(20, 5).toFloat();
        const float h = area.getHeight() * .5f;
        for (size_t ch = 0; ch < 2; ++ch) {
            float mid = area.getY() + h * (float(ch) + .5f),
                  scale = h * .45f / std::max(.00001f, data->peak);
            g.setColour(c.screenLine);
            g.drawHorizontalLine(int(mid), area.getX(), area.getRight());
            g.setColour(c.mint.withAlpha(ch == 0 ? .95f : .65f));
            for (size_t i = 0; i < 512; ++i) {
                float x = area.getX() + float(i) * area.getWidth() / 512;
                g.drawVerticalLine(int(x), mid - data->maximum[ch][i] * scale,
                                   mid - data->minimum[ch][i] * scale);
            }
            g.setFont(font(9));
            g.drawText(ch == 0 ? "L" : "R", 2, int(mid) - 6, 15, 12, Justification::centred);
        }
        if (over) {
            g.setColour(c.screenAccent.withAlpha(.35f));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
            g.setColour(c.screenText);
            g.setFont(font(12));
            g.drawText("DROP IMPULSE RESPONSE", getLocalBounds(), Justification::centred);
        }
        g.setColour(over ? c.screenAccent : c.screenLine);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f), 4, 1);
    }
};
class DynamicsView : public Component {
    LogisthProcessor &p;
    LogisthColours &c;
    FontMaker font;
    int mode = 0;
    float value(const char *id) const {
        return p.state.getRawParameterValue(id)->load();
    }
    void text(Graphics &g, const String &s, Rectangle<int> r, float size = 10) {
        g.setFont(font(size));
        g.setColour(c.screenText);
        g.drawText(s, r, Justification::centredLeft);
    }

  public:
    DynamicsView(LogisthProcessor &processor, LogisthColours &colours, FontMaker f)
        : p(processor), c(colours), font(std::move(f)) {
        setComponentID("dynamics.view");
    }
    void setMode(int next) {
        mode = next;
        repaint();
    }
    void paint(Graphics &g) override {
        using namespace logisth;
        g.setColour(c.screen);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        const int half = getWidth() / 2;
        auto graph = Rectangle<float>(24, 17, float(half - 34), float(getHeight() - 32));
        g.setColour(c.screenLine);
        for (int i = 0; i <= 4; ++i) {
            float x = graph.getX() + graph.getWidth() * i * .25f,
                  y = graph.getY() + graph.getHeight() * i * .25f;
            g.drawVerticalLine(int(x), graph.getY(), graph.getBottom());
            g.drawHorizontalLine(int(y), graph.getX(), graph.getRight());
        }
        Path curve;
        if (mode == 0) {
            const float threshold = value("compthreshold"), ratio = value("compratio"),
                        knee = value("compmode") > 0 ? value("compknee") : 0;
            float x1 = graph.getX() +
                       std::clamp((threshold - knee * .5f + 60) / 60, 0.f, 1.f) * graph.getWidth();
            float x2 = graph.getX() +
                       std::clamp((threshold + knee * .5f + 60) / 60, 0.f, 1.f) * graph.getWidth();
            g.setColour(c.screenAccent.withAlpha(.2f));
            g.fillRect(
                Rectangle<float>(x1, graph.getY(), std::max(1.f, x2 - x1), graph.getHeight()));
            for (int i = 0; i <= 200; ++i) {
                float in = -60 + float(i) * .3f;
                float wet =
                    in - compressionReduction(in, threshold, ratio, knee) + value("compmakeup");
                float dry = amplitude(in),
                      out = decibels(dry + (amplitude(wet) - dry) * value("compmix"));
                float x = graph.getX() + graph.getWidth() * float(i) / 200,
                      y = graph.getBottom() - (out + 60) / 60 * graph.getHeight();
                if (i == 0)
                    curve.startNewSubPath(x, y);
                else
                    curve.lineTo(x, y);
            }
            text(g, "KNEE " + String(knee, 1) + " dB", {6, 0, half - 12, 15}, 9);
        } else if (mode == 1) {
            const double ceiling = amplitude(value("clipceiling")),
                         knee = value("clipmode") > 0 ? value("clipknee") : 0,
                         drive = amplitude(value("clipdrive"));
            for (int i = 0; i <= 200; ++i) {
                double input = (double(i) / 100 - 1) * 1.5;
                float out =
                    float((input + (clipCurve(input * drive, ceiling, knee, value("clipmode") > 0) -
                                    input) *
                                       value("clipmix")) /
                          1.5);
                float x = graph.getX() + graph.getWidth() * float(i) / 200,
                      y = graph.getCentreY() - out * graph.getHeight() * .5f;
                if (i == 0)
                    curve.startNewSubPath(x, y);
                else
                    curve.lineTo(x, y);
            }
            text(g, "CURVE / KNEE " + String(knee * 100, 0) + "%", {6, 0, half - 12, 15}, 9);
        } else {
            const float ceiling = value("limitceiling");
            for (int i = 0; i <= 200; ++i) {
                float in = -60 + float(i) * .3f, out = std::min(in + value("limitdrive"), ceiling);
                float x = graph.getX() + graph.getWidth() * float(i) / 200,
                      y = graph.getBottom() - (out + 60) / 60 * graph.getHeight();
                if (i == 0)
                    curve.startNewSubPath(x, y);
                else
                    curve.lineTo(x, y);
            }
            text(g, "SAMPLE-PEAK / " + String(ceiling, 1) + " dBFS", {6, 0, half - 12, 15}, 9);
        }
        g.saveState();
        g.reduceClipRegion(graph.toNearestInt());
        g.setColour(c.screenAccent);
        g.strokePath(curve, PathStrokeType(1.8f));
        g.restoreState();
        auto &meter = p.dynamicsMeters[size_t(mode)];
        float reduction = meter.reduction.load();
        auto dbText = [](float v) { return v <= -160 ? String("-inf") : String(v, 1); };
        text(g,
             "IN " + dbText(meter.input.load()) + "  OUT " + dbText(meter.output.load()) + " dBFS",
             {half + 8, 0, half - 16, 15}, 9);
        std::array<float, 512> input{}, output{}, gr{};
        p.dynamicsScope.read(0, input.data(), 512);
        p.dynamicsScope.read(1, output.data(), 512);
        p.dynamicsScope.read(2, gr.data(), 512);
        Rectangle<float> wave(float(half + 8), 20, float(half - 18), float(getHeight() - 46));
        float scale = 1;
        for (float x : input)
            scale = std::max(scale, std::abs(x));
        for (float x : output)
            scale = std::max(scale, std::abs(x));
        for (int channel = 0; channel < 2; ++channel) {
            Path path;
            const auto &samples = channel == 0 ? input : output;
            for (int i = 0; i < 512; ++i) {
                float x = wave.getX() + wave.getWidth() * i / 511;
                float y =
                    mode == 2
                        ? wave.getBottom() - std::min(1.f, gr[size_t(i)] / 24) * wave.getHeight()
                        : wave.getCentreY() - samples[size_t(i)] / scale * wave.getHeight() * .45f;
                if (i == 0)
                    path.startNewSubPath(x, y);
                else
                    path.lineTo(x, y);
            }
            g.setColour(channel == 0 ? c.mint.withAlpha(.35f) : c.screenAccent);
            g.strokePath(path, PathStrokeType(channel == 0 ? 1.f : 1.5f));
        }
        const int barY = getHeight() - 17;
        g.setColour(c.screenLine);
        g.fillRoundedRectangle(float(half + 8), float(barY), float(half - 108), 8, 2);
        g.setColour(c.screenAccent);
        g.fillRoundedRectangle(float(half + 8), float(barY),
                               float(half - 108) * std::clamp(reduction / 24, 0.f, 1.f), 8, 2);
        text(g, "GR " + String(reduction, 1) + " dB", {getWidth() - 92, barY - 3, 90, 14}, 10);
    }
};
} // namespace logisth_ui
