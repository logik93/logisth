#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace logisth_ui {
// Intercepts glyphs only. Curves, knobs, images and other vector graphics retain
// their normal antialiasing. All storage and rendering stay on the GUI thread.
class TextGraphics : public juce::LowLevelGraphicsContext {
    juce::LowLevelGraphicsContext &target;
    juce::AffineTransform transform;
    std::vector<juce::AffineTransform> saved;
    bool binary;

  public:
    TextGraphics(juce::LowLevelGraphicsContext &context, bool aliased,
                 juce::AffineTransform initial = {})
        : target(context), transform(initial), binary(aliased) {}
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
        saved.push_back(transform);
        target.saveState();
    }
    void restoreState() override {
        if (!saved.empty()) {
            transform = saved.back();
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
        target.setFill(fill);
    }
    void setOpacity(float opacity) override {
        target.setOpacity(opacity);
    }
    void setInterpolationQuality(juce::Graphics::ResamplingQuality quality) override {
        target.setInterpolationQuality(quality);
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
        if (!binary && !syntheticItalic) {
            target.drawGlyphs(glyphs, positions, runTransform);
            return;
        }
        juce::Path path;
        for (size_t i = 0; i < glyphs.size(); ++i) {
            juce::Path outline;
            face->getOutlineForGlyph(font.getMetricsKind(), glyphs[i], outline);
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
        if (!binary) {
            target.fillPath(path, transform.inverted());
            return;
        }
        const auto clip =
            target.getClipBounds().toFloat().transformedBy(transform).getSmallestIntegerContainer();
        const auto bounds =
            path.getBounds().getSmallestIntegerContainer().expanded(1).getIntersection(clip);
        if (bounds.isEmpty())
            return;
        juce::Image mask(juce::Image::SingleChannel, bounds.getWidth(), bounds.getHeight(), true,
                         juce::SoftwareImageType{});
        {
            juce::Graphics graphics(mask);
            graphics.setColour(juce::Colours::white);
            graphics.fillPath(path, juce::AffineTransform::translation(-float(bounds.getX()),
                                                                       -float(bounds.getY())));
        }
        {
            juce::Image::BitmapData data(mask, juce::Image::BitmapData::readWrite);
            for (int y = 0; y < data.height; ++y)
                for (int x = 0; x < data.width; ++x) {
                    auto *pixel = data.getPixelPointer(x, y);
                    *pixel = *pixel >= 128 ? 255 : 0;
                }
        }
        target.saveState();
        target.clipToImageAlpha(
            mask, juce::AffineTransform::translation(float(bounds.getX()), float(bounds.getY()))
                      .followedBy(transform.inverted()));
        target.fillRect(target.getClipBounds(), false);
        target.restoreState();
    }
};

// Render at the current physical resolution before compositing, so disabling
// glyph AA also works with fractional UI scales and HiDPI. Cache dirty regions.
class TextImage : public juce::CachedComponentImage {
    juce::Component &component;
    juce::Image image;
    juce::RectangleList<int> dirty;
    bool binary;
    float scale = 0;

  public:
    TextImage(juce::Component &owner, bool aliased) : component(owner), binary(aliased) {}
    void paint(juce::Graphics &destination) override {
        const auto nextScale =
            juce::jlimit(.5f, 4.f, destination.getInternalContext().getPhysicalPixelScaleFactor());
        const int width = juce::jmax(1, int(std::ceil(component.getWidth() * nextScale))),
                  height = juce::jmax(1, int(std::ceil(component.getHeight() * nextScale)));
        if (nextScale != scale || image.getWidth() != width || image.getHeight() != height) {
            scale = nextScale;
            image = juce::Image(juce::Image::ARGB, width, height, true, juce::SoftwareImageType{});
            dirty = juce::RectangleList<int>(component.getLocalBounds());
        }
        if (!dirty.isEmpty()) {
            juce::RectangleList<int> pixels;
            for (auto r : dirty)
                pixels.add(r.toFloat()
                               .transformedBy(juce::AffineTransform::scale(scale))
                               .getSmallestIntegerContainer()
                               .expanded(1));
            {
                juce::Graphics native(image);
                TextGraphics text(native.getInternalContext(), binary);
                juce::Graphics graphics(text);
                graphics.reduceClipRegion(pixels);
                graphics.addTransform(juce::AffineTransform::scale(scale));
                component.paintEntireComponent(graphics, false);
            }
            dirty.clear();
        }
        juce::Graphics::ScopedSaveState save(destination);
        destination.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        destination.drawImageTransformed(image, juce::AffineTransform::scale(1 / scale));
    }
    bool invalidateAll() override {
        dirty = juce::RectangleList<int>(component.getLocalBounds());
        return true;
    }
    bool invalidate(const juce::Rectangle<int> &r) override {
        dirty.add(r);
        return true;
    }
    void releaseResources() override {
        image = {};
        invalidateAll();
    }
};
} // namespace logisth_ui
