#!/usr/bin/env python3
"""Render the technical reference with a cover, linked contents and PDF bookmarks.

Requires ReportLab. The default user-manual command also invokes this renderer.
"""
import hashlib
import json
from pathlib import Path
import re
from xml.sax.saxutils import escape

from reportlab.graphics import renderSVG
from reportlab.graphics.shapes import Drawing, Line, Polygon, Rect, String
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate, Frame, PageBreak, PageTemplate, Paragraph, Preformatted,
    Spacer, Table, TableStyle,
)
from reportlab.platypus.tableofcontents import TableOfContents

from verify_technical_manual import implementation_hash, text_hash, verify


def signal_flow():
    drawing = Drawing(468, 280)
    ink, accent = colors.HexColor('#183752'), colors.HexColor('#097da2')
    def box(x, y, width, heading, detail):
        drawing.add(Rect(x, y, width, 43, rx=5, fillColor=colors.HexColor('#eef5fa'),
                         strokeColor=colors.HexColor('#b5c8d8'), strokeWidth=.7))
        drawing.add(String(x + width / 2, y + 26, heading, fontName='OpenSans-Bold',
                           fontSize=8.5, textAnchor='middle', fillColor=ink))
        drawing.add(String(x + width / 2, y + 11, detail, fontName='OpenSans',
                           fontSize=7, textAnchor='middle', fillColor=ink))
    def arrow(x1, y1, x2, y2):
        drawing.add(Line(x1, y1, x2, y2, strokeColor=accent, strokeWidth=1))
        if y2 < y1:
            points = [x2, y2, x2 - 3, y2 + 5, x2 + 3, y2 + 5]
        else:
            points = [x2, y2, x2 - 5, y2 - 3, x2 - 5, y2 + 3]
        drawing.add(Polygon(points, fillColor=accent, strokeColor=accent))
    box(0, 231, 142, 'MIDI / KEYBOARD', 'notes, controllers, host clock')
    box(163, 231, 142, 'CHORDS', 'key mapping / reference counts')
    box(326, 231, 142, 'ARPEGGIATOR', 'Legacy / Pattern; MIDI output')
    arrow(142, 252, 163, 252)
    arrow(305, 252, 326, 252)
    arrow(397, 231, 397, 208)
    box(0, 157, 468, 'PER-NOTE SYNTHESIS: A1/A2, B1/B2, C1/C2',
        'Classic / Wavetable / Sampler / Granular; envelopes, LFOs, FM/AM/PWM')
    drawing.add(Line(397, 208, 397, 200, strokeColor=accent))
    arrow(234, 157, 234, 141)
    box(0, 96, 468, 'ROUTING -> FILTER A/B/C -> AMP / VELOCITY / MIX -> SUM',
        'select source-bus combinations; per-note stereo filters and part envelopes')
    arrow(234, 96, 234, 80)
    box(0, 35, 468, 'DISTORTION -> PHASER -> CHORUS -> EQ -> REVERB -> DELAY',
        'shared stereo effects; independent bypass and prepared tails')
    arrow(234, 35, 234, 20)
    drawing.add(String(234, 5, 'COMPRESSOR -> CLIPPER -> MAIN -> LIMITER -> STEREO OUTPUT',
                       fontName='OpenSans-Bold', fontSize=8, textAnchor='middle', fillColor=ink))
    scale = (A4[0] - 84) / 468
    drawing.scale(scale, scale)
    drawing.width *= scale
    drawing.height *= scale
    return drawing


def inline(text, root):
    # Escape text first, then add only the small supported markup vocabulary.
    text = escape(text)
    def link(match):
        label, target = match.groups()
        if target.startswith(('https://', 'http://')):
            url = target
        else:
            path = (root / 'docs' / target).resolve().relative_to(root).as_posix()
            url = 'https://github.com/logik93/logisth/blob/master/' + path
        return f'<link href="{url}" color="#087a9c">{label}</link>'
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', link, text)
    text = re.sub(r'\*\*(.+?)\*\*', r'<b>\1</b>', text)
    return re.sub(r'`([^`]+)`', r'<font name="Courier">\1</font>', text)


class TechnicalDocument(BaseDocTemplate):
    def afterFlowable(self, flowable):
        if (isinstance(flowable, Paragraph) and flowable.style.name in ('Section', 'Subsection')
                and flowable.getPlainText() != 'Contents'):
            level = 0 if flowable.style.name == 'Section' else 1
            title = flowable.getPlainText()
            key = 'section-' + hashlib.sha256(title.encode()).hexdigest()[:16]
            self.canv.bookmarkPage(key)
            self.canv.addOutlineEntry(title, key, level=level, closed=False)
            self.notify('TOCEntry', (level, title, self.page, key))


def render(root):
    root = Path(root)
    markdown = root / 'docs/TECHNICAL_MANUAL.md'
    source = markdown.read_text(encoding='utf-8')
    version = re.match(r'# logisth ([\d.]+) — Technical Manual', source).group(1)
    revision = re.search(r'Document revision: ([\d.]+)', source).group(1)
    pdf = root / 'docs/logisth-Technical-Manual-English.pdf'
    for name, file in [('OpenSans', 'OpenSans-Regular.ttf'),
                       ('OpenSans-Bold', 'OpenSans-Semibold.ttf')]:
        pdfmetrics.registerFont(TTFont(name, str(root / 'assets/fonts' / file)))
    pdfmetrics.registerFontFamily('OpenSans', normal='OpenSans', bold='OpenSans-Bold')
    body = ParagraphStyle('Body', fontName='OpenSans', fontSize=9, leading=14,
                          spaceAfter=8, textColor=colors.HexColor('#203448'))
    section = ParagraphStyle('Section', parent=body, fontName='OpenSans-Bold', fontSize=16,
                             leading=21, spaceBefore=14, spaceAfter=12, keepWithNext=True)
    subsection = ParagraphStyle('Subsection', parent=section, fontSize=11, leading=16)
    cell = ParagraphStyle('Cell', parent=body, fontSize=7.7, leading=11, spaceAfter=0)
    code = ParagraphStyle('Code', fontName='Courier', fontSize=7, leading=10,
                          spaceBefore=5, spaceAfter=12, leftIndent=8,
                          backColor=colors.HexColor('#f0f5f8'), borderPadding=8)
    cover = ParagraphStyle('Cover', parent=section, fontSize=34, leading=44,
                           textColor=colors.HexColor('#183752'), alignment=TA_CENTER)
    label = ParagraphStyle('Label', parent=body, alignment=TA_CENTER, fontSize=12, leading=19)
    drawing = signal_flow()
    renderSVG.drawToFile(drawing, str(root / 'docs/images/logisth-signal-flow.svg'))
    story = [Spacer(1, 45), Paragraph('wavelogik', label), Spacer(1, 24),
             Paragraph('logisth', cover), Paragraph('TECHNICAL MANUAL', label),
             Paragraph(f'Software baseline {version} | Document revision {revision}', label),
             Spacer(1, 28), drawing, Spacer(1, 20),
             Paragraph('Architecture · Digital signal processing · Dependencies<br/>'
                       'Data formats · Validation · Release engineering', label), PageBreak()]
    contents = TableOfContents()
    contents.levelStyles = [ParagraphStyle('ContentsEntry', parent=body, leading=16, spaceAfter=5),
                            ParagraphStyle('ContentsSub', parent=body, leftIndent=12, fontSize=8)]
    story.extend([Paragraph('Contents', section), contents, PageBreak()])
    paragraphs, rows, code_lines = [], [], []
    in_code, in_contents = False, False
    def flush_paragraph():
        if paragraphs:
            story.append(Paragraph(inline(' '.join(paragraphs), root), body))
            paragraphs.clear()
    def flush_table():
        if not rows:
            return
        count = len(rows[0])
        widths = [133, 335] if count == 2 else [117, 167, 184]
        widths = [value * (A4[0] - 84) / 468 for value in widths]
        table = Table([[Paragraph(inline(value, root), cell) for value in row] for row in rows],
                      colWidths=widths, repeatRows=1, hAlign='LEFT')
        table.setStyle(TableStyle([
            ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#deebf4')),
            ('ROWBACKGROUNDS', (0, 1), (-1, -1), [colors.white, colors.HexColor('#f7f9fb')]),
            ('LINEBELOW', (0, 0), (-1, 0), .6, colors.HexColor('#9eb7cb')),
            ('VALIGN', (0, 0), (-1, -1), 'TOP'),
            ('LEFTPADDING', (0, 0), (-1, -1), 7),
            ('RIGHTPADDING', (0, 0), (-1, -1), 7),
            ('TOPPADDING', (0, 0), (-1, -1), 6),
            ('BOTTOMPADDING', (0, 0), (-1, -1), 6),
        ]))
        story.extend([table, Spacer(1, 12)])
        rows.clear()
    # The generated cover replaces the Markdown title/producer line. Keep its
    # descriptive introduction and use a real linked PDF table of contents.
    for line in source.splitlines()[4:] + ['']:
        if line == '## Contents':
            flush_paragraph()
            in_contents = True
            continue
        if in_contents:
            if not line.startswith('## 1.'):
                continue
            in_contents = False
        if line.startswith('```'):
            flush_paragraph()
            flush_table()
            if in_code:
                story.append(Preformatted('\n'.join(code_lines), code))
                code_lines.clear()
            in_code = not in_code
            continue
        if in_code:
            code_lines.append(line)
            continue
        if line.startswith('|'):
            flush_paragraph()
            values = [value.strip() for value in line.strip('|').split('|')]
            if not all(re.fullmatch(r'[- :]+', value) for value in values):
                rows.append(values)
            continue
        flush_table()
        if not line.strip():
            flush_paragraph()
        elif line.startswith('## '):
            flush_paragraph()
            story.append(PageBreak())
            story.append(Paragraph(inline(line[3:], root), section))
        elif line.startswith('### '):
            flush_paragraph()
            story.append(Paragraph(inline(line[4:], root), subsection))
        elif line.startswith('!['):
            flush_paragraph()
            story.extend([signal_flow(), Spacer(1, 12)])
        else:
            paragraphs.append(line.strip())
    def page_frame(canvas, doc):
        canvas.saveState()
        canvas.setFont('OpenSans', 8)
        canvas.setFillColor(colors.HexColor('#60758a'))
        if doc.page > 1:
            canvas.drawString(42, A4[1] - 26, 'wavelogik / logisth — Technical Reference')
        canvas.drawString(42, 24, f'logisth {version} | English | Revision {revision}')
        canvas.drawRightString(A4[0] - 42, 24, str(doc.page))
        canvas.restoreState()
    document = TechnicalDocument(str(pdf), pagesize=A4, title=f'logisth {version} — Technical Manual',
                                 author='wavelogik', subject='Architecture, DSP and engineering reference',
                                 leftMargin=42, rightMargin=42, topMargin=48, bottomMargin=44,
                                 invariant=1)
    document.addPageTemplates(PageTemplate('Technical',
        frames=[Frame(42, 44, A4[0] - 84, A4[1] - 92, leftPadding=0, rightPadding=0,
                      topPadding=0, bottomPadding=0)], onPage=page_frame))
    document.multiBuild(story)
    record = {'format': 1, 'software_version': version,
              'implementation_sha256': implementation_hash(root),
              'markdown_sha256': text_hash(markdown),
              'pdf_sha256': hashlib.sha256(pdf.read_bytes()).hexdigest()}
    (root / 'docs/technical-manual.json').write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
    verify(root)
    print('Rendered docs/logisth-Technical-Manual-English.pdf and signal-flow diagram')


if __name__ == '__main__':
    render(Path(__file__).resolve().parents[1])
