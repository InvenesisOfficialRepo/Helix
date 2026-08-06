# Helix (Invenesis Master Hub) - Product Context

## Product Identity & Overview
- **Name:** Helix (Invenesis Master Hub)
- **Organization:** Invenesis
- **Type:** Desktop Scientific Workstation & Data Management Application (Qt6 / C++ / QML)
- **Primary Domain:** High-Throughput Screening (HTS), Electrophysiology (HiClamp/automated patch clamp), Microplate Operations, Compound & Inventory Tracking, and Automated Data Analysis / Publishing.

## Target Users & Environment
- **Primary Users:** Screening scientists, electrophysiologists, lab automation engineers, lab technicians, and data managers.
- **Operating Environment:** Laboratory workstations running Windows (and macOS), often side-by-side with automated liquid handlers (e.g. Tecan Freedom EVO / Fluent) and electrophysiology recording rigs.
- **Usage Context:** High cognitive load, long hours analyzing dense datasets, multi-monitor lab setups, fast microplate quadrant merging, quality control review, and database synchronization.

## Register & Design Lane
- **Lane:** **Product (Scientific Workstation)**
- **Priority Hierarchy:** 
  1. **Information Density & Legibility:** Maximizing usable screen real estate without clutter; dense tables, clear data grids, and tabular numbers.
  2. **Speed & Efficiency:** Instant feedback, zero lag, minimal clicks for repetitive operations (plate layout, compound registration, batch import).
  3. **Visual Clarity & Error Prevention:** Clear status indicators (Pass/Fail/Warn), explicit plate map coordinates, unambiguous destructive action prompts.

## Core Workflows & Modules
1. **Master Launcher Hub:** Centralized station launchpad for all specialized lab tools and workflows.
2. **Microplate Operations & Quadrant Merger:** 96-well to 384-well plate re-formatting, daughter plate generation, and plate layout assignment.
3. **Tecan Planner & Worklist Automation:** Visual layout maker, automated script/VBS generation, and liquid handling run coordination.
4. **Datapoint & Compound Manager:** Compound inventory, batch lifecycle tracking, solubility/dissolution logging, barcode scanning, and multi-user role management.
5. **Standalone & Batch Analysis Engine:** HiClamp trace curve fitting, dose-response calculations, QC metrics, and publication card export.
6. **Publishing & Wallboard Monitoring:** Real-time experiment status wallboards, PostgreSQL cloud/local sync, and automated reports.

## Voice, Tone & Personality
- **Precise & Scientific:** Clear scientific terminology, precise numerical displays with units, zero ambiguity.
- **Reliable & Professional:** Feels robust and sturdy, like precision lab instrumentation software.
- **Distraction-Free:** Clean visual hierarchy, subdued backgrounds, functional accenting with teal/emerald.

## Anti-References & Anti-Patterns to Avoid
- ❌ **Low-Density SaaS Spacing:** Huge empty cards, oversized padding, or single-column phone-like layouts.
- ❌ **Decorative Fluff:** Sluggish CSS/QML animations, flashy 3D gimmicks that delay user interaction, or non-functional gradients.
- ❌ **Ambiguous Icons:** Mystery meat navigation or generic icons without tooltips or labels.
- ❌ **Proportional Numbers for Scientific Data:** Never use proportional fonts for numeric data tables or plate coordinates (always use monospace/tabular figures like JetBrains Mono).
- ❌ **Hidden State:** Never hide crucial experiment or sync statuses behind deep sub-menus.
