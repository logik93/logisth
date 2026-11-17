#pragma once
#include "UserSettings.h"
#include <juce_gui_basics/juce_gui_basics.h>
namespace logisth_ui {
// Intercepts glyphs only. Curves, knobs, images and other vector graphics retain
// their normal antialiasing. All storage and rendering stay on the GUI thread.
class TextGraphics : public juce::LowLevelGraphicsContext {
    juce::LowLevelGraphicsContext &target;
    juce::AffineTransform transform;
    struct State {
        juce::AffineTransform transform;
        juce::FillType fill;
    };
    juce::FillType fill;
    std::vector<State> saved;
    logisth::UserSettings::Text settings;
    std::array<uint8_t, 256> coverage{};

  public:
    TextGraphics(juce::LowLevelGraphicsContext &context, logisth::UserSettings::Text preferences,
                 juce::AffineTransform initial = {})
        : target(context), transform(initial), settings(preferences) {
        for (int i = 0; i < 256; ++i) {
            const auto value = std::pow(double(i) / 255, 1 / settings.edgeWeight);
            coverage[size_t(i)] = uint8_t(settings.antialias ? std::lround(value * 255)
                                          : value >= .5      ? 255
                                                             : 0);
        }
    }
    TextGraphics(juce::LowLevelGraphicsContext &context, bool aliased,
                 juce::AffineTransform initial = {})
        : TextGraphics(
              context,
              [aliased] {
                  logisth::UserSettings::Text p;
                  p.antialias = !aliased;
                  p.quality = 0;
                  return p;
              }(),
              initial) {}
    bool isVectorDevice() const override {
        return target.isVectorDevice();
    }
    void setOrigin(juce::Point<int> p) override {
        transform = juce::AffineTransform::translation(p.toFloat()).followedBy(transform);
        target.setOrigin(p);
    }
    void addTransform(const juce::AffineTransform &t) override {
        transform = t.followedBy(transform);
        target.addTransform(t);
    }
    float getPhysicalPixelScaleFactor() const override {
        return target.getPhysicalPixelScaleFactor();
    }
    bool clipToRectangle(const juce::Rectangle<int> &r) override {
        return target.clipToRectangle(r);
    }
    bool clipToRectangleList(const juce::RectangleList<int> &r) override {
        return target.clipToRectangleList(r);
    }
    void excludeClipRectangle(const juce::Rectangle<int> &r) override {
        target.excludeClipRectangle(r);
    }
    void clipToPath(const juce::Path &p, const juce::AffineTransform &t) override {
        target.clipToPath(p, t);
    }
    void clipToImageAlpha(const juce::Image &i, const juce::AffineTransform &t) override {
        target.clipToImageAlpha(i, t);
    }
    bool clipRegionIntersects(const juce::Rectangle<int> &r) override {
        return target.clipRegionIntersects(r);
    }
    juce::Rectangle<int> getClipBounds() const override {
        return target.getClipBounds();
    }
    bool isClipEmpty() const override {
        return target.isClipEmpty();
    }
    void saveState() override {
        saved.push_back({transform, fill});
        target.saveState();
    }
    void restoreState() override {
        if (!saved.empty()) {
            transform = saved.back().transform;
            fill = saved.back().fill;
            saved.pop_back();
        }
        target.restoreState();
    }
    void beginTransparencyLayer(float opacity) override {
        target.beginTransparencyLayer(opacity);
    }
    void endTransparencyLayer() override {
        target.endTransparencyLayer();
    }
    void setFill(const juce::FillType &fill) override {
        this->fill = fill;
        target.setFill(fill);
    }
    void setOpacity(float opacity) override {
        fill.setOpacity(opacity);
        target.setOpacity(opacity);
    }
    void setInterpolationQuality(juce::Graphics::ResamplingQuality quality) override {
        target.setInterpolationQuality(quality);
    }
    void fillAll() override {
        target.fillAll();
    }
    void drawRect(const juce::Rectangle<float> &r, float thickness) override {
        target.drawRect(r, thickness);
    }
    void strokePath(const juce::Path &p, const juce::PathStrokeType &stroke,
                    const juce::AffineTransform &t) override {
        target.strokePath(p, stroke, t);
    }
    void drawLineWithThickness(const juce::Line<float> &line, float thickness) override {
        target.drawLineWithThickness(line, thickness);
    }
    void drawRoundedRectangle(const juce::Rectangle<float> &r, float corner,
                              float thickness) override {
        target.drawRoundedRectangle(r, corner, thickness);
    }
    void fillRoundedRectangle(const juce::Rectangle<float> &r, float corner) override {
        target.fillRoundedRectangle(r, corner);
    }
    void drawEllipse(const juce::Rectangle<float> &r, float thickness) override {
        target.drawEllipse(r, thickness);
    }
    void fillEllipse(const juce::Rectangle<float> &r) override {
        target.fillEllipse(r);
    }
    void fillRect(const juce::Rectangle<int> &r, bool replace) override {
        target.fillRect(r, replace);
    }
    void fillRect(const juce::Rectangle<float> &r) override {
        target.fillRect(r);
    }
    void fillRectList(const juce::RectangleList<float> &r) override {
        target.fillRectList(r);
    }
    void fillPath(const juce::Path &p, const juce::AffineTransform &t) override {
        target.fillPath(p, t);
    }
    void drawImage(const juce::Image &i, const juce::AffineTransform &t) override {
        target.drawImage(i, t);
    }
    void drawLine(const juce::Line<float> &line) override {
        target.drawLine(line);
    }
    void setFont(const juce::Font &font) override {
        target.setFont(font);
    }
    const juce::Font &getFont() override {
        return target.getFont();
    }
    uint64_t getFrameId() const override {
        return target.getFrameId();
    }
    void drawGlyphs(juce::Span<const uint16_t> glyphs,
                    juce::Span<const juce::Point<float>> positions,
                    const juce::AffineTransform &runTransform) override {
        const auto font = target.getFont();
        const auto face = font.getTypefacePtr();
        const bool syntheticItalic = font.isItalic() &&
                                     !face->getStyle().containsIgnoreCase("italic") &&
                                     !face->getStyle().containsIgnoreCase("oblique");
        if (settings.antialias && settings.quality == 0 && settings.edgeWeight == 1 &&
            !syntheticItalic) {
            target.drawGlyphs(glyphs, positions, runTransform);
            return;
        }
        juce::Path path;
        for (size_t i = 0; i < glyphs.size(); ++i) {
            juce::Path outline;
            face->getOutlineForGlyph(font.getMetricsKind(), glyphs[i], outline);
            if (outline.isEmpty()) {
                target.drawGlyphs({glyphs.data() + i, 1}, {positions.data() + i, 1}, runTransform);
                continue;
            }
            auto t =
                syntheticItalic ? juce::AffineTransform::shear(-.18f, 0) : juce::AffineTransform{};
            t = t.scaled(font.getHeight() * font.getHorizontalScale(), font.getHeight())
                    .translated(positions[i])
                    .followedBy(runTransform)
                    .followedBy(transform);
            path.addPath(outline, t);
        }
        if (path.isEmpty())
            return;
        const auto clip =
            target.getClipBounds().toFloat().transformedBy(transform).getSmallestIntegerContainer();
        const auto bounds =
            path.getBounds().getSmallestIntegerContainer().expanded(1).getIntersection(clip);
        if (bounds.isEmpty())
            return;
        const int supersample = !settings.antialias     ? 1
                                : settings.quality == 3 ? 4
                                : settings.quality == 2 ? 2
                                                        : 1;
        juce::Image high(juce::Image::SingleChannel, bounds.getWidth() * supersample,
                         bounds.getHeight() * supersample, true, juce::SoftwareImageType{});
        {
            juce::Graphics graphics(high);
            graphics.setColour(juce::Colours::white);
            graphics.fillPath(path, juce::AffineTransform::translation(-float(bounds.getX()),
                                                                       -float(bounds.getY()))
                                        .scaled(float(supersample)));
        }
        juce::Image mask(juce::Image::SingleChannel, bounds.getWidth(), bounds.getHeight(), true,
                         juce::SoftwareImageType{});
        {
            juce::Image::BitmapData source(high, juce::Image::BitmapData::readOnly);
            juce::Image::BitmapData output(mask, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < output.height; ++y)
                for (int x = 0; x < output.width; ++x) {
                    unsigned sum = 0;
                    for (int dy = 0; dy < supersample; ++dy)
                        for (int dx = 0; dx < supersample; ++dx)
                            sum +=
                                *source.getPixelPointer(x * supersample + dx, y * supersample + dy);
                    const auto average = (sum + unsigned(supersample * supersample / 2)) /
                                         unsigned(supersample * supersample);
                    *output.getPixelPointer(x, y) = coverage[average];
                }
        }
        // Colour the physical-pixel mask in software, then draw the small glyph
        // image directly. Native alpha-clip layers can filter the mask again and
        // alter subsequent primitive rendering (notably Direct2D bitmap brushes).
        juce::Image ink(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true,
                        juce::SoftwareImageType{});
        {
            juce::Graphics graphics(ink);
            graphics.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
            graphics.reduceClipRegion(mask, {});
            auto paint = fill;
            paint.transform = paint.transform.followedBy(transform).translated(
                -float(bounds.getX()), -float(bounds.getY()));
            graphics.setFillType(paint);
            graphics.fillRect(ink.getBounds());
        }
        target.saveState();
        target.setOpacity(1); // opacity is already in the coloured glyph pixels
        target.setInterpolationQuality(juce::Graphics::lowResamplingQuality);
        target.drawImage(
            ink, juce::AffineTransform::translation(float(bounds.getX()), float(bounds.getY()))
                     .followedBy(transform.inverted()));
        target.restoreState();
    }
};

// Intercept text during the normal component paint. Do not rasterise the whole
// interface and then resize it: that produced uneven text/needles at fractional
// UI scales, stale alpha pixels, and an unnecessary full-window bitmap.
class TextImage : public juce::CachedComponentImage {
    juce::Component &component;
    logisth::UserSettings::Text settings;

  public:
    TextImage(juce::Component &owner, logisth::UserSettings::Text preferences)
        : component(owner), settings(preferences) {}
    TextImage(juce::Component &owner, bool aliased)
        : TextImage(owner, [aliased] {
              logisth::UserSettings::Text p;
              p.antialias = !aliased;
              p.quality = 0;
              return p;
          }()) {}
    void paint(juce::Graphics &destination) override {
        auto initial = component.getTransform().withAbsoluteTranslation(0, 0);
        const auto ownScale = std::sqrt(std::abs(initial.getDeterminant()));
        const auto physical = destination.getInternalContext().getPhysicalPixelScaleFactor();
        initial = initial.scaled(ownScale > 0 ? physical / ownScale : physical);
        TextGraphics text(destination.getInternalContext(), settings, initial);
        juce::Graphics graphics(text);
        component.paintEntireComponent(graphics, false);
    }
    bool invalidateAll() override {
        return true;
    }
    bool invalidate(const juce::Rectangle<int> &) override {
        return true;
    }
    void releaseResources() override {}
};
} // namespace logisth_ui
