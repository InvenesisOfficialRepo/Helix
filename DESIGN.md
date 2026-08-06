# Helix Design System Specification (`DESIGN.md`)

## 1. Visual Foundation & Philosophy
- **Register:** Scientific Workstation (Desktop Qt6 / QML)
- **Theme Paradigm:** Dynamic Multi-Theme with Dark Workstation as the lab default.
- **Grid & Alignment:** Strict 4px / 8px baseline grid. Compact vertical rhythm optimized for high-density data review.

---

## 2. Color System & Design Tokens

### 2.1 Brand & Semantic Accents
| Token | Default Hex | Role / Usage |
| :--- | :--- | :--- |
| `brandAccent` / `accent` | `#2fb3a6` | Primary interactive color (Teal / Cyan-Green), active tabs, focus rings, primary action buttons |
| `brandAccent2` / `accent2` | `#259689` | Secondary accent, sub-headers, active secondary states |
| `statusOk` / `ok` | `#4fb06a` | Success states, valid batches, passed QC checks, active connections |
| `statusWarn` / `warn` | `#d9a441` | Warnings, partial data, pending approvals, threshold alerts |
| `statusBad` / `bad` | `#d95757` | Errors, failed QC, missing files, disconnected states |

### 2.2 Surface & Canvas Hierarchy (Workstation Dark Default)
| Token | Dark Default | Light Default | Purpose |
| :--- | :--- | :--- | :--- |
| `surfaceWindow` / `bg` | `#2b2b2b` | `#bdbdbd` | Root application background |
| `surfacePanel` / `panel` | `#333333` | `#c9c9c9` | Main containers, sidebars, toolbars |
| `surfaceRaised` / `panel2` | `#3d3d3d` | `#d6d6d6` | Cards, elevated dialogs, floating menus |
| `surfaceHeader` | `#262626` | `#b0b0b0` | Table headers, section titles |
| `surfaceInput` | `#1c1c1c` | `#eef0f2` | Text inputs, spinboxes, search fields |
| `surfaceHover` | `#434343` | `#d0d0d0` | Hover feedback on interactive rows/cards |
| `surfaceActive` | `#4a4a4a` | `#c0c0c0` | Pressed / selected state background |

### 2.3 Categorical Series Palette (Microplates, Curves, Data Channels)
| Token | Hex | Usage |
| :--- | :--- | :--- |
| `s1` | `#5aa9e6` | Blue (Series 1 / Compound A) |
| `s2` | `#e0774c` | Orange (Series 2 / Compound B) |
| `s3` | `#5cc19b` | Green (Series 3 / Compound C) |
| `s4` | `#e8c04e` | Gold / Yellow (Series 4 / Compound D) |
| `s5` | `#b98ce0` | Purple (Series 5 / Compound E) |
| `s6` | `#4fc4c4` | Teal (Series 6 / Control 1) |
| `s7` | `#e58fb0` | Pink (Series 7 / Control 2) |
| `s8` | `#9aa7b4` | Slate (Series 8 / Reference) |

---

## 3. Typography Hierarchy

| Role | Font Family | Size (px) | Weight | Use Case |
| :--- | :--- | :--- | :--- | :--- |
| **Header 1 / Page Title** | Inter | `22px` (`fs2xl`) | Bold (700) | Main hub titles, window headers |
| **Header 2 / Section Title**| Inter | `18px` (`fsXl`) | Semi-Bold (600) | Dialog headers, section cards |
| **Header 3 / Group Title**  | Inter | `15px` (`fsLg`) | Semi-Bold (600) | Panel sub-headings, table group names |
| **Body / Labels (Default)** | Inter | `13px` (`fsMd`) | Regular (400) | Form labels, button text, body copy |
| **Compact / Meta Text**     | Inter | `12px` (`fsSm`) | Regular (400) | Table sub-labels, timestamps, captions |
| **Micro / Status Badges**   | Inter | `11px` (`fsXs`) | Medium (500) | Status chips, tags, badge counts |
| **Data / Numbers / Well IDs**| JetBrains Mono | `12px` / `13px` | Regular / Bold | Plate wells (`A01`..`P24`), numeric data, formulas, barcodes |
| **Icons**                   | Material Symbols Outlined | `16px` - `24px` | Regular | UI action and status icons |

---

## 4. Spacing Scale & Density

### Spacing Tokens
- `sp1`: 2px (micro offsets)
- `sp2`: 4px (tight element grouping)
- `sp3`: 6px (standard icon-to-text gap)
- `sp4`: 8px (standard element spacing)
- `sp5`: 10px
- `sp6`: 12px (form row spacing)
- `sp7`: 16px (card inner padding)
- `sp8`: 20px
- `sp9`: 24px (section margins)
- `sp10`: 32px (major layout dividers)
- `sp11`: 40px
- `sp12`: 48px

### Density Modes
- **Compact Density:** Control height = `22px`, horizontal padding = `8px`
- **Comfortable Density:** Control height = `26px`, horizontal padding = `10px`

### Border Radii
- `radiusXs`: 2px (table cell focus)
- `radiusSm`: 3px (subtle chips, inputs)
- `radiusMd`: 4px (standard buttons, text fields)
- `radiusLg`: 6px (cards, modal panels)
- `radiusPill`: 999px (circular status dots, pill badges)

---

## 5. Component & Layout Guidelines

1. **Plate & Matrix Visualizers:**
   - 96-well / 384-well grid cells must maintain square/isometric aspect ratios.
   - Distinct visual state for empty wells, positive controls, negative controls, blanks, and compound test wells.
   - Hover preview tooltip showing Well ID (`A01`), Compound Name, Concentration, and Raw/Fitted Value.

2. **Data Tables & Lists:**
   - Always use alternating row backgrounds or crisp borders (`divider` / `borderStrong`).
   - Monospace font (`JetBrains Mono`) for all numeric, date, and identifier columns.
   - Sticky table headers with distinct surface (`surfaceHeader`).

3. **Status Badges & Chips:**
   - Subtle tinted background (`wash(0.12, 0.18)`) with solid colored text/icon for high contrast.

4. **Forms & Inputs:**
   - Solid dark/light field container (`surfaceInput`) with subtle border.
   - Focus state: `brandAccent` border glow (`Style.focus`).
   - Clear validation feedback (inline message with `statusBad` / `statusOk`).

5. **Modals & Dialogs:**
   - Explicit action buttons: Primary action in `brandAccent` on the right; Secondary / Cancel on the left or ghost style.
   - Esc key closes non-destructive dialogs.
