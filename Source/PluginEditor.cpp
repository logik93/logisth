#include "PluginEditor.h"
#include "BackgroundLibrary.h"
#include "BinaryData.h"
#include "EnvelopePlot.h"
#include "FactoryPresets.h"
#include "TextRendering.h"
#include "ThemeSettings.h"
#include <set>
using namespace juce;
using namespace logisth;
namespace {
constexpr int canvasWidth = EditorLayout::width, canvasHeight = EditorLayout::expandedHeight;
Typeface::Ptr embeddedMonoFace(bool bold) {
    static const auto regularFace =
        Typeface::createSystemTypefaceFor(BinaryData::IoskeleyMonoTermNerdFontMonoRegular_ttf,
                                          BinaryData::IoskeleyMonoTermNerdFontMonoRegular_ttfSize);
    static const auto boldFace =
        Typeface::createSystemTypefaceFor(BinaryData::IoskeleyMonoTermNerdFontMonoBold_ttf,
                                          BinaryData::IoskeleyMonoTermNerdFontMonoBold_ttfSize);
    return bold ? boldFace : regularFace;
}
Typeface::Ptr embeddedFace(bool bold) {
    static const auto regularFace = Typeface::createSystemTypefaceFor(
        BinaryData::OpenSansRegular_ttf, BinaryData::OpenSansRegular_ttfSize);
    static const auto semiboldFace = Typeface::createSystemTypefaceFor(
        BinaryData::OpenSansSemibold_ttf, BinaryData::OpenSansSemibold_ttfSize);
    return bold ? semiboldFace : regularFace;
}

std::unique_ptr<Drawable> brandLogo(Colour colour) {
    auto image =
        Drawable::createFromImageData(BinaryData::logisth_svg, BinaryData::logisth_svgSize);
    if (image)
        image->replaceColour(Colour(0xff080808), colour);
    return image;
}
class BrandLogoView : public Component {
  public:
    std::unique_ptr<Drawable> image;
    void paint(Graphics &g) override {
        if (image)
            image->drawWithin(g, getLocalBounds().toFloat(), RectanglePlacement::centred, 1);
    }
};
bool dark(const Component &c) {
    for (auto *p = &c; p != nullptr; p = p->getParentComponent())
        if (p->getProperties().contains("dark"))
            return bool(p->getProperties()["dark"]);
    return false;
}
void paintShell(Graphics &g, Rectangle<int> b, const String &title, const String &hint,
                const LogisthColours &colours) {
    g.setColour(Colours::black.withAlpha(colours.darkTheme ? .16f : .035f));
    g.fillRoundedRectangle(b.translated(0, 2).toFloat(), 8);
    if (colours.backgroundStyle == 1) {
        const auto amount = float(colours.gradientAmount);
        const auto alpha = float(colours.panelOpacity);
        g.setGradientFill(ColourGradient(
            colours.panel.darker(.2f * amount).withAlpha(alpha), float(b.getX()), float(b.getY()),
            colours.panel.interpolatedWith(colours.green, .24f * amount).withAlpha(alpha),
            float(b.getRight()), float(b.getBottom()), false));
    } else
        g.setColour(colours.backgroundStyle == 0
                        ? colours.panel
                        : colours.panel.withAlpha(float(colours.panelOpacity)));
    g.fillRoundedRectangle(b.toFloat(), 8);
    g.setColour(colours.line);
    g.drawRoundedRectangle(b.toFloat().reduced(.5f), 8, 1);
    colours.drawCaption(g, title, b.reduced(12, 0).withY(b.getY() + 8).withHeight(18), 11.5f,
                        colours.ink, Justification::centredLeft, true);
    if (hint.isNotEmpty())
        colours.drawCaption(g, hint, b.reduced(12, 0).withY(b.getY() + 8).withHeight(18), 9,
                            colours.muted, Justification::centredRight);
}
void capsule(Graphics &g, Rectangle<float> b, Colour fill, Colour outline) {
    g.setColour(fill);
    g.fillRoundedRectangle(b, 4);
    g.setColour(outline);
    g.drawRoundedRectangle(b.reduced(.5f), 4, 1);
}
} // namespace

Font LogisthColours::font(float size, bool bold) const {
    if (typography.style != 0)
        bold = typography.style == 2 || typography.style == 4;
    const float h = jmax(11.f, size * 1.18f) * float(typography.size / 100);
    Font result(FontOptions{}.withHeight(h));
    if (fontFamily == "IoskeleyMonoTerm Nerd Font Mono")
        result = Font(FontOptions(embeddedMonoFace(bold)).withHeight(h));
    else if (fontFamily == "Open Sans")
        result = Font(FontOptions(embeddedFace(bold)).withHeight(h));
    else
        result = Font(FontOptions(fontFamily, h, bold ? Font::bold : Font::plain));
    if (typography.style == 3 || typography.style == 4)
        result.setItalic(true);
    return result;
}
void LogisthColours::drawCaption(Graphics &g, const String &text, Rectangle<int> bounds, float size,
                                 Colour colour, Justification align, bool bold) const {
    g.setFont(font(size, bold));
    g.setColour(colour);
    g.drawText(text, bounds, align, true);
}
class ThemeText {
    const LogisthColours &theme;

  public:
    explicit ThemeText(const LogisthColours &c) : theme(c) {}
    Font font(float s, bool b = false) const {
        return theme.font(s, b);
    }
    void drawCaption(Graphics &g, const String &t, Rectangle<int> r, float s, Colour c,
                     Justification a = Justification::centredLeft, bool b = false) const {
        theme.drawCaption(g, t, r, s, c, a, b);
    }
};
#include "DistortionView.h"
#include "DynamicsViews.h"
#include "EqView.h"
#include "StudioViews.h"

void LogisthColours::choosePalette(int selected) {
    themeId = jlimit(0, ThemeSettings::names().size() - 1, selected);
    applyPalette(ThemeSettings::theme(themeId));
}
void LogisthColours::applyPalette(const ThemeDefinition &theme) {
    darkTheme = theme.dark;
    fontFamily = theme.fontFamily;
    skin = theme.skin;
    backgroundStyle = theme.backgroundStyle;
    backgroundMix = theme.backgroundMix;
    panelOpacity = theme.panelOpacity;
    gradientAmount = theme.gradientAmount;
    gridOpacity = theme.gridOpacity;
    knobStyle = theme.knobStyle;
    knobColour = theme.colours[10];
    knobTrack = theme.colours[11];
    body = theme.colours[0];
    panel = theme.colours[1];
    line = theme.colours[2];
    ink = theme.colours[3];
    muted = theme.colours[4];
    green = theme.colours[5];
    mint = theme.colours[6];
    screen = theme.colours[7];
    screenLine = theme.colours[8];
    screenText = theme.colours[9];
    screenAccent = readableColour(green, screen, screen, 3);
}
LogisthLook::LogisthLook() {
    regular = embeddedFace(false);
    semibold = embeddedFace(true);
    setTheme(1);
}
void LogisthLook::setDarkTheme(bool enabled) {
    setTheme(enabled ? 1 : 0);
}
void LogisthLook::setTheme(int selected) {
    choosePalette(selected);
    setPalette(ThemeSettings::theme(themeId));
}
void LogisthLook::setPalette(const ThemeDefinition &theme) {
    applyPalette(theme);
    knobPaints = {knobPaint(theme, false), knobPaint(theme, true)};
    regular = font(12, false).getTypefacePtr();
    semibold = font(12, true).getTypefacePtr();
    setColour(TextEditor::highlightedTextColourId, darkTheme ? screen : ink);
    setColour(ComboBox::textColourId, ink);
    setColour(TextButton::buttonColourId, darkTheme ? screenLine : Colour(0xffe9ede5));
    setColour(TextButton::textColourOffId, ink);
    setColour(TextButton::buttonOnColourId, green);
    setColour(TextButton::textColourOnId, darkTheme ? screen : Colours::white);
    setColour(PopupMenu::backgroundColourId, screen);
    setColour(PopupMenu::textColourId, screenText);
    setColour(PopupMenu::highlightedBackgroundColourId, green);
    setColour(PopupMenu::highlightedTextColourId, darkTheme ? screen : Colours::white);
    setColour(TooltipWindow::backgroundColourId, screen);
    setColour(TooltipWindow::textColourId, screenText);
    setColour(TooltipWindow::outlineColourId, screenLine);
    setColour(TextEditor::backgroundColourId, panel);
    setColour(TextEditor::textColourId, ink);
    setColour(TextEditor::highlightColourId, mint);
    setColour(TextEditor::focusedOutlineColourId, green);
}
Typeface::Ptr LogisthLook::getTypefaceForFont(const Font &f) {
    return f.isBold() ? semibold : regular;
}
Font LogisthLook::getTextButtonFont(TextButton &button, int) {
    return font(float(button.getProperties().getWithDefault("fontSize", 12)));
}
PopupMenu LogisthLook::contextMenu() {
    PopupMenu result;
    result.setLookAndFeel(this);
    return result;
}
Font LogisthLook::getPopupMenuFont() {
    return font(12);
}
void LogisthLook::drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool separator,
                                    bool active, bool highlighted, bool ticked, bool submenu,
                                    const String &text, const String &shortcut,
                                    const Drawable *icon, const Colour *colour) {
    logisth_ui::TextGraphics context(
        g.getInternalContext(), typography,
        AffineTransform::scale(g.getInternalContext().getPhysicalPixelScaleFactor()));
    Graphics graphics(context);
    LookAndFeel_V4::drawPopupMenuItem(graphics, area, separator, active, highlighted, ticked,
                                      submenu, text, shortcut, icon, colour);
}
namespace {
TextLayout tooltipText(const LogisthColours &theme, const String &text) {
    AttributedString attributed(text);
    attributed.setFont(theme.font(11));
    attributed.setColour(theme.screenText);
    TextLayout layout;
    layout.createLayoutWithBalancedLineLengths(attributed, 380);
    return layout;
}
} // namespace
Rectangle<int> LogisthLook::getTooltipBounds(const String &text, Point<int> position,
                                             Rectangle<int> parent) {
    auto layout = tooltipText(*this, text);
    return Rectangle<int>(position.x + 12, position.y + 18, int(std::ceil(layout.getWidth())) + 18,
                          int(std::ceil(layout.getHeight())) + 14)
        .constrainedWithin(parent);
}
void LogisthLook::drawTooltip(Graphics &g, const String &text, int width, int height) {
    g.fillAll(screen);
    g.setColour(screenLine);
    g.drawRect(0, 0, width, height);
    logisth_ui::TextGraphics context(
        g.getInternalContext(), typography,
        AffineTransform::scale(g.getInternalContext().getPhysicalPixelScaleFactor()));
    Graphics graphics(context);
    tooltipText(*this, text).draw(graphics, {9, 7, float(width - 18), float(height - 14)});
}
PopupMenu::Options LogisthLook::getOptionsForComboBoxPopupMenu(ComboBox &box, Label &label) {
    auto options =
        LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label).withStandardItemHeight(28);
    if (bool(box.getProperties()["routing"]))
        return options.withMinimumNumColumns(3).withMaximumNumColumns(3).withMinimumWidth(420);
    return options;
}
void LogisthLook::drawRotarySlider(Graphics &g, int x, int y, int w, int h, float pos, float start,
                                   float end, Slider &s) {
    const bool small = bool(s.getProperties()["trim"]), isDark = (dark(s) || darkTheme);
    const auto &paint = knobColours(dark(s));
    float radius = jmin(float(w), float(h)) * .5f - 3.f;
    if (radius <= 0)
        return;
    auto centre = Rectangle<float>(float(x), float(y), float(w), float(h)).getCentre();
    float angle = start + pos * (end - start);
    const Colour accent = isDark ? mint : green;
    Path track;
    track.addCentredArc(centre.x, centre.y, radius, radius, 0, start, end, true);
    g.setColour(paint.track);
    g.strokePath(track, PathStrokeType(small ? 1.5f : 2.f));
    const double neutral =
        s.getMinimum() < 0 && s.getMaximum() > 0 ? s.valueToProportionOfLength(0) : 0;
    float from = start + float(neutral) * (end - start);
    Path active;
    active.addCentredArc(centre.x, centre.y, radius, radius, 0, jmin(from, angle),
                         jmax(from, angle), true);
    g.setColour(s.isEnabled() ? paint.active : paint.active.withAlpha(.25f));
    g.strokePath(active, PathStrokeType(small ? 1.6f : 2.4f));
    if (s.getProperties().contains("live") && float(s.getProperties()["live"]) >= 0 && !small) {
        float live = float(s.getProperties()["live"]), lo = float(s.getProperties()["liveLow"]),
              hi = float(s.getProperties()["liveHigh"]);
        Path arc;
        arc.addCentredArc(centre.x, centre.y, radius + 3, radius + 3, 0, start + lo * (end - start),
                          start + hi * (end - start), true);
        g.setColour(mint.withAlpha(.5f));
        g.strokePath(arc, PathStrokeType(2));
        float a = start + live * (end - start) - MathConstants<float>::halfPi;
        g.setColour(mint);
        g.fillEllipse(centre.x + std::cos(a) * (radius + 3) - 2,
                      centre.y + std::sin(a) * (radius + 3) - 2, 4, 4);
    }
    float r = radius - (small ? 3 : 4);
    auto knob = Rectangle<float>(centre.x - r, centre.y - r, 2 * r, 2 * r);
    if (!small) {
        g.setColour(Colours::black.withAlpha(isDark ? .12f : .06f));
        g.fillEllipse(knob.translated(0, 2));
    }
    g.setGradientFill(ColourGradient(paint.top, centre.x, knob.getY(), paint.bottom, centre.x,
                                     knob.getBottom(), false));
    if (knobStyle != 2) {
        g.fillEllipse(knob);
    }
    g.setColour(paint.outline);
    g.drawEllipse(knob.reduced(.5f), 1);
    float a = angle - MathConstants<float>::halfPi;
    g.setColour(s.isEnabled() ? paint.pointer : muted.withAlpha(.4f));
    g.drawLine(centre.x + std::cos(a) * r * .35f, centre.y + std::sin(a) * r * .35f,
               centre.x + std::cos(a) * r * .78f, centre.y + std::sin(a) * r * .78f,
               small ? 1.8f : 2.2f);
}
Slider::SliderLayout LogisthLook::getSliderLayout(Slider &s) {
    Slider::SliderLayout layout;
    auto b = s.getLocalBounds();
    if (bool(s.getProperties()["number"])) {
        layout.sliderBounds = b;
        layout.textBoxBounds = b;
    } else if (bool(s.getProperties()["trim"]))
        layout.sliderBounds = b;
    else if (s.isHorizontal())
        return LookAndFeel_V4::getSliderLayout(s);
    else {
        layout.textBoxBounds = b.removeFromBottom(15);
        layout.sliderBounds = b;
    }
    return layout;
}
Label *LogisthLook::createSliderTextBox(Slider &s) {
    auto *box = LookAndFeel_V4::createSliderTextBox(s);
    box->setFont(font(bool(s.getProperties()["number"]) ? 11 : 10));
    box->setColour(Label::textColourId, (dark(s) || darkTheme) ? screenText : ink);
    box->setColour(Label::backgroundColourId, Colours::transparentBlack);
    box->setColour(Label::outlineColourId, Colours::transparentBlack);
    box->setBorderSize(BorderSize<int>(0));
    return box;
}
void LogisthLook::drawLinearSlider(Graphics &g, int x, int y, int w, int h, float pos, float, float,
                                   Slider::SliderStyle, Slider &s) {
    bool isDark = (dark(s) || darkTheme);
    auto b = Rectangle<float>(float(x), float(y), float(w), float(h));
    if (bool(s.getProperties()["number"])) {
        capsule(g, b, isDark ? screenLine : Colour(0xffe9ede5), isDark ? screenLine : line);
        float p = float(s.valueToProportionOfLength(s.getValue()));
        if (s.getProperties().contains("sampleProgress"))
            p = jlimit(0.f, 1.f, float(s.getProperties()["sampleProgress"]));
        if (bool(s.getProperties()["lane"])) {
            g.setColour((isDark ? mint : green).withAlpha(.24f));
            g.fillRoundedRectangle(b.withTrimmedTop(b.getHeight() * (1 - p)).reduced(1), 2);
        }
        g.setColour((isDark ? mint : green).withAlpha(.4f));
        g.fillRoundedRectangle(b.getX() + 5, b.getBottom() - 2, jmax(0.f, (b.getWidth() - 10) * p),
                               1, 0.5f);
        return;
    }
    if (s.isHorizontal()) {
        const auto left = b.getX() + 5, right = b.getRight() - 5, cy = b.getCentreY();
        const auto px = jlimit(left, right, pos);
        g.setColour(isDark ? screenLine : line);
        g.fillRoundedRectangle(left, cy - 2, right - left, 4, 2);
        g.setColour((isDark ? mint : green).withAlpha(s.isEnabled() ? .65f : .2f));
        g.fillRoundedRectangle(left, cy - 2, jmax(0.f, px - left), 4, 2);
        g.setColour(isDark ? screenText : Colours::white);
        g.fillRoundedRectangle(px - 4, cy - 8, 8, 16, 2);
        g.setColour(isDark ? screenLine : muted);
        g.drawRoundedRectangle(px - 4, cy - 8, 8, 16, 2, 1);
        return;
    }
    const float cx = b.getCentreX(), top = b.getY() + 4, bottom = b.getBottom() - 4;
    g.setColour(isDark ? screenLine : line);
    g.fillRoundedRectangle(cx - 2, top, 4, bottom - top, 2);
    const float py = jlimit(top, bottom, pos);
    g.setColour((isDark ? mint : green).withAlpha(s.isEnabled() ? .65f : .2f));
    g.fillRoundedRectangle(cx - 2, py, 4, jmax(0.f, bottom - py), 2);
    g.setColour(isDark ? screenText : Colour(0xfffcfcf8));
    g.fillRoundedRectangle(cx - 9, py - 4, 18, 8, 2);
    g.setColour(isDark ? screenLine : Colour(0xffa3b0a2));
    g.drawRoundedRectangle(cx - 9, py - 4, 18, 8, 2, 1);
    g.setColour(isDark ? green : muted);
    g.drawHorizontalLine(int(py), cx - 6, cx + 6);
}
void LogisthLook::drawComboBox(Graphics &g, int w, int h, bool, int, int, int, int, ComboBox &c) {
    bool isDark = (dark(c) || darkTheme);
    capsule(g, Rectangle<float>(0, 0, float(w), float(h)), isDark ? screenLine : Colour(0xffe9ede5),
            isDark ? screenLine : line);
    Path arrow;
    float cx = float(w) - 10, cy = float(h) * .5f;
    arrow.startNewSubPath(cx - 3, cy - 1);
    arrow.lineTo(cx, cy + 2);
    arrow.lineTo(cx + 3, cy - 1);
    g.setColour((isDark ? mint : green).withAlpha(c.isEnabled() ? .9f : .3f));
    g.strokePath(arrow, PathStrokeType(1.2f));
}
Font LogisthLook::getComboBoxFont(ComboBox &c) {
    return font(
        float(c.getProperties().getWithDefault("fontSize", c.getWidth() < 65 ? 9.5f : 11.f)));
}
void LogisthLook::positionComboBoxText(ComboBox &c, Label &l) {
    l.setBounds(7, 0, jmax(0, c.getWidth() - 24), c.getHeight());
    l.setFont(getComboBoxFont(c));
    l.setColour(Label::textColourId, (dark(c) || darkTheme) ? screenText : ink);
    l.setBorderSize(BorderSize<int>(0));
}
void LogisthLook::drawToggleButton(Graphics &g, ToggleButton &b, bool hover, bool) {
    const bool on = b.getToggleState(), isDark = (dark(b) || darkTheme);
    auto bounds = b.getLocalBounds().toFloat();
    if (b.getWidth() <= 22) {
        g.setColour(on ? mint : screenLine);
        g.fillRoundedRectangle(bounds.reduced(6), 2);
        return;
    }
    capsule(g, bounds, on ? (isDark ? mint : green) : (isDark ? screenLine : Colour(0xffe9ede5)),
            on ? green : (isDark ? screenLine : line));
    if (hover) {
        g.setColour(Colours::white.withAlpha(.06f));
        g.fillRoundedRectangle(bounds, 4);
    }
    drawCaption(g, b.getButtonText(), b.getLocalBounds().reduced(2), 9,
                on ? (isDark ? screen : Colours::white) : (isDark ? screenText : muted),
                Justification::centred, true);
}
void LogisthLook::drawCornerResizer(Graphics &g, int w, int h, bool, bool) {
    g.setColour(muted.withAlpha(.45f));
    for (int i = 1; i < 4; ++i)
        g.drawLine(float(w - i * 4), float(h - 2), float(w - 2), float(h - i * 4), 1);
}

class LogisthEditor::Console : public Component,
                               private Timer,
                               public LogisthColours,
                               public FileDragAndDropTarget,
                               private ThemeSettings::Listener,
                               private ChangeListener,
                               private ListBoxModel,
                               private KeyListener {
    class SearchButton final : public Button {
        Console &owner;

      public:
        explicit SearchButton(Console &c) : Button("Search presets"), owner(c) {
            setComponentID("preset.search");
            setTooltip("Search factory and user presets");
        }
        void paintButton(Graphics &g, bool over, bool down) override {
            capsule(g, getLocalBounds().toFloat(),
                    over || down || getToggleState() ? owner.screenLine : owner.panel, owner.line);
            g.setColour(owner.mint);
            g.drawEllipse(7, 5, 10, 10, 1.7f);
            g.drawLine(15, 14, 21, 20, 1.8f);
        }
    };
    class SearchPanel final : public Component {
        Console &owner;

      public:
        explicit SearchPanel(Console &c) : owner(c) {}
        void paint(Graphics &g) override {
            capsule(g, getLocalBounds().toFloat(), owner.screen, owner.screenLine);
        }
    };
    struct PresetMatch {
        int id;
        String name, description;
    };
    enum class Style { Knob, Trim, Fader, Number, Menu, Toggle };
    class Control : public Component, private ThemeText {
      public:
        Control(Console &o, Param index)
            : ThemeText(o), owner(o), spec(parameterSpecs[size_t(index)]), id(index) {
            setLookAndFeel(&owner.look);
            setComponentID(spec.id);
            getProperties().set("hostParameterID", spec.id);
            addAndMakeVisible(label);
            addAndMakeVisible(slider);
            addAndMakeVisible(menu);
            addAndMakeVisible(toggle);
            label.setJustificationType(Justification::centred);
            label.setFont(font(9.8f));
            label.setInterceptsMouseClicks(false, false);
            const String tooltip = String(spec.group) + " / " + spec.label;
            slider.setName(tooltip);
            menu.setName(tooltip);
            toggle.setName(tooltip);
            slider.setTooltip(tooltip);
            menu.setTooltip(tooltip);
            toggle.setTooltip(tooltip);
            setTitle(tooltip);
            setDescription(tooltip);
            if (*spec.choices) {
                auto choices = StringArray::fromTokens(spec.choices, "|", "");
                const auto extension = choiceExtension(id);
                if (extension != Param::Count) {
                    auto added =
                        StringArray::fromTokens(parameterSpecs[size_t(extension)].choices, "|", "");
                    added.remove(0);
                    choices.addArray(added);
                }
                const auto paired = pairedChoiceExtension(id);
                if (paired != Param::Count) {
                    auto added =
                        StringArray::fromTokens(parameterSpecs[size_t(paired)].choices, "|", "");
                    added.remove(0);
                    choices.addArray(added);
                }
                if (choices == StringArray{"Off", "On"}) {
                    kind = Style::Toggle;
                    toggle.setButtonText(spec.label);
                    toggleAttachment =
                        std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
                            owner.processor.state, spec.id, toggle);
                } else {
                    kind = Style::Menu;
                    menu.getProperties().set("routing", String(spec.id).contains("dest"));
                    PopupMenu cChoices;
                    for (int i = 0; i < choices.size(); ++i) {
                        const bool cDestination =
                            String(spec.id).contains("dest") && i >= int(Destination::VolumeC);
                        const bool cMacro = String(spec.id).startsWith("m") &&
                                            String(spec.id).contains("target") && i >= 31;
                        if (cDestination || cMacro) {
                            if (cDestination && i == int(Destination::VolumeC) + 16)
                                cChoices.addColumnBreak();
                            cChoices.addItem(i + 1, choices[i]);
                            continue;
                        }
                        if (String(spec.id).startsWith("m") && String(spec.id).contains("target")) {
                            if (i == 4 || i == 20)
                                menu.getRootMenu()->addColumnBreak();
                            if (i == 1 || i == 4 || i == 20)
                                menu.getRootMenu()->addSectionHeader(i == 1   ? "Filters"
                                                                     : i == 4 ? "Players"
                                                                              : "Effects / time");
                        }
                        if (String(spec.id).contains("dest") &&
                            (i == 13 || i == 19 || i == 38 || i == 45 || i == 52))
                            menu.getRootMenu()->addColumnBreak();
                        if (String(spec.id).contains("dest") && (i == 1 || i == 13 || i == 19))
                            menu.getRootMenu()->addSectionHeader(i == 1 ? "Oscillators"
                                                                 : i == 13
                                                                     ? "Filters"
                                                                     : "Modulation / effects");
                        if (String(spec.id).contains("dest") &&
                            i == int(logisth::Destination::PositionA))
                            menu.getRootMenu()->addSectionHeader("Source position / grains");
                        if (String(spec.id).contains("dest") && i == int(Destination::PositionA1))
                            menu.getRootMenu()->addSectionHeader("Individual WT players");
                        String name = choices[i];
                        if (String(spec.id).endsWith("slope"))
                            name = name.replace("/oct", "");
                        if (String(spec.id).contains("dest")) {
                            name = name.replace(" and B", "+B")
                                       .replace("Resonance", "Reso")
                                       .replace("Phaser Frequency", "Phaser freq")
                                       .replace("Distortion Amount", "Drive amount");
                        }
                        menu.addItem(name, i + 1);
                    }
                    if (cChoices.getNumItems() > 0)
                        menu.getRootMenu()->addSubMenu("Part C / A+B+C", cChoices);
                    if (String(spec.id).contains("dest"))
                        buildRoutingMenu(menu, choices);
                    if (String(spec.id).startsWith("m") && String(spec.id).contains("target"))
                        buildMacroMenu(menu, choices);
                    if (extension != Param::Count)
                        bindComposite(extension, false);
                    else
                        choiceAttachment =
                            std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
                                owner.processor.state, spec.id, menu);
                }
            } else {
                kind = Style::Knob;
                compositePolyphony = id == Param::polyphony;
                if (compositePolyphony ||
                    (String(spec.id).startsWith("o") && String(spec.id).endsWith("detune"))) {
                    if (compositePolyphony)
                        slider.setRange(1, maxPolyphony, 1);
                    else {
                        slider.setRange(0, 400, 0);
                        slider.setSkewFactorFromMidPoint(100);
                    }
                    bindComposite(compositePolyphony
                                      ? Param::polyphonyextra
                                      : detuneExtra(String(spec.id).substring(1).getIntValue() - 1),
                                  !compositePolyphony);
                    slider.onDragStart = [this] {
                        compositeGesture = true;
                        compositeBase->beginGesture();
                        compositeExtra->beginGesture();
                        if (compositeWide)
                            compositeWide->beginGesture();
                    };
                    slider.onDragEnd = [this] {
                        compositeBase->endGesture();
                        compositeExtra->endGesture();
                        if (compositeWide)
                            compositeWide->endGesture();
                        compositeGesture = false;
                    };
                } else {
                    slider.onDragStart = [this] { attachedDrag = true; };
                    slider.onDragEnd = [this] { attachedDrag = false; };
                    sliderAttachment =
                        std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
                            owner.processor.state, spec.id, slider);
                }
                slider.textFromValueFunction = [this](double v) { return formatted(v); };
                slider.valueFromTextFunction = [this](const String &t) {
                    if (String(spec.id).endsWith("sampleseek") && t.trim().equalsIgnoreCase("Auto"))
                        return double(spec.low);
                    double v = t.getDoubleValue();
                    if (t.containsIgnoreCase("k"))
                        v *= 1000;
                    const bool storedMilliseconds = String(spec.label).contains("/ ms");
                    if (t.containsIgnoreCase("ms") && !storedMilliseconds)
                        v *= .001;
                    else if (t.endsWithIgnoreCase("s") && !t.endsWithIgnoreCase("ms") &&
                             storedMilliseconds)
                        v *= 1000;
                    if (String(spec.id).startsWith("pat") && String(spec.id).endsWith("note"))
                        v -= 1;
                    if (percent())
                        v *= .01;
                    return v;
                };
                slider.setDoubleClickReturnValue(
                    true, String(spec.id).contains("amt") ? 0 : spec.initial);
                slider.onValueChange = [this] {
                    if ((compositeDetune || compositePolyphony) && !compositeUpdate)
                        writeComposite(float(slider.getValue()));
                    if (isMouseOverOrDragging(true))
                        describe();
                    if (String(spec.id).contains("amp"))
                        owner.repaint(500, 113, 240, 28);
                };
            }
            menu.onChange = [this] {
                if (compositeChoice && !compositeUpdate && menu.getSelectedId() > 0)
                    writeComposite(float(menu.getSelectedId() - 1));
                if (String(spec.id).endsWith("engine")) {
                    int slot = String(spec.id).substring(1).getIntValue() - 1;
                    owner.sourceViews[size_t(slot)].setSelectedId(menu.getSelectedId() == 1 ? 1 : 2,
                                                                  dontSendNotification);
                    owner.layout();
                    owner.repaint();
                }
                if (id == Param::distbandmode || id == Param::distbands) {
                    // JUCE has committed the attached choice before onChange. Reveal the
                    // matching crossover handles immediately, even while the UI timer is idle.
                    owner.layout();
                    owner.repaint();
                }
                if (isMouseOverOrDragging(true))
                    describe();
            };
            toggle.onClick = [this] {
                if (String(spec.id).endsWith("tailon") && toggle.getToggleState()) {
                    const int slot = String(spec.id).substring(1).getIntValue() - 1;
                    logisth_ui::change(owner.processor, "o" + String(slot + 1) + "loop", 0);
                    owner.waveView.setSlot(slot);
                    owner.waveSlot.setSelectedId(slot + 1, dontSendNotification);
                    owner.setRack(9);
                }
                if (id == Param::midionly && toggle.getToggleState()) {
                    auto *midi = owner.processor.state.getParameter("arpout");
                    midi->beginChangeGesture();
                    midi->setValueNotifyingHost(1);
                    midi->endChangeGesture();
                }
                describe();
            };
            addMouseListener(this, true);
        }
        ~Control() override {
            // A host mode change can remove a trim field before mouseUp arrives.
            if (attachedDrag)
                owner.processor.state.getParameter(spec.id)->endChangeGesture();
            removeMouseListener(this);
            setLookAndFeel(nullptr);
        }
        void configure(Rectangle<int> b, Style style, const String &title, bool isDark = false) {
            kind = style;
            caption = title;
            getProperties().set("dark", isDark);
            menu.setColour(ComboBox::textColourId,
                           (isDark || owner.darkTheme) ? owner.screenText : owner.ink);
            setBounds(b);
            setVisible(true);
            label.setText(title, dontSendNotification);
            label.setColour(Label::textColourId, isDark ? owner.mint.withAlpha(.7f) : owner.muted);
            slider.getProperties().set("lane", String(spec.id).startsWith("pat") ||
                                                   String(spec.id).startsWith("step"));
            slider.getProperties().set("number", kind == Style::Number);
            slider.getProperties().set("trim", kind == Style::Trim);
            slider.setSliderStyle(kind == Style::Fader    ? Slider::LinearVertical
                                  : kind == Style::Number ? Slider::LinearBar
                                                          : Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(kind == Style::Trim     ? Slider::NoTextBox
                                   : kind == Style::Number ? Slider::TextBoxLeft
                                                           : Slider::TextBoxBelow,
                                   false, b.getWidth(), kind == Style::Number ? b.getHeight() : 15);
            toggle.setButtonText(title);
            slider.updateText();
            resized();
        }
        void resized() override {
            bool heading = caption.isNotEmpty() && kind != Style::Toggle && kind != Style::Trim;
            label.setVisible(heading);
            auto b = getLocalBounds();
            if (heading)
                label.setBounds(b.removeFromTop(13));
            slider.setVisible(kind == Style::Knob || kind == Style::Trim || kind == Style::Fader ||
                              kind == Style::Number);
            menu.setVisible(kind == Style::Menu);
            if (kind == Style::Menu)
                menu.setSelectedId(int(owner.value(id)) + 1, dontSendNotification);
            toggle.setVisible(kind == Style::Toggle);
            slider.setBounds(b);
            // The chorus sync selector occupies a knob cell. Align its centre
            // with the rotary face, excluding the value textbox below the knobs.
            auto menuArea = String(spec.group) == "CHORUS" && heading && b.getHeight() > 50
                                ? b.withTrimmedBottom(15)
                                : b;
            menu.setBounds(
                menuArea.withSizeKeepingCentre(b.getWidth(), jmin(25, menuArea.getHeight())));
            toggle.setBounds(b);
        }
        void mouseEnter(const MouseEvent &) override {
            describe();
        }
        void describe() {
            owner.readout =
                String(spec.group) + "  /  " + String(spec.label) + "   " +
                ((compositeDetune || compositePolyphony) ? formatted(slider.getValue())
                 : compositeChoice
                     ? menu.getText()
                     : owner.processor.state.getParameter(spec.id)->getCurrentValueAsText());
            owner.repaint(118, 844, 870, 16);
        }
        Param parameter() const {
            return id;
        }
        void liveValue(float value, bool active) {
            auto &props = slider.getProperties();
            if (!active) {
                props.set("live", -1.f);
                slider.repaint();
                return;
            }
            float normalized = float(slider.valueToProportionOfLength(value));
            float base = float(slider.valueToProportionOfLength(slider.getValue()));
            liveLow = std::min(std::min(normalized, base), liveLow * .985f + base * .015f);
            liveHigh = std::max(std::max(normalized, base), liveHigh * .985f + base * .015f);
            props.set("live", normalized);
            props.set("liveLow", liveLow);
            props.set("liveHigh", liveHigh);
            slider.repaint();
        }
        float liveLow = 1, liveHigh = 0;
        void enable(bool enabled) {
            setEnabled(enabled);
            label.setAlpha(enabled ? 1.f : .4f);
        }
        void selectMenuItem(int item) {
            menu.setSelectedId(item, sendNotificationSync);
        }
        double sliderValue() const {
            return slider.getValue();
        }
        void showPlaybackPosition(std::optional<float> live) {
            if (attachedDrag || slider.isMouseButtonDown())
                return;
            for (auto *child : slider.getChildren()) {
                if (auto *valueBox = dynamic_cast<Label *>(child)) {
                    if (!valueBox->isBeingEdited()) {
                        // The moving display is telemetry. Only an edit to the
                        // attached slider writes the actual host parameter.
                        const double value = live ? double(*live) : slider.getValue();
                        valueBox->setText(formatted(value), dontSendNotification);
                        slider.getProperties().set("sampleProgress", value);
                        slider.repaint();
                    }
                    return;
                }
            }
        }

      private:
        bool percent() const {
            return (spec.low >= 0 && spec.high <= 1 && String(spec.id) != "portamento") ||
                   String(spec.id).endsWith("gspace") || String(spec.id).endsWith("gwave") ||
                   String(spec.id).endsWith("sampleseek");
        }
        String formatted(double v) const {
            const String name(spec.id);
            if (name.endsWith("sampleseek") && v < 0)
                return "Auto";
            if (name.startsWith("pat") && name.endsWith("note"))
                return String(int(v) + 1);
            if (name.contains("env") &&
                (name.endsWith("enva") || name.endsWith("envd") || name.endsWith("envr")))
                return v < 1 ? String(v * 1000, 0) + "ms" : String(v, 1) + "s";
            if (name.endsWith("fine") || name.endsWith("detune"))
                return String(v, 1) + "ct";
            if (String(spec.label).contains("/ ms") && !(name.endsWith("ssblock") && v == 0))
                return String(v, std::abs(v) >= 10 ? 0 : 1) + "ms";
            if (String(spec.label).contains("/ dB"))
                return String(v, 0) + "dB";
            if (name == "compratio")
                return String(v, 1) + ":1";
            if (name.contains("velocity"))
                return v < .5 ? String(CharPointer_UTF8("—")) : String(int(v));
            if (name == "filtercutoff")
                return String(v, 0) + "st";
            if (name == "p1cutoff" || name == "p2cutoff" || name == "p3cutoff" ||
                name.contains("freq") || name.contains("Freq") || name == "delaylowcut" ||
                name == "delayhighcut")
                return std::round(v) >= 1000 ? String(std::round(v) / 1000, 1) + "k"
                                             : String(int(std::round(v)));
            if (name.endsWith("ampa") || name.endsWith("ampd") || name.endsWith("ampr") ||
                name == "env1a" || name == "env1d" || name == "env1r" || name == "env2a" ||
                name == "env2d" || name == "env2r" || name == "portamento")
                return v < 1 ? String(v * 1000, 0) + "ms" : String(v, 1) + "s";
            if (percent())
                return String(v * 100, getComponentID().startsWith("sample.") ? 2 : 0) + "%";
            if (spec.integer)
                return (v > 0 && spec.low < 0 ? "+" : "") + String(int(v));
            if (name.endsWith("ssblock") && v == 0)
                return "Auto";
            if (name.endsWith("sstonality"))
                return v == 0      ? "Off"
                       : v >= 1000 ? String(v / 1000, 2) + "kHz"
                                   : String(v, 0) + "Hz";
            if (name.endsWith("ssformant") || name.endsWith("sssemitones"))
                return (v > 0 ? "+" : "") + String(v, 1) + "st";
            if (name.endsWith("paulamount"))
                return String(v, 1) + "x";
            if (name.endsWith("stretch") || name.endsWith("ssrate"))
                return String(v, 2) + "x";
            if (name.endsWith("phase"))
                return String(v, 0) + String(CharPointer_UTF8("°"));
            return String(v, std::abs(v) >= 10 ? 0 : 2);
        }
        static void buildRoutingMenu(ComboBox &box, const StringArray &choices) {
            std::array<std::vector<std::pair<String, int>>, 14> entries;
            const StringArray families{"Volume",     "Pitch",     "Phase", "Pan",
                                       "Cutoff",     "Resonance", "Mix",   "WT Pos / Start",
                                       "Grain size", "FM",        "AM",    "Pulse width"};
            for (int i = 1; i < choices.size(); ++i) {
                String text = choices[i].replace(" and B", "+B");
                int family = -1;
                if (text.startsWith("Phaser") || text.startsWith("Distortion"))
                    family = 13;
                else if (text.startsWith("LFO"))
                    family = 12;
                else
                    for (int k = 0; k < families.size(); ++k)
                        if (text.startsWith(families[k])) {
                            family = k;
                            break;
                        }
                if (family < 0)
                    family = 13;
                String label =
                    family < 12 ? text.substring(families[family].length()).trim() : text;
                if (family == 9)
                    label = label.replace("index ", "");
                if (family == 10)
                    label = label.replace("depth ", "");
                entries[size_t(family)].emplace_back(label, i + 1);
            }
            const StringArray scopeOrder{"A1", "A2", "B1",  "B2",  "C1",  "C2",   "A",
                                         "B",  "C",  "A+B", "A+C", "B+C", "A+B+C"};
            std::array<PopupMenu, 14> menus;
            for (size_t family = 0; family < entries.size(); ++family) {
                auto &values = entries[family];
                std::stable_sort(values.begin(), values.end(), [&](const auto &a, const auto &b) {
                    int x = scopeOrder.indexOf(a.first), y = scopeOrder.indexOf(b.first);
                    return (x < 0 ? 99 : x) < (y < 0 ? 99 : y);
                });
                for (const auto &entry : values) {
                    const StringArray names{"Volume",     "Pitch", "Phase", "Pan",
                                            "Cutoff",     "Reso",  "Mix",   "WT Pos",
                                            "Grain size", "FM",    "AM",    "Width"};
                    menus[family].addItem(entry.second, family < 12
                                                            ? names[int(family)] + " " + entry.first
                                                            : entry.first);
                }
            }
            box.clear(dontSendNotification);
            auto *root = box.getRootMenu();
            root->addItem(1, "None");
            root->addSeparator();
            PopupMenu oscillators, filters, mixer;
            for (int i = 0; i < 4; ++i)
                oscillators.addSubMenu(families[i], menus[size_t(i)]);
            filters.addSubMenu("Cutoff", menus[4]);
            filters.addSubMenu("Resonance", menus[5]);
            mixer.addSubMenu("Mix", menus[6]);
            for (const auto &entry : entries[13])
                mixer.addItem(entry.second, entry.first);
            root->addSubMenu("Oscillators", oscillators);
            root->addSubMenu("Filters", filters);
            root->addSubMenu("Mixer / effects", mixer);
            root->addSubMenu("LFO controls", menus[12]);
            for (int i = 7; i < 12; ++i)
                root->addSubMenu(i == 9    ? "FM index"
                                 : i == 10 ? "AM depth"
                                 : i == 11 ? "Width / PWM"
                                           : families[i],
                                 menus[size_t(i)]);
        }
        static void buildMacroMenu(ComboBox &box, const StringArray &choices) {
            std::array<PopupMenu, 11> menus;
            std::array<std::vector<std::pair<String, int>>, 11> groups;
            const StringArray players{"A1", "A2", "B1", "B2", "C1", "C2", "A", "B", "C"};
            for (int i = 1; i < choices.size(); ++i) {
                String name = choices[i], scope;
                int family = 10;
                if (name.startsWith("WT Pos / Start")) {
                    family = 0;
                    scope = name.fromFirstOccurrenceOf("WT Pos / Start", false, false).trim();
                    name = "WT Pos " + scope;
                } else if (name.startsWith("Width ") || name.startsWith("Pulse width ")) {
                    family = 1;
                    scope = name.fromLastOccurrenceOf(" ", false, false);
                    name = "Width " + scope;
                } else if (name.startsWith("FM ")) {
                    family = 2;
                    scope = name.substring(3);
                } else if (name.startsWith("AM ")) {
                    family = 3;
                    scope = name.substring(3);
                } else if (name.startsWith("Stretch ") || name.startsWith("Time factor ")) {
                    family = 4;
                    scope = name.fromLastOccurrenceOf(" ", false, false);
                    name = "Stretch " + scope;
                } else if (name.startsWith("Cutoff ")) {
                    family = 5;
                    scope = name.substring(7);
                } else if (name.startsWith("Resonance ")) {
                    family = 6;
                    scope = name.substring(10);
                    name = "Reso " + scope;
                } else if (name.startsWith("Mix ")) {
                    family = 7;
                    scope = name.substring(4);
                } else if (name.startsWith("Filter "))
                    family = 8;
                else if (name.startsWith("LFO"))
                    family = 9;
                groups[size_t(family)].emplace_back(name, i + 1);
            }
            for (size_t i = 0; i < groups.size(); ++i) {
                auto &entries = groups[i];
                std::stable_sort(entries.begin(), entries.end(), [&](const auto &a, const auto &b) {
                    auto rank = [&](const String &label) {
                        int n = players.indexOf(label.fromLastOccurrenceOf(" ", false, false));
                        return n < 0 ? 99 : n;
                    };
                    return rank(a.first) < rank(b.first);
                });
                for (auto &entry : entries)
                    menus[i].addItem(entry.second, entry.first);
            }
            box.clear(dontSendNotification);
            auto *root = box.getRootMenu();
            root->addItem(1, "None");
            root->addSeparator();
            root->addSubMenu("Filter control", menus[8]);
            PopupMenu sources;
            const StringArray families{"WT position / sample start", "Width / PWM", "FM index",
                                       "AM depth", "Stretch"};
            for (int i = 0; i < 5; ++i)
                sources.addSubMenu(families[i], menus[size_t(i)]);
            root->addSubMenu("Oscillators", sources);
            PopupMenu filters;
            filters.addSubMenu("Cutoff", menus[5]);
            filters.addSubMenu("Resonance", menus[6]);
            root->addSubMenu("Filters", filters);
            root->addSubMenu("Mixer", menus[7]);
            root->addSubMenu("Effects", menus[10]);
            root->addSubMenu("LFO", menus[9]);
        }
        void bindComposite(Param extra, bool detune) {
            compositeId = extra;
            compositeDetune = detune;
            compositeChoice = !detune && !compositePolyphony;
            auto *base = owner.processor.state.getParameter(spec.id);
            auto *extension = owner.processor.state.getParameter(parameterSpecs[size_t(extra)].id);
            // Attachments can run before the APVTS raw-value adapter. Keep the
            // supplied values so combined controls do not display the prior event.
            compositeValues[0] = base->convertFrom0to1(base->getValue());
            compositeValues[1] = extension->convertFrom0to1(extension->getValue());
            compositeBase = std::make_unique<ParameterAttachment>(
                *base,
                [this](float value) {
                    compositeValues[0] = value;
                    syncComposite();
                },
                nullptr);
            compositeExtra = std::make_unique<ParameterAttachment>(
                *extension,
                [this](float value) {
                    compositeValues[1] = value;
                    syncComposite();
                },
                nullptr);
            compositeWideId = detune ? detuneWide(String(spec.id).substring(1).getIntValue() - 1)
                              : compositePolyphony ? Param::Count
                                                   : pairedChoiceExtension(id);
            if (compositeWideId != Param::Count) {
                auto *wide =
                    owner.processor.state.getParameter(parameterSpecs[size_t(compositeWideId)].id);
                compositeValues[2] = wide->convertFrom0to1(wide->getValue());
                compositeWide = std::make_unique<ParameterAttachment>(
                    *wide,
                    [this](float value) {
                        compositeValues[2] = value;
                        syncComposite();
                    },
                    nullptr);
            }
            compositeBase->sendInitialUpdate();
            compositeExtra->sendInitialUpdate();
            if (compositeWide)
                compositeWide->sendInitialUpdate();
        }
        void syncComposite() {
            if (compositeUpdate)
                return;
            const ScopedValueSetter<bool> guard(compositeUpdate, true);
            if (compositeDetune || compositePolyphony)
                slider.setValue(compositeValues[0] + compositeValues[1] + compositeValues[2],
                                dontSendNotification);
            else
                menu.setSelectedId(int(expandedChoiceValue(
                                       id,
                                       [this](size_t parameter) {
                                           if (parameter == size_t(id))
                                               return compositeValues[0];
                                           if (parameter == size_t(compositeId))
                                               return compositeValues[1];
                                           if (parameter == size_t(compositeWideId))
                                               return compositeValues[2];
                                           return owner.processor.state
                                               .getRawParameterValue(parameterSpecs[parameter].id)
                                               ->load();
                                       })) +
                                       1,
                                   dontSendNotification);
        }
        void writeComposite(float value) {
            const ScopedValueSetter<bool> guard(compositeUpdate, true);
            const float firstMaximum = parameterSpecs[size_t(compositeId)].high;
            const float combinedMaximum = spec.high + firstMaximum;
            const float base = compositePolyphony   ? std::min(spec.high, value)
                               : compositeDetune    ? std::min(100.f, value)
                               : value <= spec.high ? value
                                                    : 0;
            const float extra = compositePolyphony ? std::max(0.f, value - spec.high)
                                : compositeDetune  ? std::clamp(value - 100, 0.f, 100.f)
                                : value > spec.high && value <= combinedMaximum ? value - spec.high
                                                                                : 0;
            const float wide = compositeDetune           ? std::max(0.f, value - 200)
                               : value > combinedMaximum ? value - combinedMaximum
                                                         : 0;
            auto write = [&](std::unique_ptr<ParameterAttachment> &attachment, float v) {
                if (!attachment)
                    return;
                if (compositeGesture)
                    attachment->setValueAsPartOfGesture(v);
                else
                    attachment->setValueAsCompleteGesture(v);
            };
            write(compositeBase, base);
            write(compositeExtra, extra);
            write(compositeWide, wide);
        }
        Param compositeId = Param::Count, compositeWideId = Param::Count;
        std::array<float, 3> compositeValues{};
        bool compositeChoice = false, compositeDetune = false, compositePolyphony = false,
             compositeUpdate = false, compositeGesture = false;
        std::unique_ptr<ParameterAttachment> compositeBase, compositeExtra, compositeWide;
        Console &owner;
        const ParameterSpec &spec;
        Param id;
        bool attachedDrag = false;
        Style kind = Style::Knob;
        String caption;
        Label label;
        Slider slider;
        ComboBox menu;
        ToggleButton toggle;
        std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
        std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> choiceAttachment;
        std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> toggleAttachment;
    };
    class RackButton : public Button, private ThemeText {
      public:
        RackButton(Console &c, int i, String n)
            : Button(n), ThemeText(c), owner(c), index(i), name(n) {
            onClick = [this] {
                if (!dragged)
                    owner.setRack(index);
            };
            setWantsKeyboardFocus(true);
            setTooltip(owner.effectPosition(index) >= 0
                           ? "Drag to reorder audio effects. Alt+Up/Down or right-click to move."
                       : index == 12 ? "Final output limiter: fixed after Main"
                                     : name);
        }
        void mouseDown(const MouseEvent &e) override {
            dragged = false;
            const int position = owner.effectPosition(index);
            if (e.mods.isPopupMenu() && position >= 0) {
                auto menu = owner.look.contextMenu();
                menu.addItem(1, "Move up", position > 0);
                menu.addItem(2, "Move down", position < effectStageCount - 1);
                menu.addSeparator();
                menu.addItem(3, "Reset effect order");
                Component::SafePointer<RackButton> safe(this);
                menu.showMenuAsync(
                    PopupMenu::Options().withTargetComponent(this), [safe](int action) {
                        if (!safe || action == 0)
                            return;
                        if (action == 3)
                            safe->owner.processor.setEffectOrder(EffectOrder{});
                        else {
                            const int at = safe->owner.effectPosition(safe->index);
                            safe->owner.processor.moveEffect(at, at + (action == 1 ? -1 : 1));
                        }
                        safe->owner.layout();
                        safe->owner.repaint();
                    });
                return;
            }
            Button::mouseDown(e);
        }
        void mouseDrag(const MouseEvent &e) override {
            if (owner.effectPosition(index) >= 0 && e.getDistanceFromDragStart() > 5) {
                dragged = true;
                owner.dragRack(index, e.getEventRelativeTo(&owner).getPosition());
            }
            Button::mouseDrag(e);
        }
        void mouseUp(const MouseEvent &e) override {
            Button::mouseUp(e);
            dragged = false;
        }
        bool keyPressed(const KeyPress &key) override {
            const int at = owner.effectPosition(index);
            if (at >= 0 && key.getModifiers().isAltDown() &&
                (key.isKeyCode(KeyPress::upKey) || key.isKeyCode(KeyPress::downKey))) {
                owner.processor.moveEffect(at, at + (key.isKeyCode(KeyPress::upKey) ? -1 : 1));
                owner.layout();
                owner.repaint();
                return true;
            }
            return Button::keyPressed(key);
        }
        void paintButton(Graphics &g, bool hover, bool) override {
            bool selected = owner.rack == index && !owner.settingsOpen;
            g.setColour(selected ? owner.screenLine.brighter(.1f)
                        : hover  ? owner.screenLine
                                 : Colours::transparentBlack);
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
            const bool compact = index == 8 || index == 0 || index == 9 || index == 10;
            const bool tool = index == 9 || index == 10;
            auto area = tool ? getLocalBounds() : getLocalBounds().withTrimmedLeft(21);
            if (!compact)
                area = area.withTrimmedRight(11);
            drawCaption(g, index == 0 ? "ARP" : name, area, compact ? (tool ? 9.5f : 10.f) : 10.f,
                        owner.screenText.withAlpha(selected ? 1.f : .78f),
                        tool ? Justification::centred : Justification::centredLeft, selected);
            if (owner.effectPosition(index) >= 0) {
                g.setColour(owner.screenText.withAlpha(hover || dragged ? .8f : .35f));
                for (int y : {7, 10, 13})
                    for (int x : {getWidth() - 8, getWidth() - 5})
                        g.fillEllipse(float(x), float(y), 1.4f, 1.4f);
            }
            if (selected) {
                g.setColour(owner.mint);
                g.fillRoundedRectangle(0, 5, 2, float(getHeight() - 10), 1);
            }
        }

      private:
        Console &owner;
        int index;
        String name;
        bool dragged = false;
    };
    class DistortionButton : public Button, private ThemeText {
        Console &owner;
        int slot;
        bool dragged = false;

      public:
        DistortionButton(Console &c, int index)
            : Button("Distortion stage"), ThemeText(c), owner(c), slot(index) {
            setComponentID("dist.slot." + String(slot));
            setWantsKeyboardFocus(true);
            setTooltip("Select a stage. Drag to change order; Alt+Left/Right also moves it.");
            onClick = [this] {
                if (!dragged) {
                    owner.selectedDistortion = slot;
                    owner.layout();
                    owner.repaint();
                }
            };
        }
        void mouseDown(const MouseEvent &e) override {
            dragged = false;
            if (e.mods.isPopupMenu()) {
                const auto position = owner.distortionPosition(slot);
                auto menu = owner.look.contextMenu();
                menu.addItem(1, "Move left", position > 0);
                menu.addItem(2, "Move right", position < owner.distortionColumns() - 1);
                SafePointer<DistortionButton> safe(this);
                menu.showMenuAsync(
                    PopupMenu::Options().withTargetComponent(this), [safe](int action) {
                        if (safe && action) {
                            const auto at = safe->owner.distortionPosition(safe->slot);
                            safe->owner.processor.moveDistortion(at, at + (action == 1 ? -1 : 1));
                            safe->owner.layout();
                            safe->owner.repaint();
                        }
                    });
                return;
            }
            Button::mouseDown(e);
        }
        void mouseDrag(const MouseEvent &e) override {
            if (e.getDistanceFromDragStart() > 5) {
                dragged = true;
                const auto x = e.getEventRelativeTo(&owner).position.x;
                const int at = owner.distortionPosition(slot);
                owner.processor.moveDistortion(
                    at, jlimit(0, owner.distortionColumns() - 1,
                               int((x - 414) / (520 / owner.distortionColumns()))));
                owner.layout();
                owner.repaint();
            }
            Button::mouseDrag(e);
        }
        void mouseUp(const MouseEvent &e) override {
            Button::mouseUp(e);
            dragged = false;
        }
        bool keyPressed(const KeyPress &key) override {
            if (key.getModifiers().isAltDown() &&
                (key.isKeyCode(KeyPress::leftKey) || key.isKeyCode(KeyPress::rightKey))) {
                const auto at = owner.distortionPosition(slot);
                owner.processor.moveDistortion(
                    at, jlimit(0, owner.distortionColumns() - 1,
                               at + (key.isKeyCode(KeyPress::leftKey) ? -1 : 1)));
                owner.layout();
                owner.repaint();
                return true;
            }
            return Button::keyPressed(key);
        }
        void paintButton(Graphics &g, bool hover, bool) override {
            const bool selected = owner.selectedDistortion == slot;
            g.setColour(owner.screenLine.withAlpha(selected ? 1.f : hover ? .8f : .4f));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
            const StringArray names{"OVERDRIVE", "FOLDBACK", "CLIP", "DECIMATE", "BITCRUSH"};
            const auto type = jlimit(0, 4, int(owner.value(distortionParameter(slot, 1))));
            drawCaption(g,
                        (owner.distortionIsMultiband()
                             ? "B" + String(owner.distortionPosition(slot) + 1) + "/"
                             : String(slot + 1) + " / ") +
                            names[type],
                        getLocalBounds().reduced(5, 0), 9, owner.screenText, Justification::centred,
                        selected);
        }
    };
    class PatternCanvas : public Component, private ThemeText {
      public:
        explicit PatternCanvas(Console &c) : ThemeText(c), owner(c) {
            setComponentID("pattern.canvas");
            setName("Draw pattern lanes");
            setWantsKeyboardFocus(true);
            addChildComponent(input);
            input.setJustification(Justification::centred);
            input.onReturnKey = [this] { commit(); };
            input.onEscapeKey = [this] { input.setVisible(false); };
            input.onFocusLost = [this] { commit(); };
        }
        ~PatternCanvas() override {
            finishGesture();
            input.onFocusLost = nullptr;
        }
        void paint(Graphics &g) override {
            int step = owner.processor.arpPlayhead.load();
            for (int row = 0; row < 7; ++row) {
                int lane = std::min(row, 5);
                if (row == 6)
                    lane = 3;
                int length = int(owner.value(Param(size_t(Param::arpnotelen) + size_t(lane))));
                for (int col = 0; col < 16; ++col)
                    if (owner.stepPage * 16 + col >= length) {
                        g.setColour(owner.screen.withAlpha(.55f));
                        g.fillRect(col * 26, row * 22, 24, 19);
                    }
                if (step >= 0) {
                    int col = step % length - owner.stepPage * 16;
                    if (col >= 0 && col < 16) {
                        g.setColour(owner.mint.withAlpha(.65f));
                        g.drawRoundedRectangle(
                            Rectangle<float>(float(col * 26), float(row * 22), 24, 19), 2, 1);
                    }
                }
            }
        }
        void mouseDown(const MouseEvent &e) override {
            if (e.mods.isRightButtonDown() || e.getNumberOfClicks() > 1) {
                showValue(e.position);
                return;
            }
            paintRow = std::clamp(int(e.position.y) / 22, 0, 6);
            drag(e.position, true);
        }
        void mouseDrag(const MouseEvent &e) override {
            if (!input.isVisible())
                drag(e.position, false);
        }
        void mouseUp(const MouseEvent &) override {
            finishGesture();
        }
        void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &wheel) override {
            int row = std::clamp(int(e.position.y) / 22, 0, 6),
                column = std::clamp(int(e.position.x) / 26, 0, 15);
            auto *p = parameter(row, column);
            auto *spec = specification(row, column);
            float current = owner.processor.state.getRawParameterValue(spec->id)->load();
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(
                jlimit(spec->low, spec->high, current + (wheel.deltaY > 0 ? 1.f : -1.f))));
            p->endChangeGesture();
        }
        void visibilityChanged() override {
            if (!isVisible()) {
                finishGesture();
                input.setVisible(false);
            }
        }

      private:
        String identifier(int row, int column) const {
            int step = owner.stepPage * 16 + column + 1;
            if (row == 2 || row == 6)
                return "step" + String(step) + (row == 2 ? "velocity" : "hold");
            return "pat" + String(step) +
                   StringArray{"note", "oct", "", "gate", "repeat", "chance"}[row];
        }
        const ParameterSpec *specification(int row, int column) const {
            auto id = identifier(row, column);
            for (const auto &spec : parameterSpecs)
                if (id == spec.id)
                    return &spec;
            jassertfalse;
            return &parameterSpecs[0];
        }
        RangedAudioParameter *parameter(int row, int column) const {
            return owner.processor.state.getParameter(identifier(row, column));
        }
        void finishGesture() {
            if (active) {
                active->endChangeGesture();
                active = nullptr;
            }
        }
        void drag(Point<float> at, bool initial) {
            int column = std::clamp(int(at.x) / 26, 0, 15);
            auto *p = parameter(paintRow, column);
            auto *spec = specification(paintRow, column);
            if (active != p) {
                finishGesture();
                active = p;
                active->beginChangeGesture();
            }
            float proportion = 1 - jlimit(0.f, 18.f, at.y - paintRow * 22) / 18;
            float value = std::round(spec->low + (spec->high - spec->low) * proportion);
            if (paintRow == 6) {
                if (!initial && lastColumn == column)
                    return;
                value =
                    owner.processor.state.getRawParameterValue(spec->id)->load() > 0 ? 0.f : 1.f;
            }
            active->setValueNotifyingHost(active->convertTo0to1(value));
            lastColumn = column;
        }
        void showValue(Point<float> at) {
            finishGesture();
            inputRow = std::clamp(int(at.y) / 22, 0, 6);
            inputColumn = std::clamp(int(at.x) / 26, 0, 15);
            const auto *spec = specification(inputRow, inputColumn);
            float value = owner.processor.state.getRawParameterValue(spec->id)->load();
            if (inputRow == 0)
                value += 1;
            input.setBounds(inputColumn * 26, inputRow * 22, 24, 19);
            input.setText(String(int(value)), false);
            input.setColour(TextEditor::backgroundColourId, owner.screen);
            input.setColour(TextEditor::textColourId, owner.screenText);
            input.setFont(font(10));
            input.setVisible(true);
            input.grabKeyboardFocus();
            input.selectAll();
        }
        void commit() {
            if (!input.isVisible())
                return;
            auto *p = parameter(inputRow, inputColumn);
            auto *spec = specification(inputRow, inputColumn);
            float value = float(input.getText().getDoubleValue()) - (inputRow == 0 ? 1.f : 0.f);
            input.setVisible(false);
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(jlimit(spec->low, spec->high, value)));
            p->endChangeGesture();
        }
        Console &owner;
        TextEditor input;
        RangedAudioParameter *active = nullptr;
        int paintRow = 0, lastColumn = -1, inputRow = 0, inputColumn = 0;
    };

    class Keys : public MidiKeyboardComponent, private ThemeText {
      public:
        Keys(MidiKeyboardState &s, Console &c)
            : MidiKeyboardComponent(s, horizontalKeyboard), ThemeText(c), owner(c), midi(s) {
            applyTheme();
        }
        void applyTheme() {
            setColour(whiteNoteColourId, owner.darkTheme
                                             ? owner.ink.interpolatedWith(owner.body, .22f)
                                             : Colour(0xfff8f7f1));
            setColour(blackNoteColourId, owner.darkTheme ? owner.body.darker(.3f) : owner.ink);
            setColour(keySeparatorLineColourId, owner.line);
            setColour(mouseOverKeyOverlayColourId, owner.mint.withAlpha(.4f));
            setColour(keyDownOverlayColourId, owner.green.withAlpha(.55f));
            setColour(textLabelColourId, owner.darkTheme ? owner.line : owner.muted);
            setColour(shadowColourId, Colours::transparentBlack);
        }
        void refreshPlayedKeys() {
            for (int note = getRangeStart(); note <= getRangeEnd(); ++note)
                if (getRectangleForKey(note).intersects(getLocalBounds().toFloat()) &&
                    drawn[size_t(note)] != midi.isNoteOnForChannels(getMidiChannelsToDisplay(), note)) {
                    repaint();
                    break;
                }
        }
        void drawWhiteNote(int note, Graphics &g, Rectangle<float> b, bool down, bool over,
                           Colour lineColour, Colour textColour) override {
            if (g.clipRegionIntersects(b.getSmallestIntegerContainer()))
                drawn[size_t(note)] = down;
            MidiKeyboardComponent::drawWhiteNote(note, g, b, down, over, lineColour, textColour);
        }
        void drawBlackNote(int note, Graphics &g, Rectangle<float> b, bool down, bool over,
                           Colour) override {
            if (g.clipRegionIntersects(b.getSmallestIntegerContainer()))
                drawn[size_t(note)] = down;
            g.setColour(down              ? owner.green
                        : over            ? owner.line.brighter(.15f)
                        : owner.darkTheme ? owner.body.darker(.3f)
                                          : owner.ink);
            g.fillRoundedRectangle(b.withTrimmedBottom(4), 2);
            g.setColour(Colours::white.withAlpha(.08f));
            g.fillRoundedRectangle(b.reduced(2, 1).withHeight(b.getHeight() - 9), 1);
        }

      private:
        Console &owner;
        MidiKeyboardState &midi;
        std::array<bool, 128> drawn{};
    };

  public:
    Console(LogisthProcessor &p, LogisthLook &skin)
        : processor(p), look(skin), keyboard(p.keyboard, *this), patternCanvas(*this),
          waveView(p, *this, [this](float h) { return font(h); }),
          scopeView(p, *this, [this](float h) { return font(h); }),
          impulseView(p, *this, [this](float h) { return font(h); }),
          dynamicsView(p, *this, [this](float h) { return font(h); }),
          eqView(p, *this, [this] { return int(value(Param::eqmode)); }),
          distortionView(p, *this, [this] { return distortionBandCount(); }), tooltips(this, 650) {
        setLookAndFeel(&look);
        addChildComponent(patternCanvas);
        addChildComponent(waveView);
        addAndMakeVisible(scopeView);
        addChildComponent(eqView);
        addChildComponent(distortionView);
        distortionView.onSelectionChanged = [this](int band) {
            selectedDistortion = int(processor.distortionOrder().slots[size_t(band)]);
            layout();
        };
        distAnalysis.setComponentID("distortion.analyzer.mode");
        distAnalysis.addItemList({"Off", "Pre", "Post", "Pre + Post"}, 1);
        distTilt.setComponentID("distortion.analyzer.tilt");
        distTilt.addItemList({"0 dB/oct", "1.5 dB/oct", "3 dB/oct", "4.5 dB/oct", "6 dB/oct"}, 1);
        for (auto *box : {&distAnalysis, &distTilt}) {
            box->getProperties().set("dark", true);
            addChildComponent(*box);
            box->onChange = [this] {
                const auto selected = UserSettings::Analyzer{distAnalysis.getSelectedId() - 1,
                                                             distTilt.getSelectedId() - 1};
                UserSettings::setDistortionAnalyzer(selected);
                distortionView.setAnalyzer(selected.mode, selected.tilt);
                processor.distortionSpectrumEnabled = selected.mode && distortionView.isShowing();
                distortionView.repaint();
            };
        }
        const auto distAnalyzer = UserSettings::distortionAnalyzer();
        distAnalysis.setSelectedId(distAnalyzer.mode + 1, dontSendNotification);
        distTilt.setSelectedId(distAnalyzer.tilt + 1, dontSendNotification);
        distortionView.setAnalyzer(distAnalyzer.mode, distAnalyzer.tilt);
        addChildComponent(impulseView);
        addChildComponent(dynamicsView);
        eqBandSelect.setComponentID("eq.band.select");
        for (int i = 0; i < eqMaxBands; ++i)
            eqBandSelect.addItem("BAND " + String(i + 1), i + 1);
        eqBandSelect.setSelectedId(1, dontSendNotification);
        eqBandSelect.onChange = [this] {
            selectedEqBand = eqBandSelect.getSelectedId() - 1;
            eqView.selectBand(selectedEqBand);
            layout();
            repaint();
        };
        eqView.onSelectionChanged = [this](int band) {
            selectedEqBand = band;
            eqBandSelect.setSelectedId(band + 1, dontSendNotification);
            layout();
        };
        eqAdd.setComponentID("eq.band.add");
        eqAdd.setButtonText("+ BAND");
        eqAdd.onClick = [this] {
            for (int i = 0; i < eqMaxBands; ++i)
                if (value(Param(size_t(eqBandTypes[size_t(i)]) + 3)) < .5f) {
                    logisth_ui::change(processor,
                                       parameterSpecs[size_t(eqBandTypes[size_t(i)]) + 3].id, 1);
                    eqBandSelect.setSelectedId(i + 1, sendNotificationSync);
                    break;
                }
        };
        eqRemove.setComponentID("eq.band.remove");
        eqRemove.setButtonText("REMOVE");
        eqRemove.onClick = [this] {
            logisth_ui::change(
                processor, parameterSpecs[size_t(eqBandTypes[size_t(selectedEqBand)]) + 3].id, 0);
            eqView.repaint();
        };
        for (Component *c : std::array<Component *, 3>{&eqBandSelect, &eqAdd, &eqRemove})
            addChildComponent(*c);
        for (int i = 0; i < 4; ++i) {
            distortionButtons[size_t(i)] = std::make_unique<DistortionButton>(*this, i);
            addChildComponent(*distortionButtons[size_t(i)]);
        }
        distortionHelp.setText("Signal flows left to right.\nSelect a stage to edit its model, "
                               "drive and mix.\nDrag stage headers to change their order.",
                               dontSendNotification);
        distortionHelp.setFont(font(10));
        addChildComponent(distortionHelp);
        eqPre.setComponentID("eq.analyzer.pre");
        eqPost.setComponentID("eq.analyzer.post");
        eqPre.setButtonText("PRE");
        eqPost.setButtonText("POST");
        eqPre.setTooltip("Show the spectrum at the EQ input");
        eqPost.setTooltip("Show the spectrum at the EQ output");
        for (auto *button : {&eqPre, &eqPost}) {
            button->setClickingTogglesState(true);
            button->onClick = [this] { saveEqAnalyzer(); };
        }
        eqTilt.setComponentID("eq.analyzer.tilt");
        eqTilt.addItemList({"0 dB/oct", "1.5 dB/oct", "3 dB/oct", "4.5 dB/oct", "6 dB/oct"}, 1);
        eqTilt.getProperties().set("dark", true);
        eqTilt.setTooltip("Analyzer display tilt. The EQ response stays the same.");
        eqTilt.onChange = [this] { saveEqAnalyzer(); };
        eqAnalysisLabel.setText("ANALYZER", dontSendNotification);
        eqTiltLabel.setText("TILT", dontSendNotification);
        for (auto *label : {&eqAnalysisLabel, &eqTiltLabel})
            label->setFont(font(9));
        for (Component *c : std::array<Component *, 5>{&eqPre, &eqPost, &eqTilt, &eqAnalysisLabel, &eqTiltLabel})
            addChildComponent(*c);
        applyEqAnalyzer(UserSettings::analyzer());
        modulationToggle.setComponentID("ui.modulation");
        modulationToggle.setButtonText("MODULATION -");
        modulationToggle.setName("Show or hide modulation controls");
        modulationToggle.setTooltip("Collapse the modulation panel. Modulation continues playing.");
        modulationToggle.getProperties().set("fontSize", 9);
        modulationToggle.onClick = [this] {
            if (onModulationRequested)
                onModulationRequested(!modulationCollapsed);
        };
        addAndMakeVisible(modulationToggle);
        setSize(canvasWidth, canvasHeight);
        for (size_t i = 0; i < parameterCount; ++i) {
            controls[i] = std::make_unique<Control>(*this, Param(i));
            addChildComponent(*controls[i]);
        }
        for (int i = 0; i < partCount; ++i) {
            auto &button = parts[size_t(i)];
            button.setComponentID("part." + String(i));
            button.setButtonText(String::charToString(juce_wchar('A' + i)));
            button.onClick = [this, i] { setPart(i); };
            addAndMakeVisible(button);
        }
        for (int i = 0; i < rackNames.size(); ++i) {
            rackButtons[size_t(i)] = std::make_unique<RackButton>(*this, i, rackNames[i]);
            rackButtons[size_t(i)]->setComponentID("rack." + String(i));
            addAndMakeVisible(*rackButtons[size_t(i)]);
        }
        for (int i = 0; i < 2; ++i) {
            stepPages[size_t(i)].setButtonText(i == 0 ? "01-08" : "09-16");
            stepPages[size_t(i)].onClick = [this, i] {
                stepPage = i;
                layout();
            };
            stepPages[size_t(i)].getProperties().set("dark", true);
            addChildComponent(stepPages[size_t(i)]);
        }
        presets.setTextWhenNothingSelected("Current patch");
        presets.getProperties().set("fontSize", 12.f);
        refreshPresets();
        presets.onChange = [this] { selectPreset(presets.getSelectedId()); };
        searchButton.onClick = [this] { showPresetSearch(!searchPanel.isVisible()); };
        addAndMakeVisible(searchButton);
        searchPanel.setComponentID("preset.search.panel");
        searchQuery.setComponentID("preset.search.query");
        searchQuery.setFont(font(12));
        searchQuery.setTextToShowWhenEmpty("Search factory and user presets...", muted);
        searchQuery.addKeyListener(this);
        searchQuery.onTextChange = [this] { filterPresets(); };
        searchQuery.onReturnKey = [this] { chooseSearchResult(); };
        searchQuery.onEscapeKey = [this] { showPresetSearch(false); };
        searchPanel.addAndMakeVisible(searchQuery);
        searchResults.setComponentID("preset.search.results");
        searchResults.setModel(this);
        searchResults.addKeyListener(this);
        searchResults.setRowHeight(29);
        searchPanel.addAndMakeVisible(searchResults);
        searchEmpty.setText("No matching presets", dontSendNotification);
        searchEmpty.setFont(font(12));
        searchPanel.addChildComponent(searchEmpty);
        addChildComponent(searchPanel);
        addAndMakeVisible(presets);
        previous.setButtonText(String(CharPointer_UTF8("‹")));
        next.setButtonText(String(CharPointer_UTF8("›")));
        previous.onClick = [this] {
            int i = presets.getSelectedItemIndex();
            presets.setSelectedItemIndex(i <= 0 ? presets.getNumItems() - 1 : i - 1,
                                         sendNotificationSync);
        };
        next.onClick = [this] {
            int i = presets.getSelectedItemIndex();
            presets.setSelectedItemIndex(i < 0 || i >= presets.getNumItems() - 1 ? 0 : i + 1,
                                         sendNotificationSync);
        };
        presetFiles.setButtonText("PATCH");
        presetFiles.onClick = [this] { presetMenu(); };
        addAndMakeVisible(presetFiles);
        addAndMakeVisible(previous);
        addAndMakeVisible(next);
        keyboard.setAvailableRange(36, 96);
        keyboard.setOctaveForMiddleC(4);
        keyboard.setKeyWidth(24);
        addAndMakeVisible(keyboard);
        for (auto *wheel : std::array<Slider *, 2>{&pitch, &mod}) {
            wheel->setSliderStyle(Slider::LinearVertical);
            wheel->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            wheel->getProperties().set("trim", true);
            addAndMakeVisible(wheel);
        }
        pitch.setTooltip("Pitch bend from -12 to +12. BEND / st sets the range in semitones.");
        mod.setTooltip("Modulation wheel");
        auto attachWheel = [this](Slider &slider, const char *id) {
            slider.setComponentID("wheel." + String(id));
            slider.getProperties().set("hostParameterID", id);
            return std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
                processor.state, id, slider);
        };
        pitchAttachment = attachWheel(pitch, "bendwheel");
        modAttachment = attachWheel(mod, "modwheel");
        settings.setComponentID("ui.settings");
        settings.setButtonText("SETTINGS");
        settings.setTooltip("Display, text, theme, MIDI, file folders and About logisth");
        settings.onClick = [this] {
            showPresetSearch(false);
            settingsOpen = !settingsOpen;
            layout();
            repaint();
        };
        addAndMakeVisible(settings);
        presetNotes.setComponentID("preset.comment");
        presetNotes.setName("Preset comment");
        presetNotes.setFont(font(10));
        presetNotes.setTextToShowWhenEmpty("Preset comment - click to edit", muted);
        presetNotes.setInputRestrictions(4096);
        presetNotes.onTextChange = [this] {
            if (notesRevision != processor.presetRevision.load()) {
                syncPresetNotes();
                return;
            }
            processor.setPresetComment(presetNotes.getText());
        };
        syncPresetNotes();
        presetNotes.onEscapeKey = [this] { presetNotes.giveAwayKeyboardFocus(); };
        addAndMakeVisible(presetNotes);
        settingsPanel.setComponentID("settings.panel");
        addChildComponent(settingsPanel);
        initialiseSettings();
        initialiseMorph();
        for (int i = 0; i < playerCount; ++i) {
            auto &view = sourceViews[size_t(i)];
            view.setComponentID("source.view." + String(i));
            view.addItemList({"VOICE", "SOURCE", "ENV", "GRAINS", "MOTION", "TIME", "MOD",
                              "PLAYBACK", "ROUTING"},
                             1);
            view.setSelectedId(
                processor.state.getRawParameterValue("o" + String(i + 1) + "engine")->load() > 0
                    ? 2
                    : 1);
            view.setName("Oscillator view");
            view.onChange = [this] {
                layout();
                repaint();
            };
            addChildComponent(view);
            auto &stretch = stretchAlgorithms[size_t(i)];
            stretch.setComponentID("source.stretch.algorithm." + String(i));
            stretch.setName("Stretch algorithm to edit");
            stretch.onChange = [this] { layout(); repaint(); };
            addChildComponent(stretch);
            auto &load = sourceLoad[size_t(i)];
            load.setButtonText("IMPORT");
            load.onClick = [this, i] { chooseSource(i); };
            addChildComponent(load);
            auto &clear = sourceClear[size_t(i)];
            clear.setButtonText("RESET");
            clear.onClick = [this, i] {
                processor.clearSource(i);
                repaint();
            };
            addChildComponent(clear);
            auto &frame = sourceFrame[size_t(i)];
            frame.setComponentID("source.frame." + String(i));
            frame.addItemList({"AUTO", "128", "256", "512", "1024", "2048", "4096"}, 1);
            frame.setSelectedId(1);
            frame.getProperties().set("dark", true);
            frame.setTooltip("Wavetable samples per cycle: Auto reads WAV metadata, "
                             "otherwise 2048. Short single-cycle files use their actual length.");
            addChildComponent(frame);
        }
        for (int i = 0; i < playerCount; ++i) {
            sourceWave[size_t(i)].onClick = [this, i] {
                waveView.setSlot(i);
                waveSlot.setSelectedId(i + 1, dontSendNotification);
                setRack(9);
            };
            sourceWave[size_t(i)].setTooltip("Open source waveform / wavetable bank");
            addChildComponent(sourceWave[size_t(i)]);
        }
        for (int slot = 0; slot < playerCount; ++slot) {
            auto &copy = oscCopy[size_t(slot)], &paste = oscPaste[size_t(slot)];
            copy.setButtonText("Copy");
            paste.setButtonText("Paste");
            copy.setComponentID("osc.copy." + String(slot));
            paste.setComponentID("osc.paste." + String(slot));
            copy.setTooltip("Copy this oscillator, including its source audio and envelope.");
            paste.setTooltip("Paste the copied oscillator settings and source audio.");
            copy.onClick = [this, slot] {
                auto result = processor.copyOscillator(slot);
                readout = result.wasOk() ? "Copied oscillator " +
                                               StringArray{"A1", "A2", "B1", "B2", "C1", "C2"}[slot]
                                         : result.getErrorMessage();
                for (auto &button : oscPaste)
                    button.setEnabled(LogisthProcessor::hasOscillatorClipboard());
                repaint();
            };
            paste.onClick = [this, slot] {
                auto result = processor.pasteOscillator(slot);
                readout = result.wasOk() ? "Pasted oscillator " +
                                               StringArray{"A1", "A2", "B1", "B2", "C1", "C2"}[slot]
                                         : result.getErrorMessage();
                layout();
                repaint();
            };
            paste.setEnabled(LogisthProcessor::hasOscillatorClipboard());
            addChildComponent(copy);
            addChildComponent(paste);
        }
        impulseLoad.setButtonText("LOAD IR");
        impulseReset.setButtonText("RESET IR");
        impulseLoad.setComponentID("reverb.ir.load");
        impulseReset.setComponentID("reverb.ir.reset");
        impulseName.setComponentID("reverb.ir.name");
        impulseLoad.setTooltip("Load a mono/stereo WAV, AIFF or FLAC impulse response.");
        impulseLoad.onClick = [this] {
            impulseChooser = std::make_unique<FileChooser>(
                "Load impulse response", UserSettings::folder(UserSettings::Folder::Impulse),
                "*.wav;*.aif;*.aiff;*.flac");
            SafePointer<Console> safe(this);
            impulseChooser->launchAsync(
                FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                [safe](const FileChooser &chooser) {
                    if (safe && chooser.getResult().existsAsFile())
                        safe->processor.loadImpulseAsync(chooser.getResult());
                });
        };
        impulseReset.onClick = [this] {
            processor.clearImpulse();
            refreshImpulseLabel();
            repaint();
        };
        impulseName.setFont(font(10));
        impulseName.setJustificationType(Justification::centredLeft);
        addChildComponent(impulseLoad);
        addChildComponent(impulseReset);
        addChildComponent(impulseName);
        waveView.onFileDropped = [this](int slot, const File &file) {
            importDroppedSource(slot, file);
        };
        waveSlot.setComponentID("sample.slot");
        waveSlot.addItemList({"A1", "A2", "B1", "B2", "C1", "C2"}, 1);
        waveSlot.setSelectedId(1);
        waveSlot.onChange = [this] {
            waveView.setSlot(waveSlot.getSelectedId() - 1);
            layout();
            repaint();
        };
        waveSlot.getProperties().set("dark", true);
        addChildComponent(waveSlot);
        for (int i = 0; i < 4; ++i) {
            auto &button = scopeSources[size_t(i)];
            button.setButtonText(i == 0 ? "A" : i == 1 ? "B" : i == 3 ? "C" : "L+R");
            button.onClick = [this, i] {
                scopeView.setSource(i);
                processor.state.state.setProperty("uiScope", i, nullptr);
                for (int j = 0; j < 4; ++j)
                    scopeSources[size_t(j)].setToggleState(j == i, dontSendNotification);
            };
            addAndMakeVisible(button);
        }
        int initialScope = jlimit(0, 3, int(processor.state.state.getProperty("uiScope", 2)));
        scopeView.setSource(initialScope);
        scopeSources[size_t(initialScope)].setToggleState(true, dontSendNotification);
        patternSlot.addItemList({"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L"}, 1);
        patternSlot.setSelectedId(1);
        patternSlot.setTooltip("Recall pattern slot; SAVE stores the current pattern");
        patternSlot.onChange = [this] {
            processor.recallPattern(patternSlot.getSelectedId() - 1);
            layout();
            repaint();
        };
        addChildComponent(patternSlot);
        patternSave.setButtonText("SAVE");
        patternSave.onClick = [this] {
            processor.storePattern(patternSlot.getSelectedId() - 1);
            readout = "Pattern stored";
            repaint();
        };
        addChildComponent(patternSave);
        patternRandom.setButtonText("DICE");
        patternRandom.onClick = [this] {
            processor.randomisePattern();
            repaint();
        };
        addChildComponent(patternRandom);
        for (int note = 0; note < 128; ++note)
            chordTrigger.addItem(
                MidiMessage::getMidiNoteName(note, true, true, 4) + " / " + String(note), note + 1);
        chordTrigger.setSelectedId(61);
        chordTrigger.setName("Chord trigger note");
        chordTrigger.onChange = [this] {
            refreshChord();
            repaint();
        };
        addChildComponent(chordTrigger);
        chordType.addItemList({"Unison", "Major", "Minor", "Maj 7", "Min 7", "Dominant 7", "Sus 2",
                               "Sus 4", "Power", "Diminished", "Augmented"},
                              1);
        chordType.setTextWhenNothingSelected("Voicing");
        chordType.onChange = [this] {
            processor.setChord(chordTrigger.getSelectedId() - 1, chordType.getSelectedId() - 1);
            refreshChord();
        };
        addChildComponent(chordType);
        chordLearn.setButtonText("LEARN");
        chordLearn.setTooltip("Collect up to eight MIDI notes, then click DONE");
        chordLearn.onClick = [this] {
            if (processor.learning() >= 0)
                processor.finishLearn();
            else
                processor.learnChord(chordTrigger.getSelectedId() - 1);
            refreshChord();
        };
        addChildComponent(chordLearn);
        chordCopy.setButtonText("MAP ALL");
        chordCopy.onClick = [this] {
            int type = chordType.getSelectedId() - 1;
            if (type >= 0)
                for (int key = 0; key < 128; ++key)
                    processor.setChord(key, type);
            refreshChord();
        };
        chordCopy.setTooltip("Map this voicing type to every trigger, transposed to its root");
        addChildComponent(chordCopy);
        for (int i = 0; i < 8; ++i) {
            auto &note = chordNotes[size_t(i)];
            note.setSliderStyle(Slider::LinearBar);
            note.getProperties().set("number", true);
            note.setTextBoxStyle(Slider::TextBoxLeft, false, 50, 28);
            note.setRange(-1, 127, 1);
            note.textFromValueFunction = [](double value) {
                return value < 0 ? String("-")
                                 : MidiMessage::getMidiNoteName(int(value), true, true, 4);
            };
            note.valueFromTextFunction = [](const String &text) { return text.getDoubleValue(); };
            note.onValueChange = [this, i] {
                if (!updatingChord) {
                    processor.setChordNote(chordTrigger.getSelectedId() - 1, i,
                                           int(chordNotes[size_t(i)].getValue()));
                    refreshChord();
                }
            };
            note.setTooltip("MIDI note: drag or enter 0–127; -1 removes a voice");
            addChildComponent(note);
        }
        for (Component *widget :
             std::array<Component *, 7>{&patternSlot, &patternSave, &patternRandom, &chordTrigger,
                                        &chordType, &chordLearn, &chordCopy})
            widget->getProperties().set("dark", true);
        for (auto &note : chordNotes)
            note.getProperties().set("dark", true);
        refreshChord();
        readout = "Ready  /  Choose a sound or play a note";
        auto pending = processor.appearanceDraft;
        setTheme(ThemeSettings::reload(), false);
        if (pending && pending->baseline.same(savedTheme)) {
            draftTheme = pending->edited;
            themeName.setText(draftTheme.name, false);
            themeDark.setToggleState(draftTheme.dark, dontSendNotification);
            updateColourEditor();
            applyThemePalette(draftTheme);
            processor.appearanceDraft = pending;
            refreshStyleFields();
        }
        lastThemeScan = Time::getMillisecondCounter();
        ThemeSettings::addListener(this);
        startTimerHz(24);
    }
    ~Console() override {
        processor.dynamicsView = -1;
        processor.eqSpectrumEnabled = false;
        processor.distortionSpectrumEnabled = false;
        rememberDraft();
        searchResults.removeKeyListener(this);
        searchResults.setModel(nullptr);
        searchQuery.removeKeyListener(this);
        searchQuery.onTextChange = nullptr;
        colourHex.onFocusLost = nullptr;
        colourHex.onReturnKey = nullptr;
        themeName.onTextChange = nullptr;
        colourPicker.removeChangeListener(this);
        ThemeSettings::removeListener(this);
        stopTimer();
        for (auto &c : controls)
            c.reset();
        setLookAndFeel(nullptr);
    }
    bool isInterestedInFileDrag(const StringArray &files) override {
        return files.size() == 1 && File(files[0]).hasFileExtension("wav;aif;aiff;flac");
    }
    void filesDropped(const StringArray &files, int x, int y) override {
        if (!isInterestedInFileDrag(files))
            return;
        if (rack == 5 && int(value(Param::reverbtype)) == 5 &&
            Rectangle<int>(414, 289, 520, 244).contains(x, y)) {
            processor.loadImpulseAsync(File(files[0]));
            return;
        }
        if (rack == 9 && Rectangle<int>(414, 289, 520, 244).contains(x, y)) {
            importDroppedSource(waveView.selectedSlot(), File(files[0]));
            return;
        }
        int slot = y >= 82 && y <= 266 ? (x >= 16 && x <= 476     ? selectedPart * 2
                                          : x >= 764 && x <= 1224 ? selectedPart * 2 + 1
                                                                  : -1)
                                       : -1;
        if (slot < 0) {
            readout = "Drop audio on an oscillator panel";
            repaint();
            return;
        }
        importDroppedSource(slot, File(files[0]));
    }
    void importDroppedSource(int slot, const File &file) {
        processor.loadSourceAsync(slot, file, frameSizeForImport(slot), true);
    }
    void setDarkTheme(bool enabled, bool persist = true) {
        setTheme(enabled ? 1 : 0, persist);
    }
    void themeChanged(int selected) override {
        auto next = ThemeSettings::theme(selected);
        if (next.same(savedTheme)) {
            themeId = selected;
            look.themeId = selected;
            preferenceRevision = ThemeSettings::revision();
            refreshThemeChoices();
            return;
        }
        setTheme(selected, false);
    }
    void setTheme(int selected, bool persist = true) {
        if (persist && !ThemeSettings::choose(selected, this))
            readout = "Theme changed; user settings could not be saved.";
        preferenceRevision = ThemeSettings::revision();
        choosePalette(selected);
        refreshThemeChoices();
        savedTheme = ThemeSettings::theme(themeId);
        draftTheme = savedTheme;
        processor.appearanceDraft.reset();
        themeName.setText(draftTheme.name, false);
        themeDark.setToggleState(draftTheme.dark, dontSendNotification);
        refreshStyleFields();
        updateColourEditor();
        applyThemePalette(draftTheme);
    }
    ThemeDraft currentDraft() const {
        return {draftTheme, savedTheme};
    }
    void restoreDraft(const ThemeDraft &draft) {
        if (draft.baseline.same(savedTheme)) {
            draftTheme = draft.edited;
            themeName.setText(draftTheme.name, false);
            themeDark.setToggleState(draftTheme.dark, dontSendNotification);
            refreshStyleFields();
            updateColourEditor();
            applyThemePalette(draftTheme);
        }
    }
    void rememberDraft() {
        draftTheme.name = themeName.getText();
        if (!draftTheme.same(savedTheme))
            processor.appearanceDraft =
                std::make_shared<ThemeDraft>(ThemeDraft{draftTheme, savedTheme});
        else
            processor.appearanceDraft.reset();
    }
    void applyThemePalette(const ThemeDefinition &theme) {
        const auto previousText = typography;
        typography = UserSettings::text();
        look.typography = typography;
        applyPalette(theme);
        look.themeId = themeId;
        look.setPalette(theme);
        if (skinImageData != theme.backgroundImage) {
            skinImageData = theme.backgroundImage;
            skinImage = SkinBackground::decode(skinImageData);
        }
        headerLogo = brandLogo(ink);
        brandAboutLogo.image = brandLogo(screenText);
        brandAboutLogo.repaint();
        std::function<void(Component &)> updateFonts = [&](Component &node) {
            auto update = [&](const Font &old) {
                auto &properties = node.getProperties();
                if (!properties.contains("baseTextSize")) {
                    properties.set("baseTextSize",
                                   old.getHeight() / (1.18 * previousText.size / 100));
                    properties.set("baseTextBold", &node == &aboutLabel ||
                                                       (previousText.style == 0 && old.isBold()));
                }
                return font(float(properties["baseTextSize"]), bool(properties["baseTextBold"]));
            };
            if (auto *label = dynamic_cast<Label *>(&node))
                label->setFont(update(label->getFont()));
            if (auto *edit = dynamic_cast<TextEditor *>(&node)) {
                const auto desired = update(edit->getFont());
                if (edit->getFont() != desired)
                    edit->applyFontToAllText(desired, true);
            }
            for (auto *child : node.getChildren())
                updateFonts(*child);
        };
        updateFonts(*this);
        const bool customText = !typography.antialias || typography.quality != 0 ||
                                typography.edgeWeight != 1 || typography.style == 3 ||
                                typography.style == 4;
        if (customText)
            setCachedComponentImage(new logisth_ui::TextImage(*this, typography));
        else if (getCachedComponentImage())
            setCachedComponentImage(nullptr);
        userPreferenceRevision = UserSettings::revision();
        keyboard.applyTheme();
        for (ComboBox *box :
             {&themeChoice, &scaleChoice, &colourRole, &themeFont, &themeBackground, &themeKnobs,
              &backgroundFactory, &textSize, &textWeight, &textQuality, &distAnalysis, &distTilt})
            box->setColour(ComboBox::textColourId, screenText);
        for (auto &name : macroNames) {
            name.setColour(TextEditor::backgroundColourId, screen);
            name.setColour(TextEditor::outlineColourId, screenLine);
            name.applyColourToAllText(screenText, true);
        }
        for (TextEditor *editor : {&themeName, &colourHex}) {
            editor->setColour(TextEditor::backgroundColourId, panel);
            editor->applyColourToAllText(ink, true);
            editor->setColour(TextEditor::outlineColourId, line);
        }
        aboutInfo.setColour(TextEditor::backgroundColourId, screen);
        aboutInfo.setColour(TextEditor::outlineColourId, screenLine);
        aboutInfo.applyColourToAllText(screenText, true);
        colourPicker.setColour(ColourSelector::backgroundColourId, screen);
        waveSlot.setColour(ComboBox::textColourId, screenText);
        aboutSkinName.setColour(Label::textColourId, screenText);
        for (ComboBox *box : {&patternSlot, &chordTrigger, &chordType})
            box->setColour(ComboBox::textColourId, screenText);
        for (TextButton *button : {&patternSave, &patternRandom, &chordLearn, &chordCopy}) {
            button->setColour(TextButton::buttonColourId, screenLine);
            button->setColour(TextButton::textColourOffId, screenText);
        }
        for (Label *label : {&themeLabel, &themeHelp, &aboutLabel, &scaleLabel, &nameLabel,
                             &roleLabel, &hexLabel, &fontLabel, &backgroundLabel, &knobLabel})
            label->setColour(Label::textColourId, screenText);
        for (auto *c : settingsPanel.getChildren())
            if (auto *label = dynamic_cast<Label *>(c))
                label->setColour(Label::textColourId, screenText);
        for (auto &label : folderLabels)
            label.setColour(Label::textColourId, screenText);
        for (auto &label : folderValues)
            label.setColour(Label::textColourId, screenText);
        searchQuery.setColour(TextEditor::backgroundColourId, screen);
        searchQuery.applyColourToAllText(screenText, true);
        searchQuery.setTextToShowWhenEmpty("Search factory and user presets...", muted);
        searchResults.setColour(ListBox::backgroundColourId, screen);
        searchEmpty.setColour(Label::textColourId, mint);
        presetNotes.setColour(TextEditor::backgroundColourId, panel);
        presetNotes.applyColourToAllText(ink, true);
        presetNotes.setColour(TextEditor::outlineColourId, line);
        presetNotes.setTextToShowWhenEmpty("Preset comment - click to edit", muted);
        rememberDraft();
        sendLookAndFeelChange();
        layout();
        repaint();
    }
    File browserFolder(UserSettings::Folder kind) const {
        if (kind == UserSettings::Folder::Library)
            return processor.presetDirectory();
        const auto fallback = kind == UserSettings::Folder::PresetOpen ||
                                      kind == UserSettings::Folder::PresetSave ||
                                      kind == UserSettings::Folder::Library
                                  ? processor.presetDirectory()
                                  : File::getSpecialLocation(File::userDocumentsDirectory);
        return UserSettings::folder(kind, fallback);
    }
    bool setBrowserFolder(UserSettings::Folder kind, const File &directory) {
        if (!directory.isDirectory() || !UserSettings::remember(kind, directory))
            return false;
        if (kind == UserSettings::Folder::Library) {
            processor.setPresetDirectory(directory);
            refreshPresets();
        }
        readout = "Browser folder: " + directory.getFullPathName();
        if (settingsOpen && settingsTab == 6)
            layoutSettings();
        repaint();
        return true;
    }
    void chooseBrowserFolder(UserSettings::Folder kind) {
        folderChooser =
            std::make_unique<FileChooser>("Choose browser folder", browserFolder(kind), "");
        SafePointer<Console> safe(this);
        folderChooser->launchAsync(FileBrowserComponent::openMode |
                                       FileBrowserComponent::canSelectDirectories,
                                   [safe, kind](const FileChooser &chooser) {
                                       if (safe && chooser.getResult() != File{} &&
                                           !safe->setBrowserFolder(kind, chooser.getResult())) {
                                           safe->readout = "The browser folder could not be saved.";
                                           safe->repaint();
                                       }
                                   });
    }
    Result importBackground(const File &file) {
        String encoded;
        Image image;
        auto result = SkinBackground::import(file, encoded, image);
        if (result.failed())
            return result;
        skinImage = image;
        skinImageData = encoded;
        draftTheme.backgroundImage = encoded;
        draftTheme.backgroundStyle =
            draftTheme.backgroundStyle == 2 || draftTheme.backgroundStyle == 4 ? 4 : 3;
        refreshStyleFields();
        applyThemePalette(draftTheme);
        readout = "Background loaded: " + file.getFileName() + ". Save the theme to keep it.";
        repaint();
        return Result::ok();
    }
    std::function<void(double)> onScaleRequested;
    std::function<void(bool)> onModulationRequested;
    bool isModulationCollapsed() const {
        return modulationCollapsed;
    }
    void setModulationCollapsed(bool collapsed) {
        modulationCollapsed = collapsed;
        modulationToggle.setButtonText(collapsed ? "MODULATION +" : "MODULATION -");
        modulationToggle.setToggleState(!collapsed, dontSendNotification);
        layout();
        repaint();
    }
    void displayScale(double percent) {
        scaleChoice.setText(String(percent, 2).trimCharactersAtEnd("0").trimCharactersAtEnd(".") +
                                "%",
                            dontSendNotification);
    }
    bool isDarkTheme() const {
        return darkTheme;
    }
    void showSettings(bool enabled) {
        settingsOpen = enabled;
        layout();
        repaint();
    }
    int settingsPage() const {
        return settingsTab;
    }
    void showSettingsPage(int page) {
        settingsTab = jlimit(0, 6, page);
        layoutSettings();
    }
    bool showingSettings() const {
        return settingsOpen;
    }
    void setPart(int part) {
        selectedPart = jlimit(0, partCount - 1, part);
        layout();
        repaint();
    }
    void setRack(int page) {
        settingsOpen = false;
        rack = jlimit(0, flangerPage, page);
        layout();
        repaint();
    }
    void showScenario(int index) {
        if (index != 28)
            showPresetSearch(false);
        if (index >= 35 && index <= 37) {
            settingsOpen = false;
            previewSource = previewReverb = previewEq = -1;
            rack = index == 35 ? 7 : index == 36 ? 11 : 12;
            layout();
            repaint();
            return;
        }
        if (index == 34) {
            settingsOpen = false;
            rack = 5;
            previewReverb = 5;
            previewEq = previewSource = -1;
            selectedPart = 0;
            layout();
            repaint();
            return;
        }
        if (index == 31 || index == 32 || index == 33) {
            previewPattern = previewGran = previewSource = previewWave = previewStretch =
                previewReverb = -1;
            settingsOpen = index == 33;
            settingsTab = 3;
            rack = 4;
            selectedPart = index == 32 ? 2 : 0;
            previewEq = index == 31 ? 1 : -1;
            selectedEqBand = 5;
            eqBandSelect.setSelectedId(6, dontSendNotification);
            for (auto &view : sourceViews)
                view.setSelectedId(1, dontSendNotification);
            layout();
            if (index == 33) {
                aboutInfo.moveCaretToEnd();
                for (auto *child : aboutInfo.getChildren())
                    if (auto *viewport = dynamic_cast<Viewport *>(child))
                        viewport->setViewPosition(0, jmax(0, viewport->getViewPositionY() - 26));
            }
            repaint();
            return;
        }
        if (index == 29 || index == 30) {
            settingsOpen = false;
            previewPattern = previewGran = previewWave = previewEq = previewReverb = -1;
            previewSource = index == 29 ? 2 : 1;
            previewStretch = -1;
            selectedPart = index == 30 ? 2 : 0;
            rack = 9;
            waveSlot.setSelectedId(index == 30 ? 5 : 1, dontSendNotification);
            waveView.setSlot(index == 30 ? 4 : 0);
            layout();
            for (auto &view : sourceViews)
                view.setSelectedId(index == 29 ? 11 : 2, dontSendNotification);
            layout();
            repaint();
            return;
        }
        if (index == 28) {
            showPresetSearch(true);
            searchQuery.setText("pad", false);
            filterPresets();
            return;
        }
        if (index == 26 || index == 27) {
            selectedPart = 0;
            rack = 9;
            previewSource = 2;
            previewStretch = index == 26 ? 7 : 4;
            layout();
            for (auto &view : sourceViews)
                view.setSelectedId(10, dontSendNotification);
            layout();
            repaint();
            return;
        }
        previewReverb = previewEq = previewWave = previewStretch = -1;
        if (index == 23 || index == 24 || index == 25) {
            settingsOpen = false;
            previewSource = index >= 24 ? 2 : -1;
            previewStretch = index == 25 ? 6 : -1;
            rack = index == 23 ? 10 : 9;
            selectedPart = 0;
            if (index >= 24) {
                layout();
                for (auto &v : sourceViews)
                    v.setSelectedId(index == 25 ? 8 : 9, dontSendNotification);
            }
            layout();
            repaint();
            return;
        }
        if (index >= 19 && index <= 22) {
            settingsOpen = false;
            selectedPart = 0;
            if (index == 19 || index == 20) {
                rack = 9;
                previewSource = index == 19 ? 2 : 0;
                previewWave = index == 20 ? 2 : -1;
                layout();
                for (auto &v : sourceViews)
                    v.setSelectedId(index == 19 ? 8 : 7, dontSendNotification);
            } else {
                rack = 4;
                previewEq = index - 21;
            }
            layout();
            repaint();
            return;
        }

        if (index >= 14 && index < 19) {
            settingsOpen = false;
            rack = 5;
            previewReverb = index - 14;
            layout();
            repaint();
            return;
        }

        if (index >= 8) {
            selectedPart = 0;
            rack = index == 8 || index == 9 ? 9 : 0;
            previewPattern = 1;
            previewGran = 1;
            previewSource = index == 9 ? 1 : index == 8 ? 2 : 3;
            for (auto &v : sourceViews)
                v.setSelectedId(index == 10   ? 4
                                : index == 11 ? 5
                                : index == 12 ? 6
                                              : 2,
                                dontSendNotification);
            layout();
            repaint();
            return;
        }
        if (index >= 4) {
            selectedPart = 0;
            rack = index == 6 ? 8 : 0;
            stepPage = index == 5 ? 1 : 0;
            previewPattern = 1;
            previewSource = index == 4 ? 1 : 3;
            for (auto &v : sourceViews)
                v.setSelectedId(2, dontSendNotification);
            layout();
            repaint();
            return;
        }
        previewPattern = previewSource = -1;
        for (auto &v : sourceViews)
            v.setSelectedId(1, dontSendNotification);
        selectedPart = index == 1 ? 1 : 0;
        rack = index == 1 ? 6 : index == 2 ? 2 : 0;
        stepPage = index == 3 ? 1 : 0;
        layout();
        repaint();
    }
    static Rectangle<int> sampleFieldBounds(int column) {
        return {663 + 94 * column, 297, 83, 18};
    }
    void showSampleFields(int slot, int mode) {
        // Keep attachments alive across repaints. A slot or engine change can
        // change the meaning of START and POSITION without sharing their bindings.
        const std::array<Param, 3> ids{
            logisth_ui::WaveView::startParameter(slot, mode),
            mode == 2 ? samplerSeekParam(slot) : Param(playerParamIndex(Param::o1position, slot, 16)),
            Param(playerParamIndex(Param::o1end, slot, 16))};
        constexpr std::array<const char *, 3> names{"start", "position", "end"};
        for (size_t i = 0; i < ids.size(); ++i) {
            auto &field = sampleFields[i];
            if (!field || field->parameter() != ids[i]) {
                field = std::make_unique<Control>(*this, ids[i]);
                field->setComponentID("sample." + String(names[i]));
                field->setName("Sample " + String(names[i]));
                addChildComponent(*field);
            }
            field->configure(sampleFieldBounds(int(i)), Style::Number, {}, true);
        }
        sampleFields[1]->showPlaybackPosition(waveView.playbackPosition());
    }
    bool rackHeadingVisible() const {
        return !settingsOpen && rack != 1 && rack != 9 && rack != 10 && rack != 4 && rack != 7 &&
               rack != 11 && rack != 12 && !(rack == 5 && int(value(Param::reverbtype)) == 5) &&
               !(rack == 0 && value(Param::arppattern) > 0);
    }
    String validate() {
        const bool oldSettings = settingsOpen, oldCollapsed = modulationCollapsed;
        const auto oldBounds = getBounds();
        modulationCollapsed = false;
        setSize(canvasWidth, canvasHeight);
        settingsOpen = false;
        const int oldPart = selectedPart, oldRack = rack, oldStep = stepPage,
                  oldMacro = selectedMacro, oldEqBand = selectedEqBand,
                  oldDistortion = selectedDistortion;
        const int oldSampleSlot = waveView.selectedSlot();
        std::array<int, playerCount> views, algorithms;
        for (int i = 0; i < playerCount; ++i) {
            views[size_t(i)] = sourceViews[size_t(i)].getSelectedId();
            algorithms[size_t(i)] = stretchAlgorithms[size_t(i)].getSelectedId();
        }
        std::set<size_t> visited;
        String error;
        auto inspect = [&] {
            layout();
            for (const auto &field : sampleFields)
                if (field && field->isVisible()) {
                    visited.insert(size_t(field->parameter()));
                    auto *hit = getComponentAt(field->getBounds().getCentre());
                    if (!getLocalBounds().contains(field->getBounds()) ||
                        (hit != field.get() && !field->isParentOf(hit)))
                        error = "Obscured Sample field: " + field->getComponentID();
                }
            for (size_t i = 0; i < controls.size(); ++i)
                if (controls[i]->isVisible()) {
                    visited.insert(i);
                    if (i == size_t(Param::polyphony))
                        visited.insert(size_t(Param::polyphonyextra));
                    if (pairedChoiceExtension(Param(i)) != Param::Count)
                        visited.insert(size_t(pairedChoiceExtension(Param(i))));
                    if (choiceExtension(Param(i)) != Param::Count)
                        visited.insert(size_t(choiceExtension(Param(i))));
                    for (int slot = 0; slot < playerCount; ++slot)
                        if (i == playerParamIndex(Param::o1detune, slot, 12)) {
                            visited.insert(size_t(detuneExtra(slot)));
                            visited.insert(size_t(detuneWide(slot)));
                        }
                    auto *hit = getComponentAt(controls[i]->getBounds().getCentre());
                    if (hit != controls[i].get() && !controls[i]->isParentOf(hit) &&
                        hit != &patternCanvas)
                        error = "Obscured: " + String(parameterSpecs[i].id);
                    if (!getLocalBounds().contains(controls[i]->getBounds()) ||
                        controls[i]->getWidth() < 20 || controls[i]->getHeight() < 18)
                        error = "Invalid bounds: " + String(parameterSpecs[i].id);
                    for (size_t j = 0; j < i; ++j)
                        if (controls[j]->isVisible() &&
                            controls[i]->getBounds().intersects(controls[j]->getBounds()))
                            error = "Overlap: " + String(parameterSpecs[i].id) + " / " +
                                    parameterSpecs[j].id;
                }
            if (rackHeadingVisible())
                for (auto *component : getChildren())
                    if (component->isVisible() &&
                        component->getBounds().intersects(Rectangle<int>(414, 289, 350, 20)))
                        error = "Overlapped rack heading: " + component->getComponentID();
            for (auto *component : getChildren())
                if (component->isVisible() && component != &keyboard &&
                    dynamic_cast<Control *>(component) == nullptr) {
                    if (!getLocalBounds().contains(component->getBounds()))
                        error = "Widget outside console: " + component->getName();
                }
        };
        for (auto &v : sourceViews)
            v.setSelectedId(1, dontSendNotification);
        rack = 5;
        for (int mode = 0; mode < 6; ++mode) {
            previewReverb = mode;
            inspect();
        }
        previewReverb = -1;
        previewPattern = 0;
        previewSource = 0;
        for (int clock = 0; clock < 2; ++clock) {
            previewSync = clock;
            for (int part = 0; part < partCount; ++part)
                for (int page = 0; page < rackNames.size(); ++page)
                    for (int step = 0; step < 2; ++step) {
                        selectedPart = part;
                        rack = page;
                        stepPage = step;
                        inspect();
                    }
        }
        rack = 0;
        previewPattern = 1;
        for (int step = 0; step < 2; ++step) {
            stepPage = step;
            inspect();
        }
        previewPattern = 0;
        for (int part = 0; part < partCount; ++part)
            for (int mode = 0; mode < 4; ++mode)
                for (int model = 0; model < 2; ++model)
                    for (int view = 1; view <= 11; ++view) {
                        selectedPart = part;
                        previewSource = mode;
                        previewGran = model;
                        // Rebuild the mode's view menu before selecting the requested page.
                        layout();
                        for (auto &v : sourceViews)
                            if (v.indexOfItemId(view) >= 0)
                                v.setSelectedId(view, dontSendNotification);
                        inspect();
                    }
        previewSource = 0;
        previewWave = 2;
        for (auto &view : sourceViews)
            view.setSelectedId(7, dontSendNotification);
        for (int part = 0; part < partCount; ++part) {
            selectedPart = part;
            inspect();
        }
        previewWave = -1;
        previewSource = 2;
        previewStretch = 6;
        for (int part = 0; part < partCount; ++part) {
            selectedPart = part;
            layout();
            for (auto &view : sourceViews)
                if (view.indexOfItemId(8) >= 0)
                    view.setSelectedId(8, dontSendNotification);
            inspect();
            for (auto &view : sourceViews)
                view.setSelectedId(10, dontSendNotification);
            layout();
            for (int algorithm = 1; algorithm <= phaseDiffusionMode; ++algorithm) {
                for (int slot = part * 2; slot < part * 2 + 2; ++slot)
                    stretchAlgorithms[size_t(slot)].setSelectedId(algorithm, dontSendNotification);
                inspect();
            }
        }
        previewStretch = -1;
        rack = 4;
        for (int mode = 0; mode < 2; ++mode) {
            previewEq = mode;
            inspect();
        }
        previewEq = 1;
        for (int band = 0; band < eqMaxBands; ++band) {
            selectedEqBand = band;
            inspect();
        }
        previewEq = -1;
        rack = 1;
        for (int slot = 0; slot < 4; ++slot) {
            selectedDistortion = slot;
            inspect();
        }
        previewDistortion = 1;
        for (int count = 1; count <= 4; ++count) {
            previewDistortionBands = count;
            for (int slot = 0; slot < count; ++slot) {
                selectedDistortion = int(processor.distortionOrder().slots[size_t(slot)]);
                inspect();
            }
        }
        previewDistortion = -1;
        previewDistortionBands = -1;
        selectedDistortion = oldDistortion;
        rack = 10;
        for (int macro = 0; macro < 4; ++macro) {
            selectedMacro = macro;
            inspect();
        }
        rack = 9;
        for (int engine : {2, 3}) {
            previewSource = engine;
            for (int slot = 0; slot < playerCount; ++slot) {
                waveView.setSlot(slot);
                inspect();
            }
        }
        waveView.setSlot(oldSampleSlot);
        visited.insert(size_t(Param::fixedvelocity)); // Settings uses a dedicated attached button.
        visited.insert(size_t(Param::bendwheel)); // Performance uses the attached wheels.
        visited.insert(size_t(Param::modwheel));
        for (size_t i = 0; i < controls.size(); ++i)
            if (!visited.count(i))
                error = "Unreachable: " + String(parameterSpecs[i].id);
        selectedMacro = oldMacro;
        selectedEqBand = oldEqBand;
        eqBandSelect.setSelectedId(oldEqBand + 1, dontSendNotification);
        settingsOpen = oldSettings;
        selectedPart = oldPart;
        rack = oldRack;
        stepPage = oldStep;
        previewSync = previewSource = previewPattern = previewGran = -1;
        for (int i = 0; i < playerCount; ++i) {
            sourceViews[size_t(i)].setSelectedId(views[size_t(i)], dontSendNotification);
            stretchAlgorithms[size_t(i)].setSelectedId(algorithms[size_t(i)], dontSendNotification);
        }
        modulationCollapsed = oldCollapsed;
        setBounds(oldBounds);
        layout();
        return error;
    }

  private:
    void initialiseMorph() {
        for (int i = 0; i < 4; ++i) {
            auto &name = macroNames[size_t(i)];
            name.setComponentID("morph.name." + String(i));
            name.setFont(font(10));
            name.setInputRestrictions(24);
            name.setText(processor.macroName(i), false);
            name.onTextChange = [this, i] {
                processor.setMacroName(i, macroNames[size_t(i)].getText());
            };
            addChildComponent(name);
            auto &select = macroSelect[size_t(i)];
            select.setButtonText("ROUTES");
            select.setComponentID("morph.select." + String(i));
            select.onClick = [this, i] {
                selectedMacro = i;
                layout();
                repaint();
            };
            addChildComponent(select);
            auto &lock = mutationLock[size_t(i)];
            lock.setComponentID("morph.lock." + String(i));
            lock.setButtonText(StringArray{"OSC", "FILTER", "MOD", "FX"}[i]);
            lock.setClickingTogglesState(true);
            lock.setTooltip("Lock this section against Mutate");
            lock.onClick = [this, i] {
                int bits = processor.mutationLocks();
                int bit = 1 << i;
                processor.setMutationLocks(mutationLock[size_t(i)].getToggleState() ? bits | bit
                                                                                    : bits & ~bit);
            };
            addChildComponent(lock);
        }
        mutateButton.setButtonText("MUTATE");
        mutateButton.setComponentID("morph.mutate");
        mutateButton.onClick = [this] {
            processor.mutate();
            undoButton.setEnabled(true);
        };
        addChildComponent(mutateButton);
        undoButton.setButtonText("UNDO");
        undoButton.setComponentID("morph.undo");
        undoButton.onClick = [this] {
            processor.undoMutation();
            undoButton.setEnabled(false);
        };
        addChildComponent(undoButton);
    }
    void layoutMorph() {
        for (int i = 0; i < 4; ++i) {
            auto &name = macroNames[size_t(i)];
            name.setBounds(414 + i * 102, 289, 96, 22);
            name.setVisible(true);
            size_t base = size_t(Param::macro1) + size_t(i) * 13;
            put(Param(base), {414 + i * 102, 314, 96, 60}, Style::Knob, {}, true);
            auto &select = macroSelect[size_t(i)];
            select.setBounds(414 + i * 102, 377, 96, 20);
            select.setToggleState(selectedMacro == i, dontSendNotification);
            select.setVisible(true);
            auto &lock = mutationLock[size_t(i)];
            lock.setBounds(414 + i * 73, 508, 69, 20);
            lock.setToggleState((processor.mutationLocks() & (1 << i)) != 0, dontSendNotification);
            lock.setVisible(true);
        }
        put(Param::mutateamount, {827, 289, 106, 38}, Style::Number, "MUTATE DEPTH", true);
        mutateButton.setBounds(827, 341, 106, 26);
        mutateButton.setVisible(true);
        undoButton.setBounds(827, 374, 106, 23);
        undoButton.setVisible(true);
        undoButton.setEnabled(processor.canUndoMutation());
        size_t base = size_t(Param::macro1) + size_t(selectedMacro) * 13;
        for (int row = 0; row < 4; ++row) {
            int y = 410 + row * 23;
            put(Param(base + 1 + size_t(row) * 3), {414, y, 258, 21}, Style::Menu, {}, true);
            put(Param(base + 2 + size_t(row) * 3), {682, y, 115, 21}, Style::Number, {}, true);
            put(Param(base + 3 + size_t(row) * 3), {807, y, 126, 21}, Style::Number, {}, true);
        }
    }

    void initialiseSettings() {
        const StringArray tabs{"DISPLAY", "COLOURS", "SKIN", "ABOUT", "TEXT", "MIDI", "FILES"};
        for (int i = 0; i < 7; ++i) {
            auto &b = settingsTabs[size_t(i)];
            b.setButtonText(tabs[i]);
            b.getProperties().set("fontSize", 9);
            b.setComponentID("settings.tab." + String(i));
            b.onClick = [this, i] {
                settingsTab = i;
                layoutSettings();
            };
            settingsPanel.addAndMakeVisible(b);
        }
        for (Label *l : {&themeLabel, &themeHelp, &aboutLabel, &scaleLabel, &nameLabel, &roleLabel,
                         &hexLabel}) {
            l->setFont(font(10));
            settingsPanel.addAndMakeVisible(*l);
        }
        textSize.setComponentID("settings.text.size");
        textSize.addItemList({"80%", "90%", "100%", "110%", "115%", "125%"}, 1);
        textSize.setEditableText(true);
        textSize.setTooltip("Font size 80-125%, independent of interface/window scaling. Enter a "
                            "value or choose a preset.");
        textWeight.setComponentID("settings.text.style");
        textWeight.addItemList({"Theme default", "Regular", "Bold", "Italic", "Bold italic"}, 1);
        textAA.setComponentID("settings.text.antialias");
        textAA.setButtonText("ANTIALIASING");
        textAA.setTooltip(
            "Enable glyph smoothing. Off uses a one-bit mask; vector graphics stay smooth.");
        textQuality.setComponentID("settings.text.quality");
        textQuality.addItemList({"Native", "Grayscale", "Supersampled 2x", "Supersampled 4x"}, 1);
        textQuality.getProperties().set("dark", true);
        textQuality.setTooltip("2x is the default. Grayscale/supersampling rasterise glyphs at the "
                               "current display scale. Native retains platform hinting.");
        textEdge.setComponentID("settings.text.edge");
        textEdge.getProperties().set("dark", true);
        textEdge.setSliderStyle(Slider::LinearHorizontal);
        textEdge.setRange(50, 200, 1);
        textEdge.setTextBoxStyle(Slider::TextBoxRight, false, 60, 24);
        textEdge.setTextValueSuffix("%");
        textEdge.setDoubleClickReturnValue(true, 100);
        textEdge.setTooltip(
            "Glyph edge weight: 50-200%, neutral 100%. Higher values strengthen coverage without "
            "changing font size. Native uses grayscale when adjusted.");
        textEdgeLabel.setText("EDGE WEIGHT", dontSendNotification);
        textEdgeLabel.setFont(font(10));
        textQuality.onChange = [this] {
            if (textQuality.getSelectedId() > 0) {
                auto prefs = UserSettings::text();
                prefs.quality = textQuality.getSelectedId() - 1;
                changeTypography(prefs);
            }
        };
        textEdge.onValueChange = [this] {
            auto prefs = UserSettings::text();
            prefs.edgeWeight = textEdge.getValue() / 100;
            changeTypography(prefs);
        };
        for (ComboBox *box : {&textSize, &textWeight, &textQuality})
            box->getProperties().set("dark", true);
        textSizeLabel.setText("FONT SIZE / %", dontSendNotification);
        textWeightLabel.setText("FONT STYLE", dontSendNotification);
        for (Label *label : {&textSizeLabel, &textWeightLabel, &textReference, &settingsHelp})
            label->setFont(font(10));
        textSize.onChange = [this] {
            const auto text =
                textSize.getText().trim().trimCharactersAtEnd("%").trim().replace(",", ".");
            const auto value = text.getDoubleValue();
            if (text.isEmpty() || !text.containsOnly("0123456789.") ||
                text.retainCharacters(".").length() > 1 || !std::isfinite(value) || value < 80 ||
                value > 125) {
                readout = "Enter a font size from 80 to 125%.";
                refreshTypography();
                repaint();
                return;
            }
            auto prefs = UserSettings::text();
            prefs.size = std::round(value * 100) / 100;
            changeTypography(prefs);
        };
        textWeight.onChange = [this] {
            if (textWeight.getSelectedId() > 0) {
                auto prefs = UserSettings::text();
                prefs.style = textWeight.getSelectedId() - 1;
                changeTypography(prefs);
            }
        };
        textAA.onClick = [this] {
            auto prefs = UserSettings::text();
            prefs.antialias = textAA.getToggleState();
            changeTypography(prefs);
        };
        for (Component *c : std::array<Component *, 10>{
                 &textSize, &textWeight, &textAA, &textSizeLabel, &textWeightLabel, &textReference,
                 &settingsHelp, &textQuality, &textEdge, &textEdgeLabel})
            settingsPanel.addChildComponent(*c);
        for (int i = 0; i < 5; ++i) {
            auto &label = folderLabels[size_t(i)], &path = folderValues[size_t(i)];
            label.setText(
                StringArray{"PRESET LOAD", "PRESET SAVE", "SAMPLES", "IMPULSES", "LIBRARY"}[i],
                dontSendNotification);
            label.setFont(font(9));
            path.setFont(font(9));
            path.setComponentID("settings.folder." + String(i));
            settingsPanel.addChildComponent(label);
            settingsPanel.addChildComponent(path);
            auto &choose = folderChoose[size_t(i)], &open = folderOpen[size_t(i)];
            choose.setComponentID("settings.folder.choose." + String(i));
            open.setComponentID("settings.folder.open." + String(i));
            choose.setButtonText("CHOOSE...");
            open.setButtonText("OPEN");
            choose.setTooltip("Choose the " + label.getText().toLowerCase() + " folder.");
            open.setTooltip("Open this folder in the system file browser.");
            choose.getProperties().set("fontSize", 9);
            open.getProperties().set("fontSize", 9);
            settingsPanel.addChildComponent(choose);
            settingsPanel.addChildComponent(open);
            choose.onClick = [this, i] { chooseBrowserFolder(UserSettings::Folder(i)); };
            open.onClick = [this, i] { browserFolder(UserSettings::Folder(i)).startAsProcess(); };
        }
        refreshTypography();
        fixedVelocity.setComponentID("settings.fixedvelocity");
        fixedVelocity.setButtonText("FIX MIDI VELOCITY TO MAXIMUM");
        fixedVelocity.setTooltip("Ignore note velocity and use 127 for synthesis and generated "
                                 "MIDI. Off retains normal dynamics.");
        settingsPanel.addChildComponent(fixedVelocity);
        fixedVelocityAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
            processor.state, "fixedvelocity", fixedVelocity);
        themeLabel.setText("COLOUR THEME", dontSendNotification);
        scaleLabel.setText("UI SCALE / %", dontSendNotification);
        nameLabel.setText("NAME", dontSendNotification);
        roleLabel.setText("COLOUR", dontSendNotification);
        hexLabel.setText("HEX / RGB", dontSendNotification);
        themeChoice.setComponentID("settings.theme");
        themeChoice.setName("Colour theme");
        themeChoice.getProperties().set("dark", true);
        themeChoice.onChange = [this] {
            if (themeChoice.getSelectedId() > 0)
                setTheme(themeChoice.getSelectedId() - 1);
        };
        settingsPanel.addAndMakeVisible(themeChoice);
        scaleChoice.setComponentID("settings.scale");
        scaleChoice.setName("Interface scale percent");
        scaleChoice.addItemList({"80%", "90%", "100%", "110%", "125%", "150%", "175%", "200%"}, 1);
        scaleChoice.setEditableText(true);
        scaleChoice.getProperties().set("dark", true);
        scaleChoice.setTooltip(
            "Enter 80-200%, including decimal values. The last size is restored on reopen.");
        scaleChoice.onChange = [this] {
            auto input =
                scaleChoice.getText().trim().trimCharactersAtEnd("%").trim().replace(",", ".");
            if (input.isNotEmpty() && input.containsOnly("0123456789.") &&
                input.retainCharacters(".").length() <= 1 && onScaleRequested)
                onScaleRequested(input.getDoubleValue());
            else {
                readout = "Enter a scale between 80 and 200%.";
                if (onScaleRequested)
                    onScaleRequested(std::numeric_limits<double>::quiet_NaN());
                repaint();
            }
        };
        settingsPanel.addAndMakeVisible(scaleChoice);
        auto button = [&](TextButton &b, const char *id, const char *text) {
            b.setComponentID(id);
            b.setButtonText(text);
            settingsPanel.addAndMakeVisible(b);
        };
        button(themeEdit, "settings.theme.edit", "EDIT / NEW");
        button(themeScan, "settings.theme.scan", "RESCAN");
        button(themeFolder, "settings.theme.folder", "OPEN FOLDER");
        button(themeSave, "settings.theme.save", "SAVE NEW");
        button(themeSaveAs, "settings.theme.saveas", "SAVE AS...");
        button(themeRename, "settings.theme.rename", "RENAME");
        button(themeChooseFolder, "settings.theme.choosefolder", "USE FOLDER");
        themeChooseFolder.onClick = [this] {
            themeChooser = std::make_unique<FileChooser>("Choose the theme folder",
                                                         ThemeSettings::folder(), "", true);
            SafePointer<Console> safe(this);
            themeChooser->launchAsync(
                FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
                [safe](const FileChooser &c) {
                    if (!safe || c.getResult() == File{})
                        return;
                    auto r = ThemeSettings::setFolder(c.getResult(), safe.getComponent());
                    if (r.wasOk())
                        safe->setTheme(ThemeSettings::current(), false);
                    else
                        safe->readout = r.getErrorMessage();
                });
        };
        themeSaveAs.onClick = [this] {
            rememberDraft();
            auto initial = ThemeSettings::folder().getNonexistentChildFile(
                "custom-" + File::createLegalFileName(themeName.getText()), ".json", false);
            themeChooser = std::make_unique<FileChooser>("Save theme as", initial, "*.json", true);
            SafePointer<Console> safe(this);
            themeChooser->launchAsync(
                FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles |
                    FileBrowserComponent::warnAboutOverwriting,
                [safe, initial](const FileChooser &c) {
                    if (!safe || c.getResult() == File{})
                        return;
                    auto file = c.getResult().withFileExtension("json");
                    auto draft = safe->draftTheme;
                    if (file.getFileName() != initial.getFileName())
                        draft.name = file.getFileNameWithoutExtension();
                    auto r = ThemeSettings::saveTo(draft, file, safe.getComponent());
                    if (r.wasOk())
                        safe->setTheme(ThemeSettings::current(), false);
                    else
                        safe->readout = r.getErrorMessage();
                    safe->repaint();
                });
        };
        themeRename.onClick = [this] {
            rememberDraft();
            auto r = ThemeSettings::renameCurrent(themeName.getText(), this, &draftTheme);
            if (r.wasOk())
                setTheme(ThemeSettings::current(), false);
            else
                readout = r.getErrorMessage();
            repaint();
        };
        button(themeReset, "settings.theme.reset", "RESET EDITS");
        themeEdit.onClick = [this] {
            if (draftTheme.factory)
                themeName.setText("My " + draftTheme.name, false);
            settingsTab = 1;
            layoutSettings();
        };
        themeScan.onClick = [this] {
            ThemeSettings::scan(true);
            refreshThemeChoices();
            readout = "Theme folder scanned / " + String(ThemeSettings::names().size()) + " themes";
            repaint();
        };
        themeFolder.onClick = [] { ThemeSettings::folder().revealToUser(); };
        themeSave.onClick = [this] {
            draftTheme.name = themeName.getText();
            auto result = ThemeSettings::saveAs(draftTheme, this);
            if (result.wasOk()) {
                setTheme(ThemeSettings::current(), false);
                readout = "Theme saved to " + ThemeSettings::folder().getFullPathName();
            } else
                readout = result.getErrorMessage();
            repaint();
        };
        themeReset.onClick = [this] { setTheme(themeId, false); };
        themeName.setComponentID("settings.theme.name");
        themeName.setFont(font(11));
        themeName.setInputRestrictions(64);
        themeName.onTextChange = [this] { rememberDraft(); };
        settingsPanel.addAndMakeVisible(themeName);
        themeDark.setButtonText("DARK CONTROLS");
        themeDark.setComponentID("settings.theme.dark");
        themeDark.onClick = [this] {
            draftTheme.dark = themeDark.getToggleState();
            applyThemePalette(draftTheme);
        };
        settingsPanel.addAndMakeVisible(themeDark);
        colourRole.setComponentID("settings.theme.role");
        colourRole.addItemList(ThemeSettings::colourNames, 1);
        colourRole.setSelectedId(6, dontSendNotification);
        colourRole.getProperties().set("dark", true);
        colourRole.onChange = [this] { updateColourEditor(); };
        settingsPanel.addAndMakeVisible(colourRole);
        colourHex.setComponentID("settings.theme.hex");
        colourHex.setFont(font(11));
        colourHex.setInputRestrictions(7, "#0123456789abcdefABCDEF");
        colourHex.onReturnKey = [this] {
            Colour c;
            if (ThemeSettings::colour(colourHex.getText(), c)) {
                draftTheme.colours[size_t(jlimit(0, ThemeSettings::colourNames.size() - 1,
                                                 colourRole.getSelectedId() - 1))] = c;
                colourPicker.setCurrentColour(c, dontSendNotification);
                applyThemePalette(draftTheme);
            } else {
                readout = "Use a six-digit RGB colour, for example #62ACFF.";
                updateColourEditor();
                repaint();
            }
        };
        colourHex.onFocusLost = colourHex.onReturnKey;
        settingsPanel.addAndMakeVisible(colourHex);
        colourPicker.setComponentID("settings.theme.picker");
        colourPicker.addChangeListener(this);
        settingsPanel.addAndMakeVisible(colourPicker);
        for (ComboBox *c : {&themeFont, &themeBackground, &themeKnobs, &backgroundFactory}) {
            c->getProperties().set("dark", true);
            settingsPanel.addAndMakeVisible(*c);
        }
        themeFont.setComponentID("settings.theme.font");
        auto families = Font::findAllTypefaceNames();
        families.removeString("IoskeleyMonoTerm Nerd Font Mono");
        families.insert(0, "IoskeleyMonoTerm Nerd Font Mono");
        families.removeString("Open Sans");
        families.insert(0, "Open Sans");
        themeFont.addItemList(families, 1);
        themeBackground.setComponentID("settings.theme.background");
        themeBackground.addItemList({"Flat", "Gradient", "Grid", "Image", "Image + grid"}, 1);
        backgroundFactory.setComponentID("settings.background.factory");
        backgroundFactory.setTextWhenNothingSelected("Choose a factory background...");
        for (size_t i = 0; i < BackgroundLibrary::entries.size(); ++i) {
            const auto &entry = BackgroundLibrary::entries[i];
            if (i % 2 == 0) {
                const auto themes = ThemeSettings::defaults();
                for (const auto &theme : themes)
                    if (theme.key == entry.theme)
                        backgroundFactory.addSectionHeading(theme.name);
            }
            backgroundFactory.addItem(entry.name, int(i) + 1);
        }
        backgroundFactory.onChange = [this] {
            const int index = backgroundFactory.getSelectedId() - 1;
            auto data = BackgroundLibrary::data(index);
            if (data.isEmpty())
                return;
            draftTheme.backgroundImage = Base64::toBase64(data.getData(), data.getSize());
            draftTheme.backgroundStyle =
                draftTheme.backgroundStyle == 2 || draftTheme.backgroundStyle == 4 ? 4 : 3;
            draftTheme.backgroundMix = 1;
            refreshStyleFields();
            applyThemePalette(draftTheme);
            readout = String(BackgroundLibrary::entries[size_t(index)].name) +
                      " background. Use COLOURS > SAVE AS to keep it.";
            backgroundFactory.setSelectedId(0, dontSendNotification);
        };
        backgroundLoad.setButtonText("LOAD IMAGE...");
        backgroundClear.setButtonText("CLEAR");
        backgroundLoad.setComponentID("settings.background.load");
        backgroundClear.setComponentID("settings.background.clear");
        backgroundName.setComponentID("settings.background.name");
        backgroundLoad.getProperties().set("fontSize", 9);
        backgroundClear.getProperties().set("fontSize", 9);
        backgroundName.setFont(font(9));
        backgroundLoad.onClick = [this] {
            backgroundChooser = std::make_unique<FileChooser>("Choose a PNG/JPEG background",
                                                              File{}, "*.png;*.jpg;*.jpeg");
            SafePointer<Console> safe(this);
            backgroundChooser->launchAsync(
                FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                [safe](const FileChooser &chooser) {
                    if (safe && chooser.getResult() != File{}) {
                        auto result = safe->importBackground(chooser.getResult());
                        if (result.failed()) {
                            safe->readout = result.getErrorMessage();
                            safe->repaint();
                        }
                    }
                });
        };
        backgroundClear.onClick = [this] {
            draftTheme.backgroundImage.clear();
            if (draftTheme.backgroundStyle >= 3)
                draftTheme.backgroundStyle = draftTheme.backgroundStyle == 4 ? 2 : 0;
            refreshStyleFields();
            applyThemePalette(draftTheme);
        };
        backgroundMixLabel.setText("IMAGE MIX", dontSendNotification);
        panelOpacityLabel.setText("PANELS", dontSendNotification);
        for (auto *label : {&backgroundMixLabel, &panelOpacityLabel, &gradientLabel, &gridLabel})
            label->setFont(font(9));
        for (auto *slider :
             {&backgroundMixControl, &panelOpacityControl, &gradientControl, &gridControl}) {
            slider->getProperties().set("dark", true);
            slider->setSliderStyle(Slider::LinearHorizontal);
            slider->setRange(slider == &panelOpacityControl ? 40 : 0, 100, 1);
            slider->setTextBoxStyle(Slider::TextBoxRight, false, 48, 24);
            slider->setTextValueSuffix("%");
        }
        gradientLabel.setText("GRADIENT", dontSendNotification);
        gridLabel.setText("GRID", dontSendNotification);
        gradientControl.setComponentID("settings.background.gradient");
        gridControl.setComponentID("settings.background.grid");
        gridControl.setTooltip(
            "Grid opacity. PANELS controls how much background is visible below the controls.");
        gradientControl.setTooltip(
            "Strength of the theme-coloured gradient across background and panels.");
        gradientControl.onValueChange = [this] {
            draftTheme.gradientAmount = gradientControl.getValue() / 100;
            previewBackgroundAmounts();
        };
        gridControl.onValueChange = [this] {
            draftTheme.gridOpacity = gridControl.getValue() / 100;
            previewBackgroundAmounts();
        };
        backgroundMixControl.setComponentID("settings.background.mix");
        panelOpacityControl.setComponentID("settings.background.panels");
        backgroundMixControl.onValueChange = [this] {
            draftTheme.backgroundMix = backgroundMixControl.getValue() / 100;
            previewBackgroundAmounts();
        };
        panelOpacityControl.onValueChange = [this] {
            draftTheme.panelOpacity = panelOpacityControl.getValue() / 100;
            previewBackgroundAmounts();
        };
        for (auto *c : std::array<Component *, 11>{
                 &backgroundLoad, &backgroundClear, &backgroundName, &backgroundMixControl,
                 &panelOpacityControl, &backgroundMixLabel, &panelOpacityLabel, &gradientControl,
                 &gridControl, &gradientLabel, &gridLabel})
            settingsPanel.addChildComponent(*c);
        themeKnobs.setComponentID("settings.theme.knobs");
        themeKnobs.addItemList({"Arc", "Solid", "Minimal"}, 1);
        themeFont.onChange = [this] {
            if (themeFont.getSelectedId() > 0) {
                draftTheme.fontFamily = themeFont.getText();
                applyThemePalette(draftTheme);
            }
        };
        themeBackground.onChange = [this] {
            draftTheme.backgroundStyle = themeBackground.getSelectedId() - 1;
            applyThemePalette(draftTheme);
        };
        themeKnobs.onChange = [this] {
            draftTheme.knobStyle = themeKnobs.getSelectedId() - 1;
            applyThemePalette(draftTheme);
        };
        for (auto pair : {std::pair<Label *, const char *>{&fontLabel, "FONT FAMILY"},
                          {&backgroundLabel, "BACKGROUND"},
                          {&knobLabel, "KNOB STYLE"}}) {
            pair.first->setText(pair.second, dontSendNotification);
            pair.first->setFont(font(10));
            settingsPanel.addAndMakeVisible(*pair.first);
        }
        aboutLogo.setImage(apLogo);
        settingsPanel.addChildComponent(aboutLogo);
        aboutSkinName.setText(apCaption, dontSendNotification);
        aboutSkinName.setFont(font(12, true));
        aboutSkinName.setJustificationType(Justification::centred);
        settingsPanel.addChildComponent(aboutSkinName);
        brandAboutLogo.setComponentID("settings.about.logo");
        settingsPanel.addChildComponent(brandAboutLogo);
        aboutLabel.setFont(font(11, true));
        aboutLabel.setText("WAVELOGIK / LOGISTH", dontSendNotification);
        settingsPanel.addChildComponent(aboutInfo);
        aboutInfo.setComponentID("settings.about.credits");
        aboutInfo.setMultiLine(true, true);
        aboutInfo.setReadOnly(true);
        aboutInfo.setCaretVisible(false);
        aboutInfo.setScrollbarsShown(true);
        aboutInfo.setFont(font(11));
        aboutInfo.setText("wavelogik / logisth  " + String(JucePlugin_VersionString) +
                              "\nPolyphonic instrument / VST3, CLAP, Standalone"
                              "\nOriginal code: MIT / (c) 2026 wavelogik"
                              "\nBuilt with JUCE, CLAP extensions, Open Sans\n and Ioskeley Mono."
                              "\nDependencies retain their respective licenses."
                              "\n\nPaulXStretch mode: original logisth DSP using Signalsmith Linear (MIT)."
                              "\nInspired by PaulStretch by Nasca Octavian Paul and PaulXStretch "
                              "by Xenakios and Jesse Chappell.\nhttps://github.com/essej/paulxstretch"
                              "\n\nCREDITS\n\n"
                              "Logik - Prompt engineer, tester, and everything in between.\n\n"
                              "coldwave - No. 1 tester with a head full of ideas.\n\n"
                              "Moander - Tester with a head full of ideas.\n\n"
                              "bokkenmmb - Tester, idea guy, and resident theme wizard (yes, the "
                              "FoRReSkin one).\n\n"
                              "mixedbytrzy - Tester, suggested hiding the modulation menu and "
                              "contributed many other ideas.\n\n"
                              "Open-source projects: JUCE, clap-juce-extensions, Signalsmith "
                              "Stretch, Signalsmith Linear, Open Sans and Ioskeley Mono.\n\n"
                              "Theme and size are independent of audio presets.",
                          dontSendNotification);
    }
    void refreshThemeChoices() {
        auto names = ThemeSettings::names();
        bool different = themeChoice.getNumItems() != names.size();
        for (int i = 0; i < names.size() && !different; ++i)
            different = themeChoice.getItemText(i) != names[i];
        if (different) {
            themeChoice.clear(dontSendNotification);
            themeChoice.addItemList(names, 1);
        }
        themeChoice.setSelectedId(themeId + 1, dontSendNotification);
        themeHelp.setText(String(names.size()) + " themes / scanned every 2 minutes\n" +
                              ThemeSettings::folder().getFullPathName(),
                          dontSendNotification);
        themeHelp.setTooltip(ThemeSettings::folder().getFullPathName());
    }
    void previewBackgroundAmounts() {
        backgroundMix = look.backgroundMix = draftTheme.backgroundMix;
        panelOpacity = look.panelOpacity = draftTheme.panelOpacity;
        gradientAmount = look.gradientAmount = draftTheme.gradientAmount;
        gridOpacity = look.gridOpacity = draftTheme.gridOpacity;
        rememberDraft();
        repaint();
    }
    void refreshBackgroundFields() {
        backgroundName.setText(skinImage.isValid()
                                   ? "Embedded image / " + String(skinImage.getWidth()) + " x " +
                                         String(skinImage.getHeight())
                                   : "No image loaded",
                               dontSendNotification);
        backgroundClear.setEnabled(draftTheme.backgroundImage.isNotEmpty());
        backgroundMixControl.setValue(draftTheme.backgroundMix * 100, dontSendNotification);
        panelOpacityControl.setValue(draftTheme.panelOpacity * 100, dontSendNotification);
        gradientControl.setValue(draftTheme.gradientAmount * 100, dontSendNotification);
        gridControl.setValue(draftTheme.gridOpacity * 100, dontSendNotification);
        gradientControl.setEnabled(draftTheme.backgroundStyle == 1);
        gridControl.setEnabled(draftTheme.backgroundStyle == 2 || draftTheme.backgroundStyle == 4);
        backgroundMixControl.setEnabled(draftTheme.backgroundStyle >= 3 && skinImage.isValid());
        panelOpacityControl.setEnabled(draftTheme.backgroundStyle != 0);
    }
    void refreshStyleFields() {
        themeFont.setText(draftTheme.fontFamily, dontSendNotification);
        themeBackground.setSelectedId(draftTheme.backgroundStyle + 1, dontSendNotification);
        themeKnobs.setSelectedId(draftTheme.knobStyle + 1, dontSendNotification);
        refreshBackgroundFields();
    }
    void updateColourEditor() {
        auto c = draftTheme.colours[size_t(
            jlimit(0, ThemeSettings::colourNames.size() - 1, colourRole.getSelectedId() - 1))];
        colourHex.setText("#" + c.toDisplayString(false), false);
        colourPicker.setCurrentColour(c, dontSendNotification);
    }
    void changeListenerCallback(ChangeBroadcaster *source) override {
        if (source == &colourPicker) {
            draftTheme.colours[size_t(
                jlimit(0, ThemeSettings::colourNames.size() - 1, colourRole.getSelectedId() - 1))] =
                colourPicker.getCurrentColour().withAlpha(1.f);
            updateColourEditor();
            applyThemePalette(draftTheme);
        }
    }
    void refreshTypography() {
        const auto prefs = UserSettings::text();
        textSize.setText(String(prefs.size, prefs.size == std::floor(prefs.size) ? 0 : 2) + "%",
                         dontSendNotification);
        textWeight.setSelectedId(prefs.style + 1, dontSendNotification);
        textAA.setToggleState(prefs.antialias, dontSendNotification);
        textQuality.setSelectedId(prefs.quality + 1, dontSendNotification);
        textQuality.setEnabled(prefs.antialias);
        textEdge.setValue(prefs.edgeWeight * 100, dontSendNotification);
        textReference.setText("Reference " + String(font(12).getHeight(), 1) + " px",
                              dontSendNotification);
    }
    void changeTypography(UserSettings::Text prefs) {
        if (!UserSettings::setText(prefs))
            readout = "Text preference changed but could not be saved.";
        applyThemePalette(draftTheme);
        refreshTypography();
        layout();
        repaint();
    }
    void layoutSettings() {
        for (auto *c : settingsPanel.getChildren())
            c->setVisible(false);
        auto show = [](Component &c, Rectangle<int> b) {
            c.setBounds(b);
            c.setVisible(true);
        };
        static constexpr std::array<int, 7> order{0, 4, 1, 2, 5, 6, 3};
        for (int position = 0; position < 7; ++position) {
            const auto i = order[size_t(position)];
            auto &b = settingsTabs[size_t(i)];
            show(b, {position * 74, 0, 70, 24});
            b.setToggleState(settingsTab == i, dontSendNotification);
        }
        if (settingsTab == 0) {
            show(themeLabel, {0, 40, 145, 25});
            show(themeChoice, {157, 40, 348, 28});
            show(scaleLabel, {0, 82, 145, 25});
            show(scaleChoice, {157, 82, 152, 28});
            show(themeEdit, {0, 125, 120, 26});
            show(themeScan, {128, 125, 120, 26});
            show(themeFolder, {256, 125, 120, 26});
            show(themeChooseFolder, {384, 125, 126, 26});
            show(themeHelp, {0, 174, 510, 52});
        } else if (settingsTab == 1) {
            show(nameLabel, {0, 33, 208, 19});
            show(themeName, {0, 54, 208, 25});
            show(themeDark, {0, 84, 208, 23});
            show(roleLabel, {0, 110, 208, 18});
            show(colourRole, {0, 131, 208, 25});
            show(hexLabel, {0, 161, 92, 24});
            show(colourHex, {95, 161, 113, 25});
            show(themeSave, {0, 199, 104, 27});
            show(themeSaveAs, {110, 199, 120, 27});
            show(themeRename, {236, 199, 104, 27});
            show(themeReset, {346, 199, 168, 27});
            show(colourPicker, {224, 33, 290, 158});
        } else if (settingsTab == 2) {
            show(backgroundLabel, {0, 34, 94, 26});
            show(themeBackground, {100, 34, 202, 26});
            knobLabel.setText("KNOBS", dontSendNotification);
            show(knobLabel, {314, 34, 54, 26});
            show(themeKnobs, {374, 34, 136, 26});
            show(gradientLabel, {0, 66, 85, 26});
            show(gradientControl, {88, 66, 178, 26});
            show(gridLabel, {280, 66, 65, 26});
            show(gridControl, {348, 66, 162, 26});
            show(backgroundName, {0, 98, 274, 26});
            show(backgroundLoad, {282, 98, 112, 26});
            show(backgroundClear, {402, 98, 108, 26});
            show(backgroundFactory, {0, 130, 510, 26});
            show(backgroundMixLabel, {0, 162, 85, 26});
            show(backgroundMixControl, {88, 162, 178, 26});
            show(panelOpacityLabel, {280, 162, 65, 26});
            show(panelOpacityControl, {348, 162, 162, 26});
            refreshBackgroundFields();
            settingsHelp.setText("20 factory images. PNG/JPEG up to 4096 px / 16 MiB.\n"
                                 "Save artwork in your theme: COLOURS > SAVE AS.",
                                 dontSendNotification);
            show(settingsHelp, {0, 199, 510, 29});
        } else if (settingsTab == 4) {
            refreshTypography();
            show(fontLabel, {0, 32, 150, 26});
            show(themeFont, {157, 32, 353, 28});
            show(textSizeLabel, {0, 64, 150, 26});
            show(textSize, {157, 64, 124, 28});
            show(textReference, {295, 64, 215, 28});
            show(textWeightLabel, {0, 96, 150, 26});
            show(textWeight, {157, 96, 353, 28});
            show(textAA, {0, 130, 148, 26});
            show(textQuality, {157, 130, 353, 26});
            show(textEdgeLabel, {0, 166, 148, 26});
            show(textEdge, {157, 166, 353, 26});
            settingsHelp.setText("Text renders at the display scale. Edge weight 100% is neutral; "
                                 "double-click resets it. Font family belongs to the theme.",
                                 dontSendNotification);
            show(settingsHelp, {0, 201, 510, 28});
        } else if (settingsTab == 5) {
            show(fixedVelocity, {0, 48, 510, 28});
            settingsHelp.setText("Off preserves MIDI note dynamics. On uses the maximum velocity "
                                 "for synthesis and generated MIDI.",
                                 dontSendNotification);
            show(settingsHelp, {0, 98, 510, 70});
        } else if (settingsTab == 6) {
            settingsHelp.setText("BROWSER FOLDERS / choose a location or open it",
                                 dontSendNotification);
            show(settingsHelp, {0, 30, 510, 20});
            static constexpr std::array<int, 5> order{4, 0, 1, 2, 3};
            for (int row = 0; row < 5; ++row) {
                const auto i = size_t(order[size_t(row)]);
                const auto path = browserFolder(UserSettings::Folder(i));
                folderValues[i].setText(path.getFullPathName(), dontSendNotification);
                folderValues[i].setTooltip(path.getFullPathName());
                const int y = 54 + row * 34;
                show(folderLabels[i], {0, y, 110, 28});
                show(folderValues[i], {116, y, 236, 28});
                show(folderChoose[i], {360, y, 90, 28});
                show(folderOpen[i], {456, y, 54, 28});
                folderOpen[i].setEnabled(path.isDirectory());
            }
        } else {
            show(aboutLabel, {0, 40, 510, 25});
            show(aboutInfo, {0, 74, 338, 150});
            if (skin == "ap-flexin") {
                show(aboutLogo, {372, 78, 116, 116});
                show(aboutSkinName, {347, 200, 166, 22});
            } else
                show(brandAboutLogo, {350, 74, 160, 150});
        }
    }
    static Param distortionParameter(int slot, int field) {
        static constexpr std::array<Param, 4> first{Param::dist1on, Param::disttype, Param::drive,
                                                    Param::distmix};
        return slot == 0 ? first[size_t(field)]
                         : Param(size_t(Param::dist2on) + size_t(slot - 1) * 4 + size_t(field));
    }
    bool distortionIsMultiband() const {
        return previewDistortion >= 0 ? previewDistortion != 0 : value(Param::distbandmode) > 0;
    }
    int distortionBandCount() const {
        return previewDistortionBands > 0 ? previewDistortionBands
                                          : jlimit(1, 4, int(value(Param::distbands)) + 1);
    }
    int distortionColumns() const {
        return distortionIsMultiband() ? distortionBandCount() : 4;
    }
    int distortionPosition(int slot) const {
        const auto order = processor.distortionOrder();
        return int(std::find(order.slots.begin(), order.slots.end(), unsigned(slot)) -
                   order.slots.begin());
    }
    friend class Control;
    void shell(Graphics &g, Rectangle<int> b, const String &title, const String &hint = {}) {
        paintShell(g, b, title, hint, *this);
    }
    inline static const StringArray rackNames{"ARPEGGIATOR", "DISTORTION", "PHASER", "CHORUS",
                                               "EQUALIZER", "REVERB", "DELAY", "COMPRESSOR",
                                               "CHORDS", "SAMPLE", "MORPH", "CLIPPER", "LIMITER",
                                               "FLANGER"};
    static constexpr int flangerPage = 13;
    static constexpr int rackRowTop = 332, rackRowHeight = 18;
    static constexpr int limiterRowTop = rackRowTop + effectStageCount * rackRowHeight;
    static constexpr std::array<Param, 9> enables{Param::arpon,    Param::diston, Param::phaseron,
                                                  Param::choruson, Param::eqon,   Param::reverbon,
                                                  Param::delayon,  Param::compon, Param::chordon};
    float value(Param p) const {
        if (previewEq >= 0 && p == Param::eqmode)
            return float(previewEq);
        if (previewStretch >= 0 && String(parameterSpecs[size_t(p)].id).endsWith("stretchmode"))
            return float(previewStretch);
        if (previewWave >= 0 && String(parameterSpecs[size_t(p)].id).startsWith("o") &&
            String(parameterSpecs[size_t(p)].id).endsWith("wave"))
            return float(previewWave);
        if (previewReverb >= 0 && p == Param::reverbtype)
            return float(previewReverb);
        if (previewGran >= 0 && String(parameterSpecs[size_t(p)].id).endsWith("gmodel"))
            return float(previewGran);
        if (previewSource >= 0 && String(parameterSpecs[size_t(p)].id).endsWith("engine"))
            return float(previewSource);
        if (previewPattern >= 0 && p == Param::arppattern)
            return float(previewPattern);
        if (previewSync >= 0) {
            if (p == Param::sync)
                return float(previewSync);
            if (p == Param::lfo1free || p == Param::lfo2free)
                return 0;
        }
        return processor.effectiveValue(p);
    }
    void put(Param id, Rectangle<int> bounds, Style style, const String &label = {},
             bool isDark = false) {
        controls[size_t(id)]->configure(bounds, style, label, isDark);
    }
    void control(const String &id, Rectangle<int> b, Style style, const String &title = {},
                 bool isDark = false) {
        for (size_t i = 0; i < parameterCount; ++i)
            if (id == parameterSpecs[i].id) {
                put(Param(i), b, style, title, isDark);
                return;
            }
        jassertfalse;
    }
    void refreshPresets() {
        int previousId = presets.getSelectedId();
        presets.clear(dontSendNotification);
        presets.addSectionHeading("FACTORY");
        for (int i = 0; i < factoryPresetCount; ++i)
            presets.addItem(i < 10 ? legacyFactoryNames[size_t(i)]
                                   : designedSounds[size_t(i - 10)].name,
                            i + 1);
        userPresets =
            processor.presetDirectory().findChildFiles(File::findFiles, true, "*.logisth");
        userPresets.sort();
        auto active = processor.currentPresetFile();
        if (active.existsAsFile() && !userPresets.contains(active))
            userPresets.add(active);
        if (!userPresets.isEmpty())
            presets.addSectionHeading("USER");
        for (int i = 0; i < userPresets.size(); ++i)
            presets.addItem(userPresets[i].getFileNameWithoutExtension(), 1000 + i);
        int current = userPresets.indexOf(active);
        if (current >= 0)
            presets.setSelectedId(1000 + current, dontSendNotification);
        else if (processor.currentPresetName().isNotEmpty())
            presets.setText(processor.currentPresetName(), dontSendNotification);
        else if (previousId > 0 && previousId <= factoryPresetCount)
            presets.setSelectedId(previousId, dontSendNotification);
        else
            presets.setSelectedId(0, dontSendNotification);
        presetRevision = processor.presetRevision.load();
        if (searchPanel.isVisible())
            filterPresets();
    }
    void selectPreset(int selected) {
        if (selected >= 1 && selected <= factoryPresetCount) {
            processor.loadPreset(selected - 1);
            readout = "Factory sound / " + processor.currentPresetName();
            syncPresetNotes();
        } else if (selected >= 1000 && selected - 1000 < userPresets.size())
            processor.loadPresetAsync(userPresets[selected - 1000]);
        repaint();
    }
    void showPresetSearch(bool show) {
        searchPanel.setVisible(show);
        searchButton.setToggleState(show, dontSendNotification);
        if (show) {
            refreshPresets();
            filterPresets();
            searchPanel.toFront(false);
            searchQuery.grabKeyboardFocus();
        } else
            searchQuery.giveAwayKeyboardFocus();
        repaint();
    }
    void filterPresets() {
        matches.clear();
        auto words = StringArray::fromTokens(searchQuery.getText().trim(), " ", "");
        auto add = [&](int id, const String &name, const String &description) {
            String text = name + " " + description;
            for (const auto &word : words)
                if (!text.containsIgnoreCase(word))
                    return;
            matches.push_back({id, name, description});
        };
        for (int i = 0; i < factoryPresetCount; ++i)
            add(i + 1,
                i < 10 ? String(legacyFactoryNames[size_t(i)])
                       : String(designedSounds[size_t(i - 10)].name),
                i < 10 ? String("Factory") : String(designedSounds[size_t(i - 10)].description));
        for (int i = 0; i < userPresets.size(); ++i)
            add(1000 + i, "User / " + userPresets[i].getFileNameWithoutExtension(),
                userPresets[i].getParentDirectory().getFileName());
        searchEmpty.setVisible(matches.empty());
        searchResults.updateContent();
        searchResults.selectRow(matches.empty() ? -1 : 0);
        searchResults.repaint();
    }
    int getNumRows() override {
        return int(matches.size());
    }
    void paintListBoxItem(int row, Graphics &g, int width, int height, bool selected) override {
        if (row < 0 || row >= int(matches.size()))
            return;
        if (selected) {
            g.setColour(green.withAlpha(.35f));
            g.fillRect(0, 0, width, height);
        }
        g.setColour(screenText);
        g.setFont(font(12));
        g.drawText(matches[size_t(row)].name, 9, 0, width - 18, height, Justification::centredLeft,
                   true);
    }
    void returnKeyPressed(int row) override {
        searchResults.selectRow(row);
        chooseSearchResult();
    }
    void listBoxItemClicked(int row, const MouseEvent &) override {
        searchResults.selectRow(row);
        chooseSearchResult();
    }
    bool keyPressed(const KeyPress &key, Component *origin) override {
        if (searchPanel.isVisible() && key == KeyPress::escapeKey) {
            showPresetSearch(false);
            return true;
        }
        if (origin != &searchQuery)
            return false;
        if (key == KeyPress::upKey || key == KeyPress::downKey) {
            int row = searchResults.getSelectedRow() + (key == KeyPress::downKey ? 1 : -1);
            searchResults.selectRow(jlimit(0, jmax(0, int(matches.size()) - 1), row));
            return true;
        }
        return false;
    }
    void chooseSearchResult() {
        int row = searchResults.getSelectedRow();
        if (row < 0 || row >= int(matches.size()))
            return;
        int selected = matches[size_t(row)].id;
        showPresetSearch(false);
        presets.setSelectedId(selected, dontSendNotification);
        selectPreset(selected);
    }
    void syncPresetNotes() {
        presetNotes.setText(processor.presetComment(), false);
        notesRevision = processor.presetRevision.load();
        commentStamp = processor.commentRevision.load();
    }
    void presetMenu() {
        showPresetSearch(false);
        if (notesRevision == processor.presetRevision.load())
            processor.setPresetComment(presetNotes.getText());
        else
            syncPresetNotes();
        auto menu = look.contextMenu();
        menu.addItem(1, "Save as...");
        auto current = processor.currentPresetFile();
        menu.addItem(2, "Overwrite current user preset", current != File{});
        menu.addItem(3, "Load preset file...");
        menu.addSeparator();
        menu.addItem(4, "Choose library folder...");
        menu.addItem(5, "Refresh library");
        menu.addItem(6, "Move current preset to trash", current.existsAsFile());
        Component::SafePointer<Console> safe(this);
        menu.showMenuAsync(PopupMenu::Options{}.withTargetComponent(&presetFiles), [safe, current](
                                                                                       int choice) {
            if (!safe)
                return;
            if (choice == 2) {
                safe->processor.savePresetAsync(current);
                return;
            }
            if (choice == 5) {
                safe->refreshPresets();
                return;
            }
            if (choice == 6) {
                AlertWindow::showOkCancelBox(
                    AlertWindow::QuestionIcon, "Delete user preset?",
                    "Move " + current.getFileName() + " to the trash?", "Move to trash", "Cancel",
                    safe.getComponent(),
                    ModalCallbackFunction::create([safe, current](int confirmed) {
                        if (safe && confirmed && current.moveToTrash()) {
                            safe->readout = "Preset moved to trash";
                            safe->refreshPresets();
                            safe->repaint();
                        }
                    }));
                return;
            }
            if (choice != 1 && choice != 3 && choice != 4)
                return;
            auto directory = UserSettings::folder(choice == 1   ? UserSettings::Folder::PresetSave
                                                  : choice == 3 ? UserSettings::Folder::PresetOpen
                                                                : UserSettings::Folder::Library,
                                                  safe->processor.presetDirectory());
            safe->presetChooser = std::make_unique<FileChooser>(
                choice == 1   ? "Save logisth preset"
                : choice == 3 ? "Load logisth preset"
                              : "Choose preset library folder",
                choice == 1 ? directory.getChildFile("New preset.logisth") : directory,
                choice == 4 ? String{} : "*.logisth");
            int flags =
                choice == 4
                    ? FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories
                : choice == 1
                    ? FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles |
                          FileBrowserComponent::warnAboutOverwriting
                    : FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
            safe->presetChooser->launchAsync(flags, [safe, choice](const FileChooser &chooser) {
                if (!safe)
                    return;
                auto file = chooser.getResult();
                if (file == File{})
                    return;
                if (choice == 4) {
                    safe->processor.setPresetDirectory(file);
                    safe->refreshPresets();
                } else if (choice == 1)
                    safe->processor.savePresetAsync(file.withFileExtension(".logisth"));
                else
                    safe->processor.loadPresetAsync(file);
            });
        });
    }
    int frameSizeForImport(int slot) const {
        int selection = sourceFrame[size_t(slot)].getSelectedId();
        return selection <= 1 ? 0 : 128 << (selection - 2);
    }
    void chooseSource(int index) {
        sourceChooser = std::make_unique<FileChooser>(
            "Import audio source", UserSettings::folder(UserSettings::Folder::Sample),
            "*.wav;*.aif;*.aiff;*.flac");
        int frame = frameSizeForImport(index);
        Component::SafePointer<Console> safe(this);
        sourceChooser->launchAsync(FileBrowserComponent::openMode |
                                       FileBrowserComponent::canSelectFiles,
                                   [safe, index, frame](const FileChooser &chooser) {
                                       if (safe) {
                                           auto file = chooser.getResult();
                                           if (file.existsAsFile())
                                               safe->processor.loadSourceAsync(index, file, frame);
                                       }
                                   });
    }
    void refreshChord() {
        updatingChord = true;
        const auto map = processor.chordMap();
        const auto &chord = map->keys[size_t(chordTrigger.getSelectedId() - 1)];
        for (int i = 0; i < 8; ++i)
            chordNotes[size_t(i)].setValue(i < chord.count ? chord.notes[size_t(i)] : -1,
                                           dontSendNotification);
        chordLearn.setButtonText(processor.learning() >= 0 ? "DONE" : "LEARN");
        updatingChord = false;
    }
    void oscillator(int number, Rectangle<int> b) {
        String id = "o" + String(number);
        const size_t slot = size_t(number - 1);
        oscCopy[slot].setBounds(b.getX() + 12, b.getY() + 149, 62, 15);
        oscCopy[slot].setVisible(true);
        oscPaste[slot].setBounds(b.getX() + 12, b.getY() + 167, 62, 15);
        oscPaste[slot].setVisible(true);
        oscPaste[slot].setEnabled(LogisthProcessor::hasOscillatorClipboard());
        control(id + "engine", {b.getX() + 300, b.getY() + 6, 146, 24}, Style::Menu);
        sourceViews[slot].setBounds(b.getX() + 191, b.getY() + 6, 99, 24);
        sourceViews[slot].setVisible(true);
        const int mode = int(value(Param(playerParamIndex(Param::o1engine, int(slot), 16))));
        const bool granModel = value(Param(playerParamIndex(Param::o1gmodel, int(slot), 15))) > 0;
        auto &menu = sourceViews[slot];
        const int menuState = mode * 2 + int(granModel);
        if (sourceMenuStates[slot] != menuState) {
            int selected = menu.getSelectedId();
            menu.clear(dontSendNotification);
            menu.addItem("VOICE", 1);
            if (mode > 0)
                menu.addItem(mode == 1 ? "WAVETABLE" : "SOURCE", 2);
            menu.addItem("ENV", 3);
            if (mode == 3) {
                menu.addItem("GRAINS", 4);
                if (granModel) {
                    menu.addItem("MOTION", 5);
                    menu.addItem("TIME", 6);
                }
            }
            menu.addItem("MOD", 7);
            menu.addItem("ROUTING", 9);
            if (mode >= 2)
                menu.addItem("PLAYBACK", 8);
            if (mode == 2) {
                menu.addItem("STRETCH", 10);
                menu.addItem("LOOP TAIL", 11);
            }
            menu.setSelectedId(menu.indexOfItemId(selected) >= 0 ? selected
                               : mode == 0                       ? 1
                                                                 : 2,
                               dontSendNotification);
            sourceMenuStates[slot] = menuState;
        }
        int view = menu.getSelectedId();
        if (menu.indexOfItemId(view) < 0) {
            view = mode == 0 ? 1 : 2;
            menu.setSelectedId(view, dontSendNotification);
        }
        sourceWave[slot].setBounds(b.getX() + 86, b.getY() + 114, 358, 14);
        sourceWave[slot].setVisible(view == 2);
        if (view == 11 && mode == 2) {
            control(id + "tailon", {b.getX() + 88, b.getY() + 40, 128, 24}, Style::Toggle,
                    "LOOP TAIL");
            for (int i = 0; i < 3; ++i)
                control(id + StringArray{"tailstart", "tailend", "tailfade"}[i],
                        {b.getX() + 88 + i * 117, b.getY() + 77, 104, 82}, Style::Knob,
                        StringArray{"LOOP IN", "LOOP OUT", "CROSSFADE"}[i]);
        } else if (view == 1) {
            StringArray names{"VOLUME", "PHASE", "DETUNE", "BLEND", "STEREO", "PAN"};
            StringArray suffix{"level", "phase", "detune", "blend", "stereo", "pan"};
            for (int i = 0; i < 3; ++i)
                control(id + StringArray{"octave", "tune", "fine"}[i],
                        {b.getX() + 12, b.getY() + 34 + i * 39, 62, 33}, Style::Number,
                        StringArray{"OCTAVE", "NOTE", "FINE"}[i]);
            for (int i = 0; i < 6; ++i) {
                if (mode >= 2 && i == 1)
                    continue;
                control(id + suffix[i], {b.getX() + 86 + i * 60, b.getY() + 35, 57, 86},
                        Style::Knob, names[i]);
            }
            if (mode == 0)
                control(id + "wave", {b.getX() + 87, b.getY() + 137, 120, 26}, Style::Menu);
            else if (mode == 1)
                control(id + "position", {b.getX() + 87, b.getY() + 128, 110, 40}, Style::Number,
                        "WT POS");
            control(id + "voices", {b.getX() + 217, b.getY() + 130, 58, 38}, Style::Number,
                    "VOICES");
            if (mode <= 1)
                control(id + "retrig", {b.getX() + 287, b.getY() + 141, 74, 22}, Style::Toggle,
                        "RETRIG");
            control(id + "invert", {b.getX() + 371, b.getY() + 141, 74, 22}, Style::Toggle,
                    "INVERT");
            controls[size_t(Param(playerParamIndex(Param::o1wave, number - 1, 12)))]->enable(
                value(Param(playerParamIndex(Param::o1engine, int(slot), 16))) == 0);
            controls[size_t(Param(playerParamIndex(Param::o1retrig, number - 1, 12)))]->enable(
                value(Param(playerParamIndex(Param::o1engine, int(slot), 16))) <= 1);
            controls[size_t(Param(playerParamIndex(Param::o1phase, number - 1, 12)))]->enable(
                value(Param(playerParamIndex(Param::o1engine, int(slot), 16))) <= 1);
        } else {
            for (int i = 0; i < 3; ++i)
                control(id + StringArray{"octave", "tune", "fine"}[i],
                        {b.getX() + 12, b.getY() + 34 + i * 39, 62, 33}, Style::Number,
                        StringArray{"OCTAVE", "NOTE", "FINE"}[i]);
            if (view == 7) {
                StringArray keys{"fmdepth",    "fmratio",  "amdepth", "amratio",
                                 "pulsewidth", "pwmdepth", "pwmrate"};
                StringArray names{"FM INDEX",     "FM RATIO",  "AM DEPTH", "AM RATIO",
                                  "WIDTH / WARP", "PWM DEPTH", "PWM RATE"};
                for (int i = 0; i < keys.size(); ++i)
                    control(id + keys[i],
                            {b.getX() + 88 + (i % 4) * 88, b.getY() + 35 + (i / 4) * 69, 80, 66},
                            Style::Knob, names[i]);
            } else if (view == 9) {
                control(id + "fmsource", {b.getX() + 88, b.getY() + 35, 170, 42}, Style::Menu,
                        "FM MODULATOR");
                control(id + "amsource", {b.getX() + 269, b.getY() + 35, 175, 42}, Style::Menu,
                        "AM MODULATOR");
                control(id + "pwmsync", {b.getX() + 88, b.getY() + 103, 110, 25}, Style::Toggle,
                        "PWM SYNC");
                control(id + "pwmdivision", {b.getX() + 269, b.getY() + 93, 175, 42}, Style::Menu,
                        "PWM DIVISION");
                if (mode == 3)
                    control(id + "trimstart", {b.getX() + 88, b.getY() + 142, 170, 28},
                            Style::Number, "TRIM START");
            } else if (view == 8 && mode == 3) {
                control(id + "interpolation", {b.getX() + 88, b.getY() + 36, 172, 42},
                        Style::Menu, "INTERPOLATION");
                for (int i = 0; i < 3; ++i)
                    control(id + StringArray{"stretch", "ssrate", "sssemitones"}[i],
                            {b.getX() + 88 + i * 118, b.getY() + 86, 108, 80}, Style::Knob,
                            StringArray{"TIME FACTOR", "RATE", "SEMITONES"}[i]);
            } else if (view == 8) {
                control(id + "stretchmode", {b.getX() + 88, b.getY() + 36, 170, 42}, Style::Menu,
                        "STRETCH");
                control(id + "interpolation", {b.getX() + 269, b.getY() + 36, 175, 42}, Style::Menu,
                        "INTERPOLATION");
                control(id + "stretch", {b.getX() + 88, b.getY() + 86, 95, 80}, Style::Knob,
                        "TIME FACTOR");
                if (value(Param(playerParamIndex(Param::o1stretchmode, int(slot), 10))) == 6)
                    control(id + "kickattack", {b.getX() + 269, b.getY() + 86, 100, 80},
                            Style::Knob, "KICK ATTACK");
            } else if (view == 10) {
                auto &picker = stretchAlgorithms[slot];
                const auto layers = processor.samplerLayerMask(int(slot));
                const int solo = int(value(Param(playerParamIndex(Param::o1stretchmode, int(slot), 10))));
                if (!picker.getNumItems() || stretchPickerMasks[slot] != layers) {
                    constexpr const char *names[]{"Tape", "Granular", "WSOLA", "Tonal", "Transient",
                                                   "Kick", "Signalsmith Stretch", "PaulXStretch"};
                    const int chosen = picker.getSelectedId();
                    picker.clear(dontSendNotification);
                    for (int i = 1; i <= phaseDiffusionMode; ++i)
                        picker.addItem(String(names[i - 1]) + ((layers & (1u << i)) ? "  [ON]" : ""), i);
                    picker.setSelectedId(chosen, dontSendNotification);
                    stretchPickerMasks[slot] = layers;
                }
                if (picker.getSelectedId() == 0)
                    picker.setSelectedId(solo > 0 ? solo : phaseDiffusionMode, dontSendNotification);
                picker.setBounds(b.getX() + 88, b.getY() + 35, 206, 24);
                picker.setVisible(true);
                const int algorithm = picker.getSelectedId();
                const auto enabled = stretchLayerParam(int(slot), algorithm);
                put(Param(enabled), {b.getX() + 307, b.getY() + 35, 63, 24}, Style::Toggle, "ON");
                if (algorithm == phaseDiffusionMode) {
                    control(id + "paulamount", {b.getX() + 88, b.getY() + 61, 105, 51}, Style::Knob,
                            "STRETCH AMOUNT");
                    control(id + "paulfft", {b.getX() + 208, b.getY() + 63, 144, 40}, Style::Menu,
                            "FFT SIZE (SAMPLES)");
                    for (int i = 0; i < 3; ++i)
                        control(id + StringArray{"pauldown", "paulunity", "paulup"}[i],
                                {b.getX() + 88 + 119 * i, b.getY() + 114, 110, 54}, Style::Knob,
                                StringArray{"RATIO 0.5X", "RATIO 1X", "RATIO 2X"}[i]);
                } else {
                    const bool signalsmith = algorithm == 6 || algorithm == 7;
                    StringArray keys{"ssrate", "sssemitones", "sstonality", "ssformant", "ssblock",
                                     "ssoverlap", "stretch", "kickattack"};
                    StringArray labels{"RATE", "SEMITONES", "TONALITY LIMIT", "FORMANT", "BLOCK (ms)",
                                       "OVERLAP", "TIME FACTOR", "KICK ATTACK"};
                    std::vector<int> visible{6, 0, 1};
                    if (signalsmith) {
                        visible.push_back(2);
                        visible.push_back(3);
                    }
                    if (algorithm >= 2) {
                        visible.push_back(4);
                        visible.push_back(5);
                    }
                    if (algorithm == 6)
                        visible.push_back(7);
                    for (int at = 0; at < int(visible.size()); ++at) {
                        const int i = visible[size_t(at)];
                        control(id + keys[i],
                                {b.getX() + 88 + (at % 4) * 89, b.getY() + 62 + (at / 4) * 55, 83, 52},
                                i == 5 ? Style::Menu : Style::Knob, labels[i]);
                    }
                }
            } else if (view == 4) {
                control(id + "gmodel", {b.getX() + 88, b.getY() + 35, 150, 24}, Style::Menu);
                for (int i = 0; i < 4; ++i)
                    control(id + (granModel ? StringArray{"gattack", "ghold", "gspace", "gwave"}[i]
                                            : StringArray{"size", "density", "spray", "scan"}[i]),
                            {b.getX() + 87 + i * 90, b.getY() + 68, 76, 96}, Style::Knob,
                            granModel ? StringArray{"ATTACK", "HOLD", "GR.SPACE", "WAVE.SPACE"}[i]
                                      : StringArray{"SIZE", "DENSITY", "SPRAY", "SCAN"}[i]);
            } else if (view == 5) {
                for (int i = 0; i < 4; ++i)
                    control(id + "g" + StringArray{"pan", "depth", "speed", "random"}[i],
                            {b.getX() + 87 + i * 90, b.getY() + 38, 76, 88}, Style::Knob,
                            StringArray{"PAN", "DEPTH", "SPEED", "RAND"}[i]);
                control(id + "gfreeze", {b.getX() + 88, b.getY() + 141, 95, 22}, Style::Toggle,
                        "TIME HOLD");
                control(id + "loop", {b.getX() + 196, b.getY() + 141, 85, 22}, Style::Toggle,
                        "LOOP");
                control(id + "reverse", {b.getX() + 294, b.getY() + 141, 104, 22}, Style::Toggle,
                        "REVERSE");
            } else if (view == 6) {
                control(id + "gtrans", {b.getX() + 88, b.getY() + 36, 105, 24}, Style::Menu);
                control(id + "gkey", {b.getX() + 203, b.getY() + 36, 115, 24}, Style::Menu);
                control(id + "gsync", {b.getX() + 328, b.getY() + 36, 116, 24}, Style::Toggle,
                        "TEMPO SYNC");
                control(id + "gthold", {b.getX() + 88, b.getY() + 73, 80, 91}, Style::Knob,
                        "TRANS HOLD");
                control(id + "gbeats", {b.getX() + 188, b.getY() + 73, 80, 91}, Style::Knob,
                        "SAMPLE BEATS");
            } else if (view == 3) {
                control(id + "envelope", {b.getX() + 88, b.getY() + 36, 150, 22}, Style::Toggle,
                        "OSC ENVELOPE");
                for (int i = 0; i < 4; ++i)
                    control(id + "env" + StringArray{"a", "d", "s", "r"}[i],
                            {b.getX() + 90 + i * 89, b.getY() + 65, 70, 99}, Style::Fader,
                            StringArray{"A", "D", "S", "R"}[i]);
            } else {
                StringArray names =
                    mode == 3   ? StringArray{"VOLUME", "POSITION", "SIZE", "DENSITY", "SPRAY"}
                    : mode == 2 ? StringArray{"VOLUME", "START", "END", "STEREO", "PAN"}
                                : StringArray{"VOLUME", "WT POS", "WARP", "STEREO", "PAN"};
                StringArray suffix =
                    mode == 3   ? StringArray{"level", "position", "size", "density", "spray"}
                    : mode == 2 ? StringArray{"level", "position", "end", "stereo", "pan"}
                                : StringArray{"level", "position", "warp", "stereo", "pan"};
                if (mode == 3 &&
                    value(Param(playerParamIndex(Param::o1gmodel, int(slot), 15))) > 0) {
                    names = {"VOLUME", "START", "HOLD", "GR.SPACE", "WAVE.SPACE"};
                    suffix = {"level", "position", "ghold", "gspace", "gwave"};
                }
                for (int i = 0; i < 5; ++i)
                    control(id + suffix[i], {b.getX() + 86 + i * 72, b.getY() + 35, 66, 80},
                            Style::Knob, names[i]);
                if (mode >= 2) {
                    control(id + "root", {b.getX() + 88, b.getY() + 129, 48, 35}, Style::Number,
                            "ROOT");
                    if (mode == 3 &&
                        value(Param(playerParamIndex(Param::o1gmodel, int(slot), 15))) == 0)
                        control(id + "scan", {b.getX() + 145, b.getY() + 129, 68, 35},
                                Style::Number, "SCAN");
                    else
                        control(id + "loop", {b.getX() + 145, b.getY() + 141, 68, 22},
                                Style::Toggle, "LOOP");
                    control(id + "reverse", {b.getX() + 223, b.getY() + 141, 68, 22}, Style::Toggle,
                            "REVERSE");
                    sourceLoad[slot].setBounds(b.getX() + 300, b.getY() + 141, 68, 22);
                    sourceClear[slot].setBounds(b.getX() + 379, b.getY() + 141, 65, 22);
                } else {
                    sourceLoad[slot].setBounds(b.getX() + 88, b.getY() + 141, 68, 22);
                    sourceClear[slot].setBounds(b.getX() + 254, b.getY() + 141, 68, 22);
                }
                sourceLoad[slot].setVisible(true);
                sourceClear[slot].setVisible(true);
            }
        }
    }

    void envelope(const String &base, Rectangle<int> b, bool mini) {
        const int w = mini ? 29 : 48, gap = mini ? 6 : 12, left = mini ? 10 : 19,
                  top = mini ? 31 : 59, height = mini ? 83 : 105;
        for (int i = 0; i < 4; ++i)
            control(base + StringArray{"a", "d", "s", "r"}[i],
                    {b.getX() + left + i * (w + gap), b.getY() + top, w, height}, Style::Fader,
                    StringArray{"A", "D", "S", "R"}[i]);
    }
    void routes(const String &base, Rectangle<int> b, int y, int row, bool isDark = false) {
        for (int i = 1; i <= 2; ++i) {
            control(base + "amt" + String(i), {b.getX() + 8, b.getY() + y + (i - 1) * row, 30, 30},
                    Style::Trim, {}, isDark);
            control(base + "dest" + String(i),
                    {b.getX() + 43, b.getY() + y + 3 + (i - 1) * row, b.getWidth() - 53, 24},
                    Style::Menu, {}, isDark);
        }
    }
    void lfo(int index, Rectangle<int> b) {
        String id = "lfo" + String(index);
        control(id + "wave", {b.getX() + 10, b.getY() + 30, 111, 23}, Style::Menu);
        control(id + "free", {b.getX() + 130, b.getY() + 30, 59, 23}, Style::Toggle, "FREE");
        bool clock =
            value(Param::sync) > 0 && value(index == 1 ? Param::lfo1free : Param::lfo2free) == 0;
        control(id + (clock ? "division" : "rate"), {b.getX() + 10, b.getY() + 60, 56, 66},
                clock ? Style::Menu : Style::Knob, "RATE");
        control(id + "gain", {b.getX() + 72, b.getY() + 60, 56, 66}, Style::Knob, "GAIN");
        control(id + "offset", {b.getX() + 134, b.getY() + 60, 56, 66}, Style::Knob, "OFFSET");
        routes(id, b, 133, 35);
    }
    void misc(int first, Rectangle<int> b) {
        for (int source = 0; source < 2; ++source) {
            String id = "misc" + String(first + source);
            int y = source == 0 ? 28 : 113;
            control(id + "source", {b.getX() + 10, b.getY() + y, b.getWidth() - 20, 23},
                    Style::Menu);
            for (int i = 1; i <= 2; ++i) {
                control(id + "amt" + String(i),
                        {b.getX() + 8, b.getY() + y + 26 + (i - 1) * 27, 28, 27}, Style::Trim);
                control(id + "dest" + String(i),
                        {b.getX() + 43, b.getY() + y + 28 + (i - 1) * 27, b.getWidth() - 53, 23},
                        Style::Menu);
            }
        }
    }
    static int effectRackPage(EffectStage stage) {
        constexpr std::array<int, effectStageCount> pages{1, 2, 3, 4, 5, 6, 7, 11, flangerPage};
        return pages[size_t(stage)];
    }
    int effectPosition(int page) const {
        const auto order = processor.effectOrder();
        for (int i = 0; i < effectStageCount; ++i)
            if (effectRackPage(order.stages[size_t(i)]) == page)
                return i;
        return -1;
    }
    void dragRack(int page, Point<int> position) {
        if (!Rectangle<int>(289, rackRowTop, 112, effectStageCount * rackRowHeight).contains(position))
            return;
        const int target = jlimit(0, effectStageCount - 1, (position.y - rackRowTop) / rackRowHeight);
        if (processor.moveEffect(effectPosition(page), target)) {
            readout = "Effect order saved with the patch / Main -> Limiter -> Output";
            layout();
            repaint();
        }
    }
    void rackLayout() {
        Rectangle<int> content{414, 320, 520, 203};
        for (auto &name : macroNames)
            name.setVisible(false);
        for (auto &button : macroSelect)
            button.setVisible(false);
        for (auto &button : mutationLock)
            button.setVisible(false);
        mutateButton.setVisible(false);
        undoButton.setVisible(false);
        const auto order = processor.effectOrder();
        auto place = [&](int i, Rectangle<int> bounds) {
            rackButtons[size_t(i)]->setBounds(bounds);
            if (i < 9 || i == 11 || i == 12 || i == flangerPage) {
                const Param enabled =
                    i < 9 ? enables[size_t(i)] : i == 11 ? Param::clipon
                        : i == 12 ? Param::limiton : Param::flangeron;
                put(enabled, {bounds.getX() + 3, bounds.getY(), 20, bounds.getHeight()},
                    Style::Toggle, {}, true);
                controls[size_t(enabled)]->toFront(false);
            }
        };
        place(8, {291, 294, 62, 22});
        place(0, {353, 294, 46, 22});
        for (int row = 0; row < effectStageCount; ++row)
            place(effectRackPage(order.stages[size_t(row)]), {291, rackRowTop + row * rackRowHeight, 108, rackRowHeight});
        place(12, {291, limiterRowTop, 108, rackRowHeight});
        place(9, {291, 514, 54, 20});
        place(10, {346, 514, 53, 20});
        settingsPanel.setVisible(settingsOpen);
        if (settingsOpen) {
            settingsPanel.setBounds(414, 289, 520, 234);
            layoutSettings();
            settingsPanel.toFront(false);
            return;
        }
        if (rack == 1) {
            const auto order = processor.distortionOrder();
            const bool multi = distortionIsMultiband();
            const int columns = distortionColumns(), columnWidth = 520 / columns;
            put(Param::distbandmode,
                multi ? Rectangle<int>{520, 286, 116, 24} : Rectangle<int>{710, 286, 224, 24},
                Style::Menu, {}, true);
            if (multi) {
                put(Param::distbands, {642, 286, 46, 24}, Style::Menu, {}, true);
                distAnalysis.setBounds(694, 286, 120, 24);
                distAnalysis.setVisible(true);
                distTilt.setBounds(820, 286, 114, 24);
                distTilt.setVisible(true);
                if (distortionPosition(selectedDistortion) >= columns)
                    selectedDistortion = int(order.slots[0]);
            }
            for (int position = 0; position < columns; ++position) {
                const auto slot = int(order.slots[size_t(position)]);
                auto &button = *distortionButtons[size_t(slot)];
                button.setBounds(414 + position * columnWidth, 315, columnWidth - 8, 25);
                button.setVisible(true);
                put(distortionParameter(slot, 0),
                    {414 + position * columnWidth, multi ? 343 : 346, columnWidth - 8,
                     multi ? 22 : 24},
                    Style::Toggle, "ON / OFF", true);
            }
            if (multi) {
                distortionView.setBounds(414, 368, 520, 81);
                distortionView.selectBand(distortionPosition(selectedDistortion));
                const auto analysis = UserSettings::distortionAnalyzer();
                distortionView.setAnalyzer(analysis.mode, analysis.tilt);
                distortionView.setVisible(true);
                processor.distortionSpectrumEnabled = analysis.mode != 0;
                put(distortionParameter(selectedDistortion, 1), {414, 453, 240, 38}, Style::Menu,
                    "MODEL", true);
                for (int cut = 0; cut < columns - 1; ++cut)
                    put(Param(size_t(Param::distcross1) + size_t(cut)),
                        {414 + cut * 82, 494, 76, 30}, Style::Number,
                        "C" + String(cut + 1) + " / Hz", true);
                put(distortionParameter(selectedDistortion, 2), {674, 453, 124, 71}, Style::Knob,
                    "DRIVE", true);
                put(distortionParameter(selectedDistortion, 3), {810, 453, 124, 71}, Style::Knob,
                    "MIX", true);
            } else {
                put(distortionParameter(selectedDistortion, 1), {414, 383, 180, 43}, Style::Menu,
                    "MODEL", true);
                put(distortionParameter(selectedDistortion, 2), {612, 383, 152, 129}, Style::Knob,
                    "DRIVE", true);
                put(distortionParameter(selectedDistortion, 3), {782, 383, 152, 129}, Style::Knob,
                    "MIX", true);
                distortionHelp.setBounds(414, 434, 180, 81);
                distortionHelp.setColour(Label::textColourId, screenText);
                distortionHelp.setVisible(true);
            }
            return;
        }
        if (rack == 7 || rack == 11 || rack == 12) {
            const int mode = rack == 7 ? 0 : rack == 11 ? 1 : 2;
            dynamicsView.setMode(mode);
            dynamicsView.setBounds(414, 289, 520, 90);
            dynamicsView.setVisible(true);
            const std::vector<Param> ids =
                mode == 0 ? std::vector<Param>{Param::compmode,   Param::compthreshold,
                                               Param::compratio,  Param::compknee,
                                               Param::compattack, Param::comprelease,
                                               Param::compmakeup, Param::compmix}
                : mode == 1
                    ? std::vector<Param>{Param::clipmode, Param::clipdrive, Param::clipceiling,
                                         Param::clipknee, Param::clipmix,   Param::clipquality}
                    : std::vector<Param>{Param::limitdrive, Param::limitceiling,
                                         Param::limitrelease};
            const int columns = mode == 0 ? 4 : 3;
            const int cellWidth = 520 / columns;
            for (size_t i = 0; i < ids.size(); ++i) {
                const auto &spec = parameterSpecs[size_t(ids[i])];
                put(ids[i],
                    {414 + int(i % columns) * cellWidth, 389 + int(i / columns) * 65, cellWidth - 8,
                     mode == 2 ? 123 : 63},
                    *spec.choices ? Style::Menu : Style::Knob,
                    String(spec.label).upToFirstOccurrenceOf(" /", false, false).toUpperCase(),
                    true);
            }
            if (mode == 0)
                controls[size_t(Param::compknee)]->enable(value(Param::compmode) > 0);
            if (mode == 1)
                controls[size_t(Param::clipknee)]->enable(value(Param::clipmode) > 0);
            return;
        }
        if (rack == 5 && int(value(Param::reverbtype)) == 5) {
            put(Param::reverbtype, {414, 289, 180, 25}, Style::Menu, {}, true);
            impulseLoad.setBounds(608, 289, 108, 25);
            impulseLoad.setVisible(true);
            impulseReset.setBounds(728, 289, 108, 25);
            impulseReset.setVisible(true);
            impulseName.setBounds(414, 320, 520, 30);
            impulseView.setBounds(414, 354, 520, 70);
            impulseView.setVisible(true);
            impulseName.setVisible(true);
            refreshImpulseLabel();
            put(Param::irnormalize, {846, 289, 88, 25}, Style::Toggle, "NORMALIZE", true);
            for (int i = 0; i < 4; ++i)
                put(std::array<Param, 4>{Param::reverb, Param::reverbpredelay, Param::reverbwidth,
                                         Param::reverbout}[size_t(i)],
                    {414 + i * 130, 432, 122, 85}, Style::Knob,
                    StringArray{"MIX", "PREDELAY", "WIDTH", "WET GAIN"}[i], true);
            return;
        }
        if (rack == 10) {
            layoutMorph();
            return;
        }
        if (rack == 4) {
            put(Param::eqmode, {414, 289, 114, 24}, Style::Menu, {}, true);
            put(Param::eqautogain, {536, 289, 100, 24}, Style::Toggle, "AUTO GAIN", true);
            eqAnalysisLabel.setBounds(414, 317, 78, 22);
            eqPre.setBounds(494, 317, 66, 22);
            eqPost.setBounds(568, 317, 66, 22);
            eqTiltLabel.setBounds(646, 317, 40, 22);
            eqTilt.setBounds(688, 317, 116, 22);
            eqAnalysisLabel.setVisible(true);
            for (auto *label : {&eqAnalysisLabel, &eqTiltLabel})
                label->setColour(Label::textColourId, screenText);
            eqPre.setColour(TextButton::buttonOnColourId, mint);
            eqPost.setColour(TextButton::buttonOnColourId, screenAccent);
            eqPre.setColour(TextButton::textColourOnId, screen);
            eqPost.setColour(TextButton::textColourOnId, screen);
            eqView.setBounds(414, 345, 520, 87);
            eqView.setVisible(true);
            applyEqAnalyzer(UserSettings::analyzer());
            if (value(Param::eqmode) > 0) {
                eqBandSelect.setBounds(644, 289, 112, 24);
                eqBandSelect.setVisible(true);
                eqAdd.setBounds(764, 289, 78, 24);
                eqAdd.setVisible(true);
                bool freeBand = false;
                for (auto type : eqBandTypes)
                    freeBand |= value(Param(size_t(type) + 3)) < .5f;
                eqAdd.setEnabled(freeBand);
                eqRemove.setBounds(850, 289, 84, 24);
                eqRemove.setVisible(true);
                size_t at = size_t(eqBandTypes[size_t(selectedEqBand)]);
                put(Param(at), {414, 440, 158, 34}, Style::Menu, "TYPE", true);
                put(Param(at + 3), {582, 447, 82, 25}, Style::Toggle, "ENABLED", true);
                put(Param::eqoutput, {414, 482, 112, 34}, Style::Number, "OUTPUT", true);
                const auto slope = eqBandSlope(selectedEqBand);
                put(slope, {538, 482, 126, 34}, Style::Menu, "SLOPE (dB/OCT)", true);
                controls[size_t(slope)]->enable(eqIsCut(int(value(Param(at)))));
                put(Param(at + 1), {672, 440, 82, 76}, Style::Knob, "FREQ", true);
                put(eqBandGains[size_t(selectedEqBand)], {762, 440, 82, 76}, Style::Knob, "GAIN",
                    true);
                put(Param(at + 2), {852, 440, 82, 76}, Style::Knob, "Q", true);
                controls[size_t(eqBandGains[size_t(selectedEqBand)])]->enable(value(Param(at)) < 3);
                eqView.selectBand(selectedEqBand);
            } else {
                put(Param::eqoutput, {803, 289, 131, 24}, Style::Number, "OUT / dB", true);
                for (int i = 0; i < 4; ++i)
                    put(Param(size_t(Param::eqbass) + size_t(i)), {414 + i * 130, 440, 122, 78},
                        Style::Knob, StringArray{"BASS", "BASS FREQ", "TREBLE", "TREBLE FREQ"}[i],
                        true);
            }
            return;
        }
        if (rack == 9) {
            waveSlot.setBounds(414, 289, 110, 23);
            waveSlot.setVisible(true);
            waveView.setBounds(414, 320, 520, 203);
            waveView.setVisible(true);
            int slot = waveView.selectedSlot();
            int mode = int(value(Param(playerParamIndex(Param::o1engine, slot, 16))));
            waveView.setMode(mode);
            if (mode == 2 || mode == 3) {
                waveSlot.setSize(67, 23);
                showSampleFields(slot, mode);
            } else if (mode == 1) {
                sourceFrame[size_t(slot)].setBounds(846, 289, 88, 23);
                sourceFrame[size_t(slot)].setVisible(true);
            }
            return;
        }
        if (rack == 8) {
            chordTrigger.setBounds(414, 329, 140, 30);
            chordTrigger.setVisible(true);
            chordType.setBounds(566, 329, 145, 30);
            chordType.setVisible(true);
            chordLearn.setBounds(724, 329, 92, 30);
            chordLearn.setVisible(true);
            chordCopy.setBounds(829, 329, 98, 30);
            chordCopy.setVisible(true);
            for (int i = 0; i < 8; ++i) {
                chordNotes[size_t(i)].setBounds(414 + i * 65, 405, 58, 36);
                chordNotes[size_t(i)].setVisible(true);
            }
            return;
        }
        if (rack == 0 && value(Param::arppattern) > 0) {
            put(Param::arppattern, {414, 320, 100, 24}, Style::Menu, {}, true);
            put(Param::arpswing, {652, 289, 60, 22}, Style::Number, {}, true);
            put(Param::arplatch, {521, 320, 55, 24}, Style::Toggle, "LATCH", true);
            put(value(Param::sync) > 0 ? Param::arpdivision : Param::arptime, {582, 320, 64, 24},
                value(Param::sync) > 0 ? Style::Menu : Style::Number, {}, true);
            put(Param::arpvelocity, {652, 320, 100, 24}, Style::Menu, {}, true);
            patternSlot.setBounds(758, 320, 44, 24);
            patternSlot.setVisible(true);
            patternSave.setBounds(808, 320, 48, 24);
            patternSave.setVisible(true);
            patternRandom.setBounds(862, 320, 64, 24);
            patternRandom.setVisible(true);
            for (int i = 0; i < 2; ++i) {
                stepPages[size_t(i)].setButtonText(i ? "17-32" : "01-16");
                stepPages[size_t(i)].setBounds(813 + i * 60, 289, 54, 22);
                stepPages[size_t(i)].setToggleState(stepPage == i, dontSendNotification);
                stepPages[size_t(i)].setVisible(true);
            }
            for (int row = 0; row < 7; ++row) {
                if (row < 6)
                    put(Param(size_t(Param::arpnotelen) + size_t(row)),
                        {468, 359 + row * 22, 32, 19}, Style::Number, {}, true);
                for (int i = 0; i < 16; ++i) {
                    int step = stepPage * 16 + i + 1;
                    String id = row == 2   ? "step" + String(step) + "velocity"
                                : row == 6 ? "step" + String(step) + "hold"
                                           : "pat" + String(step) +
                                                 StringArray{"note",   "oct",    "", "gate",
                                                             "repeat", "chance", ""}[row];
                    control(id, {509 + i * 26, 359 + row * 22, 24, 19},
                            row == 6 ? Style::Toggle : Style::Number, {}, true);
                }
            }
            patternCanvas.setBounds(509, 359, 416, 151);
            patternCanvas.setVisible(true);
            patternCanvas.toFront(false);
            return;
        }
        if (rack == 0) {
            put(Param::arppattern, {813, 289, 112, 22}, Style::Menu, {}, true);
            control("arpmode", {content.getX(), content.getY(), 128, 38}, Style::Menu, "MODE",
                    true);
            control("arpvelocity", {content.getX(), content.getY() + 43, 128, 38}, Style::Menu,
                    "VELOCITY", true);
            bool sync = value(Param::sync) > 0;
            put(sync ? Param::arpdivision : Param::arptime,
                {content.getX() + 140, content.getY(), 72, 80}, sync ? Style::Menu : Style::Knob,
                "TIME", true);
            put(Param::arpgate, {content.getX() + 222, content.getY(), 62, 80}, Style::Knob, "GATE",
                true);
            put(Param::arpswing, {content.getX() + 290, content.getY(), 62, 80}, Style::Knob,
                "SWING", true);
            put(Param::arpoctaves, {content.getX() + 365, content.getY() + 3, 44, 36},
                Style::Number, "OCT", true);
            put(Param::arpwrap, {content.getX() + 418, content.getY() + 3, 44, 36}, Style::Number,
                "WRAP", true);
            put(Param::arpsteps, {content.getX() + 471, content.getY() + 3, 44, 36}, Style::Number,
                "LENGTH", true);
            for (int i = 0; i < 2; ++i) {
                stepPages[size_t(i)].setButtonText(i ? "09-16" : "01-08");
                stepPages[size_t(i)].setBounds(content.getX() + 365 + i * 77, content.getY() + 55,
                                               70, 22);
                stepPages[size_t(i)].setToggleState(stepPage == i, dontSendNotification);
                stepPages[size_t(i)].setVisible(true);
            }
            for (int i = 0; i < 8; ++i) {
                int step = stepPage * 8 + i + 1, x = content.getX() + 42 + i * 59;
                String base = "step" + String(step);
                control(base + "transpose", {x + 3, content.getY() + 108, 50, 23}, Style::Number,
                        {}, true);
                control(base + "velocity", {x + 3, content.getY() + 136, 50, 23}, Style::Number, {},
                        true);
                control(base + "hold", {x + 6, content.getY() + 165, 44, 18}, Style::Toggle, "HOLD",
                        true);
                controls[size_t(Param(size_t(Param::step1transpose) + size_t(step - 1) * 3))]
                    ->enable(value(Param::arpmode) >= 8);
            }
        } else {
            const auto group = rackNames[rack];
            const auto enable = rack == flangerPage ? Param::flangeron : enables[size_t(rack)];
            std::vector<size_t> list;
            bool sync = value(Param::sync) > 0;
            for (size_t i = 0; i < parameterCount; ++i) {
                auto &s = parameterSpecs[i];
                String id(s.id);
                if (group != s.group || Param(i) == enable)
                    continue;
                bool seconds = id == "phaserrate" || id == "chorusrate" || id == "delaytime" ||
                               id == "delayright";
                bool division = id == "phaserdivision" || id == "chorusdivision" ||
                                id == "delaydivL" || id == "delaydivR";
                if ((seconds && sync) || (division && !sync))
                    continue;
                int reverbType = int(value(Param::reverbtype));
                if (((id == "shimmeramount" || id == "shimmerpitch") && reverbType != 4) ||
                    (id == "springdispersion" && reverbType != 2) ||
                    (id == "vintagemod" && reverbType != 3) ||
                    (id == "irnormalize" && reverbType != 5))
                    continue;
                list.push_back(i);
            }
            if (group == "CHORUS") {
                // Keep RATE in the same cell in Sync and free modes. The host
                // parameter order places the division later than the Hz rate.
                list.clear();
                for (auto parameter :
                     {Param::chorusdelay, Param::chorusdepth, Param::Count,
                      sync ? Param::chorusdivision : Param::chorusrate, Param::choruswidth,
                      Param::chorus, Param::chorusfeedback, Param::chorusdual})
                    list.push_back(size_t(parameter));
            }
            if (group == "REVERB") {
                auto type = std::find(list.begin(), list.end(), size_t(Param::reverbtype));
                if (type != list.end())
                    std::rotate(list.begin(), type, type + 1);
            }
            const int columns = list.size() <= 6 ? int(list.size()) : int((list.size() + 1) / 2);
            int rows = int((list.size() + size_t(columns) - 1) / size_t(columns));
            int cellWidth = 520 / columns;
            for (size_t k = 0; k < list.size(); ++k) {
                auto i = list[k];
                if (i == parameterCount)
                    continue;
                auto &s = parameterSpecs[i];
                String label = String(s.label)
                                   .replace(" / Hz", "")
                                   .replace(" / ms", "")
                                   .replace(" / dB", "")
                                   .replace(" / s", "")
                                   .replace("Sync division", "RATE")
                                   .replace("LFO rate", "RATE")
                                   .toUpperCase();
                if (String(s.id) == "delaydivL")
                    label = "LEFT";
                if (String(s.id) == "delaydivR")
                    label = "RIGHT";
                Style style = *s.choices
                                  ? (String(s.choices) == "Off|On" ? Style::Toggle : Style::Menu)
                                  : Style::Knob;
                int h = rows == 1 ? 117 : 89;
                auto cell = Rectangle<int>(
                    content.getX() + int(k % size_t(columns)) * cellWidth + 4,
                    content.getY() + (rows == 1 ? 20 : 0) + int(k / size_t(columns)) * 100,
                    cellWidth - 8, h);
                if (style == Style::Toggle) {
                    if (group == "CHORUS")
                        cell = cell.withHeight(cell.getHeight() - 2);
                    cell = cell.withSizeKeepingCentre(cell.getWidth(), 24);
                }
                put(Param(i), cell, style, label, true);
            }
        }
    }
    void layout() {
        for (auto &c : controls) {
            c->setVisible(false);
            c->enable(true);
        }
        patternCanvas.setVisible(false);
        waveView.setVisible(false);
        for (auto &field : sampleFields)
            if (field)
                field->setVisible(false);
        eqView.setVisible(false);
        distortionView.setVisible(false);
        distAnalysis.setVisible(false);
        distTilt.setVisible(false);
        processor.distortionSpectrumEnabled = false;
        processor.eqSpectrumEnabled = false;
        for (auto &button : distortionButtons)
            button->setVisible(false);
        distortionHelp.setVisible(false);
        for (Component *c : std::array<Component *, 5>{&eqPre, &eqPost, &eqTilt, &eqAnalysisLabel,
                                                       &eqTiltLabel})
            c->setVisible(false);
        impulseView.setVisible(false);
        dynamicsView.setVisible(false);
        processor.dynamicsView = !settingsOpen && (rack == 7 || rack == 11 || rack == 12)
                                     ? (rack == 7    ? 0
                                        : rack == 11 ? 1
                                                     : 2)
                                     : -1;
        for (Component *c : std::array<Component *, 3>{&eqBandSelect, &eqAdd, &eqRemove})
            c->setVisible(false);
        waveSlot.setVisible(false);
        for (auto &area : sourceWave)
            area.setVisible(false);
        for (auto &button : stepPages)
            button.setVisible(false);
        for (auto &w : sourceViews)
            w.setVisible(false);
        for (auto &w : stretchAlgorithms)
            w.setVisible(false);
        for (auto &w : oscCopy)
            w.setVisible(false);
        for (auto &w : oscPaste)
            w.setVisible(false);
        impulseLoad.setVisible(false);
        impulseReset.setVisible(false);
        impulseName.setVisible(false);
        for (auto &w : sourceLoad)
            w.setVisible(false);
        for (auto &w : sourceClear)
            w.setVisible(false);
        for (auto &w : sourceFrame)
            w.setVisible(false);
        for (Component *w :
             std::array<Component *, 7>{&patternSlot, &patternSave, &patternRandom, &chordTrigger,
                                        &chordType, &chordLearn, &chordCopy})
            w->setVisible(false);
        for (auto &w : chordNotes)
            w.setVisible(false);
        presets.setBounds(364, 24, 274, 26);
        searchButton.setBounds(644, 24, 26, 26);
        searchPanel.setBounds(332, 56, 446, 300);
        searchQuery.setBounds(10, 10, 426, 27);
        searchResults.setBounds(10, 45, 426, 245);
        searchEmpty.setBounds(19, 50, 408, 30);
        if (searchPanel.isVisible())
            searchPanel.toFront(false);
        previous.setBounds(332, 24, 26, 26);
        next.setBounds(676, 24, 26, 26);
        presetFiles.setBounds(708, 24, 70, 26);
        presetNotes.setBounds(332, 54, 446, 23);
        for (int i = 0; i < partCount; ++i) {
            parts[size_t(i)].setBounds(832 + i * 29, 28, 26, 24);
            parts[size_t(i)].setToggleState(i == selectedPart, dontSendNotification);
        }
        put(Param::solo, {927, 28, 68, 24}, Style::Menu);
        put(Param::polyphony, {1010, 16, 60, 38}, Style::Number, "POLY");
        put(Param::sync, {1085, 28, 64, 24}, Style::Toggle, "SYNC");
        modulationToggle.setBounds(skin == "ap-flexin" ? Rectangle<int>(120, 28, 95, 24)
                                                       : Rectangle<int>(85, 28, 130, 24));
        settings.setBounds(1160, 28, 64, 24);
        settings.setToggleState(settingsOpen, dontSendNotification);
        put(Param::arpout, {225, 29, 82, 24}, Style::Toggle, "MIDI OUT");
        put(Param::midionly, {225, 55, 82, 19}, Style::Toggle, "MIDI ONLY");
        const int first = selectedPart * 2 + 1;
        oscillator(first, {16, 82, 460, 184});
        oscillator(first + 1, {764, 82, 460, 184});
        envelope("p" + String(selectedPart + 1) + "amp", {486, 82, 268, 184}, false);
        String filter = "p" + String(selectedPart + 1);
        control(filter + "input", {28, 311, 60, 24}, Style::Menu);
        control(filter + "type", {94, 311, 100, 24}, Style::Menu);
        control(filter + "slope", {200, 311, 56, 24}, Style::Menu);
        for (int i = 0; i < 3; ++i)
            control(filter + StringArray{"cutoff", "res", "drive"}[i], {28 + i * 76, 341, 70, 72},
                    Style::Knob, StringArray{"CUTOFF", "RESO", "DRIVE"}[i]);
        put(Param::filtercutoff, {27, 462, 57, 66}, Style::Knob, "CUTOFF");
        put(Param::filterres, {89, 462, 57, 66}, Style::Knob, "RESO");
        put(Param::keytrack, {151, 462, 57, 66}, Style::Knob, "KEYTRACK");
        put(Param::warmdrive, {215, 479, 42, 22}, Style::Toggle, "WARM");
        scopeView.setBounds(972, 309, 236, 85);
        for (int i = 0; i < 4; ++i)
            scopeSources[size_t(i)].setBounds(972 + (i == 2   ? 3
                                                     : i == 3 ? 2
                                                              : i) *
                                                        60,
                                              402, 56, 22);
        put(Param::mixA, {975, 429, 35, 95}, Style::Fader, "MIX A");
        put(Param::mixB, {1015, 429, 35, 95}, Style::Fader, "MIX B");
        put(Param::mixC, {1055, 429, 35, 95}, Style::Fader, "MIX C");
        put(Param::master, {1095, 429, 35, 95}, Style::Fader, "MAIN");
        if (!modulationCollapsed) {
            envelope("env1", {16, 548, 152, 208}, true);
            envelope("env2", {176, 548, 152, 208}, true);
            routes("env1", {16, 548, 152, 208}, 128, 35);
            routes("env2", {176, 548, 152, 208}, 128, 35);
            lfo(1, {336, 548, 200, 208});
            lfo(2, {544, 548, 200, 208});
            misc(1, {752, 548, 232, 208});
            misc(3, {992, 548, 232, 208});
        }
        const int lowerOffset = modulationCollapsed ? EditorLayout::modulationHeight : 0;
        rackLayout();
        pitch.setBounds(29, 784 - lowerOffset, 20, 46);
        mod.setBounds(63, 784 - lowerOffset, 20, 46);
        keyboard.setBounds(118, 771 - lowerOffset, 864, 70);
        keyboard.setLowestVisibleKey(36);
        put(Param::monolegato, {1004, 779 - lowerOffset, 94, 21}, Style::Toggle, "MONO / LEGATO");
        put(Param::portamode, {1108, 779 - lowerOffset, 104, 21}, Style::Menu);
        put(Param::bendrange, {1004, 809 - lowerOffset, 48, 28}, Style::Number, "BEND / st");
        put(Param::portamento, {1126, 813 - lowerOffset, 76, 24}, Style::Number);
        parts[0].repaint();
        parts[1].repaint();
        parts[2].repaint();
        for (auto &b : rackButtons)
            b->repaint();
        layoutStamp = currentLayoutState();
    }
    std::array<int64_t, 14 + 4 * playerCount> currentLayoutState() const {
        std::array<int64_t, 14 + 4 * playerCount> state{
            int(value(Param::sync)), int(value(Param::lfo1free)), int(value(Param::lfo2free)),
            int(value(Param::arpmode) >= 8), int(value(Param::arppattern))};
        for (int o = 0; o < playerCount; ++o) {
            state[size_t(5 + o)] = int(value(Param(playerParamIndex(Param::o1engine, o, 16))));
            state[size_t(5 + playerCount + o)] =
                int(value(Param(playerParamIndex(Param::o1gmodel, o, 15))));
            state[size_t(7 + 2 * playerCount + o)] =
                int(value(Param(playerParamIndex(Param::o1wave, o, 12))));
            state[size_t(7 + 3 * playerCount + o)] =
                int(value(Param(playerParamIndex(Param::o1stretchmode, o, 10)))) +
                16 * int(processor.samplerLayerMask(o));
        }
        state[5 + 2 * playerCount] = int(value(Param::reverbtype));
        state[6 + 2 * playerCount] = int(value(Param::eqmode));
        int mask = 0;
        for (int i = 0; i < eqMaxBands; ++i)
            if (value(Param(size_t(eqBandTypes[size_t(i)]) + 3)) > 0)
                mask |= 1 << i;
        state[7 + 4 * playerCount] = mask + 65536 * int(value(eqBandTypes[size_t(selectedEqBand)]));
        state[8 + 4 * playerCount] = int(value(Param::compmode));
        state[9 + 4 * playerCount] = int(value(Param::clipmode));
        state[10 + 4 * playerCount] = int64_t(processor.effectOrder().packed());
        state[11 + 4 * playerCount] = int(processor.distortionOrder().packed());
        state[12 + 4 * playerCount] = int(value(Param::distbandmode));
        state[13 + 4 * playerCount] = int(value(Param::distbands));
        return state;
    }
    void saveEqAnalyzer() {
        // Keep the original preference bits so PRE and POST can toggle independently.
        const int mode = (eqPre.getToggleState() ? 1 : 0) | (eqPost.getToggleState() ? 2 : 0);
        const UserSettings::Analyzer analyzer{mode, eqTilt.getSelectedId() - 1};
        if (!UserSettings::setAnalyzer(analyzer))
            readout = "Analyzer preference could not be saved.";
        applyEqAnalyzer(analyzer);
        repaint();
    }
    void applyEqAnalyzer(UserSettings::Analyzer analyzer) {
        const bool visible = eqView.isVisible();
        const bool enabled = analyzer.mode != 0;
        eqPre.setToggleState((analyzer.mode & 1) != 0, dontSendNotification);
        eqPost.setToggleState((analyzer.mode & 2) != 0, dontSendNotification);
        eqPre.setVisible(visible);
        eqPost.setVisible(visible);
        eqTilt.setSelectedId(analyzer.tilt + 1, dontSendNotification);
        eqTilt.setVisible(visible && enabled);
        eqTiltLabel.setVisible(visible && enabled);
        processor.eqSpectrumEnabled = enabled && eqView.isShowing();
        eqView.setAnalyzer(analyzer.mode, analyzer.tilt);
    }
    void refreshImpulseLabel() {
        auto data = processor.impulseData();
        if (!data)
            return;
        impulseName.setText(data->name + "\n" + String(data->channels == 1 ? "Mono" : "Stereo") +
                                " / " + String(data->sampleRate / 1000, 1) + " kHz / " +
                                String(double(data->samples[0].size()) / data->sampleRate, 2) +
                                " s" + (processor.impulseReady() ? "" : " / preparing..."),
                            dontSendNotification);
        impulseName.setColour(Label::textColourId, screenText);
    }
    void timerCallback() override {
        if (keyboard.isShowing())
            keyboard.refreshPlayedKeys();
        for (auto &button : oscPaste)
            button.setEnabled(LogisthProcessor::hasOscillatorClipboard());
        if (impulseName.isVisible()) {
            refreshImpulseLabel();
            impulseView.repaint();
        }
        if (dynamicsView.isVisible())
            dynamicsView.repaint();
        const auto distAnalyzer = UserSettings::distortionAnalyzer();
        processor.distortionSpectrumEnabled = distortionView.isShowing() && distAnalyzer.mode != 0;
        if (distortionView.isVisible()) {
            distAnalysis.setSelectedId(distAnalyzer.mode + 1, dontSendNotification);
            distTilt.setSelectedId(distAnalyzer.tilt + 1, dontSendNotification);
            distortionView.setAnalyzer(distAnalyzer.mode, distAnalyzer.tilt);
            distortionView.updateSpectrum();
        }
        const auto analyzer = UserSettings::analyzer();
        applyEqAnalyzer(analyzer);
        if (eqView.isVisible())
            eqView.updateSpectrum();
        for (auto &button : distortionButtons)
            if (button->isVisible())
                button->repaint();
        auto now = Time::getMillisecondCounter();
        if (ThemeSettings::scanDue(now, lastThemeScan)) {
            ThemeSettings::scan();
        }
        processor.serviceAssets();
        if (macroStamp != processor.macroRevision.load()) {
            macroStamp = processor.macroRevision.load();
            for (int i = 0; i < 4; ++i)
                if (!macroNames[size_t(i)].hasKeyboardFocus(true))
                    macroNames[size_t(i)].setText(processor.macroName(i), false);
        }
        if (rack == 10)
            undoButton.setEnabled(processor.canUndoMutation());
        auto status = processor.sourceStatus();
        if (status != lastStatus) {
            lastStatus = status;
            if (status.isNotEmpty())
                readout = status;
            repaint(120, getHeight() - 16, 870, 16);
        }
        for (int i = 0; i < playerCount; ++i) {
            auto data = processor.sourceData(i);
            if (displayedRevisions[size_t(i)] != data->revision) {
                displayedRevisions[size_t(i)] = data->revision;
                if (i / 2 == selectedPart)
                    repaint(i % 2 ? 764 : 16, 82, 460, 184);
            }
        }
        if (presetRevision != processor.presetRevision.load()) {
            refreshPresets();
            syncPresetNotes();
            int scope = jlimit(0, 3, int(processor.state.state.getProperty("uiScope", 2)));
            scopeView.setSource(scope);
            for (int i = 0; i < 4; ++i)
                scopeSources[size_t(i)].setToggleState(i == scope, dontSendNotification);
            patternSlot.setSelectedId(processor.activePattern() + 1, dontSendNotification);
        }
        if (userPreferenceRevision != UserSettings::revision()) {
            const auto next = UserSettings::text();
            if (next.size != typography.size || next.style != typography.style ||
                next.antialias != typography.antialias || next.quality != typography.quality ||
                next.edgeWeight != typography.edgeWeight) {
                applyThemePalette(draftTheme);
                layout();
                repaint();
            }
            userPreferenceRevision = UserSettings::revision();
        }
        if (preferenceRevision != ThemeSettings::revision())
            setTheme(ThemeSettings::current(), false);
        if (commentStamp != processor.commentRevision.load()) {
            if (presetNotes.getText() != processor.presetComment())
                presetNotes.setText(processor.presetComment(), false);
            commentStamp = processor.commentRevision.load();
        }
        if (rack == 9) {
            waveView.setMode(
                int(value(Param(playerParamIndex(Param::o1engine, waveView.selectedSlot(), 16)))));
            waveView.repaint();
            if (sampleFields[1] && sampleFields[1]->isVisible())
                sampleFields[1]->showPlaybackPosition(waveView.playbackPosition());
        }
        for (int slot = 0; slot < playerCount; ++slot) {
            auto picture = processor.playback[size_t(slot)].read();
            for (auto pair :
                 {std::pair<Param, float>{Param(playerParamIndex(Param::o1fmdepth, slot, 10)),
                                          picture.fm},
                  {Param(playerParamIndex(Param::o1amdepth, slot, 10)), picture.am},
                  {Param(playerParamIndex(Param::o1pulsewidth, slot, 10)), picture.width}})
                controls[size_t(pair.first)]->liveValue(pair.second, picture.active);
        }
        if (rack == 4)
            eqView.repaint();
        if (rack == 8)
            refreshChord();
        if (currentLayoutState() != layoutStamp)
            layout();
        for (int i = 0; i < playerCount; ++i)
            controls[size_t(Param(playerParamIndex(Param::o1phase, i, 12)))]->enable(
                value(Param(playerParamIndex(Param::o1engine, i, 16))) <= 1);
        controls[size_t(Param::polyphony)]->enable(value(Param::monolegato) == 0);
        if (!pitch.isMouseButtonDown())
            pitch.setValue(processor.displayedBend(), dontSendNotification);
        if (!mod.isMouseButtonDown())
            mod.setValue(processor.displayedMod(), dontSendNotification);
        if (rack == 0) {
            repaint(414, 350, 520, 165);
            patternCanvas.repaint();
        }
        if (rack == 8)
            repaint(414, 366, 520, 145);
        for (int slot = selectedPart * 2; slot < selectedPart * 2 + 2; ++slot)
            if (sourceViews[size_t(slot)].getSelectedId() == 2)
                repaint(slot % 2 ? 850 : 102, 196, 358, 68);
        repaint(956, 278, 268, 258);
    }
    void paint(Graphics &g) override {
        if (backgroundStyle == 0)
            g.fillAll(body);
        else {
            g.setGradientFill(ColourGradient(
                backgroundStyle == 1 ? body.darker(float(.3 * gradientAmount)) : body, 0, 0,
                backgroundStyle == 1 ? body.interpolatedWith(green, float(.45 * gradientAmount))
                                     : panel,
                float(getWidth()), float(getHeight()), false));
            g.fillAll();
        }
        if (backgroundStyle >= 3 && skinImage.isValid()) {
            g.setOpacity(float(backgroundMix));
            g.drawImageWithin(skinImage, 0, 0, getWidth(), getHeight(),
                              RectanglePlacement::fillDestination);
            g.setOpacity(1);
        }
        if ((backgroundStyle == 2 || backgroundStyle == 4) && gridOpacity > 0) {
            g.setColour(readableColour(line, body, panel, 4.5).withAlpha(float(gridOpacity)));
            for (int x = 24; x < getWidth(); x += 24)
                g.drawVerticalLine(x, 0, float(getHeight()));
            for (int y = 24; y < getHeight(); y += 24)
                g.drawHorizontalLine(y, 0, float(getWidth()));
        }
        if (skin == "ap-flexin") {
            for (int i = 0; i < 48; ++i) {
                auto colour = i % 3 == 0   ? Colour(0xff5352ff)
                              : i % 3 == 1 ? Colour(0xff00d9ee)
                                           : Colour(0xffb63cff);
                g.setColour(colour.withAlpha(.16f));
                float x = float((i * 157) % 1240), y = float(75 + (i * 79) % 765);
                g.drawLine(x, y, x + 12 + (i % 5) * 9, y, 1);
            }
        }
        g.setColour(green);
        if (skin == "ap-flexin") {
            g.setImageResamplingQuality(Graphics::highResamplingQuality);
            g.drawImageWithin(apLogo, 17, 3, 96, 51, RectanglePlacement::centred);
            drawCaption(g, apCaption, {10, 55, 110, 20}, 10.5f, ink, Justification::centred, true);
        } else {
            if (headerLogo)
                headerLogo->drawWithin(g, {20, 10, 54, 54}, RectanglePlacement::centred, 1);
        }
        drawCaption(g, "SOUND LIBRARY", {364, 9, 280, 13}, 8, muted);
        drawCaption(g, "PART", {795, 30, 33, 20}, 8.5f, muted);
        drawCaption(g, "SOLO", {927, 12, 68, 13}, 8, muted);
        drawCaption(g, "OPTIONS", {1162, 12, 62, 13}, 8, muted);
        String part = String::charToString(juce_wchar('A' + selectedPart));
        shell(g, {16, 82, 460, 184}, "OSCILLATOR " + part + "1", {});
        shell(g, {764, 82, 460, 184}, "OSCILLATOR " + part + "2", {});
        shell(g, {486, 82, 268, 184}, "AMP ENVELOPE " + part, "ADSR");
        for (int slot = selectedPart * 2; slot < selectedPart * 2 + 2; ++slot)
            if (sourceViews[size_t(slot)].getSelectedId() == 2) {
                auto data = processor.sourceData(slot);
                int x = slot % 2 ? 850 : 102;
                const int mode = int(value(Param(playerParamIndex(Param::o1engine, slot, 16))));
                auto telemetry = processor.playback[size_t(slot)].read();
                float position = float(value(Param(playerParamIndex(Param::o1position, slot, 16))));
                if (telemetry.active && telemetry.revision == data->revision && mode == 1)
                    position = telemetry.position;
                Path path;
                for (int i = 0; i < 340; ++i) {
                    float sample =
                        mode == 1 ? data->frameSample(data->frameIndex(position), double(i) / 340)
                                  : data->sample(
                                        double(i) * double(data->samples[0].size() - 2) / 340, 0);
                    float y = 204 - jlimit(-1.f, 1.f, sample) * 6;
                    if (i == 0)
                        path.startNewSubPath(float(x + i), y);
                    else
                        path.lineTo(float(x + i), y);
                }
                g.setColour(green.withAlpha(.6f));
                g.strokePath(path, PathStrokeType(1));
                drawCaption(g,
                            String(data->name) +
                                (mode == 1 ? "  /  " + String(data->frameIndex(position) + 1) +
                                                 " / " + String(data->frames)
                                           : String{}),
                            {x, 248, 340, 14}, 8, muted);
            }
        auto curveBounds = Rectangle<float>(508, 116, 224, 22);
        size_t ampBase = partParamIndex(Param::p1ampa, selectedPart, 10);
        auto points = envelopePlot({value(Param(ampBase)), value(Param(ampBase + 1)),
                                    value(Param(ampBase + 2)), value(Param(ampBase + 3))});
        Path curve;
        for (size_t i = 0; i < points.size(); ++i) {
            float x = curveBounds.getX() + points[i].x * curveBounds.getWidth();
            float y = curveBounds.getBottom() - points[i].level * curveBounds.getHeight();
            if (i == 0)
                curve.startNewSubPath(x, y);
            else
                curve.lineTo(x, y);
        }
        g.setColour(green.withAlpha(.7f));
        g.strokePath(curve, PathStrokeType(1.5f));
        shell(g, {16, 278, 252, 144}, "FILTER " + part, "INPUT / TYPE / SLOPE");
        shell(g, {16, 434, 252, 102}, "FILTER CONTROL", "A+B+C");
        shell(g, {956, 278, 268, 258}, "MIXER",
              "OUT / " + String(processor.voices.load()) + " VOICES");
        for (int i = 0; i < 2; ++i) {
            int x = 1162 + i * 23;
            drawCaption(g, i == 0 ? "L" : "R", {x, 428, 15, 14}, 8, muted, Justification::centred);
            float level = processor.channelPeaks[size_t(i)].load();
            float db = 20 * std::log10(jmax(.00001f, level));
            int lit = jlimit(0, 16, int((db + 48) / 3));
            for (int j = 0; j < 16; ++j) {
                g.setColour(j < lit     ? (level > 1 && j >= 14 ? Colour(0xffd35f55)
                                           : j > 13             ? Colour(0xffbb7c46)
                                                                : green)
                            : darkTheme ? line
                                        : Colour(0xffdee2d8));
                g.fillRoundedRectangle(float(x + 3), float(506 - j * 4), 9, 3, 1);
            }
        }

        float outputLevel = processor.peak.load();
        float outputDb = outputLevel > 0 ? 20 * std::log10(outputLevel) : -100;
        if (std::abs(outputDb) < .05f)
            outputDb = 0;
        drawCaption(g, outputLevel > 0 ? String(outputDb, 1) + " dBFS" : "-inf dBFS",
                    {1140, 517, 76, 16}, 9, outputLevel > 1 ? Colour(0xffd35f55) : muted,
                    Justification::centred);

        if (backgroundStyle == 1)
            g.setGradientFill(ColourGradient(
                screen.darker(float(.2 * gradientAmount)).withAlpha(float(panelOpacity)), 280, 278,
                screen.interpolatedWith(green, float(.16 * gradientAmount))
                    .withAlpha(float(panelOpacity)),
                948, 536, false));
        else
            g.setColour(backgroundStyle == 0 ? screen : screen.withAlpha(float(panelOpacity)));
        g.fillRoundedRectangle(Rectangle<float>(280, 278, 668, 258), 8);
        g.setColour(screenLine);
        g.drawRoundedRectangle(Rectangle<float>(280.5f, 278.5f, 667, 257), 8, 1);
        g.drawVerticalLine(404, 292, 523);
        drawCaption(g, "GENERATORS", {292, 281, 108, 13}, 8, screenText.withAlpha(.65f));
        drawCaption(g, "AUDIO FX / DRAG", {292, 317, 108, 13}, 8, screenText.withAlpha(.65f));
        g.setColour(screenLine);
        g.drawHorizontalLine(513, 292, 399);
        drawCaption(g,
                    !settingsOpen && rack == 1 ? String("DISTORTION")
                    : rackHeadingVisible()     ? rackNames[rack]
                                               : String{},
                    {414, 289, rack == 1 ? 100 : 350, 20}, 12, screenText,
                    Justification::centredLeft, true);

        if (settingsOpen) {
            // The settings labels and selector draw inside the central rack.
        } else if (rack == 10) {
            drawCaption(g, "TARGET", {414, 398, 258, 12}, 8, muted);
            drawCaption(g, "FROM", {682, 398, 115, 12}, 8, muted);
            drawCaption(g, "TO", {807, 398, 126, 12}, 8, muted);
        } else if (rack == 9) {
            int mode =
                int(value(Param(playerParamIndex(Param::o1engine, waveView.selectedSlot(), 16))));
            String title = mode == 1 ? "WAVETABLE / WT POS" : "SAMPLE / PLAYBACK";
            if (mode == 2) {
                auto parameter =
                    Param(playerParamIndex(Param::o1stretchmode, waveView.selectedSlot(), 10));
                auto names =
                    StringArray::fromTokens(parameterSpecs[size_t(parameter)].choices, "|", "");
                title = "SAMPLE / " + names[int(value(parameter))].toUpperCase();
                const unsigned layers = processor.samplerLayerMask(waveView.selectedSlot());
                if (layers == (1u << phaseDiffusionMode))
                    title = "SAMPLE / PaulXStretch";
                else if (layers)
                    title = "SAMPLE / STRETCH LAYERS";
            }
            const Rectangle<int> titleArea =
                mode >= 2 ? Rectangle<int>(491, 289, 166, 23) : Rectangle<int>(540, 289, 295, 23);
            drawCaption(g, title, titleArea, 11, screenText);
            if (sampleFields[1] && sampleFields[1]->isVisible()) {
                constexpr std::array<const char *, 3> captions{"START", "POSITION", "END"};
                for (int i = 0; i < int(captions.size()); ++i)
                    drawCaption(g, captions[size_t(i)], sampleFieldBounds(i).withY(285).withHeight(12),
                                8, mint, Justification::centred);
            }
        } else if (rack == 8) {
            drawCaption(g, "TRIGGER", {414, 309, 140, 18}, 9, mint);
            drawCaption(g, "VOICING", {566, 309, 145, 18}, 9, mint);
            drawCaption(g,
                        processor.learning() >= 0 ? "Play a chord, then choose DONE"
                                                  : "Each trigger stores up to eight notes",
                        {414, 371, 510, 24}, 12, screenText);
            for (int i = 0; i < 8; ++i)
                drawCaption(g, "VOICE " + String(i + 1), {414 + i * 65, 446, 58, 18}, 8, mint);
            drawCaption(g, "CHORDS > ARPEGGIATOR > SYNTH / MIDI OUT", {414, 490, 520, 24}, 10,
                        mint.withAlpha(.65f));
        } else if (rack == 0 && value(Param::arppattern) > 0) {
            drawCaption(g, "SWING", {600, 291, 47, 18}, 8, mint);
            drawCaption(g, "LANE / LEN", {414, 345, 87, 12}, 7, mint.withAlpha(.65f));
            StringArray labels{"NOTE", "OCTAVE", "VELOCITY", "GATE", "REPEAT", "CHANCE", "TIE"};
            for (int row = 0; row < 7; ++row)
                drawCaption(g, labels[row], {414, 359 + row * 22, 51, 19}, 8, mint);
            for (int column = 0; column < 16; ++column)
                drawCaption(g, String(stepPage * 16 + column + 1), {509 + column * 26, 345, 24, 12},
                            7, mint.withAlpha(.65f), Justification::centred);
        } else if (rack == 0) {
            for (int i = 0; i < 8; ++i) {
                int x = 459 + i * 59;
                int n = stepPage * 8 + i + 1;
                drawCaption(g, String(n).paddedLeft('0', 2), {x, 408, 50, 17}, 8,
                            mint.withAlpha(.6f), Justification::centred);
            }
            drawCaption(g, "PITCH", {414, 432, 37, 14}, 7.5f, mint.withAlpha(.65f));
            drawCaption(g, "VELOCITY", {414, 460, 37, 14}, 7.5f, mint.withAlpha(.65f));
            drawCaption(g, "TRANSPOSE  /  VELOCITY  /  HOLD", {414, 512, 520, 14}, 7,
                        mint.withAlpha(.38f), Justification::centredRight);
        }
        if (!modulationCollapsed) {
            shell(g, {16, 548, 152, 208}, "MOD ENV 1");
            shell(g, {176, 548, 152, 208}, "MOD ENV 2");
            shell(g, {336, 548, 200, 208}, "LFO 1");
            shell(g, {544, 548, 200, 208}, "LFO 2");
            shell(g, {752, 548, 232, 208}, "MISC 1");
            shell(g, {992, 548, 232, 208}, "MISC 2");
            g.setColour(line);
            g.drawHorizontalLine(656, 762, 974);
            g.drawHorizontalLine(656, 1002, 1214);
        }
        const int lowerOffset = modulationCollapsed ? EditorLayout::modulationHeight : 0;
        shell(g, {16, 768 - lowerOffset, 90, 76}, {});
        shell(g, {990, 768 - lowerOffset, 234, 76}, {});
        drawCaption(g, "BEND", {25, 769 - lowerOffset, 29, 13}, 7, muted, Justification::centred);
        drawCaption(g, "MOD", {60, 769 - lowerOffset, 29, 13}, 7, muted, Justification::centred);
        drawCaption(g, "GLIDE", {1060, 818 - lowerOffset, 53, 16}, 8, muted,
                    Justification::centredRight);
        drawCaption(g, readout, {120, 846 - lowerOffset, 718, 12}, 8, muted);
        drawCaption(g, "wavelogik / logisth / polyphonic instrument",
                    {854, 846 - lowerOffset, 370, 12}, 7.5f, muted, Justification::centredRight);
    }
    LogisthProcessor &processor;
    LogisthLook &look;
    std::array<std::unique_ptr<Control>, parameterCount> controls;
    std::array<ToggleButton, partCount> parts;
    std::array<ToggleButton, 2> stepPages;
    std::array<std::unique_ptr<RackButton>, 14> rackButtons;
    std::array<ComboBox, playerCount> sourceViews, sourceFrame, stretchAlgorithms;
    std::array<unsigned, playerCount> stretchPickerMasks{};
    std::array<int, playerCount> sourceMenuStates{-1, -1, -1, -1, -1, -1};
    std::array<TextButton, playerCount> sourceLoad, sourceClear, oscCopy, oscPaste;
    TextButton impulseLoad, impulseReset;
    Label impulseName;
    std::unique_ptr<FileChooser> impulseChooser;
    ComboBox patternSlot, chordTrigger, chordType;
    TextButton patternSave, patternRandom, chordLearn, chordCopy;
    std::array<Slider, 8> chordNotes;
    bool updatingChord = false;
    std::unique_ptr<FileChooser> sourceChooser;
    int previewSource = -1, previewPattern = -1, previewGran = -1, previewReverb = -1,
        previewEq = -1, previewWave = -1, previewStretch = -1;
    ComboBox presets;
    SearchButton searchButton{*this};
    SearchPanel searchPanel{*this};
    TextEditor searchQuery;
    ListBox searchResults;
    Label searchEmpty;
    std::vector<PresetMatch> matches;
    ToggleButton settings;
    TextButton modulationToggle;
    bool modulationCollapsed = false;
    TextEditor presetNotes;
    Component settingsPanel;
    ComboBox themeChoice;
    Label themeLabel, themeHelp, aboutLabel, scaleLabel, nameLabel, roleLabel, hexLabel;
    TextEditor aboutInfo;
    ToggleButton fixedVelocity;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> fixedVelocityAttachment;
    ComboBox scaleChoice, colourRole, themeFont, themeBackground, themeKnobs, backgroundFactory;
    Label fontLabel, backgroundLabel, knobLabel;
    std::unique_ptr<FileChooser> themeChooser;
    TextButton themeSaveAs, themeRename, themeChooseFolder;
    TextEditor themeName, colourHex;
    ToggleButton themeDark;
    TextButton themeEdit, themeScan, themeFolder, themeSave, themeReset;
    std::array<TextButton, 7> settingsTabs;
    ComboBox textSize, textWeight, textQuality;
    Slider textEdge, backgroundMixControl, panelOpacityControl, gradientControl, gridControl;
    Label textEdgeLabel, backgroundName, backgroundMixLabel, panelOpacityLabel, gradientLabel,
        gridLabel;
    TextButton backgroundLoad, backgroundClear;
    Image skinImage;
    String skinImageData;
    std::unique_ptr<FileChooser> backgroundChooser, folderChooser;
    ToggleButton textAA;
    Label textSizeLabel, textWeightLabel, textReference, settingsHelp;
    std::array<Label, 5> folderLabels, folderValues;
    std::array<TextButton, 5> folderChoose, folderOpen;
    ColourSelector colourPicker{ColourSelector::showColourspace | ColourSelector::showColourAtTop};
    ThemeDefinition draftTheme, savedTheme;
    int settingsTab = 0;
    uint32 lastThemeScan = 0;
    bool settingsOpen = false;
    uint64_t preferenceRevision = 0, userPreferenceRevision = 0;
    int commentStamp = -1, notesRevision = -1;
    TextButton previous, next;
    Keys keyboard;
    PatternCanvas patternCanvas;
    logisth_ui::WaveView waveView;
    std::array<std::unique_ptr<Control>, 3> sampleFields;
    logisth_ui::ScopeView scopeView;
    logisth_ui::ImpulseView impulseView;
    logisth_ui::DynamicsView dynamicsView;
    logisth_ui::EqView eqView;
    logisth_ui::DistortionView distortionView;
    ComboBox distAnalysis, distTilt;
    int previewDistortion = -1, previewDistortionBands = -1;
    ComboBox eqBandSelect;
    TextButton eqAdd, eqRemove;
    int selectedEqBand = 0, selectedDistortion = 0;
    std::array<std::unique_ptr<DistortionButton>, 4> distortionButtons;
    Label distortionHelp;
    TextButton eqPre, eqPost;
    ComboBox eqTilt;
    Label eqAnalysisLabel, eqTiltLabel;
    std::array<logisth_ui::ClickArea, playerCount> sourceWave;
    std::array<ToggleButton, 4> scopeSources;
    TextButton presetFiles;
    ComboBox waveSlot;
    std::array<TextEditor, 4> macroNames;
    std::array<TextButton, 4> macroSelect, mutationLock;
    TextButton mutateButton, undoButton;
    int selectedMacro = 0, macroStamp = -1;
    static constexpr const char *apCaption = "AP always flexin'!";
    ImageComponent aboutLogo;
    Label aboutSkinName;
    BrandLogoView brandAboutLogo;
    std::unique_ptr<Drawable> headerLogo;
    Image apLogo = ImageCache::getFromMemory(BinaryData::apmusicgroupwhite_png,
                                             BinaryData::apmusicgroupwhite_pngSize);

    std::unique_ptr<FileChooser> presetChooser;
    Array<File> userPresets;
    int presetRevision = -1;
    struct BendWheel final : Slider {
        void stoppedDragging() override { setValue(0, sendNotificationSync); }
    } pitch;
    Slider mod;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> pitchAttachment, modAttachment;
    TooltipWindow tooltips;
    String readout, lastStatus;
    std::array<uint64_t, playerCount> displayedRevisions{};
    std::array<int64_t, 14 + 4 * playerCount> layoutStamp{};
    int selectedPart = 0, rack = 9, stepPage = 0, previewSync = -1;
};
LogisthEditor::LogisthEditor(LogisthProcessor &p) : AudioProcessorEditor(p) {
    setLookAndFeel(&look);
    console = std::make_unique<Console>(p, look);
    addAndMakeVisible(*console);
    setResizable(true, true);

    console->onScaleRequested = [this](double percent) { setUIScale(percent); };
    console->onModulationRequested = [this](bool collapsed) { setModulationCollapsed(collapsed); };
    auto saved = ThemeSettings::window();
    console->setModulationCollapsed(saved.modulationCollapsed);
    const int logicalHeight = EditorLayout::height(saved.modulationCollapsed);
    getConstrainer()->setFixedAspectRatio(double(canvasWidth) / logicalHeight);
    setResizeLimits(992, roundToInt(logicalHeight * .8), 2480, logicalHeight * 2);
    if (saved.width > 0) {
        scalePercent = saved.scale;
        setSize(saved.width, saved.height);
    } else {
        float scale = 1.f;
        if (auto *display = Desktop::getInstance().getDisplays().getPrimaryDisplay())
            scale = jlimit(.8f, 1.f,
                           jmin(float(display->userArea.getWidth() - 40) / canvasWidth,
                                float(display->userArea.getHeight() - 40) / logicalHeight));
        scalePercent = scale * 100;
        setSize(roundToInt(canvasWidth * scale), roundToInt(logicalHeight * scale));
    }
    console->displayScale(scalePercent);
    rememberSize = true;
    sizeDirty = saved.width == 0;
    if (sizeDirty)
        startTimer(300);
}
LogisthEditor::~LogisthEditor() {
    stopTimer();
    if (rememberSize && sizeDirty)
        ThemeSettings::saveWindow(getWidth(), getHeight(), scalePercent, isModulationCollapsed());
    console.reset();
    setLookAndFeel(nullptr);
}
void LogisthEditor::resized() {
    if (rememberSize && !applyingScale)
        scalePercent = std::round(double(getWidth()) * 10000 / canvasWidth) / 100;
    if (console) {
        console->displayScale(scalePercent);
        const int logicalHeight = EditorLayout::height(isModulationCollapsed());
        console->setBounds(0, 0, canvasWidth, logicalHeight);
        const auto scale =
            jmin(float(getWidth()) / canvasWidth, float(getHeight()) / logicalHeight);
        console->setTransform(AffineTransform::scale(scale));
    }
    if (rememberSize) {
        sizeDirty = true;
        startTimer(300);
    }
}
void LogisthEditor::timerCallback() {
    stopTimer();
    if (sizeDirty) {
        ThemeSettings::saveWindow(getWidth(), getHeight(), scalePercent, isModulationCollapsed());
        sizeDirty = false;
    }
}
void LogisthEditor::setUIScale(double percent) {
    if (!std::isfinite(percent) || percent < 80 || percent > 200) {
        console->displayScale(scalePercent);
        return;
    }
    scalePercent = std::round(percent * 100) / 100;
    applyingScale = true;
    setSize(roundToInt(canvasWidth * scalePercent / 100),
            roundToInt(EditorLayout::height(isModulationCollapsed()) * scalePercent / 100));
    applyingScale = false;
    console->displayScale(scalePercent);
    sizeDirty = true;
    timerCallback();
}
int LogisthEditor::getControlParameterIndex(Component &component) {
    auto &state = static_cast<LogisthProcessor &>(processor).state;
    for (auto *node = &component; node != nullptr && node != this;
         node = node->getParentComponent())
        if (auto id = node->getProperties()["hostParameterID"].toString(); id.isNotEmpty())
            if (auto *parameter = state.getParameter(id))
                return parameter->getParameterIndex();
    return AudioProcessorEditor::getControlParameterIndex(component);
}
Image LogisthEditor::snapshotPage(int scenario) {
    console->setDarkTheme(scenario >= 4 && scenario != 7 && scenario != 13, false);
    console->showScenario(scenario);
    return createComponentSnapshot(getLocalBounds());
}
Image LogisthEditor::snapshotTheme(int selected) {
    auto draft = console->currentDraft();
    int oldTheme = console->themeId;
    bool oldSettings = console->showingSettings();
    int oldPage = console->settingsPage();
    console->setTheme(selected, false);
    console->showSettingsPage(0);
    console->showSettings(true);
    auto result = createComponentSnapshot(getLocalBounds());
    console->setTheme(oldTheme, false);
    console->showSettings(oldSettings);
    console->showSettingsPage(oldPage);
    console->restoreDraft(draft);
    return result;
}
String LogisthEditor::validateLayout() {
    return console->validate();
}

void LogisthEditor::setDarkTheme(bool enabled) {
    console->setDarkTheme(enabled);
}
void LogisthEditor::setTheme(int selected) {
    console->setTheme(selected);
}
int LogisthEditor::selectedTheme() const {
    return console->themeId;
}
bool LogisthEditor::isDarkTheme() const {
    return console->isDarkTheme();
}

Result LogisthEditor::importBackground(const File &file) {
    return console->importBackground(file);
}
bool LogisthEditor::setBrowserFolder(UserSettings::Folder kind, const File &folder) {
    return console->setBrowserFolder(kind, folder);
}

bool LogisthEditor::isModulationCollapsed() const {
    return console->isModulationCollapsed();
}
void LogisthEditor::setModulationCollapsed(bool collapsed) {
    if (collapsed == isModulationCollapsed())
        return;
    const int logicalHeight = EditorLayout::height(collapsed);
    applyingScale = true;
    console->setModulationCollapsed(collapsed);
    getConstrainer()->setFixedAspectRatio(double(canvasWidth) / logicalHeight);
    setResizeLimits(992, roundToInt(logicalHeight * .8), 2480, logicalHeight * 2);
    setSize(roundToInt(canvasWidth * scalePercent / 100),
            roundToInt(logicalHeight * scalePercent / 100));
    applyingScale = false;
}
